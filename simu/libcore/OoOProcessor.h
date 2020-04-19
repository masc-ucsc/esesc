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
#define DEP_LIST_SIZE 64

//#define BTT_SIZE 512 //16 //512
#define NUM_LOADS 7 //6 //16 // maximum number of loads trackable by LDBP framework
#define NUM_OPS 4 //8 //16 // maximum number of operations between LD and BR in code snippet
#define BTT_MAX_ACCURACY 7
#define MAX_POWER_SAVE_MODE_CTR 100000
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
#ifdef ENABLE_LDBP
  GStatsCntr ldbp_power_mode_cycles;
  GStatsCntr ldbp_power_save_cycles;
#endif

  // BEGIN VIRTUAL FUNCTIONS of GProcessor
  bool       advance_clock(FlowID fid);
  StallCause addInst(DInst *dinst);
  void       retire();

  // END VIRTUAL FUNCTIONS of GProcessor
public:
  OoOProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~OoOProcessor();

#ifdef ENABLE_LDBP

  int64_t power_save_mode_ctr;
  int64_t power_clock;
  int64_t tmp_power_clock;
  const int BTT_SIZE;
  const int MAX_TRIG_DIST;

  void classify_ld_br_chain(DInst *dinst, RegType br_src1, int reg_flag);
  void classify_ld_br_double_chain(DInst *dinst, RegType br_src1, RegType br_src2, int reg_flag);
  void ct_br_hit_double(DInst *dinst, RegType b1, RegType b2, int reg_flag);
  void lgt_br_miss_double(DInst *dinst, RegType b1, RegType b2);
  void lgt_br_hit_double(DInst *dinst, RegType b1, RegType b2, int idx);
  DataType extract_load_immediate(AddrType li_pc);
  void generate_trigger_load(DInst *dinst, RegType reg, int lgt_index, int tl_type);
  int hit_on_lgt(DInst *dinst, int reg_flag, RegType reg1, RegType reg2 = LREG_R0);


  //new interface !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#if 0
  //LOAD TABLE
  void hit_on_load_table(DInst *dinst, bool is_li);
  int return_load_table_index(AddrType pc);
#endif
  void power_save_mode_table_reset();
  //RTT
  void rtt_load_hit(DInst *dinst);
  void rtt_alu_hit(DInst *dinst);
  void rtt_br_hit(DInst *dinst);
  //BTT
  int return_btt_index(AddrType pc);
  void btt_br_miss(DInst *dinst);
  void btt_br_hit(DInst *dinst, int btt_index);
  void btt_trigger_load(DInst *dinst, AddrType ld_ptr);
  int btt_pointer_check(DInst *dinst, int btt_index);
  // 1 -> btt_ptr == plq_ptr && all tracking set => trigger_loads
  // 2 -> btt_ptr == plq_ptr but all tracking not set => do nothing
  // 3 -> if x in BTT.ptr but not in PLQ.ptr => sp.track++
  // 4 -> if x in PLQ but not in BTT => sp..track = 0
#if 0
  //PLQ
  int return_plq_index(AddrType pc);
#endif

#if 0
  struct load_table { //store stride pref info
    // fields: LDPC, last_addr, delta, conf
    load_table() {
      ldpc         = 0;
      ld_addr      = 0;
      prev_ld_addr = 0;
      delta        = 0;
      prev_delta   = 0;
      conf         = 0;
      tracking     = 0;
      is_li = false;
    }
    AddrType ldpc;
    AddrType ld_addr;
    AddrType prev_ld_addr;
    int64_t delta;
    int64_t prev_delta;
    int conf;
    bool is_li;
    int tracking; //0 to 3 -> useful counter

    void lt_load_miss(DInst *dinst) {
      load_table();
      ldpc         = dinst->getPC();
      ld_addr      = dinst->getAddr();
    }

    void lt_load_hit(DInst *dinst) {
      ldpc         = dinst->getPC();
      prev_delta   = delta;
      prev_ld_addr = ld_addr;
      ld_addr      = dinst->getAddr();
      delta        = ld_addr - prev_ld_addr;
      if(delta == prev_delta) {
        conf++;
      }else {
        conf = conf / 2;
      }
      //MSG("LT clk=%d ldpc=%llx addr=%d del=%d conf=%d", globalClock, ldpc, ld_addr, delta, conf);
    }

    void lt_load_imm(DInst *dinst) {
      ldpc  = dinst->getPC();
      is_li = true;
      conf = 4096;
    }

    void lt_update_tracking(bool inc) {
      if(inc && tracking < 3) {
        tracking++;
      }else if(!inc && tracking > 0) {
        tracking --;
      }
    }
  };

  std::vector<load_table> load_table_vec = std::vector<load_table>(LOAD_TABLE_SIZE);
#endif

  struct rename_tracking_table {
    //tracks number of ops between LD and BR in a snippet
    //fields: num_ops and load_table_pointer
    //indexed by DST_REG
    rename_tracking_table() {
      num_ops = NUM_OPS + 1; //indicates that RTT entry is reset
      //num_ops = 0; //indicates that RTT entry is reset
      is_li = 0;
      load_table_pointer.clear();
    }
    int num_ops; //num of operations between load and branch in a snippet
    std::vector<AddrType> load_table_pointer = std::vector<AddrType>(NUM_LOADS); // pointer from Load Table to refer loads
    int is_li;

    void reset_rtt() {
      num_ops = NUM_OPS + 1; //indicates that RTT entry is reset
      is_li = 0;
      load_table_pointer.clear();
    }
  };

  std::vector<rename_tracking_table> rtt_vec = std::vector<rename_tracking_table>(LREG_MAX);

  struct branch_trigger_table { //BTT
    //Fields: brpc(index), br_pred accuracy, stride pointers
    branch_trigger_table() {
      brpc = 0;
      accuracy = 0;
      load_table_pointer.clear();
    }
    AddrType brpc;
    int accuracy; // MAX 0 to 7
    std::vector<AddrType> load_table_pointer = std::vector<AddrType>(NUM_LOADS); // pointer from Load Table to refer loads

    void btt_update_accuracy(DInst *dinst, int id) {
      if(dinst->isBranch_hit2_miss3()) {
        if(accuracy > 0)
          accuracy--;
      //}else if(dinst->isBranch_hit3_miss2()) {
      }else if(dinst->isBranchHit_level3()) {
        if(accuracy < 7)
          accuracy++;
      }
    }
  };

  std::vector<branch_trigger_table> btt_vec = std::vector<branch_trigger_table>(BTT_SIZE);

#if 0
  struct pending_load_queue { //queue of LOADS
    //fields: stride_ptr and tracking
    pending_load_queue() {
      tracking = 0;
      load_pointer = 0;
    }
    AddrType load_pointer;
    int tracking; // 0 to 3 - just like tracking in load_table_vec

    void plq_update_tracking(bool inc) {
      if(inc && tracking < 3) {
        tracking++;
      }else if(!inc && tracking > 0) {
        tracking --;
      }
    }

  };

  std::vector<pending_load_queue> plq_vec = std::vector<pending_load_queue>(PLQ_SIZE);

  struct classify_table_entry { //classifies LD-BR chain(32 entries; index -> Dest register)
    classify_table_entry(){
      dest_reg   = LREG_R0;
      ldpc       = 0;
      lipc       = 0;
      ld_addr    = 0;
      dep_depth  = 0;
      ldbr_type  = 0;
      ld_used    = false;
      simple     = false;
      is_li      = 0;
      valid      = false;
      mv_src    = LREG_R0;
      mv_active = false;
      is_tl_r1   = false; // TL by R1
      is_tl_r2   = false; // TL by R2
      is_tl_r1r2 = false; // TL by both R1 and R2
      is_li_r1   = false; // R1 uses Li
      is_li_r2   = false; // R2 uses Li
      is_li_r1r2 = false;   // R1 and R2 uses Li
      for(int i = 0; i < DEP_LIST_SIZE; i++)
        dep_list[i] = 0;
    }

    RegType dest_reg;
    AddrType ldpc;
    AddrType lipc; // Load Immediate PC
    AddrType ld_addr;
    int dep_depth;
    AddrType dep_list[DEP_LIST_SIZE]; //list of dependent instruction between LD and BR //FIXME - not static size
    int ldbr_type;

    // 1->simple -> R1 => LD->BR && R2 => 0 && dep == 1
    // 2->simple -> R2 => LD->BR && R1 => 0 && dep == 1
    // 3->simple -> R1 => LD->ALU+->BR           && R2 => 0 && dep > 1
    // 4->simple -> R1 => LD->ALU*->Li->ALU+->BR && R2 => 0 && dep > 1
    // 5->simple -> R2 => LD->ALU+->BR           && R1 => 0 && dep > 1
    // 6->simple -> R2 => LD->ALU*->Li->ALU+->BR && R1 => 0 && dep > 1
    // 7->simple -> R1 => LD->ALU*->Li->BR && R2 => 0 && dep > 1
    // 8->simple -> R2 => LD->ALU*->Li->BR && R1 => 0 && dep > 1
    // 9->complex  -> R1 => LD->ALU*->Li->BR && R2 => LD->ALU+->BR && dep > 1
    // 10->complex -> R1 => LD->ALU*->Li->BR && R2 => LD->ALU*->Li->BR && dep > 1
    // 11->complex -> R1 => LD->ALU*->Li->BR && R2 => LD->ALU*->Li->ALU+->BR && dep > 1
    // 12->complex -> R2 => LD->ALU*->Li->BR && R1 => LD->ALU+->BR && dep > 1
    // 13->complex -> R2 => LD->ALU*->Li->BR && R1 => LD->ALU*->Li->ALU+->BR && dep > 1
    // 14->double  -> R1 == R2 == LD->BR && dep == 1
    // 15->double  -> R1 => LD->BR && R2 => LD->ALU+->BR && dep > 1
    // 16->double  -> R1 => LD->BR && R2 => LD->ALU*->Li->ALU+->BR && dep > 1
    // 17->double  -> R1 => LD->BR && R2 => LD->ALU*->Li->BR && dep > 1
    // 18->double  -> R2 => LD->BR && R1 => LD->ALU+->BR && dep > 1
    // 19->double  -> R2 => LD->BR && R1 => LD->ALU*->Li->ALU+->BR && dep > 1
    // 20->double  -> R2 => LD->BR && R1 => LD->ALU*->Li->BR && dep > 1
    // 21->Any type + mv to src2(src1->src2) -> if a mv instn swaps data(similar to qsort) Br src2 data = mv data
    // 22->Any type + mv to src1(src2->src1) -> if a mv instn swaps data(similar to qsort) Br src1 data = mv data

    //OUTCOME_CALC => 1, 2, 7, 8, 10, 14, 17, 20
    //DOC          => 3, 4, 5, 6, 9, 11, 12, 13, 15, 16, 18, 19

    bool ld_used; //is ld inst executed before the current dependent branch inst?
    bool simple; //does BR have only one Src operand
    DataType li_data;
    int is_li; //is one Br data dependent on a Li or Lui instruction
    // 0=>LD->ALU+->BR or LD->BR; 1=>LD->ALU*->Li->BR; 2=>LD->ALU*->Li->ALU+->BR
    bool valid;
    //parameters to track move instructions
    RegType mv_src; //source Reg of mv instruction
    bool mv_active; //
    //parameters below help with LD-BR classification
    bool is_tl_r1;  // TL by R1
    bool is_tl_r2;  // TL by R2
    bool is_tl_r1r2; // TL by both R1 and R2
    bool is_li_r1;   // R1 uses Li
    bool is_li_r2;   // R2 uses Li
    bool is_li_r1r2; // R1 and R2 uses Li
    std::vector<Time_t> mem_lat_vec = std::vector<Time_t>(10);
    Time_t max_mem_lat;
    int num_mem_lat;
    AddrType goal_addr;
    AddrType prev_goal_addr;

    void ct_load_hit(DInst *dinst) { //add/reset entry on CT
      classify_table_entry(); // reset entries
      if(dinst->getPC() != ldpc) {
        mem_lat_vec.clear();
        max_mem_lat = 0;
        num_mem_lat = 0;
        prev_goal_addr = -1;
      }
      dest_reg  = dinst->getInst()->getDst1();
      ldpc      = dinst->getPC();
      lipc      = 0;
      ld_addr   = dinst->getAddr();
      dep_depth = 1;
      is_li = 0;
      ldbr_type = 0;
      valid     = true;
      ld_used   = true;
      is_tl_r1   = false; // TL by R1
      is_tl_r2   = false; // TL by R2
      is_tl_r1r2 = false; // TL by both R1 and R2
      is_li_r1   = false; // R1 uses Li
      is_li_r2   = false; // R2 uses Li
      is_li_r1r2 = false;   // R1 and R2 uses Li
      for(int i = 0; i < DEP_LIST_SIZE; i++)
        dep_list[i] = 0;
    }

    void ct_br_hit(DInst *dinst, int reg_flag) {
      ldbr_type = 0;
      simple = false;
      if(dinst->getInst()->getSrc1() == 0 || dinst->getInst()->getSrc2() == 0) {
        simple = true;
      }
      if(simple) {
        if(dep_depth == 1) {
          ldbr_type = 1;
          if(reg_flag == 2)
            ldbr_type = 2;
        }else if(dep_depth > 1 && dep_depth < 4) {
          ldbr_type = 3;
          if(is_li == 1) {
            ldbr_type = 7;
          }else if(is_li == 2) {
            ldbr_type = 0;//4; //FIXME
          }
          if(reg_flag == 2) {
            ldbr_type = 5;
            if(is_li == 1) {
              ldbr_type = 8;
            }else if(is_li == 2) {
              ldbr_type = 0;//6; //FIXME
            }
          }
        }
      }else {
        if(reg_flag == 3) { //Br's R1 gets LD/ALU data and R2->Li
          if(dep_depth == 1) {
            if(is_li == 0) {
              ldbr_type = 17; // R1=>LD; R2=>Li
            }
          }else if(dep_depth > 1 && dep_depth < 4) {
            if(is_li == 0) {
              ldbr_type = 0;//12;        // R1=>LD->ALU+->BR; R2=>Li
            }else if(is_li == 2) {
              ldbr_type = 0;//13;        // R1=>LD->ALU*->Li->ALU+->BR; R2=>Li
            }
          }
        }else if(reg_flag == 4) { //Br's R2 gets LD/ALU data and R1->Li
          if(dep_depth == 1 && is_li == 0) {
            ldbr_type = 20; // R1=>Li; R2=>LD
          }else if(dep_depth > 1 && dep_depth < 4) {
            if(is_li == 0) {
              ldbr_type = 0;//9;         // R1=>Li; R2=>LD->ALU+->BR
            }else if(is_li == 1) {
              ldbr_type = 10;        // R1=>Li; R2=>Li
            }else if(is_li == 2) {
              ldbr_type = 0;//11;        // R1=>Li; R2=>LD->ALU*->Li->ALU+->BR
            }
          }
        }else if(reg_flag == 6) { // Br's R1 get LD/ALU data and R2=>Li->ALU+
          if(dep_depth == 1 && is_li == 0)
            ldbr_type = 0;//16; //R1=>LD; R2=>Li->ALU+->BR
        }else if(reg_flag == 7) { // Br's R2 get LD/ALU data and R1=>Li->ALU+
          if(dep_depth == 1 && is_li == 0)
            ldbr_type = 0;//19; //R2=>LD; R1=>Li->ALU+->BR
        }
#if 0
        if(reg_flag == 3) {
          if(dep_depth == 1 && !is_li) {
            ldbr_type = 10;
          }else if(dep_depth > 1 && !is_li) {
            ldbr_type = 8;
          }
        }else if(reg_flag == 4) {
          //ldbr_type = 0;
          if(dep_depth == 1 && !is_li) {
            ldbr_type = 9;
          }else if(dep_depth > 1 && !is_li) {
            ldbr_type = 7;
          }
        }
#endif
      }
    }

    void ct_alu_hit(DInst *dinst) {
      //FIXME - check if alu is Li or Lui
      dep_depth++;
      dep_list[dep_depth] = dinst->getPC();
    }


  };

  struct load_gen_table_entry {
    load_gen_table_entry() {
      //ld fields
      ldpc            = 0;
      start_addr      = 0;
      end_addr        = 0;
      constant        = 0;
      prev_constant   = 0;
      last_trig_addr  = 0;
      //ld_delta        = 0;
      //prev_delta      = 0;
      ld_conf         = 0;
      //
      ldbr_type       = 0;
      dep_depth       = 0;
      //dump fields
      br_miss_ctr     = 0;
      hit2_miss3      = 0;
      hit3_miss2      = 0;
      //br fields
      inf_branch      = 0;
      brpc            = 0;
      brop            = ILLEGAL_BR;
      br_outcome      = false;
      br_mv_outcome   = 0;
      //LD2 fields
      ldpc2            = 0;
      start_addr2      = 0;
      end_addr2        = 0;
      ld_conf2         = 0;
      dep_depth2       = 0;
      //constant2        = 0;
      //prev_constant2   = 0;
      //last_trig_addr2  = 0;

      //mv stats
      mv_type = 0;
    }

    //ld vars
    AddrType ldpc;
    AddrType start_addr;
    AddrType end_addr;
    int64_t ld_delta;
    int64_t prev_delta;
    uint64_t ld_conf;
    AddrType constant;
    AddrType prev_constant;
    AddrType last_trig_addr;
    //mv stats
    DataType mv_data; // data from a mv inst
    int mv_type; //0-> mv inactive, 1 = src1->Src2, 2 = src2->src1

    //br fields
    int64_t inf_branch;  //number of inflight branches
    AddrType brpc;
    BrOpType brop;
    bool br_outcome;
    DataType br_data1;
    DataType br_data2;
    int br_mv_outcome; //0->N/A; 1->Not Taken; 2->Taken
    //
    int ldbr_type;
    int dep_depth;
    //stats/dump fields
    int br_miss_ctr;
    int hit2_miss3; //hit in level2 BP, miss in level 3
    int hit3_miss2; // hit in level 3 BP, miss in level 2

    //for LD2
    AddrType ldpc2;
    AddrType start_addr2;
    AddrType end_addr2;
    int64_t ld_delta2;
    int64_t prev_delta2;
    uint64_t ld_conf2;
    int dep_depth2;
    //AddrType constant2;
    //AddrType prev_constant2;
    //AddrType last_trig_addr2;

    void lgt_br_hit_li(DInst *dinst, int ldbr, int depth) {
      ldbr_type  = ldbr;
      dep_depth  = depth;
      lgt_update_br_fields(dinst);
    }

    void lgt_br_hit(DInst *dinst, AddrType ld_addr, int ldbr, int depth, bool is_r1) {
      if(is_r1) {
        prev_delta = ld_delta;
        ld_delta   = ld_addr - start_addr;
        start_addr = ld_addr;
        if(ld_delta == prev_delta) {
          ld_conf++;
        }else {
          ld_conf = ld_conf / 2;
        }
        dep_depth  = depth;
      }else {
        prev_delta2 = ld_delta2;
        ld_delta2   = ld_addr - start_addr2;
        start_addr2 = ld_addr;
        if(ld_delta2 == prev_delta2) {
          ld_conf2++;
        }else {
          ld_conf2 = ld_conf2 / 2;
        }
        dep_depth2  = depth;
      }
      ldbr_type  = ldbr;
      lgt_update_br_fields(dinst);
    }

    void lgt_br_miss(DInst *dinst, int ldbr, AddrType _ldpc, AddrType ld_addr, bool is_r1) {
      if(is_r1) {
        ldpc         = _ldpc;
        start_addr   = ld_addr;
      }else {
        ldpc2         = _ldpc;
        start_addr2   = ld_addr;
      }
      ldbr_type    = ldbr;
      lgt_update_br_fields(dinst);
      //MSG("LGT_BR_MISS clk=%u ldpc=%llx ld_addr=%u del=%u prev_del=%u conf=%u brpc=%llx ldbr=%d", globalClock, ldpc, start_addr, ld_delta, prev_delta, ld_conf, brpc, ldbr_type);
    }

    void lgt_update_br_fields(DInst *dinst) {
      brpc            = dinst->getPC();
      inf_branch      = dinst->getInflight(); //FIXME use dinst->getInflight() instead of variable
      if(br_mv_outcome == 0) {
        if(ldbr_type > 20) {
          br_mv_outcome = 1; //swap data on BR not taken
          if(br_outcome)
            br_mv_outcome = 2; //swap data on BR taken
        }
      }
      br_outcome      = dinst->isTaken();
      br_data1        = dinst->getData();
      br_data2        = dinst->getData2();

#if 0
      if(dinst->isBranchMiss_level2()) {
        if(br_miss_ctr < 7)
          br_miss_ctr++;
      }else{
        if(br_miss_ctr > 0)
          br_miss_ctr--;
      }
#endif
      if(dinst->isBranch_hit2_miss3()) {
        if(hit2_miss3 < 7)
          hit2_miss3++;
      }else {
        if(hit2_miss3 > 0)
          hit2_miss3--;
      }
      if(dinst->isBranch_hit3_miss2()) {
        hit3_miss2++;
        if(hit2_miss3 > 0)
          hit2_miss3--;
      }
    }

  };

  std::vector<classify_table_entry> ct_table = std::vector<classify_table_entry>(32);
  std::vector<load_gen_table_entry> lgt_table = std::vector<load_gen_table_entry>(LGT_SIZE);
#endif

  MemObj *DL1;
  AddrType ldbp_brpc;
  AddrType ldbp_ldpc;
  AddrType ldbp_curr_addr;
  int64_t ldbp_delta;
  int64_t inflight_branch;
#if 0
  Time_t max_mem_lat;
  Time_t last_mem_lat;
  Time_t diff_mem_lat;
  int num_mem_lat;
  std::vector<Time_t> mem_lat_vec = std::vector<Time_t>(10);
#endif

#if 0
  bool is_tl_r1;
  bool is_tl_r2;
  bool is_tl_r1r2;
  bool is_li_r1;
  bool is_li_r2;
  bool is_li_r1r2;
#endif

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
