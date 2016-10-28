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

#include "nanassert.h"

#include "EmulInterface.h"

#include "BPred.h"
#include "GStats.h"

class GMemorySystem;
class IBucket;

class FetchEngine {
private:

  GMemorySystem *const gms;

  BPredictor   *bpred;

  uint16_t      FetchWidth;
  uint16_t      FetchWidthBits;
  uint16_t      Fetch2Width;
  uint16_t      Fetch2WidthBits;

  bool          TargetInLine;
  bool          FetchOneLine;
	bool          AlignedFetch;
	bool          TraceAlign;

  uint16_t      BB4Cycle;
  uint16_t      maxBB;

  TimeDelta_t   IL1HitDelay;
  uint16_t      LineSize;
  uint16_t      LineSizeBits;

  // InstID of the address that generated a misprediction
 
  bool              missInst; // branch missprediction. Stop fetching until solved
  CallbackContainer cbPending;

  Time_t    lastMissTime; // FIXME: maybe we need an array

  bool      enableICache;
 
protected:
  //bool processBranch(DInst *dinst, uint16_t n2Fetched);
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
  // *******************

public:
  FetchEngine(FlowID i
              ,GMemorySystem *gms
              ,FetchEngine *fe = 0);

  ~FetchEngine();

  void fetch(IBucket *buffer, EmulInterface *eint, FlowID fid);
  typedef CallbackMember3<FetchEngine, IBucket *, EmulInterface* , FlowID, &FetchEngine::fetch>  fetchCB;

  void realfetch(IBucket *buffer, EmulInterface *eint, FlowID fid, int32_t n2Fetched);
  //typedef CallbackMember4<FetchEngine, IBucket *, EmulInterface* , FlowID, int32_t, &FetchEngine::realfetch>  realfetchCB;

  void unBlockFetch(DInst* dinst, Time_t missFetchTime);
  typedef CallbackMember2<FetchEngine, DInst*, Time_t,  &FetchEngine::unBlockFetch> unBlockFetchCB;

  void unBlockFetchBPredDelay(DInst* dinst, Time_t missFetchTime);
  typedef CallbackMember2<FetchEngine, DInst* , Time_t, &FetchEngine::unBlockFetchBPredDelay> unBlockFetchBPredDelayCB;

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

  void clearMissInst(DInst * dinst, Time_t missFetchTime);
  void setMissInst(DInst * dinst);
};

#endif   // FETCHENGINE_H
