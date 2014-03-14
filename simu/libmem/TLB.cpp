#if 0
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
#include "TLB.h"
/* }}} */
#define TLB_ALWAYS_HITS 0  

TLB::TLB(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
    ,delay(SescConf->getInt(section, "hitDelay")) // this is the delay with which the mem requests will be sent 
    ,nWrite("%s_nWrite",name)
    ,nWriteAddress("%s_nWriteAddress",name)
    ,nRead("%s_nRead",name)
    ,nBusReadRead("%s_nBusReadRead",name)
    ,nBusReadWrite("%s_nBusReadWrite",name)
    ,tlbReadHit("%s:Hit",name)
    ,tlbReadMiss("%s:Miss",name)
    ,tlbMSHRcant_issue("%s:MSHRcant_issue",name)
    ,tlblowerReadHit("%s:LowerTLBHit",name)
    ,tlblowerReadMiss("%s:LowerTLBMiss",name)
    ,tlbInvalidate("%s:tlbInvalidate",name)
    ,mmuavgMissLat("%s_mmuavgMissLat",name)
    ,mmuavgMemLat("%s_mmuavgMemLat",name)
    ,avgMissLat("%s_avgMissLat",name)
    ,avgMemLat("%s_avgMemLat",name)
{
  localcall = false;
  I(current);
  SescConf->isInt(section, "hitDelay");

  SescConf->isInt(section, "numPorts");
  NumUnits_t  num         = SescConf->getInt(section, "numPorts");

  char cadena[100];
  sprintf(cadena,"Cmd%s", name);
  cmdPort                 = PortGeneric::create(cadena, num, 1);

  MemObj *lower_level     = NULL;
  lower_level             = current->declareMemoryObj(section, "lowerLevel");

  if (lower_level){
    addLowerLevel(lower_level);
    lowerCache = lower_level;
  } else {
    lowerCache = NULL;
  }

  char tmpName[512];
  sprintf(tmpName, "%s_TLB", name);
  tlbBank                 = CacheType::create(section, "", tmpName);
  uint32_t lineSize       = tlbBank->getLineSize();
  I(tlbBank->getLineSize()>1024); // Line size is the TLB page size, so make sure that it is big (4K standard?)

  const char* mshrSection = SescConf->getCharPtr(section,"MSHR");
  bmshr                   = MSHR::create(tmpName, mshrSection, lineSize);

  if(SescConf->checkCharPtr(section, "lowerTLB")){
    SescConf->isInt(section, "lowerTLB_delay");
    lowerTLBdelay = SescConf->getInt(section, "lowerTLB_delay");
    lowerTLB      = current->declareMemoryObj(section, "lowerTLB");
    } else {
    lowerTLB      = NULL;
    lowerTLBdelay = 0;
  }

}
/* }}} */

Time_t TLB::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t TLB::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t TLB::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t TLB::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot(mreq->getStatsFlag());
}
/* }}} */
Time_t TLB::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */
Time_t TLB::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return globalClock;
}
/* }}} */

void TLB::readPage1(MemRequest *mreq) 
{
  MemRequest *newreq = MemRequest::createRead(this
      ,mreq->getDInst()
      ,calcPage2Addr(mreq->getAddr())
      ,readPage2CB::create(this,mreq)
      );

  newreq->markMMU();
  router->fwdBusRead(newreq, delay);
}

void TLB::readPage2(MemRequest *mreq) 
{
  MemRequest *newreq = MemRequest::createRead(this
      ,mreq->getDInst()
      ,calcPage3Addr(mreq->getAddr())
      ,readPage3CB::create(this,mreq)
      );

  newreq->markMMU();
  router->fwdBusRead(newreq, delay);
}

void TLB::readPage3(MemRequest *mreq) 
{
  // Alamelu : Anything more to be added here?
  tlbBank->fillLine(mreq->getAddr());
  bmshr->retire(mreq->getAddr());
  //MSG("TLB::readPage3 :--------------  Remove  PC %x from the BMSHR",mreq->getAddr());

  router->fwdBusRead(mreq);
}

void TLB::read(MemRequest *mreq)
{
  nRead.inc();

  //MSG("TLB:read %llx",(mreq->getDInst())->getPC());
  if(mreq->getAddr() == 0) {
    mreq->ack(delay);
    return;
  }

  doRead(mreq,false);
}

void TLB::write(MemRequest *mreq)
{
  nWrite.inc();

  //MSG("TLB:write %llx",(mreq->getDInst())->getPC());
  if(mreq->getAddr() == 0) {
    mreq->ack(delay);
    return;
  }
  doRead(mreq,false);

}

/* }}} */
void TLB::writeAddress(MemRequest *mreq)
{
  nWriteAddress.inc();

    mreq->ack(delay);
    return;

  //MSG("TLB:writeAddress %llx",(mreq->getDInst())->getPC());
  I(lowerCache != NULL);
  // The processor may want to write to the L1 cache. The first TLB will be above the cache. 
  lowerCache->writeAddress(mreq); //FIXME: RISKY.
}
/* }}} */

void TLB::busRead(MemRequest *mreq)
{
  if (mreq->isWrite()) {
    nBusReadWrite.inc();
  }else{
    nBusReadRead.inc();
  }

  //MSG("TLB:busRead %llx",(mreq->getDInst())->getPC());
  doRead(mreq,false);
}


void TLB::doRead(MemRequest *mreq,bool retrying) {
#if TLB_ALWAYS_HITS 
    mmuavgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
    avgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());

    if (retrying) {
      bmshr->retire(mreq->getAddr());
    }

    tlbReadHit.inc(mreq->getStatsFlag());
    router->fwdBusRead(mreq,delay);
    return;
#endif

  Line *l = tlbBank->readLine(mreq->getAddr());
  if (l) { //TLB Hit
    mmuavgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
    avgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());

    if (retrying) {
      bmshr->retire(mreq->getAddr());
    }

    tlbReadHit.inc(mreq->getStatsFlag());
    router->fwdBusRead(mreq,delay);
    return;
  }

  // TLB Miss
  tlbReadMiss.inc(mreq->getStatsFlag());

  if (lowerTLB != NULL){
    //Check the lowerTLB for a miss. 
    if (lowerTLB->checkL2TLBHit(mreq) == true){
      tlbBank->fillLine(mreq->getAddr());
      tlblowerReadHit.inc(mreq->getStatsFlag());
      router->fwdBusRead(mreq,lowerTLBdelay);
      return;
    }

    tlblowerReadMiss.inc(mreq->getStatsFlag());
  }

  if(!retrying && !bmshr->canIssue(mreq->getAddr())) {
    //MSG("TLB::busRead:Waiting to add PC %x by the BMSHR HomeNode(%s)",mreq->getAddr(),mreq->getHomeNode());
    tlbMSHRcant_issue.inc(mreq->getStatsFlag());
    CallbackBase *cb  = doReadCB::create(this, mreq,true);
    bmshr->addEntry(mreq->getAddr(), cb);
    return;
  }

  //MSG("TLB::busRead:Adding PC %x to the BMSHR",mreq->getAddr());
  bmshr->addEntry(mreq->getAddr());

  MemRequest *newreq = MemRequest::createRead(this
        ,mreq->getDInst()
        ,calcPage1Addr(mreq->getAddr())
        ,readPage1CB::create(this,mreq)
        );
  router->fwdBusRead(newreq,delay);
}

void TLB::pushDown(MemRequest *mreq)
{
  I(mreq->isInvalidate() || mreq->isWriteback());


  router->fwdPushDown(mreq);
}

void TLB::pushUp(MemRequest *mreq)
{
  //MSG("TLB:pushUp %llx",(mreq->getDInst())->getPC());
  if (mreq->isMMU()) {
    mmuavgMissLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
    mmuavgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  }else{
    avgMissLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
    avgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  }

  if (mreq->isInvalidate()){
    tlbInvalidate.inc(); 
  }

  if(!mreq->isHomeNode()) {
    router->fwdPushUp(mreq);
    return;
  }

  mreq->ack();
}

void TLB::invalidate(MemRequest *mreq)
{
  I(!mreq->isHomeNode());

  // broadcast the invalidate through the upper nodes
  router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
  router->fwdPushDown(mreq, 1); // invalidate ack
}

bool TLB::canAcceptRead(DInst *dinst) const
{
  AddrType addr = dinst->getAddr();
  if (!bmshr->canAccept(addr))
    return false;
  return lowerCache->canAcceptRead(dinst);
}

bool TLB::canAcceptWrite(DInst *dinst) const
{
  AddrType addr = dinst->getAddr();
  if (!bmshr->canAccept(addr))
    return false;
  return lowerCache->canAcceptWrite(dinst);
}

TimeDelta_t TLB::ffread(AddrType addr, DataType data)
{ 
  tlbBank->fillLine(addr);
  return router->ffread(addr,data) + delay;
}

TimeDelta_t TLB::ffwrite(AddrType addr, DataType data)
{ 
  tlbBank->fillLine(addr);
  return router->ffwrite(addr,data) + delay;
}

void TLB::ffinvalidate(AddrType addr, int32_t ilineSize)
{ 
  //I(0);
  // FIXME: router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}

// TLB direct requests  
bool TLB::checkL2TLBHit(MemRequest *mreq) {
  AddrType addr = mreq->getAddr();
  Line *l = tlbBank->readLine(addr);
  if (l) {
    //mreq->ack(delay); // Alamelu: Should I?? 
    return true;
  }
  return false;
}
#endif
