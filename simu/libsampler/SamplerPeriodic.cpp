/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan K.Ardestani


This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


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
#if DEBUG
GStatsCntr*SamplerPeriodic::ipcl01 = 0;
GStatsCntr*SamplerPeriodic::ipcl03 = 0;
GStatsCntr*SamplerPeriodic::ipcl05 = 0;
GStatsCntr*SamplerPeriodic::ipcg05 = 0;
#endif

SamplerPeriodic::SamplerPeriodic(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : SamplerBase(iname,section,  emu, fid)
  /* SamplerPeriodic constructor {{{1 */
{
  if (progressedTime==0)
      progressedTime = new GStatsMax("progressedTime");

#if DEBUG
  if (ipcl01 == 0)
    ipcl01      = new GStatsCntr("ipcl01");
  if (ipcl03 == 0)
    ipcl03      = new GStatsCntr("ipcl03");
  if (ipcl05 == 0)
    ipcl05      = new GStatsCntr("ipcl05");
  if (ipcg05 == 0)
    ipcg05      = new GStatsCntr("ipcg05");
#endif

  dsync      = new GStatsCntr("S(%u):dsync", fid);

  nInstRabbit = static_cast<uint64_t>(SescConf->getDouble(section,"nInstRabbit"));
  nInstWarmup = static_cast<uint64_t>(SescConf->getDouble(section,"nInstWarmup"));
  nInstDetail = static_cast<uint64_t>(SescConf->getDouble(section,"nInstDetail"));
  nInstTiming = static_cast<uint64_t>(SescConf->getDouble(section,"nInstTiming"));
  if (fid != 0) // first thread might need a different skip
    nInstSkip   = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkipThreads"));

  maxnsTicks  = static_cast<uint64_t>(SescConf->getDouble(section,"maxnsTicks"));
#if DEBUG
  nSampleMax    = static_cast<uint64_t>(SescConf->getDouble(section,"nSampleMax"));
#else
  printf("\n\nIgnoring nSampleMax in Release Mode. Only maxnsTicks is used in TBS.\n\n");
#endif
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
#if DEBUG
  updateIntervalRatioStats();
#endif

  //MSG("fid:%d intervalRatio:%f\n", sFid, intervalRatio);
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
    printf ("fid: %d ", fid);

    std::cout<<"mode "<<lastMode<<" Timeinterval "<<ti<<" mytime "<<mytime<<" last time "<<lastTime<<"freq:"<<getFreq()<<"\n";  
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

#if DEBUG
    if (doPower && fid == winnerFid && lastMode == EmuTiming) { 
      I(0); 
    }
#endif
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
#if DEBUG
  if (totalnSamples >= nSampleMax || endSimSiged) {
#else
  if (totalnInst >= nInstMax)
    markDone();
  if (endSimSiged) {
#endif
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

#if DEBUG
void SamplerPeriodic::updateIntervalRatioStats() {
  if (intervalRatio < 0.1)
    ipcl01->inc();
  else if (intervalRatio < 0.3)
    ipcl03->inc();
  else if (intervalRatio < 0.5)
    ipcl05->inc();
  else
    ipcg05->inc();
}
#endif

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
