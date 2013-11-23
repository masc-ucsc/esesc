#if 0
// Contributed by Jose Renau
//                Basilio Fraguela
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

#include "SescConf.h"
#include "MemorySystem.h"
#include "StridePrefetcher.h"
/* }}} */

static pool < std::queue<MemRequest *> > activeMemReqPool(32, "StridePrefetcher");

StridePrefetcher::StridePrefetcher(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  :MemObj(section, name)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,predictions("%s:predictions", name)
  ,accesses("%s:accesses", name)
  ,unitStrideStreams("%s:unitStrideStreams", name)
  ,nonUnitStrideStreams("%s:nonUnitStrideStreams", name)
  ,ignoredStreams("%s:ignoredStreams", name)
{
  MemObj *lower_level = NULL;

  // If MemLatency is 200 and busOcc is 10, then there can be at most 20
  // requests without saturating the bus. (VERIFY???) something like half sound
  // conservative (10 pending requests)
  MaxPendingRequests = 8; // FIXME: SescConf

  SescConf->isInt(section, "depth");
  depth = SescConf->getInt(section, "depth");

  SescConf->isInt(section, "missWindow");
  missWindow = SescConf->getInt(section, "missWindow");

  SescConf->isInt(section, "maxStride");
  maxStride = SescConf->getInt(section, "maxStride");

  SescConf->isInt(section, "hitDelay");
  hitDelay = SescConf->getInt(section, "hitDelay");

  SescConf->isInt(section, "missDelay");
  missDelay = SescConf->getInt(section, "missDelay");

  SescConf->isInt(section, "learnHitDelay");
  learnHitDelay = SescConf->getInt(section, "learnHitDelay");

  SescConf->isInt(section, "learnMissDelay");
  learnMissDelay = SescConf->getInt(section, "learnMissDelay");

  I(depth > 0);

  const char *buffSection = SescConf->getCharPtr(section, "buffCache");
  if (buffSection) {
    buff = BuffType::create(buffSection, "", name);

    SescConf->isInt(buffSection, "bknumPorts");
    numBuffPorts = SescConf->getInt(buffSection, "bknumPorts");

    SescConf->isInt(buffSection, "bkportOccp");
    buffPortOccp = SescConf->getInt(buffSection, "bkportOccp");
  }

  const char *streamSection = SescConf->getCharPtr(section, "streamCache");
  if (streamSection) {
    char tableName[128];
    sprintf(tableName, "%sPrefTable", name);
    table = PfTable::create(streamSection, "", tableName);

    GMSG(pEntrySize != SescConf->getInt(streamSection, "BSize"),
   "The prefetch buffer streamBSize field in the configuration file should be %d.", pEntrySize);

    SescConf->isInt(streamSection, "bknumPorts");
    numTablePorts = SescConf->getInt(streamSection, "bknumPorts");

    SescConf->isInt(streamSection, "bkportOccp");
    tablePortOccp = SescConf->getInt(streamSection, "bkportOccp");
  }

  char portName[128];
  sprintf(portName, "%s_buff", name);
  buffPort  = PortGeneric::create(portName, numBuffPorts, buffPortOccp);
  sprintf(portName, "%s_table", name);
  tablePort = PortGeneric::create(portName, numTablePorts, tablePortOccp);

  defaultMask  = ~(buff->getLineSize()-1);

  I(current);
  lower_level = current->declareMemoryObj(section, "lowerLevel");
  if (lower_level != NULL)
    addLowerLevel(lower_level);


  //Is this the memory  
  isMemoryBus = false; 
}
/* }}} */

Time_t StridePrefetcher::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  //I(0);
  return globalClock;
}
/* }}} */
Time_t StridePrefetcher::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  //I(0);
  return globalClock;
}
/* }}} */
Time_t StridePrefetcher::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
  if (nextBuffSlot() > nextTableSlot()) 
    return nextBuffSlot();
  
  return nextTableSlot();
}
/* }}} */
Time_t StridePrefetcher::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  if (nextBuffSlot() > nextTableSlot()) 
    return nextBuffSlot();
  
  return nextTableSlot();
}
/* }}} */
Time_t StridePrefetcher::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  if (nextBuffSlot() > nextTableSlot()) 
    return nextBuffSlot();
  
  return nextTableSlot();

}
/* }}} */
Time_t StridePrefetcher::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  if (nextBuffSlot() > nextTableSlot()) 
    return nextBuffSlot();
  
  return nextTableSlot();
}
/* }}} */

void StridePrefetcher::read(MemRequest *mreq)
  /* no read in StridePrefetcher {{{1 */
{
  I(0); // StridePrefetcher should not be a first level object
}
/* }}} */
void StridePrefetcher::write(MemRequest *mreq)
  /* no write in StridePrefetcher {{{1 */
{
  I(0); // StridePrefetcher should not be a first level object
}
/* }}} */
void StridePrefetcher::writeAddress(MemRequest *mreq)
  /* no writeAddress in StridePrefetcher {{{1 */
{
  I(0); // StridePrefetcher should not be a first level object
}
/* }}} */

void StridePrefetcher::busReadAck(MemRequest *mreq)
{
  pendingRequests--;
  AddrType paddr = mreq->getAddr() & defaultMask;
  mshr->retire(paddr);
  router->fwdPushUp(mreq);
}

void StridePrefetcher::busRead(MemRequest *mreq)
  /* forward StridePrefetcher read {{{1 */
{
  AddrType paddr = mreq->getAddr() & defaultMask;
  bLine *l = buff->readLine(paddr);

  /****** HIT *******/
  if(l) { 
    hit.inc(mreq->getStatsFlag());
    router->fwdPushUp(mreq, hitDelay);
    return;
  }

  if (!mshr->canAccept(paddr)) {
    CallbackBase *cb  = busReadAckCB::create(this, mreq);

    pendingRequests++;
    mshr->addEntry(paddr, cb);
    return;
  }

  pendingRequests++;
  mshr->addEntry(paddr);
  router->fwdBusRead(mreq);
}

/* }}} */
void StridePrefetcher::pushDown(MemRequest *mreq)
  /* push down {{{1 */
{
  Line *l = buff->writeLine(mreq->getAddr()); // Also for energy

  if (mreq->isInvalidate()) {
    if (l)
      l->invalidate();

    I(!isMemoryBus);
        mreq->decPending();
    if (!mreq->hasPending()) {
      MemRequest *parent = mreq->getParent();
      router->fwdPushDown(parent);
    }

    mreq->destroy();
    return;
  }

  I(mreq->isWriteback());

  router->fwdPushDown(mreq);
}
/* }}} */

void StridePrefetcher::pushUp(MemRequest *mreq)
  /* push up {{{1 */
{

  if(mreq->isHomeNode()) {
    buff->fillLine(mreq->getAddr());

    mreq->destroy();
    return;
  }

  learnMiss(paddr);

  busReadAck(mreq);
}
/* }}} */

void StridePrefetcher::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  uint32_t paddr = mreq->getAddr() & defaultMask; // FIXME: Maybe delete the defaultMask

  nextBuffSlot();

  bLine *l = buff->readLine(paddr);
  if(l)
    l->invalidate();

  // broadcast the invalidate through the upper nodes
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(),hitDelay /*delay*/);
}
/* }}} */

bool StridePrefetcher::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  return true;
}
/* }}} */

bool StridePrefetcher::canAcceptWrite(DInst *disnt) const
/* always can accept reads {{{1 */
{
  return true;
}
/* }}} */

TimeDelta_t StridePrefetcher::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  I(0);
  return router->ffread(addr,data) /*+ delay*/;
}
/* }}} */

TimeDelta_t StridePrefetcher::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  I(0);
  return router->ffwrite(addr,data) /*+ delay*/ ;
}
/* }}} */

void StridePrefetcher::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0);
  // FIXME: router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}
/* }}} */

void StridePrefetcher::learnMiss(AddrType addr) {

  AddrType paddr = addr & defaultMask;
  Time_t lat = nextTableSlot() - globalClock;

  bool foundUnitStride = false;
  uint32_t newStride = 0;
  uint32_t minDelta = (uint32_t) -1;
  bool goingUp = true;

  if(lastMissesQ.empty()) {
    lastMissesQ.push_back(paddr);
    return;
  }

  // man, this is baad. i have to do a better search here
  std::deque<AddrType>::iterator it = lastMissesQ.begin();
  while(it != lastMissesQ.end()) {

   uint32_t delta;
    if(paddr < (*it)) {
      goingUp = false;
      delta = (*it) - paddr;
    } else {
      goingUp = true;
      delta = paddr - (*it);
    }
    minDelta = (delta < minDelta ? delta : minDelta);

    if((*it) == paddr - buff->getLineSize() || (*it) == paddr + buff->getLineSize()) {
      foundUnitStride = true;
      break;
    }
    it++;
  }

  // putting the new miss in the queue after we computed the stride
  lastMissesQ.push_back(paddr);

  if(lastMissesQ.size() > missWindow)
    lastMissesQ.pop_front();

  if(foundUnitStride) {
    unitStrideStreams.inc();
    newStride = buff->getLineSize();
  } else {
    nonUnitStrideStreams.inc();
    newStride = minDelta;
  }

  if(newStride == 0 || newStride == (uint32_t) -1 || newStride > maxStride) {
    ignoredStreams.inc();
    return;
  }

  AddrType nextAddr = goingUp ? paddr + newStride : paddr - newStride;

  if(!table->readLine(nextAddr) && !table->readLine(paddr)) {
    pEntry *pe = table->fillLine(paddr);
    pe->stride = newStride;
    pe->goingUp = goingUp;
  }

  if (pendingRequests> MaxPendingRequests) {
    // FIXME: fetch the depth following addresses

    AddrType paddr = nextAddr & defaultMask;
    bLine *l = buff->readLine(paddr);
    if (l==0) {
      MemRequest *mreq = MemRequest::createRead(this, nextAddr, 0);
      router->fwdBusRead(mreq, missDelay); 
    }
  }

}


#endif
