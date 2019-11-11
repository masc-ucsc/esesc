// Contributed by Jose Renau
//                Sushant Kondguli
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "EmuSampler.h"
#include "EmulInterface.h"
#include "SescConf.h"

bool EmuSampler::roi_skip;

bool              cuda_go_ahead = false;
bool              MIMDmode      = false;
std::vector<bool> EmuSampler::done;
volatile bool     EmuSampler::terminated = false;
uint64_t *        EmuSampler::instPrev;
uint64_t *        EmuSampler::clockPrev;
uint64_t *        EmuSampler::fticksPrev;

float EmuSampler::turboRatio = 1.0;
extern long long int icount;

EmuSampler::EmuSampler(const char *iname, EmulInterface *emu, FlowID fid)
    /* EmuSampler constructor  */
    : nextSwitch(0)
    , name(strdup(iname))
    , sFid(fid)
    , emul(emu)
    , lastPhasenInst(0)
    , meauCPI(0.0)
    , restartRabbit(false) {
  clock_gettime(CLOCK_REALTIME, &startTime);

  const char *samp_sec = SescConf->getCharPtr(emu->getSection(), "sampler");
  nInstMax             = static_cast<uint64_t>(SescConf->getDouble(samp_sec, "nInstMax"));
  nInstSkip            = static_cast<uint64_t>(SescConf->getDouble(samp_sec, "nInstSkip"));
  if(fid != 0) { // first thread might need a different skip
    nInstSkip = static_cast<uint64_t>(SescConf->getDouble(samp_sec, "nInstSkipThreads"));
  }
  roi_skip = SescConf->getBool(samp_sec, "ROIOnly");

  if (roi_skip) {
    icount = 1ULL<<40; // start skipping fast
  }

  const char *sys_sec = SescConf->getCharPtr(emu->getSection(), "syscall");
  syscall_enable      = SescConf->getBool(sys_sec, "enable");
  syscall_generate    = SescConf->getBool(sys_sec, "generate");
  syscall_runtime     = SescConf->getBool(sys_sec, "runtime");
  if(!syscall_runtime && syscall_enable) {
    const char *fname = SescConf->getCharPtr(sys_sec, "file");
    if(syscall_generate)
      syscall_file = fopen(fname, "w");
    else
      syscall_file = fopen(fname, "r");
    if(syscall_file == 0) {
      MSG("ERROR: syscall could not open file %s", fname);
      exit(-3);
    }
  } else
    syscall_file = 0;

  mode      = EmuInit;
  next_mode = EmuRabbit;

  phasenInst = 0;
  totalnInst = 0;
  meaCPI     = 1.0;

  globalClock_Timing_prev = 0;

  static int sampler_count = 0;

  nSwitches = new GStatsCntr("S(%d):nSwitches", sampler_count);

  tusage[EmuInit]   = new GStatsCntr("S(%d):InitTime", sampler_count);
  tusage[EmuRabbit] = new GStatsCntr("S(%d):RabbitTime", sampler_count);
  tusage[EmuWarmup] = new GStatsCntr("S(%d):WarmupTime", sampler_count);
  tusage[EmuDetail] = new GStatsCntr("S(%d):DetailTime", sampler_count);
  tusage[EmuTiming] = new GStatsCntr("S(%d):TimingTime", sampler_count);

  iusage[EmuInit]   = new GStatsCntr("S(%d):InitInst", sampler_count);
  iusage[EmuRabbit] = new GStatsCntr("S(%d):RabbitInst", sampler_count);
  iusage[EmuWarmup] = new GStatsCntr("S(%d):WarmupInst", sampler_count);
  iusage[EmuDetail] = new GStatsCntr("S(%d):DetailInst", sampler_count);
  iusage[EmuTiming] = new GStatsCntr("S(%d):TimingInst", sampler_count);

  globalClock_Timing = new GStatsCntr("S(%d):globalClock_Timing", sampler_count);

  ipc  = new GStatsAvg("P(%d)_ipc", sampler_count);
  uipc = new GStatsAvg("P(%d)_uipc", sampler_count);
  char str[100];
  sprintf(str, "P(%d):nCommitted", fid);
  nCommitted = GStats::getRef(str);
  I(nCommitted);
  prevnCommitted = 0;

  sampler_count++;

  freq = SescConf->getDouble("technology", "frequency");

  stopJustCalled    = true; // implicit stop at the beginning
  calcCPIJustCalled = false;

  next = 2 * 1024 * 1024 + 7;

  pthread_mutex_init(&stop_lock, NULL);
  numFlow = SescConf->getRecordSize("", "cpuemul");
  if(done.size() != numFlow)
    done.resize(numFlow);

  instPrev   = new uint64_t[emul->getNumEmuls()];
  clockPrev  = new uint64_t[emul->getNumEmuls()];
  fticksPrev = new uint64_t[emul->getNumEmuls()];
  for(size_t i = 0; i < emul->getNumEmuls(); i++) {
    instPrev[i]   = 0;
    clockPrev[i]  = 0;
    fticksPrev[i] = 0;
  }
}
/*  */

void EmuSampler::setNextSwitch(uint64_t instNum) {
  if(instNum < nInstMax) {
    nextSwitch = instNum;
  } else {
    nextSwitch = nInstMax;
  }
}

bool EmuSampler::toggle_roi() {
  roi_skip       = !roi_skip;
  stopJustCalled = false;

  if(roi_skip) {
    MSG("### SamplerBase::toggle_roi() called: STOP Simulation");
    syncTime();
    //setNextSwitch(nInstSkip);
    //startRabbit(0);
  } else if(nInstSkip) {
    MSG("### SamplerBase::toggle_roi() called: START Simulation (nInstSkip!=0 -> restart Rabbit)");

    // totalnInst = 0; // reset
    setNextSwitch(nInstSkip);
    startRabbit(0);
  }else{
    MSG("### SamplerBase::toggle_roi() called: START Simulation (nInstSkip==0 -> restart last mode)");
  }

  return roi_skip;
}

EmuSampler::~EmuSampler()
/* Destructor  */
{
  if(syscall_file) {
    fclose(syscall_file);
    syscall_file = 0;
  }
  free((char *)name); // cast needed because name is const *
}
/*  */

void EmuSampler::setMode(EmuMode mod, FlowID fid)
/* Stop and start statistics for a given mode  */
{
  if(mode != mod) {
    nSwitches->inc();
  }
  stop();
  switch(mod) {
  case EmuRabbit:
    startRabbit(fid);
    break;
  case EmuWarmup:
    startWarmup(fid);
    break;
  case EmuDetail:
    startDetail(fid);
    break;
  case EmuTiming:
    startTiming(fid);
    break;
  default:
    I(0);
  }
}
/*  */

FlowID EmuSampler::getNumFlows()
// get number of flows
{
  return emul->getNumFlows();
}
//

void EmuSampler::beginTiming(EmuMode mod)
/* Start sampling a new mode  */
{
  // I(stopJustCalled);
  stopJustCalled = false;
  // phasenInst         = 0;
  mode = mod;

  syncTime();

  clock_gettime(CLOCK_REALTIME, &startTime);

  const char *mode2name[] = {"Init", "Rabbit", "Warmup", "Detail", "Timing", "InvalidValue"};
  GMSG(mode != mod, "Sampler %s: starting %s mode", name, mode2name[mod]);
}
/*  */

void EmuSampler::syncTime() {

  if(globalClock_Timing_prev == globalClock)
    return;

  struct timespec endTime;
  clock_gettime(CLOCK_REALTIME, &endTime);
  uint64_t usecs = endTime.tv_sec - startTime.tv_sec;
  usecs *= 1000000;
  usecs += (endTime.tv_nsec - startTime.tv_nsec) / 1000;

  startTime = endTime;
  tusage[mode]->add(usecs);
  iusage[mode]->add(phasenInst);

  // get the globalClock_Timing, it includes idle clock as well
  if(mode == EmuTiming) {
    // A bit complex swap because it is multithreaded
    Time_t gcct = globalClock;

    uint64_t cticks = gcct - globalClock_Timing_prev;
    globalClock_Timing->add(cticks);

    globalClock_Timing_prev = gcct;
  } else {
    globalClock_Timing_prev = globalClock;
  }

  calcCPI();

  // I(phasenInst); // There should be something executed (more likely)
  lastPhasenInst    = phasenInst;
  phasenInst        = 0;
  calcCPIJustCalled = false;
}

void EmuSampler::stop()
/* stop a given mode, and assign statistics  */
{
  //  if (terminated)
  //    return;

  if(totalnInst >= cuda_inst_skip)
    cuda_go_ahead = true;

  // MSG("Sampler:STOP");
  pthread_mutex_lock(&stop_lock);
  if(stopJustCalled) {
    pthread_mutex_unlock(&stop_lock);
    return;
  }

  syncTime();

  I(!stopJustCalled);
  stopJustCalled = true;

  pthread_mutex_unlock(&stop_lock);
}
/*  */

void EmuSampler::startInit(FlowID fid)
/* Start Init Mode : No timing or warmup, go as fast as possible  */
{
  // MSG("Sampler:STARTRABBIT");
  I(stopJustCalled);
  if(mode != EmuInit)
    emul->startRabbit(fid);
  beginTiming(EmuInit);
}
///

void EmuSampler::startRabbit(FlowID fid)
/* Start Rabbit Mode : No timing or warmup, go as fast as possible  */
{
  // MSG("Sampler:STARTRABBIT");
  // I(stopJustCalled);
  if(mode != EmuRabbit)
    emul->startRabbit(fid);
  beginTiming(EmuRabbit);
}
/*  */

void EmuSampler::startWarmup(FlowID fid)
/* Start Rabbit Mode : No timing but it has cache/bpred warmup  */
{
  // I(stopJustCalled);
  if(mode != EmuWarmup)
    emul->startWarmup(fid);
  beginTiming(EmuWarmup);
}
/*  */

void EmuSampler::startDetail(FlowID fid)
/* Start Rabbit Mode : Detailing modeling without no statistics gathering  */
{
  // I(stopJustCalled);
  if(mode != EmuDetail)
    emul->startDetail(fid);
  beginTiming(EmuDetail);
}
/*  */

void EmuSampler::startTiming(FlowID fid)
/* Start Timing Mode : full timing model  */
{
  // MSG("Sampler:STARTTIMING");
  // I(stopJustCalled);

  // if (mode!=EmuTiming)
  emul->startTiming(fid);

  beginTiming(EmuTiming);
}
/*  */

bool EmuSampler::execute(FlowID fid, uint64_t icount)
/* called for every instruction that qemu/gpu executes  */
{

  GI(mode == EmuTiming, icount == 1);

  AtomicAdd(&phasenInst, icount);
  AtomicAdd(&totalnInst, icount);

  if(likely(totalnInst <= next)) // This is an likely taken branch, pass the info to gcc
    return !done[fid];

  next += 2 * 1024 * 1024; // Note, this is racy code. We can miss a rwdt from time to time, but who cares?

  if(done[fid]) {
    fprintf(stderr, "X");
    fflush(stderr);
    {
      // We can not really, hold the thread, because it can hold locks inside qemu
      fprintf(stderr, "X[%d]", fid);
      usleep(10000);
    }
  } else {
    if(mode == EmuRabbit) {
      if(!roi_skip)
        fprintf(stderr, "r%d", fid);
    } else if(mode == EmuWarmup)
      fprintf(stderr, "w%d", fid);
    else if(mode == EmuDetail)
      fprintf(stderr, "d%d", fid);
    else if(mode == EmuTiming)
      fprintf(stderr, "t%d", fid);
    else if(mode == EmuInit)
      fprintf(stderr, ">%d", fid);
    else
      fprintf(stderr, "?%d", fid);
  }

  // An adjustment when adding more than one instruction. Keeps the sample prints valid
  // Repeat: Note, this is racy code. We can miss a rwdt from time to time, but who cares?
  if(icount > 1) {
    while(next < totalnInst) {
      if(mode == EmuRabbit) {
        if(!roi_skip)
          fprintf(stderr, "r%d", fid);
      }
      next += 2 * 1024 * 1024;
    }
    next = totalnInst;
  }

  return !done[fid];
}
/*  */

void EmuSampler::markDone()
/* indicate the sampler that a flow is done for good  */
{
  uint32_t endfid = emul->getNumEmuls() - 1;
  I(!stopJustCalled || endfid == 0);

  I(done.size() > endfid);
  for(size_t i = 0; i < endfid; i++) {
    done[i] = true;
  }

  phasenInst = 0;
  //  mode       = EmuInit;
  terminate();
}
/*  */

void EmuSampler::syscall(uint32_t num, uint64_t usecs, FlowID fid)
/* Create an syscall instruction and inject in the pipeline  */
{
  if(!syscall_enable)
    return;

  Time_t nsticks = static_cast<uint64_t>(((double)usecs / freq) * 1.0e12);

  if(syscall_generate) {
    // Generate
    uint32_t cycles = static_cast<uint32_t>(nsticks);
    I(syscall_file);
    fprintf(syscall_file, "%u %u\n", num, cycles);
  } else if(!syscall_runtime) {
    // use the file
    I(syscall_file);
    uint32_t num2;
    uint32_t cycles2;
    int      ret = fscanf(syscall_file, "%u %u\n", &num2, &cycles2);
    if(num2 != num && ret != 2) {
      MSG("Error: the Syscall trace file does not seem consistent with the execution\n");
    } else {
      nsticks = cycles2;
    }
  }

  if(mode != EmuTiming)
    return;

  emul->syscall(num, nsticks, fid);
}
/*  */

void EmuSampler::calcCPI()
/* calculates cpi for the last EmuTiming mode  */
{
  if(mode != EmuTiming)
    return;

  if(terminated)
    return;

  I(calcCPIJustCalled == false);
  I(!stopJustCalled);
  calcCPIJustCalled = true;

  // get instruction count
  uint64_t timingInst = iusage[EmuTiming]->getSamples();
  uint64_t instCount  = timingInst - instPrev[sFid];

  char str[255];
  sprintf(str, "P(%d):clockTicks", sFid); // FIXME: needs mapping
  GStats *gref = 0;
  gref         = GStats::getRef(str);
  I(gref);
  // fprintf(stderr, "\n\nCalcCPI: %s->getSamples returns %lld (previously : %lld)\n\n",str,gref->getSamples(),clockPrev[sFid]);
  uint64_t cticks        = gref->getSamples();
  uint64_t clockInterval = cticks - clockPrev[sFid] + 1;

  if(instCount < 100 || clockInterval < 100) // To avoid too frequent sampling errors
    return;

  instPrev[sFid]  = timingInst;
  clockPrev[sFid] = cticks;

  // Get freezed cycles due to thermal throttling
  sprintf(str, "P(%d):nFreeze", sFid); // FIXME: needs mapping
  gref = 0;
  gref = GStats::getRef(str);
  I(gref);
  uint64_t fticks = gref->getSamples();
  // fprintf(stderr,"\n\nCalcCPI: %s->getSamples returns %lld\n\n",str,gref->getSamples());
  uint64_t fticksInterval = fticks - fticksPrev[sFid] + 1;
  fticksPrev[sFid]        = fticks;

  uint64_t uInstCount = nCommitted->getSamples() - prevnCommitted;
  prevnCommitted      = nCommitted->getSamples();

  uint64_t adjustedClock = clockInterval - fticksInterval;

  float newCPI = static_cast<float>(adjustedClock) / static_cast<float>(instCount);
  // newuCPI should be used, but as the sampler, we can only scale Inst, not uInst.
  // float newCPI  =  static_cast<float>(clockInterval)/static_cast<float>(uInstCount);
  I(newCPI > 0);

  /*
  //printf("InstCnt:%ld, prevInst:%ld, clock:%ld, ipc:%f\n", instCount, instPrev, clockInterval,
  static_cast<float>(instCount)/static_cast<float>(clockInterval)); if (newCPI <= 0){ fprintf(stderr,"newCPI = %f, clockInterval =
  %lld, adjustedClock = %lld, uInstCount = %lld",newCPI,clockInterval,adjustedClock, uInstCount);
  }
  */

  float newipc  = static_cast<float>(instCount) / static_cast<float>(adjustedClock);
  float newuipc = static_cast<float>(uInstCount) / static_cast<float>(adjustedClock);
  uipc->sample(100 * newuipc, true);
  ipc->sample(100 * newipc, true);

  // I(newCPI<=4);
  if(newCPI > 5) {
    // newCPI = 5.0;
    // return;
  }
  // meaCPI = newCPI;
  meaCPI  = newCPI;
  meauCPI = 1.0 / newuipc;
}
/*  */
