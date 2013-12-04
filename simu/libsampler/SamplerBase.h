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
