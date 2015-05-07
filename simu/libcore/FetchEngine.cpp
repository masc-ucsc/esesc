// Contributed by Jose Renau
//                Milos Prvulovic
//                Smruti Sarangi
//                Luis Ceze
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

#include "FetchEngine.h"

#include "SescConf.h"
#include "alloca.h"

#include "MemObj.h"
#include "MemRequest.h"
#include "GMemorySystem.h"

#include "Pipeline.h"
extern bool MIMDmode;


FetchEngine::FetchEngine(FlowID id
  ,GMemorySystem *gms_
  ,FetchEngine *fe)
  :gms(gms_)
  ,avgBranchTime("P(%d)_FetchEngine_avgBranchTime", id)
  ,avgBranchTime2("P(%d)_FetchEngine_avgBranchTime2", id)
  ,avgFetchTime("P(%d)_FetchEngine_avgFetchTime", id)
  ,nDelayInst1("P(%d)_FetchEngine:nDelayInst1", id)
  ,nDelayInst2("P(%d)_FetchEngine:nDelayInst2", id) // Not enough BB/LVIDs per cycle
  ,nDelayInst3("P(%d)_FetchEngine:nDelayInst3", id) // bpredDelay
  ,nFetched("P(%d)_FetchEngine:nFetched", id)
  ,nBTAC("P(%d)_FetchEngine:nBTAC", id) // BTAC corrections to BTB
  //  ,szBB("FetchEngine(%d):szBB", id)
  //  ,szFB("FetchEngine(%d):szFB", id)
  //  ,szFS("FetchEngine(%d):szFS", id)
  //,unBlockFetchCB(this)
  //,unBlockFetchBPredDelayCB(this)
{
  // Constraints
  SescConf->isInt("cpusimu", "fetchWidth",id);
  SescConf->isBetween("cpusimu", "fetchWidth", 1, 1024, id);
  SescConf->isPower2("cpusimu", "fetchWidth", id);
  FetchWidth = SescConf->getInt("cpusimu", "fetchWidth", id);
  FetchWidthBits = log2i(FetchWidth);

  AlignedFetch   = SescConf->getBool("cpusimu","alignedFetch", id);

  SescConf->isBetween("cpusimu", "bb4Cycle",0,1024,id);
  BB4Cycle = SescConf->getInt("cpusimu", "bb4Cycle",id);
  if( BB4Cycle == 0 )
    BB4Cycle = USHRT_MAX;

  const char *bpredSection = SescConf->getCharPtr("cpusimu","bpred",id);

  BTACDelay = SescConf->getInt(bpredSection, "BTACDelay");

  if (BTACDelay)
    SescConf->isBetween("cpusimu", "bpredDelay",1,BTACDelay,id);
  else
    SescConf->isBetween("cpusimu", "bpredDelay",1,1024,id);

  bpredDelay = SescConf->getInt("cpusimu", "bpredDelay",id);


  if( fe )
    bpred = new BPredictor(id, FetchWidth, bpredSection, "", fe->bpred);
  else
    bpred = new BPredictor(id, FetchWidth, bpredSection, "");

  SescConf->isInt(bpredSection, "BTACDelay");
  SescConf->isBetween(bpredSection, "BTACDelay", 0, 1024);


  if (BTACDelay!=0) {
    if (SescConf->checkCharPtr("cpusimu","bpred2",id)) {
      bpredSection = SescConf->getCharPtr("cpusimu","bpred2",id);
      if( fe )
        bpred2 = new BPredictor(id, FetchWidth, bpredSection, "2", fe->bpred2);
      else
        bpred2 = new BPredictor(id, FetchWidth, bpredSection, "2");
    }else{
      bpred2 = 0;
    }
  }else{
    bpred2 = 0;
  }

  lastd     = 0;
  missInst  = false;

  // Get some icache L1 parameters
  enableICache = SescConf->getBool("cpusimu","enableICache", id);
  if (enableICache) {
    // If icache is enabled, do not overchage delay (go to memory cache)
    IL1HitDelay = 0;
  }else{
    const char *iL1Section = SescConf->getCharPtr("cpusimu","IL1", id);
    if (iL1Section) {
      char *sec = strdup(iL1Section);
      char *end = strchr(sec, ' ');
      if (end)
        *end=0; // Get only the first word
      // Must be the i-cache
      SescConf->isInList(sec,"deviceType","icache");

      IL1HitDelay = SescConf->getInt(sec,"hitDelay");
    }else{
      IL1HitDelay = 1; // 1 cycle if impossible to find the information required
    }
  }

  lastMissTime = 0;
}

FetchEngine::~FetchEngine() {
  delete bpred;
}

bool FetchEngine::processBranch(DInst *dinst, uint16_t n2Fetch, uint16_t* to_delete_maxbb) {
  const Instruction *inst = dinst->getInst();

  I(dinst->getInst()->isControl()); // getAddr is target only for br/jmp
  PredType prediction     = bpred->predict(dinst, true);

  if(prediction == CorrectPrediction) {
    if( !dinst->isTaken() ) 
      return false;

    maxBB--;

    // Only when the branch is taken check maxBB
    if (bpredDelay > 1) {
      // Block fetching (not really a miss, but the taken takes time).
      setMissInst(dinst);
      unBlockFetchBPredDelayCB::schedule(bpredDelay-1, this , dinst, globalClock);
      return true;
    }

    if( maxBB < 1 ) {
      // No instructions fetched (stall)
      nDelayInst2.add(n2Fetch, dinst->getStatsFlag());
      return true;
    }

    return false;
  }

  //MSG("2: Dinst not taken Adding PC %x ID %llu\t  ",dinst->getPC(), dinst->getID());
  setMissInst(dinst);
  Time_t n = (globalClock-lastMissTime);
  avgFetchTime.sample(n, dinst->getStatsFlag());
  if( prediction != NoBTBPrediction || BTACDelay == 0) {
    dinst->lockFetch(this);
    return true;
  }

  if( inst->doesJump2Label() ) {
    nBTAC.inc(dinst->getStatsFlag());
    unBlockFetchBPredDelayCB::schedule(BTACDelay,this, dinst,globalClock); // Do not add stats for it
  }else if( bpred2) {
    nBTAC.inc(dinst->getStatsFlag());
    if (bpred2->predict2(dinst, true) == CorrectPrediction) {
      unBlockFetchBPredDelayCB::schedule(BTACDelay,this, dinst,globalClock); // Do not add stats for it
    }else{
      dinst->lockFetch(this);
    }
  }else{
    dinst->lockFetch(this);
  }

  return true;
}

void FetchEngine::realfetch(IBucket *bucket, EmulInterface *eint, FlowID fid, int32_t n2Fetch, uint16_t maxbb) {

  uint16_t tempmaxbb = maxbb; // FIXME: delete me

  AddrType lastpc = 0;
  bool     lastdiff = false;

#if 0
  RegType  last_dest = LREG_R0;
  RegType  last_src1 = LREG_R0;
  RegType  last_src2 = LREG_R0;
#endif

  do {
    DInst *dinst = 0;
    dinst = eint->executeHead(fid);
    if (dinst == 0) {
      //if (fid)
      //I(0);
      break;
    }

#if 1
    if (!dinst->getStatsFlag() && dinst->getPC() == 0) {
      if (dinst->getInst()->isLoad()) {
        MemRequest::sendReqReadWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      } else if (dinst->getInst()->isStore()) {
        MemRequest::sendReqWriteWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      }
    }
    if (dinst == 0) { // Drain cache (mostly) during warmup. FIXME: add a drain cache method?
      EventScheduler::advanceClock();
      EventScheduler::advanceClock();
      EventScheduler::advanceClock();
      EventScheduler::advanceClock();
      EventScheduler::advanceClock();
      EventScheduler::advanceClock();
      continue;
    }
#endif
    if (lastpc == 0) {
      if (AlignedFetch) {
        n2Fetch -= ((dinst->getPC())>>FetchWidthBits) & (FetchWidth-1);
      }
      n2Fetch--;
      lastdiff = false;
    }else{
      if ((lastpc+4) != dinst->getPC()) {
        //        n2Fetch -= (dinst->getPC()-lastpc)>>2;
        n2Fetch--;
        if (lastdiff) {
          n2Fetch--; // Missed NOP
        }
        lastdiff = true;
      }else{
        n2Fetch--;
        lastdiff = false;
      }
    }
    lastpc  = dinst->getPC();




    I(!missInst);

    dinst->setFetchTime();
    bucket->push(dinst);
        
    if(dinst->getInst()->isControl()) {
      bool stall_fetch = processBranch(dinst, n2Fetch,&tempmaxbb);
      if (stall_fetch) {
        //bucket->push(dinst);
        break;
      }
#if 0
      RegType src1 = dinst->getInst()->getSrc1();
      if (dinst->getInst()->doesJump2Label() && dinst->getInst()->getSrc2() == LREG_R0 
          && (src1 == last_dest || src1 == last_src1 || src1 == last_src2 || src1 == LREG_R0) ) {
        //MSG("pc %x fusion with previous", dinst->getPC());
        dinst->scrap(eint);
      }else{
        bucket->push(dinst);
      }
#endif
      I(!missInst);
    }else{
      //bucket->push(dinst);
    }
#if 0
    last_dest = dinst->getInst()->getDst1();
    last_src1 = dinst->getInst()->getSrc1();
    last_src2 = dinst->getInst()->getSrc2();
#endif

    // Fetch uses getHead, ROB retires getTail
  } while(n2Fetch>0);

  if(enableICache && !bucket->empty()) {
    nFetched.add(FetchWidth - n2Fetch, bucket->top()->getStatsFlag());
    MemRequest::sendReqRead(gms->getIL1(), bucket->top()->getStatsFlag(), bucket->top()->getPC(), &(bucket->markFetchedCB));
  }else{
    bucket->markFetchedCB.schedule(IL1HitDelay);
#if 0
    if (bucket->top() != NULL)
    MSG("@%lld: Bucket [%p] (Top ID = %d) to be markFetched after %d cycles i.e. @%lld"
        ,(long long int)globalClock
        , bucket
        , (int) bucket->top()->getID()
        , IL1HitDelay
        , (long long int)(globalClock)+IL1HitDelay
        );
#endif
  }

}

void FetchEngine::fetch(IBucket *bucket, EmulInterface *eint, FlowID fid) {

  // Reset the max number of BB to fetch in this cycle (decreased in processBranch)
  maxBB = BB4Cycle;

  // You pass maxBB because there may be many fetches calls to realfetch in one cycle
  // (thanks to the callbacks)
  realfetch(bucket, eint, fid, FetchWidth,maxBB);

}


void FetchEngine::dump(const char *str) const {
  char *nstr = (char *)alloca(strlen(str) + 20);
  sprintf(nstr, "%s_FE", str);
  bpred->dump(nstr);
}

void FetchEngine::unBlockFetchBPredDelay(DInst* dinst, Time_t missFetchTime) {
  clearMissInst(dinst, missFetchTime);

  Time_t n = (globalClock-missFetchTime);
  avgBranchTime2.sample(n, dinst->getStatsFlag()); // Not short branches
  //n *= FetchWidth; // FOR CPU
  n *= 1; //FOR GPU

  nDelayInst3.add(n, dinst->getStatsFlag());
}


void FetchEngine::unBlockFetch(DInst* dinst, Time_t missFetchTime) {
  clearMissInst(dinst, missFetchTime);

  I(missFetchTime != 0);
  Time_t n = (globalClock-missFetchTime);
  avgBranchTime.sample(n, dinst->getStatsFlag()); // Not short branches
  //n *= FetchWidth;  //FOR CPU
  n *= 1; //FOR GPU and for MIMD
  nDelayInst1.add(n, dinst->getStatsFlag());

  lastMissTime = globalClock;
}


void FetchEngine::clearMissInst(DInst * dinst, Time_t missFetchTime){

  //MSG("\t\t\t\t\tCPU: %d\tU:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getID(),(int) dinst->getID(),dinst->getPE(),dinst->getPC());
  missInst = false;

  I(lastd == dinst);
  cbPending.mycall();
}

void FetchEngine::setMissInst(DInst * dinst) {
  //MSG("CPU: %d\tL:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getID(),(int) dinst->getID(),dinst->getPE(),dinst->getPC());

  I(!missInst);

  missInst      = true;
  lastd         = dinst;
}

