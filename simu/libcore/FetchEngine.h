// Contributed by Jose Renau
//                Milos Prvulovic
//                Smruti Sarangi
//                Luis Ceze
//                Kerry Veenstra
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

#ifndef FETCHENGINE_H
#define FETCHENGINE_H

#include "AddressPredictor.h"
#include "EmulInterface.h"
#include "nanassert.h"

#include "BPred.h"
#include "GStats.h"
//#define ENABLE_LDBP

class GMemorySystem;
class IBucket;

class FetchEngine {
private:
  GMemorySystem *const gms;

  BPredictor *bpred;

  uint16_t FetchWidth;
  uint16_t FetchWidthBits;
  uint16_t Fetch2Width;
  uint16_t Fetch2WidthBits;

#ifdef ESESC_TRACE_DATA

  AddressPredictor *ideal_apred;
  struct OracleDataLastEntry {
    OracleDataLastEntry() {
      addr    = 0;
      data    = 0;
      delta0  = false;
      used    = 0;
      chained = 0;
    }
    DataType data;
    AddrType addr;
    uint8_t  used;
    int      chained;
    bool     delta0;

    void clear(DataType d, AddrType a) {
      data = d;
      addr = a;
      used = 0;
    }
    void set(DataType d, AddrType a) {
      data   = d;
      delta0 = (addr == a);
      addr   = a;
      if(used > 0)
        used--;
    }

    bool isChained() const {
      return used >= 6;
    }
    void chain() {
      if(used < 6)
        used += 2;
      used = 0; // disable chaining loads
    }

    int inc_chain() {
      chained++;
      return chained;
    }
    void dec_chain() {
      if(chained == 0)
        return;
      chained--;
    }
  };

  AddrType lastPredictable_ldpc;
  AddrType lastPredictable_addr;
  DataType lastPredictable_data;

  SCTable lastData;

  struct OracleDataRATEntry {
    OracleDataRATEntry() {
      ldpc  = 0;
      depth = 32768;
    }
    AddrType ldpc;
    int      depth;
  };
  HASH_MAP<AddrType, OracleDataLastEntry> oracleDataLast;
  OracleDataRATEntry                      oracleDataRAT[LREG_MAX];

  HASH_MAP<AddrType, AddrType> ldpc2brpc; // FIXME: Only a small table of address to track
#endif

  bool TargetInLine;
  bool FetchOneLine;
  bool AlignedFetch;
  bool TraceAlign;

  uint16_t BB4Cycle;
  uint16_t maxBB;

  TimeDelta_t IL1HitDelay;
  uint16_t    LineSize;
  uint16_t    LineSizeBits;

  // InstID of the address that generated a misprediction

  bool missInst; // branch missprediction. Stop fetching until solved
  ID(DInst *missDInst);
  CallbackContainer cbPending;

  Time_t lastMissTime; // FIXME: maybe we need an array

  bool enableICache;

protected:
  // bool processBranch(DInst *dinst, uint16_t n2Fetched);
  bool processBranch(DInst *dinst, uint16_t n2Fetchedi);

  // ******************* Statistics section
  GStatsAvg  avgFetchLost;
  GStatsAvg  avgBranchTime;
  GStatsAvg  avgBranchTime2;
  GStatsAvg  avgFetchTime;
  GStatsAvg  avgFetched;
  GStatsCntr nDelayInst1;
  GStatsCntr nDelayInst2;
  GStatsCntr nDelayInst3;
  GStatsCntr nBTAC;
  GStatsCntr zeroDinst;
#ifdef ESESC_TRACE_DATA
  GStatsHist dataHist;
  GStatsHist dataSignHist;
  GStatsHist nbranchMissHist;
  GStatsHist nLoadData_per_branch;
  GStatsHist nLoadAddr_per_branch;

#endif
  // *******************

public:
  FetchEngine(FlowID i, GMemorySystem *gms, FetchEngine *fe = 0);

  ~FetchEngine();

#ifdef ENABLE_LDBP

  DInst* init_ldbp(DInst *dinst, DataType dd, AddrType ldpc);
  MemObj *DL1;
  DInst *ld_dinst;
  AddrType dep_pc; //dependent instn's PC
  int fetch_br_count;

  AddrType pref_addr;
  AddrType check_line_addr;
  AddrType base_addr;
  AddrType tmp_base_addr;
  uint64_t p_delta;
  uint64_t inf;
  uint64_t constant;
#endif

  void fetch(IBucket *buffer, EmulInterface *eint, FlowID fid);

  typedef CallbackMember3<FetchEngine, IBucket *, EmulInterface *, FlowID, &FetchEngine::fetch> fetchCB;

  void realfetch(IBucket *buffer, EmulInterface *eint, FlowID fid, int32_t n2Fetched);
  // typedef CallbackMember4<FetchEngine, IBucket *, EmulInterface* , FlowID, int32_t, &FetchEngine::realfetch>  realfetchCB;

  void chainPrefDone(AddrType pc, int distance, AddrType addr);
  void chainLoadDone(DInst *dinst);
  typedef CallbackMember3<FetchEngine, AddrType, int, AddrType, &FetchEngine::chainPrefDone> chainPrefDoneCB;

  void unBlockFetch(DInst *dinst, Time_t missFetchTime);
  typedef CallbackMember2<FetchEngine, DInst *, Time_t, &FetchEngine::unBlockFetch> unBlockFetchCB;

  void unBlockFetchBPredDelay(DInst *dinst, Time_t missFetchTime);
  typedef CallbackMember2<FetchEngine, DInst *, Time_t, &FetchEngine::unBlockFetchBPredDelay> unBlockFetchBPredDelayCB;

#if 0
  void unBlockFetch();
  StaticCallbackMember0<FetchEngine,&FetchEngine::unBlockFetch> unBlockFetchCB;

  void unBlockFetchBPredDelay();
  StaticCallbackMember0<FetchEngine,&FetchEngine::unBlockFetchBPredDelay> unBlockFetchBPredDelayCB;
#endif

  void dump(const char *str) const;

  bool isBlocked() const {
    return missInst;
  }
#ifdef DEBUG
  DInst *getMissDInst() const {
    return missDInst;
  }
#endif

  void clearMissInst(DInst *dinst, Time_t missFetchTime);
  void setMissInst(DInst *dinst);
};

#endif // FETCHENGINE_H
