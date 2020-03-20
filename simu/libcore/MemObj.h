// Contributed by Jose Renau
//                Basilio Fraguela
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

#ifndef MEMOBJ_H
#define MEMOBJ_H

#include <vector>
#include <queue>
#include "DInst.h"
#include "callback.h"

#include "nanassert.h"

#include "MRouter.h"
#include "Resource.h"

class MemRequest;

#define PSIGN_NONE 0
#define PSIGN_RAS 1
#define PSIGN_NLINE 2
#define PSIGN_STRIDE 3
#define PSIGN_TAGE 4
#define PSIGN_INDIRECT 5
#define PSIGN_CHASE 6
#define PSIGN_MEGA 7
#define LDBUFF_SIZE 512
#define CIR_QUEUE_WINDOW 512 //FIXME: need to change this to a conf variable

#define LOT_QUEUE_SIZE 64 //512 //FIXME: need to change this to a conf variable
//#define BOT_SIZE 512 //16 //512
//#define LOR_SIZE 512
//#define LOAD_TABLE_SIZE 512 //64 //512
//#define PLQ_SIZE 512 //512
#define LOAD_TABLE_CONF 63
//#define ENABLE_LDBP

class MemObj {
private:
protected:
  friend class MRouter;

  MRouter *   router;
  const char *section;
  const char *name;
  const char *deviceType;

  const uint16_t  id;
  static uint16_t id_counter;
  int16_t         coreid;
  bool            firstLevelIL1;
  bool            firstLevelDL1;

  void addLowerLevel(MemObj *obj);
  void addUpperLevel(MemObj *obj);

public:


  MemObj(const char *section, const char *sName);
  MemObj();
  virtual ~MemObj();

#ifdef ENABLE_LDBP

  const int BOT_SIZE;
  const int LOR_SIZE;
  const int LOAD_TABLE_SIZE;
  const int PLQ_SIZE;
  const int CODE_SLICE_DELAY;
#if 0
  const int BOT_SIZE;
  //Load data buffer interface functions
  int hit_on_ldbuff(AddrType pc);
  void fill_ldbuff_mem(AddrType pc, AddrType sa, AddrType ea, int64_t del, AddrType raddr, int q_idx, int tl_type);
  void shift_load_data_buffer(AddrType pc, int tl_type);
  void flush_ldbuff_mem(AddrType pc);

  //BOT interface functions
  void find_cir_queue_index(MemRequest *mreq);
  int hit_on_bot(AddrType pc);
  void flush_bot_mem(int idx);
  void shift_cir_queue(AddrType pc, int tl_type);
  void fill_fetch_count_bot(AddrType pc);
  void fill_retire_count_bot(AddrType pc);
  void fill_bpred_use_count_bot(AddrType pc, int _hit2_miss3, int _hit3_miss2);
  void fill_bot_retire(AddrType pc, AddrType ldpc, AddrType saddr, AddrType eaddr, int64_t del, int lbtype, int tl_type);
  void fill_mv_stats(AddrType pc, int ldbr, DataType d1, DataType d2, bool swap, int mv_out);
  void fill_li_at_retire(AddrType brpc, int ldbr, bool d1_valid, bool d2_valid, int depth1, int depth2, DataType li1, DataType li2 = 0);

  int getQSize() {
    return CIR_QUEUE_WINDOW;
  }


  int getBotSize() {
    return BOT_SIZE;
  }

  int getLdBuffSize() {
    return LDBUFF_SIZE;
  }

  void setRetBrCount(int cnt) {
    ret_br_count = cnt;
  }

  int getRetBrCount() const {
    return ret_br_count;
  }
#endif

  //NEW INTERFACE !!!!!! Nov 20, 2019
  ////Load Table
  void hit_on_load_table(DInst *dinst, bool is_li);
  int return_load_table_index(AddrType pc);
  int getLoadTableConf() const {
    return LOAD_TABLE_CONF;
  }
  //PLQ
  int return_plq_index(AddrType pc);
  //LOR
  void lor_allocate(AddrType brpc, AddrType ld_ptr, AddrType ld_start, int64_t ld_del, int data_pos, bool is_li);
  //void lor_find_index(MemRequest *mreq);
  void lor_find_index(AddrType mreq_addr);
  void lor_trigger_load_complete(AddrType mreq_addr);
  typedef CallbackMember1<MemObj, AddrType, &MemObj::lor_trigger_load_complete> lor_trigger_load_completeCB;
  int return_lor_index(AddrType ld_ptr);
  int compute_lor_index(AddrType brpc, AddrType ld_ptr);
  //LOT
  void lot_fill_data(int lot_index, int lot_queue_index, AddrType tl_addr);
  bool lot_tl_addr_range(AddrType tl_addr, AddrType start_addr, AddrType end_addr, int64_t delta);
  int getBotSize() const {
    return BOT_SIZE;
  }
  int getLotQueueSize() const {
    return LOT_QUEUE_SIZE;
  }
  //BOT
  int return_bot_index(AddrType brpc);
  void bot_allocate(AddrType brpc, AddrType ld_ptr, AddrType ld_ptr_addr);


  struct load_table { //store stride pref info
    // fields: LDPC, last_addr, delta, conf
    load_table() {
      ldpc         = 0;
      ld_addr      = 0;
      prev_ld_addr = 0;
      delta        = 0;
      prev_delta   = 0;
      conf         = 0;
      use_slice    = 0;
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
    int use_slice; // set to 1 when Ld retires, reset to 0 when Br retires
    //-> if 1, indicates Br went through Ld else Br didn't use LD(and we don't have to trigger LD)
    int tracking; //0 to 3 -> useful counter

    void lt_load_miss(DInst *dinst) {
      load_table();
      ldpc         = dinst->getPC();
      ld_addr      = dinst->getAddr();
    }

    void lt_load_hit(DInst *dinst) {
      ldpc         = dinst->getPC();
      use_slice    = 1;
      prev_delta   = delta;
      prev_ld_addr = ld_addr;
      ld_addr      = dinst->getAddr();
      delta        = ld_addr - prev_ld_addr;
      if(delta == prev_delta) {
        if(conf < (LOAD_TABLE_CONF + 1))
          conf++;
      }else {
        //conf = conf / 2;
#if 1
        if(conf > (LOAD_TABLE_CONF/2))
          conf = conf - 4;
        else
          conf = conf / 2;
#endif
      }
#if 0
      if((dinst->getPC() == 0x119c0 || dinst->getPC() == 0x119be))
        MSG("LT clk=%d ldpc=%llx addr=%d del=%d conf=%d data=%d", globalClock, ldpc, ld_addr, delta, conf, dinst->getData());
#endif
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

  struct load_outcome_reg {
    //tracks trigger load info as each TL completes execution
    //fields: load start, delta, index(or n data), stride pointer, data position

    load_outcome_reg() {
      brpc = 0;
      ld_pointer = 0;
      ld_start = 0;
      ld_delta = 0;
      data_pos = 0; // ++ @Fetch and 0 @flush
      use_slice = 0;
      trig_ld_dist = 4;
      is_li = false;
    }
    AddrType ld_start; //load start addr
    int64_t ld_delta;
    AddrType ld_pointer; //load pointer from stride pref table
    AddrType brpc; //helps differentiate LOR entries when 2 Brs use same LD pair
    //int64_t data_pos; //
    int data_pos; //
    //tracks data position in LOT queue; used to index lot queue when TL returns
    int use_slice; //LOR's use_slice variable
    // init to 0, LOR accessed at fetch only when use_slice == 1
    bool is_li; //ESESC flag to not trigger load if Li
    int64_t trig_ld_dist; //

    void reset_entry() {
      brpc = 0;
      ld_pointer = 0;
      ld_start = 0;
      ld_delta = 0;
      data_pos = 0;
      use_slice = 0;
      trig_ld_dist = 4;
      is_li = false;
    }
  };

  std::vector<load_outcome_reg> lor_vec = std::vector<load_outcome_reg>(LOR_SIZE);

  struct load_outcome_table { //same number of entries as LOR
    //stores trigger load data
    load_outcome_table() {
#if 0
      for(int i = 0; i < LOT_QUEUE_SIZE; i++) {
        tl_addr[i] = 0;
        valid[i] = 0;
      }
#endif
      std::fill(tl_addr.begin(), tl_addr.end(), 0);
      std::fill(valid.begin(), valid.end(), 0);
    }
    //std::vector<DataType> data = std::vector<DataType>(LOT_QUEUE_SIZE);
    std::vector<AddrType> tl_addr = std::vector<AddrType>(LOT_QUEUE_SIZE);
    std::vector<int> valid = std::vector<int>(LOT_QUEUE_SIZE);

    void reset_valid() {
      std::fill(tl_addr.begin(), tl_addr.end(), 0);
      std::fill(valid.begin(), valid.end(), 0);
#if 0
      for(int i = 0; i < LOT_QUEUE_SIZE; i++) {
        tl_addr[i] = 0;
        valid[i] = 0;
      }
#endif
    }
  };

  std::vector<load_outcome_table> lot_vec = std::vector<load_outcome_table>(LOR_SIZE);

  struct branch_outcome_table {
    branch_outcome_table() {
      brpc      = 0;
      outcome_ptr = 0;
      br_flip = -1;
      load_ptr.clear();
      curr_br_addr.clear();
      std::fill(valid.begin(), valid.end(), 0);
#if 0
      for(int i = 0; i < LOT_QUEUE_SIZE; i++) {
        valid[i] = 0;
      }
#endif
    }

    AddrType brpc;
    //int64_t outcome_ptr; //Br count at fetch; used to index BOT queue at fetch
    int outcome_ptr; //Br count at fetch; used to index BOT queue at fetch
    int br_flip; // stop LOT update when br-flips
    // init to -1; 0 -> flip on NT; 1 -> flip on T
    std::vector<AddrType> load_ptr = std::vector<AddrType>(16);
    std::vector<AddrType> curr_br_addr = std::vector<AddrType>(16); //current ld addr used by Br (ESESC only param - for debugging)
    std::vector<int> valid = std::vector<int>(LOT_QUEUE_SIZE);

    void reset_valid() {
      outcome_ptr = 0;
      std::fill(valid.begin(), valid.end(), 0);
#if 0
      for(int i = 0; i < LOT_QUEUE_SIZE; i++) {
        valid[i] = 0;
      }
#endif
    }
  };

  std::vector<branch_outcome_table> bot_vec = std::vector<branch_outcome_table>(BOT_SIZE);

#if 0
  struct bot_entry {
    bot_entry() {
      seq_start_addr = 0;
      seq_start_addr2 = 0;
      brpc         = 0;
      ldpc         = 0;
      req_addr     = 0;
      start_addr   = 0;
      end_addr     = 0;
      delta        = 0;
      ldpc2         = 0;
      req_addr2     = 0;
      start_addr2   = 0;
      end_addr2     = 0;
      delta2        = 0;
      fetch_count   = 0;
      retire_count  = 0;
      hit2_miss3    = 0;
      hit3_miss2    = 0;
      mv_type = 0;
      br_mv_outcome = 0;
      br_mv_init    = true;
      ldbr  = 0;
      ldbr2 = 0;
      li_data_valid = false;
      li_data2_valid = false;
      for(int i = 0; i < CIR_QUEUE_WINDOW; i++) {
        set_flag[i]  = 0;
        ld_used[i]   = false;
        ldbr_type[i] = 0;
        dep_depth[i] = 0;
        trig_addr[i] = 0;
        set_flag2[i]  = 0;
        ld_used2[i]   = false;
        ldbr_type2[i] = 0;
        dep_depth2[i] = 0;
        trig_addr2[i] = 0;

      }
    }
    //LD fields'
    AddrType seq_start_addr; //start addr of a sequence of LDs (is reset only when we flush on delta change)
    AddrType ldpc;
    AddrType req_addr;
    AddrType start_addr; //start addr when TL is triggered (changes with each TL)
    AddrType end_addr; //end addr when TL is triggered (changes with each TL)
    int64_t delta;
    //
    int ldbr;  //ldbr type
    int fetch_count;
    int retire_count;
    int hit2_miss3;
    int hit3_miss2;
    //br fields
    AddrType brpc;
    DataType br_data1;
    DataType br_data2;
    int br_mv_outcome;
    bool br_mv_init; //when this flag is false, LDBP doesn't swap data mandatorily
    //mv stats
    DataType mv_data;
    int mv_type;
    //circular queues
    std::vector<int> set_flag  = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<bool> ld_used  = std::vector<bool>(CIR_QUEUE_WINDOW);
    std::vector<int> ldbr_type = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<int> dep_depth = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<AddrType> trig_addr = std::vector<AddrType>(CIR_QUEUE_WINDOW);

    //for LD2
    AddrType ldpc2;
    AddrType seq_start_addr2; //start addr of a sequence of LDs (is reset only when we flush on delta change)
    AddrType req_addr2;
    AddrType start_addr2;
    AddrType end_addr2;
    int64_t delta2;
    int ldbr2;
    std::vector<int> set_flag2  = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<bool> ld_used2  = std::vector<bool>(CIR_QUEUE_WINDOW);
    std::vector<int> ldbr_type2 = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<int> dep_depth2 = std::vector<int>(CIR_QUEUE_WINDOW);
    std::vector<AddrType> trig_addr2 = std::vector<AddrType>(CIR_QUEUE_WINDOW);

    //Li Fields
    DataType li_data;
    bool li_data_valid;
    DataType li_data2;
    bool li_data2_valid;

  };

  struct load_data_buffer_entry{
    load_data_buffer_entry() {
      brpc         = 0;
      ldpc         = 0;
      start_addr   = 0;
      end_addr     = 0;
      delta        = 0;
      ldpc2         = 0;
      start_addr2   = 0;
      end_addr2     = 0;
      delta2        = 0;
      for(int i = 0; i < CIR_QUEUE_WINDOW; i++) {
        req_addr[i]  = 0;
        //req_data[i]  = 0;
        marked[i]    = false;
        valid[i]     = false;
        req_addr2[i]  = 0;
        //req_data2[i]  = 0;
        marked2[i]    = false;
        valid2[i]     = false;
      }
    }
    AddrType brpc;
    AddrType ldpc;
    AddrType start_addr;
    AddrType end_addr;
    int64_t delta;
    std::vector<AddrType> req_addr = std::vector<AddrType>(CIR_QUEUE_WINDOW);
    std::vector<DataType> req_data = std::vector<DataType>(CIR_QUEUE_WINDOW);
    std::vector<bool> marked = std::vector<bool>(CIR_QUEUE_WINDOW);
    std::vector<bool> valid  = std::vector<bool>(CIR_QUEUE_WINDOW);
    //for LD2
    AddrType ldpc2;
    AddrType start_addr2;
    AddrType end_addr2;
    int64_t delta2;
    std::vector<AddrType> req_addr2 = std::vector<AddrType>(CIR_QUEUE_WINDOW);
    std::vector<DataType> req_data2 = std::vector<DataType>(CIR_QUEUE_WINDOW);
    std::vector<bool> marked2 = std::vector<bool>(CIR_QUEUE_WINDOW);
    std::vector<bool> valid2  = std::vector<bool>(CIR_QUEUE_WINDOW);

  };

  bool zero_delta; //flag for mreq with delta == 0
  int ret_br_count;

  //std::vector<int> cir_queue = std::vector<int>(CIR_QUEUE_WINDOW);
  std::vector<bot_entry> cir_queue = std::vector<bot_entry>(BOT_SIZE);
  std::vector<load_data_buffer_entry> load_data_buffer = std::vector<load_data_buffer_entry>(LDBUFF_SIZE);
  //std::vector<load_data_buffer_entry> load_data_buffer = std::vector<load_data_buffer_entry>(CIR_QUEUE_WINDOW);
#endif


#endif

  const char *getSection() const {
    return section;
  }
  const char *getName() const {
    return name;
  }
  const char *getDeviceType() const {
    return deviceType;
  }
  uint16_t getID() const {
    return id;
  }
  int16_t getCoreID() const {
    return coreid;
  }
  void setCoreDL1(int16_t cid) {
    coreid        = cid;
    firstLevelDL1 = true;
  }
  void setCoreIL1(int16_t cid) {
    coreid        = cid;
    firstLevelIL1 = true;
  }
  bool isFirstLevel() const {
    return coreid != -1;
  };
  bool isFirstLevelDL1() const {
    return firstLevelDL1;
  };
  bool isFirstLevelIL1() const {
    return firstLevelIL1;
  };

  MRouter *getRouter() {
    return router;
  }

  virtual void tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb = 0) = 0;

  // Interface for fast-forward (no BW, just warmup caches)
  virtual TimeDelta_t ffread(AddrType addr)  = 0;
  virtual TimeDelta_t ffwrite(AddrType addr) = 0;

  // DOWN
  virtual void req(MemRequest *req)         = 0;
  virtual void setStateAck(MemRequest *req) = 0;
  virtual void disp(MemRequest *req)        = 0;

  virtual void doReq(MemRequest *req)         = 0;
  virtual void doSetStateAck(MemRequest *req) = 0;
  virtual void doDisp(MemRequest *req)        = 0;

  // UP
  virtual void blockFill(MemRequest *req);
  virtual void reqAck(MemRequest *req)   = 0;
  virtual void setState(MemRequest *req) = 0;

  virtual void doReqAck(MemRequest *req)   = 0;
  virtual void doSetState(MemRequest *req) = 0;

  virtual bool isBusy(AddrType addr) const = 0;

  // Print stats
  virtual void dump() const;

  // Optional virtual methods
  virtual bool checkL2TLBHit(MemRequest *req);
  virtual void replayCheckLSQ_removeStore(DInst *);
  virtual void updateXCoreStores(AddrType addr);
  virtual void replayflush();
  virtual void setTurboRatio(float r);
  virtual void plug();

  virtual void setNeedsCoherence();
  virtual void clearNeedsCoherence();

  virtual bool Invalid(AddrType addr) const;
#if 0
  virtual bool get_cir_queue(int index ,AddrType pc);
#endif
};

class DummyMemObj : public MemObj {
private:
protected:
public:
  DummyMemObj();
  DummyMemObj(const char *section, const char *sName);

  // Entry points to schedule that may schedule a do?? if needed
  void req(MemRequest *req) {
    doReq(req);
  };
  void reqAck(MemRequest *req) {
    doReqAck(req);
  };
  void setState(MemRequest *req) {
    doSetState(req);
  };
  void setStateAck(MemRequest *req) {
    doSetStateAck(req);
  };
  void disp(MemRequest *req) {
    doDisp(req);
  }

  // This do the real work
  void doReq(MemRequest *req);
  void doReqAck(MemRequest *req);
  void doSetState(MemRequest *req);
  void doSetStateAck(MemRequest *req);
  void doDisp(MemRequest *req);

  TimeDelta_t ffread(AddrType addr);
  TimeDelta_t ffwrite(AddrType addr);

  void tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb = 0);

  bool isBusy(AddrType addr) const;
};

#endif // MEMOBJ_H
