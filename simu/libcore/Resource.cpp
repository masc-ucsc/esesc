//  Contributed by Jose Renau
//                 Basilio Fraguela
//                 James Tuck
//                 Smruti Sarangi
//                 Luis Ceze
//                 Karin Strauss
//                 David Munday
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

#include <limits.h>

//#define MEM_TSO 1
//#define MEM_TSO2 1
#include "Resource.h"

#include "Cluster.h"
#include "DInst.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "GProcessor.h"
#include "LSQ.h"
#include "MemRequest.h"
#include "OoOProcessor.h"
#include "Port.h"
#include "TaskHandler.h"

// late allocation flag
#define USE_PNR
#define LSQ_LATE_EXECUTED 1

/* }}} */

Resource::Resource(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, uint32_t cpuid)
    /* constructor {{{1 */
    : cluster(cls)
    , gen(aGen)
    , avgRenameTime("P(%d)_%s_%d_avgRenameTime", cpuid, cls->getName(), type)
    , avgIssueTime("P(%d)_%s_%d_avgIssueTime", cpuid, cls->getName(), type)
    , avgExecuteTime("P(%d)_%s_%d_avgExecuteTime", cpuid, cls->getName(), type)
    , avgRetireTime("P(%d)_%s_%d_avgRetireTime", cpuid, cls->getName(), type)
    , lat(l)
    , usedTime(0)
    , coreid(cpuid) {
  I(cls);
  if(gen)
    gen->subscribe();

  const char *str = SescConf->getCharPtr("cpusimu", "type", cpuid);
  if(strcasecmp(str, "inorder") == 0)
    inorder = true;
  else
    inorder = false;
}
/* }}} */

void Resource::setStats(const DInst *dinst) {

  if(!dinst->getStatsFlag())
    return;

  Time_t t;

  t = globalClock - dinst->getExecutedTime();
  avgRetireTime.sample(t, true);

  t = dinst->getExecutedTime() - dinst->getIssuedTime();
  avgExecuteTime.sample(t, true);

  t = dinst->getIssuedTime() - dinst->getRenamedTime();
  avgIssueTime.sample(t, true);

  t = dinst->getRenamedTime() - dinst->getFetchTime();
  avgRenameTime.sample(t, true);
}

Resource::~Resource()
/* destructor {{{1 */
{
  GMSG(!EventScheduler::empty(), "Resources destroyed with %zu pending instructions", EventScheduler::size());

  if(gen)
    gen->unsubscribe();
}
/* }}} */

/***********************************************/

MemResource::MemResource(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *_lsq, StoreSet *ss, Prefetcher *_pref, TimeDelta_t l,
                         GMemorySystem *ms, int32_t id, const char *cad)
    /* constructor {{{1 */
    : MemReplay(type, cls, aGen, ss, l, id)
    , firstLevelMemObj(ms->getDL1())
    , memorySystem(ms)
    , lsq(_lsq)
    , pref(_pref)
    , stldViolations("P(%d)_%s_%s:stldViolations", id, cls->getName(), cad) {
  if(strcasecmp(firstLevelMemObj->getDeviceType(), "cache") == 0) {
    DL1 = firstLevelMemObj;
  } else {
    MRouter *router = firstLevelMemObj->getRouter();
    DL1             = router->getDownNode();
    if(strcasecmp(DL1->getDeviceType(), "cache") != 0) {
      MRouter *router = DL1->getRouter();
      DL1             = router->getDownNode();
      if(strcasecmp(DL1->getDeviceType(), "cache") != 0) {
        printf("ERROR: Neither first or second or 3rd level is a cache %s\n", DL1->getDeviceType());
        exit(-1);
      }
    }
  }
}
/* }}} */

MemReplay::MemReplay(uint8_t type, Cluster *cls, PortGeneric *gen, StoreSet *ss, TimeDelta_t l, uint32_t cpuid)
    : Resource(type, cls, gen, l, cpuid)
    , lfSize(8)
    , storeset(ss) {
  lf = (struct FailType *)malloc(sizeof(struct FailType) * lfSize);
  for(uint32_t i = 0; i < lfSize; i++) {
    lf[i].ssid = -1;
  }
}

void MemReplay::replayManage(DInst *dinst) {

  if(dinst->getSSID() == -1)
    return;

  int  pos     = -1;
  int  pos2    = 0;
  bool updated = false;

  SSID_t did = dinst->getSSID();

  for(uint32_t i = 0; i < lfSize; i++) {
    if(lf[pos2].id < lf[i].id && lf[i].ssid != did)
      pos2 = i;

    if(lf[i].id >= dinst->getID())
      continue;
    if(lf[i].ssid == -1) {
      if(!updated) {
        lf[i].ssid = did;
        lf[i].id   = dinst->getID();
        lf[i].pc   = dinst->getPC();
        lf[i].addr = dinst->getAddr();
        lf[i].op   = dinst->getInst()->getOpcode();
        updated    = true;
      }
      continue;
    }
    if((dinst->getID() - lf[i].id) > 500) {
      if(!updated) {
        lf[i].ssid = did;
        lf[i].id   = dinst->getID();
        lf[i].pc   = dinst->getPC();
        lf[i].addr = dinst->getAddr();
        lf[i].op   = dinst->getInst()->getOpcode();
        updated    = true;
      }
      continue;
    }
    if(lf[i].ssid == did) {
      continue;
    }
    if(lf[i].addr != dinst->getAddr()) {
      if(pos == -1)
        pos = i;
      continue;
    }

    if(pos != -3) {
      pos = -2;
#if 0
      printf("1.merging %d and %d : pc %x and %x : addr %x and %x : %dx%d : data %x and %x : id %d and %d (%d) : %x\n"
             ,lf[i].ssid, did, lf[i].pc, dinst->getPC(), lf[i].addr, dinst->getAddr()
             ,lf[i].op, dinst->getInst()->getOpcode()
             ,lf[i].data, dinst->getData(), lf[i].id, dinst->getID(), dinst->getID() - lf[i].id, dinst->getConflictStorePC());
#endif

      SSID_t newid = storeset->mergeset(lf[i].ssid, did);
      did          = newid;
      lf[i].ssid   = newid;
      lf[i].id     = dinst->getID();
      lf[i].pc     = dinst->getPC();
      lf[i].addr   = dinst->getAddr();
      lf[i].op     = dinst->getInst()->getOpcode();
      updated      = true;
    }
  }

  static int rand = 0;
  rand++;
#if 1
  if(pos >= 0 && (rand & 7) == 0) {
    int i = pos;
#if 0
      printf("2.merging %d and %d : pc %x and %x : addr %x and %x : %dx%d : data %x and %x : id %d and %d (%d) : %x\n"
          ,lf[i].ssid, dinst->getSSID(), lf[i].pc, dinst->getPC(), lf[i].addr, dinst->getAddr()
          , lf[i].op, dinst->getInst()->getOpcode()
          , lf[i].data, dinst->getData(), lf[i].id, dinst->getID(), dinst->getID() - lf[i].id, dinst->getConflictStorePC());
#endif
    SSID_t newid = storeset->mergeset(lf[i].ssid, dinst->getSSID());
    lf[i].ssid   = newid;
    lf[i].id     = dinst->getID();
    lf[i].pc     = dinst->getPC();
    lf[i].addr   = dinst->getAddr();
    updated      = true;
  }
#endif

  if(!updated) {
    int i = pos2;
    if(dinst->getID() > lf[i].id && dinst->getSSID() != lf[i].ssid) {
#if DEBUG
      printf("3.merging %d and %d : pc %lx and %lx : addr %lx and %lx : id %lu and %lu (%lu)\n", lf[i].ssid, dinst->getSSID(),
             lf[i].pc, dinst->getPC(), lf[i].addr, dinst->getAddr(), lf[i].id, dinst->getID(), dinst->getID() - lf[i].id);
      storeset->mergeset(lf[i].ssid, dinst->getSSID());
      if(lf[i].ssid > dinst->getSSID())
        lf[i].ssid = dinst->getSSID();
#endif
      lf[i].ssid = dinst->getSSID();
      lf[i].id   = dinst->getID();
      lf[i].pc   = dinst->getPC();
      lf[i].addr = dinst->getAddr();
    }
  }
}

/***********************************************/

FULoad::FULoad(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *_lsq, StoreSet *ss, Prefetcher *_pref, TimeDelta_t lsdelay,
               TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad)
    /* Constructor {{{1 */
    : MemResource(type, cls, aGen, _lsq, ss, _pref, l, ms, id, cad)
#ifdef MEM_TSO2
    , tso2Replay("P(%d)_%s_%s:tso2Replay", id, cls->getName(), cad)
#endif
    , LSDelay(lsdelay)
    , freeEntries(size) {
  char cadena[1000];
  sprintf(cadena, "P(%d)_%s_%s", id, cls->getName(), cad);
  enableDcache = SescConf->getBool("cpusimu", "enableDcache", id);
  I(ms);
}
/* }}} */

StallCause FULoad::canIssue(DInst *dinst) {
  /* canIssue {{{1 */

  if(freeEntries <= 0) {
#ifndef LSQ_LATE_EXECUTED
    I(freeEntries == 0); // Can't be negative
#endif
    return OutsLoadsStall;
  }
  if(!lsq->hasFreeEntries())
    return OutsLoadsStall;

  if(inorder)
    if(lsq->hasPendingResolution())
      return OutsLoadsStall;

  if(!lsq->insert(dinst))
    return OutsLoadsStall;

  storeset->insert(dinst);
  // call vtage->rename() here????

  lsq->decFreeEntries();

  if(!LSQlateAlloc)
    freeEntries--;
  return NoStall;
}
/* }}} */

void FULoad::executing(DInst *dinst) {
  /* executing {{{1 */

#ifndef LSQ_LATE_EXECUTED
  if(LSQlateAlloc)
    freeEntries--;
#endif

  cluster->executing(dinst);
  Time_t when = gen->nextSlot(dinst->getStatsFlag()) + lat;

  DInst *qdinst = lsq->executing(dinst);
  I(qdinst == 0);
  if(qdinst) {
    I(qdinst->getInst()->isStore());
    dinst->getGProc()->replay(dinst);
    if(!dinst->getGProc()->isFlushing())
      stldViolations.inc(dinst->getStatsFlag());

    storeset->stldViolation(qdinst, dinst);
  }

  // The check in the LD Queue is performed always, for hit & miss
  if(dinst->isLoadForwarded() || !enableDcache) {
    if(enableDcache) {
      MemRequest::sendReqRead(firstLevelMemObj, dinst->getStatsFlag(), dinst->getAddr(), dinst->getPC()); // Just to use cache ports
    }
    pref->exe(dinst);
    performedCB::scheduleAbs(when + LSDelay, this, dinst);
    dinst->markDispatched();
  } else {
    cacheDispatchedCB::scheduleAbs(when, this, dinst);
  }
}
/* }}} */

void FULoad::cacheDispatched(DInst *dinst) {
  /* cacheDispatched {{{1 */

  I(enableDcache);
  // I(!dinst->isLoadForwarded());
  //
  dinst->markDispatched();

  // MSG("FULoad %lld 0x%x 0x%x @%lld",dinst->getID(), dinst->getAddr(), dinst->getPC(), globalClock);
  MemRequest::sendReqDL1Read(firstLevelMemObj, dinst->getStatsFlag(), dinst->getAddr(), dinst->getPC(), dinst,
                             performedCB::create(this, dinst));

  pref->exe(dinst);
}
/* }}} */

void FULoad::executed(DInst *dinst) {
  /* executed {{{1 */

#ifdef LSQ_LATE_EXECUTED
  if(LSQlateAlloc)
    freeEntries--;
#endif

  if(dinst->getChained()) {
    I(dinst->getFetchEngine());
    dinst->getFetchEngine()->chainLoadDone(dinst);
  } else {
    I(!dinst->getFetchEngine());
  }

  storeset->remove(dinst);

  /*************************/
  // HERE: Do a call to mark the LD as predicted (index the and set if dataRATptr is to dinst then
  // dataRAT[dinst->getinst()->getDst1()] = dinst->getData();)
  //         dataRATptr[...] = 0 if dataRATotr[...] == dinst

  cluster->executed(dinst);
}
/* }}} */

bool FULoad::preretire(DInst *dinst, bool flushing)
/* retire {{{1 */
{
  bool done = dinst->isDispatched();
  if(!done)
    return false;

#ifdef USE_PNR
  freeEntries++;
#endif

  pref->ret(dinst);

  return true;
}
/* }}} */

bool FULoad::retire(DInst *dinst, bool flushing)
/* retire {{{1 */
{
  if(!dinst->isPerformed())
    return false;

  lsq->incFreeEntries();
#ifndef USE_PNR
  freeEntries++;
#endif

#ifdef MEM_TSO2
  if(DL1->Invalid(dinst->getAddr())) {
    // MSG("Sync head/tail @%lld",globalClock);
    tso2Replay.inc(dinst->getStatsFlag());
    dinst->getGProc()->replay(dinst);
#if 0
    EmulInterface *eint = TaskHandler::getEmul(coreid);
    eint->syncHeadTail( coreid );
#endif
  }
#endif

  lsq->remove(dinst);
  // VTAGE->updateVtageTables() here ???? vtage validation

#if 0
  // Merging for tradcore
  if(dinst->isReplay() && !flushing)
    replayManage(dinst);
#endif

  setStats(dinst);

  return true;
}
/* }}} */

void FULoad::performed(DInst *dinst) {
  /* memory operation was globally performed {{{1 */

  // MSG("DONE   %lld 0x%x 0x%x @%lld",dinst->getID(), dinst->getAddr(), dinst->getPC(), globalClock);
  dinst->markPerformed();

  // For LDs, a perform means finish the executed
  // executedCB::schedule(0);
  executed(dinst);
}
/* }}} */

/***********************************************/

FUStore::FUStore(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *_lsq, StoreSet *ss, Prefetcher *_pref, TimeDelta_t l,
                 GMemorySystem *ms, int32_t size, int32_t id, const char *cad)
    /* constructor {{{1 */
    : MemResource(type, cls, aGen, _lsq, ss, _pref, l, ms, id, cad)
    , freeEntries(size) {

  enableDcache = SescConf->getBool("cpusimu", "enableDcache", id);
  scbSize      = SescConf->getInt("cpusimu", "scbSize", id);
  scbEntries   = scbSize;
  if(SescConf->checkBool("cpusimu", "LSQlateAlloc", id))
    LSQlateAlloc = SescConf->getBool("cpusimu", "LSQlateAlloc", id);
  else
    LSQlateAlloc = false;

  const char *dl1_section = DL1->getSection();
  int         bsize       = SescConf->getInt(dl1_section, "bsize");
  lineSizeBits            = log2i(bsize);
}
/* }}} */

StallCause FUStore::canIssue(DInst *dinst) {
  /* canIssue {{{1 */

  if(dinst->getInst()->isStoreAddress())
    return NoStall;

  if(freeEntries <= 0) {
    I(freeEntries == 0); // Can't be negative
    return OutsStoresStall;
  }
  if(!lsq->hasFreeEntries())
    return OutsStoresStall;

  if(inorder)
    if(lsq->hasPendingResolution())
      return OutsStoresStall;

  if(!lsq->insert(dinst))
    return OutsStoresStall;

  storeset->insert(dinst);

  lsq->decFreeEntries();
  freeEntries--;

#if 0
    //update load data buffer if there is any older store for the same address
    if(dinst->getInst()->getOpcode() == iSALU_ST) {
      AddrType st_addr = dinst->getAddr();
      for(int i = 0; i < DL1->getLdBuffSize(); i++) {
        AddrType saddr = DL1->load_data_buffer[i].start_addr;
        AddrType eaddr = DL1->load_data_buffer[i].end_addr;
        int64_t del    = DL1->load_data_buffer[i].delta;
        if(st_addr >= saddr && st_addr <= eaddr) {
          int idx        = 0;
          if(del != 0) {
            idx = abs((int)(st_addr - saddr) / del);
          }
          if(st_addr == (idx * del + saddr)) { //check if st_addr exactly matches this entry in buff
            DL1->load_data_buffer[i].req_data[idx] = dinst->getData2();
            DL1->load_data_buffer[i].valid[idx] = true;
            DL1->load_data_buffer[i].marked[idx] = true;
          }
        }
        AddrType saddr2 = DL1->load_data_buffer[i].start_addr2;
        AddrType eaddr2 = DL1->load_data_buffer[i].end_addr2;
        int64_t del2   = DL1->load_data_buffer[i].delta2;
        if(st_addr >= saddr2 && st_addr <= eaddr2) {
          int idx2        = 0;
          if(del2 != 0) {
            idx2 = abs((int)(st_addr - saddr2) / del2);
          }
          if(st_addr == (idx2 * del2 + saddr2)) { //check if st_addr exactly matches this entry in buff
            DL1->load_data_buffer[i].req_data2[idx2] = dinst->getData2();
            DL1->load_data_buffer[i].valid2[idx2] = true;
            DL1->load_data_buffer[i].marked2[idx2] = true;
          }
        }
      }
    }
#endif
  return NoStall;
}
/* }}} */

void FUStore::executing(DInst *dinst) {
  /* executing {{{1 */
  if(!dinst->getInst()->isStoreAddress()) {
    DInst *qdinst = lsq->executing(dinst);
    if(qdinst) {
      dinst->getGProc()->replay(qdinst);
      if(!dinst->getGProc()->isFlushing())
        stldViolations.inc(dinst->getStatsFlag());
      storeset->stldViolation(qdinst, dinst);
    }
  }

  cluster->executing(dinst);
  gen->nextSlot(dinst->getStatsFlag());

  if(dinst->getInst()->isStoreAddress()) {
#if 0
    if (enableDcache && !firstLevelMemObj->isBusy(dinst->getAddr()) ){
      MemRequest::sendReqWritePrefetch(firstLevelMemObj, dinst->getStatsFlag(), dinst->getAddr(), 0); // executedCB::create(this,dinst));
    }
    executed(dinst);
#else
    executed(dinst);
#endif
  } else {
    executed(dinst);
  }
}
/* }}} */

void FUStore::executed(DInst *dinst) {
  /* executed {{{1 */

  if(dinst->getInst()->isStore())
    storeset->remove(dinst);

  cluster->executed(dinst);
  // We have data&address, the LSQ can be populated
}
/* }}} */

bool FUStore::preretire(DInst *dinst, bool flushing) {
  /* retire {{{1 */
  if(!dinst->isExecuted())
    return false;

  if(dinst->getInst()->isStoreAddress())
    return true;

  if(flushing) {
    performed(dinst);
    return true;
  }

  if(scbEntries <= 0)
    return false;

#if 0
  if(firstLevelMemObj->isBusy(dinst->getAddr())) {
    return false;
  }
#endif

  freeEntries++;
  scbEntries--;
  scbQueue.push_back(dinst);

  // FIXME: scbmerge is wrong. We should create a memory block for it before the L1 cache
  bool pendingLine = false; // scbMerge[(dinst->getAddr()>>lineSizeBits) & 1023]>0;
  scbMerge[(dinst->getAddr() >> lineSizeBits) & 1023]++;

  if(enableDcache && !pendingLine) {
    MemRequest::sendReqWrite(firstLevelMemObj, dinst->getStatsFlag(), dinst->getAddr(), dinst->getPC(),
                             performedCB::create(this, dinst));
  } else {
    // Merge request if pending
    performed(dinst);
  }

  return true;
}
/* }}} */

void FUStore::performed(DInst *dinst) {
  /* memory operation was globally performed {{{1 */

  setStats(dinst); // Not retire for stores

  I(!dinst->isPerformed());
  if(dinst->isRetired()) {
    dinst->recycle();
  }
  dinst->markPerformed();

  scbMerge[(dinst->getAddr() >> lineSizeBits) & 1023]--;

#ifndef MEM_TSO
  // RC
  scbEntries++;
#endif
  if(scbQueue.front() == dinst) {
#ifdef MEM_TSO
    scbEntries++;
#endif
    scbQueue.pop_front();
    return;
  }

  int allzeroes = 0;

  for(SCBQueueType::iterator it = scbQueue.begin(); it != scbQueue.end(); it++) {
    if((*it == 0 || *it == dinst) && allzeroes >= 0)
      allzeroes++;
    else
      allzeroes = -1;

    if((*it) == dinst) {
      if(allzeroes > 0) {
#ifdef MEM_TSO
        scbEntries += allzeroes;
#endif
        it++;
        scbQueue.erase(scbQueue.begin(), it);
      } else {
        *it = 0;
      }
      return;
    }
  }
}
/* }}} */

bool FUStore::retire(DInst *dinst, bool flushing) {
  /* retire {{{1 */
  if(dinst->getInst()->isStoreAddress()) {
    setStats(dinst);
    return true;
  }

  // if(!dinst->isPerformed())
  //  return false;

  I(!dinst->isRetired());
  dinst->markRetired();

  dinst->clearRATEntry(); // Stores could have dependences for LL and atomics. Clear at retire

  lsq->remove(dinst);
  lsq->incFreeEntries();
  // freeEntries++;

  return true;
}
/* }}} */

/***********************************************/

FUGeneric::FUGeneric(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, uint32_t cpuid)
    /* constructor {{{1 */
    : Resource(type, cls, aGen, l, cpuid) {
}
/* }}} */

StallCause FUGeneric::canIssue(DInst *dinst) {
  /* canIssue {{{1 */
#if 0
  if (inorder) {
    Time_t t = gen->calcNextSlot();
    if (t>globalClock)
      return DivergeStall;
  }
#endif
  return NoStall;
}
/* }}} */

void FUGeneric::executing(DInst *dinst) {
  /* executing {{{1 */
  Time_t nlat = gen->nextSlot(dinst->getStatsFlag()) + lat;
#if 0
  if (dinst->getPC() == 1073741832) {
    MSG("@%lld Scheduling callback for FID[%d] PE[%d] Warp [%d] pc 1073741832 at @%lld"
        , (long long int)globalClock
        , dinst->getFlowId()
        , dinst->getPE()
        , dinst->getWarpID()
        , (long long int) nlat);
  }
#endif
  cluster->executing(dinst);
  executedCB::scheduleAbs(nlat, this, dinst);
}
/* }}} */

void FUGeneric::executed(DInst *dinst) {
  /* executed {{{1 */
  cluster->executed(dinst);
  dinst->markPerformed();
}
/* }}} */

bool FUGeneric::preretire(DInst *dinst, bool flushing)
/* preretire {{{1 */
{
  return dinst->isExecuted();
}
/* }}} */

bool FUGeneric::retire(DInst *dinst, bool flushing)
/* retire {{{1 */
{
  setStats(dinst);

  return true;
}
/* }}} */

void FUGeneric::performed(DInst *dinst) {
  /* memory operation was globally performed {{{1 */
  dinst->markPerformed();
  I(0); // It should be called only for memory operations
}
/* }}} */

/***********************************************/

FUBranch::FUBranch(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, uint32_t cpuid, int32_t mb, bool dom)
    /* constructor {{{1 */
    : Resource(type, cls, aGen, l, cpuid)
    , freeBranches(mb)
    , drainOnMiss(dom) {
  I(freeBranches > 0);
}
/* }}} */

StallCause FUBranch::canIssue(DInst *dinst) {
  /* canIssue {{{1 */

  I(dinst->getInst()->isControl());
  if(freeBranches == 0)
    return OutsBranchesStall;

  freeBranches--;

  return NoStall;
}
/* }}} */

void FUBranch::executing(DInst *dinst) {
  /* executing {{{1 */
  cluster->executing(dinst);
  executedCB::scheduleAbs(gen->nextSlot(dinst->getStatsFlag()) + lat, this, dinst);
}
/* }}} */

void FUBranch::executed(DInst *dinst) {
  /* executed {{{1 */
  cluster->executed(dinst);
  dinst->markPerformed();

  if(!drainOnMiss && dinst->isBranchMiss()) {
    (dinst->getFetchEngine())->unBlockFetch(dinst, dinst->getFetchTime());
  }

  // NOTE: assuming that once the branch is executed the entry can be recycled
  freeBranches++;
}
/* }}} */

bool FUBranch::preretire(DInst *dinst, bool flushing)
/* preretire {{{1 */
{
  if(drainOnMiss && dinst->isExecuted() && dinst->isBranchMiss()) {
    (dinst->getFetchEngine())->unBlockFetch(dinst, dinst->getFetchTime());
  }
  return dinst->isExecuted();
}
/* }}} */

bool FUBranch::retire(DInst *dinst, bool flushing)
/* retire {{{1 */
{
  setStats(dinst);
  return true;
}
/* }}} */

void FUBranch::performed(DInst *dinst) {
  /* memory operation was globally performed {{{1 */
  dinst->markPerformed();
  I(0); // It should be called only for memory operations
}
/* }}} */

/***********************************************/

FURALU::FURALU(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, int32_t id)
    /* constructor {{{1 */
    : Resource(type, cls, aGen, l, id)
    , dmemoryBarrier("P(%d)_%s_dmemoryBarrier", id, cls->getName())
    , imemoryBarrier("P(%d)_%s_imemoryBarrier", id, cls->getName()) {
  blockUntil = 0;
}
/* }}} */

StallCause FURALU::canIssue(DInst *dinst)
/* canIssue {{{1 */
{
  I(dinst->getPC() != 0xf00df00d); // It used to be a Syspend, but not longer true

  if(dinst->getPC() == 0xdeaddead) {
    // This is the PC for a syscall (QEMUReader::syscall)
    if(blockUntil == 0) {
      // LOG("syscall %d executed, with %d delay", dinst->getAddr(), dinst->getData());
      // blockUntil = globalClock+dinst->getData();
      blockUntil = globalClock + 100;
      return SyscallStall;
    }

    // is this where we poweron the GPU threads and then poweroff the QEMU thread?
    if(globalClock >= blockUntil) {
      blockUntil = 0;
      return NoStall;
    }

    return SyscallStall;
  } else if(!dinst->getInst()->hasDstRegister() && !dinst->getInst()->hasSrc1Register() && !dinst->getInst()->hasSrc2Register()) {
    if(dinst->getGProc()->isROBEmpty())
      return NoStall;
    if(dinst->getAddr() == 0xbeefbeef)
      imemoryBarrier.inc(dinst->getStatsFlag());
    else
      dmemoryBarrier.inc(dinst->getStatsFlag());
    // FIXME return SyscallStall;
  }

  return NoStall;
}
/* }}} */

void FURALU::executing(DInst *dinst)
/* executing {{{1 */
{
  cluster->executing(dinst);
  executedCB::scheduleAbs(gen->nextSlot(dinst->getStatsFlag()) + lat, this, dinst);

  // Recommended poweron the GPU threads and then poweroff the QEMU thread?
}
/* }}} */

void FURALU::executed(DInst *dinst)
/* executed {{{1 */
{
  cluster->executed(dinst);
  dinst->markPerformed();
}
/* }}} */

bool FURALU::preretire(DInst *dinst, bool flushing)
/* preretire {{{1 */
{
  return dinst->isExecuted();
}
/* }}} */

bool FURALU::retire(DInst *dinst, bool flushing)
/* retire {{{1 */
{
  setStats(dinst);
  return true;
}
/* }}} */

void FURALU::performed(DInst *dinst)
/* memory operation was globally performed {{{1 */
{
  dinst->markPerformed();
  I(0); // It should be called only for memory operations
}
/* }}} */
