/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Karin Strauss

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

#ifndef GPROCESSOR_H
#define GPROCESSOR_H

#define SCOORE_CORE 1//0

#include "estl.h"

#include <stdint.h>

// Generic Processor Interface.
//
// This class is a generic interface for Processors. It has been
// design for Traditional and SMT processors in mind. That's the
// reason why it manages the execution engine (RDEX).

#include "nanassert.h"

#include "callback.h"
#include "Cluster.h"
#include "ClusterManager.h"
#include "Instruction.h"
#include "FastQueue.h"
#include "GStats.h"
#include "Pipeline.h"
#include "EmulInterface.h"
#include "EmuSampler.h"


#include "Resource.h"
#include "Snippets.h"
#include "LSQ.h"


class FetchEngine;
class GMemorySystem;
class BPredictor;

class GProcessor {
  private:
  protected:
    // Per instance data
    const uint32_t cpu_id;
    const FlowID   MaxFlows;

    const int32_t FetchWidth;
    const int32_t IssueWidth;
    const int32_t RetireWidth;
    const int32_t RealisticWidth;
    const int32_t InstQueueSize;
    const size_t  MaxROBSize;

    EmulInterface   *eint;
    GMemorySystem   *memorySystem;

    StoreSet           storeset;
    FastQueue<DInst *> rROB; // ready/retiring/executed ROB
    FastQueue<DInst *> ROB;

    // Updated by Processor or SMTProcessor. Shows the number of clocks
    // that the processor have been active (fetch + exe engine)
    ID(int32_t prevDInstID);

    bool       active;

    // BEGIN  Statistics
    //
    GStatsCntr *nStall[MaxStall];
    GStatsCntr *nInst[iMAX];

    // OoO Stats
    GStatsAvg  rrobUsed;
    GStatsAvg  robUsed;
    GStatsAvg  nReplayInst;
    GStatsHist nReplayDist;
    GStatsCntr nCommitted; // committed instructions

    // "Lack of Retirement" Stats
    GStatsCntr noFetch;
    GStatsCntr noFetch2;

    GStatsCntr   nFreeze;
    GStatsCntr   clockTicks;

    static Time_t       lastWallClock;
    static GStatsCntr   *wallClock;

    // END Statistics
    uint32_t     throtting_cntr;
    uint32_t     throtting;
    uint64_t     lastReplay;

    // Construction
    void buildInstStats(GStatsCntr *i[iMAX], const char *txt);
    void buildUnit(const char *clusterName, GMemorySystem *ms, Cluster *cluster, InstOpcode type);
    void buildCluster(const char *clusterName, GMemorySystem * ms);
    void buildClusters(GMemorySystem *ms);

    GProcessor(GMemorySystem *gm, CPU_t i, size_t numFlows);
    int32_t issue(PipeQueue &pipeQ);

    virtual void retire();
    virtual StallCause addInst(DInst *dinst) = 0;

  public:

    virtual ~GProcessor();
    int getId() const { return cpu_id; }
    GStatsCntr *getnCommitted() { return &nCommitted;}

    GMemorySystem *getMemorySystem() const { return memorySystem; }
    virtual LSQ *getLSQ() = 0;    
    virtual bool isFlushing() = 0;
    virtual bool isReplayRecovering() = 0;
    virtual Time_t getReplayID() = 0;

    // Notify the fetch that an exception/replay happen. Stall the rename until
    // the rob replay is retired.
    virtual void replay(DInst *target) { };// = 0;

    bool isROBEmpty() const { return ROB.empty() && rROB.empty(); }

    // Returns the maximum number of flows this processor can support
    FlowID getMaxFlows(void) const { return MaxFlows; }

    void report(const char *str);

    // Different types of cores extend this function. See SMTProcessor and
    // Processor.
    virtual void fetch(FlowID fid) = 0;
    virtual bool execute() = 0;

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
      active = false;
    }

    void setWallClock(bool en=true) {
      if (lastWallClock == globalClock || !en)
        return;
      lastWallClock = globalClock;
      wallClock->inc(en);
    }

    StoreSet *getSS() { return &storeset; }

#ifdef ENABLE_CUDA
    float getTurboRatioGPU() { return EmuSampler::getTurboRatioGPU(); };
#endif
    float getTurboRatio() { return EmuSampler::getTurboRatio(); };

};

#endif   // GPROCESSOR_H
