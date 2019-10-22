// Contributed by Jose Renau
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

#ifndef EMU_SAMPLER_H
#define EMU_SAMPLER_H

#include <stdint.h>
#include <time.h>
#include <vector>

#include "DInst.h"
#include "GStats.h"
#include "nanassert.h"

#include "EmulInterface.h"
extern uint64_t cuda_inst_skip;

class EmuSampler {
public:
  enum EmuMode { EmuInit, EmuRabbit, EmuWarmup, EmuDetail, EmuTiming, EmuMax };

private:
  uint64_t nextSwitch;

protected:
  void     setNextSwitch(uint64_t instNum);
  uint64_t getNextSwitch() const {
    return nextSwitch;
  }

  void syncTime();

  const char *    name;
  FlowID          sFid;
  FILE *          syscall_file;
  struct timespec startTime;

  EmulInterface *emul;

  static uint64_t __thread local_icount;

  uint64_t nInstMax;
  uint64_t nInstSkip;

  uint64_t next;
  uint64_t phasenInst; // ninst since the current mode started
  uint64_t lastPhasenInst;

  bool syscall_enable;
  bool syscall_generate;
  bool syscall_runtime;
  bool stopJustCalled;

  EmuMode mode;
  EmuMode next_mode;

  double freq; // CPU/GPU frequency
  double meaCPI;
  double meauCPI;
  bool   calcCPIJustCalled;

  GStatsCntr *nSwitches;
  GStatsCntr *tusage[EmuMax];
  GStatsCntr *iusage[EmuMax];
  GStatsCntr *globalClock_Timing;

  GStats *        nCommitted;
  GStatsAvg *     ipc;
  GStatsAvg *     uipc;
  Time_t          globalClock_Timing_prev;
  uint64_t        prevnCommitted;
  pthread_mutex_t stop_lock;
  pthread_mutex_t mode_lock;

  static std::vector<bool> done;
  static volatile bool     terminated;
  static uint64_t *        instPrev;
  static uint64_t *        clockPrev;
  static uint64_t *        fticksPrev;

  void markDone();

  void beginTiming(EmuMode mod);

  void     setMode(EmuMode mod, FlowID fid);
  bool     restartRabbit;
  uint32_t numFlow;

  static bool roi_skip;

  static float turboRatio;

public:
  uint64_t totalnInst; // total # instructions
  EmuMode  getMode() const {
    return mode;
  }

  bool toggle_roi();

  void setModeNativeRabbit() {
    restartRabbit = true; // This flag is only set by the sampler
  }

  void stopNativeRabbit() {
    restartRabbit = false;
  }

  static bool isTerminated() {
    return terminated;
  }

  bool getrestartrabbitstatus() {
    return restartRabbit;
  }

  EmuSampler(const char *name, EmulInterface *emu, FlowID fid);
  virtual ~EmuSampler();

  // 4 possible states:
  //
  // rabbit  : no timing or warmup, go as fast as possible skipping stuff
  //
  // warmup  : no timing but it has system warmup (caches/bpred)
  //
  // detail  : detailed modeling, but without updating statistics
  //
  // timing  : full timing modeling

  void stop();
  void stopClockTicks(FlowID fid, double weigth = 1);
  void startInit(FlowID fid);
  void startRabbit(FlowID fid);
  void startWarmup(FlowID fid);
  void startDetail(FlowID fid);
  void startTiming(FlowID fid);

  static void setTurboRatio(float turbor) {
    turboRatio = turbor;
  };
  static float getTurboRatio() {
    return turboRatio;
  };

  uint64_t getTotalnInst() const {
    return totalnInst;
  }

  // Return the time from the beginning of simulation in nanoseconds
  virtual uint64_t getTime() = 0;

  bool execute(FlowID fid, uint64_t icount);

  void AtomicDecrPhasenInst(uint64_t i) {
    AtomicSub(&phasenInst, i);
  }
  void AtomicDecrTotalnInst(uint64_t i) {
    AtomicSub(&totalnInst, i);
  }

  uint64_t do_native_ffwd() {
    // FIXME
    // Set nInstrabbit = 0:
    // Switch mode to warmup directly.. or cause it to switch (in GPUThread Manager?)
    return 0;
  }

  virtual bool isActive(FlowID fid) = 0;

  virtual uint64_t queue(uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, int src1, int src2, int dest,
                         int dest2, uint64_t data2 = 0)                            = 0;
  virtual void     getGPUCycles(FlowID fid, float ratio = 1.0) = 0;
  void             syscall(uint32_t num, uint64_t usecs, FlowID fid);

  virtual FlowID resumeThread(FlowID uid, FlowID last_fid) = 0;
  virtual FlowID resumeThread(FlowID uid)                  = 0;
  virtual void   pauseThread(FlowID fid)                   = 0;
  virtual void   terminate()                               = 0;

  virtual void setStatsFlag(DInst *dinst) = 0;

  float getMeaCPI() {
    calcCPIJustCalled = false;
    return meaCPI;
  }
  float getMeauCPI() {
    calcCPIJustCalled = false;
    return meauCPI;
  }
  void calcCPI();

  FlowID getNumFlows();
  // FlowID getFirstFlow();

  FlowID getFid(FlowID last_fid) {
    return emul->getFid(last_fid);
  }
  void freeFid(FlowID last_fid) {
    emul->freeFid(last_fid);
  }
  void setFid(FlowID cpudid) {
    emul->setFid(cpudid);
  }

  FlowID getsFid() const {
    return sFid;
  }

  FlowID mapLid(FlowID lid) const {
    return emul->mapLid(lid);
  }

  void startMode(FlowID fid);
  void drainFIFO() {
    emul->drainFIFO();
  }
  virtual int64_t getThreads2Simulate() {
    return 0;
  }
  bool getStatsFlag() const {
    return mode == EmuTiming;
  }
};
#endif
