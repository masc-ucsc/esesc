// Contributed by Jose Renau
//                Ehsan K.Ardestani
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

#include <iostream>

#include "BootLoader.h"
#include "EmulInterface.h"
#include "GMemorySystem.h"
#include "GProcessor.h"
#include "MemObj.h"
#include "SamplerPeriodic.h"
#include "SescConf.h"
#include "TaskHandler.h"

#include <inttypes.h>

int32_t SamplerPeriodic::PerfSampleLeftForTemp = 0;

SamplerPeriodic::SamplerPeriodic(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
    : SamplerBase(iname, section, emu, fid)
    , totalnInstForcedDetail(0)
    , winnerFid(999999)
/* SamplerPeriodic constructor {{{1 */
{
  dsync = new GStatsCntr("S(%u):dsync", fid);

  TempToPerfRatio = static_cast<uint64_t>(SescConf->getDouble(section, "TempToPerfRatio"));

  if(nInstDetail != 0 && nInstTiming != 0 && nInstRabbit == 0 && nInstWarmup == 0) {
    printf("Error: Timing-Detail only cycle is not supported in Periodic sampler.\n");
    exit(0);
  }

  nInstForcedDetail = 1000;

  if(sequence_mode.empty()) {
    MSG("ERROR: SamplerPeriodic needs at least one valid interval");
    exit(-2);
  }

  sequence_pos  = 0;
  intervalRatio = 1.0;

  if(emul->cputype != GPU) {
    {
      GProcessor *gproc = TaskHandler::getSimu(fid);
      MemObj *    mobj  = gproc->getMemorySystem()->getDL1();
      DL1               = mobj;
    }
  }

  setNextSwitch(nInstSkip);
  if(nInstSkip)
    startInit(fid);

  validP  = 0;
  headPtr = 0;

  lastMode = EmuInit;
}
/* }}} */

SamplerPeriodic::~SamplerPeriodic()
/* DestructorRabbit {{{1 */
{
  // Free name, but who cares
}
/* }}} */

uint64_t SamplerPeriodic::queue(uint64_t pc, uint64_t addr, uint64_t data, FlowID fid, char op, int src1, int src2, int dest,
                                int dest2, uint64_t data2)
/* main qemu/gpu/tracer/... entry point {{{1 */
{
  I(fid < emul->getNumEmuls());
  if(likely(!execute(fid, 1)))
    return 0; // QEMU can still send a few additional instructions (emul should stop soon)

  I(!done[fid]);

  // process the current sample mode
  if(getNextSwitch() > totalnInst) {
    if(mode == EmuRabbit || mode == EmuInit) {
      uint64_t rabbitInst = getNextSwitch() - totalnInst;
      execute(fid, rabbitInst);
      return rabbitInst;
    }
    if(mode == EmuDetail || mode == EmuTiming) {
      emul->queueInstruction(pc, addr, data, op, fid, src1, src2, dest, dest2, getStatsFlag(), data2);
      return 0;
    }
    I(mode == EmuWarmup);
    doWarmupOpAddr(static_cast<InstOpcode>(op), addr);
    return 0;
  }

#if 0
  // We did enough
  if (mode == EmuTiming)
    if (isItDone(fid))
      return;
#endif

  // We are not done yet though. Look for the new mode
  I(getNextSwitch() <= totalnInst);
  coordinateWithOthersAndNextMode(fid);
  I(mode == next_mode); // it was a detailed sync
  return 0;
}
/* }}} */

void SamplerPeriodic::updateCPI(FlowID fid)
/* extract cpi of last sample interval {{{1 */
{
  if(lastMode != EmuTiming)
    return;

  updateCPIHist();
  loadPredCPI();
}
/* }}} */

void SamplerPeriodic::updateCPIHist() {
  insertNewCPI();
  computeEstCPI();
}

void SamplerPeriodic::computeEstCPI() {
  estCPI = 0;

  for(size_t i = 0; i < cpiHistSize; i++) {
    estCPI += cpiHist[i];
  }

  // circular
  if(++headPtr == cpiHistSize)
    headPtr = 0;

  float d = cpiHistSize;
  if(validP < cpiHistSize) {
    validP++;
    d = static_cast<float>(validP);
  }
  estCPI /= d;

  if(estCPI > 5 || estCPI < 0.2) {
    // Strange, use the global CPI for a bit
    double cpi2 = globalClock_Timing->getDouble() / (1 + iusage[EmuTiming]->getDouble());
    MSG("estCPI out of range %g, using %g", estCPI, cpi2);
    estCPI = cpi2;
  }
#if 0
  double cpi2 = globalClock_Timing->getDouble() / (1+iusage[EmuTiming]->getDouble());
  MSG("fid %d: estCPI=%g gcpi=%g",sFid,estCPI,cpi2);
#endif
}

void SamplerPeriodic::insertNewCPI() {
  cpiHist[headPtr] = getMeaCPI();
}

void SamplerPeriodic::loadPredCPI() {
  if(!BootLoader::getPowerModelPtr()->predictionStatus())
    estCPI = getMeaCPI();

  updateIntervalRatio();
}

void SamplerPeriodic::nextMode(bool rotate, FlowID fid, EmuMode mod) {
  winnerFid = 999999;
  if(rotate) {
    totalnInstForcedDetail = 0;

    I(next_mode != EmuInit);
    setMode(next_mode, fid);
    I(mode == next_mode);

    winnerFid = fid;

    setNextSwitch(getNextSwitch() + static_cast<uint64_t>(sequence_size[sequence_pos] * intervalRatio));

    // a hack for stacking validation
    if(getNextSwitch() < totalnInst) {
      setNextSwitch(totalnInst + sequence_size[sequence_pos]);
    }

  } else { // SET_MODE
    I(0);
    setMode(mod, fid);
    I(mode == mod);

    switch(mod) {
    case EmuRabbit:
      if(totalnInstForcedDetail <= nInstRabbit) {
        setNextSwitch(getNextSwitch() + static_cast<uint64_t>(nInstRabbit * intervalRatio));
        setNextSwitch(getNextSwitch() - ((next2EmuTiming == EmuRabbit) ? totalnInstForcedDetail : 0));
      }
      sequence_pos = 3;
      break;
    case EmuWarmup:
      if(totalnInstForcedDetail <= nInstWarmup) {
        setNextSwitch(getNextSwitch() + static_cast<uint64_t>(nInstWarmup * intervalRatio));
        setNextSwitch(getNextSwitch() - ((next2EmuTiming == EmuWarmup) ? totalnInstForcedDetail : 0));
      }
      sequence_pos = 0;
      break;
    case EmuDetail:
      setNextSwitch(getNextSwitch() + nInstForcedDetail);
      totalnInstForcedDetail += nInstForcedDetail;
      sequence_pos = 1;
      break;
    case EmuTiming:
      setNextSwitch(getNextSwitch() + static_cast<uint64_t>(nInstTiming * intervalRatio));
      sequence_pos = 2;
      // intervalRatio = 1 + static_cast<float>totalnInstForcedDetail/static_cast<float>nInstTiming;
      break;
    default:
      I(0);
    }
  }
}

void SamplerPeriodic::doPWTH(FlowID fid) {
  if(!doPower)
    return;

  uint64_t mytime = 0;
  if(PerfSampleLeftForTemp <= 0) {

    mytime = getTime();
    I(mytime > lastTime);

    uint64_t ti = mytime - lastTime;
    I(ti);

    if(ti == 0) {
      PerfSampleLeftForTemp = TempToPerfRatio;
      return;
    }

    BootLoader::getPowerModelPtr()->setSamplingRatio(getSamplingRatio());
    BootLoader::getPowerModelPtr()->calcStats(ti, !(lastMode == EmuTiming), fid);
    if(doTherm) {
      BootLoader::getPowerModelPtr()->updateSescTherm(ti);
    }
    PerfSampleLeftForTemp = TempToPerfRatio;
    lastTime              = mytime;
  }

  PerfSampleLeftForTemp--;
}

void SamplerPeriodic::syncTimeAndTimingModes(FlowID fid) {

  I(!done[fid]);

  if(mode == EmuTiming /*&& next_mode != EmuTiming*/) {
    lastMode = mode;
    syncTimeAndWaitForOthers(fid);
    return;
  }

  I(mode != EmuTiming || next_mode == EmuTiming);

  if(mode == EmuDetail && lastMode == EmuTiming) {
    syncTimeAndWaitForOthers(fid);
  } else {
    lastMode = mode;
    syncTimeAndContinue(fid);

    if(next_mode == EmuRabbit) {
      setModeNativeRabbit();
    }

    I(!(doPower && fid == winnerFid && lastMode == EmuTiming));
  }
}

void SamplerPeriodic::syncTimeAndSamples(FlowID fid) {
  if(mode == EmuDetail && lastMode == EmuTiming) {
    lastMode = mode;
    syncTimeAndFinishWaitingForOthers(fid);
    return;
  }

  lastMode = mode;
  syncTimeAndContinue(fid);
  if(next_mode == EmuRabbit) {
    setModeNativeRabbit();
  }
}

void SamplerPeriodic::coordinateWithOthersAndNextMode(FlowID fid) {

  fetchNextMode();

  syncTimeAndSamples(fid);

  if(lastMode == EmuTiming) {
    BootLoader::reportSample();

    if(getTime() >= maxnsTime || totalnInst >= nInstMax) {
      markDone();
      MSG("finishing QEMU thread");
      pthread_exit(0);
      return;
    }
    // std::cout<<"mode "<<lastMode<<" fid:"<<sFid<<" sID:"<<nSamples[sFid]<<" totalSamples:"<<totalnSamples<<"
    // time:"<<getTime()<<"\n";
  }

  if(fid == winnerFid) {
    if(lastMode == EmuTiming) { // timing is going to be over
      doPWTH(fid);
    }
  }
}

void SamplerPeriodic::updateIntervalRatio() {
  intervalRatio = 1.0 / estCPI;
}

void SamplerPeriodic::syncTimeAndWaitForOthers(FlowID fid) {
  nextMode(SET_MODE, fid, EmuDetail);
  updateCPI(fid);
  dsync->inc();
}

void SamplerPeriodic::syncTimeAndContinue(FlowID fid) {
  nextMode(ROTATE, fid);
  updateCPI(fid);
}

void SamplerPeriodic::syncTimeAndFinishWaitingForOthers(FlowID fid) {
  nextMode(SET_MODE, fid, next2EmuTiming);
}

void SamplerPeriodic::dumpCPI() {
  static bool  first = true;
  static FILE *log;
  if(first) {
    log   = genReportFileNameAndOpen("cpi_");
    first = false;
  }

  fprintf(log, "fid%d %g\n", sFid, getMeaCPI());
}

void SamplerPeriodic::dumpTime() {
  static bool  first = true;
  static FILE *log;
  if(first) {
    log   = genReportFileNameAndOpen("time_");
    first = false;
  }

  for(size_t i = 0; i < emul->getNumFlows(); i++) {
    if(isActive(mapLid(i)))
      fprintf(log, "%lu ", getTime());
    else
      fprintf(log, "%d ", 0);
  }

  fprintf(log, "\n");
}

void SamplerPeriodic::setStatsFlag(DInst *dinst) {
  // Keep same
}
