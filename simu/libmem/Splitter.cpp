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
#include "Splitter.h"
/* }}} */

Splitter::Splitter(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
{
  //routerLeft = new MRouter(this);
  //lower_level = NULL;
  //lower_levelLeft = NULL;

  I(current);
  SescConf->isInt(section, "numBanks");

  numBanks = SescConf->getInt(section, "numBanks");
  SescConf->isPower2(section,"numBanks");
  numBankMask = numBanks -1;

  scratchPort = (PortGeneric **)malloc(sizeof(PortGeneric *)*numBanks);
  char cadena[1024];
  for(size_t i=0;i<numBanks;i++) {
    sprintf(cadena,"%s_%zu",name,i);
    scratchPort[i] = PortGeneric::create(cadena, 1, 1);
  }

  lower_level = current->declareMemoryObj(section, "lowerLevel");   
  I(lower_level);
  addLowerLevel(lower_level);

// FERMI specific...
  lower_levelLeft = current->declareMemoryObj(section, "lowerLevelLeft");
  I(lower_levelLeft);
  addLowerLevel(lower_levelLeft);
}
/* }}} */

Time_t Splitter::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(rdPort->nextSlot(),nextBankSlot()); 
}
/* }}} */

Time_t Splitter::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(wrPort->nextSlot(),nextBankSlot()); 
}
/* }}} */
Time_t Splitter::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t Splitter::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t Splitter::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t Splitter::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */

void Splitter::read(MemRequest *mreq)
  /* read if splitter above L1 {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
#ifdef ENABLE_CUDA
  if(mreq->isSharedAddress()){ 
    Time_t when = scratchPort[(mreq->getAddr()>>3)&numBankMask]->nextSlot();
    //routerLeft->fwdBusRead(mreq, when-globalClock); 
    router->fwdBusReadPos(1,mreq, when-globalClock);
    return;
  }
#endif

  router->fwdBusRead(mreq); 
}

/* }}} */
void Splitter::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  //I(0); // Cannot be top level
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
#ifdef ENABLE_CUDA
  if(mreq->isSharedAddress()){ 
    Time_t when = scratchPort[(mreq->getAddr()>>3)&numBankMask]->nextSlot();
    //routerLeft->fwdBusRead(mreq, when-globalClock); 
    router->fwdBusReadPos(1, mreq, when-globalClock);
    return;
  }
#endif
  router->fwdBusRead(mreq); 
}
/* }}} */
void Splitter::writeAddress(MemRequest *mreq)
  /* FIXME don't know if needed in Splitter {{{1 */
{
  I(0); // Do we need this in the splitter?
  mreq->ack(1);
}
/* }}} */

void Splitter::busRead(MemRequest *mreq)
  /* Splitter busRead (exclusive or shared) request {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
#ifdef ENABLE_CUDA
  if(mreq->isSharedAddress()){ 
    Time_t when = scratchPort[(mreq->getAddr()>>3)&numBankMask]->nextSlot();
    //routerLeft->fwdBusRead(mreq, when-globalClock); 
    router->fwdBusReadPos(1, mreq, when-globalClock); 
    return;
  }
#endif
  router->fwdBusRead(mreq); 
}

/* }}} */
void Splitter::pushDown(MemRequest *mreq)
  /* Splitter push down (writeback or invalidate ack) {{{1 */
{  
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  if (mreq->isHomeNode()){
    mreq->ack();
    return;
  }

#ifdef ENABLE_CUDA
  if(mreq->isSharedAddress()){ 
    Time_t when = scratchPort[(mreq->getAddr()>>3)&numBankMask]->nextSlot();
    router->fwdPushDownPos(1, mreq, when-globalClock);
    return;
  }
#endif
  router->fwdPushDown(mreq); 
}

/* }}} */
void Splitter::pushUp(MemRequest *mreq)
  /* Splitter push up (fwdBusRead ack) {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  if (mreq->isHomeNode()){
    mreq->ack();
    return;
  }

#ifdef ENABLE_CUDA
  if(mreq->isSharedAddress()){ 
    Time_t when = scratchPort[(mreq->getAddr()>>3)&numBankMask]->nextSlot();
    //router->fwdPushUpPos(1,mreq, when-globalClock); // Note, you don't need a pos here, because the splitter sends both shared and global addresses to the same upper level.
    router->fwdPushUp(mreq, when-globalClock); 
    return;
  }
#endif
  router->fwdPushUp(mreq); 
}
/* }}} */

void Splitter::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  router->fwdPushDown(mreq, 0); // invalidate ack with nothing
}
/* }}} */

bool Splitter::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
#ifdef ENABLE_CUDA
  if(dinst->isSharedAddress())
    return true;
#endif

  return lower_level->canAcceptRead(dinst);
}
/* }}} */

bool Splitter::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{
#ifdef ENABLE_CUDA
  if(dinst->isSharedAddress())
    return true;
#endif

  return lower_level->canAcceptWrite(dinst);
}
/* }}} */

TimeDelta_t Splitter::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  I(0); // No warmup with GPU
  return router->ffread(addr,data);
}
/* }}} */

TimeDelta_t Splitter::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  I(0); // No warmup with GPU
  return router->ffwrite(addr,data);
}
/* }}} */

void Splitter::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0); // No warmup with GPU
  // FIXME: router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}
/* }}} */

