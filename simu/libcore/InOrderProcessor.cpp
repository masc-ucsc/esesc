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

#include "TaskHandler.h"
#include "InOrderProcessor.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "ClusterManager.h"

InOrderProcessor::InOrderProcessor(GMemorySystem *gm, CPU_t i)
  :GProcessor(gm, i, 1)
  ,IFID(i, this, gm)
  ,pipeQ(i)
  ,lsq(i)
  ,rROB(SescConf->getInt("cpusimu", "robSize", i))
  ,clusterManager(gm, this)
{
  spaceInInstQueue = InstQueueSize;
  bzero(RAT,sizeof(DInst*)*LREG_MAX);

  busy = false;
}

InOrderProcessor::~InOrderProcessor() {
  // Nothing to do
}

void InOrderProcessor::fetch(FlowID fid) {
  I(eint);
  if(!active){
    TaskHandler::removeFromRunning(cpu_id);
    return;
  }

  if( IFID.isBlocked(0)) {
    busy = true;
  }else{
    IBucket *bucket = pipeQ.pipeLine.newItem();
    if( bucket ) {
      IFID.fetch(bucket, eint, fid);
      if (!bucket->empty()) {
        busy = true;
      }
    }
  }
}

bool InOrderProcessor::execute() {

  if (!active) {
    // time to remove from the running queue
    TaskHandler::removeFromRunning(cpu_id);
    return false;
  }
 

  if (!busy) {
    return false;
  }

  bool getStatsFlag = false;
  if( !ROB.empty() ) {
    getStatsFlag = ROB.top()->getStatsFlag();
  }
  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  // ID Stage (insert to instQueue)
  if (spaceInInstQueue >= FetchWidth) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      //      I(bucket->top()->getInst()->getAddr());

      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);
    }else{
      noFetch2.inc(getStatsFlag);
    }
  }else{
    noFetch.inc(getStatsFlag);
  }

  // RENAME Stage
  if ( !pipeQ.instQueue.empty() ) {
    spaceInInstQueue += issue(pipeQ);
  }else if (ROB.empty() && rROB.empty()) {
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    return true;
  }

  retire();

  return true;
}

StallCause InOrderProcessor::addInst(DInst *dinst) {

  const Instruction *inst = dinst->getInst();

#ifdef ESESC_FUZE
  // FIXME: IANLEE1521 - Need to do this over all srcs in the iterator.
  if(RAT[inst->getSrc(0)] != 0 ||
     RAT[inst->getSrc(1)] != 0 ||
     RAT[inst->getDst(0)] != 0 ||
     RAT[inst->getDst(1)] != 0) {
    return SmallWinStall;
  }
#else
  if(RAT[inst->getSrc1()] != 0 ||
     RAT[inst->getSrc2()] != 0 ||
     RAT[inst->getDst1()] != 0 ||
     RAT[inst->getDst2()] != 0) {
    return SmallWinStall;
  }
#endif

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

  nInst[inst->getOpcode()]->inc(dinst->getStatsFlag()); // FIXME: move to cluster

  ROB.push(dinst);

  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field

  // No src check for in-order
#ifdef ESESC_FUZE
  // FIXME: IANLEE1521 - Need to do this over all srcs in the iterator.
  dinst->setRAT1Entry(&RAT[inst->getDst(0)]);
  dinst->setRAT2Entry(&RAT[inst->getDst(1)]);
  RAT[inst->getDst(0)] = dinst;
  RAT[inst->getDst(1)] = dinst;
#else  
  dinst->setRAT1Entry(&RAT[inst->getDst1()]);
  dinst->setRAT2Entry(&RAT[inst->getDst2()]);
  RAT[inst->getDst1()] = dinst;
  RAT[inst->getDst2()] = dinst;
#endif

  I(dinst->getCluster());
  dinst->getCluster()->addInst(dinst);

  return NoStall;
}

void InOrderProcessor::retire() {

  // Pass all the ready instructions to the rrob
  bool stats = false;
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();
    stats = dinst->getStatsFlag();

    if( !dinst->isExecuted() )
      break; 

    bool done = dinst->getClusterResource()->preretire(dinst, false);
    if( !done )
      break;

    rROB.push(dinst);
    ROB.pop();

    nCommitted.inc(dinst->getStatsFlag());
  }

  robUsed.sample(ROB.size(), stats);
  rrobUsed.sample(rROB.size(), stats);

  for(uint16_t i=0 ; i<RetireWidth && !rROB.empty() ; i++) {
    DInst *dinst = rROB.top();

    if (!dinst->isExecuted())
      break;

    I(dinst->getCluster());

    bool done = dinst->getCluster()->retire(dinst, false);
    if( !done ) {
      //dinst->getInst()->dump("not ret");
      return;
    }

#if 0
    FlowID fid = dinst->getFlowId();
    if( active) {
      EmulInterface *eint = TaskHandler::getEmul(fid);
      eint->reexecuteTail( fid );
    }
#endif

    dinst->destroy(eint);
    rROB.pop();
  }

}

void InOrderProcessor::replay(DInst *dinst) {

  MSG("Inorder cores do not support replays. Set NoMemoryReplay = true at the confguration");

  // FIXME: foo should be equal to the number of in-flight instructions (check OoOProcessor)
  size_t foo= 1;
  nReplayInst.sample(foo, dinst->getStatsFlag());

  // FIXME: How do we manage a replay in this processor??
}
