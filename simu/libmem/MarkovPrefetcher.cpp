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
#include "CacheCore.h"

MarkovPrefetcher::MarkovPrefetcher(MemorySystem* current,const char *section,const char *name)
   // {{{1 constructor
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
// 1}}}

void MarkovPrefetcher::doReq(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  uint32_t paddr = mreq->getAddr() & defaultMask;
  insertTable(paddr); //NOTE miss

  TimeDelta_t when = cmdPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleReq(mreq, when); 
}
/* }}} */

void MarkovPrefetcher::doDisp(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  uint32_t paddr = mreq->getAddr() & defaultMask;
  insertTable(paddr); //NOTE miss

  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleDisp(mreq, when);  
}
/* }}} */

void MarkovPrefetcher::doReqAck(MemRequest *mreq)
  /* data is coming back {{{1 */
{
  if (mreq->isHomeNode()) {
    predictions.inc();
    pendingFetches.insert(mreq->getAddr());
    return;
  }
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag())+delay;
  router->scheduleReqAck(mreq, when);   
}
/* }}} */

void MarkovPrefetcher::doSetState(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
  router->sendSetStateAll(mreq, mreq->getAction(), delay); 
}
/* }}} */

void MarkovPrefetcher::doSetStateAck(MemRequest *mreq)
  /* forward set state to all the upper nodes {{{1 */
{
  router->scheduleSetStateAck(mreq, delay);
}
/* }}} */

bool MarkovPrefetcher::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  return false;
}
/* }}} */

void MarkovPrefetcher::tryPrefetch(AddrType addr, bool doStats)
  /* drop prefetch {{{1 */
{ 
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

void MarkovPrefetcher::prefetch(AddrType prefAddr, Time_t lat)
{
  uint32_t paddr = prefAddr & defaultMask;

  if(buff->readLine(paddr))
    return;

  penFetchSet::iterator it = pendingFetches.find(paddr);
  if(it == pendingFetches.end()) {
    MemRequest *mreq = MemRequest::createReqRead(this, paddr, false); // FIXME, not in the stats
    router->scheduleReqAckAbs(mreq, missDelay); //Send out the prefetch!
  }
}

void MarkovPrefetcher::insertTable(AddrType addr)
{
  static int flag_first = false;
  int i, j;
  if (flag_first == false){
    for(i = 0; i < num_rows; i++){
      for(j=0;j <num_columns; j++){
        Markov_Table[i][0] = 0;
      }
    }
    flag_first = true;
  }

  static int prev_row = 0;
  int row = -1;

  Time_t lat = 0;
  //check if current address is in the table.
  for(i = 0; i < num_rows; i++){
    if(Markov_Table[i][0] == addr){
      row = i;
      break;
    }else if(Markov_Table[i][0] == 0){ //found a blank spot in the table
      Markov_Table[i][0] = addr;
      printf("HELLO");
      break;
    }
  }

  if(row != -1){ //if we hit the table
    lat = nextTableSlot() - globalClock;
    if(Markov_Table[row][1] != 0)
      prefetch(Markov_Table[row][1],lat);
    if(Markov_Table[row][2] != 0)
      prefetch(Markov_Table[row][2],lat);
  }else{
    printf("FIX ME: Not enough table space");
  }

  if(Markov_Table[prev_row][1] == 0){
    Markov_Table[prev_row][1] = addr;
  }else if(Markov_Table[prev_row][2] == 0){
    Markov_Table[prev_row][2] = addr;
  }else{
    Markov_Table[prev_row][1] = Markov_Table[prev_row][2];
    Markov_Table[prev_row][2] = addr;
  }

  prev_row = row;
}

