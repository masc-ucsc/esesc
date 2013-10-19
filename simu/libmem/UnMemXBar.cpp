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
#include "UnMemXBar.h"
/* }}} */

UnMemXBar::UnMemXBar(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : GXBar(section, name)
{
  printf("building an UnMemXbar named:%s\n",name);
  Xbar_unXbar_balance--; //decrement balance of XBars
  lower_level = NULL; 

  SescConf->isInt(section, "lowerLevelBanks");
  SescConf->isPower2(section,"lowerLevelBanks");

  SescConf->isInt(section, "LineSize");
  SescConf->isInt(section, "Modfactor");

  numLowerLevelBanks = SescConf->getInt(section, "lowerLevelBanks");    
  LineSize           = SescConf->getInt(section, "LineSize");
  Modfactor          = SescConf->getInt(section, "Modfactor");

  if(Modfactor < numLowerLevelBanks){
    printf("ERROR: UNXBAR: %s does not have a Modfactor(%d) bigger than the number of structures(%d) below it!\n",name,Modfactor,numLowerLevelBanks);
    exit(1);
  }

  std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevel");
  size_t size = strlen(vPars[0]);
  char *tmp = (char*)malloc(size + 6);

  sprintf(tmp,"%s(0)",vPars[1]);
  //lower_level = current->declareMemoryObj(section, "lowerLevel");   
  lower_level = current->declareMemoryObj_uniqueName(tmp,vPars[0]);         
  addLowerLevel(lower_level);
    
  I(current);
}
/* }}} */

Time_t UnMemXBar::nextReadSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t UnMemXBar::nextWriteSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t UnMemXBar::nextBusReadSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t UnMemXBar::nextPushDownSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t UnMemXBar::nextPushUpSlot(const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t UnMemXBar::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */

void UnMemXBar::read(MemRequest *mreq)
  /* read if splitter above L1 {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  } 

  router->fwdBusRead(mreq);
}
/* }}} */
void UnMemXBar::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  router->fwdBusRead(mreq);
}
/* }}} */
void UnMemXBar::writeAddress(MemRequest *mreq)
  /* FIXME don't know if needed in MemXBar {{{1 */
{
  I(0);

  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
}
/* }}} */

void UnMemXBar::busRead(MemRequest *mreq)
  /* MemXBar busRead (exclusive or shared) request {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  router->fwdBusRead(mreq);
}
/* }}} */

void UnMemXBar::pushDown(MemRequest *mreq)
  /* MemXBar push down (writeback or invalidate ack) {{{1 */
{  
  router->fwdPushDown(mreq);
}
/* }}} */
void UnMemXBar::pushUp(MemRequest *mreq)
  /* MemXBar push up (fwdBusRead ack) {{{1 */
{
  uint32_t pos = addrHash(mreq->getAddr(),LineSize,Modfactor,numLowerLevelBanks);
  router->fwdPushUpPos(pos, mreq, 0); 
}
/* }}} */

void UnMemXBar::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  I(0);
  // FIXME: Something like this:
  // uint32_t pos = addrHash(mreq->getAddr(),numLowerLevelBanks); 
  // router->fwdPushUpPos(pos, mreq, 0); 
 
  // broadcast the invalidate through the upper nodes
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), 0);
}
/* }}} */

bool UnMemXBar::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  return router->canAcceptRead(dinst);
}
/* }}} */

bool UnMemXBar::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{
  return router->canAcceptWrite(dinst);
}
/* }}} */

TimeDelta_t UnMemXBar::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  return router->ffread(addr, data);
}
/* }}} */

TimeDelta_t UnMemXBar::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  return router->ffwrite(addr, data);
}
/* }}} */

void UnMemXBar::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  //routerarray[addrHash(addr)]->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
  I(0);
  exit(1);
}
/* }}} */

