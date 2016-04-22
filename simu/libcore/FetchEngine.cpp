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

#define ENABLE_FAST_WARMUP 1
//#define FETCH_TRACE 1

FetchEngine::FetchEngine(FlowID id
  ,GMemorySystem *gms_
  ,FetchEngine *fe)
  :gms(gms_)
  ,avgFetchLost("P(%d)_FetchEngine_avgFetchLost", id)
  ,avgBranchTime("P(%d)_FetchEngine_avgBranchTime", id)
  ,avgBranchTime2("P(%d)_FetchEngine_avgBranchTime2", id)
  ,avgFetchTime("P(%d)_FetchEngine_avgFetchTime", id)
  ,nDelayInst1("P(%d)_FetchEngine:nDelayInst1", id)
  ,nDelayInst2("P(%d)_FetchEngine:nDelayInst2", id) // Not enough BB/LVIDs per cycle
  ,nDelayInst3("P(%d)_FetchEngine:nDelayInst3", id) 
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

  FetchWidth     = SescConf->getInt("cpusimu", "fetchWidth", id);
  FetchWidthBits = log2i(FetchWidth);

  Fetch2Width     = FetchWidth/2;
  Fetch2WidthBits = log2i(Fetch2Width);

  AlignedFetch   = SescConf->getBool("cpusimu","alignedFetch", id);
  if ( SescConf->checkBool("cpusimu","traceAlign", id)) {
    TraceAlign = SescConf->getBool("cpusimu","TraceAlign", id);
  }else{
    TraceAlign = false;
  }

  SescConf->isBetween("cpusimu", "bb4Cycle",0,1024,id);
  BB4Cycle = SescConf->getInt("cpusimu", "bb4Cycle",id);
  if( BB4Cycle == 0 )
    BB4Cycle = USHRT_MAX;

  if( fe )
    bpred = new BPredictor(id, gms->getIL1(), fe->bpred);
  else
    bpred = new BPredictor(id, gms->getIL1());

  lastd     = 0;
  missInst  = false;

  const char *isection = SescConf->getCharPtr("cpusimu","IL1", id);
  const char *pos = strchr(isection,' ');
  if (pos) {
    char *pos = strdup(isection);
    *strchr(pos,' ') = 0;
    isection = pos;
  }

  const char *itype    = SescConf->getCharPtr(isection,"deviceType");
  
  if (strcasecmp(itype,"icache")!=0) {
    isection = SescConf->getCharPtr(isection,"lowerLevel");
    const char *pos = strchr(isection,' ');
    if (pos) {
      char *pos = strdup(isection);
      *strchr(pos,' ') = 0;
      isection = pos;
    }
    itype    = SescConf->getCharPtr(isection,"deviceType");
    if (strcasecmp(itype,"icache")!=0) {
      MSG("ERROR: the icache should be the first or second after IL1 in the core");
      SescConf->notCorrect();
    }
  }
  lineSize = SescConf->getInt(isection,"bsize");

  // Get some icache L1 parameters
  enableICache = SescConf->getBool("cpusimu","enableICache", id);
  IL1HitDelay = SescConf->getInt(isection,"hitDelay");

  lastMissTime = 0;
}

FetchEngine::~FetchEngine() {
  delete bpred;
}

bool FetchEngine::processBranch(DInst *dinst, uint16_t n2Fetch) {
  const Instruction *inst = dinst->getInst();

  I(dinst->getInst()->isControl()); // getAddr is target only for br/jmp

  bool fastfix;
  TimeDelta_t  delay = bpred->predict(dinst, &fastfix);
  if (delay==0)
    return false;

  setMissInst(dinst);

  Time_t n = (globalClock-lastMissTime);
  avgFetchTime.sample(n, dinst->getStatsFlag());

  if (fastfix)
    unBlockFetchBPredDelayCB::schedule(delay, this , dinst, globalClock);
  else 
    dinst->lockFetch(this);

  return true;
}

void FetchEngine::realfetch(IBucket *bucket, EmulInterface *eint, FlowID fid, int32_t n2Fetch) {

  AddrType lastpc = 0;

#ifdef USE_FUSE
  RegType  last_dest = LREG_R0;
  RegType  last_src1 = LREG_R0;
  RegType  last_src2 = LREG_R0;
#endif

  do {
    DInst *dinst = 0;
    dinst = eint->peekHead(fid);
    if (dinst == 0)
      break;

#ifdef ENABLE_FAST_WARMUP
    if (dinst->getPC() == 0) {
      eint->executeHead(fid); // Consume it for sure! (warmup mode)

      do{
        EventScheduler::advanceClock();
      }while(gms->getDL1()->isBusy(dinst->getAddr()));

      if (dinst->getInst()->isLoad()) {
        MemRequest::sendReqReadWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      } else if (dinst->getInst()->isStore()) {
        MemRequest::sendReqWriteWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      }else{
        I(0);
        dinst->scrap(eint);
        dinst = 0;
      }
      continue;
    }
#endif
    if (lastpc == 0) {
      bpred->fetchBoundaryBegin(dinst);
      if (!TraceAlign) {

        uint64_t entryPC=dinst->getPC()>>2;

        uint16_t fetchLost;

        if (AlignedFetch) {
          fetchLost =  (entryPC) & (FetchWidth-1);
        }else{
          fetchLost =  (entryPC) & (Fetch2Width-1);
        }

        // No matter what, do not pass cache line boundary
        uint16_t fetchMaxPos = (entryPC & (lineSize/4-1)) + FetchWidth; 
        if (fetchMaxPos>lineSize/4) {
          fetchLost += (fetchMaxPos-lineSize/4);
        }

        avgFetchLost.sample(fetchLost,dinst->getStatsFlag());

        n2Fetch -= fetchLost;
      }

      n2Fetch--;
    }else{
      if ((lastpc+4) == dinst->getPC()) {
        n2Fetch--;
      }else if ((lastpc+8) != dinst->getPC()) {
        maxBB--;
        if( maxBB < 1 ) {
          nDelayInst2.add(n2Fetch, dinst->getStatsFlag());
          break;
        }
        n2Fetch--; // There may be a NOP in the delay slot, do not count it
      }else{
        n2Fetch-=2; // NOP still consumes delay slot
      }
    }
    lastpc  = dinst->getPC();

    I(!missInst);
    eint->executeHead(fid); // Consume for sure

    dinst->setFetchTime();
    bucket->push(dinst);
        
#ifdef USE_FUSE
    if(dinst->getInst()->isControl()) {
      RegType src1 = dinst->getInst()->getSrc1();
      if (dinst->getInst()->doesJump2Label() && dinst->getInst()->getSrc2() == LREG_R0 
          && (src1 == last_dest || src1 == last_src1 || src1 == last_src2 || src1 == LREG_R0) ) {
        //MSG("pc %x fusion with previous", dinst->getPC());
        dinst->scrap(eint);
        continue;
      }
    }
#endif

#ifdef FETCH_TRACE
    static int bias_ninst   = 0;
    static int bias_firstPC = 0;
    bias_ninst++;
#endif

#ifdef FETCH_TRACE
    dinst->dump("FECHT");
#endif
        
    if(dinst->getInst()->isControl()) {
      bool stall_fetch = processBranch(dinst, n2Fetch);
      if (stall_fetch) {
        DInst *dinstn = eint->peekHead(fid);
        if (dinstn && dinst->getAddr() && n2Fetch) { // Taken branch and next inst is delay slot
          if (dinstn->getPC() == (lastpc+4)) {
            bucket->push(eint->executeHead(fid));
            n2Fetch--;
          }
        }
#ifdef FETCH_TRACE
        if (dinst->isBiasBranch() && dinst->getFetch()) {
          // OOPS. Thought that it was bias and it is a long misspredict
          MSG("ABORT first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
        if (bias_ninst>256) {
          MSG("1ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
#endif
        break;
      }
      I(!missInst);
#ifdef FETCH_TRACE
      if (bias_ninst>256) {
        MSG("2ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
        bias_firstPC = dinst->getAddr();
        bias_ninst   = 0;
      }else if (!dinst->isBiasBranch() ) {

        if ( dinst->isTaken() && (dinst->getAddr() > dinst->getPC() && (dinst->getAddr() + 8<<2) <= dinst->getPC())) {
          // Move instructions to predicated (up to 8)
          MSG("PRED  first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
        }else{
          if (bias_ninst<16) {
            MSG("NOTRA first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          }else{
            MSG("3ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          }
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
      }
#endif
    }

#ifdef USE_FUSE
    last_dest = dinst->getInst()->getDst1();
    last_src1 = dinst->getInst()->getSrc1();
    last_src2 = dinst->getInst()->getSrc2();
#endif

    // Fetch uses getHead, ROB retires getTail
  } while(n2Fetch>0);

  bpred->fetchBoundaryEnd();

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
  realfetch(bucket, eint, fid, FetchWidth);

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
  Time_t n = (globalClock-missFetchTime)-1;
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

