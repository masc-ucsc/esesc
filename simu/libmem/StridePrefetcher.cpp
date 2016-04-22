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
//#include "MSHR.h"
#include <iostream>

/* }}} */

StridePrefetcher::StridePrefetcher(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  :MemObj(section, name)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,predictions("%s:predictions", name)
  ,accesses("%s:accesses", name)
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



  const char *buffSection = SescConf->getCharPtr(section, "buffCache");
  if (buffSection) {
   buff = BuffType::create(buffSection, "", name);
   lineSize  = buff->getLineSize();

    SescConf->isInt(buffSection, "bkNumPorts");
    numBuffPorts = SescConf->getInt(buffSection, "bkNumPorts");

    SescConf->isInt(buffSection, "bkPortOccp");
    buffPortOccp = SescConf->getInt(buffSection, "bkPortOccp");

    //SescConf->isInt(buffSection, "bsize");
    //lineSize = SescConf->getInt(buffSection,"bsize");
  }

  //defaultMask  = ~(lineSize-1);

  numBuffPorts = SescConf->getInt(buffSection, "bkNumPorts");
  buffPortOccp = SescConf->getInt(buffSection, "bkPortOccp");
/*
  char cadena[100];
  sprintf(cadena,"Data%s", name);
  dataPort = PortGeneric::create(cadena, num, occ);
  sprintf(cadena,"Cmd%s", name);
  cmdPort  = PortGeneric::create(cadena, num, 1);
*/
  char portName[128];
  sprintf(portName, "%s_buff", name);
  dataPort  = PortGeneric::create(portName, numBuffPorts, buffPortOccp);
  cmdPort = PortGeneric::create(portName, numBuffPorts, 1);

  I(current);
  lower_level = current->declareMemoryObj(section, "lowerLevel");
  if (lower_level != NULL)
    addLowerLevel(lower_level);
}
/* }}} */

void StridePrefetcher::doReq(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  TimeDelta_t when = cmdPort->nextSlotDelta(mreq->getStatsFlag())+delay;

  uint32_t paddr = mreq->getAddr();
  bLine *l = buff->readLine(paddr);

  if(l) { //hit
    hit.inc(mreq->getStatsFlag()); //increment counter
    mreq->convert2ReqAck(ma_setExclusive);
    router->scheduleReqAck(mreq, when);   /* schedule reqAck up {{{1 */
    return;
  }
  I(0); // FIXME: still not calling to learn the prefetch tables

  router->scheduleReq(mreq, when);  
}
/* }}} */

void StridePrefetcher::doDisp(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleDisp(mreq, when);
}
/* }}} */

void StridePrefetcher::doReqAck(MemRequest *mreq)
  /* data is coming back {{{1 */
{
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleReqAck(mreq, when);
}
/* }}} */

void StridePrefetcher::doSetState(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
  router->sendSetStateAll(mreq, mreq->getAction(), delay);  /* send setState to others, return how many {{{1 */
}
/* }}} */

void StridePrefetcher::doSetStateAck(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
  bLine *b = buff->readLine(mreq->getAddr());
  if (b)
    b->invalidate();

  router->scheduleSetStateAck(mreq, delay);
}
/* }}} */

bool StridePrefetcher::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  return false;
}
/* }}} */

void StridePrefetcher::tryPrefetch(AddrType addr, bool doStats)
  /* drop prefetch {{{1 */
{ 
}
/* }}} */

TimeDelta_t StridePrefetcher::ffread(AddrType addr)
  /* fast forward reads {{{1 */
{ 
  return delay;
}
/* }}} */

TimeDelta_t StridePrefetcher::ffwrite(AddrType addr)
  /* fast forward writes {{{1 */
{ 
  return delay;
}
/* }}} */

void StridePrefetcher::ifHit(MemRequest *mreq)
  /* forward StridePrefetcher read {{{1 */
{
  bLine *l = buff->readLine(mreq->getAddr());

  if(l) { 
    hit.inc(mreq->getStatsFlag());
    router->scheduleReqAckAbs(mreq, hitDelay);
    return;
  }
}
/* }}} */


void StridePrefetcher::ifMiss(MemRequest *mreq)
  /* push up {{{1 */
{
  AddrType paddr = mreq->getAddr() & defaultMask;
  learnMiss(paddr); //NOTE LEARN MISS
}
/* }}} */

void StridePrefetcher::learnMiss(AddrType addr) {

  AddrType paddr = addr & defaultMask;

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
    //unitStrideStreams.inc();
    newStride = buff->getLineSize();
  } else {
    //nonUnitStrideStreams.inc();
    newStride = minDelta;
  }

  if(newStride == 0 || newStride == (uint32_t) -1 || newStride > maxStride) {
    return;
  }

  AddrType nextAddr = goingUp ? paddr + newStride : paddr - newStride;

  if(!table->readLine(nextAddr) && !table->readLine(paddr)) {
    pEntry *pe = table->fillLine(paddr);
    pe->stride = newStride;
    pe->goingUp = goingUp;
  }

  paddr = nextAddr & defaultMask;
  bLine *l = buff->readLine(paddr);
  if (l==0) {
    pendingFetches.insert(paddr);
  }
}

