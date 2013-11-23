// Contributed by David Munday
//                Jose Renau
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

