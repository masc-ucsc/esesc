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

#ifndef EMU_SAMPLER_PERIODIC_H
#define EMU_SAMPLER_PERIODIC_H

// Comment out to enable Instruction Based Sampling (Original SMARTS)
#define TBS 0


#include "SamplerBase.h"
#include "GStats.h"


class SamplerPeriodic : public SamplerBase {
private:
protected:

  uint64_t nInstRabbit;
  uint64_t nInstWarmup;
  uint64_t nInstDetail;
  uint64_t nInstTiming;
  uint64_t nInstForcedDetail;
  uint64_t maxnsTicks;

  uint64_t totalnInstForcedDetail;
  float    intervalRatio;
  uint32_t TempToPerfRatio;

  uint64_t nextSwitch;
  EmuMode  next2EmuTiming;

  FlowID winnerFid;

  GStatsCntr  *dsync;
  GStatsMax  *threadProgressedTime;
  static GStatsMax  *progressedTime;
#if DEBUG
  static GStatsCntr  *ipcl01;
  static GStatsCntr  *ipcl03;
  static GStatsCntr  *ipcl05;
  static GStatsCntr  *ipcg05;
#endif

  MemObj * DL1;

  static int32_t PerfSampleLeftForTemp; 

  void doWarmupOpAddrData(char op, uint64_t addr, uint64_t data);

  bool isItDone();
  void allDone();

  void coordinateWithOthersAndNextMode(FlowID fid);
  void syncTimeAndTimingModes(FlowID fid);
  void syncTimeAndWaitForOthers(FlowID fid);
  void syncTimeAndContinue(FlowID fid);
  void syncTimeAndFinishWaitingForOthers(FlowID fid);
  void syncTimeAndSamples(FlowID fid);
  void nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);

  void updateCPI();
  void updateCPIHist();
  void insertNewCPI();
  void computeEstCPI();
  void loadPredCPI();
  void updateIntervalRatio();
  void updateIntervalRatioStats();

  void doPWTH(FlowID fid);

  void dumpCPI();
  void dumpTime();

public:

  SamplerPeriodic(const char *name, const char *section, EmulInterface *emu, FlowID fid);
  virtual ~SamplerPeriodic();

  void queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env);

  void dumpThreadProgressedTime(FlowID fid);
  float getSamplingRatio() {return static_cast<float>(nInstTiming)/static_cast<float>(nInstRabbit + nInstWarmup + nInstDetail + nInstTiming);};
};

#endif

