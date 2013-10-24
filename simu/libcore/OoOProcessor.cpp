/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz

   Contributed by Jose Renau
                  Basilio Fraguela
                  James Tuck
                  Milos Prvulovic
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

#include "OoOProcessor.h"

#include "TaskHandler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "EmuSampler.h"

/* }}} */

OoOProcessor::OoOProcessor(GMemorySystem *gm, CPU_t i)
  /* constructor {{{1 */
  :GOoOProcessor(gm, i)
  ,NoMemoryReplay(SescConf->getBool("cpusimu", "NoMemoryReplay",i))
  ,IFID(i, this, gm)
  ,pipeQ(i)
  ,lsq(i)
  ,retire_lock_checkCB(this)
  ,clusterManager(gm, this)
  ,serialize_level_hist("P(%d)_serialize_level_hist",i)
  ,avgFetchWidth("P(%d)_avgFetchWidth",i)
{
  bzero(RAT,sizeof(DInst*)*LREG_MAX);
  bzero(serializeRAT,sizeof(DInst*)*LREG_MAX);

  spaceInInstQueue = InstQueueSize;

  busy             = false;
  flushing         = false;
  replayRecovering = false;
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
  if (SescConf->checkBool("cpusimu", "scooreMemory" , gm->getId()))
    scooreMemory=SescConf->getBool("cpusimu", "scooreMemory",gm->getId());
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

  if(!active){
  //  TaskHandler::removeFromRunning(cpu_id);
    return;
  }

  I(eint);

  if( IFID.isBlocked(0)) {
//    I(0);
    busy = true;
  }else{
    IBucket *bucket = pipeQ.pipeLine.newItem();
    if( bucket ) {
      IFID.fetch(bucket, eint, fid);
      if (!bucket->empty()) {
        avgFetchWidth.sample(bucket->size());
        busy = true;
      }
    }
  }
}
/* }}} */

bool OoOProcessor::execute()
  /* Full execution: fetch|rename|retire {{{1 */
{

  if (!active) {
    // time to remove from the running queue
    //TaskHandler::removeFromRunning(cpu_id);
    return false;
  }

  if (!busy) {
    if (!active) {
      // time to remove from the running queue
      //TaskHandler::removeFromRunning(cpu_id);
    }
    return false;
  }

  bool getStatsFlag = false;
  if( !ROB.empty() ) {
    getStatsFlag = ROB.top()->getStatsFlag();
  }

  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  if (unlikely(throtting)) { 
    throtting_cntr++;

    uint32_t skip = ceil(throtting/getTurboRatio()); 

    if (throtting_cntr < skip) {
      return true;
    }
    throtting_cntr = 1;
  }

  // ID Stage (insert to instQueue)
  if( spaceInInstQueue >= FetchWidth ) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      //      I(bucket->top()->getInst()->getAddr());

      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);

      //GMSG(getId()==1,"instqueue insert %p", bucket);
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
      
      nReplayDist.sample(replayID - lastReplay);
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

StallCause OoOProcessor::addInst(DInst *dinst)
  /* rename (or addInst) a new instruction {{{1 */
{
  if(replayRecovering && dinst->getID() > replayID) {
    return ReplaysStall;
  }

  if( (ROB.size()+rROB.size()) >= MaxROBSize )
    return SmallROBStall;

  Cluster *cluster = dinst->getCluster();
  if( !cluster ) {
    Resource *res = clusterManager.getResource(dinst);
    cluster       = res->getCluster();
    dinst->setCluster(cluster, res);
  }

  StallCause sc = cluster->canIssue(dinst);
  if (sc != NoStall)
    return sc;


  // BEGIN INSERTION (note that cluster already inserted in the window)
  // dinst->dump("");

  const Instruction *inst = dinst->getInst();

  //#if 1
  if(!scooreMemory){ //no dynamic serialization for tradcore
    if (serialize_for>0 && !replayRecovering) {
      serialize_for--;
      if (inst->isMemory() && dinst->isSrc3Ready()) {
        if (last_serialized && !last_serialized->isExecuted()) {
          last_serialized->addSrc3(dinst);
        }
        last_serialized = dinst;
      } 
    }
    //#else
  }else{
    if (serialize_for>0 && !replayRecovering) {
      serialize_for--;

      serialize_level_hist.sample(serialize_level);
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

  if( !dinst->isSrc2Ready() ) {
    // It already has a src2 dep. It means that it is solved at
    // retirement (Memory consistency. coherence issues)
    if( RAT[inst->getSrc1()] )
      RAT[inst->getSrc1()]->addSrc1(dinst);
  }else{
    if( RAT[inst->getSrc1()] )
      RAT[inst->getSrc1()]->addSrc1(dinst);

    if( RAT[inst->getSrc2()] )
      RAT[inst->getSrc2()]->addSrc2(dinst);
  }

  dinst->setRAT1Entry(&RAT[inst->getDst1()]);
  dinst->setRAT2Entry(&RAT[inst->getDst2()]);
  RAT[inst->getDst1()] = dinst;
  RAT[inst->getDst2()] = dinst;

  I(dinst->getCluster());
  dinst->getCluster()->addInst(dinst);

  return NoStall;
}
/* }}} */

void OoOProcessor::retire_lock_check()
  /* Detect simulator locks and flush the pipeline {{{1 */
{
  RetireState state;
  if (active) {
    eint->getSampler()->syncStats();
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
    MSG("Lock detected in P(%d), flushing pipeline", getId());
    if (!rROB.empty()) {
//      replay(rROB.top());
    }
    if (!ROB.empty()) {
//      ROB.top()->markExecuted();
//      replay(ROB.top());
    }
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

    if( !dinst->isExecuted() )
      break;

    bool done = dinst->getClusterResource()->preretire(dinst, flushing);
    GI(flushing, done);
    if( !done )
      break;

    rROB.push(dinst);
    ROB.pop();

  }

  if(!ROB.empty())
    robUsed.sample(ROB.size(), ROB.top()->getStatsFlag());

  if(!rROB.empty())
    rrobUsed.sample(rROB.size(), rROB.top()->getStatsFlag());

  for(uint16_t i=0 ; i<RetireWidth && !rROB.empty() ; i++) {
    DInst *dinst = rROB.top();

    if (!dinst->isExecuted())
      break;
    
    GI(!flushing, dinst->isExecuted());
    I(dinst->getCluster());
 
    bool done = dinst->getCluster()->retire(dinst, flushing);
    if( !done ) {
      //dinst->getInst()->dump("not ret");
      return;
    }
    
    FlowID fid = dinst->getFlowId();
    if( dinst->isReplay() ) {
      flushing = true;
      flushing_fid = fid;
    }
    if (!flushing) {
      nCommitted.inc(dinst->getStatsFlag());
    }

    //dinst->dump("destroy");
    dinst->destroy(eint);

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

  if( NoMemoryReplay ) {
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
{
  uint32_t size = ROB.size();
  printf("ROB: (%d)\n",size);

  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = ROB.getIdFromTop(i);

    DInst *dinst = ROB.getData(pos);
    dinst->dump("");
  }

  size = rROB.size();
  printf("rROB: (%d)\n",size);
  for(uint32_t i=0;i<size;i++) {
    uint32_t pos = rROB.getIdFromTop(i);

    DInst *dinst = rROB.getData(pos);
    if (dinst->isReplay())
      printf("-----REPLAY--------\n");
    dinst->dump("");
  }
}
