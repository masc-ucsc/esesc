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
#include "SiftCacheSplitter.h"
/* }}} */

SiftCacheSplitter::SiftCacheSplitter(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
{
  if (SescConf->checkInt(section, "siftcaches"))
    num_pes = SescConf->getInt(section,"siftcaches");
  else {
    MSG("\nUnspecified number of Sift Caches..");
    SescConf->notCorrect();
  }
  siftcache         = new MemObj* [num_pes];

  for (uint32_t pe = 0; pe < num_pes; pe++){
    siftcache[pe]        = current->declareMemoryObj(section, "lowerLevel");   
    I(siftcache[pe]);
    addLowerLevel(siftcache[pe]);
  }

}
/* }}} */

Time_t SiftCacheSplitter::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(rdPort->nextSlot(),nextBankSlot()); 
}


/* }}} */
Time_t SiftCacheSplitter::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(wrPort->nextSlot(),nextBankSlot()); 
}


/* }}} */
Time_t SiftCacheSplitter::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplitter::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplitter::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
Time_t SiftCacheSplitter::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}


/* }}} */
void SiftCacheSplitter::read(MemRequest *mreq)
  /* read if SiftCacheSplitter above L1 {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  uint32_t pe_id = (mreq->getDInst())->getPE();
  
  I(pe_id < num_pes);
  router->fwdBusReadPos(pe_id,mreq); 
  //FIXME : Do we need to add a delay here?
}


/* }}} */
void SiftCacheSplitter::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  uint32_t pe_id = (mreq->getDInst())->getPE();
  
  I(pe_id< num_pes);
  router->fwdBusReadPos(pe_id,mreq); 
  //FIXME : Do we need to add a delay here?
}

/* }}} */
void SiftCacheSplitter::writeAddress(MemRequest *mreq)
  /* FIXME don't know if needed in SiftCacheSplitter {{{1 */
{
  mreq->ack(1);
}

/* }}} */
void SiftCacheSplitter::busRead(MemRequest *mreq)
  /* SiftCacheSplitter busRead (exclusive or shared) request {{{1 */
{
  I(0); // SiftCacheSplitter should be the top level
}


/* }}} */
void SiftCacheSplitter::pushDown(MemRequest *mreq)
  /* SiftCacheSplitter push down (writeback or invalidate ack) {{{1 */
{  
  I(0); // SiftCacheSplitter should be the top level
}


/* }}} */
void SiftCacheSplitter::pushUp(MemRequest *mreq)
  /* SiftCacheSplitter push up (fwdBusRead ack) {{{1 */
{
// In case we want to place the SiftCacheSplitter between the core and DL1
  I(mreq->isHomeNode());
  mreq->ack();
}


/* }}} */
void SiftCacheSplitter::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  I(0);
  for (uint32_t pe_id = 0; pe_id < num_pes; pe_id++){
    router->fwdPushDownPos(pe_id,mreq, 0); // invalidate ack with nothing
  }
}


/* }}} */
bool SiftCacheSplitter::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  uint32_t pe_id = dinst->getPE();
  I(pe_id< num_pes);
  return siftcache[pe_id]->canAcceptRead(dinst);
}


/* }}} */
bool SiftCacheSplitter::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{
  uint32_t pe_id = dinst->getPE();
  I(pe_id< num_pes);
  return siftcache[pe_id]->canAcceptWrite(dinst);
}


/* }}} */
TimeDelta_t SiftCacheSplitter::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  I(0); // No warmup with GPU
  return 0;
}


/* }}} */
TimeDelta_t SiftCacheSplitter::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  I(0); // No warmup with GPU
  return 0;
}


/* }}} */
void SiftCacheSplitter::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0); // No warmup with GPU
}


/* }}} */
