// Contributed by Jose Renau
//                Karin Strauss
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

#ifndef GPROCESSOR_H
#define GPROCESSOR_H

//#define WAVESNAP_EN

#include "estl.h"

#include <stdint.h>

// Generic Processor Interface.
//
// This class is a generic interface for Processors. It has been
// design for Traditional and SMT processors in mind. That's the
// reason why it manages the execution engine (RDEX).

#include "nanassert.h"

#include "Cluster.h"
#include "ClusterManager.h"
#include "EmuSampler.h"
#include "EmulInterface.h"
#include "FastQueue.h"
#include "GStats.h"
#include "Instruction.h"
#include "Pipeline.h"
#include "callback.h"

#include "LSQ.h"
#include "Prefetcher.h"
#include "Resource.h"
#include "Snippets.h"

class GMemorySystem;
class BPredictor;

#ifdef WAVESNAP_EN
#include "wavesnap.h"
#endif

class GProcessor {
private:
protected:
  // Per instance data
  const uint32_t cpu_id;

  const int32_t FetchWidth;
  const int32_t IssueWidth;
  const int32_t RetireWidth;
  const int32_t RealisticWidth;
  const int32_t InstQueueSize;
  const size_t  MaxROBSize;

  FlowID         maxFlows;
  EmulInterface *eint;
  GMemorySystem *memorySystem;

  StoreSet           storeset;
  Prefetcher         prefetcher;
  FastQueue<DInst *> rROB; // ready/retiring/executed ROB
  FastQueue<DInst *> ROB;

  // Updated by Processor or SMTProcessor. Shows the number of clocks
  // that the processor have been active (fetch + exe engine)
  ID(int32_t prevDInstID);

  uint32_t smt;     // 1...
  uint32_t smt_ctx; // 0... smt_ctx = cpu_id % smt

  bool active;

  // BEGIN  Statistics
  //
  GStatsCntr *nStall[MaxStall];
  GStatsCntr *nInst[iMAX];

  // OoO Stats
  GStatsAvg  rrobUsed;
  GStatsAvg  robUsed;
  GStatsAvg  nReplayInst;
  GStatsCntr nCommitted; // committed instructions

  // "Lack of Retirement" Stats
  GStatsCntr noFetch;
  GStatsCntr noFetch2;

  GStatsCntr nFreeze;
  GStatsCntr clockTicks;

  static Time_t      lastWallClock;
  Time_t             lastUpdatedWallClock;
  Time_t             activeclock_start;
  Time_t             activeclock_end;
  static GStatsCntr *wallClock;

  // END Statistics
  float    throttlingRatio;
  uint32_t throttling_cntr;

  uint64_t lastReplay;

  // Construction
  void buildInstStats(GStatsCntr *i[iMAX], const char *txt);
  void buildUnit(const char *clusterName, GMemorySystem *ms, Cluster *cluster, InstOpcode type);
  void buildCluster(const char *clusterName, GMemorySystem *ms);
  void buildClusters(GMemorySystem *ms);

  GProcessor(GMemorySystem *gm, CPU_t i);
  int32_t issue(PipeQueue &pipeQ);

  virtual void retire();

  virtual void       fetch(FlowID fid)     = 0;
  virtual StallCause addInst(DInst *dinst) = 0;

public:
#ifdef WAVESNAP_EN
  wavesnap *snap;
#endif
  virtual ~GProcessor();
  int getID() const {
    return cpu_id;
  }
  GStatsCntr *getnCommitted() {
    return &nCommitted;
  }

  GMemorySystem *getMemorySystem() const {
    return memorySystem;
  }
  virtual void   executing(DInst *dinst) = 0;
  virtual void   executed(DInst *dinst)  = 0;
  virtual LSQ *  getLSQ()                = 0;
  virtual bool   isFlushing()            = 0;
  virtual bool   isReplayRecovering()    = 0;
  virtual Time_t getReplayID()           = 0;

  virtual void replay(DInst *target){}; // = 0;

  bool isROBEmpty() const {
    return (ROB.empty() && rROB.empty());
  }
  int getROBsize() const {
    return (ROB.size() + rROB.size());
  }
  void drain() {
    retire();
  }

  // Returns the maximum number of flows this processor can support
  FlowID getMaxFlows(void) const {
    return maxFlows;
  }

  void report(const char *str);

  // Different types of cores extend this function. See SMTProcessor and
  // Processor.
  virtual bool advance_clock(FlowID fid) = 0;

  void setEmulInterface(EmulInterface *e) {
    eint = e;
  }

  void freeze(Time_t nCycles) {
    nFreeze.add(nCycles);
    clockTicks.add(nCycles);
  }

  void setActive() {
    active = true;
  }
  void clearActive() {
    I(isROBEmpty());
    active = false;
  }
  bool isActive() const {
    return active;
  }

  void setWallClock(bool en = true) {

    // FIXME: Periods of no fetch do not advance clock.

    trackactivity();

    if(lastWallClock == globalClock || !en)
      return;

    lastWallClock = globalClock;
    wallClock->inc(en);
  }
  static Time_t getWallClock() {
    return lastWallClock;
  }

  void trackactivity() {
    if(activeclock_end == (lastWallClock - 1)) {
    } else {
      if(activeclock_start != activeclock_end) {
        // MSG("\nCPU[%d]\t%lld\t%lld\n"
        //    ,cpu_id
        //    ,(long long int) activeclock_start
        //    ,(long long int) activeclock_end
        //    );
      }
      activeclock_start = lastWallClock;
    }
    activeclock_end = lastWallClock;
  }

  void dumpactivity() {
    // MSG("\nCPU[%d]\t%lld\t%lld\n"
    //    ,cpu_id
    //    ,(long long int) activeclock_start
    //    ,(long long int) activeclock_end
    //   );
  }

  StoreSet *getSS() {
    return &storeset;
  }
  Prefetcher *getPrefetcher() {
    return &prefetcher;
  }

  float getTurboRatio() {
    return EmuSampler::getTurboRatio();
  };
};

#endif // GPROCESSOR_H
