#if 0
// Contributed by Jose Renau
//                David Munday
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <iostream>

#include "nanassert.h"

#include "SescConf.h"
#include "MemorySystem.h"
#include "VPC.h"
#include "MSHR.h"
#include "GProcessor.h"

#define ALLOW_WRITEMISSES 0
#define ALLOW_WB_DISPLACE 0

LSQVPC *VPC::replayCheckLSQ=0; //declare the static instance of the replayCheckLSQ across all VPCs.
std::vector<VPC*> VPC::VPCvec;

 
/* }}} */
VPC::VPC(MemorySystem *gms, const char *section, const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
  ,wcb(SescConf->getInt(section,"WCBsize"),log2i(SescConf->getInt(section,"bsize")),name)// entries, offset
  ,wcb_lineSize(SescConf->getInt(section,"bsize"))
  ,hitDelay  (SescConf->getInt(section,"hitDelay"))
  ,missDelay (SescConf->getInt(section,"missDelay"))
  ,ms        (gms)
    // wcb  stats
  ,wcbHit          ("P(%d):wcbHit",       gms->getID())
  ,wcbMiss         ("P(%d):wcbMiss",      gms->getID())

  ,wcbPass         ("%s:wcbPass",         name)
  ,wcbFail         ("%s:wcbFail",         name)

  ,vpcPass         ("%s:vpcPass",         name)
  ,vpcFail         ("%s:vpcFail",         name)
  ,vpcOverride     ("%s:vpcOverride",     name)

    // read stats
  ,readHit         ("%s:readHit",         name)
  ,readMiss        ("%s:readMiss",        name)
  ,readHalfMiss    ("%s:readHalfMiss",    name)
    // write stats
  ,writeHit        ("%s:writeHit",        name)
  ,writeMiss       ("%s:writeMiss",       name)
  ,writeHalfMiss   ("%s:writeHalfMiss",   name)
  ,writeBack       ("%s:writeBack",       name)
  ,writeExclusive  ("%s:writeExclusive",  name)
    // other stats
  ,lineFill        ("%s:lineFill",        name)
  ,avgMissLat      ("%s_avgMissLat",      name)
  ,avgMemLat       ("%s_avgMemLat",       name)
  ,invalidateHit   ("%s_invalidateHit",   name)
  ,invalidateMiss  ("%s_invalidateMiss",  name)
  ,replayInflightStore("%s_replayInflightStore",       name)
  ,VPCreplayXcoreStore("%s_VPCreplayXcoreStore",       name)
{
  if (replayCheckLSQ==0)
    replayCheckLSQ = new LSQVPC;

  printf("DEBUG: VPC constructor invoked\n");
  const char* mshrSection = SescConf->getCharPtr(section,"MSHR");

  char tmpName[512];

  sprintf(tmpName, "%s", name);
  VPCBank   = VPCType::create(section, "", tmpName);
  lineSize  = VPCBank->getLineSize();
  mshr      = MSHR::create(tmpName, mshrSection, lineSize);

  I(gms);

  // BANK
  int numPorts = SescConf->getInt(section, "bkNumPorts");
  int portOccp = SescConf->getInt(section, "bkPortOccp");
  sprintf(tmpName, "%s_bk", name);
  bkPort = PortGeneric::create(tmpName, numPorts, portOccp);

  // READ 
  numPorts = SescConf->getInt(section, "rdNumPorts");
  portOccp = SescConf->getInt(section, "rdPortOccp");
  sprintf(tmpName, "%s_rd", name);
  rdPort = PortGeneric::create(tmpName, numPorts, portOccp);

  // WRITE 
  numPorts = SescConf->getInt(section, "wrNumPorts");
  portOccp = SescConf->getInt(section, "wrPortOccp");
  sprintf(tmpName, "%s_wr", name);
  wrPort = PortGeneric::create(tmpName, numPorts, portOccp);

  // Low-Level 
  numPorts = SescConf->getInt(section, "llNumPorts");
  portOccp = SescConf->getInt(section, "llPortOccp");
  sprintf(tmpName, "%s_ll", name);
  ctPort = PortGeneric::create(tmpName, numPorts, portOccp);

  SescConf->isInt(section, "hitDelay");
  SescConf->isInt(section, "missDelay");

  maxPendingWrites = SescConf->getInt(section, "maxWrites");
  if(maxPendingWrites == 0)
    maxPendingWrites = 32768;
  maxPendingReads = SescConf->getInt(section, "maxReads");
  if(maxPendingReads == 0)
    maxPendingReads = 32768;

  pendingReads = 0;
  pendingWrites = 0; 

  lower_level = gms->declareMemoryObj(section, "lowerLevel");
  if(lower_level)
    addLowerLevel(lower_level);

  mispredictions = 0;
  committed_at_last_replay = 0;
  wcb_replays = 0;
  vpc_replays = 0;
  skips = 0;  
  VPCvec.push_back(this); //push a pointer to this VPC onto the global array of VPC pointers
}
/* }}} */

VPC::~VPC()
/* destructor {{{1 */
{
  VPCBank->destroy();
}
/* }}} */

void VPC::displaceLine(AddrType addr, MemRequest *mreq, bool modified)
/* displace a line from the cache. writeback if necessary {{{1 */
{
  I(0);
  I(addr != mreq->getAddr()); // addr is the displace address, mreq is the trigger
}
/* }}} */

VPC::VPCLine* VPC::allocateLine(AddrType addr, MemRequest *mreq)
/* find a new cacheline for addr (it can return 0) {{{1 */
{
  I(VPCBank->findLineDebug(addr) == 0);
  VPCLine *l = VPCBank->fillLine(addr);
  lineFill.inc(mreq->getStatsFlag());  
  I(l); // Ignore lock guarantees to find line  
  return l;
}
/* }}} */

void VPC::doWriteback(AddrType addr)
/* NOT FOR VPC: write back a line {{{1 */
{
  I(0);
}
/* }}} */

Time_t VPC::nextReadSlot(const MemRequest *mreq)  
/*NOT FOR VPC: get next free slot {{{1 */
{ 
  //I(0);
  return max(rdPort->nextSlot(mreq->getStatsFlag()),nextBankSlot(mreq->getStatsFlag())); 
}
/* }}} */
Time_t VPC::nextWriteSlot(const MemRequest *mreq) 
/*NOT FOR VPC: get next free slot {{{1 */
{ 
  //I(0);
  //return max(wrPort->nextSlot(),nextBankSlot()); 
  return max(rdPort->nextSlot(mreq->getStatsFlag()),nextBankSlot(mreq->getStatsFlag())); 
}
/* }}} */
Time_t VPC::nextBusReadSlot(const MemRequest *mreq)
/*NOT FOR VPC: get next free slot {{{1 */
{
  I(0);
  return nextReadSlot(mreq->getAddr(), mreq->getStatsFlag());
}
/* }}}*/
Time_t VPC::nextPushDownSlot(const MemRequest *mreq)
/* get next free slot {{{1 */
{
  I(0);
  return nextCtrlSlot(mreq->getAddr(), mreq->getStatsFlag());
}
/* }}}*/
Time_t VPC::nextPushUpSlot(const MemRequest *mreq)
/*NOT FOR VPC: get next free slot {{{1 */
{
   return nextCtrlSlot(mreq->getAddr(), mreq->getStatsFlag());
}
/* }}}*/
Time_t VPC::nextInvalidateSlot(const MemRequest *mreq)
/*NOT FOR VPC: get next free slot {{{1 */
{
  return globalClock; // No timing for invalidate
}
/* }}}*/

void VPC::replayCheckLSQ_removeStore(DInst *dinst){
  replayCheckLSQ->remove(dinst);
}

void VPC::replayflush() {
  std::vector<AddrType> tags;
  wcb.clear(tags);  //WCB flash clear everything      
  for(uint32_t i=0;i<tags.size();i++){
    VPCLine *l = VPCBank->readLine(tags[i]);
    if (l) 
      l->invalidate();
  }
}

void VPC::checkSetXCoreStore(AddrType addr){
  wcb.checkSetXCoreStore(addr);  
  VPCLine *l = VPCBank->readLine(addr);
  if(l!=0) ///line exists in VPC
    l->setXCoreStore(addr);
}

void VPC::updateXCoreStores(AddrType addr){
  for(uint32_t i=0; i<VPCvec.size(); i++){ //check if xCore VPC sharing possible
    if(i==ms->getID())
      continue;
    VPCvec[i]->checkSetXCoreStore(addr);    
  }
}



bool VPC::probe(DInst * dinst){ //return true if data found in VPC
  bool correct = false;
  if (wcb.loadCheck(dinst,correct)) //WCB first
    return correct;
  AddrType addr = dinst->getAddr();
  VPCLine *l = VPCBank->readLine(addr);
  if(l==0)
    return false; //wasn't in WCB or in the VPC
  DataType  check_data;
  check_data = l->getData(addr);
  correct   = (check_data == dinst->getData());
  return correct; 
}

void VPC::doRead(MemRequest *mreq, bool retrying)
/* processor issued read {{{1 */
{
  DInst *dinst = mreq->getDInst();
  AddrType addr = dinst->getAddr();
  //AddrType PC = dinst->getPC(); 

  I(dinst->getInst()->isLoad());
  I(mreq->isHomeNode());
  if (retrying) {
    pendingReads--;
  }
 
  bool correct = false;
  if (wcb.loadCheck(dinst,correct)){ //WCB first
    wcbHit.inc(dinst->getStatsFlag());
    avgMemLat.sample(mreq->getTimeDelay()+hitDelay, mreq->getStatsFlag());         
    //correct = (correct || dinst->getNoMemReplay()); //override correct if NoMemReplay set
    if(correct) {
      wcbPass.inc(dinst->getStatsFlag());
    }else{
      wcbFail.inc(dinst->getStatsFlag());
      //wcb.invalidate(wcb.disp_pos); //invalidate the WCB line, its now in the VPC
      VPCLine *l = VPCBank->readLine(addr);
      if(l==0){
        l = VPCBank->fillLine(addr);// allocate a line in the VPC
        l->setShared();
      }
      l->setCorrect_Override(true); //since L1 is always correct, this line is assumed always correct
      mispredictions++;      

#if DEBUG
      if((!(mispredictions%10000)) && (mispredictions!=0)){
        printf("\nmispredictions:%lu ",(long unsigned int)mispredictions);
        printf("\tskipped:%lu",(long unsigned int)skips); 
        printf("\tLD-PC:0x%08lX ",(long unsigned int)dinst->getPC());
        printf("\tST-PC:0x%08lX\n",(long unsigned int)wcb.incorrect_pc);
      }
#endif
      wcb_replays++;
      dinst->getCluster()->getGProcessor()->replay(dinst);
      dinst->setConflictStorePC(wcb.incorrect_pc);
      wcb.incorrect_pc = 0; //clear to prevent future incorrect linkings
    }
    if(retrying){
      mshr->retire(addr);
    }
    avgMemLat.sample(mreq->getTimeDelay()+1);//1 for VPC-WB small structure
    mreq->ack(1); //1 for VPC-WB small structure
    return;
  }else{
    wcbMiss.inc(dinst->getStatsFlag()); 
  }
  correct = false; //reset flag 
  VPCLine *l = VPCBank->readLine(addr);
  if (l == 0) { //VPC MISS
#if 1
    AddrType conflictPC = replayCheckLSQ->replayCheck(dinst);
    if(conflictPC!=0){ //if true, replay needed
      readHit.inc(mreq->getStatsFlag()); // FIXME: need to make readIncorrect
      vpcFail.inc(dinst->getStatsFlag());
      replayInflightStore.inc(dinst->getStatsFlag()); //record CheckLSQ replay
      dinst->getCluster()->getGProcessor()->replay(dinst);
      dinst->setConflictStorePC(conflictPC);
      mreq->ack(hitDelay);
      return;
    }
#endif   
    pendingReads++;
    if(!retrying && !mshr->canIssue(addr)) {
      readHalfMiss.inc(mreq->getStatsFlag());
      CallbackBase *cb  = doReadCB::create(this, mreq, true);
      mshr->addEntry(addr, cb);
      return;
    }
    readMiss.inc(mreq->getStatsFlag());
    mshr->addEntry(addr);
    router->fwdBusRead(mreq, missDelay);
    return;   
  }
  //VPC HIT Case   
  if(l->getCorrect_Override()) { // || dinst->getNoMemReplay()) //either over-ride flag is turned on
    correct = true;
    //if (l->isShared())
      //vpcPass.inc(dinst->getStatsFlag()); // It was not displaced
    //else
    if(!l->isShared())
      vpcOverride.inc(dinst->getStatsFlag());
  }else{
    DataType  read_data;
    read_data = l->getData(addr);
    correct   = (read_data == mreq->getDInst()->getData());
  }
  if(!correct){
    vpcFail.inc(dinst->getStatsFlag());
    readHit.inc(dinst->getStatsFlag()); // FIXME: need to make readIncorrect
    if(l->getXCoreStore(addr))
      VPCreplayXcoreStore.inc(dinst->getStatsFlag());    
    dinst->getCluster()->getGProcessor()->replay(dinst);
    dinst->setConflictStorePC(l->getStorePC(addr));
  
    if (retrying) {
      mshr->retire(addr);
    }

    avgMemLat.sample(mreq->getTimeDelay()+hitDelay);
    mreq->ack(hitDelay);
    return;
  }else{
    vpcPass.inc(dinst->getStatsFlag());
    readHit.inc(mreq->getStatsFlag());
  }
  
  // Correct HIT, do not propagate to the lower levels
  
  I(mreq->isHomeNode());
  if (retrying) {
    mshr->retire(addr);
  }
  avgMemLat.sample(mreq->getTimeDelay()+hitDelay);
  mreq->ack(hitDelay);
}
/* }}} */

void VPC::doWrite(MemRequest *mreq, bool retrying)
/* processor issued write {{{1 */
{
  DInst *dinst = mreq->getDInst();
  I(dinst->getAddr());
  I(dinst->getAddr() == mreq->getAddr());
  I(dinst->getInst()->isStore());
  if (retrying) { // reissued operation
    I(0); //should never happen because there are never doWrite CBs
  }
  if(wcb.write(dinst)){ //displacement
    doCacheWrite(dinst);
  }
#if 0
  VPCLine *l = VPCBank->writeLine(addr);
  if (l == 0) { //VPC MISS
    MemRequest *mreq = MemRequest::createRead(this, dinst, addr);
    router->fwdBusRead(mreq, missDelay); 
  }
#endif
}
/* }}} */

void VPC::doBusRead(MemRequest *mreq, bool retrying)
/*NOT FOR VPC: cache busRead (exclusive or shared) request {{{1 */
{
  I(0);
}
/* }}} */

void VPC::doPushDown(MemRequest *mreq)
/* cache push down (writeback or invalidate ack) {{{1 */
{
  I(0);
}
/* }}} */

void VPC::doPushUp(MemRequest *mreq)
/* cache push up (fwdBusRead ack) {{{1 */
{
  AddrType addr = mreq->getAddr();
  VPCLine *l = VPCBank->readLine(addr); 
  if (l == 0) {
    l = allocateLine(addr, mreq);
  }
  I(mreq->isRead() || mreq->isWrite());
  I(mreq->isHomeNode());

  if(mreq->isWrite()) {
    // Triggered by a WCB displacement
    l->setExclusive();
    pendingWrites--;
  }else{
    l->setShared();
    pendingReads--;
  }

  l->setCorrect_Override(true); //simulate overlay WCB&L1, thus all correct 
  mshr->retire(addr);
  avgMissLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  avgMemLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  I(mreq->isHomeNode());
  mreq->ack(1);  
}
/* }}} */

void VPC::doInvalidate(MemRequest *mreq)
/* cache invalidate request {{{1 */
{
  // For MULTICORE, I would help to improve the incoherence to remove the cache
  // line (not guarantee, but less replays)
  router->fwdPushDown(mreq); //invalidate ack
}
/* }}} */

void VPC::doBankUpdate(MemRequest *mreq)
{
  AddrType addr = mreq->getAddr();
  VPCLine *l;
#if ALLOW_WRITEMISSES
  l = VPCBank->readLine(addr); 
  if(l==0)
    writeMiss.inc(mreq->getDInst()->getStatsFlag());
  else
    writeHit.inc(mreq->getDInst()->getStatsFlag());
  l = VPCBank->fillLine(addr);        
  l->setData(addr,mreq->getData());
  l->setStorePC(addr,mreq->getPC());
  return;
#else
  l = VPCBank->readLine(addr); 
#endif
  if(l==0){
    writeMiss.inc(mreq->getDInst()->getStatsFlag());
    return;
  }
  writeHit.inc(mreq->getDInst()->getStatsFlag());
  l->setData(addr,mreq->getData());
  l->setStorePC(addr,mreq->getPC());
  l->clearXCoreStore(addr);
}


void VPC::doCacheWrite(DInst *dinst){
  //THIS CODE SHOULD BE USED FOR A WCB->VPC DISPLACEMENT
  //DISPLACEMENT MAY ONLY OCCUR FROM WRITEADDRESS
  //THIS CODE was originally in doWrite
  //
  //
  // Check if all the words are present
  // if all present
  //   just do a readLine(addr) and do the write for all the words
  // else
  //   trigger a cacheMiss (fwdBusRead), when it comes back, write the words to 
  //   the NEWLY allocated cache line
  //     NOTE: instead of the doWriteCB, you should have 
  //
  //     NOTE: the cache miss will trigger (later in time) a doPushUp, Then it is 
  //           when you do the write to the NEWLY allocated cache line 
  //
  //     NOTE: there is a miss IF and ONLY IF there is a cache miss and there are 
  //           missing words in the WCB
  //
  //     NOTE: if you have all the words, just call "VPCBank->fillLine(addr, foo); 
  //           and ignore foo (remember to write values):
  //
  //
  I(wcb.displaced.valid);
  bool all_present = true;
  AddrType addr = wcb.displaced.tag<<wcb_lineSize;
  VPCLine *l = VPCBank->readLine(addr);
  for(uint32_t i=0;i<wcb.getnWords();i++) {
    if (!wcb.displaced.present[i]){
      all_present = false;
      break;
    }
  }
  if(all_present){ //All words are in WCB, displace to main VPC
    I(0);
    if(l==0) {
      l = VPCBank->fillLine(addr);// allocate a line in the VPC
      l->setModified();
      writeHalfMiss.inc(dinst->getStatsFlag());
    }else
      writeHit.inc(dinst->getStatsFlag());
    I(l);
    for(uint32_t i=0;i<wcb.getnWords();i++, addr+=4){
      l->setData(addr,wcb.displaced.data[i]);
      l->setStorePC(addr,wcb.displaced.storepc[i]);
      l->setSTID(addr,wcb.displaced.stID[i]);
    }
    l->setCorrect_Override(false); //only true when data comes from L1
    wcb.displaced.valid = INVALID; //guarantee we don't try to displace this line again.
    return;
  }
  I(!dinst->getInst()->isStoreAddress());
  writeMiss.inc(dinst->getStatsFlag());
#if 0
  // This is very interesting. Even if we invalidate ALL the lines that are
  // displaced, we do not get any significant performance impact. It is an
  // overkill because the VPC contents could be updated at retirement
  if(l) {
    l->invalidate();
  }
#endif
#if ALLOW_WB_DISPLACE
  if (l==0) {
    MemRequest *mreq = MemRequest::createWrite(this, addr);
    l = VPCBank->fillLine(addr);// allocate a line in the VPC
    l->setModified();
    for(uint32_t i=0;i<wcb.getnWords();i++, addr+=4){
      l->setData(addr,wcb.displaced.data[i]);
      l->setStorePC(addr,wcb.displaced.storepc[i]);
      l->setSTID(addr,wcb.displaced.stID[i]);
    }
    writeMiss.inc();
    writeMissHandler(mreq,false);
  }else{
    l->setExclusive();
  }
#endif
}

void VPC::writeMissHandler(MemRequest *mreq, bool retrying)
{
  if(!retrying && !mshr->canIssue(mreq->getAddr())) { 
    CallbackBase *cb  = writeMissHandlerCB::create(this, mreq, true);
    mshr->addEntry(mreq->getAddr(), cb);
    return;
  }
  mshr->addEntry(mreq->getAddr()); 
  pendingWrites++;
  router->fwdBusRead(mreq, missDelay); 
}

void VPC::debug(AddrType addr) {
  printf("debug called 0x%llx\n",(long long unsigned int)addr);
}

void VPC::read(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  // predicated ARM instructions can be with zero address
  if((mreq->getAddr() == 0)){
    mreq->ack(hitDelay);
    return;
  }

  if(mreq->getDInst()->getCluster()->getGProcessor()->isFlushing() || 
     mreq->getDInst()->getCluster()->getGProcessor()->isReplayRecovering()){
    if(mreq->getDInst()->getID() >= mreq->getDInst()->getCluster()->getGProcessor()->getReplayID()){
      mreq->ack(hitDelay);
      return;
    }
  }  

  doRead(mreq,false);
}
/* }}} */

void VPC::write(MemRequest *mreq)
/* main processor write entry point {{{1 */
{
  // predicated ARM insturctions can be with zero address
  if((mreq->getAddr() == 0)){
    mreq->ack(1);
    return;
  }
  
  if(mreq->isVPCWriteUpdate()){ //for VPC bank updating at retirement
    doBankUpdate(mreq);
    mreq->ack(1);
    return;
  }

  if(mreq->getDInst()->getCluster()->getGProcessor()->isFlushing() || 
     mreq->getDInst()->getCluster()->getGProcessor()->isReplayRecovering()){
    if(mreq->getDInst()->getID() >= mreq->getDInst()->getCluster()->getGProcessor()->getReplayID()){
      mreq->ack(hitDelay);
      return;
    }
  }   

  replayCheckLSQ->insert(mreq->getDInst());
  doWrite(mreq,false);
  mreq->ack(1);
}
/* }}} */

void VPC::writeAddress(MemRequest *mreq)
/* main processor write entry point {{{1 */
{
#if 0
  if(wcb.writeAddress(mreq->getDInst())){ //displacement
    doCacheWrite(mreq->getDInst()); 
  }
#endif
#if 0
  mreq->ack(1);
#else
  lower_level->writeAddress(mreq);
#endif
}
/* }}} */

void VPC::busRead(MemRequest *mreq)
/* main cache busRead entry point {{{1 */
{
  I(0);
  //doBusRead(mreq,false);
}
/* }}} */

void VPC::pushDown(MemRequest *mreq)
/* main cache pushDown entry point {{{1 */
{
  I(0);
  doPushDown(mreq);
}
/* }}} */

void VPC::pushUp(MemRequest *mreq)
/* main cache pushUp entry point {{{1 */
{
  doPushUp(mreq);
}
/* }}} */

void VPC::invalidate(MemRequest *mreq)
/* main cache invalidate entry point {{{1 */
{
  doInvalidate(mreq);
}
/* }}} */

bool VPC::canAcceptWrite(DInst *dinst) const
/* check if cache can accept more writes {{{1 */
{
  AddrType addr = dinst->getAddr();
  if(pendingWrites >= maxPendingWrites)
    return false;

  if (!mshr->canAccept(addr))
    return false;

  return true;
}
/* }}} */

bool VPC::canAcceptRead(DInst *dinst) const
/* check if cache can accept more reads {{{1 */
{
  AddrType addr = dinst->getAddr();

  if (pendingReads >= maxPendingReads)
    return false;

  if (!mshr->canAccept(addr))
    return false;

  return true;
}
/* }}} */

void VPC::dump() const
/* Dump some cache statistics {{{1 */
{
  double total =   readMiss.getDouble()  + readHit.getDouble()
    + writeMiss.getDouble() + writeHit.getDouble() + 1;

  MSG("%s:total=%8.0f:rdMiss=%8.0f:rdMissRate=%5.3f:wrMiss=%8.0f:wrMissRate=%5.3f"
      ,getName()
      ,total
      ,readMiss.getDouble()
      ,100*readMiss.getDouble()  / total
      ,writeMiss.getDouble()
      ,100*writeMiss.getDouble() / total
      );

  mshr->dump();
}
/* }}} */

TimeDelta_t VPC::ffread(AddrType addr, DataType data)
/* can accept reads? {{{1 */
{ 
  if (VPCBank->readLine(addr))
    return hitDelay;   // done!

  VPCLine *l = VPCBank->fillLine(addr);
  l->setShared();

  return router->ffread(addr,data) + missDelay;
}
/* }}} */

TimeDelta_t VPC::ffwrite(AddrType addr, DataType data)
/* can accept writes? {{{1 */
{ 
  if (VPCBank->writeLine(addr))
    return hitDelay;   // done!

  VPCLine *l = VPCBank->fillLine(addr);
  l->setShared(); // Make sure that does not trigger replays from old instructions

  return router->ffwrite(addr,data) + missDelay;
}
/* }}} */

void VPC::ffinvalidate(AddrType addr, int32_t ilineSize)
/* can accept writes? {{{1 */
{ 
  int32_t ls = getLineSize();
  int32_t maxLines = ilineSize/(ls+1)+1;
  bool allInvalid = true;
  for(int32_t i =0 ; i < maxLines ; i++) {
    VPCLine *l = VPCBank->readLine(addr+i*ls);
    if (l) {
      allInvalid = false;
      l->invalidate();
    }
  }
  if (allInvalid)
    return;
}
#endif
