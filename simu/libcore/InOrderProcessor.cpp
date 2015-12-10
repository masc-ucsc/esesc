//  Contributed by Jose Renau
//                 Basilio Fraguela
//                 James Tuck
//                 Milos Prvulovic
//                 Luis Ceze
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

#include "SescConf.h"

#include "TaskHandler.h"
#include "InOrderProcessor.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "ClusterManager.h"

InOrderProcessor::InOrderProcessor(GMemorySystem *gm, CPU_t i)
  :GProcessor(gm, i)
  ,IFID(i, gm)
  ,pipeQ(i)
  ,lsq(i,32768)
  ,rROB(SescConf->getInt("cpusimu", "robSize", i))
  ,clusterManager(gm, this)
{/*{{{*/


  spaceInInstQueue = InstQueueSize;

  uint32_t smtnum = getMaxFlows();
  RAT = new DInst* [LREG_MAX * smtnum * 128];
  bzero(RAT,sizeof(DInst*)*LREG_MAX * smtnum * 128);

  busy = false;/*}}}*/
}/*}}}*/

InOrderProcessor::~InOrderProcessor()
{/*{{{*/
  delete RAT;
  // Nothing to do
}/*}}}*/

void InOrderProcessor::fetch(FlowID fid)
{/*{{{*/
  // TODO: Move this to GProcessor (same as in OoOProcessor)
  I(eint);
  I(active);

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
}/*}}}*/

bool InOrderProcessor::advance_clock(FlowID fid)
{/*{{{*/

  if (!active) {
    // time to remove from the running queue
    //TaskHandler::removeFromRunning(cpu_id);
    return false;
  }
  //IS(if (cpu_id == 1) MSG("\n**\n@%lld: Processing CPU (%d)",(long long int)globalClock,cpu_id));
  fetch(fid);

  if (!busy)
    return false;

  bool getStatsFlag = false;
  if( !ROB.empty() ) {
    getStatsFlag = ROB.top()->getStatsFlag();
  }
  //IS(if (cpu_id == 1) MSG("@%lld: Fetching CPU (%d)",(long long int)globalClock,cpu_id));
  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  // ID Stage (insert to instQueue)
  if (spaceInInstQueue >= FetchWidth) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      //IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) fetched bucket size is %d ",(long long int)globalClock,cpu_id,bucket->size()));
      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);
    }else{
      //IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) Empty Bucket ",(long long int)globalClock,cpu_id));
      noFetch2.inc(getStatsFlag);
    }
  }else{
    //IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) NO space in InstQueue",(long long int)globalClock,cpu_id));
    noFetch.inc(getStatsFlag);
  }

  //IS(if (cpu_id == 1) MSG("@%lld: Renaming CPU (%d)",(long long int)globalClock,cpu_id));
  // RENAME Stage
  if ( !pipeQ.instQueue.empty() ) {
    uint32_t n_insn =  issue(pipeQ);
    spaceInInstQueue += n_insn;
    //IS(if (cpu_id == 1) MSG("@%lld: Issuing %d items in pipeline for CPU (%d)",(long long int)globalClock,n_insn, cpu_id));
  }else if (ROB.empty() && rROB.empty()) {
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    //IS(if (cpu_id == 1) MSG("@%lld: ROB and rROB are both empty for CPU (%d)",(long long int)globalClock,cpu_id));
    //IS(if ((cpu_id == 1)&&(busy)) MSG("@%lld: pipeline also has outstanding items CPU (%d)",(long long int)globalClock,cpu_id));
    return true;
  } else {
    //IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) PipeQ.instQueue is empty, and either ROB and rROB are empty",(long long int)globalClock,cpu_id));
  }

  //IS(if (cpu_id == 1) MSG("@%lld: Retiring CPU (%d)",(long long int)globalClock,cpu_id));
  retire();

  return true;
}/*}}}*/

StallCause InOrderProcessor::addInst(DInst *dinst)
{/*{{{*/

  const Instruction *inst = dinst->getInst();

#if 0
  // Simple in-order
  if(((RAT[inst->getSrc1()] != 0) && (inst->getSrc1() != LREG_NoDependence) && (inst->getSrc1() != LREG_InvalidOutput)) ||
     ((RAT[inst->getSrc2()] != 0) && (inst->getSrc2() != LREG_NoDependence) && (inst->getSrc2() != LREG_InvalidOutput))||
     ((RAT[inst->getDst1()] != 0) && (inst->getDst1() != LREG_InvalidOutput))||
     ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput))){
#else
    // scoreboard, no output dependence
  if(((RAT[inst->getDst1()] != 0) && (inst->getDst1() != LREG_InvalidOutput))||
     ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput))){
#endif
#if 0
    //Useful for debug
    if (cpu_id == 1 ){
      MSG("\n-------------------------");
      string str ="";
      str.append("\nCONFLICT->");
      if (RAT[inst->getSrc1()] != 0){
        str.append("src1, ");
        MSG(" SRC1 = %d, RAT[entry] = %d",inst->getSrc1(), RAT[inst->getSrc1()] );
        RAT[inst->getSrc1()]->dump("\nSRC1 in use by:");
      }

      if (RAT[inst->getSrc2()] != 0){
        str.append("src2, ");
        RAT[inst->getSrc2()]->dump("\nSRC2 in use by:");
      }

      if ((RAT[inst->getDst1()] != 0) && (inst->getDst2() != LREG_InvalidOutput)){
        str.append("dst1, ");
        RAT[inst->getDst1()]->dump("\nDST1 in use by:");
      }

      if ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput)){
        str.append("dst2, ");
        RAT[inst->getDst2()]->dump("\nDST2 in use by:");
      }

      dinst->dump(str.c_str());

    }
#endif
    return SmallWinStall;
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

  // FIXME: rafactor the rest of the function that it is the same as in OoOProcessor (share same function in GPRocessor)

  // BEGIN INSERTION (note that cluster already inserted in the window)
  // dinst->dump("");

  nInst[inst->getOpcode()]->inc(dinst->getStatsFlag()); // FIXME: move to cluster

  ROB.push(dinst);

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

  dinst->getCluster()->addInst(dinst);

  RAT[inst->getDst1()] = dinst;
  RAT[inst->getDst2()] = dinst;

  I(dinst->getCluster());
  dinst->markRenamed();

  return NoStall;
}/*}}}*/

void InOrderProcessor::retire()
{/*{{{*/

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
#if 0
    FlowID fid = dinst->getFlowId();
    if (fid == 1){
    fprintf(stderr,"\nCommitting from ROB , FlowID(%d), dinst->pc = %llx, dinst->addr = %llx, dinst->pe_id = %d, dinst->warp_id = %d",
          fid,
          dinst->getPC(),
          dinst->getAddr(),
          dinst->getPE(),
          dinst->getWarpID()
         );
    }
#endif

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
      //dinst->getInst()->dump("");
      //if ( dinst->getFlowId() == 1 ) dinst->dump("\nCannot Retire...");
      return;
    } else {
      //if ( dinst->getFlowId() == 1 ) dinst->dump("\nFinished...");
    }

#if 0
    FlowID fid = dinst->getFlowId();
    if( active) {
      EmulInterface *eint = TaskHandler::getEmul(fid);
      eint->reexecuteTail( fid );
    }
#endif

#if 0
    FlowID fid = dinst->getFlowId();
    fprintf(stderr,"\nRetiring from rROB , FlowID(%d), dinst->pc = %llx, dinst->addr = %llx, dinst->pe_id = %d, dinst->warp_id = %d",
          fid,
          dinst->getPC(),
          dinst->getAddr(),
          dinst->getPE(),
          dinst->getWarpID()
         );

#endif

#ifdef DEBUG
    if (!dinst->getInst()->isStore()) // Stores can perform after retirement
      I(dinst->isPerformed());
#endif

   if (dinst->isPerformed()) // Stores can perform after retirement
      dinst->destroy(eint);
    else{
      eint->reexecuteTail(dinst->getFlowId());
    }
    rROB.pop();
  }

}/*}}}*/

void InOrderProcessor::replay(DInst *dinst)
{/*{{{*/

  MSG("Inorder cores do not support replays. Set MemoryReplay = false in the confguration");

  // FIXME: foo should be equal to the number of in-flight instructions (check OoOProcessor)
  size_t foo= 1;
  nReplayInst.sample(foo, dinst->getStatsFlag());

  // FIXME: How do we manage a replay in this processor??
}/*}}}*/
