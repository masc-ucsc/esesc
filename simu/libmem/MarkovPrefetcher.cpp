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
#include "MarkovPrefetcher.h"

static pool < std::queue<MemRequest *> > activeMemReqPool(128,"MarkovPrefetcherer");
static AddrType TESTdata[11];


MarkovPrefetcher::MarkovPrefetcher(MemorySystem* current
             ,const char *section
             ,const char *name)
  : MemObj(section, name)
  ,gms(current)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,predictions("%s:predictions", name)
  ,accesses("%s:accesses", name)
{
  MemObj *lower_level = NULL;


  SescConf->isInt(section, "depth");
  depth = SescConf->getInt(section, "depth");

  SescConf->isInt(section, "width");
  width = SescConf->getInt(section, "width");

  const char *Section = SescConf->getCharPtr(section, "nextLevel");
  if (Section) {
    lineSize = SescConf->getInt(Section, "bsize");
  }

  const char *buffSection = SescConf->getCharPtr(section, "buffCache");
  if (buffSection) {
    buff = BuffType::create(buffSection, "", name);

    SescConf->isInt(buffSection, "numPorts");
    numBuffPorts = SescConf->getInt(buffSection, "numPorts");

    SescConf->isInt(buffSection, "portOccp");
    buffPortOccp = SescConf->getInt(buffSection, "portOccp");
  }

  const char *streamSection = SescConf->getCharPtr(section, "streamCache");
  if (streamSection) {
    char tableName[128];
    sprintf(tableName, "%sPrefTable", name);
    table = MarkovTable::create(streamSection, "", tableName);

    GMSG(tEntrySize != SescConf->getInt(streamSection, "BSize"),
   "The prefetch buffer streamBSize field in the configuration file should be %d.", tEntrySize);

    SescConf->isInt(streamSection, "numPorts");
    numTablePorts = SescConf->getInt(streamSection, "numPorts");

    SescConf->isInt(streamSection, "portOccp");
    tablePortOccp = SescConf->getInt(streamSection, "portOccp");
  }

  defaultMask  = ~(buff->getLineSize()-1);

  char portName[128];
  sprintf(portName, "%s_buff", name);
  buffPort  = PortGeneric::create(portName, numBuffPorts, buffPortOccp);

  sprintf(portName, "%s_table", name);
  tablePort = PortGeneric::create(portName, numTablePorts, tablePortOccp);

  ptr = 0;
  lastAddr = 0;

  I(current);
  lower_level = current->declareMemoryObj(section, k_lowerLevel);
  if (lower_level != NULL)
    addLowerLevel(lower_level);

}

void MarkovPrefetcher::access(MemRequest *mreq)
{
  uint32_t paddr = mreq->getAddr() & defaultMask;

  if (mreq->getMemOperation() == MemRead
      || mreq->getMemOperation() == MemReadW) {
    read(mreq);
    insertTable(paddr);
  }else{
      bLine *l = buff->readLine(paddr);
      if(l)
  l->invalidate();
    mreq->goDown(0, lowerLevel[0]);
  }
  accesses.inc();
}

void MarkovPrefetcher::read(MemRequest *mreq)
{
  uint32_t paddr = mreq->getAddr() & defaultMask;
  bLine *l = buff->readLine(paddr);

  if(l) { //hit
    hit.inc();
    mreq->goUpAbs(nextBuffSlot());
    return;
  }

  penFetchSet::iterator it = pendingFetches.find(paddr);
  if(it != pendingFetches.end()) { // half-miss
    //LOG("GHBP: half-miss on %08lx", paddr);
    halfMiss.inc();
    penReqMapper::iterator itR = pendingRequests.find(paddr);

    if (itR == pendingRequests.end()) {
      pendingRequests[paddr] = activeMemReqPool.out();
      itR = pendingRequests.find(paddr);
    }

    I(itR != pendingRequests.end());

    (*itR).second->push(mreq);
    return;
  }

  //LOG("GHBP: miss on [%08lx]", paddr);
  miss.inc();
  mreq->goDown(0, lowerLevel[0]);
}

void MarkovPrefetcher::prefetch(AddrType prefAddr, Time_t lat)
{
  uint32_t paddr = prefAddr & defaultMask;

  if(!buff->readLine(paddr)) { // it is not in the buff
    penFetchSet::iterator it = pendingFetches.find(paddr);
    if(it == pendingFetches.end()) {
      CBMemRequest *r;

      r = CBMemRequest::create(lat, lowerLevel[0], MemRead, paddr,
             processAckCB::create(this, paddr));
      if(lat != 0) { // if lat=0, the req might not exist anymore at this point
  r->markPrefetch();
      }

      predictions.inc();
      pendingFetches.insert(paddr);
    }
  }
}


void MarkovPrefetcher::returnAccess(MemRequest *mreq)
{
  mreq->goUp(0);
}

bool MarkovPrefetcher::canAcceptStore(AddrType addr)
{
  return true;
}

void MarkovPrefetcher::invalidate(AddrType addr,uint16_t size,MemObj *oc)
{
  uint32_t paddr = addr & defaultMask;
   nextBuffSlot();

   bLine *l = buff->readLine(paddr);
   if(l)
     l->invalidate();
}

Time_t MarkovPrefetcher::getNextFreeCycle() const
{
  return cachePort->nextSlot(); 
}

void MarkovPrefetcher::processAck(AddrType addr)
{
  uint32_t paddr = addr & defaultMask;

  penFetchSet::iterator itF = pendingFetches.find(paddr);
  if(itF == pendingFetches.end())
    return;

  buff->fillLine(paddr);

  penReqMapper::iterator it = pendingRequests.find(paddr);

  if(it != pendingRequests.end()) {
    //LOG("GHBP: returnAccess [%08lx]", paddr);
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

void MarkovPrefetcher::insertTable(AddrType addr){
  uint32_t tag = table->calcTag(addr);
  Time_t lat = 0;

  if(tag){

    tEntry = table->readLine(addr);

    if(tEntry){
      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr1,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr2,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr3,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr4,lat);


      //LOG("Prefetch %d", tEntry->predAddr1);
      //LOG("Prefetch %d", tEntry->predAddr2);
    }else{
      tEntry = table->fillLine(addr);
    }

    LOG("last Addr %d", lastAddr);
    tEntry = table->readLine(lastAddr);

    //update last entry
    tEntry->predAddr4 = tEntry->predAddr3;
    tEntry->predAddr3 = tEntry->predAddr2;
    tEntry->predAddr2 = tEntry->predAddr1;
    tEntry->predAddr1 = addr;

    lastAddr = addr;
  }
}

void MarkovPrefetcher::TESTinsertTable(AddrType addr){

  TESTdata[0] = 10000;
  TESTdata[1] = 20000;
  TESTdata[2] = 30000;
  TESTdata[3] = 40000;
  TESTdata[4] = 30000;
  TESTdata[5] = 10000;
  TESTdata[6] = 30000;
  TESTdata[7] = 40000;
  TESTdata[8] = 20000;
  TESTdata[9] = 30000;
  TESTdata[10] = 10000;

  for(int32_t i = 0; i < 11; i++) {
    AddrType d = TESTdata[i];
    LOG("Addr %d", d);
    insertTable(d);
  }

}
#endif
