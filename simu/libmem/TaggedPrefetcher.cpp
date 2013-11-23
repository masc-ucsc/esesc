#if 0
// Contributed by Jose Renau
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
#include "TaggedPrefetcher.h"
//#include "ThreadContext.h"

// A tagged prefetcher prefetches the next line on a demand fetch and when the
// memRequest was already prefetched.

static pool < std::queue<MemRequest *> > activeMemReqPool(32,"TaggedPrefetcher");

TaggedPrefetcher::TaggedPrefetcher(MemorySystem* current
             ,const char *section
             ,const char *name)
  : MemObj(section, name)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,accesses("%s:accesses", name)
  ,predictions("%s:predictions", name)
{
  MemObj *lower_level = NULL;

  const char *Section = SescConf->getCharPtr(section, "nextLevel");
  if (Section) {
    lineSize = SescConf->getInt(Section, "bsize");
  }

  I(current);

  lower_level = current->declareMemoryObj(section, k_lowerLevel);

  const char *buffSection = SescConf->getCharPtr(section, "buffCache");
  if (buffSection) {
    buff = BuffType::create(buffSection, "", name);

    SescConf->isInt(buffSection, "numPorts");
    numBuffPorts = SescConf->getInt(buffSection, "numPorts");

    SescConf->isInt(buffSection, "portOccp");
    buffPortOccp = SescConf->getInt(buffSection, "portOccp");
  }

  defaultMask  = ~(buff->getLineSize()-1);

  char portName[128];
  sprintf(portName, "%s_buff", name);
  buffPort  = PortGeneric::create(portName, numBuffPorts, buffPortOccp);

  if (lower_level != NULL)
    addLowerLevel(lower_level);
}

void TaggedPrefetcher::access(MemRequest *mreq)
{
  if (mreq->getMemOperation() == MemRead
      || mreq->getMemOperation() == MemReadW) {
    read(mreq);
  } else {
    mreq->goDown(0, lowerLevel[0]);
  }
  accesses.inc(mreq->getStatsFlag());
}

void TaggedPrefetcher::read(MemRequest *mreq)
{
  uint32_t paddr = mreq->getAddr() & defaultMask;
  bLine *l = buff->readLine(paddr);

  if(l) { //hit
    LOG("NLAP: hit on [%08lx]", paddr);
    hit.inc(mreq->getStatsFlag());
    mreq->goUpAbs(nextBuffSlot());

    prefetch(paddr+(buff->getLineSize()), 0);

    prefetch(paddr+(2*buff->getLineSize()), 0);

    return;
  }

  penFetchSet::iterator it = pendingFetches.find(paddr);
  if(it != pendingFetches.end()) { // half-miss
    LOG("NLAP: half-miss on %08lx", paddr);
    halfMiss.inc(mreq->getStatsFlag());
    penReqMapper::iterator itR = pendingRequests.find(paddr);

    if (itR == pendingRequests.end()) {
      pendingRequests[paddr] = activeMemReqPool.out();
      itR = pendingRequests.find(paddr);
    }

    I(itR != pendingRequests.end());
    (*itR).second->push(mreq);
    return;
  }

  LOG("NLAP: miss on [%08lx]", paddr);
  miss.inc(mreq->getStatsFlag());
  mreq->goDown(0, lowerLevel[0]);

  prefetch(paddr+(buff->getLineSize()), 0);

  prefetch(paddr+(2*buff->getLineSize()), 0);
}

void TaggedPrefetcher::prefetch(AddrType prefAddr, Time_t lat)
{
  uint32_t paddr = prefAddr & defaultMask;

    if(!buff->readLine(prefAddr)) { // it is not in the buff
      penFetchSet::iterator it = pendingFetches.find(prefAddr);
      if(it == pendingFetches.end()) {
  CBMemRequest *r;

  r = CBMemRequest::create(lat, lowerLevel[0], MemRead, paddr,
         processAckCB::create(this, paddr));
  r->markPrefetch();
  predictions.inc(mreq->getStatsFlag());
  LOG("NLAP: prefetch [0x%08lx]", paddr);
  pendingFetches.insert(paddr);
      }
    }
}

void TaggedPrefetcher::returnAccess(MemRequest *mreq)
{
  mreq->goUp(0);
}

bool TaggedPrefetcher::canAcceptStore(AddrType addr)
{
  return true;
}

void TaggedPrefetcher::invalidate(AddrType addr,uint16_t size,MemObj *oc)
{
  uint32_t paddr = addr & defaultMask;
   nextBuffSlot();

   bLine *l = buff->readLine(paddr);
   if(l)
     l->invalidate();
}

Time_t TaggedPrefetcher::getNextFreeCycle() const
{
  return cachePort->nextSlot();
}

void TaggedPrefetcher::processAck(AddrType addr)
{
  uint32_t paddr = addr & defaultMask;

  penFetchSet::iterator itF = pendingFetches.find(paddr);
  if(itF == pendingFetches.end())
    return;

  buff->fillLine(paddr);

  penReqMapper::iterator it = pendingRequests.find(paddr);

  if(it != pendingRequests.end()) {
    LOG("NLAP: returnAccess [%08lx]", paddr);
    std::queue<MemRequest *> *tmpReqQueue;
    tmpReqQueue = (*it).second;
    while (tmpReqQueue->size()) {
      tmpReqQueue->front()->goUpAbs(nextBuffSlot());
      tmpReqQueue->pop();
    }
    pendingRequests.erase(paddr);
    activeMemReqPool.in(tmpReqQueue);
  }
  pendingFetches.erase(paddr);
}
#endif
