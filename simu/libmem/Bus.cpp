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
#include "Bus.h"
/* }}} */

Bus::Bus(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
  ,delay(SescConf->getInt(section, "delay"))
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
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t Bus::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t Bus::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return dataPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t Bus::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
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

