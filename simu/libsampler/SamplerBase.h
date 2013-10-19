
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

#ifndef EMU_SAMPLER_BASIC_H
#define EMU_SAMPLER_BASIC_H

#define ROTATE 1
#define SET_MODE 0

#include <vector>
#include <map>

#include "nanassert.h"
#include "EmuSampler.h"
#include "TaskHandler.h"

class MemObj;

class SamplerBase : public EmuSampler {
protected:

  uint64_t nInstSkip;
  uint64_t nInstMax;
  EmuMode lastMode;

  uint64_t nextCheck;
  uint64_t interval;
  static uint64_t local_nsticks;
  uint64_t gClock;
  bool     localTicksUptodate;
  bool     endSimSiged;
  std::vector<EmuMode>  sequence_mode;
  std::vector<uint64_t> sequence_size;
  size_t   sequence_pos;

  uint64_t lastGlobalClock; // FIXME: Might need to define this as static for multicore
  static uint64_t pastGlobalTicks;
  static uint64_t gpuEstimatedCycles;
  float           gpuSampledRatio;

  static uint64_t lastTime;
  static uint64_t prednsTicks;
  static uint64_t nsTicks[128];
  static bool     justResumed[128];
  static bool     finished[128];


  double   estIPC;
  double   estCPI;
  double   freq;
  bool     first;

  bool     doPower;
  bool     doTherm;
  bool     doIPCPred;
  size_t   cpiHistSize;
  size_t   validP;
  size_t   headPtr;
  std::vector<float> cpiHist;

  uint64_t SamplInterval;     // can be removed?
  uint64_t rabbitPwrSkip;



  void     allDone();
  FILE *genReportFileNameAndOpen(const char *str);
  void fetchNextMode();
public:
  SamplerBase(const char *name, const char *section, EmulInterface *emul, FlowID fid = 0);
  virtual ~SamplerBase();

  virtual void syncStats();
  uint64_t getTime();
  uint64_t getLocalTime(FlowID fid = 999);
  uint64_t getLocalnsTicks();
  void     updateLocalTime();
  void addLocalTime(FlowID fid, uint64_t nsticks);
  void getGPUCycles(FlowID fid, float ratio = 1.0);
  void getClockTicks();

  uint64_t get_totalnInst(){return totalnInst;}
  EmuMode get_mode(){return mode;}
  bool callPowerModel(uint64_t &ti, FlowID fid);
  bool isActive(FlowID fid);

  void setRabbit();
  void setTiming();

  virtual void updateCPI() { I(0); };
  virtual void updateCPIHist() { I(0); };
  virtual void loadPredCPI() { I(0); };
  virtual void doPWTH() { I(0); };

  void pauseThread(FlowID fid);
  FlowID resumeThread(FlowID uid, FlowID last_fid);
  FlowID resumeThread(FlowID uid);
  FlowID getFid(FlowID last_fid);
  void syncRunning();
  void terminate();
  int isSamplerDone();
  void nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);
  void syncnsTicks(FlowID fid);
  void syncTimes(FlowID fid);
  void syncnSamples(FlowID fid);
  void setgClock() { gClock = globalClock; }
  uint64_t getgClock() { return gClock; }
  double getFreq(){ return freq*getTurboRatio(); }
  double getNominatedFreq(){ return freq; }

  virtual float getSamplingRatio() = 0;
};

#endif
