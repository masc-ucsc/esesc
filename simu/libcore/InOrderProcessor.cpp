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

#include "ClusterManager.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "InOrderProcessor.h"
#include "TaskHandler.h"
#include "estl.h"

class feqstr {
public:
  inline bool operator()(const char *s1, const char *s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};

typedef HASH_MAP<const char *, SMTFetch *, HASH<const char *>, feqstr> FetchMapType;
static FetchMapType                                                    fetchMap;

InOrderProcessor::InOrderProcessor(GMemorySystem *gm, CPU_t i)
    : GProcessor(gm, i)
    , RetireDelay(SescConf->getInt("cpusimu", "RetireDelay", i))
    , pipeQ(i)
    , lsq(i, 32768)
    , clusterManager(gm, i, this) { // {{{1
  char fName[1024];

  sprintf(fName, "Fetch(%d)", smt_ctx);
  FetchMapType::const_iterator it = fetchMap.find(fName);

  if(it != fetchMap.end() && smt > 1) {
    ifid = new FetchEngine(i, gm, it->second->fe);
    sf   = it->second;
  } else {
    ifid            = new FetchEngine(i, gm);
    sf              = new SMTFetch;
    sf->fe          = ifid;
    fetchMap[fName] = sf;
  }

  spaceInInstQueue = InstQueueSize;

  uint32_t smtnum = 1; // getMaxFlows();
  RAT             = new DInst *[LREG_MAX * smtnum * 128];
  bzero(RAT, sizeof(DInst *) * LREG_MAX * smtnum * 128);

  busy                 = false;
  lastrob_getStatsFlag = false;
}
// 1}}}

InOrderProcessor::~InOrderProcessor() { /*{{{*/
  delete RAT;
  // Nothing to do
} /*}}}*/

bool SMTFetch::update(bool space) {
  // {{{1
  if(smt_lastTime != globalClock) {
    smt_lastTime = globalClock;
    smt_active   = smt_cnt;
    smt_cnt      = 1;
  } else {
    smt_cnt++;
  }
  I(smt_active > 0);

  smt_turn--;
  if(smt_turn < 0 && space) {
    if(smt_cnt == smt_active)
      smt_turn = 0;
    else
      smt_turn = smt_active;
    return true;
  }

  return false;
} // 1}}}

void InOrderProcessor::fetch(FlowID fid) { /*{{{*/
  // TODO: Move this to GProcessor (same as in OoOProcessor)
  I(eint);
  I(active);

  if(smt > 1) {
    bool run = sf->update(spaceInInstQueue >= FetchWidth);
    if(!run)
      return;
  }

  if(ifid->isBlocked())
    return;

  IBucket *bucket = pipeQ.pipeLine.newItem();
  if(bucket) {
    ifid->fetch(bucket, eint, fid);
  }

} /*}}}*/

bool InOrderProcessor::advance_clock(FlowID fid) { /*{{{*/

  if(!active) {
    I(isROBEmpty());
    return false;
  }

  I(cpu_id == fid);

  fetch(fid);

  bool getStatsFlag = lastrob_getStatsFlag;
  if(!ROB.empty()) {
    getStatsFlag         = ROB.top()->getStatsFlag();
    lastrob_getStatsFlag = getStatsFlag;
  }

  // IS(if (cpu_id == 1) MSG("@%lld: Fetching CPU (%d)",(long long int)globalClock,cpu_id));
  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  // ID Stage (insert to instQueue)
  if(spaceInInstQueue >= FetchWidth) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if(bucket) {
      I(!bucket->empty());
      // IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) fetched bucket size is %d ",(long long int)globalClock,cpu_id,bucket->size()));
      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);
    } else {
      // IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) Empty Bucket ",(long long int)globalClock,cpu_id));
      noFetch2.inc(getStatsFlag);
    }
  } else {
    // IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) NO space in InstQueue",(long long int)globalClock,cpu_id));
    noFetch.inc(getStatsFlag);
  }

  // IS(if (cpu_id == 1) MSG("@%lld: Renaming CPU (%d)",(long long int)globalClock,cpu_id));
  // RENAME Stage
  if(!pipeQ.instQueue.empty()) {
    busy            = true;
    uint32_t n_insn = issue(pipeQ);
    spaceInInstQueue += n_insn;
    // IS(if (cpu_id == 1) MSG("@%lld: Issuing %d items in pipeline for CPU (%d)",(long long int)globalClock,n_insn, cpu_id));
  } else if(ROB.empty() && rROB.empty()) {
    // Still busy if we have some in-flight requests
    busy = pipeQ.pipeLine.hasOutstandingItems();
    // IS(if (cpu_id == 1) MSG("@%lld: ROB and rROB are both empty for CPU (%d)",(long long int)globalClock,cpu_id));
    // IS(if ((cpu_id == 1)&&(busy)) MSG("@%lld: pipeline also has outstanding items CPU (%d)",(long long int)globalClock,cpu_id));
    return true;
  } else {
    busy = true;
    // IS(if (cpu_id == 1) MSG("@%lld: CPU (%d) PipeQ.instQueue is empty, and either ROB and rROB are empty",(long long
    // int)globalClock,cpu_id));
  }

  // IS(if (cpu_id == 1) MSG("@%lld: Retiring CPU (%d)",(long long int)globalClock,cpu_id));
  retire();

  return true;
} /*}}}*/

void InOrderProcessor::executing(DInst *dinst) {
}

void InOrderProcessor::executed(DInst *dinst) {
}

StallCause InOrderProcessor::addInst(DInst *dinst) { /*{{{*/

  const Instruction *inst = dinst->getInst();
  FlowID rat_off = 0; // no need, addInst is private per thread. Cluster is shared (dinst->getFlowId() % getMaxFlows())*LREG_MAX;

#if 1
#if 0
  // Simple in-order
  if(((RAT[inst->getSrc1()] != 0) && (inst->getSrc1() != LREG_NoDependence) && (inst->getSrc1() != LREG_InvalidOutput)) ||
     ((RAT[inst->getSrc2()] != 0) && (inst->getSrc2() != LREG_NoDependence) && (inst->getSrc2() != LREG_InvalidOutput))||
     ((RAT[inst->getDst1()] != 0) && (inst->getDst1() != LREG_InvalidOutput))||
     ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput))) {
#else
#if 1
  // Simple in-order for RAW, but not WAW or WAR
  if(((RAT[inst->getSrc1() + rat_off] != 0) && (inst->getSrc1() != LREG_NoDependence)) ||
     ((RAT[inst->getSrc2() + rat_off] != 0) && (inst->getSrc2() != LREG_NoDependence))) {
#else
                      // scoreboard, no output dependence
  if(((RAT[inst->getDst1()] != 0) && (inst->getDst1() != LREG_InvalidOutput)) ||
     ((RAT[inst->getDst2()] != 0) && (inst->getDst2() != LREG_InvalidOutput))) {
#endif
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
#endif

if((ROB.size() + rROB.size()) >= (MaxROBSize-1))
  return SmallROBStall;

Cluster *cluster = dinst->getCluster();
if(!cluster) {
  Resource *res = clusterManager.getResource(dinst);
  cluster       = res->getCluster();
  dinst->setCluster(cluster, res);
}

I(dinst->getFlowId() == cpu_id);

StallCause sc = cluster->canIssue(dinst);
if(sc != NoStall)
  return sc;

// FIXME: rafactor the rest of the function that it is the same as in OoOProcessor (share same function in GPRocessor)

// BEGIN INSERTION (note that cluster already inserted in the window)
// dinst->dump("");

nInst[inst->getOpcode()]->inc(dinst->getStatsFlag()); // FIXME: move to cluster

ROB.push(dinst);

if(!dinst->isSrc2Ready()) {
  // It already has a src2 dep. It means that it is solved at
  // retirement (Memory consistency. coherence issues)
  if(RAT[inst->getSrc1() + rat_off])
    RAT[inst->getSrc1() + rat_off]->addSrc1(dinst);
} else {
  if(RAT[inst->getSrc1() + rat_off])
    RAT[inst->getSrc1() + rat_off]->addSrc1(dinst);

  if(RAT[inst->getSrc2() + rat_off])
    RAT[inst->getSrc2() + rat_off]->addSrc2(dinst);
}

I(!dinst->isExecuted());

dinst->setRAT1Entry(&RAT[inst->getDst1() + rat_off]);
dinst->setRAT2Entry(&RAT[inst->getDst2() + rat_off]);

dinst->getCluster()->addInst(dinst);

RAT[inst->getDst1() + rat_off] = dinst;
RAT[inst->getDst2() + rat_off] = dinst;

I(dinst->getCluster());
dinst->markRenamed();

return NoStall;
} /*}}}*/

void InOrderProcessor::retire() { /*{{{*/

  // Pass all the ready instructions to the rrob
  bool stats = false;
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();
    stats        = dinst->getStatsFlag();

    I(cpu_id == dinst->getFlowId());

    bool done = dinst->getClusterResource()->preretire(dinst, false);
    if(!done)
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

  for(uint16_t i = 0; i < RetireWidth && !rROB.empty(); i++) {
    DInst *dinst = rROB.top();

    if(!dinst->isExecuted())
      break;

    if((dinst->getExecutedTime() + RetireDelay) >= globalClock)
      break;

    I(dinst->getCluster());

    bool done = dinst->getCluster()->retire(dinst, false);
    if(!done)
      return;

#ifdef DEBUG
    if(!dinst->getInst()->isStore()) // Stores can perform after retirement
      I(dinst->isPerformed());
#endif

    if(dinst->isPerformed()) // Stores can perform after retirement
      dinst->destroy(eint);
    else {
      eint->reexecuteTail(dinst->getFlowId());
    }
    rROB.pop();
  }

} /*}}}*/

void InOrderProcessor::replay(DInst *dinst) { /*{{{*/

  MSG("Inorder cores do not support replays. Set MemoryReplay = false in the confguration");

  // FIXME: foo should be equal to the number of in-flight instructions (check OoOProcessor)
  size_t foo = 1;
  nReplayInst.sample(foo, dinst->getStatsFlag());

  // FIXME: How do we manage a replay in this processor??
} /*}}}*/
