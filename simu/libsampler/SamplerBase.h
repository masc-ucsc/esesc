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

#include <map>
#include <vector>

#include "EmuSampler.h"
#include "TaskHandler.h"
#include "nanassert.h"

class MemObj;

class SamplerBase : public EmuSampler {

private:
protected:
  MemObj *DL1; // For warmup

  uint64_t nInstRabbit;
  uint64_t nInstWarmup;
  uint64_t nInstDetail;
  uint64_t nInstTiming;
  uint64_t nInstForcedDetail;
  uint64_t maxnsTime;
  double   last_addtime;

  EmuMode next2EmuTiming;
  EmuMode lastMode;

  uint64_t              pwr_updateInterval;
  std::vector<EmuMode>  sequence_mode;
  std::vector<uint64_t> sequence_size;
  size_t                sequence_pos;

  static GStatsMax *progressedTime;
  uint64_t          lastGlobalClock; // FIXME: Might need to define this as static for multicore
  static uint64_t   pastGlobalTicks;
  static uint64_t   gpuEstimatedCycles;
  float             gpuSampledRatio;

  static uint64_t lastTime;
  static bool     justResumed[128];
  static bool     finished[128];

  double dt_ratio;
  double estCPI;
  double freq;
  bool   first;

  bool               doPower;
  bool               doTherm;
  bool               doIPCPred;
  size_t             cpiHistSize;
  size_t             validP;
  size_t             headPtr;
  std::vector<float> cpiHist;

  bool allDone();
  void markThisDone(FlowID fid);

  FILE *genReportFileNameAndOpen(const char *str);
  void  fetchNextMode();
  void  doWarmupOpAddr(InstOpcode op, uint64_t addr);

public:
  SamplerBase(const char *name, const char *section, EmulInterface *emul, FlowID fid = 0);
  virtual ~SamplerBase();

  uint64_t getTime();
  void     getGPUCycles(FlowID fid, float ratio = 1.0);
  void     getClockTicks();

  uint64_t get_totalnInst() {
    return totalnInst;
  }
  EmuMode get_mode() {
    return mode;
  }
  bool callPowerModel(FlowID fid);
  bool isActive(FlowID fid);

  void setRabbit();
  void setTiming();

  virtual void updateCPI(FlowID id) {
    I(0);
  };
  virtual void updateCPIHist() {
    I(0);
  };
  virtual void loadPredCPI() {
    I(0);
  };
  virtual void doPWTH(FlowID id) {
    I(0);
  };

  void   pauseThread(FlowID fid);
  FlowID resumeThread(FlowID uid, FlowID last_fid);
  FlowID resumeThread(FlowID uid);
  FlowID getFid(FlowID last_fid);
  void   terminate();
  void   nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);
  double getFreq() const {
    return freq * getTurboRatio();
  }
  double getNominatedFreq() {
    return freq;
  }

  float getSamplingRatio() const {
    return static_cast<float>(nInstTiming) / static_cast<float>(nInstRabbit + nInstWarmup + nInstDetail + nInstTiming);
  };
};

#endif
