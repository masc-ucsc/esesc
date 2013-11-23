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

#include "SamplerPeriodic.h"
#include "EmulInterface.h"
#include "SescConf.h"
#include "BootLoader.h"
#include "TaskHandler.h"
#include "MemObj.h"
#include "GProcessor.h"
#include "GMemorySystem.h"

#include <inttypes.h>

int32_t SamplerPeriodic::PerfSampleLeftForTemp = 0;
GStatsMax *SamplerPeriodic::progressedTime = 0;

SamplerPeriodic::SamplerPeriodic(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : SamplerBase(iname,section,  emu, fid)
  /* SamplerPeriodic constructor {{{1 */
{
  if (progressedTime==0)
      progressedTime = new GStatsMax("progressedTime");

  dsync      = new GStatsCntr("S(%u):dsync", fid);

  nInstRabbit = static_cast<uint64_t>(SescConf->getDouble(section,"nInstRabbit"));
  nInstWarmup = static_cast<uint64_t>(SescConf->getDouble(section,"nInstWarmup"));
  nInstDetail = static_cast<uint64_t>(SescConf->getDouble(section,"nInstDetail"));
  nInstTiming = static_cast<uint64_t>(SescConf->getDouble(section,"nInstTiming"));
  if (fid != 0) // first thread might need a different skip
    nInstSkip   = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkipThreads"));

  maxnsTicks  = static_cast<uint64_t>(SescConf->getDouble(section,"maxnsTicks"));
  nInstMax    = static_cast<uint64_t>(SescConf->getDouble(section,"nInstMax"));
  TempToPerfRatio    =  static_cast<uint64_t>(SescConf->getDouble(section,"TPR"));

  if (nInstWarmup>0) {
    sequence_mode.push_back(EmuWarmup);
    sequence_size.push_back(nInstWarmup);
    next2EmuTiming = EmuWarmup;
  }

  if (nInstDetail>0) {
    sequence_mode.push_back(EmuDetail);
    sequence_size.push_back(nInstDetail);
    if (nInstWarmup == 0)
      next2EmuTiming = EmuDetail;
  }
  if (nInstTiming>0) {
    sequence_mode.push_back(EmuTiming);
    sequence_size.push_back(nInstTiming);
    if (nInstWarmup == 0 && nInstDetail == 0)
      next2EmuTiming = EmuTiming;
  }

  // Rabbit last because we start with nInstSkip
  if (nInstRabbit>0) {
    sequence_mode.push_back(EmuRabbit);
    sequence_size.push_back(nInstRabbit);
    next2EmuTiming = EmuRabbit;
  }

  if (nInstDetail != 0 && nInstTiming != 0 && 
      nInstRabbit == 0 && nInstWarmup == 0){
    printf("Error: Timing-Detail only cycle is not supported in Periodic sampler.\n");
    exit(0);
  }


  //nInstForcedDetail = nInstDetail==0? nInstTiming/5:nInstDetail;
  nInstForcedDetail = 1000;

  if (sequence_mode.empty()) {
    MSG("ERROR: SamplerPeriodic needs at least one valid interval");
    exit(-2);
  }

  sequence_pos = 0;
  intervalRatio = 1.0;


  if (emul->cputype != GPU) {
    {
      GProcessor *gproc = TaskHandler::getSimu(fid);
      MemObj *mobj      =  gproc->getMemorySystem()->getDL1();
      DL1 = mobj;
    }
  }

  nextSwitch = nInstSkip;
  if (nInstSkip)
    startInit(fid);


  validP  = 0;
  headPtr = 0;

  doIPCPred  = SescConf->getBool(section, "doPowPrediction"); 
  if (doIPCPred) { 
    cpiHistSize =SescConf->getInt(section, "PowPredictionHist"); 
  }else{
    cpiHistSize = 1;
  }
  cpiHist.resize(cpiHistSize);

  lastMode = EmuInit;
  std::cout << "Sampler: TBS, R:" << nInstRabbit
            << ", W:"             << nInstWarmup
            << ", D:"             << nInstDetail
            << ", T:"             << nInstTiming
            << endl;

  std::cout << "Sampler: TBS, nInstMax:" << nInstMax
            << ", nInstSkip:"            << nInstSkip
            << ", maxnsTicks:"           << maxnsTicks
            << endl;

}
/* }}} */

SamplerPeriodic::~SamplerPeriodic() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

void SamplerPeriodic::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, FlowID fid, char op, uint64_t icount, void *env)
  /* main qemu/gpu/tracer/... entry point {{{1 */
{
  I(fid < emul->getNumEmuls());
  if(likely(!execute(fid, icount)))
    return; // QEMU can still send a few additional instructions (emul should stop soon)

  I(insn);

  // process the current sample mode
  if (nextSwitch>totalnInst) {
    if (mode == EmuRabbit || mode == EmuInit)
      return;
    if (mode == EmuDetail || mode == EmuTiming) {
      emul->queueInstruction(insn,pc,addr,data, (op&0xc0) /* thumb */ ,fid, env, getStatsFlag());
      return;
    }
    I(mode == EmuWarmup);
    doWarmupOpAddrData(op, addr, data);
    return;
  }

  // We did enough
  if (isItDone())
    return;
 

  // We are not done yet though. Look for the new mode
  I(nextSwitch <= totalnInst);
  coordinateWithOthersAndNextMode(fid);
  I(mode == next_mode); //it was a detailed sync
}
/* }}} */

void SamplerPeriodic::updateCPI()
  /* extract cpi of last sample interval {{{1 */
{
  if(lastMode == EmuTiming) {
    updateCPIHist();
    loadPredCPI();
  }
}
/* }}} */

void SamplerPeriodic::updateCPIHist() 
{
  insertNewCPI();
  computeEstCPI();
}

void SamplerPeriodic::computeEstCPI() {
  estCPI = 0;

  for(size_t i=0; i< cpiHistSize;i++){
    estCPI           += cpiHist[i];
  }

  // circular 
  if(++headPtr == cpiHistSize)
    headPtr = 0;

  float d = cpiHistSize;
  if (validP<cpiHistSize){
    validP++;
    d = static_cast<float>(validP);
  }
  estCPI /= d;

  if (estCPI > 10) {
    I(0);
    estCPI = 10;
  }
}

void SamplerPeriodic::insertNewCPI() {
  cpiHist[headPtr] = getMeaCPI(); 
}

void SamplerPeriodic::loadPredCPI()
{
  if (!BootLoader::getPowerModelPtr()->predictionStatus())
    estCPI = getMeaCPI();

  updateIntervalRatio();
}


void SamplerPeriodic::nextMode(bool rotate, FlowID fid, EmuMode mod) {
  winnerFid = 999999;
  if (rotate){
    totalnInstForcedDetail = 0;

    I(next_mode != EmuInit);
    setMode(next_mode, fid);
    I(mode == next_mode);

    winnerFid = fid;

    nextSwitch       = nextSwitch + sequence_size[sequence_pos]*intervalRatio;

    // a hack for stacking validation
    if (nextSwitch < totalnInst)
      nextSwitch = totalnInst + sequence_size[sequence_pos];

  }else{ //SET_MODE
    setMode(mod, fid);
    I(mode == mod);

    switch (mod){
      case EmuRabbit:
        if (totalnInstForcedDetail <= nInstRabbit){
          nextSwitch      += nInstRabbit*intervalRatio;
          nextSwitch      -= (next2EmuTiming == EmuRabbit) ? totalnInstForcedDetail : 0;
        }
        sequence_pos = 3;
        break;
      case EmuWarmup:
        if (totalnInstForcedDetail <= nInstWarmup){
          nextSwitch      += nInstWarmup*intervalRatio;
          nextSwitch      -= (next2EmuTiming == EmuWarmup) ? totalnInstForcedDetail : 0;
        }
        sequence_pos = 0;
        break;
      case EmuDetail:
        nextSwitch      += nInstForcedDetail;;
        totalnInstForcedDetail += nInstForcedDetail;
        sequence_pos = 1;
        break;
      case EmuTiming:
        nextSwitch      += nInstTiming*intervalRatio;
        sequence_pos = 2;
        //intervalRatio = 1 + static_cast<float>totalnInstForcedDetail/static_cast<float>nInstTiming;
        break;
      default:
        I(0);
    }
  }
}


void SamplerPeriodic::doPWTH(FlowID fid) {
  uint64_t mytime = 0.0;
  if (PerfSampleLeftForTemp == 0){

    mytime   = getLocalTime();
    I(mytime > lastTime);

    uint64_t ti = static_cast<uint64_t> (mytime - lastTime);
    I(ti);

    if (ti == 0) {
      PerfSampleLeftForTemp = TempToPerfRatio;
      return;
    }

    TaskHandler::syncStats();

    //printf ("fid: %d ", fid);
    //std::cout<<"mode "<<lastMode<<" Timeinterval "<<ti<<" mytime "<<mytime<<" last time "<<lastTime<<"freq:"<<getFreq()<<"\n";  

    BootLoader::getPowerModelPtr()->setSamplingRatio(getSamplingRatio()); 
    BootLoader::getPowerModelPtr()->calcStats(ti, !(lastMode == EmuTiming), fid); 
    BootLoader::getPowerModelPtr()->sescThermWrapper->sesctherm.updateMetrics(ti);  
    PerfSampleLeftForTemp = TempToPerfRatio;
    lastTime = mytime;
  }
  if (PerfSampleLeftForTemp) PerfSampleLeftForTemp--;

  endSimSiged = (mytime>=maxnsTicks)?1:0;
}


void SamplerPeriodic::syncTimeAndTimingModes(FlowID fid) {
  if(mode == EmuTiming /*&& next_mode != EmuTiming*/)
  {
    clearInTiming(fid);
    lastMode = mode;
    syncTimeAndWaitForOthers(fid);
    return;
  }

  I(mode != EmuTiming || next_mode == EmuTiming);

  if( mode == EmuDetail && lastMode == EmuTiming){
    syncTimeAndWaitForOthers(fid);
    return;
  }else{
    lastMode = mode;
    syncTimeAndContinue(fid);

    if (next_mode == EmuRabbit){
      setModeNativeRabbit();
    }

    I(!(doPower && fid == winnerFid && lastMode == EmuTiming));
    return;
  }
  I(0); // If othersStillInTiming is set and the mode is Rabbit or Warmup, there is a problem!

}


void SamplerPeriodic::syncTimeAndSamples(FlowID fid) {
  if( mode == EmuDetail && lastMode == EmuTiming) {
    lastMode = mode;
    syncTimeAndFinishWaitingForOthers(fid);
    return;
  }

  lastMode = mode;
  syncTimeAndContinue(fid);
  if (next_mode == EmuRabbit){
    setModeNativeRabbit();
  }
  if( mode == next2EmuTiming ){
    clearInTiming(fid);
    updatenSamples(); 
  }

}

void SamplerPeriodic::coordinateWithOthersAndNextMode(FlowID fid) {

  fetchNextMode();

#ifdef TBS
  if (othersStillInTiming(fid))
#else
  if(false)
#endif
  {
    syncTimeAndTimingModes(fid);
  } else{
    syncTimeAndSamples(fid);

    if (lastMode == EmuTiming) { 
      progressedTime->sample(getLocalTime());
      endSimSiged = (getLocalTime()>=maxnsTicks)?1:0;
      //std::cout<<"mode "<<lastMode<<" fid:"<<sFid<<" sID:"<<nSamples[sFid]<<" totalSamples:"<<totalnSamples<<" time:"<<getLocalTime()<<"\n";  
    }

    if (doPower && fid == winnerFid) {
      if (lastMode == EmuTiming) { // timing is going to be over
        doPWTH(fid);
      }
    }

    return ;
  }
}

bool SamplerPeriodic::isItDone() {
  if (endSimSiged) {
    markDone();
    return true;
  }
  return false;
}


void SamplerPeriodic::doWarmupOpAddrData(char op, uint64_t addr, uint64_t data) {
    if(addr) {
      if (emul->cputype == GPU) {
        I(0); // GPU uses its own shared sampler
      } else {
        if ( (op&0x3F) == 1)
          DL1->ffread(addr,data);
        else if ( (op&0x3F) == 2)
          DL1->ffwrite(addr,data);
      }
    }
}


void SamplerPeriodic::updateIntervalRatio() {
#ifdef TBS
  intervalRatio = 1.0/estCPI;
#else
  intervalRatio = 1.0;
#endif
}

void SamplerPeriodic::syncTimeAndWaitForOthers(FlowID fid) {
      nextMode(SET_MODE, fid, EmuDetail);
      updateCPI();
      updateLocalTime();
      dsync->inc();
}

void SamplerPeriodic::syncTimeAndContinue(FlowID fid) {
      nextMode(ROTATE, fid);
      updateCPI();
      updateLocalTime();
}

void SamplerPeriodic::syncTimeAndFinishWaitingForOthers(FlowID fid) {
      nextMode(SET_MODE, fid, next2EmuTiming);
      updateLocalTime();
}


void SamplerPeriodic::dumpCPI() {
  static bool first = true;
  static FILE *log;
  if (first) {
    log = genReportFileNameAndOpen("cpi_");
    first = false;
  }

  fprintf(log, "fid%d %g\n", sFid, getMeaCPI());
}


void SamplerPeriodic::dumpTime() {
  static bool first = true;
  static FILE *log;
  if (first) {
    log = genReportFileNameAndOpen("time_");
    first = false;
  }

  for(size_t i=0; i<emul->getNumFlows(); i++) {
    if (isActive(mapLid(i)))
      fprintf(log, "%lu ", getLocalTime(mapLid(i)));
    else
      fprintf(log, "%d ", 0);
  }

  fprintf(log, "\n");
}


void SamplerPeriodic::dumpThreadProgressedTime(FlowID fid) {
  if (getKeepStatsFlag()) {
    threadProgressedTime = new GStatsMax("P(%d)_progressedTime", fid);
    threadProgressedTime->sample(getLocalTime(fid));
  }
}
