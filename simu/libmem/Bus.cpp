/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Luis Ceze

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
#include "Bus.h"
/* }}} */

Bus::Bus(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
  ,delay(SescConf->getInt(section, "delay"))
  ,avgFree("%s_avgFree",name)
{
  //busyUpto = 0;
  MemObj *lower_level = NULL;

  SescConf->isInt(section, "numPorts");
  SescConf->isInt(section, "portOccp");
  SescConf->isInt(section, "delay");

  NumUnits_t  num = SescConf->getInt(section, "numPorts");
  TimeDelta_t occ = SescConf->getInt(section, "portOccp");

  char cadena[100];
  sprintf(cadena,"Data%s", name);
  dataPort = PortGeneric::create(cadena, num, occ);
  sprintf(cadena,"Cmd%s", name);
  cmdPort  = PortGeneric::create(cadena, num, 1);

  I(current);
  lower_level = current->declareMemoryObj(section, "lowerLevel");   
  if (lower_level)
    addLowerLevel(lower_level);

  isMemoryBus = false; // DELETE ME
}
/* }}} */

Time_t Bus::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0);
  return globalClock;
}
/* }}} */
Time_t Bus::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0);
  return globalClock;
}
/* }}} */
Time_t Bus::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t Bus::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t Bus::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return dataPort->nextSlot();
}
/* }}} */
Time_t Bus::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */

void Bus::read(MemRequest *mreq)
  /* no read in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */
void Bus::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */
void Bus::writeAddress(MemRequest *mreq)
  /* no writeAddress in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */

void Bus::busRead(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  if(isMemoryBus) {

    mreq->front = NULL;
    mreq->back  = NULL;
    int chID = DRAM[0]->getChannelID(mreq);
    DRAM[chID]->arrive(mreq);

    return;
  }
  router->fwdBusRead(mreq);
#if 0
  if (busyUpto<globalClock && busyUpto)
    avgFree.sample(globalClock-busyUpto);

  busyUpto = dataPort->calcNextSlot();
#endif
}
/* }}} */
void Bus::pushDown(MemRequest *mreq)
  /* push down {{{1 */
{
  if (mreq->isInvalidate()) {
    I(!isMemoryBus);
        mreq->decPending();
    if (!mreq->hasPending()) {
      MemRequest *parent = mreq->getParent();
      router->fwdPushDown(parent);
    }

    mreq->destroy();
    return;
  }
  if(isMemoryBus) {

    mreq->front = NULL;
    mreq->back  = NULL;
    int chID = DRAM[0]->getChannelID(mreq);
    DRAM[chID]->arrive(mreq);

    return;
  }

  I(mreq->isWriteback());

  router->fwdPushDown(mreq);
}
/* }}} */
void Bus::pushUp(MemRequest *mreq)
  /* push up {{{1 */
{
  I(!isMemoryBus);

  router->fwdPushUp(mreq);
}
/* }}} */
void Bus::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  I(!isMemoryBus);
  // broadcast the invalidate through the upper nodes
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}
/* }}} */

bool Bus::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  return true;
}
/* }}} */

bool Bus::canAcceptWrite(DInst *disnt) const
/* always can accept reads {{{1 */
{
  return true;
}
/* }}} */

TimeDelta_t Bus::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  //I(0);
  return router->ffread(addr,data) + delay;
}
/* }}} */

TimeDelta_t Bus::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  //I(0);
  return router->ffwrite(addr,data) + delay;
}
/* }}} */

void Bus::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  //I(0);
  // FIXME: router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}
/* }}} */

