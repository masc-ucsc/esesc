// Contributed by Jose Renau
//                Ehsan K.Ardestani
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

#include "SamplerBase.h"
#include "BootLoader.h"
#include "EmulInterface.h"
#include "GMemorySystem.h"
#include "GProcessor.h"
#include "MemObj.h"
#include "Report.h"
#include "SescConf.h"

#include <iostream>

uint64_t SamplerBase::lastTime           = 0;
bool     SamplerBase::justResumed[128]   = {false};
bool     SamplerBase::finished[128]      = {true};
uint64_t SamplerBase::pastGlobalTicks    = 0;
uint64_t SamplerBase::gpuEstimatedCycles = 0;

uint64_t cuda_inst_skip;

GStatsMax *SamplerBase::progressedTime = 0;

SamplerBase::SamplerBase(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
    : EmuSampler(iname, emu, fid)
    , nInstForcedDetail(0)
    , gpuSampledRatio(0)
    , validP(0)
    , headPtr(0)
/* SamplerBase constructor {{{1 */
{
  if(progressedTime == NULL)
    progressedTime = new GStatsMax("progressedTime");

  nInstRabbit = static_cast<uint64_t>(SescConf->getDouble(section, "nInstRabbit"));
  nInstWarmup = static_cast<uint64_t>(SescConf->getDouble(section, "nInstWarmup"));
  nInstDetail = static_cast<uint64_t>(SescConf->getDouble(section, "nInstDetail"));
  nInstTiming = static_cast<uint64_t>(SescConf->getDouble(section, "nInstTiming"));

  if(fid != 0) { // first thread might need a different skip
    cuda_inst_skip = nInstSkip;
  }

  maxnsTime = static_cast<uint64_t>(SescConf->getDouble(section, "maxnsTime"));
  SescConf->isBetween(section, "maxnsTime", 1, 1e12);

  // Rabbit first because we start with nInstSkip
  if(nInstRabbit > 0) {
    sequence_mode.push_back(EmuRabbit);
    sequence_size.push_back(nInstRabbit);
    next2EmuTiming = EmuRabbit;
  }

  if(nInstWarmup > 0) {
    sequence_mode.push_back(EmuWarmup);
    sequence_size.push_back(nInstWarmup);
    next2EmuTiming = EmuWarmup;
  }
  if(nInstDetail > 0) {
    sequence_mode.push_back(EmuDetail);
    sequence_size.push_back(nInstDetail);
    next2EmuTiming = EmuDetail;
  }
  if(nInstTiming > 0) {
    sequence_mode.push_back(EmuTiming);
    sequence_size.push_back(nInstTiming);
    next2EmuTiming = EmuTiming;
  }

  if(sequence_mode.empty()) {
    MSG("ERROR: SamplerSMARTS needs at least one valid interval");
    exit(-2);
  }

  sequence_pos = 0;

  const char *pwrsection = SescConf->getCharPtr("", "pwrmodel", 0);

  freq    = SescConf->getDouble("technology", "frequency");
  doPower = SescConf->getBool(pwrsection, "doPower", 0);
  if(doPower) {
    doTherm            = SescConf->getBool(pwrsection, "doTherm", 0);
    pwr_updateInterval = static_cast<uint64_t>(SescConf->getDouble("pwrmodel", "updateInterval"));
  } else {
    doTherm            = false;
    pwr_updateInterval = 0;
  }

  estCPI = 1.0;

  doIPCPred   = SescConf->getBool(section, "doPowPrediction");
  cpiHistSize = static_cast<uint32_t>(SescConf->getDouble(section, "PowPredictionHist"));
  cpiHist.resize(cpiHistSize);

  first = true;
  for(unsigned int i = 0; i < cpiHistSize; i++)
    cpiHist.push_back(1.0);

  pthread_mutex_init(&mode_lock, NULL);
  lastGlobalClock = 0;

  GProcessor *gproc = TaskHandler::getSimu(fid);
  MemObj *    mobj  = gproc->getMemorySystem()->getDL1();
  DL1               = mobj;

  double ninst_d = SescConf->getDouble(section, "nInstDetail");
  double ninst_t = SescConf->getDouble(section, "nInstTiming");
  double ninst_r = SescConf->getDouble(section, "nInstRabbit");
  double ninst_w = SescConf->getDouble(section, "nInstWarmup");
  dt_ratio       = (ninst_t + ninst_d + ninst_w + ninst_r) / (ninst_t + 1);

  lastMode = EmuInit;

  last_addtime = 0;
}
/* }}} */

void SamplerBase::doWarmupOpAddr(InstOpcode op, uint64_t addr) {
  // {{{1 update cache stats when in warmup mode
  if(addr == 0)
    return;

  I(mode == EmuWarmup);
  I(emul->cputype != GPU);

#if 0
	if ( op == iLALU_LD)
		DL1->ffread(addr);
	else if (op == iSALU_ST)
		DL1->ffwrite(addr);
#endif
}
// 1}}}

bool SamplerBase::callPowerModel(FlowID fid)
// {{{1 Check if it's time to call Power/Thermal Model
{
  if(!doPower)
    return false;

  // Update metrics at each power pwr_updateInterval, now the same for all threads
  static std::vector<uint64_t> totalnInstPrev(emul->getNumFlows());

  uint64_t nPassedInst = totalnInst - totalnInstPrev[fid];
  if(nPassedInst >= pwr_updateInterval) {
    uint64_t mytime = getTime();
    uint64_t ti     = mytime - lastTime;
    I(ti > 0);
    totalnInstPrev[fid] = totalnInst;
    lastTime            = mytime;
    if(first) {
      first = false;
      return false;
    }

    BootLoader::getPowerModelPtr()->calcStats(ti, !(mode == EmuTiming), fid);

    return true;
  }
  return false;
}
// }}}

SamplerBase::~SamplerBase()
/* DestructorRabbit {{{1 */
{
  // Free name, but who cares
}
/* }}} */

uint64_t SamplerBase::getTime()
/* time in ns since the beginning {{{1 */
{
  // FIXME: currently it is per thread
  // I(phasenInst==0);

  // FIXME: try to use core stats nInst and clockTicks to get CPI
  double cpi2    = globalClock_Timing->getDouble() / (1 + iusage[EmuTiming]->getDouble());
  double addtime = cpi2 * totalnInst;
  addtime        = addtime * (1e9 / getFreq());

  if(addtime <= (last_addtime + 20000))
    addtime = last_addtime + 20000; // To provide a monotonically increasing clock in rabbit mode (20us steps)

  last_addtime = addtime;

  // MSG("cpi=%g/%g = %g ; cpi*totalninst(%lld)/freq = %g",globalClock_Timing->getDouble(), iusage[EmuTiming]->getDouble(), cpi2,
  // totalnInst, addtime);
  return static_cast<uint64_t>(addtime);
}
/* }}} */

void SamplerBase::pauseThread(FlowID fid) {
  TaskHandler::pauseThread(fid);
}

FlowID SamplerBase::getFid(FlowID last_fid) {
  return (emul->getFid(last_fid));
}

FlowID SamplerBase::resumeThread(FlowID uid, FlowID last_fid) {
  FlowID fid       = TaskHandler::resumeThread(uid, last_fid);
  justResumed[fid] = true;
  finished[fid]    = false;
  return fid;
}

FlowID SamplerBase::resumeThread(FlowID uid) {
  return (TaskHandler::resumeThread(uid));
}

void SamplerBase::terminate() {
  terminated = true;
  progressedTime->sample(getTime(), true);
  TaskHandler::terminate();
}

bool SamplerBase::isActive(FlowID fid) {
  // printf("s");
  return TaskHandler::isActive(fid);
}

void SamplerBase::getGPUCycles(FlowID fid, float ratio) {

  char str[255];
  sprintf(str, "P(%u):clockTicks", getsFid());
  GStats *gref = 0;
  gref         = GStats::getRef(str);
  I(gref);
  uint64_t cticks = gref->getSamples(); // clockTicks
  if(doPower)
    gpuEstimatedCycles = cticks * (ratio);
  else
    gpuEstimatedCycles = cticks;

  gpuSampledRatio = ratio;
}

bool SamplerBase::allDone() {
  progressedTime->sample(getTime(), true);
  for(size_t i = 0; i < emul->getNumFlows(); i++) {
    if(!finished[i])
      return false;
  }
  return true;
}

void SamplerBase::markThisDone(FlowID fid) {
  if(!finished[fid]) {
    finished[fid] = true;
    printf("fid %d finished, waiting for the rest...\n", fid);
  }
}

FILE *SamplerBase::genReportFileNameAndOpen(const char *str) {
  FILE *fp;
  char *fname = (char *)alloca(1023);
  strcpy(fname, str);
  strcat(fname, Report::getNameID());
  fp = fopen(fname, "w");
  if(fp == 0) {
    GMSG(fp == 0, "ERROR: could not open file \"%s\"", fname);
    exit(-1);
  }

  return fp;
}

void SamplerBase::fetchNextMode() {
  if(roi_skip) {
    next_mode = EmuRabbit;
    return;
  }

  sequence_pos++;
  if(sequence_pos >= sequence_mode.size())
    sequence_pos = 0;
  next_mode = sequence_mode[sequence_pos];
}
