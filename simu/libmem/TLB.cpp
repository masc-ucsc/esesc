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
#include "TLB.h"
/* }}} */
#define TLB_ALWAYS_HITS 0  

TLB::TLB(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
    ,delay(SescConf->getInt(section, "hitDelay")) // this is the delay with which the mem requests will be sent 
    ,avgFree("%s_avgFree",name)
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
  return cmdPort->nextSlot();
}
/* }}} */
Time_t TLB::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t TLB::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t TLB::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
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

