// Contributed by Jose Renau
//                Alamelu Sankaranarayanan
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

#include "TLB.h"
#include "MemorySystem.h"
#include "SescConf.h"
/* }}} */
//#define TLB_ALWAYS_HITS 1

TLB::TLB(MemorySystem *current, const char *section, const char *name)
    /* constructor {{{1 */
    : MemObj(section, name)
    , delay(SescConf->getInt(section, "hitDelay")) // this is the delay with which the mem requests will be sent
    , tlbReadHit("%s:readHit", name)
    , tlbReadMiss("%s:readMiss", name)
    , tlblowerReadHit("%s:LowerTLBHit", name)
    , tlblowerReadMiss("%s:LowerTLBMiss", name)
    , avgMissLat("%s_avgMissLat", name)
    , avgMemLat("%s_avgMemLat", name) {
  I(current);
  SescConf->isInt(section, "hitDelay");

  SescConf->isInt(section, "numPorts");
  NumUnits_t num = SescConf->getInt(section, "numPorts");

  char cadena[100];
  sprintf(cadena, "Cmd%s", name);
  cmdPort = PortGeneric::create(cadena, num, 1);

  MemObj *lower_level = NULL;
  lower_level         = current->declareMemoryObj(section, "lowerLevel");

  maxRequests = SescConf->getInt(section, "maxRequests");
  curRequests = 0;

  if(lower_level) {
    addLowerLevel(lower_level);
    lowerCache = lower_level;
  } else {
    lowerCache = NULL;
  }

  char tmpName[512];
  sprintf(tmpName, "%s_TLB", name);
  tlbBank = CacheType::create(section, "", tmpName);
  I(tlbBank->getLineSize() > 1024); // Line size is the TLB page size, so make sure that it is big (4K standard?)

  if(SescConf->checkCharPtr(section, "lowerTLB")) {
    SescConf->isInt(section, "lowerTLB_delay");
    lowerTLBdelay = SescConf->getInt(section, "lowerTLB_delay");
    lowerTLB      = current->declareMemoryObj(section, "lowerTLB");
  } else {
    lowerTLB      = NULL;
    lowerTLBdelay = delay;
  }
}
/* }}} */

void TLB::doReq(MemRequest *mreq)
/* forward bus read {{{1 */
{
  bool retrying = mreq->isRetrying();
  if(retrying)
    mreq->clearRetrying();
    // MSG("@%lld bus %s 0x%lx %d",globalClock, mreq->getCurrMem()->getName(), mreq->getAddr(), mreq->getAction());

#ifdef TLB_ALWAYS_HITS
  bool l = true;
#else
  Line *l = tlbBank->readLine(mreq->getAddr());
#endif
  if(l == 0 && mreq->isWarmup()) {
    l = tlbBank->fillLine(mreq->getAddr(), 0xdeadbeef);
    if(lowerTLB) {
      lowerTLB->ffread(mreq->getAddr()); // Fill the L2 too
    }
  }
  if(l) { // TLB Hit

    tlbReadHit.inc(mreq->getStatsFlag());
    router->scheduleReq(mreq, delay);

    if(retrying) {
      I(pending.front() == mreq);
      pending.pop_front();
      wakeupNext();
    }
    return;
  }

  // TLB Miss
  tlbReadMiss.inc(mreq->getStatsFlag());

  if(lowerTLB != NULL) {
    // Check the lowerTLB for a miss.
    if(lowerTLB->checkL2TLBHit(mreq) == true) {

      tlbBank->fillLine(mreq->getAddr(), 0xdeadbeef);
      tlblowerReadHit.inc(mreq->getStatsFlag());

      router->scheduleReq(mreq, lowerTLBdelay);

      if(retrying) {
        I(pending.front() == mreq);
        pending.pop_front();
        wakeupNext();
      }
      return;
    }

    tlblowerReadMiss.inc(mreq->getStatsFlag());
  }

  if(pending.empty() || retrying) {
    // 1 outstanding miss at most (TLB, no MSHR complications)
    MemRequest::sendReqRead(lowerCache, mreq->getStatsFlag(), calcPage1Addr(mreq->getAddr()),
                            0xbeefbeef, // PC signature for TLB
                            readPage1CB::create(this, mreq));
  }

  if(!retrying)
    pending.push_back(mreq);
}
/* }}} */

void TLB::doDisp(MemRequest *mreq)
/* forward bus read {{{1 */
{
  router->scheduleDisp(mreq, delay);
}
/* }}} */

void TLB::doReqAck(MemRequest *mreq)
/* data is coming back {{{1 */
{
  avgMemLat.sample(mreq->getTimeDelay() + delay, mreq->getStatsFlag());

  if(mreq->isHomeNode()) {
    mreq->ack();
    return;
  }

  router->scheduleReqAck(mreq, delay);
}
/* }}} */

void TLB::doSetState(MemRequest *mreq)
/* forward set state to all the upper nodes {{{1 */
{
  if(router->isTopLevel()) {
    mreq->convert2SetStateAck(ma_setInvalid, false);
    router->scheduleSetStateAck(mreq, delay);
    return;
  }
  router->sendSetStateAll(mreq, mreq->getAction(), delay);
}
/* }}} */

void TLB::doSetStateAck(MemRequest *mreq)
/* forward set state to all the upper nodes {{{1 */
{
  router->scheduleSetStateAck(mreq, delay);
}
/* }}} */

bool TLB::isBusy(AddrType addr) const
/* accept requests if no pending misses {{{1 */
{
  if(!pending.empty())
    return true;

  if(curRequests >= maxRequests)
    return true;

  if(lowerCache == 0)
    return false;

  return lowerCache->isBusy(addr);
}
/* }}} */

void TLB::tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* forward tryPrefetch {{{1 */
{
  router->tryPrefetch(addr, doStats, degree, pref_sign, pc, cb);
}
/* }}} */

void TLB::readPage1(MemRequest *mreq) {
  MemRequest::sendReqRead(lowerCache, mreq->getStatsFlag(), calcPage2Addr(mreq->getAddr()),
                          0xbeefbeef, // PC signature for TLB
                          readPage2CB::create(this, mreq));
}

void TLB::readPage2(MemRequest *mreq) {
  MemRequest::sendReqRead(lowerCache, mreq->getStatsFlag(), calcPage3Addr(mreq->getAddr()),
                          0xbeefbeef, // PC signature for TLB
                          readPage3CB::create(this, mreq));
}

void TLB::wakeupNext() {
  if(pending.empty())
    return;

  MemRequest *preq = pending.front();
  // pending.pop_front();
  preq->setRetrying();

  doReq(preq);
}

void TLB::readPage3(MemRequest *mreq) {
  tlbBank->fillLine(mreq->getAddr(), 0xdeadbeef);
  TimeDelta_t lat = 0;
  if(lowerTLB)
    lat += lowerTLB->ffread(mreq->getAddr()); // Fill the L2 too

  I(!pending.empty());
  I(pending.front() == mreq);

  pending.pop_front();

  avgMissLat.sample(lat + delay, mreq->getStatsFlag());
  router->scheduleReq(mreq, delay);
  wakeupNext();
}

TimeDelta_t TLB::ffread(AddrType addr)
// {{{1 rabbit read
{
  if(tlbBank->readLine(addr))
    return delay; // done!

  if(lowerTLB)
    lowerTLB->ffread(addr);

  tlbBank->fillLine(addr, 0xdeadbeef);
  if(lowerCache)
    return router->ffread(addr) + lowerTLBdelay;
  return delay;
}
// 1}}}

TimeDelta_t TLB::ffwrite(AddrType addr)
// {{{1 rabbit write
{
  if(tlbBank->readLine(addr))
    return delay; // done!

  if(lowerTLB)
    lowerTLB->ffwrite(addr);

  tlbBank->fillLine(addr, 0xdeadbeef);
  if(lowerCache)
    return router->ffwrite(addr) + delay;
  return delay;
}
/// 1}}}

bool TLB::checkL2TLBHit(MemRequest *mreq)
// {{{1 TLB direct requests
{
  AddrType addr = mreq->getAddr();
  Line *   l    = tlbBank->readLine(addr);
  if(l) {
    return true;
  }

  return false;
}
// 1}}}

void TLB::req(MemRequest *mreq)
/* main read entry point {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack(1);
    return;
  }
  if(!mreq->isRetrying())
    curRequests++;
  // I(curRequests<=maxRequests && !mreq->isWarmup());
  I(curRequests > 0);
  // I(!mreq->isRetrying());
  mreq->redoReqAbs(cmdPort->nextSlot(mreq->getStatsFlag()));
}
// }}}

void TLB::reqAck(MemRequest *mreq)
/* main read entry point {{{1 */
{
  // I(!mreq->isRetrying());
  if(!mreq->isRetrying())
    curRequests--;
  // I(curRequests<=maxRequests && !mreq->isWarmup());
  I(curRequests >= 0);
  mreq->redoReqAckAbs(cmdPort->nextSlot(mreq->getStatsFlag()));
}
// }}}

void TLB::setState(MemRequest *mreq)
/* main read entry point {{{1 */
{
  I(!mreq->isRetrying());
  mreq->redoSetStateAbs(cmdPort->nextSlot(mreq->getStatsFlag()));
}
// }}}

void TLB::setStateAck(MemRequest *mreq)
/* main read entry point {{{1 */
{
  I(!mreq->isRetrying());
  mreq->redoSetStateAckAbs(cmdPort->nextSlot(mreq->getStatsFlag()));
}
// }}}

void TLB::disp(MemRequest *mreq)
/* displacement entry point {{{1 */
{
  I(!mreq->isRetrying());
  mreq->redoDispAbs(cmdPort->nextSlot(mreq->getStatsFlag()));
}
// }}}
