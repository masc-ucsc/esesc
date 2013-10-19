/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by David Munday
                  Jose Renau
                 
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

