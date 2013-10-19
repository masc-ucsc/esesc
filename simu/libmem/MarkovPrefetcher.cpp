#if 0
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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
  return cachePort->calcNextSlot();
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
