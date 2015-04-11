#if 0
// Contributed by Jose Renau
//                Basilio Fraguela
//                Luis Ceze
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
  // return max(rdPort->nextSlot(mreq->getStatsFlag()),nextBankSlot()); 
}
/* }}} */

Time_t Splitter::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
// Bus style or cache style?
  // return max(wrPort->nextSlot(mreq->getStatsFlag()),nextBankSlot()); 
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
  return lower_level->canAcceptRead(dinst);
}
/* }}} */

bool Splitter::canAcceptWrite(DInst *dinst) const
/* always can accept reads {{{1 */
{

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

#endif

