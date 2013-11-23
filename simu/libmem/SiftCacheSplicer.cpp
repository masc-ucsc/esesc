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
