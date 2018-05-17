#if 0
// Contributed by Luis Ceze
//                Jose Renau
//                Karin Strauss
//                Milos Prvulovic
//                David Munday
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

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "nanassert.h"

#include "MSHR.h"
#include "MemorySystem.h"
#include "SL0Cache.h"
#include "SescConf.h"
/* }}} */

SL0Cache::SL0Cache(MemorySystem *gms, const char *section, const char *name)
  /* constructor {{{1 */
  : Cache(gms, section, name)
  ,wcb(8, 5) // 8 entries, 5 bits offset
  // stats
  ,wcbPass("%s:wcbPass"  , name)
  ,wcbFail("%s:wcbFail"  , name)
  ,dataPass("%s:dataPass", name)
  ,dataFail("%s:dataFail", name)
{
  printf("SL0 cache constructor invoked\n");
  exit(1);
}
/* }}} */

SL0Cache::~SL0Cache()
  /* destructor {{{1 */
{
}
/* }}} */

void SL0Cache::displaceLine(AddrType addr, MemRequest *mreq, bool modified)
  /* displace a line from the cache. writeback if necessary {{{1 */
{
  // Nothing to do with SL0. Never displace
}
/* }}} */

void SL0Cache::doWriteback(MemRequest *wbreq)
/* write back a line {{{1 */
{
  I(0);
  // Nothing to do with SL0. Never displace
}
/* }}} */

void SL0Cache::doRead(MemRequest *mreq, bool ignoreCanIssue)
  /* SL0 intercepting doRead ops {{{1 */
{

#if 1
  DInst *dinst = mreq->getDInst();
  I(dinst->getAddr());
  I(dinst->getAddr() == mreq->getAddr());
  I(dinst->getInst()->isLoad());

  //DEBUG
  //printf("LD id=%lld: @%lld pc=0x%x addr=0x%x data=0x%x\n",
  //(long long)dinst->getID(), (long long)globalClock,dinst->getPC(),dinst->getAddr(),dinst->getData());
  //DEBUG

  bool correct;
  if (wcb.loadCheck(dinst,correct)) {
    // The WCB 1st
    wcbPass.inc(mreq->getStatsFlag());
    // gproc->replay(dinst); FIXME: mark dinst so that Resource:performed can act
  }else{
          wcbFail.inc(mreq->getStatsFlag());
    // Look at the cache bank
    CacheData::const_iterator it = cacheData.find(mreq->getAddr()>>2);
    if (it != cacheData.end()) {
      if (it->second == dinst->getData()) {
        dataPass.inc(mreq->getStatsFlag());
      }else{
        dataFail.inc(mreq->getStatsFlag());
        // gproc->replay(dinst);
      }
    }
  }

#endif

  // do the read after the checks because it can clean up mreq
  Cache::doRead(mreq,ignoreCanIssue);
}
/* }}} */

void SL0Cache::doWrite(MemRequest *mreq, bool ignoreCanIssue)
  /* SL0 intercepting doWrite ops {{{1 */
{
  //I(mreq->getAddr() != 0x408006dc);
#if 1
  DInst *dinst = mreq->getDInst();
  I(dinst->getAddr());
  I(dinst->getAddr() == mreq->getAddr());
  I(dinst->getInst()->isStore());

  if (wcb.write(dinst)) {
    I(wcb.displaced.valid);
    AddrType addr = wcb.displaced.tag<<5; // FIXME, 2**5 is cache line size
    for(uint32_t i=0;i<wcb.getnWords();i++, addr+=4) {
      if (!wcb.displaced.present[i])
        continue;
      DataType data = wcb.displaced.data[i];
      cacheData[addr>>2] = data;
    }
  }
#endif

  // do the write after the checks because it can clean up mreq
  Cache::doWrite(mreq,ignoreCanIssue);
}
/* }}} */

void SL0Cache::writeAddress(MemRequest *mreq)
{
  //I(mreq->getAddr() != 0x408006dc);
  wcb.writeAddress(mreq->getDInst());

  Cache::writeAddress(mreq);
}

void SL0Cache::doBusRead(MemRequest *mreq, bool ignoreCanIssue)
  /* cache busRead (exclusive or shared) request {{{1 */
{
  I(0); // it should never be called (first level cache)
}
/* }}} */

void SL0Cache::doPushDown(MemRequest *mreq)
  /* cache push down (writeback or invalidate ack) {{{1 */
{
  I(0); // it should never be called (first level cache)
}
/* }}} */

void SL0Cache::doInvalidate(MemRequest *mreq)
  /* cache invalidate request {{{1 */
{
  // TODO: do we invalida or speculetive update the wcb? (remote write to wcb)

  uint16_t inv_lineSize = mreq->getLineSize();
  AddrType sAddr = mreq->getAddr() & (inv_lineSize-1);
  AddrType eAddr = sAddr + inv_lineSize;

  for(AddrType addr=sAddr;addr<eAddr;addr+=getLineSize()) {
    Line *l = cacheBank->findLineDebug(addr);
    if (l) {
      l->invalidate();
    }
  }
  // Use only upper 30bits (2 lower are zero) for the dataCache
  sAddr = sAddr >> 2;
  eAddr = eAddr >> 2;
  for(AddrType addr=sAddr;addr<eAddr;addr++) {
    CacheData::iterator it = cacheData.find(addr);
    if (it != cacheData.end())
      cacheData.erase(it);
  }

  router->fwdPushDown(mreq, 0); // invalidate ack
}
/* }}} */

TimeDelta_t SL0Cache::ffread(AddrType addr, DataType data)
  /* can accept writes? {{{1 */
{
  I(0);//FIXME: change the WCB

  return Cache::ffread(addr,data);
}
/* }}} */

TimeDelta_t SL0Cache::ffwrite(AddrType addr, DataType data)
  /* can accept writes? {{{1 */
{
  I(0);//FIXME: change the WCB

  return Cache::ffwrite(addr,data);
}
/* }}} */

void SL0Cache::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* can accept writes? {{{1 */
{
  I(0);//FIXME: change the WCB

  Cache::ffinvalidate(addr,ilineSize);
}
/* }}} */

bool SL0Cache::canAcceptRead(DInst *dinst)
/* SLO can accept read check which avoids fast LD/ST {{{1 */
{
  bool b = Cache::canAcceptRead(dinst);

  if (!b)
    return false;

  return !wcb.detect_stallread(dinst);
  //return true;
}
/* }}} */

bool SL0Cache::canAcceptWrite(DInst *dinst)
/* SLO can accept read check which avoids fast LD/ST {{{1 */
{
  bool b = Cache::canAcceptWrite(dinst);

  if (!b)
    return false;

  //I(dinst->getAddr() != 0x408006dc);

  return !wcb.detect_stallwrite(dinst);
  //return true;
}
/* }}} */
#endif
