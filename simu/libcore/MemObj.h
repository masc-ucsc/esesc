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
#define CIR_QUEUE_WINDOW 128 //FIXME: need to change this to a conf variable
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
  struct load_data_buffer_entry{
    load_data_buffer_entry() {
      ld_addr    = 0;
      ld_data    = 0;
      marked     = false;
      ld_br_type = 0;
    }
    AddrType ld_addr;
    DataType ld_data;
    bool marked;
    int ld_br_type;

    void reset() {
      ld_addr    = 0;
      ld_data    = 0;
      marked     = false;
      ld_br_type = 0;
    }

    void fill_ld_br_type(int _lb) {
      ld_br_type = _lb; 
    }

    void fill_addr(AddrType addr) {
      ld_addr = addr; 
    }

    void fill_data(DataType data) {
      ld_data = data; 
    }
  };

  AddrType q_start_addr;
  AddrType q_end_addr;
  uint64_t q_delta;
  AddrType curr_dep_pc;
  bool zero_delta; //flag for mreq with delta == 0
  int ret_br_count;
  //std::vector<std::vector<int>> cir_queue = std::vector<std::vector<int>>(4, std::vector<int>(CIR_QUEUE_WINDOW, 0));
  //std::vector<std::vector<load_data_buffer_entry>> load_data_buffer = std::vector<std::vector<load_data_buffer_entry>>(64, std::vector<load_data_buffer_entry>(CIR_QUEUE_WINDOW, load_data_buffer_entry()));
  std::vector<int> cir_queue = std::vector<int>(CIR_QUEUE_WINDOW);
  std::vector<load_data_buffer_entry> load_data_buffer = std::vector<load_data_buffer_entry>(CIR_QUEUE_WINDOW);

  void find_cir_queue_index(MemRequest *mreq, const char *str);
  void reset_cir_queue();
  void reset_load_data_buffer();
  void shift_cir_queue();
  void shift_load_data_buffer();
  void fill_cir_queue(MemRequest *mreq, int index);

  int getQSize() {
    return CIR_QUEUE_WINDOW;
  }

  void setQDelta(uint64_t _delta) {
    q_delta = _delta;
  }

  AddrType getQDelta() const {
    return q_delta;
  }

  void setRetBrCount(int cnt) {
    ret_br_count = cnt;
  }

  int getRetBrCount() const {
    return ret_br_count;
  }

  void setQStartAddr(AddrType _addr) {
    q_start_addr = _addr;
  }

  AddrType getQStartAddr() const {
    return q_start_addr;
  }

  void setQEndAddr(AddrType _addr) {
    q_end_addr = _addr;
  }

  AddrType getQEndAddr() const {
    return q_end_addr;
  }
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
  virtual bool get_cir_queue(int index ,AddrType pc);
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
