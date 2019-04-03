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

#include <vector>
#include <algorithm>
#include "nanassert.h"

#include "CodeProfile.h"
#include "FastQueue.h"
#include "FetchEngine.h"
#include "GOoOProcessor.h"
#include "Pipeline.h"
#include "callback.h"
#include "GStats.h"

//#define TRACK_FORWARDING 1
#define TRACK_TIMELEAK 1
//#define ENABLE_LDBP

class OoOProcessor : public GOoOProcessor {
private:
  class RetireState {
  public:
    double committed;
    Time_t r_dinst_ID;
    Time_t dinst_ID;
    DInst *r_dinst;
    DInst *dinst;
    bool   operator==(const RetireState &a) const {
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

  const bool    MemoryReplay;
  const int32_t RetireDelay;

  FetchEngine IFID;
  PipeQueue   pipeQ;
  LSQFull     lsq;

  uint32_t serialize_level;
  uint32_t serialize;
  int32_t  serialize_for;
  uint32_t forwardProg_threshold;
  DInst *  last_serialized;
  DInst *  last_serializedST;

  int32_t spaceInInstQueue;
  DInst * RAT[LREG_MAX];
  int32_t nTotalRegs;

  DInst *  serializeRAT[LREG_MAX];
  RegType  last_serializeLogical;
  AddrType last_serializePC;

  bool   busy;
  bool   replayRecovering;
  bool   getStatsFlag;
  Time_t replayID;
  bool   flushing;

  FlowID flushing_fid;

  RetireState                                                           last_state;
  void                                                                  retire_lock_check();
  bool                                                                  scooreMemory;
  StaticCallbackMember0<OoOProcessor, &OoOProcessor::retire_lock_check> retire_lock_checkCB;

  void fetch(FlowID fid);

protected:
  ClusterManager clusterManager;

  GStatsAvg avgFetchWidth;
#ifdef TRACK_TIMELEAK
  GStatsAvg  avgPNRHitLoadSpec;
  GStatsHist avgPNRMissLoadSpec;
#endif
#ifdef TRACK_FORWARDING
  GStatsAvg  avgNumSrc;
  GStatsAvg  avgNumDep;
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
  bool       advance_clock(FlowID fid);
  StallCause addInst(DInst *dinst);
  void       retire();

  // END VIRTUAL FUNCTIONS of GProcessor
public:
  OoOProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~OoOProcessor();
 
#ifdef ENABLE_LDBP
  struct ld_br_entry {
    ld_br_entry() {
      ld_pc            = 0;
      ld_addr          = 0;
      ld_data          = 0;
      ld_delta         = 0;
      ld_reg           = LREG_R0; 
      br_pc            = 0;
      br_src2          = LREG_R0;
      br_data2         = 0;
      ld_br_type       = 0;
      dependency_depth = 0;
      is_li            = false;
      simple           = false;
      direct           = false;
    }
    AddrType ld_pc;
    AddrType ld_addr;
    AddrType ld_data;
    uint64_t ld_delta;
    RegType ld_reg;
    AddrType br_pc;
    RegType br_src2; //src2 here refers to the source which is not dependent on Ld(it can be rs1 or rs2)
    AddrType br_data2;
    int ld_br_type;
    int dependency_depth;
    bool simple;
    bool direct;
    bool is_li; //is branch dependent on a load immediate?

    void reset_entry() {
      ld_delta         = 0;
      ld_reg           = LREG_R0;
      br_pc            = 0;
      br_src2          = LREG_R0;
      br_data2         = 0;
      ld_br_type       = 0;
      dependency_depth = 0;
      simple           = false;
      direct           = false;
      is_li            = false;
    }

    bool is_hit(RegType reg) {
      return reg == ld_reg;
    }

    void set_simple() {
      simple = true;
    }

    bool is_simple() {
      return simple;
    }

    void set_direct() {
      direct = false;
      if(dependency_depth == 1) {
        direct = true;
      }
    } 

    bool is_direct() {
      return direct;
    }
    
    void set_ld_retire(DInst *dinst) {
      if(ld_pc != dinst->getPC()) {
        reset_entry();
        ld_pc   = dinst->getPC();
        ld_addr = dinst->getAddr();
        ld_data = dinst->getData();
        ld_reg  = dinst->getInst()->getDst1();
      }else {
        /*if(ld_delta == 0) {
          ld_delta = dinst->getAddr() - ld_addr;
        } else if(ld_delta != (dinst->getAddr() - ld_addr)) {
          ld_delta = dinst->getAddr() - ld_addr;
        }*/
        ld_delta = dinst->getAddr() - ld_addr;
        ld_addr = dinst->getAddr();
        ld_data = dinst->getData();
      }
    }

    void set_br_retire(DInst *dinst) {
      br_pc = dinst->getPC();
      dependency_depth++;
      //br_src2 = dinst->getInst()->getSrc2();
      //br_data2 = dinst->getBrData2();
    }

  };

  HASH_MAP<RegType, ld_br_entry> ldbp_retire_table;
  MemObj *DL1; 
  AddrType ldbp_brpc;
  AddrType ldbp_ldpc;
  AddrType ldbp_curr_addr;
  AddrType ldbp_start_addr;
  AddrType ldbp_end_addr;
  uint64_t ldbp_delta;
  int brpc_count; //MATCH THIS COUNT WITH VEC_INDEX
  uint64_t inflight_branch;
  bool ldbp_reset;
  Time_t avg_mem_lat;
  Time_t total_mem_lat;
  Time_t max_mem_lat;
  Time_t last_mem_lat;
  Time_t diff_mem_lat;
  int num_mem_lat;
  std::vector<Time_t> mem_lat_vec = std::vector<Time_t>(10);

  //std::vector<int> ldbp_vector = std::vector<int>(LDBP_VEC_SIZE);

  void ldbp_reset_field(){
    ldbp_curr_addr      = 0;
    ldbp_start_addr      = 0;
    ldbp_end_addr        = 0;
    ldbp_delta           = 0;
    ldbp_brpc            = 0;
    ldbp_ldpc            = 0;
    ldbp_reset           = true;
    brpc_count           = 0;

    //for(int i = 0; i < LDBP_VEC_SIZE; i++) {
    //  ldbp_vector[i] = 0;
    //}
  }
  
#endif

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  LSQ *getLSQ() {
    return &lsq;
  }
  void replay(DInst *target);
  bool isFlushing() {
    return flushing;
  }
  bool isReplayRecovering() {
    return replayRecovering;
  }
  Time_t getReplayID() {
    return replayID;
  }

  void dumpROB();

  bool isSerializing() const {
    return serialize_for != 0;
  }
};

#endif
