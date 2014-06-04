#if 1
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

#include <iostream>
#include "SescConf.h"
#include "MemorySystem.h"
#include "MarkovPrefetcher.h"

static pool < std::queue<MemRequest *> > activeMemReqPool(128,"MarkovPrefetcherer");
static AddrType TESTdata[11];


MarkovPrefetcher::MarkovPrefetcher(MemorySystem* current,const char *section,const char *name)
  :MemObj(section, name)
  ,halfMiss("%s:halfMiss", name)
  ,miss("%s:miss", name)
  ,hit("%s:hits", name)
  ,predictions("%s:predictions", name)
  ,accesses("%s:accesses", name)
{
  MemObj *lower_level = NULL;
  lower_level = current->declareMemoryObj(section, "lowerLevel");
/*
  SescConf->isInt(section, "depth");
  depth = SescConf->getInt(section, "depth");

  SescConf->isInt(section, "width");
  width = SescConf->getInt(section, "width");

  const char *Section = SescConf->getCharPtr(section, "nextLevel");
*/
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

  //defaultMask  = ~(buff->getLineSize()-1);

  char portName[128];
  sprintf(portName, "%s_buff", name);
  dataPort  = PortGeneric::create(portName, numBuffPorts, buffPortOccp);
  cmdPort = PortGeneric::create(portName, numBuffPorts, 1);

  sprintf(portName, "%s_table", name);
  tablePort = PortGeneric::create(portName, numTablePorts, tablePortOccp);

  ptr = 0;
  lastAddr = 0;

  I(current);


  if (lower_level != NULL)
  addLowerLevel(lower_level);

}






void MarkovPrefetcher::doReq(MemRequest *mreq)
  /* forward bus read {{{1 */
{
   //uint32_t paddr = mreq->getAddr() & defaultMask;
   //insertTable(paddr); //NOTE miss

  TimeDelta_t when = cmdPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleReq(mreq, when);  /* schedule req down {{{1 */
  //printf("BLAH BLAH BLAH -JASH");
}
/* }}} */

void MarkovPrefetcher::doDisp(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  //uint32_t paddr = mreq->getAddr() & defaultMask;
  //insertTable(paddr); //NOTE miss

  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleDisp(mreq, when);  /* schedule Displace (down) {{{1 */
}
/* }}} */






void MarkovPrefetcher::doReqAck(MemRequest *mreq)
  /* data is coming back {{{1 */
{
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleReqAck(mreq, when);   /* schedule reqAck up {{{1 */
}
/* }}} */


void MarkovPrefetcher::doSetState(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
//  ifHit(mreq);
  router->sendSetStateAll(mreq, mreq->getAction(), delay);  /* send setState to others, return how many {{{1 */
}
/* }}} */

void MarkovPrefetcher::doSetStateAck(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
  router->scheduleSetStateAck(mreq, delay);  /* schedule setStateAck down {{{1 */
}
/* }}} */

bool MarkovPrefetcher::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  return false;
}
/* }}} */

TimeDelta_t MarkovPrefetcher::ffread(AddrType addr)
  /* fast forward reads {{{1 */
{ 
  return delay;
}
/* }}} */

TimeDelta_t MarkovPrefetcher::ffwrite(AddrType addr)
  /* fast forward writes {{{1 */
{ 
  return delay;
}
/* }}} */




// Not associtated with BUS.CPP


//WHERE WE ACTUALLY PREFETCH!
void MarkovPrefetcher::prefetch(AddrType prefAddr, Time_t lat)
{
  uint32_t paddr = prefAddr & defaultMask;

  if(!buff->readLine(paddr)) { // it is not in the buff
    penFetchSet::iterator it = pendingFetches.find(paddr);
     if(it == pendingFetches.end()) {

      MemRequest *mreq = MemRequest::create(this, paddr, false, 0);
      router->scheduleReqAckAbs(mreq, missDelay); //Send out the prefetch!

      predictions.inc(); //used for statistics
      pendingFetches.insert(paddr);
    } 
  }
}



void MarkovPrefetcher::insertTable(AddrType addr){
  uint32_t tag = table->calcTag(addr);
  Time_t lat = 0;

  if(tag){

    tEntry = table->readLine(addr); //index of the table for this missed address

    if(tEntry){
      //if this table entry has been seen before we can start prefetching the addresses we have seen before
      lat = nextTableSlot() - globalClock; //JASH: uncertain about the lat caluculations
      prefetch(tEntry->predAddr1,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr2,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr3,lat);

      lat = nextTableSlot() - globalClock;
      prefetch(tEntry->predAddr4,lat);

    }else{ //if we have not seen this address before than we need to create an entry in the table for this miss.
      tEntry = table->fillLine(addr);
    }
//Now let us update the previous addresses information and what it will prefetch
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
