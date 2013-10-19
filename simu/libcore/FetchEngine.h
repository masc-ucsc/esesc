/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Milos Prvulovic
                  Smruti Sarangi
                  Luis Ceze
                  Kerry Veenstra

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

#ifndef FETCHENGINE_H
#define FETCHENGINE_H

#include "nanassert.h"

#include "EmulInterface.h"

#include "BPred.h"
#include "GStats.h"

class GMemorySystem;
class IBucket;

// A FetchEngine holds only one execution flow. An SMT processor
// should instantiate several FetchEngines.

class GProcessor;

class FetchEngine {
private:

  GMemorySystem *const gms;
  GProcessor    *const gproc;

  BPredictor   *bpred;

  uint16_t      FetchWidth;

  uint16_t      BB4Cycle;
  uint16_t      bpredDelay;
  uint16_t      maxBB;

  TimeDelta_t   BTACDelay;
  TimeDelta_t   IL1HitDelay;

  // InstID of the address that generated a misprediction
 
  uint32_t  numSP;
  bool      *missInst; // branch missprediction. Stop fetching until solved
  DInst     **lastd;
  CallbackContainer *cbPending;

  AddrType  missInstPC;

  bool      enableICache;
 
protected:
  //bool processBranch(DInst *dinst, uint16_t n2Fetched);
  bool processBranch(DInst *dinst, uint16_t n2Fetchedi, uint16_t* maxbb);

  // ******************* Statistics section
  GStatsAvg  avgBranchTime;
  GStatsAvg  avgInstsFetched;
  GStatsCntr nDelayInst1;
  GStatsCntr nDelayInst2;
  GStatsCntr nDelayInst3;
  GStatsCntr nFetched;
  GStatsCntr nBTAC;
  // *******************

public:
  FetchEngine(FlowID i
              ,GProcessor *gproc
              ,GMemorySystem *gms
              ,FetchEngine *fe = 0);

  ~FetchEngine();

  void fetch(IBucket *buffer, EmulInterface *eint, FlowID fid);
  typedef CallbackMember3<FetchEngine, IBucket *, EmulInterface* , FlowID, &FetchEngine::fetch>  fetchCB;

  void realfetch(IBucket *buffer, EmulInterface *eint, FlowID fid, DInst* oldinst, int32_t n2Fetched, uint16_t maxbb);
  typedef CallbackMember6<FetchEngine, IBucket *, EmulInterface* , FlowID, DInst*, int32_t, uint16_t, &FetchEngine::realfetch>  realfetchCB;

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


  bool isBlocked(DInst* inst) const {
    for (uint32_t i = 0; i < numSP; i++){
      if (missInst[i] == false)
        return false;
    }
    return true;
  }

  void clearMissInst(DInst * dinst);
  void setMissInst(DInst * dinst);
    /*
  void clearMissInst(DInst * dinst) {
    MSG("UnLocking Dinst ID: %llu,DInst PE:%d, Dinst PC %x\n",dinst->getID(),dinst->getPE(),dinst->getPC());
    missInst[dinst->getPE()] = false;
  }

  void setMissInst(DInst * dinst) {
    MSG("CPU: %d\tLocking Dinst ID: %llu, DInst PE:%d, Dinst PC %x",(int) this->gproc->getId(), dinst->getID(),dinst->getPE(),dinst->getPC());
    I(dinst->getPE()!=0);
    I(!missInst[dinst->getPE()]);
    missInst[dinst->getPE()] = true;
    lastd = dinst;
  }
*/
};

#endif   // FETCHENGINE_H
