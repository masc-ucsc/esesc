/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic

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

#ifndef _OOOPROCESSOR_H_
#define _OOOPROCESSOR_H_

#include "nanassert.h"

#include "callback.h"
#include "GOoOProcessor.h"
#include "Pipeline.h"
#include "FetchEngine.h"
#include "FastQueue.h"

#define MAX_REMEMBERED_VALUES 16384


class OoOProcessor : public GOoOProcessor {
private:
  class RetireState {
  public:
    double committed;
    Time_t r_dinst_ID;
    Time_t dinst_ID;
    DInst *r_dinst;
    DInst *dinst;    
    bool operator==(const RetireState& a) const {
      return a.committed == committed;
    };
    RetireState() {
      committed  = 0;
      r_dinst_ID = 0;
      dinst_ID   = 0;
      r_dinst    = 0;
      dinst      = 0;
    }
  };
  
  const bool NoMemoryReplay;
  
  FetchEngine IFID;
  PipeQueue   pipeQ;
  LSQFull     lsq;

  uint32_t  serialize_level;
  uint32_t  serialize;
  uint32_t  serialize_for;
  uint32_t  forwardProg_threshold;
  DInst    *last_serialized;
  DInst    *last_serializedST;

  int32_t spaceInInstQueue;
  DInst   *RAT[LREG_MAX];

  DInst   *serializeRAT[LREG_MAX];
  RegType  last_serializeLogical;
  AddrType last_serializePC;

  bool busy;
  bool replayRecovering;
  Time_t replayID;
  bool flushing;

  FlowID flushing_fid;

  RetireState last_state;
  void retire_lock_check();
  bool scooreMemory;
  StaticCallbackMember0<OoOProcessor, &OoOProcessor::retire_lock_check> retire_lock_checkCB;

protected:
  ClusterManager clusterManager;

  GStatsHist serialize_level_hist;
  GStatsAvg avgFetchWidth;

  // BEGIN VIRTUAL FUNCTIONS of GProcessor
  void fetch(FlowID fid);
  bool execute();
  StallCause addInst(DInst *dinst);
  void retire();

  // END VIRTUAL FUNCTIONS of GProcessor
public:
  OoOProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~OoOProcessor();

  LSQ *getLSQ() { return &lsq; }
  void replay(DInst *target);
  bool isFlushing() {return flushing;}
  bool isReplayRecovering() {return replayRecovering;}
  Time_t getReplayID() {return replayID;}

  void dumpROB();

  bool isSerializing() const { return serialize_for!=0; }

};

#endif
