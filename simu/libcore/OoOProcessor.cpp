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

#include "SescConf.h"

#include "OoOProcessor.h"

#include "EmuSampler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "TaskHandler.h"
#include "FastQueue.h"
#include "MemRequest.h"

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
  num_mem_lat         = 0;
  last_mem_lat        = 0;
  diff_mem_lat        = 0;
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

void OoOProcessor::generate_trigger_load(DInst *dinst, RegType reg, int lgt_index, int tl_type) {

  uint64_t constant  = 0; //4; //8; //32;
  int64_t delta2    = max_mem_lat;                   // max_mem_lat of last 10 dependent LDs
  //AddrType load_addr = 0;
  AddrType trigger_addr = 0;

  if(inflight_branch > 1 && inflight_branch <= 4)
    constant = inflight_branch + 8;
  else if(inflight_branch > 4 && inflight_branch <= 7)
    constant = inflight_branch + 12;
  else if(inflight_branch > 7)
    constant = inflight_branch + 16;

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
  if(dinst->getPC() == 0x19744)
    MSG("TRIG_LD@1 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%d conf=%u brpc=%llx", globalClock, ldbp_curr_addr, trigger_addr, ldbp_ldpc, ldbp_delta, delta2, inflight_branch, lgt_table[lgt_index].ld_conf, dinst->getPC());
#endif

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
  trigger_addr       = ldbp_curr_addr + ldbp_delta*(constant + delta2);

#if 1
  //if(ldbp_delta != 0)
  MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
#if 1
  if(ldbp_delta != 0 && (last_mem_lat > max_mem_lat)) {
    diff_mem_lat = last_mem_lat - max_mem_lat + 6;
    for(int i = 1; i <= diff_mem_lat; i++) {
      trigger_addr       = ldbp_curr_addr + ldbp_delta*(constant + delta2 + i);
#if 0
      if(dinst->getPC() == 0x19744)
        MSG("TRIG_LD@2 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%u brpc=%llx", globalClock, ldbp_curr_addr, trigger_addr, lgt_table[lgt_index].ldpc, ldbp_delta, delta2, inflight_branch, dinst->getPC());
#endif
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }
  }else if(ldbp_delta != 0 && (last_mem_lat < max_mem_lat)) {
    diff_mem_lat = max_mem_lat - last_mem_lat + 6;
    for(int i = diff_mem_lat; i > 0; i--) {
      trigger_addr       = ldbp_curr_addr + ldbp_delta*(constant + delta2 - i);
#if 0
      if(dinst->getPC() == 0x19744)
        MSG("TRIG_LD@3 clk=%u curr_addr=%u trig_addr=%u ldpc=%llx delta=%d max_lat=%u inf=%u brpc=%llx", globalClock, ldbp_curr_addr, trigger_addr, lgt_table[lgt_index].ldpc, ldbp_delta, delta2, inflight_branch, dinst->getPC());
#endif
      MemRequest::triggerReqRead(DL1, dinst->getStatsFlag(), trigger_addr, ldbp_ldpc, dinst->getPC(), ldbp_curr_addr, end_addr, ldbp_delta, inflight_branch, lb_type, lgt_table[lgt_index].dep_depth, tl_type, ct_table[reg].ld_used);
    }
  }
#endif
#endif
  last_mem_lat = max_mem_lat;
  ct_table[reg].ld_used = false;
}

void OoOProcessor::ct_br_hit_double(DInst *dinst, RegType b1, RegType b2, int reg_flag) {
#if 1
  ct_table[b1].ldbr_type = 0;
  ct_table[b2].ldbr_type = 0;
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

#if 1
  if(ct_table[b1].mv_active && ct_table[b1].mv_src == b2) {
    ct_table[b1].ldbr_type = 15;
    ct_table[b2].ldbr_type = 15;
  }else if(ct_table[b2].mv_active && ct_table[b2].mv_src == b1) {
    ct_table[b1].ldbr_type = 14;
    ct_table[b2].ldbr_type = 14;
  }
#endif

#endif
}

void OoOProcessor::lgt_br_miss_double(DInst *dinst, RegType b1, RegType b2) {
  //fill for LD1
  lgt_table[LGT_SIZE-1].ldpc = ct_table[b1].ldpc;
  lgt_table[LGT_SIZE-1].start_addr = ct_table[b1].ld_addr;
  lgt_table[LGT_SIZE-1].ldbr_type = ct_table[b1].ldbr_type;

  //fill for LD2
  lgt_table[LGT_SIZE-1].ldpc2 = ct_table[b2].ldpc;
  lgt_table[LGT_SIZE-1].start_addr2 = ct_table[b2].ld_addr;

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

  lgt_table[i].lgt_update_br_fields(dinst);
}

void OoOProcessor::classify_ld_br_double_chain(DInst *dinst, RegType b1, RegType b2, int reg_flag) {

  if(ct_table[b1].valid && ct_table[b2].valid) {
    //fill CT table
    ct_br_hit_double(dinst, b1, b2, reg_flag);
#if 0
    MSG("CT_BR clk=%u brpc=%llx ldpc1=%llx ldbr1=%d depth1=%d ldpc2=%llx ldbr2=%d depth2=%d", globalClock, dinst->getPC(), ct_table[b1].ldpc, ct_table[b1].ldbr_type, ct_table[b1].dep_depth, ct_table[b2].ldpc, ct_table[b2].ldbr_type, ct_table[b2].dep_depth);
#endif
    //fill LGT table
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
  }
}

void OoOProcessor::classify_ld_br_chain(DInst *dinst, RegType br_src1, int reg_flag) {

  if(ct_table[br_src1].valid) {
    ct_table[br_src1].ct_br_hit(dinst, reg_flag);
    int tmp = ct_table[br_src1].ldbr_type;
#if 0
    MSG("CT_BR clk=%u brpc=%llx reg_flag=%d reg1=%d reg2=%d ldpc=%llx ldbr=%d depth=%d", globalClock, dinst->getPC(), reg_flag, dinst->getInst()->getSrc1(), dinst->getInst()->getSrc2(), ct_table[br_src1].ldpc, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
#endif

    if(reg_flag == 3 && !ct_table[br_src1].complex_br_set) {
      return;
    }else if(reg_flag == 4 && !ct_table[br_src1].complex_br_set) {
      return;
    }

    bool lgt_hit = false;
    for(int i    = 0; i < LGT_SIZE; i++) {
      if(lgt_table[i].ldpc == ct_table[br_src1].ldpc && lgt_table[i].brpc == dinst->getPC()) { // hit
        lgt_hit = true;
        lgt_table[i].lgt_br_hit(dinst, ct_table[br_src1].ld_addr, ct_table[br_src1].ldbr_type, ct_table[br_src1].dep_depth);
        //brpc_count++;
        DL1->fill_retire_count_bot(dinst->getPC());

        DL1->fill_bpred_use_count_bot(dinst->getPC(), lgt_table[i].hit2_miss3, lgt_table[i].hit3_miss2);
        /*if(ldbp_brpc != dinst->getPC()) {
          brpc_count = 1;
          ldbp_brpc  = dinst->getPC();
        }*/
        //DL1->setRetBrCount(brpc_count);
#if 0
        if(dinst->getPC() == 0x1d47c)
        MSG("LGT_BR_HIT1 clk=%u ldpc=%llx ld_addr=%u del=%d prev_del=%d conf=%u brpc=%llx d1=%d d2=%d ldbr=%d", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].prev_delta, lgt_table[i].ld_conf, lgt_table[i].brpc, dinst->getData(), dinst->getData2(), lgt_table[i].ldbr_type);
#endif
        //MSG("I=%d", i);
        if(lgt_table[i].ld_conf > 7) {
#if 0
          MSG("LGT_BR_HIT clk=%u ldpc=%llx ld_addr=%u del=%u prev_del=%u conf=%u brpc=%llx ldbr=%d r_count=%u", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].prev_delta, lgt_table[i].ld_conf, lgt_table[i].brpc, lgt_table[i].ldbr_type, brpc_count);
#endif
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
      //brpc_count = 1;
      ldbp_brpc = dinst->getPC();
      //DL1->setRetBrCount(brpc_count);
      int i = LGT_SIZE - 1;
#if 0
      MSG("LGT_BR_MISS clk=%u ldpc=%llx ld_addr=%u del=%u prev_del=%u conf=%u brpc=%llx ldbr=%d r_count=%u", globalClock, lgt_table[i].ldpc, lgt_table[i].start_addr, lgt_table[i].ld_delta, lgt_table[i].prev_delta, lgt_table[i].ld_conf, lgt_table[i].brpc, lgt_table[i].ldbr_type, brpc_count);
#endif
      //lgt_update_on_miss(dinst, ct_table[br_src1].ldpc, ct_table[br_src1].ld_addr, ct_table[br_src1].ldbr_type);
    }
  }

}
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
    if(dinst->getInst()->isLoad()) {
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

      RegType ld_dst = dinst->getInst()->getDst1();
      ct_table[ld_dst].ct_load_hit(dinst);
    }

    if(dinst->getInst()->getOpcode() == iAALU) { //if ALU hit on ldbp_retire_table
      RegType alu_dst = dinst->getInst()->getDst1();
      RegType alu_src1 = dinst->getInst()->getSrc1();
      RegType alu_src2 = dinst->getInst()->getSrc2();
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = false;
        ct_table[alu_dst].mv_src    = LREG_R0;
        ct_table[alu_dst].ct_alu_hit(dinst);
        if(alu_src1 == LREG_R0 && alu_src2 == LREG_R0) { //if load-immediate
          ct_table[alu_dst].is_li = true;
#if 0
          MSG("CT_ALU_Li clk=%u alu_pc=%llx dst_reg=%d depth=%d ldpc=%llx", globalClock, dinst->getPC(), alu_dst, ct_table[alu_dst].dep_depth, ct_table[alu_dst].ldpc);
#endif
        }else {
          ct_table[alu_dst].is_li = false;
        }
      }
    }

#if 1
    // condition to find a MV instruction
    if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() != LREG_R0 && dinst->getInst()->getSrc2() == LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc1();
      }
    }else if(dinst->getInst()->getOpcode() == iRALU && (dinst->getInst()->getSrc1() == LREG_R0 && dinst->getInst()->getSrc2() != LREG_R0)) {
      RegType alu_dst = dinst->getInst()->getDst1();
      if(ct_table[alu_dst].valid) {
        ct_table[alu_dst].mv_active = true;
        ct_table[alu_dst].mv_src    = dinst->getInst()->getSrc2();
      }
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

      //classify Br
      RegType br_src1 = dinst->getInst()->getSrc1();
      RegType br_src2 = dinst->getInst()->getSrc2();
#if 1
      if(br_src2 == LREG_R0) {
        classify_ld_br_chain(dinst, br_src1, 1);//update CT on Br hit
      }else if(br_src1 == LREG_R0) {
        classify_ld_br_chain(dinst, br_src2, 2);//update CT on Br hit
      }else if(br_src1 != LREG_R0 && br_src2 != LREG_R0) {
        if(!ct_table[br_src1].is_li && !ct_table[br_src2].is_li){
#if 0
          ct_table[br_src1].ldbr_type = 0;
          ct_table[br_src2].ldbr_type = 0;
#endif
          classify_ld_br_double_chain(dinst, br_src1, br_src2, 5);
        }else{
          classify_ld_br_chain(dinst, br_src1, 3);
          if(!ct_table[br_src1].complex_br_set) { //br_src1 is Li
            classify_ld_br_chain(dinst, br_src2, 4);
          }
        }
#if 0
#endif
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
      codeProfile.sample(dinst->getPC(), dinst->getID(), 0, dinst->isBiasBranch() ? 1.0 : 0, 0, dinst->isBranchMiss(), dinst->isPrefetch(), dinst->getLBType(), dinst->isBranchMiss_level1(), dinst->isBranchMiss_level2(), dinst->isBranchMiss_level3(), dinst->isBranchHit_level1(), dinst->isBranchHit_level2(), dinst->isBranchHit_level3(), dinst->isBranch_hit2_miss3(), dinst->isBranch_hit3_miss2());
      AddrType p = dinst->getPC();
      //MSG("BR_PROFILE clk=%u brpc=%llx br_miss=%d\n", globalClock, dinst->getPC(), dinst->isBranchMiss());
    }
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
