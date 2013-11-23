// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Milos Prvulovic
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

#ifdef ENABLE_CUDA

#include "SescConf.h"

#include "TaskHandler.h"
#include "GPUSMProcessor.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "ClusterManager.h"

GPUSMProcessor::GPUSMProcessor(GMemorySystem *gm, CPU_t i)
  :GProcessor(gm, i, 1)
  ,IFID(i, this, gm)
  ,pipeQ(i)
  ,lsq(i)
  ,rROB(SescConf->getInt("cpusimu", "robSize", i))
  ,clusterManager(gm, this)
{
  numSP = SescConf->getInt("cpusimu", "sp_per_sm", i); 
  uint32_t maxwarps = SescConf->getInt("cpuemul", "max_warps_sm", i); 
  IS(MSG("Number of SPs = %d, maxwarps = %d", numSP,maxwarps));

  spaceInInstQueue = InstQueueSize;

  RAT = new DInst* [LREG_MAX * numSP * maxwarps];
  bzero(RAT,sizeof(DInst*)*LREG_MAX*numSP*maxwarps);
  
  busy = false;
}

GPUSMProcessor::~GPUSMProcessor() {
  delete RAT;
  // Nothing to do
}

void GPUSMProcessor::fetch(FlowID fid) {
  I(eint);

  if(!active){
    TaskHandler::removeFromRunning(cpu_id);
    return;
  }

  // Do not block fetch for a branch miss
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

bool GPUSMProcessor::execute() {

  if (!active) {
    // time to remove from the running queue
    TaskHandler::removeFromRunning(cpu_id);
    return false;
  }

  if (!busy) {
    return false;
  }

  clockTicks.inc();
  setWallClock();

  if (throtting) { 
    throtting_cntr++;

    int skip = ceil(throtting/getTurboRatioGPU()); 

    if (throtting_cntr < skip) {
      return true;
    }
    throtting_cntr = 1;
  }

  // ID Stage (insert to instQueue)
  if (spaceInInstQueue >= FetchWidth) {
    //MSG("\nFor CPU %d:",getId());
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);
    }else{
      noFetch2.inc();
    }
  }else{
    noFetch.inc();
  }

  // RENAME Stage
  if ( !pipeQ.instQueue.empty() ) {
    spaceInInstQueue += issue(pipeQ);
  }else if (ROB.empty() && rROB.empty()) {
    //I(0);
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    return true;
  }

  retire();

  return true;
}

StallCause GPUSMProcessor::addInst(DInst *dinst) {

  const Instruction *inst = dinst->getInst();

#if 0
  if(RAT[inst->getSrc(0)+dinst->getPE()*LREG_MAX] != 0 ||
     RAT[inst->getSrc(1)+dinst->getPE()*LREG_MAX] != 0 ||
     RAT[inst->getDst(0)+dinst->getPE()*LREG_MAX] != 0 ||
     RAT[inst->getDst(1)+dinst->getPE()*LREG_MAX] != 0) {
    return SmallWinStall;
  }
#endif
  if(((RAT[inst->getSrc1()] != 0) && (inst->getSrc1() != LREG_NoDependence) && (inst->getSrc1() != LREG_InvalidOutput)) ||
     ((RAT[inst->getSrc2()] != 0) && (inst->getSrc2() != LREG_NoDependence) && (inst->getSrc2() != LREG_InvalidOutput))||
     ((RAT[inst->getDst1()] != 0) && (inst->getDst1() != LREG_InvalidOutput))||
     ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput))){
#if 0
    //Useful for debug
    if (cpu_id > 0 ){
      string str ="";
      str.append("CONFLICT->");
      MSG(" SRC1 = %d, RAT[entry] = %d",inst->getSrc1(), RAT[inst->getSrc1()] );
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


  // BEGIN INSERTION (note that cluster already inserted in the window)
  //dinst->dump("");

  nInst[inst->getOpcode()]->inc(); // FIXME: move to cluster

  ROB.push(dinst);
  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field


  dinst->setRAT1Entry(&RAT[inst->getDst1()]);
  dinst->setRAT2Entry(&RAT[inst->getDst2()]);
  RAT[inst->getDst1()] = dinst;
  RAT[inst->getDst2()] = dinst;

  I(dinst->getCluster());
  dinst->getCluster()->addInst(dinst);

  return NoStall;
}

void GPUSMProcessor::retire() {

  // Pass all the ready instructions to the rrob
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();

    if( !dinst->isExecuted() )
      break;

    bool done = dinst->getClusterResource()->preretire(dinst, false);
    if( !done )
      break;

    rROB.push(dinst);
    ROB.pop();

  }

  robUsed.sample(ROB.size());
  rrobUsed.sample(rROB.size());

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

    nCommitted.inc();

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

void GPUSMProcessor::replay(DInst *dinst) {

  MSG("GPU_SM cores(which are essentially inorder) do not support replays. Set NoMemoryReplay = true at the confguration");

  // FIXME: foo should be equal to the number of in-flight instructions (check OoOProcessor)
  size_t foo= 1;
  nReplayInst.sample(foo);

  // FIXME: How do we manage a replay in this processor??
}
#endif
