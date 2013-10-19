/* License & includes {{{1 */
/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Luis Ceze
  Jose Renau
  Karin Strauss
  Milos Prvulovic

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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <iostream>

#include "nanassert.h"

#include "SescConf.h"
#include "MemorySystem.h"
#include "Cache.h"
#include "MSHR.h"
#include "GProcessor.h"
#include "TaskHandler.h"
/* }}} */
#define alamelu_dbg 0
Cache::Cache(MemorySystem *gms, const char *section, const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
  ,hitDelay  (SescConf->getInt(section,"hitDelay"))
  ,invDelay  (SescConf->getInt(section,"invDelay"))
  ,missDelay (SescConf->getInt(section,"missDelay"))
  ,ms        (gms)
    // read stats
  ,readHit         ("%s:readHit",         name)
  ,readMiss        ("%s:readMiss",        name)
  ,readHalfMiss    ("%s:readHalfMiss",    name)
    // write stats
  ,writeHit        ("%s:writeHit",        name)
  ,writeMiss       ("%s:writeMiss",       name)
  ,writeHalfMiss   ("%s:writeHalfMiss",   name)
  ,displaced       ("%s:displaced",       name)
  ,writeBack       ("%s:writeBack",       name)
  ,writeExclusive  ("%s:writeExclusive",  name)
    // other stats
  ,lineFill        ("%s:lineFill",        name)
  ,avgMissLat      ("%s_avgMissLat",      name)
  ,avgMemLat       ("%s_avgMemLat",       name)
  ,capInvalidateHit   ("%s_capInvalidateHit",   name)
  ,capInvalidateMiss  ("%s_capInvalidateMiss",  name)
  ,invalidateHit   ("%s_invalidateHit",   name)
  ,invalidateMiss  ("%s_invalidateMiss",  name)
{

  const char* mshrSection = SescConf->getCharPtr(section,"MSHR");

  char tmpName[512];

  sprintf(tmpName, "%s", name);

  cacheBank = CacheType::create(section, "", tmpName);
  lineSize  = cacheBank->getLineSize();
  mshr      = MSHR::create(tmpName, mshrSection, lineSize);

  I(getLineSize() < 4096); // To avoid bank selection conflict (insane cache line)
  I(gms);

  // BANK
  int32_t numBanks = 1;

  if  (SescConf->checkInt(section, "nBanks")){
    numBanks = SescConf->getInt(section, "nBanks");
    I(numBanks >= 1);
  }

  log2nBanks = log2i(numBanks); 
  
  SescConf->checkBool(section, "llc");
  llc = SescConf->getBool(section, "llc");

  inclusive = true; // true by default
  if (SescConf->checkBool(section, "inclusive"))
    inclusive = SescConf->getBool(section, "inclusive");
  
  int numPorts = SescConf->getInt(section, "bkNumPorts");

  int portOccp = SescConf->getInt(section, "bkPortOccp");

  bkPort = new PortGeneric* [numBanks]; 
  for (int i = 0; i < numBanks; i++){
    sprintf(tmpName, "%s_bk(%d)", name,i);
    bkPort[i] = PortGeneric::create(tmpName, numPorts, portOccp);
  }

  // READ 
  numPorts = SescConf->getInt(section, "rdNumPorts");
  portOccp = SescConf->getInt(section, "rdPortOccp");
  sprintf(tmpName, "%s_rd", name);
  rdPort = PortGeneric::create(tmpName, numPorts, portOccp);

  // WRITE 
  numPorts = SescConf->getInt(section, "wrNumPorts");
  portOccp = SescConf->getInt(section, "wrPortOccp");
  sprintf(tmpName, "%s_wr", name);
  wrPort = PortGeneric::create(tmpName, numPorts, portOccp);

  // Low-Level 
  numPorts = SescConf->getInt(section, "llNumPorts");
  portOccp = SescConf->getInt(section, "llPortOccp");
  sprintf(tmpName, "%s_ll", name);
  ctPort = PortGeneric::create(tmpName, numPorts, portOccp);

  SescConf->isInt(section, "hitDelay");
  SescConf->isInt(section, "missDelay");

  maxPendingWrites = SescConf->getInt(section, "maxWrites");
  if(maxPendingWrites == 0)
    maxPendingWrites = 32768;
  maxPendingReads = SescConf->getInt(section, "maxReads");
  if(maxPendingReads == 0)
    maxPendingReads = 32768;

  pendingReads = 0;
  pendingWrites = 0;

  MemObj *lower_level = gms->declareMemoryObj(section, "lowerLevel");
  if(lower_level)
    addLowerLevel(lower_level);

}
/* }}} */

Cache::~Cache()
/* destructor {{{1 */
{
  cacheBank->destroy();
}
/* }}} */

void Cache::displaceLine(AddrType addr, MemRequest *mreq, bool modified)
/* displace a line from the cache. writeback if necessary {{{1 */
{
  I(addr != mreq->getAddr()); // addr is the displace address, mreq is the trigger
  // broadcast invalidate to all the upper nodes (there may be none like in the L1 cache)
  //
  if (inclusive)
    router->sendInvalidateAll(getLineSize(), mreq, addr, 1);

  displaced.inc(mreq->getStatsFlag());

  if (modified) {
    doWriteback(addr);
  }
}
/* }}} */

Cache::Line *Cache::allocateLine(AddrType addr, MemRequest *mreq)
/* find a new cacheline for addr (it can return 0) {{{1 */
{
  AddrType rpl_addr=0;

  I(cacheBank->findLineDebug(addr) == 0);
  Line *l = cacheBank->fillLine(addr, rpl_addr);
  lineFill.inc(mreq->getStatsFlag());

  I(l); // Ignore lock guarantees to find line

  if(l->isValid())
    displaceLine(rpl_addr, mreq, l->isModified());

  return l;
}
/* }}} */

void Cache::doWriteback(AddrType addr)
/* write back a line {{{1 */
{
  MemRequest *wbreq = MemRequest::createWriteback(this, addr);
  writeBack.inc();

  router->fwdPushDown(wbreq, hitDelay);
}
/* }}} */

Time_t Cache::nextReadSlot(const MemRequest *mreq)  
/* get next free slot {{{1 */
{ 
  return max(rdPort->nextSlot(),nextBankSlot((void*)mreq)); 
}
/* }}} */
Time_t Cache::nextWriteSlot(const MemRequest *mreq) 
/* get next free slot {{{1 */
{ 
  return max(wrPort->nextSlot(),nextBankSlot((void*)mreq)); 
}
/* }}} */
Time_t Cache::nextBusReadSlot(const MemRequest *mreq)
/* get next free slot {{{1 */
{
  return nextReadSlot((void*)mreq);
}
/* }}}*/
Time_t Cache::nextPushDownSlot(const MemRequest *mreq)
/* get next free slot {{{1 */
{
  return nextCtrlSlot((void*)mreq);
}
/* }}}*/
Time_t Cache::nextPushUpSlot(const MemRequest *mreq)
/* get next free slot {{{1 */
{
  return nextCtrlSlot((void*)mreq);
}
/* }}}*/
Time_t Cache::nextInvalidateSlot(const MemRequest *mreq)
/* get next free slot {{{1 */
{
  return nextCtrlSlot((void*)mreq);
}
/* }}}*/

void Cache::doRead(MemRequest *mreq, bool retrying)
/* processor issued read {{{1 */
{
  AddrType addr = mreq->getAddr();

  if (retrying) { // reissued operation
    pendingReads--;
  }

  Line *l = cacheBank->readLine(addr);
  if (l == 0) {
    pendingReads++;

    if(!retrying && !mshr->canIssue(addr)) {
      readHalfMiss.inc(mreq->getStatsFlag());
      CallbackBase *cb  = doReadCB::create(this, mreq, true);
      mshr->addEntry(addr, cb);
      return;
    }

    // inclusion guarantees that nobody higher than us has the data
    readMiss.inc(mreq->getStatsFlag());
    mshr->addEntry(addr);
    router->fwdBusRead(mreq, missDelay);
    return;
  }

  readHit.inc(mreq->getStatsFlag());

  I(mreq->isHomeNode());

  if (retrying) {
    mshr->retire(addr);
  }

  avgMemLat.sample(mreq->getTimeDelay()+getHitDelay(mreq));
  mreq->ack(getHitDelay(mreq));
}
/* }}} */

void Cache::doWrite(MemRequest *mreq, bool retrying)
/* processor issued write {{{1 */
{
  AddrType addr = mreq->getAddr();

  if (retrying) { // reissued operation
    pendingWrites--;
  }

  Line *l = cacheBank->readLine(addr);
  if (l == 0) {

    pendingWrites++;
    if(!retrying && !mshr->canIssue(addr)) {

      writeHalfMiss.inc(mreq->getStatsFlag());

      CallbackBase *cb  = doWriteCB::create(this, mreq, true);
      mshr->addEntry(addr, cb);
      return;
    }

    // inclusion guarantees that nobody higher than us has the data
    writeMiss.inc(mreq->getStatsFlag());

    mshr->addEntry(addr);
    router->fwdBusRead(mreq, missDelay);
    return;
  }

  I(l->isValid());

  TimeDelta_t lat=getHitDelay(mreq);
  
  if (l->isShared()) {
    // Maker sure that the upper levels do not have a copy of the data
    
    l->invalidate();
    
    writeExclusive.inc(mreq->getStatsFlag());
    
    if (!llc) {
      mshr->addEntry(addr);
      router->fwdBusRead(mreq, lat);
      return;
    }
    lat += invDelay;
  }
  writeHit.inc(mreq->getStatsFlag());

  l->setModified();

  if (retrying) {
    mshr->retire(addr);
  }
  
  avgMemLat.sample(mreq->getTimeDelay()+lat);
  mreq->ack(lat);
}
/* }}} */

void Cache::doBusRead(MemRequest *mreq, bool retrying)
/* cache busRead (exclusive or shared) request {{{1 */
{
  //static int load_count = 0;
  //static int modder = 100;
  //static int hit  = 0;
  //static int miss = 0;

#if 0
  if (!retrying)
    MSG("1. %s: pe %d addr 0x%llX @%lld %p %p\n",this->getName(),mreq->getDInst()->getPE(),mreq->getDInst()->getAddr(),globalClock,mreq,mshr);
  else
    MSG("2. %s: pe %d addr 0x%llX @%lld\n",this->getName(),mreq->getDInst()->getPE(),mreq->getDInst()->getAddr(),globalClock);
#endif


  AddrType addr = mreq->getAddr();
  // if(!strcmp(this->getName(), "L2(0)")){
  //   load_count++;
  //   if(!(load_count%modder))
  //     printf("ESESC %s: callback %d for pc 0x%X\n",this->getName(),load_count,mreq->getDInst()->getPC());
  // }
 
  // FIXME to have better contention management. If the MSHR can not accept a new request, we should schedule a timeout for something later.

  Line *l = cacheBank->readLine(addr);
  if (l == 0) {
    if(!retrying && !mshr->canIssue(addr)) {
      if (mreq->isRead()) {
        readHalfMiss.inc(mreq->getStatsFlag());
      }else{
        writeHalfMiss.inc(mreq->getStatsFlag());
      }
   
      CallbackBase *cb  = doBusReadCB::create(this, mreq, true);
      mshr->addEntry(addr, cb);
      return;
    }

    if (mreq->isRead()) {
      readMiss.inc(mreq->getStatsFlag());
    }else{
      writeMiss.inc(mreq->getStatsFlag());
    }

    mshr->addEntry(addr);
    router->fwdBusRead(mreq, missDelay); // cache miss
    return;
  } 

  if (mreq->isRead()) {
    readHit.inc(mreq->getStatsFlag());
    avgMemLat.sample(mreq->getTimeDelay()+getHitDelay(mreq));
    router->fwdPushUp(mreq, getHitDelay(mreq)); // ack the busRead
    if (retrying) {
      mshr->retire(addr);
    }
    return;
  }else{ // write
    writeHit.inc(mreq->getStatsFlag());
  }

  I(mreq->isWrite());

  TimeDelta_t lat = getHitDelay(mreq);

  if (inclusive) {
    if (l->isShared() && mreq->isWrite()) {
      
      writeExclusive.inc(mreq->getStatsFlag());
      
      l->invalidate();
      
      if(router->sendInvalidateOthers(getLineSize(), mreq, mreq->getAddr(), 1)) {
        // This line was commented after we detected a possible livelock (run mcf with SHIP)
        // RENAU CallbackBase *cb  = doBusReadCB::create(this, mreq, true); // try once invs are done
        mshr->addEntry(addr);
        router->fwdBusRead(mreq, missDelay); 
        return;
      }

      if (!llc) {
        mshr->addEntry(addr);
        router->fwdBusRead(mreq, missDelay);
        return;
      }
      lat += invDelay;
    }

    I(l->isModified() || l->isExclusive());

    if (mreq->isWrite()) {
      l->setModified();
    }else{
      // Even a single copy may exist, but no way to know without tracking it
      // FIXME: use a new CB, not a choldWriteback
      if (l->isModified()) {
        doWriteback(addr);
      }

      l->setShared();
    }
  }

  avgMemLat.sample(mreq->getTimeDelay()+lat);
  router->fwdPushUp(mreq, lat); // ack the busRead
  if (retrying) {
    mshr->retire(addr);
  }
}
/* }}} */

void Cache::doPushDown(MemRequest *mreq)
/* cache push down (writeback or invalidate ack) {{{1 */
{
  AddrType addr = mreq->getAddr();

  if (mreq->isInvalidate()) {
    mreq->decPending();
    if (!mreq->hasPending()) {
      // We got all the ACKs and 
      // This line was commented after we detected a possible livelock (run mcf with SHIP)
      // RENAU mshr->retire(addr); // It can trigger a busReadQueued or invalidateQueued
    }
  }else{
    I(mreq->isWriteback());
    // writeback done
    Line *l = cacheBank->findLineDebug(addr);
    // I(l); // inclusion. We miss some due to timing. Can someone fix the bug?
    // RENAU GI(l,l->isModified() || l->isExclusive()); // L2 should be Modified if L1 is modified
    if(l)
      l->setModified();
  }

  mreq->destroy();
}
/* }}} */

void Cache::doPushUp(MemRequest *mreq)
/* cache push up (fwdBusRead ack) {{{1 */
{
  AddrType addr = mreq->getAddr();
  Line *l = cacheBank->readLine(addr);
  if (l == 0) {
    l = allocateLine(addr, mreq);
  }
#if 0
  MSG("3. %s: pe %d addr 0x%llX @%lld %p\n",this->getName(),mreq->getDInst()->getPE(),mreq->getDInst()->getAddr(),globalClock,mreq);
#endif


  I(mreq->isRead() || mreq->isWrite());

  if(mreq->isWrite())
    l->setModified();
  else
    l->setExclusive();

  mshr->retire(addr);

  avgMissLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  avgMemLat.sample(mreq->getTimeDelay()+1);
  if(mreq->isHomeNode()) {
    if (mreq->isRead()){
      pendingReads--;
    }else{
      pendingWrites--;
    }
    mreq->ack(1);
  }else {
    router->fwdPushUp(mreq, 1); // ack the busRead
  }
}
/* }}} */

void Cache::doInvalidate(MemRequest *mreq)
/* cache invalidate request {{{1 */
{
  if (!inclusive) {
    router->fwdPushDown(mreq, 0); // invalidate ack with nothing else if not inclusive
    return;
  }

  AddrType addr    = mreq->getAddr();
  int32_t ls       = getLineSize();
  int32_t maxLines = mreq->getLineSize()/(ls+1)+1;
  bool allInvalid  = true;

  for(int32_t i =0 ; i < maxLines ; i++) {
    Line *l = cacheBank->readLine(addr+i*ls);
    if (l) {
      allInvalid = false;
      l->invalidate();
    }
  }
  if (allInvalid) {
    mreq->isCapacityInvalidate()? capInvalidateHit.inc(mreq->getStatsFlag()): invalidateHit.inc(mreq->getStatsFlag());
    router->fwdPushDown(mreq, 1); // invalidate ack
    return;
  }
  mreq->isCapacityInvalidate()?capInvalidateMiss.inc(mreq->getStatsFlag()):invalidateMiss.inc(mreq->getStatsFlag());

  // Need to propagate invalidate to upper levels
  // RENAU3: if inclusive do this
  if(router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), 1)) {
    // This line was commented after we detected a possible livelock (run mcf with SHIP)
    // RENAU CallbackBase *cb  = doInvalidateCB::create(this, mreq); // FIXME: use MemRequest CBs
    router->fwdPushDown(mreq, 1); // invalidate ack
    // This line was commented after we detected a possible livelock (run mcf with SHIP)
    // RENAU mshr->addEntry(addr, cb);
  }else{
    router->fwdPushDown(mreq, 1); // invalidate ack
  }
}
/* }}} */

void Cache::read(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  // predicated ARM instructions canbe with zero address
  if(mreq->getAddr() == 0) {
    mreq->ack(getHitDelay(mreq));
    return;
  }

  doRead(mreq,false);
}
/* }}} */

void Cache::write(MemRequest *mreq)
/* main processor write entry point {{{1 */
{
  // predicated ARM insturctions can be with zero address
  if(mreq->getAddr() == 0) {
    mreq->ack(getHitDelay(mreq));
    return;
  }

  doWrite(mreq,false);
}
/* }}} */

void Cache::writeAddress(MemRequest *mreq)
/* main processor write entry point {{{1 */
{
  // FIXME: Maybe we can do prefetching in the cache
  mreq->ack(1);
}
/* }}} */

void Cache::busRead(MemRequest *mreq)
/* main cache busRead entry point {{{1 */
{
  doBusRead(mreq,false);
}
/* }}} */

void Cache::pushDown(MemRequest *mreq)
/* main cache pushDown entry point {{{1 */
{
  doPushDown(mreq);
}
/* }}} */

void Cache::pushUp(MemRequest *mreq)
/* main cache pushUp entry point {{{1 */
{
  doPushUp(mreq);
}
/* }}} */

void Cache::invalidate(MemRequest *mreq)
/* main cache invalidate entry point {{{1 */
{
  doInvalidate(mreq);
}
/* }}} */

bool Cache::canAcceptWrite(DInst *dinst) const
/* check if cache can accept more writes {{{1 */
{
  AddrType addr = dinst->getAddr();
  if(pendingWrites >= maxPendingWrites)
    return false;

  if (!mshr->canAccept(addr))
    return false;

  // return router->canAccept(dinst);
  return true;
}
/* }}} */

bool Cache::canAcceptRead(DInst *dinst) const
/* check if cache can accept more reads {{{1 */
{
  AddrType addr = dinst->getAddr();

  if (pendingReads >= maxPendingReads)
    return false;

  if (!mshr->canAccept(addr))
    return false;

  //return router->canAccept(dinst);
  return true;
}
/* }}} */

void Cache::dump() const
/* Dump some cache statistics {{{1 */
{
  double total =   readMiss.getDouble()  + readHit.getDouble()
    + writeMiss.getDouble() + writeHit.getDouble() + 1;

  MSG("%s:total=%8.0f:rdMiss=%8.0f:rdMissRate=%5.3f:wrMiss=%8.0f:wrMissRate=%5.3f"
      ,getName()
      ,total
      ,readMiss.getDouble()
      ,100*readMiss.getDouble()  / total
      ,writeMiss.getDouble()
      ,100*writeMiss.getDouble() / total
      );

  mshr->dump();
}
/* }}} */

TimeDelta_t Cache::ffread(AddrType addr, DataType data)
/* can accept reads? {{{1 */
{ 
  AddrType addr_r=0;

  if (cacheBank->readLine(addr))
    return hitDelay;   // done!

  Line *l = cacheBank->fillLine(addr, addr_r);
  l->setShared();

  // Cache miss (propagate the miss)
  // FIXME: send invalidate up for the displaced addr_r
  return router->ffread(addr,data) + missDelay;
}
/* }}} */

TimeDelta_t Cache::ffwrite(AddrType addr, DataType data)
/* can accept writes? {{{1 */
{ 
  AddrType addr_r=0;

  if (cacheBank->writeLine(addr))
    return hitDelay;   // done!

  Line *l = cacheBank->fillLine(addr, addr_r);
  l->setExclusive();

  // Cache miss (propagate the miss)
  // FIXME: send invalidate up for the displaced addr_r
  // FIXME: if line is shared, send invalidates up to everybody but itself
  return router->ffwrite(addr,data) + missDelay;
}
/* }}} */

void Cache::ffinvalidate(AddrType addr, int32_t ilineSize)
/* can accept writes? {{{1 */
{ 
  int32_t ls = getLineSize();
  int32_t maxLines = ilineSize/(ls+1)+1;
  bool allInvalid = true;
  for(int32_t i =0 ; i < maxLines ; i++) {
    Line *l = cacheBank->readLine(addr+i*ls);
    if (l) {
      allInvalid = false;
      l->invalidate();
    }
  }
  if (allInvalid)
    return;

  // FIXME: send invalidates up in the hierarchy
}
/* }}} */

// FIXME: This is NASTY NASTY NASTY, slow and buggy prone. Please delete and correct
TimeDelta_t Cache::getHitDelay(MemRequest *mreq) {
    if ( (strcasecmp(getName(), "DL1" ) == 0) &&
         (strcasecmp(getName(), "IL1" ) == 0) &&
         (strcasecmp(getName(), "DTLB") == 0) ){

      return (hitDelay * mreq -> getL1clkRatio());
    } else if (strcasecmp(getName(), "L3" ) == 0) {
      return (hitDelay * mreq -> getL3clkRatio());
    }

    return hitDelay;
}

TimeDelta_t Cache::getMissDelay(MemRequest *mreq) {
    if ( (strcasecmp(getName(), "DL1" ) == 0) &&
         (strcasecmp(getName(), "IL1" ) == 0) &&
         (strcasecmp(getName(), "DTLB") == 0) ){

      return (missDelay * mreq -> getL1clkRatio());
    } else if (strcasecmp(getName(), "L3" ) == 0) {
      return (missDelay * mreq -> getL3clkRatio());
    }

    return missDelay;
}
