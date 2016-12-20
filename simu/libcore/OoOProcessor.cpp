// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Milos Prvulovic
//                Luis Ceze
//                Islam Atta (IBM)
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

#include <math.h>

#include "SescConf.h"

#include "OoOProcessor.h"

#include "TaskHandler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "EmuSampler.h"

/* }}} */
//#define ESESC_CODEPROFILE
// FIXME: to avoid deadlock, prealloc n to the n oldest instructions
//#define LATE_ALLOC_REGISTER

OoOProcessor::OoOProcessor(GMemorySystem *gm, CPU_t i)
  /* constructor {{{1 */
  :GOoOProcessor(gm, i)
  ,MemoryReplay(SescConf->getBool("cpusimu", "MemoryReplay",i))
  ,RetireDelay(SescConf->getInt("cpusimu", "RetireDelay",i))
  ,IFID(i, gm)
  ,pipeQ(i)
  ,lsq(i, SescConf->checkInt("cpusimu", "maxLSQ",i)?SescConf->getInt("cpusimu", "maxLSQ",i):32768) // 32K (unlimited or fix)
  ,retire_lock_checkCB(this)
  ,clusterManager(gm, i, this)
  ,avgFetchWidth("P(%d)_avgFetchWidth",i)
#ifdef TRACK_FORWARDING
  ,avgNumSrc("P(%d)_avgNumSrc",i)
  ,avgNumDep("P(%d)_avgNumDep",i)
  ,fwd0done0("P(%d)_fwd0done0",i)
  ,fwd1done0("P(%d)_fwd1done0",i)
  ,fwd1done1("P(%d)_fwd1done1",i)
  ,fwd2done0("P(%d)_fwd2done0",i)
  ,fwd2done1("P(%d)_fwd2done1",i)
  ,fwd2done2("P(%d)_fwd2done2",i)
#endif
  ,codeProfile("P(%d)_prof",i)
{
  bzero(RAT,sizeof(DInst*)*LREG_MAX);
  bzero(serializeRAT,sizeof(DInst*)*LREG_MAX);
#ifdef TRACK_FORWARDING
  bzero(fwdDone, sizeof(Time_t)*LREG_MAX);
#endif

  spaceInInstQueue = InstQueueSize;

  codeProfile_trigger = 0;

  nTotalRegs = SescConf->getInt("cpusimu","nTotalRegs", gm->getCoreId());
  if (nTotalRegs == 0)
    nTotalRegs = 1024*1024*1024; // Unlimited :)

  busy             = false;
  flushing         = false;
  replayRecovering = false;
  getStatsFlag     = false;
  replayID         = 0;

  last_state.dinst_ID = 0xdeadbeef;

  if (SescConf->checkInt("cpusimu","serialize",i))
    serialize= SescConf->getInt("cpusimu","serialize",i);
  else
    serialize= 0;

  serialize_level = 2; // 0 full, 1 all ld, 2 same reg  
  serialize_for   = 0;
  last_serialized = 0;
  last_serializedST = 0;
  forwardProg_threshold = 200;
  if (SescConf->checkBool("cpusimu", "scooreMemory" , gm->getCoreId()))
    scooreMemory=SescConf->getBool("cpusimu", "scooreMemory",gm->getCoreId());
  else
    scooreMemory = false;
}
/* }}} */

OoOProcessor::~OoOProcessor()
 /* destructor {{{1 */
{
  // Nothing to do
}
/* }}} */

void OoOProcessor::fetch(FlowID fid)
  /* fetch {{{1 */
{
  I(fid == cpu_id);
  I(active);
  I(eint);

  if( IFID.isBlocked()) {
//    I(0);
    busy = true;
  }else{
    IBucket *bucket = pipeQ.pipeLine.newItem();
    if( bucket ) {
      IFID.fetch(bucket, eint, fid);
      if (!bucket->empty()) {
        avgFetchWidth.sample(bucket->size(), bucket->top()->getStatsFlag());
        busy = true;
      }
    }
  }
}
/* }}} */

bool OoOProcessor::advance_clock(FlowID fid)
  /* Full execution: fetch|rename|retire {{{1 */
{

  if (!active) {
    // time to remove from the running queue
    //TaskHandler::removeFromRunning(cpu_id);
    return false;
  }

  fetch(fid);

  if( !ROB.empty() ) {
    // Else use last time
    getStatsFlag = ROB.top()->getStatsFlag();
  }

  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  if (!busy)
    return false;

  if (unlikely(throttlingRatio>1)) { 
    throttling_cntr++;

    uint32_t skip = (uint32_t)ceil(throttlingRatio/getTurboRatio()); 

    if (throttling_cntr < skip) {
      return true;
    }
    throttling_cntr = 1;
  }

  // ID Stage (insert to instQueue)
  if( spaceInInstQueue >= FetchWidth ) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      //      I(bucket->top()->getInst()->getAddr());

      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);

      //GMSG(getID()==1,"instqueue insert %p", bucket);
    }else{
      noFetch2.inc(getStatsFlag);
    }
  }else{
    noFetch.inc(getStatsFlag);
  }

  // RENAME Stage
  if( replayRecovering ) {
    if ((rROB.empty() && ROB.empty())) {
      // Recovering done
      EmulInterface *eint = TaskHandler::getEmul(flushing_fid);
      eint->syncHeadTail( flushing_fid );

      I(flushing);
      replayRecovering = false;
      flushing         = false;

      if ((lastReplay+2*forwardProg_threshold) < replayID){
        serialize_level = 3; // One over max to start with 2
        //MSG("%d Reset Serialize level @%lld : %lld %lld",cpu_id, globalClock,lastReplay, replayID);
      }
      if ((lastReplay+forwardProg_threshold) > replayID){
        if (serialize_level) {
            //MSG("%d One level less %d for %d @%lld : %lld %lld", cpu_id, serialize_level, serialize_for, globalClock, lastReplay, replayID);
          serialize_level--;
        }else{
          //MSG("%d already at level 0 @%lld", cpu_id, globalClock);
        }
        serialize_for = serialize;
        //forwardProg_threshold = replayID - lastReplay;
        //serialize_for = forwardProg_threshold; 
      }
      
      lastReplay = replayID;
    }else{
      nStall[ReplaysStall]->add(RealisticWidth, getStatsFlag);
      retire();
      return true;
    }
  }
  
  if( !pipeQ.instQueue.empty() ) {
    spaceInInstQueue += issue(pipeQ);
  }else if( ROB.empty() && rROB.empty() ) {
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    return true;
  }

  retire();

  return true;
}
/* }}} */

void OoOProcessor::executing(DInst *dinst) 
  // {{{1 Called when the instruction starts to execute
{
#ifdef LATE_ALLOC_REGISTER
    if (dinst->getInst()->hasDstRegister())
      nTotalRegs--;
#endif
#ifdef TRACK_FORWARDING
    if (dinst->getStatsFlag()) {
      const Instruction *inst = dinst->getInst();
      avgNumSrc.sample(inst->getnsrc(), true);

      int nForward = 0;
      int nNeeded  = 0;
      if (inst->hasSrc1Register()) {
        nNeeded++;
        Time_t t = fwdDone[inst->getSrc1()];
        if ((t+2) >= globalClock)
          nForward++;
      }
      if (inst->hasSrc2Register()) {
        nNeeded++;
        Time_t t = fwdDone[inst->getSrc2()];
        if ((t+2) >= globalClock)
          nForward++;
      }

      if (nNeeded == 0)
        fwd0done0.inc(true);
      else if (nNeeded == 1) {
        if (nForward)
          fwd1done1.inc(true);
        else
          fwd1done0.inc(true);
      }else{
        if (nForward==2)
          fwd2done2.inc(true);
        else if (nForward==1)
          fwd2done1.inc(true);
        else
          fwd2done0.inc(true);
      }
    }
#endif
}
// 1}}}
//
void OoOProcessor::executed(DInst *dinst) {
#ifdef TRACK_FORWARDING
  fwdDone[dinst->getInst()->getDst1()] = globalClock;
  fwdDone[dinst->getInst()->getDst2()] = globalClock;
#endif
}

StallCause OoOProcessor::addInst(DInst *dinst)
  /* rename (or addInst) a new instruction {{{1 */
{
  if(replayRecovering && dinst->getID() > replayID) {
    return ReplaysStall;
  }

  if( (ROB.size()+rROB.size()) >= MaxROBSize )
    return SmallROBStall;

  const Instruction *inst = dinst->getInst();

  if (nTotalRegs<=0) 
    return SmallREGStall;

  Cluster *cluster = dinst->getCluster();
  if( !cluster ) {
    Resource *res = clusterManager.getResource(dinst);
    cluster       = res->getCluster();
    dinst->setCluster(cluster, res);
  }

  StallCause sc = cluster->canIssue(dinst);
  if (sc != NoStall) {
    return sc;
  }

  // BEGIN INSERTION (note that cluster already inserted in the window)
  // dinst->dump("");

#ifndef LATE_ALLOC_REGISTER
  if (inst->hasDstRegister()) {
    nTotalRegs--;
  }
#endif

  //#if 1
  if(!scooreMemory){ //no dynamic serialization for tradcore
    if (serialize_for>0 && !replayRecovering) {
      serialize_for--;
      if (inst->isMemory() && dinst->isSrc3Ready()) {
        if (last_serialized && !last_serialized->isExecuted()) {
          //last_serialized->addSrc3(dinst); FIXME
          //MSG("addDep3 %8ld->%8lld %lld",last_serialized->getID(), dinst->getID(), globalClock);
        }
        last_serialized = dinst;
      } 
    }
    //#else
  }else{
    if (serialize_for>0 && !replayRecovering) {
      serialize_for--;

      if (serialize_level==0) {
        // Serialize all the memory operations
        if (inst->isMemory() && dinst->isSrc3Ready()) {
          if (last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serialized = dinst;
        } 
      }else if (serialize_level==1) {
        // Serialize stores, and loads depend on stores (no loads on loads)
        if (inst->isLoad() && dinst->isSrc3Ready()) {
          if (last_serializedST && !last_serializedST->isIssued()) {
            last_serializedST->addSrc3(dinst);
          }
          last_serialized = dinst;
        } 
        if (inst->isStore() && dinst->isSrc3Ready()) {
          if (last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serializedST = dinst;
        } 
      }else{
        // Serialize if same register is being accessed
        if (inst->getSrc1() < LREG_ARCH0) {
          last_serializeLogical = inst->getSrc1();
        }else if (last_serializePC != dinst->getPC()) {
          last_serializeLogical = LREG_InvalidOutput;
        }
        last_serializePC      = dinst->getPC();

        if (last_serializeLogical < LREG_ARCH0) {
          if (inst->isMemory()) { 
            if (serializeRAT[last_serializeLogical]) {
              if (inst->isLoad()) {
                if (serializeRAT[last_serializeLogical]->getInst()->isStore())
                  serializeRAT[last_serializeLogical]->addSrc3(dinst);
              }else{
                serializeRAT[last_serializeLogical]->addSrc3(dinst);
              }
            }

            dinst->setSerializeEntry(&serializeRAT[last_serializeLogical]);
            serializeRAT[last_serializeLogical] = dinst;
          }else{
            serializeRAT[inst->getDst1()] = 0;
            serializeRAT[inst->getDst2()] = 0;
          }
        } 
      }
    }
  }
 //#endif

  nInst[inst->getOpcode()]->inc(dinst->getStatsFlag()); // FIXME: move to cluster

  ROB.push(dinst);

  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field

  int n = 0;
  if( !dinst->isSrc2Ready() ) {
    // It already has a src2 dep. It means that it is solved at
    // retirement (Memory consistency. coherence issues)
    if( RAT[inst->getSrc1()] ) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      //MSG("addDep0 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }
  }else{
    if( RAT[inst->getSrc1()] ) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      //MSG("addDep1 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }

    if( RAT[inst->getSrc2()] ) {
      RAT[inst->getSrc2()]->addSrc2(dinst);
      n++;
      //MSG("addDep2 %8ld->%8lld %lld",RAT[inst->getSrc2()]->getID(), dinst->getID(), globalClock);
    }
  }
  avgNumSrc.sample(inst->getnsrc(), dinst->getStatsFlag());
  avgNumDep.sample(n,dinst->getStatsFlag());

  dinst->setRAT1Entry(&RAT[inst->getDst1()]);
  dinst->setRAT2Entry(&RAT[inst->getDst2()]);

  I(!dinst->isExecuted());

  dinst->getCluster()->addInst(dinst);

  if(!dinst->isExecuted()) {
    RAT[inst->getDst1()] = dinst;
    RAT[inst->getDst2()] = dinst;
  }

  I(dinst->getCluster());

  dinst->markRenamed();

  return NoStall;
}
/* }}} */

void OoOProcessor::retire_lock_check()
  /* Detect simulator locks and flush the pipeline {{{1 */
{
  RetireState state;
  if (active) {
    state.committed = nCommitted.getDouble();
  }else{
    state.committed = 0;
  }
  if (!rROB.empty()) {
    state.r_dinst    = rROB.top();
    state.r_dinst_ID = rROB.top()->getID();
  }

  if (!ROB.empty()) {
    state.dinst    = ROB.top();
    state.dinst_ID = ROB.top()->getID();
  }

  if (last_state == state && active) {
    I(0);
    MSG("Lock detected in P(%d), flushing pipeline", getID());
  }

  last_state = state;

  retire_lock_checkCB.scheduleAbs(globalClock + 100000);
}
/* }}} */

void OoOProcessor::retire()
  /* Try to retire instructions {{{1 */
{
  // Pass all the ready instructions to the rrob
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();

    bool done = dinst->getClusterResource()->preretire(dinst, flushing);
    GI(flushing && dinst->isExecuted(), done);
    if( !done ) {
      break;
    }

    rROB.push(dinst);
    ROB.pop();
  }

  if(!ROB.empty())
    robUsed.sample(ROB.size(), ROB.top()->getStatsFlag());

  if(!rROB.empty()) {
    rrobUsed.sample(rROB.size(), rROB.top()->getStatsFlag());

#ifdef ESESC_CODEPROFILE
    if (rROB.top()->getStatsFlag()) {
      if (codeProfile_trigger<=clockTicks.getDouble()) {
        DInst *dinst = rROB.top();

        codeProfile_trigger = clockTicks.getDouble() + 121;

        double wt = dinst->getIssuedTime() - dinst->getRenamedTime();
        double et = dinst->getExecutedTime() - dinst->getIssuedTime();
        bool   flush = dinst->getFetch()!=0?1:0;

        codeProfile.sample(rROB.top()->getPC(),nCommitted.getDouble(), clockTicks.getDouble(), wt, et, flush);
      }
    }
#endif
  }

  for(uint16_t i=0 ; i<RetireWidth && !rROB.empty() ; i++) {
    DInst *dinst = rROB.top();

    if ((dinst->getExecutedTime()+RetireDelay) >= globalClock) {
#if 0
      if (rROB.size()>8) {
        dinst->getInst()->dump("not ret");
        printf("----------------------\n");
        dumpROB();
      }
#endif
      break;
    }

    I(dinst->isExecuted());

    GI(!flushing, dinst->isExecuted());
    I(dinst->getCluster());

    bool done = dinst->getCluster()->retire(dinst, flushing);
    if( !done ) 
      break;

#if 0
    static int conta=0;
    if ((globalClock-dinst->getExecutedTime())>500)
      conta++;
    if (conta > 1000) {
      dinst->getInst()->dump("not ret");
      conta = 0;
      dumpROB();
    }
#endif

    FlowID fid = dinst->getFlowId();
    if( dinst->isReplay() ) {
      flushing = true;
      flushing_fid = fid;
    }

    nCommitted.inc(!flushing && dinst->getStatsFlag());

#ifdef ESESC_TRACE
    MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d   %lld %lld %lld %lld %lld"
        ,dinst->getID()
        ,dinst->getPC()
        ,dinst->getInst()->getDst1()
        ,dinst->getInst()->getDst2()
        ,dinst->getInst()->getSrc1()
        ,dinst->getInst()->getOpcode()
        ,dinst->getInst()->getSrc2()
        ,dinst->getFetchedTime()
        ,dinst->getRenamedTime()
        ,dinst->getIssuedTime()
        ,dinst->getExecutedTime()
        ,globalClock);
#endif

#if 0
    dinst->dump("RT ");
    fprintf(stderr,"\n");
#endif

    if (dinst->getInst()->hasDstRegister())
      nTotalRegs++;

    if (!dinst->getInst()->isStore()) // Stores can perform after retirement
      I(dinst->isPerformed());

    if (dinst->isPerformed()) // Stores can perform after retirement
      dinst->destroy(eint);
    else{
      eint->reexecuteTail(fid);
    }

    if (last_serialized == dinst)
      last_serialized = 0;
    if (last_serializedST == dinst)
      last_serializedST = 0;

    rROB.pop();
  }
}
/* }}} */

void OoOProcessor::replay(DInst *target)
  /* trigger a processor replay {{{1 */
{
  if (serialize_for)
    return;

  I(serialize_for<=0);
  // Same load can be marked by several stores in a OoO core : I(replayID != target->getID());
  I(target->getInst()->isLoad());

  if( !MemoryReplay ) {
    return;
  }
  target->markReplay();

  if (replayID < target->getID())
    replayID = target->getID();

  if( replayRecovering )
    return;
  replayRecovering = true; 

  // Count the # instructions wasted
  size_t fetch2rename = 0;
  fetch2rename += (InstQueueSize-spaceInInstQueue);
  fetch2rename += pipeQ.pipeLine.size();

  nReplayInst.sample(fetch2rename+ROB.size(), target->getStatsFlag());
}
/* }}} */

void OoOProcessor::dumpROB()
  // {{{1 Dump rob statistics
{
#if 0
  uint32_t size = ROB.size();
  fprintf(stderr,"ROB: (%d)\n",size);

  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = ROB.getIDFromTop(i);

    DInst *dinst = ROB.getData(pos);
    dinst->dump("");
  }

  size = rROB.size();
  fprintf(stderr,"rROB: (%d)\n",size);
  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = rROB.getIDFromTop(i);

    DInst *dinst = rROB.getData(pos);
    if (dinst->isReplay())
      printf("-----REPLAY--------\n");
    dinst->dump("");
  }
#endif
}
// 1}}}

