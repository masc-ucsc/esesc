/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Alamelu Sankaranarayanan

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
#include "SiftCacheSplicer.h"
/* }}} */

SiftCacheSplicer::SiftCacheSplicer(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
{
  if (SescConf->checkInt(section, "siftcaches"))
    num_pes = SescConf->getInt(section,"siftcaches");
  else {
    MSG("\nUnspecified number of Sift Caches for the SiftCacheSplicer..");
    SescConf->notCorrect();
  }

  lower_level = current->declareMemoryObj(section, "lowerLevel");
  addLowerLevel(lower_level);
}
/* }}} */

Time_t SiftCacheSplicer::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(rdPort->nextSlot(),nextBankSlot()); 
}


/* }}} */
Time_t SiftCacheSplicer::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(wrPort->nextSlot(),nextBankSlot()); 
}


/* }}} */
Time_t SiftCacheSplicer::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplicer::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplicer::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplicer::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
void SiftCacheSplicer::read(MemRequest *mreq)
{
  I(0); // Splicer can NEVER be a top level.

  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  router->fwdBusRead(mreq); 
}


/* }}} */
void SiftCacheSplicer::write(MemRequest *mreq)
{
  I(0); // Splicer can NEVER be a top level.
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  router->fwdBusRead(mreq); 
}

/* }}} */
void SiftCacheSplicer::writeAddress(MemRequest *mreq)
  /* FIXME don't know if needed in SiftCacheSplicer {{{1 */
{
  I(0);
  if(mreq->getAddr() == 0) {
    mreq->ack(1);
  }
}


/* }}} */
void SiftCacheSplicer::busRead(MemRequest *mreq)
  /* SiftCacheSplicer busRead (exclusive or shared) request {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }
  router->fwdBusRead(mreq);
}


/* }}} */
void SiftCacheSplicer::pushDown(MemRequest *mreq)
  /* SiftCacheSplicer push down (writeback or invalidate ack) {{{1 */
{  
  router->fwdPushDown(mreq); // Will always have 1 down.
}


/* }}} */
void SiftCacheSplicer::pushUp(MemRequest *mreq)
  /* SiftCacheSplicer push up (fwdBusRead ack) {{{1 */
{
  DInst* dinst = mreq->getDInst();
  uint32_t pe_id = dinst->getPE();
  I(pe_id< num_pes);
  router->fwdPushUpPos(pe_id, mreq, 1); 
}


/* }}} */
void SiftCacheSplicer::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  I(0);
  for (uint32_t pe_id = 0; pe_id < 32; pe_id++){
    router->fwdPushDownPos(pe_id,mreq, 0); // invalidate ack with nothing
  }
}


/* }}} */
bool SiftCacheSplicer::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  return router->canAcceptRead(dinst);
}


/* }}} */
bool SiftCacheSplicer::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{
  return router->canAcceptWrite(dinst);
}


/* }}} */
TimeDelta_t SiftCacheSplicer::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  I(0); // No warmup with GPU
  return 0;
}


/* }}} */
TimeDelta_t SiftCacheSplicer::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  I(0); // No warmup with GPU
  return 0;
}


/* }}} */
void SiftCacheSplicer::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0); // No warmup with GPU
}
