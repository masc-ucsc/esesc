/*
ESESC: Super ESCalar simulator
Copyright (C) 2010 University of California, Santa Cruz.

Contributed by Jose Renau
Milos Prvulovic
Smruti Sarangi
Luis Ceze

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

#include "SescConf.h"
#include "alloca.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "MemRequest.h"
#include "GProcessor.h"
#include "Pipeline.h"

FetchEngine::FetchEngine(FlowID id
  ,GProcessor *gproc_
  ,GMemorySystem *gms_
  ,FetchEngine *fe)
  :gms(gms_)
  ,gproc(gproc_)
  ,avgBranchTime("P(%d)_FetchEngine_avgBranchTime", id)
  ,avgInstsFetched("P(%d)_FetchEngine_avgInstsFetched", id)
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
  FetchWidth = SescConf->getInt("cpusimu", "fetchWidth", id);

  SescConf->isBetween("cpusimu", "bb4Cycle",0,1024,id);
  BB4Cycle = SescConf->getInt("cpusimu", "bb4Cycle",id);
  if( BB4Cycle == 0 )
    BB4Cycle = USHRT_MAX;
  SescConf->isBetween("cpusimu", "bpredDelay",1,1024,id);
  bpredDelay = SescConf->getInt("cpusimu", "bpredDelay",id);

  const char *bpredSection = SescConf->getCharPtr("cpusimu","bpred",id);

  if( fe )
    bpred = new BPredictor(id, FetchWidth, bpredSection, fe->bpred);
  else
    bpred = new BPredictor(id, FetchWidth, bpredSection);

  SescConf->isInt(bpredSection, "BTACDelay");
  SescConf->isBetween(bpredSection, "BTACDelay", 0, 1024);

  BTACDelay = SescConf->getInt(bpredSection, "BTACDelay");

  //Supporting multiple SPs (processing elements) in the processor
  if(SescConf->checkInt("cpusimu", "sp_per_sm", id)) {
    numSP = SescConf->getInt("cpusimu", "sp_per_sm", id); 
  } else {
    // What about an SMT-like processor?
    numSP = 1;
  } 

  missInst  = new bool[numSP];
  lastd     = new DInst* [numSP];
  cbPending = new CallbackContainer[numSP];

  for (uint32_t i = 0; i < numSP; i++){
    missInst[i]      = false;
  }

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
}

FetchEngine::~FetchEngine() {
  delete bpred;
}

bool FetchEngine::processBranch(DInst *dinst, uint16_t n2Fetched, uint16_t* to_delete_maxbb) {
  const Instruction *inst = dinst->getInst();
  Time_t missFetchTime=0;

  I(dinst->getInst()->isControl()); // getAddr is target only for br/jmp
  PredType prediction     = bpred->predict(dinst, true);

  if(prediction == CorrectPrediction) {
    if( dinst->isTaken() ) {
      bool dobreak = false;
      // Only when the branch is taken check maxBB
      if (bpredDelay > 1) {
        // Block fetching (not really a miss, but the taken takes time).
        missFetchTime = globalClock;
        setMissInst(dinst);
        unBlockFetchBPredDelayCB::schedule(bpredDelay-1, this , dinst, missFetchTime);
        dobreak = true;
      }

      if( maxBB <= 1 ) {
        // No instructions fetched (stall)
        if (!missInst[dinst->getPE()]) {
          nDelayInst2.add(n2Fetched, dinst->getStatsFlag());
          //I(0);
          return true;
        }
      }
      maxBB--;
      return dobreak;
    }
    return false;
  }

  I(!missInst[dinst->getPE()]);
  I(missFetchTime==0);

  missFetchTime = globalClock;

  //MSG("2: Dinst not taken Adding PC %x ID %llu\t  ",dinst->getPC(), dinst->getID());
  setMissInst(dinst);

  if( BTACDelay ) {
    if( prediction == NoBTBPrediction && inst->doesJump2Label() ) {
      nBTAC.inc(dinst->getStatsFlag());
      unBlockFetchBPredDelayCB::schedule(BTACDelay,this, dinst,globalClock); // Do not add stats for it
    }else{
      dinst->lockFetch(this); // blocked fetch (awaked in Resources)
    }
  }else{
    dinst->lockFetch(this); // blocked fetch (awaked in Resources)
  }

  return true;
}

void FetchEngine::realfetch(IBucket *bucket, EmulInterface *eint, FlowID fid, DInst* oldinst, int32_t n2Fetched, uint16_t maxbb) {

  uint16_t tempmaxbb = maxbb; // FIXME: delete me
  bool getStatsFlag = false;

  if (oldinst) {
    if (missInst[oldinst->getPE()]) {
      if (lastd[oldinst->getPE()] != oldinst) {
        cbPending[oldinst->getPE()].add(realfetchCB::create(this,bucket,eint,fid,oldinst,n2Fetched,tempmaxbb));
      }else{
        I(0);
      }
      return;
    }
    
    if(oldinst->getInst()->isControl()) {
      processBranch(oldinst, n2Fetched,&tempmaxbb);
    }

   bucket->push(oldinst);
   n2Fetched--;

  } else {
    do {
      DInst *dinst = eint->executeHead(fid);
      if (dinst == 0) {
        //if (fid)
          //I(0);
        break;
      }
      getStatsFlag |= dinst->getStatsFlag();

      if (missInst[dinst->getPE()]) {
        I(lastd[dinst->getPE()] != dinst);
        //MSG("Oops! FID: %d PE: %d is locked now..Adding DInst ID %d to a list of callback[%d]", (int) fid, (int) dinst->getPE(), (int) dinst->getID(),(int) dinst->getPE()); 
        cbPending[dinst->getPE()].add(realfetchCB::create(this,bucket,eint,fid,dinst,n2Fetched,tempmaxbb));
        return;
      }


      bucket->push(dinst);
      n2Fetched--;

      if(dinst->getInst()->isControl()) {
        bool stall_fetch = processBranch(dinst, n2Fetched,&tempmaxbb);
        if (stall_fetch) {
          break;
        }
        I(!missInst[dinst->getPE()]);
      }

      // Fetch uses getHead, ROB retires getTail
    } while(n2Fetched>0);
  }

  uint16_t tmp = FetchWidth - n2Fetched;

  nFetched.add(tmp, getStatsFlag);

  if(enableICache && !bucket->empty()) {
    MemRequest *mreq=MemRequest::createRead(gms->getIL1(), bucket->top(), bucket->top()->getPC(), &(bucket->markFetchedCB));
    mreq->fwdRead();
  }else{
    bucket->markFetchedCB.schedule(IL1HitDelay);
  }

}

void FetchEngine::fetch(IBucket *bucket, EmulInterface *eint, FlowID fid) {

  // Reset the max number of BB to fetch in this cycle (decreased in processBranch)
  maxBB = BB4Cycle; 

  // You pass maxBB because there may be many fetches calls to realfetch in one cycle (thanks to the callbacks) 
  realfetch(bucket, eint, fid, NULL, FetchWidth,maxBB);

}


void FetchEngine::dump(const char *str) const {
  char *nstr = (char *)alloca(strlen(str) + 20);
  sprintf(nstr, "%s_FE", str);
  bpred->dump(nstr);
}

void FetchEngine::unBlockFetchBPredDelay(DInst* dinst, Time_t missFetchTime) {
  clearMissInst(dinst);

  Time_t n = (globalClock-missFetchTime);
  //n *= FetchWidth; // FOR CPU
  n *= 1; //FOR GPU

  nDelayInst3.add(n, dinst->getStatsFlag());
}


void FetchEngine::unBlockFetch(DInst* dinst, Time_t missFetchTime) {
  clearMissInst(dinst);

  I(missFetchTime != 0);
  Time_t n = (globalClock-missFetchTime);
  avgBranchTime.sample(n, dinst->getStatsFlag());
  //n *= FetchWidth;  //FOR CPU
  n *= 1; //FOR GPU
  nDelayInst1.add(n, dinst->getStatsFlag());
}


void FetchEngine::clearMissInst(DInst * dinst) {
  //MSG("\t\t\t\t\tCPU: %d\tU:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getId(),(int) dinst->getID(),dinst->getPE(),dinst->getPC());
  missInst[dinst->getPE()] = false;

  I(lastd[ dinst->getPE()] == dinst);
  //cbPending[dinst->getPE()].call();
  cbPending[dinst->getPE()].mycall();
}

void FetchEngine::setMissInst(DInst * dinst) {
  //MSG("CPU: %d\tL:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getId(),(int) dinst->getID(),dinst->getPE(),dinst->getPC());

  I(!missInst[dinst->getPE()]);
  missInst[dinst->getPE()] = true;
  lastd[ dinst->getPE()] = dinst;
} 

