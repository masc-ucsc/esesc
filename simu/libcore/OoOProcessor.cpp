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
#include <fstream>
#include <math.h>
#include <algorithm>
#include <iterator>
#include <numeric>

#include "SescConf.h"

#include "OoOProcessor.h"
#include "EmuSampler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "TaskHandler.h"
#include "FastQueue.h"
#include "MemRequest.h"

//#define ESESC_TRACE

/* }}} */

//#define ESESC_CODEPROFILE
#define ESESC_BRANCHPROFILE

// FIXME: to avoid deadlock, prealloc n to the n oldest instructions
//#define LATE_ALLOC_REGISTER
extern "C" uint64_t esesc_mem_read(uint64_t addr);

OoOProcessor::OoOProcessor(GMemorySystem *gm, CPU_t i)
    /* constructor {{{1 */
    : GOoOProcessor(gm, i)
    , MemoryReplay(SescConf->getBool("cpusimu", "MemoryReplay", i))
#ifdef ENABLE_LDBP
    , LGT_SIZE(SescConf->getInt("cpusimu", "lgt_size", i))
#endif
    , RetireDelay(SescConf->getInt("cpusimu", "RetireDelay", i))
    , IFID(i, gm)
    , pipeQ(i)
    , lsq(i,
          SescConf->checkInt("cpusimu", "maxLSQ", i) ? SescConf->getInt("cpusimu", "maxLSQ", i) : 32768) // 32K (unlimited or fix)
    , retire_lock_checkCB(this)
    , clusterManager(gm, i, this)
    , avgFetchWidth("P(%d)_avgFetchWidth", i)
#ifdef TRACK_TIMELEAK
    , avgPNRHitLoadSpec("P(%d)_avgPNRHitLoadSpec", i)
    , avgPNRMissLoadSpec("P(%d)_avgPNRMissLoadSpec", i)
#endif
#ifdef TRACK_FORWARDING
    , avgNumSrc("P(%d)_avgNumSrc", i)
    , avgNumDep("P(%d)_avgNumDep", i)
    , fwd0done0("P(%d)_fwd0done0", i)
    , fwd1done0("P(%d)_fwd1done0", i)
    , fwd1done1("P(%d)_fwd1done1", i)
    , fwd2done0("P(%d)_fwd2done0", i)
    , fwd2done1("P(%d)_fwd2done1", i)
    , fwd2done2("P(%d)_fwd2done2", i)
#endif
    , codeProfile("P(%d)_prof", i) {
  bzero(RAT, sizeof(DInst *) * LREG_MAX);
  bzero(serializeRAT, sizeof(DInst *) * LREG_MAX);
#ifdef TRACK_FORWARDING
  bzero(fwdDone, sizeof(Time_t) * LREG_MAX);
#endif

  spaceInInstQueue = InstQueueSize;

  codeProfile_trigger = 0;

  nTotalRegs = SescConf->getInt("cpusimu", "nTotalRegs", gm->getCoreId());
  if(nTotalRegs == 0)
    nTotalRegs = 1024 * 1024 * 1024; // Unlimited :)

  busy             = false;
  flushing         = false;
  replayRecovering = false;
  getStatsFlag     = false;
  replayID         = 0;

  last_state.dinst_ID = 0xdeadbeef;

  if(SescConf->checkInt("cpusimu", "serialize", i))
    serialize = SescConf->getInt("cpusimu", "serialize", i);
  else
    serialize = 0;

  serialize_level       = 2; // 0 full, 1 all ld, 2 same reg
  serialize_for         = 0;
  last_serialized       = 0;
  last_serializedST     = 0;
  forwardProg_threshold = 200;

#ifdef ENABLE_LDBP
  DL1 = gm->getDL1();
  ldbp_curr_addr      = 0;
  ldbp_delta          = 0;
  inflight_branch     = 0;
  ldbp_brpc           = 0;
  ldbp_ldpc           = 0;
#if 0
  num_mem_lat         = 0;
  last_mem_lat        = 0;
  diff_mem_lat        = 0;
#endif
#endif
  if(SescConf->checkBool("cpusimu", "scooreMemory", gm->getCoreId()))
    scooreMemory = SescConf->getBool("cpusimu", "scooreMemory", gm->getCoreId());
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

  if(IFID.isBlocked()) {
    //    I(0);
    busy = true;
  } else {
    IBucket *bucket = pipeQ.pipeLine.newItem();
    if(bucket) {
#ifdef ENABLE_LDBP
#if 0
      //dinst->setBasePrefAddr(ldbp_curr_addr);
      dinst->setRetireBrCount(brpc_count);
      dinst->setBrPC(ldbp_brpc);
      dinst->setInflight(inflight_branch);
      dinst->setDelta(ldbp_delta);
#endif
#endif
      IFID.fetch(bucket, eint, fid);
      if(!bucket->empty()) {
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

  if(!active) {
    // time to remove from the running queue
    // TaskHandler::removeFromRunning(cpu_id);
    return false;
  }

  fetch(fid);

  if(!ROB.empty()) {
    // Else use last time
    getStatsFlag = ROB.top()->getStatsFlag();
  }

  clockTicks.inc(getStatsFlag);
  setWallClock(getStatsFlag);

  if(!busy)
    return false;

  if(unlikely(throttlingRatio > 1)) {
    throttling_cntr++;

    uint32_t skip = (uint32_t)ceil(throttlingRatio / getTurboRatio());

    if(throttling_cntr < skip) {
      return true;
    }
    throttling_cntr = 1;
  }

  // ID Stage (insert to instQueue)
  if(spaceInInstQueue >= FetchWidth) {
    IBucket *bucket = pipeQ.pipeLine.nextItem();
    if(bucket) {
      I(!bucket->empty());
      //      I(bucket->top()->getInst()->getAddr());

      spaceInInstQueue -= bucket->size();
      pipeQ.instQueue.push(bucket);

      // GMSG(getID()==1,"instqueue insert %p", bucket);
    } else {
      noFetch2.inc(getStatsFlag);
    }
  } else {
    noFetch.inc(getStatsFlag);
  }

  // RENAME Stage
  if(replayRecovering) {
    if((rROB.empty() && ROB.empty())) {
      // Recovering done
      EmulInterface *eint = TaskHandler::getEmul(flushing_fid);
      eint->syncHeadTail(flushing_fid);

      I(flushing);
      replayRecovering = false;
      flushing         = false;

      if((lastReplay + 2 * forwardProg_threshold) < replayID) {
        serialize_level = 3; // One over max to start with 2
        // MSG("%d Reset Serialize level @%lld : %lld %lld",cpu_id, globalClock,lastReplay, replayID);
      }
      if((lastReplay + forwardProg_threshold) > replayID) {
        if(serialize_level) {
          // MSG("%d One level less %d for %d @%lld : %lld %lld", cpu_id, serialize_level, serialize_for, globalClock, lastReplay,
          // replayID);
          serialize_level--;
        } else {
          // MSG("%d already at level 0 @%lld", cpu_id, globalClock);
        }
        serialize_for = serialize;
        // forwardProg_threshold = replayID - lastReplay;
        // serialize_for = forwardProg_threshold;
      }

      lastReplay = replayID;
    } else {
      nStall[ReplaysStall]->add(RealisticWidth, getStatsFlag);
      retire();
      return true;
    }
  }

  if(!pipeQ.instQueue.empty()) {
    spaceInInstQueue += issue(pipeQ);
  } else if(ROB.empty() && rROB.empty()) {
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
  dinst->markExecuting();

#ifdef LATE_ALLOC_REGISTER
  if(dinst->getInst()->hasDstRegister())
    nTotalRegs--;
#endif
#ifdef TRACK_FORWARDING
  if(dinst->getStatsFlag()) {
    const Instruction *inst = dinst->getInst();
    avgNumSrc.sample(inst->getnsrc(), true);

    int nForward = 0;
    int nNeeded  = 0;
    if(inst->hasSrc1Register()) {
      nNeeded++;
      Time_t t = fwdDone[inst->getSrc1()];
      if((t + 2) >= globalClock)
        nForward++;
    }
    if(inst->hasSrc2Register()) {
      nNeeded++;
      Time_t t = fwdDone[inst->getSrc2()];
      if((t + 2) >= globalClock)
        nForward++;
    }

    if(nNeeded == 0)
      fwd0done0.inc(true);
    else if(nNeeded == 1) {
      if(nForward)
        fwd1done1.inc(true);
      else
        fwd1done0.inc(true);
    } else {
      if(nForward == 2)
        fwd2done2.inc(true);
      else if(nForward == 1)
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

  if((ROB.size() + rROB.size()) >= (MaxROBSize-1))
    return SmallROBStall;

  const Instruction *inst = dinst->getInst();

  if(nTotalRegs <= 0)
    return SmallREGStall;

  Cluster *cluster = dinst->getCluster();
  if(!cluster) {
    Resource *res = clusterManager.getResource(dinst);
    cluster       = res->getCluster();
    dinst->setCluster(cluster, res);
  }

  StallCause sc = cluster->canIssue(dinst);
  if(sc != NoStall) {
    return sc;
  }

  // if no stalls were detected do the following:
  //
  // BEGIN INSERTION (note that cluster already inserted in the window)
  // dinst->dump("");

#ifndef LATE_ALLOC_REGISTER
  if(inst->hasDstRegister()) {
    nTotalRegs--;
  }
#endif

  //#if 1
  if(!scooreMemory) { // no dynamic serialization for tradcore
    if(serialize_for > 0 && !replayRecovering) {
      serialize_for--;
      if(inst->isMemory() && dinst->isSrc3Ready()) {
        if(last_serialized && !last_serialized->isExecuted()) {
          // last_serialized->addSrc3(dinst); FIXME
          // MSG("addDep3 %8ld->%8lld %lld",last_serialized->getID(), dinst->getID(), globalClock);
        }
        last_serialized = dinst;
      }
    }
    //#else
  } else {
    if(serialize_for > 0 && !replayRecovering) {
      serialize_for--;

      if(serialize_level == 0) {
        // Serialize all the memory operations
        if(inst->isMemory() && dinst->isSrc3Ready()) {
          if(last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serialized = dinst;
        }
      } else if(serialize_level == 1) {
        // Serialize stores, and loads depend on stores (no loads on loads)
        if(inst->isLoad() && dinst->isSrc3Ready()) {
          if(last_serializedST && !last_serializedST->isIssued()) {
            last_serializedST->addSrc3(dinst);
          }
          last_serialized = dinst;
        }
        if(inst->isStore() && dinst->isSrc3Ready()) {
          if(last_serialized && !last_serialized->isIssued()) {
            last_serialized->addSrc3(dinst);
          }
          last_serializedST = dinst;
        }
      } else {
        // Serialize if same register is being accessed
        if(inst->getSrc1() < LREG_ARCH0) {
          last_serializeLogical = inst->getSrc1();
        } else if(last_serializePC != dinst->getPC()) {
          last_serializeLogical = LREG_InvalidOutput;
        }
        last_serializePC = dinst->getPC();

        if(last_serializeLogical < LREG_ARCH0) {
          if(inst->isMemory()) {
            if(serializeRAT[last_serializeLogical]) {
              if(inst->isLoad()) {
                if(serializeRAT[last_serializeLogical]->getInst()->isStore())
                  serializeRAT[last_serializeLogical]->addSrc3(dinst);
              } else {
                serializeRAT[last_serializeLogical]->addSrc3(dinst);
              }
            }

            dinst->setSerializeEntry(&serializeRAT[last_serializeLogical]);
            serializeRAT[last_serializeLogical] = dinst;
          } else {
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
  if(!dinst->isSrc2Ready()) {
    // It already has a src2 dep. It means that it is solved at
    // retirement (Memory consistency. coherence issues)
    if(RAT[inst->getSrc1()]) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      // MSG("addDep0 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }
  } else {
    if(RAT[inst->getSrc1()]) {
      RAT[inst->getSrc1()]->addSrc1(dinst);
      n++;
      // MSG("addDep1 %8ld->%8lld %lld",RAT[inst->getSrc1()]->getID(), dinst->getID(), globalClock);
    }

    if(RAT[inst->getSrc2()]) {
      RAT[inst->getSrc2()]->addSrc2(dinst);
      n++;
      // MSG("addDep2 %8ld->%8lld %lld",RAT[inst->getSrc2()]->getID(), dinst->getID(), globalClock);
    }
  }
#ifdef TRACK_FORWARDING
  avgNumSrc.sample(inst->getnsrc(), dinst->getStatsFlag());
  avgNumDep.sample(n, dinst->getStatsFlag());
#endif

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

#ifdef WAVESNAP_EN
//add instruction to wavesnap
if(!SINGLE_WINDOW) {
  if(WITH_SAMPLING) {
    if(dinst->getStatsFlag()) {
      snap->add_instruction(dinst);
    }
  } else {
    snap->add_instruction(dinst);
  }
}
#endif

  return NoStall;
}
/* }}} */

void OoOProcessor::retire_lock_check()
/* Detect simulator locks and flush the pipeline {{{1 */
{
  RetireState state;
  if(active) {
    state.committed = nCommitted.getDouble();
  } else {
    state.committed = 0;
  }
  if(!rROB.empty()) {
    state.r_dinst    = rROB.top();
    state.r_dinst_ID = rROB.top()->getID();
  }

  if(!ROB.empty()) {
    state.dinst    = ROB.top();
    state.dinst_ID = ROB.top()->getID();
  }

  if(last_state == state && active) {
    I(0);
    MSG("Lock detected in P(%d), flushing pipeline", getID());
  }

  last_state = state;

  retire_lock_checkCB.scheduleAbs(globalClock + 100000);
}
/* }}} */

#ifdef ENABLE_LDBP
#if 0
void OoOProcessor::hit_on_load_table(DInst *dinst, bool is_li) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = LOAD_TABLE_SIZE - 1; i >= 0; i--) {
    if(dinst->getPC() == load_table_vec[i].ldpc) {
      if(is_li) {
        load_table_vec[i].lt_load_imm(dinst);
      }else {
        load_table_vec[i].lt_load_hit(dinst);
      }
      load_table l = load_table_vec[i];
      load_table_vec.erase(load_table_vec.begin() + i);
      load_table_vec.push_back(load_table());
      load_table_vec[LOAD_TABLE_SIZE - 1] = l;
      if(load_table_vec[LOAD_TABLE_SIZE - 1].tracking > 0) {
        //append ld_ptr to PLQ
        int plq_idx = return_plq_index(load_table_vec[LOAD_TABLE_SIZE - 1].ldpc);
        if(plq_idx != - 1) {
          pending_load_queue p = plq_vec[plq_idx];
          plq_vec.erase(plq_vec.begin() + plq_idx);
        }else {
          plq_vec.erase(plq_vec.begin());
        }
        plq_idx = PLQ_SIZE - 1;
        plq_vec.push_back(pending_load_queue());
        plq_vec[plq_idx].load_pointer = load_table_vec[LOAD_TABLE_SIZE - 1].ldpc;
        if(load_table_vec[LOAD_TABLE_SIZE - 1].delta == load_table_vec[LOAD_TABLE_SIZE - 1].prev_delta) {
          plq_vec[plq_idx].plq_update_tracking(true);
        }else {
          plq_vec[plq_idx].plq_update_tracking(false);
        }
      }
      return;
    }
  }
  //if load_table miss
  load_table_vec.erase(load_table_vec.begin());
  load_table_vec.push_back(load_table());
  if(is_li) {
    load_table_vec[LOAD_TABLE_SIZE - 1].lt_load_imm(dinst);
  }else {
    load_table_vec[LOAD_TABLE_SIZE - 1].lt_load_miss(dinst);
  }
}

int OoOProcessor::return_load_table_index(AddrType pc) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = LOAD_TABLE_SIZE - 1; i >= 0; i--) {
    if(pc == load_table_vec[i].ldpc) {
      return i;
    }
  }
  return -1;
}
#endif

void OoOProcessor::rtt_load_hit(DInst *dinst) {
  //reset entry on hit and update fields
  RegType dst          = dinst->getInst()->getDst1();
  int lt_idx = DL1->return_load_table_index(dinst->getPC());
  if(DL1->load_table_vec[lt_idx].conf > 63) {
    rtt_vec[dst] = rename_tracking_table();
    rtt_vec[dst].num_ops = 0;
    if(!DL1->load_table_vec[lt_idx].is_li) {
      rtt_vec[dst].load_table_pointer.push_back(dinst->getPC());
    }else {
      // FIXME - use PC for Li or some random number????????
      //rtt_vec[dst].load_table_pointer.push_back(1);
      rtt_vec[dst].load_table_pointer.push_back(dinst->getPC());
    }
  }else {
    rtt_vec[dst].num_ops = NUM_OPS + 1;
  }
}

void OoOProcessor::rtt_alu_hit(DInst *dinst) {
  //update fields using information from RTT and LT
  //num_ops = src1(nops) + src2(nops) + 1
  //ld_ptr = ptr of src1 and src2
  RegType dst  = dinst->getInst()->getDst1();
  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();

  if(src1 == LREG_R0 && src2 == LREG_R0) { //if ALU is Li
    rtt_vec[dst].num_ops = 0 + 0 + 1;
    rtt_vec[dst].load_table_pointer.clear();
    return;
  }
  int nops1 = rtt_vec[src1].num_ops;
  int nops2 = rtt_vec[src2].num_ops;
  int nops = nops1 + nops2 + 1;
  //concatenate ptr list of 2 srcs into dst
  std::vector<AddrType> tmp;
  tmp.clear();
  tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.end());
  tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.end());
  std::sort(tmp.begin(), tmp.end());
  tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
  rtt_vec[dst].num_ops = nops;
  rtt_vec[dst].load_table_pointer = tmp;
}

void OoOProcessor::rtt_br_hit(DInst *dinst) {
  //read num ops and stride ptr fields on hit
  //use stride ptr to figure out if the LD(s) are predictable
  int btt_id = return_btt_index(dinst->getPC());
  if(btt_id == -1) { //do the below checks only if BTT_MISS
    if(dinst->isBiasBranch()) {
      //Index RTT with BR only when tage is not high-conf
      return;
    }else if(!dinst->isBranchMiss()) {
      return;
    }
  }
  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();
  int all_ld_conf1 = 0; //invalid
  int all_ld_conf2 = 0; //invalid
  int nops1 = rtt_vec[src1].num_ops;
  int nops2 = rtt_vec[src2].num_ops;
#if 1
  for(int i = 0; i < rtt_vec[src1].load_table_pointer.size(); i++) {
    int lt_index = DL1->return_load_table_index(rtt_vec[src1].load_table_pointer[i]);
    //Above line can cause SEG_FAULT if index of Li is checked
    if(DL1->load_table_vec[lt_index].conf > 63) {
      all_ld_conf1 = 1;
    }else {
      all_ld_conf1 = 0;
      break;
    }
  }
  for(int i = 0; i < rtt_vec[src2].load_table_pointer.size(); i++) {
    int lt_index = DL1->return_load_table_index(rtt_vec[src2].load_table_pointer[i]);
    //Above line can cause SEG_FAULT if index of Li is checked
    if(DL1->load_table_vec[lt_index].conf > 63) {
      all_ld_conf2 = 1;
    }else {
      all_ld_conf2 = 0;
      break;
    }
  }
#endif
  if(src2 == LREG_R0) {
    all_ld_conf2 = 1;
  }
  int nops     = nops1 + nops2;
  int nlds     = rtt_vec[src1].load_table_pointer.size() + rtt_vec[src2].load_table_pointer.size();
  bool all_conf = all_ld_conf1 && all_ld_conf2;
#if 0
  if(dinst->isBranchMiss_level2() && (nops < NUM_LOADS && nlds < NUM_LOADS))
    MSG("RTT brpc=%llx nops=%d nloads=%d all_conf=%d", dinst->getPC(), nops, nlds, all_conf);
#endif

  //if all_conf && nops < num_ops && nlds < num_loads
  //fill BTT with brpc and str ptrs; acc = 4; sp.tracking = true for all str ptrs
  if(all_conf && (nops < NUM_OPS) && (nlds < NUM_LOADS)) {
    //if BTT miss and TAGE low conf
    if(btt_id == -1) {
      btt_br_miss(dinst);
    }else {
      btt_br_hit(dinst, btt_id);
    }
  }
}

int OoOProcessor::return_btt_index(AddrType pc) {
  //if hit on BTT, update and move entry to LRU position
  for(int i = BTT_SIZE - 1; i >= 0; i--) {
    if(pc == btt_vec[i].brpc) {
      return i;
    }
  }
  return -1;
}

void OoOProcessor::btt_br_miss(DInst *dinst) {
  RegType src1 = dinst->getInst()->getSrc1();
  RegType src2 = dinst->getInst()->getSrc2();
  btt_vec.erase(btt_vec.begin());
  btt_vec.push_back(branch_trigger_table());
  //merge LD ptr list from src1 and src2 entries in RTT
  std::vector<AddrType> tmp;
  std::vector<AddrType> tmp_addr;
  tmp.clear();
  tmp.insert(tmp.begin(), rtt_vec[src1].load_table_pointer.begin(), rtt_vec[src1].load_table_pointer.end());
  tmp.insert(tmp.end(), rtt_vec[src2].load_table_pointer.begin(), rtt_vec[src2].load_table_pointer.end());
  std::sort(tmp.begin(), tmp.end());
  tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());
  btt_vec[BTT_SIZE - 1].load_table_pointer = tmp;
  btt_vec[BTT_SIZE - 1].brpc = dinst->getPC();
  btt_vec[BTT_SIZE - 1].accuracy = BTT_MAX_ACCURACY / 2; //bpred accuracy set to intermediate value
  for(int i = 0; i < tmp.size(); i++) { //update sp.tracking for all ld_ptr in BTT
    int id = DL1->return_load_table_index(tmp[i]);
    if(id != -1) {
      DL1->load_table_vec[id].lt_update_tracking(true);
      //allocate entries at LOR and LOT
      // set LOR_START to next LD-BR chain's ld_addr
      // When current BR retires, there maybe other BRs inflight
      // set ld_start considering these inflight BRs
      //AddrType lor_start_addr = load_table_vec[id].ld_addr + (load_table_vec[id].delta * TRIG_LD_JUMP);
      //int look_ahead = dinst->getInflight() + 1;
      int look_ahead = 0;
      //lor.start -> lor.start + lor.delta
      //(as this allocate is done on Br miss, there will be no inflight Br. So lor.start should be updated to next LD-BR's addr)
      AddrType lor_start_addr = DL1->load_table_vec[id].ld_addr + DL1->load_table_vec[id].delta;
      //tmp_addr.push_back(DL1->load_table_vec[id].ld_addr);
      tmp_addr.push_back(lor_start_addr);
      bool is_li     = false;
      if(DL1->load_table_vec[id].ld_addr == 0) {
        lor_start_addr = 0;
        is_li          = true;
      }
      DL1->lor_allocate(DL1->load_table_vec[id].ldpc, lor_start_addr, DL1->load_table_vec[id].delta, look_ahead, is_li);
      //FIXME -> is 3rd param in next line correct???????
      DL1->bot_allocate(dinst->getPC(), DL1->load_table_vec[id].ldpc, DL1->load_table_vec[id].ld_addr);
#if 0
      if(dinst->getPC() == 0x1044e)
        MSG("BOT_ALLOC clk=%d brid=%d brpc=%llx ldpc=%llx ld_addr=%d lor_start=%d", globalClock, dinst->getID(), dinst->getPC(), tmp[i], DL1->load_table_vec[id].ld_addr, lor_start_addr);
#endif
    }
  }
  int bot_id                    = DL1->return_bot_index(dinst->getPC());
  DL1->bot_vec[bot_id].load_ptr = tmp;
  DL1->bot_vec[bot_id].curr_br_addr = tmp_addr;
}

void OoOProcessor::btt_br_hit(DInst *dinst, int btt_id) {
  btt_vec[btt_id].btt_update_accuracy(dinst, btt_id); //update BTT accuracy
  if(btt_vec[btt_id].accuracy == 0) { //clear sp.tracking if acc == 0 and reset BTT entry
    for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
      int lt_idx = DL1->return_load_table_index(btt_vec[btt_id].load_table_pointer[i]);
      if(lt_idx != -1) {
        DL1->load_table_vec[lt_idx].tracking = 0;
      }
    }
    //clear entry
    btt_vec.erase(btt_vec.begin() + btt_id);
    btt_vec.push_back(branch_trigger_table());
    return;
  }
  //check if btt_strptr == plq_strptr and tracking bits are set
  //if yes and associated LOR are tracking the str_ptr, then trigger loads
  int tl_flag = btt_pointer_check(dinst, btt_id);
  if(tl_flag == 1) { //trigger loads
    //trigger loads
    //MSG("LOAD_PTR pc=%llx size=%d", dinst->getPC(), btt_vec[btt_id].load_table_pointer.size());
    for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
      btt_trigger_load(dinst, btt_vec[btt_id].load_table_pointer[i]);
    }
  }
}

void OoOProcessor::btt_trigger_load(DInst *dinst, AddrType ld_ptr) {
  int lor_id = DL1->return_lor_index(ld_ptr);
  int lt_idx = DL1->return_load_table_index(ld_ptr);
  if(lor_id != -1) {
    AddrType lor_start_addr = DL1->lor_vec[lor_id].ld_start;
    AddrType lt_load_addr = DL1->load_table_vec[lt_idx].ld_addr;
    //MSG("TL brpc=%llx ldpc=%llx ld_addr=%d conf=%d", dinst->getPC(), ld_ptr, lor_start_addr, DL1->load_table_vec[lt_idx].conf);
    int64_t lor_delta = DL1->lor_vec[lor_id].ld_delta;
    for(int i = 1; i <= 1; i++) {
      AddrType trigger_addr = lor_start_addr + (lor_delta * 31); //trigger few delta ahead of current ld_addr
#if 0
      MSG("TL clk=%d br_id=%d brpc=%llx ldpc=%llx ld_addr=%d lor_start=%d trig_addr=%d conf=%d", globalClock, dinst->getID(), dinst->getPC(), ld_ptr, lt_load_addr, lor_start_addr, trigger_addr, DL1->load_table_vec[lt_idx].conf);
#endif
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ld_ptr, dinst->getPC(), lor_start_addr, 0, lor_delta, inflight_branch, 0, 0, 0, 0);
    }
    //update lor_start by delta so that next TL doesnt trigger redundant loads
    DL1->lor_vec[lor_id].ld_start = DL1->lor_vec[lor_id].ld_start + DL1->lor_vec[lor_id].ld_delta;
    DL1->lor_vec[lor_id].data_pos++;
  }
}

int OoOProcessor::btt_pointer_check(DInst *dinst, int btt_id) {
  //returns 1 if all ptrs are found; else 0
  bool all_ptr_found = true;
  bool all_ptr_track = true;
  std::vector<AddrType> tmp_plq;

  for(int i = 0; i < DL1->plq_vec.size(); i++) {
    tmp_plq.push_back(DL1->plq_vec[i].load_pointer);
  }

  for(int i = 0; i < btt_vec[btt_id].load_table_pointer.size(); i++) {
    auto it = std::find(tmp_plq.begin(), tmp_plq.end(), btt_vec[btt_id].load_table_pointer[i]);
    if(it != tmp_plq.end()) {
      //element in BTT found in PLQ
      int plq_id = DL1->return_plq_index(*it);
      if(DL1->plq_vec[plq_id].tracking == 0) {
        //Delta changed for ld_ptr "i"; don't trigger load
        all_ptr_track = false;
      }
    }else {
      //element in BRR not found in PLQ
      all_ptr_found = false;
      //start tracking this ld_ptr "i" as its part of the current LD-BR slice
      int lt_idx = DL1->return_load_table_index(btt_vec[btt_id].load_table_pointer[i]);
      if(lt_idx != -1) {
        //DL1->load_table_vec[lt_idx].tracking++;
        DL1->load_table_vec[lt_idx].lt_update_tracking(true);
      }
    }
  }
  if(all_ptr_found) {
    for(int i = 0; i < DL1->plq_vec.size(); i++) {
      //clears sp.track and PLQ entry if x is present in PLQ but not in BTT
      auto it = std::find(btt_vec[btt_id].load_table_pointer.begin(), btt_vec[btt_id].load_table_pointer.end(), DL1->plq_vec[i].load_pointer);
      if(it != btt_vec[btt_id].load_table_pointer.end()) {
        //element in PLQ is found in BTT
      }else {
        //No need to track ld-ptr in ld_table and plq if this ld-ptr is not part of current LD-BR slice
        int lt_idx = DL1->return_load_table_index(DL1->plq_vec[i].load_pointer);
        if(lt_idx != -1) {
          DL1->load_table_vec[lt_idx].tracking = 0;
        }
        DL1->plq_vec.erase(DL1->plq_vec.begin() + i);
        DL1->plq_vec.push_back(MemObj::pending_load_queue());
      }
    }
    if(all_ptr_track)
      return 1;
  }

  return 0;
}

#if 0
int OoOProcessor::return_plq_index(AddrType pc) {
  //if hit on load_table, update and move entry to LRU position
  for(int i = PLQ_SIZE - 1; i >= 0; i--) {
    if(pc == plq_vec[i].load_pointer) {
      return i;
    }
  }
  return -1;
}
#endif

DataType OoOProcessor::extract_load_immediate(AddrType pc) {
  AddrType raw_op = esesc_mem_read(pc);
  uint32_t d0 = (raw_op >> 2) & (~0U >> 27);
  uint64_t d1 = (int64_t)(raw_op << 51) >> 63;
  DataType d  = d1 | d0;
  //MSG("op=%llx data=%d", raw_op, d);
  return d;
}

#if 0
void OoOProcessor::generate_trigger_load(DInst *dinst, RegType reg, int lgt_index, int tl_type) {
  uint64_t constant  = 0; //4; //8; //32;
  int64_t delta2    = ct_table[reg].max_mem_lat;                   // max_mem_lat of last 10 dependent LDs
  //AddrType load_addr = 0;
  AddrType trigger_addr = 0;

  constant = inflight_branch;
#if 1
  if(inflight_branch > 1 && inflight_branch <= 4)
    constant = inflight_branch + 8;
  else if(inflight_branch > 4 && inflight_branch <= 7)
    constant = inflight_branch + 12;
  else if(inflight_branch > 7)
    constant = inflight_branch + 16;
#endif

  if(tl_type == 1) {
    ldbp_ldpc          = lgt_table[lgt_index].ldpc;
    ldbp_delta         = lgt_table[lgt_index].ld_delta;
    ldbp_curr_addr     = lgt_table[lgt_index].start_addr;
  }else if(tl_type == 2) {
    ldbp_ldpc          = lgt_table[lgt_index].ldpc2;
    ldbp_delta         = lgt_table[lgt_index].ld_delta2;
    ldbp_curr_addr     = lgt_table[lgt_index].start_addr2;
  }

  int lb_type = lgt_table[lgt_index].ldbr_type;
  AddrType end_addr  = ldbp_curr_addr + ldbp_delta * (DL1->getQSize() - 1);

#if 0
  if(!ct_table[reg].ld_used) {
    //if LD is not executed again before BR, no point in triggering a load
    //update cir_queue and exit
    DL1->no_tl_fill_cir_queue(dinst->getPC(), trigger_addr, lb_type, lgt_table[lgt_index].dep_depth, ldbp_delta, tl_type);
    return;
  }
#endif

  DL1->shift_cir_queue(dinst->getPC(), tl_type);
  DL1->fill_bot_retire(dinst->getPC(), ldbp_ldpc, ldbp_curr_addr, end_addr, ldbp_delta, lb_type, tl_type);
  ct_table[reg].goal_addr = constant + delta2;
  lgt_table[lgt_index].constant = constant + delta2;
  //trigger_addr = constant + delta2;
  //trigger_addr       = ldbp_curr_addr + ldbp_delta*(constant + delta2);
  //trigger_addr       = ldbp_curr_addr + ldbp_delta * lgt_table[lgt_index].constant;
  //trigger_addr       = ldbp_curr_addr + ldbp_delta * ct_table[reg].goal_addr;
  //
#if 1
  trigger_addr = ldbp_curr_addr + ldbp_delta * TRIG_LD_JUMP;
  MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
  if(ldbp_delta != 0) {
    for(int i = 1; i <= TRIG_LD_BURST; i++) {
      AddrType tmp_addr = trigger_addr + (ldbp_delta * i);
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), tmp_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }
  }
#endif

#if 0
  if(dinst->getPC() == 0x1044e)
  MSG("TRIG_LD@1 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%d conf=%u brpc=%llx ldbr=%d", globalClock, ldbp_curr_addr, trigger_addr, ldbp_ldpc, ldbp_delta, delta2, inflight_branch, lgt_table[lgt_index].ld_conf, dinst->getPC(), lb_type);
#endif

#if 0
  trigger_addr       = ldbp_curr_addr + ldbp_delta * ct_table[reg].goal_addr;
  MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
  //if(ldbp_delta != 0 && (last_mem_lat > max_mem_lat)) {
  if(ldbp_delta != 0 && (ct_table[reg].prev_goal_addr != -1) && (ct_table[reg].goal_addr > ct_table[reg].prev_goal_addr)) {
    int diff_mem_lat = (ct_table[reg].goal_addr - ct_table[reg].prev_goal_addr);
    diff_mem_lat = (diff_mem_lat > 8) ? 8 : diff_mem_lat;
    for(int i = 1; i <= diff_mem_lat; i++) {
      trigger_addr       = ldbp_curr_addr + ldbp_delta*(ct_table[reg].prev_goal_addr + i);
#if 0
      if(dinst->getPC() == 0x1044e || dinst->getPC() == 0x11010)
      MSG("TRIG_LD@2 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%u brpc=%llx ldbr=%d", globalClock, ldbp_curr_addr, trigger_addr, lgt_table[lgt_index].ldpc, ldbp_delta, delta2, inflight_branch, dinst->getPC(), lb_type);
#endif
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }
  }else if(ldbp_delta != 0 && (ct_table[reg].goal_addr < ct_table[reg].prev_goal_addr)) {
    int diff_mem_lat = (ct_table[reg].prev_goal_addr - ct_table[reg].goal_addr);
    diff_mem_lat = (diff_mem_lat > 8) ? 8 : diff_mem_lat;
    for(int i = 1; i <= diff_mem_lat; i++) {
      trigger_addr       = ldbp_curr_addr + ldbp_delta*(ct_table[reg].prev_goal_addr - i);
#if 0
      if(dinst->getPC() == 0x1044e || dinst->getPC() == 0x11010)
      MSG("TRIG_LD@3 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%u brpc=%llx ldbr=%d", globalClock, ldbp_curr_addr, trigger_addr, lgt_table[lgt_index].ldpc, ldbp_delta, delta2, inflight_branch, dinst->getPC(), lb_type);
#endif
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }
  }
#endif

#if 0
  trigger_addr       = ldbp_curr_addr + ldbp_delta * lgt_table[lgt_index].constant;
  if(ldbp_delta != 0) {
    if(lgt_table[lgt_index].last_trig_addr == 0) {
      lgt_table[lgt_index].last_trig_addr = trigger_addr;
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }else {
      if(1 || trigger_addr > lgt_table[lgt_index].last_trig_addr) {
        AddrType tmp_addr = 0;
        for(int i = 1; i <= TRIG_LD_BURST; i++) {
          tmp_addr = lgt_table[lgt_index].last_trig_addr + (ldbp_delta * i);
          if(tmp_addr <= trigger_addr) {
            MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), tmp_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
          }
        }
        lgt_table[lgt_index].last_trig_addr = tmp_addr > trigger_addr ? trigger_addr : tmp_addr;
        //lgt_table[lgt_index].last_trig_addr = tmp_addr;
      }
    }
  }
#endif

  //last_mem_lat = max_mem_lat;
  ct_table[reg].prev_goal_addr = ct_table[reg].goal_addr;
  lgt_table[lgt_index].prev_constant = lgt_table[lgt_index].constant;
  ct_table[reg].ld_used = false;
}

void OoOProcessor::ct_br_hit_double(DInst *dinst, RegType b1, RegType b2, int reg_flag) {
#if 1
  ct_table[b1].ldbr_type = 0;
  ct_table[b2].ldbr_type = 0;

  if(ct_table[b1].dep_depth == 1) {
    if(ct_table[b2].dep_depth == 1) { // Br's R1->LD and R2->LD
      ct_table[b1].ldbr_type = 14;
      ct_table[b2].ldbr_type = 14;
    }else if(ct_table[b2].dep_depth > 1) { //Br's R1->LD and R2=>LD->ALU+->BR
      if(ct_table[b2].is_li == 0) {
        ct_table[b1].ldbr_type = 15;
        ct_table[b2].ldbr_type = 15;
      }
    }
  }else if(ct_table[b2].dep_depth == 1) {
    if(ct_table[b1].dep_depth > 1) { //Br's R2->LD and R1=>LD->ALU+->BR
      if(ct_table[b1].is_li == 0) {
        ct_table[b1].ldbr_type = 18;
        ct_table[b2].ldbr_type = 18;
      }
    }
  }
#if 0
  if(ct_table[b1].dep_depth == 1) {
    if(ct_table[b2].dep_depth == 1) {
      ct_table[b1].ldbr_type = 11;
      ct_table[b2].ldbr_type = 11;
    }else if(ct_table[b2].dep_depth > 1) {
      ct_table[b1].ldbr_type = 12;
      ct_table[b2].ldbr_type = 12;
    }
  }else if(ct_table[b2].dep_depth == 1) {
    if(ct_table[b1].dep_depth > 1) {
      ct_table[b1].ldbr_type = 13;
      ct_table[b2].ldbr_type = 13;
    }
  }
#endif

#if 1
  if(ct_table[b1].mv_active && ct_table[b1].mv_src == b2) {
    ct_table[b1].ldbr_type = 22;
    ct_table[b2].ldbr_type = 22;
  }else if(ct_table[b2].mv_active && ct_table[b2].mv_src == b1) {
    ct_table[b1].ldbr_type = 21;
    ct_table[b2].ldbr_type = 21;
  }
#endif

#endif
}

void OoOProcessor::lgt_br_miss_double(DInst *dinst, RegType b1, RegType b2) {
  //fill for LD1
  lgt_table[LGT_SIZE-1].ldpc = ct_table[b1].ldpc;
  lgt_table[LGT_SIZE-1].start_addr = ct_table[b1].ld_addr;

  //fill for LD2
  lgt_table[LGT_SIZE-1].ldpc2 = ct_table[b2].ldpc;
  lgt_table[LGT_SIZE-1].start_addr2 = ct_table[b2].ld_addr;

  lgt_table[LGT_SIZE-1].ldbr_type = ct_table[b1].ldbr_type;
  lgt_table[LGT_SIZE-1].lgt_update_br_fields(dinst);
}

void OoOProcessor::lgt_br_hit_double(DInst *dinst , RegType b1, RegType b2, int i) {
  //fill LD1 fields
  lgt_table[i].prev_delta      = lgt_table[i].ld_delta;
  lgt_table[i].ld_delta        = ct_table[b1].ld_addr - lgt_table[i].start_addr;
  lgt_table[i].start_addr      = ct_table[b1].ld_addr;
  if(lgt_table[i].ld_delta == lgt_table[i].prev_delta) {
    lgt_table[i].ld_conf++;
  }else {
    lgt_table[i].ld_conf = lgt_table[i].ld_conf / 2;
  }
  lgt_table[i].ldbr_type       = ct_table[b1].ldbr_type;
  lgt_table[i].dep_depth       = ct_table[b1].dep_depth;

  //fill LD2 fields
  lgt_table[i].prev_delta2      = lgt_table[i].ld_delta2;
  lgt_table[i].ld_delta2        = ct_table[b2].ld_addr - lgt_table[i].start_addr2;
  lgt_table[i].start_addr2      = ct_table[b2].ld_addr;
  if(lgt_table[i].ld_delta2 == lgt_table[i].prev_delta2) {
    lgt_table[i].ld_conf2++;
  }else {
    lgt_table[i].ld_conf2 = lgt_table[i].ld_conf2 / 2;
  }
  lgt_table[i].dep_depth2       = ct_table[b2].dep_depth;
#if 0
  if(dinst->getPC() == 0x11f32) {
    MSG("LD_CHECK clk=%d ld1=%llx addr1=%d del1=%d conf1=%d ld2=%llx addr2=%d del2=%d conf2=%d", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].ld_conf, lgt_table[i].ldpc2, lgt_table[i].start_addr2, lgt_table[i].ld_delta2, lgt_table[i].ld_conf2);
  }
#endif
  lgt_table[i].lgt_update_br_fields(dinst);
}

void OoOProcessor::classify_ld_br_double_chain(DInst *dinst, RegType b1, RegType b2, int reg_flag) {

  if(ct_table[b1].valid && ct_table[b2].valid) {
    //fill CT table
    ct_br_hit_double(dinst, b1, b2, reg_flag);
#if 0
    //if(dinst->getPC() == 0x19744)
    MSG("CT_BR_DOUBLE clk=%u brpc=%llx id=%u reg_flag=%d reg1=%d reg2=%d ldpc1=%llx ldpc2=%llx ldbr=%d d1=%d d2=%d", globalClock, dinst->getPC(), dinst->getID(), reg_flag, dinst->getInst()->getSrc1(), dinst->getInst()->getSrc2(), ct_table[b1].ldpc, ct_table[b2].ldpc, ct_table[b1].ldbr_type, dinst->getData(), dinst->getData2());
#endif
    //fill LGT table
    bool lgt_hit = false;
    dinst->setLBType(ct_table[b1].ldbr_type);
    int i = hit_on_lgt(dinst, reg_flag, b1, b2);
    /*LGT HIT CRITERIA
     * ct_ldpc == lgt_ldpc && dinst->getPC == lgt_brpc => for TL
     * dinst->getPC == lgt_brpc => for Li
     * */
    if(i != -1) {
      if(ct_table[b1].is_tl_r1r2 && ct_table[b2].is_tl_r1r2) {
        lgt_br_hit_double(dinst, b1, b2, i);
        int ldbr = ct_table[b1].ldbr_type;
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
        if(lgt_table[i].br_mv_outcome > 0) {
          DL1->fill_mv_stats(dinst->getPC(), ldbr, dinst->getData(), dinst->getData2(), lgt_table[i].br_mv_outcome == (dinst->isTaken() + 1), lgt_table[i].br_mv_outcome);
        }
        //generate trigger load
        if(lgt_table[i].ld_conf > 7) { //if LD1 addr is conf, then TL
          if(ldbr != 22) {
            generate_trigger_load(dinst, b1, i, 1);
            dinst->setTrig_ld1_pred();
          }
        }else {
          dinst->setTrig_ld1_unpred();
        }
        if(lgt_table[i].ld_conf2 > 7) { //if LD2 addr is conf, then TL
          if(ldbr != 21) {
            generate_trigger_load(dinst, b2, i, 2);
            dinst->setTrig_ld2_pred();
          }
        }else {
          dinst->setTrig_ld2_unpred();
        }

      }else if(ct_table[b1].is_li_r1r2 && ct_table[b2].is_li_r1r2) {
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
        lgt_table[i].ldbr_type  = ct_table[b1].ldbr_type;
        lgt_table[i].dep_depth  = ct_table[b1].dep_depth;
        lgt_table[i].dep_depth2 = ct_table[b2].dep_depth;
        lgt_table[i].lgt_update_br_fields(dinst);
        DL1->fill_li_at_retire(dinst->getPC(), ct_table[b1].ldbr_type, ct_table[b1].is_li_r1r2, ct_table[b1].is_li_r1r2, ct_table[b1].dep_depth, ct_table[b2].dep_depth, ct_table[b1].li_data, ct_table[b2].li_data);
      }
    }else { //on Miss
      if(ct_table[b1].is_tl_r1r2 && ct_table[b2].is_tl_r1r2) {
        lgt_table.erase(lgt_table.begin());
        lgt_table.push_back(load_gen_table_entry());
        lgt_br_miss_double(dinst, b1, b2);
        int ldbr = ct_table[b1].ldbr_type;
        if(lgt_table[LGT_SIZE-1].br_mv_outcome > 0) {
          DL1->fill_mv_stats(dinst->getPC(), ldbr, dinst->getData(), dinst->getData2(), lgt_table[LGT_SIZE-1].br_mv_outcome == (dinst->isTaken() + 1), lgt_table[LGT_SIZE-1].br_mv_outcome);
        }
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
        ldbp_brpc = dinst->getPC();
      }else if(ct_table[b1].is_li_r1r2 && ct_table[b2].is_li_r1r2) {
        lgt_table.erase(lgt_table.begin());
        lgt_table.push_back(load_gen_table_entry());
        lgt_table[LGT_SIZE-1].ldbr_type = ct_table[b1].ldbr_type;
        lgt_table[LGT_SIZE-1].lgt_update_br_fields(dinst);
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
        DL1->fill_li_at_retire(dinst->getPC(), ct_table[b1].ldbr_type, ct_table[b1].is_li_r1r2, ct_table[b1].is_li_r1r2, ct_table[b1].dep_depth, ct_table[b2].dep_depth, ct_table[b1].li_data, ct_table[b2].li_data);
      }
    }


#if 0
    bool lgt_hit = false;
    for(int i = 0; i < LGT_SIZE; i++) {
      if(lgt_table[i].ldpc == ct_table[b1].ldpc && lgt_table[i].ldpc2 == ct_table[b2].ldpc && lgt_table[i].brpc == dinst->getPC()) {
        lgt_hit = true;
        lgt_br_hit_double(dinst, b1, b2, i);
        if(lgt_table[i].br_mv_outcome > 0) {
          DL1->fill_mv_stats(dinst->getPC(), dinst->getData(), dinst->getData2(), lgt_table[i].br_mv_outcome);
        }
        DL1->fill_retire_count_bot(dinst->getPC());
#if 0
        //send MOV stats to cir_queue
        if(lgt_table[i].ldbr_type == 14) {
          DL1->fill_mv_stats(dinst->getPC(), dinst->getData2(), 1); //
        }else if(lgt_table[i].ldbr_type == 15) {
          DL1->fill_mv_stats(dinst->getPC(), dinst->getData(), 2); //
        }
#endif

        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
#if 0
        if(dinst->getPC() == 0x1d47c)
          MSG("LGT_BR_HIT2 clk=%u brpc=%llx id=%d ldpc1=%llx ld_addr1=%u del1=%d conf1=%u depth1=%d ldpc2=%llx ld_addr2=%u del2=%d conf2=%u depth2=%d d1=%d d2=%d ldbr=%d", globalClock, lgt_table[i].brpc, dinst->getID(), lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].ld_conf, lgt_table[i].dep_depth, lgt_table[i].ldpc2, lgt_table[i].start_addr2, lgt_table[i].ld_delta2, lgt_table[i].ld_conf2, lgt_table[i].dep_depth2, dinst->getData(), dinst->getData2(), lgt_table[i].ldbr_type);
#endif
        //generate trigger load
        if(lgt_table[i].ld_conf > 7) { //if LD1 addr is conf, then TL
          generate_trigger_load(dinst, b1, i, 1);
        }
#if 1
        if(lgt_table[i].ld_conf2 > 7) { //if LD2 addr is conf, then TL
          generate_trigger_load(dinst, b2, i, 2);
        }
#endif
        return;
      }
    }
    if(!lgt_hit) { //IF MISS ON LGT
      lgt_table.erase(lgt_table.begin());
      lgt_table.push_back(load_gen_table_entry());
      lgt_br_miss_double(dinst, b1, b2);
      if(lgt_table[LGT_SIZE-1].br_mv_outcome > 0) {
        DL1->fill_mv_stats(dinst->getPC(), dinst->getData(), dinst->getData2(), lgt_table[LGT_SIZE-1].br_mv_outcome);
      }
      DL1->fill_retire_count_bot(dinst->getPC());
      DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
      ldbp_brpc = dinst->getPC();
    }
#endif
  }
}

void OoOProcessor::classify_ld_br_chain(DInst *dinst, RegType br_src1, int reg_flag) {

  if(ct_table[br_src1].valid) {
    ct_table[br_src1].ct_br_hit(dinst, reg_flag);
#if 0
    MSG("CT_BR clk=%u brpc=%llx reg_flag=%d reg1=%d reg2=%d ldpc=%llx ldbr=%d depth=%d", globalClock, dinst->getPC(), reg_flag, dinst->getInst()->getSrc1(), dinst->getInst()->getSrc2(), ct_table[br_src1].ldpc, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
#endif

    bool lgt_hit = false;
    dinst->setLBType(ct_table[br_src1].ldbr_type);
    int i = hit_on_lgt(dinst, reg_flag, br_src1);
    /*LGT HIT CRITERIA
     * ct_ldpc == lgt_ldpc && dinst->getPC == lgt_brpc => for TL
     * dinst->getPC == lgt_brpc => for Li
     * */
    if(i != -1) {
      if(ct_table[br_src1].is_tl_r1 || ct_table[br_src1].is_tl_r2) {
        RegType rr = LREG_R0;
        uint64_t tl_conf = lgt_table[i].ld_conf;
        if(ct_table[br_src1].is_tl_r1 && ct_table[br_src1].is_li_r2) {
          rr = dinst->getInst()->getSrc2();
          tl_conf = lgt_table[i].ld_conf;
        }else if(ct_table[br_src1].is_tl_r2 && ct_table[br_src1].is_li_r1){
          rr = dinst->getInst()->getSrc1();
          tl_conf = lgt_table[i].ld_conf2;
        }

        //Use TL
        lgt_table[i].lgt_br_hit(dinst, ct_table[br_src1].ld_addr, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth, ct_table[br_src1].is_tl_r1);
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
        if(rr != LREG_R0) {
          ct_table[rr].ldbr_type = ct_table[br_src1].ldbr_type;
          lgt_table[i].lgt_br_hit_li(dinst, ct_table[br_src1].ldbr_type, ct_table[rr].dep_depth);
          DL1->fill_li_at_retire(dinst->getPC(), ct_table[br_src1].ldbr_type, ct_table[br_src1].is_li_r1, ct_table[br_src1].is_li_r2, ct_table[br_src1].dep_depth, 0, ct_table[br_src1].is_li_r1 ? ct_table[rr].li_data:0, ct_table[br_src1].is_li_r2 ? ct_table[rr].li_data:0);
        }
        if(tl_conf > 7) {
#if 0
        MSG("LGT_BR_HIT1 clk=%u ldpc=%llx ld_addr=%u del=%d prev_del=%d conf=%u brpc=%llx d1=%d d2=%d ldbr=%d", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].prev_delta, lgt_table[i].ld_conf, lgt_table[i].brpc, dinst->getData(), dinst->getData2(), lgt_table[i].ldbr_type);
#endif
          generate_trigger_load(dinst, br_src1, i, 1);
          dinst->setTrig_ld1_pred();
        }else {
          dinst->setTrig_ld1_unpred();
        }
      }else if(ct_table[br_src1].is_li_r1 || ct_table[br_src1].is_li_r2) {
        //Use Li
        lgt_table[i].lgt_br_hit_li(dinst, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
        //fill Li value in cir_queue
        DL1->fill_li_at_retire(dinst->getPC(), ct_table[br_src1].ldbr_type, ct_table[br_src1].is_li_r1, ct_table[br_src1].is_li_r2, ct_table[br_src1].dep_depth, 0, ct_table[br_src1].is_li_r1 ? ct_table[br_src1].li_data:0, ct_table[br_src1].is_li_r2 ? ct_table[br_src1].li_data:0);
      }
    }else { //miss on LGT
      //insert in LGT on miss only if BR is a legal LDBR type. If LDBR == 0, don't insert in LGT
      if(ct_table[br_src1].is_tl_r1 || ct_table[br_src1].is_tl_r2) {
        lgt_table.erase(lgt_table.begin());
        lgt_table.push_back(load_gen_table_entry());
        RegType rr = LREG_R0;
        if(ct_table[br_src1].is_tl_r1 && ct_table[br_src1].is_li_r2) {
          rr = dinst->getInst()->getSrc2();
        }else if(ct_table[br_src1].is_tl_r2 && ct_table[br_src1].is_li_r1){
          rr = dinst->getInst()->getSrc1();
        }
        lgt_table[LGT_SIZE-1].lgt_br_miss(dinst, ct_table[br_src1].ldbr_type, ct_table[br_src1].ldpc, ct_table[br_src1].ld_addr, ct_table[br_src1].is_tl_r1);
        ldbp_brpc = dinst->getPC();
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
        if(rr != LREG_R0) {
          ct_table[rr].ldbr_type = ct_table[br_src1].ldbr_type;
          lgt_table[i].lgt_br_hit_li(dinst, ct_table[br_src1].ldbr_type, ct_table[rr].dep_depth);
          DL1->fill_li_at_retire(dinst->getPC(), ct_table[br_src1].ldbr_type, ct_table[br_src1].is_li_r1, ct_table[br_src1].is_li_r2, ct_table[br_src1].dep_depth, 0, ct_table[br_src1].is_li_r1 ? ct_table[rr].li_data:0, ct_table[br_src1].is_li_r2 ? ct_table[rr].li_data:0);
        }
      }else if(ct_table[br_src1].is_li_r1 || ct_table[br_src1].is_li_r2) {
        lgt_table.erase(lgt_table.begin());
        lgt_table.push_back(load_gen_table_entry());
        //lgt_table[LGT_SIZE-1].lgt_br_miss(dinst, ct_table[br_src1].ldbr_type, 0, 0);
        lgt_table[i].lgt_br_hit_li(dinst, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
        DL1->fill_li_at_retire(dinst->getPC(), ct_table[br_src1].ldbr_type, ct_table[br_src1].is_li_r1, ct_table[br_src1].is_li_r2, ct_table[br_src1].dep_depth, 0, ct_table[br_src1].is_li_r1 ? ct_table[br_src1].li_data:0, ct_table[br_src1].is_li_r2 ? ct_table[br_src1].li_data:0);
      }
    }

#if 0
    for(int i    = 0; i < LGT_SIZE; i++) {
      if(lgt_table[i].ldpc == ct_table[br_src1].ldpc && lgt_table[i].brpc == dinst->getPC()) { // hit
        lgt_hit = true;
        lgt_table[i].lgt_br_hit(dinst, ct_table[br_src1].ld_addr, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
        DL1->fill_retire_count_bot(dinst->getPC());
        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
#if 0
        if(dinst->getPC() == 0x1d47c)
        MSG("LGT_BR_HIT1 clk=%u ldpc=%llx ld_addr=%u del=%d prev_del=%d conf=%u brpc=%llx d1=%d d2=%d ldbr=%d", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].prev_delta, lgt_table[i].ld_conf, lgt_table[i].brpc, dinst->getData(), dinst->getData2(), lgt_table[i].ldbr_type);
#endif
        if(lgt_table[i].ld_conf > 7) {
          generate_trigger_load(dinst, br_src1, i, 1);
        }
        return;
      }
    }
    if(!lgt_hit) { // miss on LGT
      lgt_table.erase(lgt_table.begin());
      lgt_table.push_back(load_gen_table_entry());
      lgt_table[LGT_SIZE-1].lgt_br_miss(dinst, ct_table[br_src1].ldpc, ct_table[br_src1].ld_addr, ct_table[br_src1].ldbr_type);
      DL1->fill_retire_count_bot(dinst->getPC());
      DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[LGT_SIZE-1].hit2_miss3, lgt_table[LGT_SIZE-1].hit3_miss2);
      ldbp_brpc = dinst->getPC();
      int i = LGT_SIZE - 1;
    }
#endif
  }
}

int OoOProcessor::hit_on_lgt(DInst *dinst, int reg_flag, RegType reg1, RegType reg2) {
  int ldbr     = ct_table[reg1].ldbr_type;
  int tl_r1[]    = {1, 3, 12, 16, 17};
  int tl_r2[]    = {2, 5, 9, 19, 20};
  int tl_r1_r2[] = {14, 15, 18, 21, 22};
  int li_r1[]    = {4, 7, 9, 19, 20};
  int li_r2[]    = {6, 8, 12, 16, 17};
  int li_r1_r2[] = {10, 11, 13};
  //int mv_r1r2[] = {21, 22};
  int reg_flag_r1[] = {1, 3, 6};
  int reg_flag_r2[] = {2, 4, 7};
#if 0
  is_tl_r1 = false;
  is_tl_r2 = false;
  is_tl_r1r2 = false;
  is_li_r1 = false;
  is_li_r2 = false;
  is_li_r1r2 = false;
#endif

  if((std::find(std::begin(reg_flag_r1), std::end(reg_flag_r1), reg_flag) != std::end(reg_flag_r1)) || (std::find(std::begin(reg_flag_r2), std::end(reg_flag_r2), reg_flag) != std::end(reg_flag_r2))) { //R1 or R2
    ct_table[reg1].is_tl_r1 = false;
    ct_table[reg1].is_tl_r2 = false;
    ct_table[reg1].is_tl_r1r2 = false;
    ct_table[reg1].is_li_r1 = false;
    ct_table[reg1].is_li_r2 = false;
    ct_table[reg1].is_li_r1r2 = false;
    ct_table[reg1].is_tl_r1 = std::find(std::begin(tl_r1), std::end(tl_r1), ldbr) != std::end(tl_r1);
    ct_table[reg1].is_li_r1 = std::find(std::begin(li_r1), std::end(li_r1), ldbr) != std::end(li_r1);
    ct_table[reg1].is_tl_r2 = std::find(std::begin(tl_r2), std::end(tl_r2), ldbr) != std::end(tl_r2);
    ct_table[reg1].is_li_r2 = std::find(std::begin(li_r2), std::end(li_r2), ldbr) != std::end(li_r2);
#if 0
  }else if(std::find(std::begin(reg_flag_r2), std::end(reg_flag_r2), reg_flag) != std::end(reg_flag_r2)) { //R2
    ct_table[reg1].is_tl_r1 = false;
    ct_table[reg1].is_tl_r2 = false;
    ct_table[reg1].is_tl_r1r2 = false;
    ct_table[reg1].is_li_r1 = false;
    ct_table[reg1].is_li_r2 = false;
    ct_table[reg1].is_li_r1r2 = false;
    ct_table[reg1].is_tl_r1 = std::find(std::begin(tl_r1), std::end(tl_r1), ldbr) != std::end(tl_r1);
    ct_table[reg1].is_li_r1 = std::find(std::begin(li_r1), std::end(li_r1), ldbr) != std::end(li_r1);
    ct_table[reg1].is_tl_r2 = std::find(std::begin(tl_r2), std::end(tl_r2), ldbr) != std::end(tl_r2);
    ct_table[reg1].is_li_r2 = std::find(std::begin(li_r2), std::end(li_r2), ldbr) != std::end(li_r2);
#endif
  }else {
    //Type with 2 LD/Li
    ct_table[reg1].is_tl_r1 = false;
    ct_table[reg1].is_tl_r2 = false;
    ct_table[reg1].is_tl_r1r2 = false;
    ct_table[reg1].is_li_r1 = false;
    ct_table[reg1].is_li_r2 = false;
    ct_table[reg1].is_li_r1r2 = false;

    ct_table[reg2].is_tl_r1 = false;
    ct_table[reg2].is_tl_r2 = false;
    ct_table[reg2].is_tl_r1r2 = false;
    ct_table[reg2].is_li_r1 = false;
    ct_table[reg2].is_li_r2 = false;
    ct_table[reg2].is_li_r1r2 = false;

    ct_table[reg1].is_tl_r1r2 = std::find(std::begin(tl_r1_r2), std::end(tl_r1_r2), ldbr) != std::end(tl_r1_r2);
    ct_table[reg1].is_li_r1r2 = std::find(std::begin(li_r1_r2), std::end(li_r1_r2), ldbr) != std::end(li_r1_r2);
    ct_table[reg2].is_tl_r1r2 = ct_table[reg1].is_tl_r1r2;
    ct_table[reg2].is_li_r1r2 = ct_table[reg1].is_li_r1r2;
  }

  for(int i = 0; i < LGT_SIZE; i++) {
    if((ct_table[reg1].is_tl_r1 || ct_table[reg1].is_tl_r2) && (lgt_table[i].ldpc == ct_table[reg1].ldpc) && (lgt_table[i].brpc == dinst->getPC())) {
      //If TL is needed
      return i;
    }else if(ct_table[reg1].is_tl_r1r2 && (lgt_table[i].ldpc == ct_table[reg1].ldpc) && (lgt_table[i].ldpc2 == ct_table[reg2].ldpc) && (lgt_table[i].brpc == dinst->getPC())) {
      //If double TL is needed
      return i;
    }else if((ct_table[reg1].is_li_r1 || ct_table[reg1].is_li_r2 || ct_table[reg1].is_li_r1r2) && (lgt_table[i].brpc == dinst->getPC())) {
      //No need to check for LD match for Li. Hit if BR pc matches
      return i;
    }
  }

  return -1;
}

#endif

#endif

void OoOProcessor::retire()
/* Try to retire instructions {{{1 */
{
  // Pass all the ready instructions to the rrob
  while(!ROB.empty()) {
    DInst *dinst = ROB.top();
    uint64_t num_inflight_branches = 0;

    bool done = dinst->getClusterResource()->preretire(dinst, flushing);
    //AddrType ppc = dinst->getPC();
    //MSG("MV");
    GI(flushing && dinst->isExecuted(), done);
    if(!done) {
      break;
    }

#ifdef ENABLE_LDBP
#if 0
    //extracting Li's immediate value
    if(dinst->getPC() == 0x10400 || dinst->getPC() == 0x10402) {
      AddrType value = esesc_mem_read(dinst->getPC());
      uint32_t d0 = (value >> 2) & (~0U >> 27);
      uint64_t d1 = (int64_t)(value << 51) >> 63;
      DataType d  = d1 | d0;
      MSG("op=%llx data=%d", value, d);
    }
#endif
    if(dinst->getInst()->isLoad()) {
#if 0
      num_mem_lat++;
      //use max mem latency of last 10 loads to calculate trigger load address
      if(num_mem_lat >= 10) {
        num_mem_lat   = 0;
        mem_lat_vec.clear();
      }
      mem_lat_vec.push_back((globalClock - dinst->getExecutingTime()));
      max_mem_lat = *std::max_element(mem_lat_vec.begin(), mem_lat_vec.end());
      if(max_mem_lat > 8) {
        max_mem_lat = 8;
      }
#endif

#ifdef NEW_LDBP_INTERFACE
      DL1->hit_on_load_table(dinst, false);
      //int lt_idx = return_load_table_index(dinst->getPC());
      rtt_load_hit(dinst);
#endif

#if 0
      RegType ld_dst = dinst->getInst()->getDst1();
      if(ld_dst <= LREG_R31) {
        ct_table[ld_dst].ct_load_hit(dinst);
        ct_table[ld_dst].num_mem_lat++;
        //use max mem latency of last 10 loads to calculate trigger load address
        if(ct_table[ld_dst].num_mem_lat >= 10) {
          ct_table[ld_dst].num_mem_lat   = 0;
          //ct_table[ld_dst].mem_lat_vec.clear();
        }
        //ct_table[ld_dst].mem_lat_vec.push_back((globalClock - dinst->getExecutingTime()));
        ct_table[ld_dst].mem_lat_vec[ct_table[ld_dst].num_mem_lat] = (globalClock - dinst->getExecutingTime());
        ct_table[ld_dst].max_mem_lat = *std::max_element(ct_table[ld_dst].mem_lat_vec.begin(), ct_table[ld_dst].mem_lat_vec.end());
        //ct_table[ld_dst].max_mem_lat = std::accumulate(ct_table[ld_dst].mem_lat_vec.begin(), ct_table[ld_dst].mem_lat_vec.end(), 0.0) / ct_table[ld_dst].mem_lat_vec.size();
        if(ct_table[ld_dst].max_mem_lat > 16) {
          //ct_table[ld_dst].max_mem_lat = 16;
        }
      }
#endif
    }

#ifdef NEW_LDBP_INTERFACE
    //handle complex ALU in RTT
    if(dinst->getInst()->isComplex()) {
      RegType dst  = dinst->getInst()->getDst1();
      rtt_vec[dst].num_ops = NUM_OPS + 1;
    }
#endif

    if(dinst->getInst()->isALU()) { //if ALU hit on ldbp_retire_table
      RegType alu_dst = dinst->getInst()->getDst1();
      RegType alu_src1 = dinst->getInst()->getSrc1();
      RegType alu_src2 = dinst->getInst()->getSrc2();
#ifdef NEW_LDBP_INTERFACE
      if(alu_src1 == LREG_R0 && alu_src2 == LREG_R0) { //Load Immediate
        //DL1->hit_on_load_table(dinst, true);
        //rtt_load_hit(dinst); //load_table_vec index is always 0 for Li; index 1 to LOAD_TABLE_SIZE is for other LDs
        //Treat Li as ALU and not as LOAD (FIXME???????????)
        rtt_alu_hit(dinst);
      }else { // other ALUs
        rtt_alu_hit(dinst);
      }
#endif

#if 0
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = false;
        ct_table[alu_dst].mv_src    = LREG_R0;
        ct_table[alu_dst].ct_alu_hit(dinst);
        if(alu_src1 == LREG_R0 && alu_src2 == LREG_R0) { //if load-immediate
          ct_table[alu_dst].is_li = 1;
          ct_table[alu_dst].lipc  = dinst->getPC();
          ct_table[alu_dst].li_data = extract_load_immediate(dinst->getPC());
#if 0
          MSG("CT_ALU_Li clk=%u alu_pc=%llx dst_reg=%d depth=%d ldpc=%llx", globalClock, dinst->getPC(), alu_dst, ct_table[alu_dst].dep_depth, ct_table[alu_dst].ldpc);
#endif
        }else if(ct_table[alu_dst].is_li > 0){
          ct_table[alu_dst].is_li = 2;
        }
      }
#endif

    }

#if 1
    // condition to find a MV instruction
    if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() != LREG_R0 && dinst->getInst()->getSrc2() == LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      rtt_alu_hit(dinst);
#if 0
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc1();
      }
#endif
    }else if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() == LREG_R0 && dinst->getInst()->getSrc2() != LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      rtt_alu_hit(dinst);
#if 0
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc2();
      }
#endif
    }
#endif

    if(dinst->getInst()->getOpcode() == iBALU_LBRANCH) {
      //compute num of inflight branches
      for(uint32_t i = 0; i < ROB.size(); i++) { //calculate num of inflight branches
        uint32_t pos   = ROB.getIDFromTop(i);
        DInst*  tmp_dinst = ROB.getData(pos);
        if(tmp_dinst->getInst()->isBranch() && (tmp_dinst->getPC() == dinst->getPC()))
          num_inflight_branches++;
      }
      inflight_branch = num_inflight_branches;
      dinst->setInflight(inflight_branch);

      rtt_br_hit(dinst);
#if 0
      int btt_id = return_btt_index(dinst->getPC());
      if(btt_id != -1) {
        btt_br_hit(dinst, btt_id);
      }
#endif

#if 0
      //classify Br
      RegType br_src1 = dinst->getInst()->getSrc1();
      RegType br_src2 = dinst->getInst()->getSrc2();
      if(br_src2 == LREG_R0) {
        classify_ld_br_chain(dinst, br_src1, 1);//update CT on Br hit
      }else if(br_src1 == LREG_R0) {
        classify_ld_br_chain(dinst, br_src2, 2);//update CT on Br hit
      }else if(br_src1 != LREG_R0 && br_src2 != LREG_R0) {
        if(ct_table[br_src1].is_li == 0 && ct_table[br_src2].is_li == 0){
          classify_ld_br_double_chain(dinst, br_src1, br_src2, 5);
        }else{
          if(ct_table[br_src2].is_li == 1) { //br_src2 is Li
            classify_ld_br_chain(dinst, br_src1, 3);
          }else if(ct_table[br_src1].is_li == 1) { //br_src1 is Li
            classify_ld_br_chain(dinst, br_src2, 4);
          }else if(ct_table[br_src2].is_li == 2) { //br_src2 is Li->ALU+
            classify_ld_br_chain(dinst, br_src1, 6);
          }else if(ct_table[br_src1].is_li == 2) { //br_src1 is Li->ALU+
            classify_ld_br_chain(dinst, br_src2, 7);
          }
        }
      }
#endif

    }

#endif

    I(IFID.getMissDInst() != dinst);

    rROB.push(dinst);
    ROB.pop();
  }

  if(!ROB.empty() && ROB.top()->getStatsFlag()) {
    robUsed.sample(ROB.size(), true);
#ifdef TRACK_TIMELEAK
    int total_hit  = 0;
    int total_miss = 0;
    for(uint32_t i = 0; i < ROB.size(); i++) {
      uint32_t pos   = ROB.getIDFromTop(i);
      DInst *  dinst = ROB.getData(pos);

      if(!dinst->getStatsFlag())
        continue;
      if(!dinst->getInst()->isLoad())
        continue;
      if(dinst->isPerformed())
        continue;

      if(dinst->isFullMiss())
        total_miss++;
      else
        total_hit++;
    }
    avgPNRHitLoadSpec.sample(total_hit, true);
    avgPNRMissLoadSpec.sample(true, total_miss);
#endif
  }

  if(!rROB.empty()) {
    rrobUsed.sample(rROB.size(), rROB.top()->getStatsFlag());

#ifdef ESESC_CODEPROFILE
    if(rROB.top()->getStatsFlag()) {
      if(codeProfile_trigger <= clockTicks.getDouble()) {
        DInst *dinst = rROB.top();

        codeProfile_trigger = clockTicks.getDouble() + 121;

        double wt    = dinst->getIssuedTime() - dinst->getRenamedTime();
        double et    = dinst->getExecutedTime() - dinst->getIssuedTime();
        bool   flush = dinst->isBranchMiss();

        codeProfile.sample(rROB.top()->getPC(), nCommitted.getDouble(), clockTicks.getDouble(), wt, et, flush, dinst->isPrefetch(), dinst->isBranchMiss_level1(), dinst->isBranchMiss_level2(), dinst->isBranchMiss_level3(), dinst->isBranchHit_level1(), dinst->isBranchHit_level2(), dinst->isBranchHit_level3(), dinst->isBranch_hit2_miss3(), dinst->isBranch_hit3_miss2());
      }
    }
#endif
  }

  for(uint16_t i = 0; i < RetireWidth && !rROB.empty(); i++) {
    DInst *dinst = rROB.top();

    if((dinst->getExecutedTime() + RetireDelay) >= globalClock) {
#if 0
      if (rROB.size()>8) {
        dinst->getInst()->dump("not ret");
        printf("----------------------\n");
        dumpROB();
      }
#endif
      break;
    }

    I(dinst->getCluster());

    bool done = dinst->getCluster()->retire(dinst, flushing);
    if(!done)
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
    if(dinst->isReplay()) {
      flushing     = true;
      flushing_fid = fid;
    }

#ifdef FETCH_TRACE
    // Call eint->markTrace
#endif

    nCommitted.inc(!flushing && dinst->getStatsFlag());

#ifdef ESESC_BRANCHPROFILE
    if(dinst->getInst()->isBranch() && dinst->getStatsFlag()) {
      codeProfile.sample(dinst->getPC(), dinst->getID(), 0, dinst->isBiasBranch() ? 1.0 : 0, 0, dinst->isBranchMiss(), dinst->isPrefetch(), dinst->getLBType(), dinst->isBranchMiss_level1(), dinst->isBranchMiss_level2(), dinst->isBranchMiss_level3(), dinst->isBranchHit_level1(), dinst->isBranchHit_level2(), dinst->isBranchHit_level3(), dinst->isBranch_hit2_miss3(), dinst->isBranch_hit3_miss2(), dinst->isTrig_ld1_pred(), dinst->isTrig_ld1_unpred(), dinst->isTrig_ld2_pred(), dinst->isTrig_ld2_unpred());
      AddrType p = dinst->getPC();
      //MSG("BR_PROFILE clk=%u brpc=%llx br_miss=%d\n", globalClock, dinst->getPC(), dinst->isBranchMiss());
    }
#endif
#if 0
    if (dinst->getPC() == 0x1100c || dinst->getPC() == 0x11008)
      MSG("TR %8llx %lld\n",dinst->getPC(), dinst->getAddr());
#endif
#ifdef ESESC_TRACE2
    MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d   %lld %lld %lld %lld %lld", dinst->getID(), dinst->getPC(),
        dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
        dinst->getInst()->getSrc2(), dinst->getFetchedTime(), dinst->getRenamedTime(), dinst->getIssuedTime(),
        dinst->getExecutedTime(), globalClock);
#endif
#ifdef ESESC_TRACE
    MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d   %lld %lld %lld %lld %lld", dinst->getID(), dinst->getPC(),
        dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
        dinst->getInst()->getSrc2(), dinst->getFetchedTime() - globalClock, dinst->getRenamedTime() - globalClock,
        dinst->getIssuedTime() - globalClock, dinst->getExecutedTime() - globalClock, globalClock - globalClock);
#endif
    // sjeng 2nd: if (dinst->getPC() == 0x1d46c || dinst->getPC() == 0x1d462)
#if 0
    if (dinst->getPC() == 0x10c96 || dinst->getPC() == 0x10c9a || dinst->getPC() == 0x10c9e)
      MSG("TR %8lld %8llx R%-2d,R%-2d=R%-2d op=%-2d R%-2d  %lld %lld %lld", dinst->getID(), dinst->getPC(),
          dinst->getInst()->getDst1(), dinst->getInst()->getDst2(), dinst->getInst()->getSrc1(), dinst->getInst()->getOpcode(),
          dinst->getInst()->getSrc2(), dinst->getData(), dinst->getData2(), dinst->getAddr());
#endif

#if 0
    dinst->dump("RT ");
    fprintf(stderr,"\n");
#endif

#ifdef WAVESNAP_EN
//updading wavesnap instruction windows
if(SINGLE_WINDOW) {
  if(WITH_SAMPLING) {
    if(!flushing && dinst->getStatsFlag()) {
      snap->update_single_window(dinst, (uint64_t)globalClock);
    }
  } else {
    snap->update_single_window(dinst, (uint64_t)globalClock);
  }
} else {
  if(WITH_SAMPLING) {
    if(dinst->getStatsFlag()) {
      snap->update_window(dinst, (uint64_t)globalClock);
    }
  } else {
    snap->update_window(dinst, (uint64_t)globalClock);
  }

}
#endif
    if(dinst->getInst()->hasDstRegister())
      nTotalRegs++;

    if(!dinst->getInst()->isStore()) // Stores can perform after retirement
      I(dinst->isPerformed());

    if(dinst->isPerformed()) // Stores can perform after retirement
      dinst->destroy(eint);
    else {
      eint->reexecuteTail(fid);
    }

    if(last_serialized == dinst)
      last_serialized = 0;
    if(last_serializedST == dinst)
      last_serializedST = 0;

    rROB.pop();
  }
}
/* }}} */

void OoOProcessor::replay(DInst *target)
/* trigger a processor replay {{{1 */
{
  if(serialize_for)
    return;

  I(serialize_for <= 0);
  // Same load can be marked by several stores in a OoO core : I(replayID != target->getID());
  I(target->getInst()->isLoad());

  if(!MemoryReplay) {
    return;
  }
  target->markReplay();

  if(replayID < target->getID())
    replayID = target->getID();

  if(replayRecovering)
    return;
  replayRecovering = true;

  // Count the # instructions wasted
  size_t fetch2rename = 0;
  fetch2rename += (InstQueueSize - spaceInInstQueue);
  fetch2rename += pipeQ.pipeLine.size();

  nReplayInst.sample(fetch2rename + ROB.size(), target->getStatsFlag());
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
