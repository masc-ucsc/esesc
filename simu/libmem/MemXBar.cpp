// Contributed by David Munday
//                Jose Renau
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
#include "MemXBar.h"

/* }}} */

MemXBar::MemXBar(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : GXBar(section, name)
  ,readHit          ("%s:readHit",         name)
  ,writeHit         ("%s:writeHit",         name)

{
  char * tmp;
  printf("building a memXbar named:%s\n",name);  
  Xbar_unXbar_balance++; //increment balance of XBars
  lower_level_banks = NULL; 

  SescConf->isInt(section, "lowerLevelBanks");
  SescConf->isInt(section, "LineSize");
  SescConf->isInt(section, "Modfactor");

  numLowerLevelBanks = SescConf->getInt(section, "lowerLevelBanks");
  LineSize           = SescConf->getInt(section, "LineSize");
  Modfactor          = SescConf->getInt(section, "Modfactor");

  SescConf->isPower2(section,"lowerLevelBanks");
  SescConf->isGT(section,"lowerLevelBanks",0);

  if(Modfactor < numLowerLevelBanks){
    printf("ERROR: XBAR: %s does not have a Modfactor(%d) bigger than the number of structures(%d) below it!\n",name,Modfactor,numLowerLevelBanks);
    exit(1);
  }

  lower_level_banks = new MemObj*[numLowerLevelBanks];

  std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevel");
  //size_t size = strlen(vPars[0]);

  for(size_t i=0;i<numLowerLevelBanks;i++) {    
    //lower_level_banks[i] = current->declareMemoryObj(section, "lowerLevel");   
    tmp = (char*)malloc(255);
    sprintf(tmp,"%s(%lu)",vPars[0],i);
    lower_level_banks[i] = current->declareMemoryObj_uniqueName(tmp,vPars[0]);         
    addLowerLevel(lower_level_banks[i]);
  }

  if(Xbar_unXbar_balance !=0){
    printf("ERROR: Crossbars and UnCrossbars are unbalanced: %d\n",Xbar_unXbar_balance);
    exit(1);
  }
    
  //free(tmp);
  I(current);
}
/* }}} */

Time_t MemXBar::nextReadSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t MemXBar::nextWriteSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t MemXBar::nextBusReadSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t MemXBar::nextPushDownSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t MemXBar::nextPushUpSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t MemXBar::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */

void MemXBar::read(MemRequest *mreq)
  /* read if splitter above L1 {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  } 
  readHit.inc(mreq->getStatsFlag());
  uint32_t pos = addrHash(mreq->getAddr(),LineSize,Modfactor,numLowerLevelBanks);  
  router->fwdBusReadPos(pos, mreq);
}
/* }}} */
void MemXBar::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  writeHit.inc(mreq->getStatsFlag());
  uint32_t pos = addrHash(mreq->getAddr(),LineSize,Modfactor,numLowerLevelBanks);
  router->fwdBusReadPos(pos, mreq);
}
/* }}} */
void MemXBar::writeAddress(MemRequest *mreq)
  /* FIXME don't know if needed in MemXBar {{{1 */
{
  mreq->ack();

  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
}
/* }}} */

void MemXBar::busRead(MemRequest *mreq)
  /* MemXBar busRead (exclusive or shared) request {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  readHit.inc(mreq->getStatsFlag());
  uint32_t pos = addrHash(mreq->getAddr(),LineSize, Modfactor,numLowerLevelBanks);
  router->fwdBusReadPos(pos, mreq);
}
/* }}} */

void MemXBar::pushDown(MemRequest *mreq)
  /* MemXBar push down (writeback or invalidate ack) {{{1 */
{  
  uint32_t pos = addrHash(mreq->getAddr(),LineSize, Modfactor,numLowerLevelBanks);
  router->fwdPushDownPos(pos, mreq);
}
/* }}} */
void MemXBar::pushUp(MemRequest *mreq)
  /* MemXBar push up (fwdBusRead ack) {{{1 */
{
  if(mreq->isHomeNode()) {
    mreq->ack();
    return;
  }
  router->fwdPushUp(mreq, 0); 
}
/* }}} */

void MemXBar::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  // broadcast the invalidate through the upper nodes
  writeHit.inc(mreq->getStatsFlag());
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), 0);
  uint32_t pos = addrHash(mreq->getAddr(),LineSize, Modfactor,numLowerLevelBanks);
  router->fwdPushDownPos(pos, mreq);
}
/* }}} */

bool MemXBar::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  uint32_t pos = addrHash(dinst->getAddr(),LineSize, Modfactor,numLowerLevelBanks);
  return router->canAcceptReadPos(pos, dinst);
}
/* }}} */

bool MemXBar::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{
  uint32_t pos = addrHash(dinst->getAddr(),LineSize, Modfactor,numLowerLevelBanks);
  return router->canAcceptWritePos(pos, dinst);
}
/* }}} */

TimeDelta_t MemXBar::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  uint32_t pos = addrHash(addr,LineSize, Modfactor,numLowerLevelBanks);
  return router->ffreadPos(pos, addr, data);
}
/* }}} */

TimeDelta_t MemXBar::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  uint32_t pos = addrHash(addr,LineSize, Modfactor,numLowerLevelBanks);
  return router->ffwritePos(pos, addr, data);
}
/* }}} */

void MemXBar::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0);
  exit(1);
}
/* }}} */

