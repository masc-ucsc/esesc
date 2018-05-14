// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2017, Regents of the University of California and
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
#ifndef ADDRESS_PREDICTOR_H
#define ADDRESS_PREDICTOR_H

#include "estl.h"
#include "nanassert.h"

#include "DInst.h"
#include "GStats.h"

//#define DEBUG_STRIDESO2 1
//#define UNLIMITED_BIMODAL 1

#define VTAGE_LOGG 10 /* logsize of the  tagged TAGE tables*/
#define VTAGE_UWIDTH 2
#define VTAGE_CWIDTH 3
#define VTAGE_MAXHIST 14
#define VTAGE_MINHIST 1
#define VTAGE_TBITS 12 // minimum tag width

#define STRIDE_DELTA0 1

class DInst;
class MemObj;

class AddressPredictor {
private:
protected:
  AddressPredictor() {
  }
  AddrType pcSign(AddrType pc) const {
    AddrType cid = pc >> 2;
    cid          = (cid >> 7) ^ (cid);
    return cid;
  }
  uint64_t doHash(AddrType addr, uint64_t offset) {
    uint64_t sign = (addr << 1) ^ offset;
    return sign;
  }

public:
  static AddressPredictor *create(const char *section, const char *str);

  virtual AddrType predict(AddrType pc, int distance, bool inLine = false)   = 0;
  virtual bool     try_chain_predict(MemObj *dl1, AddrType pc, int distance) = 0;
  virtual uint16_t exe_update(AddrType pc, AddrType addr, DataType data = 0) = 0;
  virtual uint16_t ret_update(AddrType pc, AddrType addr, DataType data = 0) = 0;
};

/**********************
 *
STRIDE Address Predictor

 **********************/

class BimodalLastEntry {
public:
  BimodalLastEntry() {
    addr   = 0;
    data   = 0;
    delta  = 0;
    conf   = 0;
    delta2 = 0;
    conf2  = 0;
  }
  DataType data;
  AddrType addr;
  int      delta;
  int      delta2;
  uint16_t conf;
  uint16_t conf2;

  void update(int ndelta);
};

class BimodalStride {
protected:
  const int size;

#ifdef UNLIMITED_BIMODAL
  int get_index(AddrType pc) const {
    return pc;
  };

  HASH_MAP<int, BimodalLastEntry> table;
#else
  int get_index(AddrType pc) const {
    AddrType cid = pc >> 2;
    cid          = (cid >> 7) ^ (cid);
    return cid & (size - 1);
  };

  std::vector<BimodalLastEntry> table;
#endif

public:
  BimodalStride(int _size);

  void update(AddrType pc, AddrType naddr);
  void update_delta(AddrType pc, int delta);
  void update_addr(AddrType pc, AddrType addr);

  uint16_t get_conf(AddrType pc) const {
    return table[get_index(pc)].conf;
  };

  int get_delta(AddrType pc) const {
    return table[get_index(pc)].delta;
  };

  AddrType get_addr(AddrType pc) const {
    return table[get_index(pc)].addr;
  };
};

class StrideAddressPredictor : public AddressPredictor {
private:
  BimodalStride bimodal;

public:
  StrideAddressPredictor();

  AddrType predict(AddrType pc, int distance, bool inLine);
  bool     try_chain_predict(MemObj *dl1, AddrType pc, int distance);
  uint16_t exe_update(AddrType pc, AddrType addr, DataType data);
  uint16_t ret_update(AddrType pc, AddrType addr, DataType data);
};

/*****************************

  VTAGE

 *************************************/

class vtage_gentry {
private:
public:
  int      delta;
  int      conf;
  int      u;
  uint16_t loff; // Signature per LD in the entry

  AddrType tag;
  bool     thit;
  bool     hit;

  uint16_t update_conf;
  vtage_gentry() {
  }

  ~vtage_gentry() {
  }

  void allocate();
  void select(AddrType t, int b);
  bool conf_steal();
  void conf_force_steal(int delta);
  void conf_update(int ndelta);
  void reset(AddrType tag, uint16_t loff, int ndelta);
  bool isHit() const {
    return hit;
  }
  bool isTagHit() const {
    return thit;
  }
  bool isConfident_prefetch() const {
    return get_conf() >= 2;
  }

  int get_conf() const {
    if(!hit)
      return 0; // bias to taken if nothing is known

    return conf;
  }

  bool conf_weak() const {
    if(!hit)
      return true;

    return (conf == 0 || conf == -1);
  }

  bool conf_high() const {
    if(!hit)
      return false;

    return (abs(2 * conf + 1) >= (1 << VTAGE_CWIDTH) - 1);
  }

  int get_delta() const {
    return delta;
  }

  int u_get() const {
    return u;
  }

  void u_inc() {
    if(u < (1 << VTAGE_UWIDTH) - 1)
      u++;
  }

  void u_dec() {
    if(u)
      u--;
  }

  void u_clear() {
    u = 0;
  }
};

class vtage : public AddressPredictor {
private:
protected:
  struct LastAccess {
    AddrType addr;            // May be shorter in reality
    AddrType pc;              // Not needed, just debug
    int      last_delta[256]; // Last Delta, not sure if needed
    uint8_t  last_delta_pos;  // 8 bits to match last_delta size
    uint8_t  last_delta_spec_pos;

    LastAccess() {
      last_delta_pos = 0;
    };

    int get_delta(AddrType _addr) const {
      return _addr - addr;
    };
    void clear_spec() {
      last_delta_spec_pos = last_delta_pos;
    }
    void spec_add(int delta) {
      last_delta_spec_pos++;
      last_delta[last_delta_spec_pos] = delta;
    }
    void reset(AddrType _addr, AddrType _pc) {
      addr                = _addr;
      pc                  = _pc;
      last_delta_spec_pos = last_delta_pos;
    }
    void add(AddrType _addr, AddrType _pc) {
      int ndelta = get_delta(_addr);
      reset(_addr, _pc);

      last_delta_pos++;

      last_delta[last_delta_pos] = ndelta;
      last_delta_spec_pos        = last_delta_pos;
    }
  };

  int get_last_index(AddrType pc) const {
    return pcSign(pc) & 1023;
  };

  LastAccess last[1024]; // FIXME: Use a fix size table (configuration time)

  BimodalStride bimodal;
  GStatsCntr    tagePrefetchHistNum;
  GStatsCntr    tagePrefetchBaseNum;

  int pred_delta; // overall predicted delta
  int pred_conf;
  int hitpred_delta;
  int altpred_delta;

  int hit_bank;
  int alt_bank;

  int *     m;    // [NHIST + 1]; // history lengths
  int *     TB;   //[NHIST + 1];   // tag width for the different tagged tables
  int *     logg; // [NHIST + 1];  // log of number entries of the different tagged tables
  AddrType *GI;   //[NHIST + 1];   // indexes to the different tables are computed only once
  AddrType *GTAG; //[NHIST + 1];    // tags for the different tables are computed only once

  AddrType lastBoundaryPC; // last PC that fetchBoundary was called
  int      TICK;           // for reset of u counter

  AddrType index_tracker;

  bool   use_alt_pred;
  int8_t use_alt_on_na;

  void rename(DInst *dinst);

  uint16_t get_offset(AddrType pc) const {
    return (pc & ((1UL << log2fetchwidth) - 1)) >> 2;
  };
  AddrType get_boundary(AddrType pc) const {
    return (pc >> log2fetchwidth) << log2fetchwidth;
  };
  AddrType get_pc(AddrType bpc, uint16_t offset) const {
    return bpc | (offset << 2);
  };

  void fetchBoundaryLdOffset(AddrType pc);
  void fetchBoundaryBegin(AddrType pc);
  void setVtageIndex(uint16_t offset);

  AddrType gindex(uint16_t offset, int bank);

  void setVtagePrediction(AddrType pc);
  void updateVtage(AddrType pc, int delta, uint16_t loff);

  int            tmp = 0;
  const uint64_t log2fetchwidth;
  const uint64_t nhist; // num of tagged tables
  vtage_gentry **gtable;

public:
  vtage(uint64_t _bsize, uint64_t _log2fetchwidth, uint64_t _bwidth, uint64_t _nhist);

  ~vtage() {
  }

  AddrType predict(AddrType pc, int dist, bool inLine);
  bool     try_chain_predict(MemObj *dl1, AddrType pc, int distance);
  uint16_t exe_update(AddrType pc, AddrType addr, DataType data);
  uint16_t ret_update(AddrType pc, AddrType addr, DataType data);
};

// INDIRECT Address Predictor

class IndirectAddressPredictor : public AddressPredictor {
private:
  bool     chainPredict;
  uint16_t maxPrefetch;

  BimodalStride bimodal;

  void performed(MemObj *DL1, AddrType pc, AddrType ld1_addr);

public:
  typedef CallbackMember3<IndirectAddressPredictor, MemObj *, AddrType, AddrType, &IndirectAddressPredictor::performed> performedCB;
  IndirectAddressPredictor();

  AddrType predict(AddrType pc, int distance, bool inLine);
  bool     try_chain_predict(MemObj *dl1, AddrType pc, int distance);
  uint16_t exe_update(AddrType pc, AddrType addr, DataType data);
  uint16_t ret_update(AddrType pc, AddrType addr, DataType data);
};
#endif
