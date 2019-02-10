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

#ifndef EMU_SAMPLER_PERIODIC_H
#define EMU_SAMPLER_PERIODIC_H

// Comment out to enable Instruction Based Sampling (Original SMARTS)
#define TBS 0

#include "GStats.h"
#include "SamplerBase.h"

class SamplerPeriodic : public SamplerBase {
private:
protected:
  uint64_t totalnInstForcedDetail;
  float    intervalRatio;
  uint32_t TempToPerfRatio;

  FlowID winnerFid;

  GStatsCntr *dsync;

  static int32_t PerfSampleLeftForTemp;

  void coordinateWithOthersAndNextMode(FlowID fid);
  void syncTimeAndTimingModes(FlowID fid);
  void syncTimeAndWaitForOthers(FlowID fid);
  void syncTimeAndContinue(FlowID fid);
  void syncTimeAndFinishWaitingForOthers(FlowID fid);
  void syncTimeAndSamples(FlowID fid);
  void nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);

  void updateCPI(FlowID id);
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

  uint64_t queue(uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, int src1, int src2, int dest, int dest2, uint64_t data2 = 0);
  void     setStatsFlag(DInst *dinst);
};

#endif
