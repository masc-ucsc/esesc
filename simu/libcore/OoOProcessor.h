// Contributed by Jose Renau
//                Basilio Fraguela
//                Milos Prvulovic
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

#ifndef _OOOPROCESSOR_H_
#define _OOOPROCESSOR_H_

#include "nanassert.h"

#include "callback.h"
#include "GOoOProcessor.h"
#include "Pipeline.h"
#include "FetchEngine.h"
#include "FastQueue.h"
#include "CodeProfile.h"

#define MAX_REMEMBERED_VALUES 16384

#define RCMEM 1

#define TRACK_FORWARDING 1

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
  
  const bool MemoryReplay;
  const int32_t RetireDelay;
  
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
  int32_t  nTotalRegs;

  DInst   *serializeRAT[LREG_MAX];
  RegType  last_serializeLogical;
  AddrType last_serializePC;

  bool busy;
  bool replayRecovering;
  bool getStatsFlag;
  Time_t replayID;
  bool flushing;

  FlowID flushing_fid;

  RetireState last_state;
  void retire_lock_check();
  bool scooreMemory;
  StaticCallbackMember0<OoOProcessor, &OoOProcessor::retire_lock_check> retire_lock_checkCB;

  void fetch(FlowID fid);
protected:
  ClusterManager clusterManager;

  GStatsAvg avgFetchWidth;
#ifdef TRACK_FORWARDING
  GStatsAvg avgNumSrc;
  GStatsAvg avgNumDep;
  Time_t     fwdDone[LREG_MAX];
  GStatsCntr fwd0done0;
  GStatsCntr fwd1done0;
  GStatsCntr fwd1done1;
  GStatsCntr fwd2done0;
  GStatsCntr fwd2done1;
  GStatsCntr fwd2done2;
#endif
  CodeProfile codeProfile;
  double      codeProfile_trigger;

  // BEGIN VIRTUAL FUNCTIONS of GProcessor
  bool advance_clock(FlowID fid);
  StallCause addInst(DInst *dinst);
  void retire();

  // END VIRTUAL FUNCTIONS of GProcessor
public:
  OoOProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~OoOProcessor();

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  LSQ *getLSQ() { return &lsq; }
  void replay(DInst *target);
  bool isFlushing() {return flushing;}
  bool isReplayRecovering() {return replayRecovering;}
  Time_t getReplayID() {return replayID;}

  void dumpROB();

  bool isSerializing() const { return serialize_for!=0; }

};

#endif
