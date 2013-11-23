// Contributed by Jose Renau
//                Alamelu Sankaranarayanan
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
