// copyright and includes {{{1
// Contributed by Luis Ceze
//                Jose Renau
//                Karin Strauss
//                Milos Prvulovic
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
#include "CCache.h"
#include "MSHR.h"
#include "GProcessor.h"
#include "TaskHandler.h"
#include "MemRequest.h"
#include "../libsampler/BootLoader.h"
/* }}} */

CCache::CCache(MemorySystem *gms, const char *section, const char *name)
  // Constructor {{{1
  :MemObj(section, name)
	 ,hitDelay  (SescConf->getInt(section,"hitDelay"))
	 ,missDelay (SescConf->getInt(section,"missDelay"))
	 ,displaced       ("%s:displaced",       name)
	 ,invAll          ("%s:invAll",          name)
	 ,invOne          ("%s:invOne",          name)
	 ,invNone         ("%s:invNone",         name)
	 ,writeBack       ("%s:writeBack",       name)
	 ,lineFill        ("%s:lineFill",        name)
	 ,avgMissLat      ("%s_avgMissLat",      name)
	 ,avgMemLat       ("%s_avgMemLat",       name)
	 ,capInvalidateHit   ("%s_capInvalidateHit",   name)
	 ,capInvalidateMiss  ("%s_capInvalidateMiss",  name)
	 ,invalidateHit   ("%s_invalidateHit",   name)
	 ,invalidateMiss  ("%s_invalidateMiss",  name)
	 ,writeExclusive  ("%s:writeExclusive",  name)
{
  SescConf->isInt(section, "hitDelay");
  SescConf->isInt(section, "missDelay");
  SescConf->isGT(section, "hitDelay" ,0);
  SescConf->isGT(section, "missDelay",0);

  s_reqHit[ma_setInvalid]   = new GStatsCntr("%s:setInvalidHit",name);
  s_reqHit[ma_setValid]     = new GStatsCntr("%s:readHit",name);
  s_reqHit[ma_setDirty]     = new GStatsCntr("%s:writeHit",name);
  s_reqHit[ma_setShared]    = new GStatsCntr("%s:setSharedHit",name);
  s_reqHit[ma_setExclusive] = new GStatsCntr("%s:setExclusiveHit",name);
  s_reqHit[ma_MMU]          = new GStatsCntr("%s:MMUHit",name);
  s_reqHit[ma_VPCWU]        = new GStatsCntr("%s:VPCMUHit",name);

  s_reqMiss[ma_setInvalid]   = new GStatsCntr("%s:setInvalidMiss",name);
  s_reqMiss[ma_setValid]     = new GStatsCntr("%s:readMiss",name);
  s_reqMiss[ma_setDirty]     = new GStatsCntr("%s:writeMiss",name);
  s_reqMiss[ma_setShared]    = new GStatsCntr("%s:setSharedMiss",name);
  s_reqMiss[ma_setExclusive] = new GStatsCntr("%s:setExclusiveMiss",name);
  s_reqMiss[ma_MMU]          = new GStatsCntr("%s:MMUMiss",name);
  s_reqMiss[ma_VPCWU]        = new GStatsCntr("%s:VPCMUMiss",name);

  s_reqHalfMiss[ma_setInvalid]   = new GStatsCntr("%s:setInvalidHalfMiss",name);
  s_reqHalfMiss[ma_setValid]     = new GStatsCntr("%s:readHalfMiss",name);
  s_reqHalfMiss[ma_setDirty]     = new GStatsCntr("%s:writeHalfMiss",name);
  s_reqHalfMiss[ma_setShared]    = new GStatsCntr("%s:setSharedHalfMiss",name);
  s_reqHalfMiss[ma_setExclusive] = new GStatsCntr("%s:setExclusiveHalfMiss",name);
  s_reqHalfMiss[ma_MMU]          = new GStatsCntr("%s:MMUHalfMiss",name);
  s_reqHalfMiss[ma_VPCWU]        = new GStatsCntr("%s:VPCMUHalfMiss",name);

  s_reqAck[ma_setInvalid]   = new GStatsCntr("%s:setInvalidAck",name);
  s_reqAck[ma_setValid]     = new GStatsCntr("%s:setValidAck",name);
  s_reqAck[ma_setDirty]     = new GStatsCntr("%s:setDirtyAck",name);
  s_reqAck[ma_setShared]    = new GStatsCntr("%s:setSharedAck",name);
  s_reqAck[ma_setExclusive] = new GStatsCntr("%s:setExclusiveAck",name);
  s_reqAck[ma_MMU]          = new GStatsCntr("%s:MMUAck",name);
  s_reqAck[ma_VPCWU]        = new GStatsCntr("%s:VPCMUAck",name);

  s_reqSetState[ma_setInvalid]   = new GStatsCntr("%s:setInvalidsetState",name);
  s_reqSetState[ma_setValid]     = new GStatsCntr("%s:setValidsetState",name);
  s_reqSetState[ma_setDirty]     = new GStatsCntr("%s:setDirtysetState",name);
  s_reqSetState[ma_setShared]    = new GStatsCntr("%s:setSharedsetState",name);
  s_reqSetState[ma_setExclusive] = new GStatsCntr("%s:setExclusivesetState",name);
  s_reqSetState[ma_MMU]          = new GStatsCntr("%s:MMUsetState",name);
  s_reqSetState[ma_VPCWU]        = new GStatsCntr("%s:VPCMUsetState",name);

  dyn_hitDelay    = hitDelay;   // DVFS managed
  dyn_missDelay   = missDelay;

  coreCoupledFreq = SescConf->getBool(section,"coreCoupledFreq");
  if (coreCoupledFreq) {
    BootLoader::getPowerModelPtr()->addTurboCoupledMemory(this);
  }

  const char* mshrSection = SescConf->getCharPtr(section,"MSHR");

  char tmpName[512];

  sprintf(tmpName, "%s", name);

  cacheBank = CacheType::create(section, "", tmpName);
  lineSize  = cacheBank->getLineSize();
  lineSizeBits = log2i(lineSize);
  mshr      = MSHR::create(tmpName, mshrSection, lineSize);

  I(getLineSize() < 4096); // To avoid bank selection conflict (insane CCache line)
  I(gms);

	numBanks   = SescConf->getInt(section, "numBanks");
	numBanks   = SescConf->isBetween(section, "numBanks", 1, 1024);  // More than 1024???? (more likely a bug in the conf)
  SescConf->isPower2(section,"numBanks");
  int32_t log2numBanks = log2i(numBanks);
  if (numBanks>1)
    numBanksMask = (1<<log2numBanks)-1;
  else
    numBanksMask = 0;
  
  SescConf->checkBool(section, "inclusive");
  SescConf->checkBool(section, "directory");

  inclusive = SescConf->getBool(section, "inclusive");
  directory = SescConf->getBool(section, "directory");
	if (directory && !inclusive) {
		I(0);
		MSG("ERROR: %s CCache can not have a 'directory' without being 'inclusive'",section);
		SescConf->notCorrect();
	}
  
  int numPorts = SescConf->getInt(section, "bkNumPorts");
  int portOccp = SescConf->getInt(section, "bkPortOccp");

  bkPort = new PortGeneric* [numBanks]; 
  for (uint32_t i = 0; i < numBanks; i++){
    sprintf(tmpName, "%s_bk(%d)", name,i);
    bkPort[i] = PortGeneric::create(tmpName, numPorts, portOccp);
  }
  sprintf(tmpName, "%s_ack", name);
  ackPort = PortGeneric::create(tmpName,numPorts,portOccp);

  maxRequests = SescConf->getInt(section, "maxRequests");
  if(maxRequests == 0)
    maxRequests = 32768;

  curRequests = 0;

  MemObj *lower_level = gms->declareMemoryObj(section, "lowerLevel");
  if(lower_level)
    addLowerLevel(lower_level);
}
// }}}

CCache::~CCache()
/* destructor {{{1 */
{
  cacheBank->destroy();
}
// }}}

void CCache::displaceLine(AddrType naddr, MemRequest *mreq, Line *l)
/* displace a line from the CCache. writeback if necessary {{{1 */
{
  I(naddr != mreq->getAddr()); // naddr is the displace address, mreq is the trigger
	I(l->isValid());

  bool doStats = mreq->getStatsFlag();

  if (inclusive) {
		if (directory) {
			if (l->getSharingCount() == 0) {
				invNone.inc(doStats);

				// DONE! Nice directory tracking detected no higher level sharing
			}else if (l->getSharingCount() == 1) {
				invOne.inc(doStats);

				MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, getLineSize(), doStats);
				int32_t i = router->sendSetStateOthersPos(l->getFirstSharingPos(), inv_req, ma_setInvalid, dyn_hitDelay); 
        if (i==0)
          inv_req->ack();
			}else{
        // TODO: if l->getSharingCount() small, better than a broadcast
				invAll.inc(doStats);

				MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, getLineSize(), doStats);
				int32_t i = router->sendSetStateAll(inv_req, ma_setInvalid, dyn_hitDelay);
        if (i==0)
          inv_req->ack();
			}
		}else{
			invAll.inc(doStats);

			MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, getLineSize(), doStats);
      int32_t i = router->sendSetStateAll(inv_req, ma_setInvalid, dyn_hitDelay);
      if (i==0)
        inv_req->ack();
		}
  }

  displaced.inc(doStats);

  if (l->needsDisp()) {
    router->sendDisp(naddr, mreq->getStatsFlag(),1);
		writeBack.inc();
  }
}
// }}}

CCache::Line *CCache::allocateLine(AddrType addr, MemRequest *mreq)
/* find a new CCacheline for addr (it can return 0) {{{1 */
{
  AddrType rpl_addr=0;

  I(cacheBank->findLineDebug(addr) == 0);
  Line *l = cacheBank->fillLine(addr, rpl_addr);
  lineFill.inc(mreq->getStatsFlag());

  I(l); // Ignore lock guarantees to find line

  if(l->isValid())
    displaceLine(rpl_addr, mreq, l);

  return l;
}
// }}}

void CCache::mustForwardReqDown(MemRequest *mreq)
  // pass a req down the hierarchy Check with MSR {{{1
{
  s_reqMiss[mreq->getAction()]->inc(mreq->getStatsFlag());

  I(!mreq->isRetrying());
  router->scheduleReq(mreq, dyn_missDelay);
}
// }}}

bool CCache::CState::shouldNotifyLowerLevels(MsgAction ma) const
// {{{1
{
  switch(ma) {
    case ma_setValid:     return state == I;
    case ma_setInvalid:   return true; 
    case ma_setDirty:     return state != O && state != M && state != E;
    case ma_setShared:    return state != O && state != S && state != E;
    case ma_setExclusive: return state != E;
    default:          I(0);
  }
  I(0);
  return false;
}
// }}}

bool CCache::CState::shouldNotifyHigherLevels(MemRequest *mreq, int16_t port) const
// {{{1
{
  if (nSharers == 1 && share[0] == port)
    return false; // Nobody but requester 

  switch(mreq->getAction()) {
    case ma_setValid:     return state != S && state != O;
    case ma_setInvalid:   return state != I;
    case ma_setDirty:     return state != M;
    case ma_setShared:    return state != S && state != O;
    case ma_setExclusive: return state != E;
    default: I(0);
  }
  return false;
}
// }}}

CCache::CState::StateType CCache::CState::calcAdjustState(MemRequest *mreq) const
// {{{1
{
  StateType nstate = state;
  switch(mreq->getAction()) {
    case ma_setValid: 
      if (state == I)
        nstate = E; // Simpler not E state
      // else keep the same
      break;
    case ma_setInvalid:    
      nstate = I;
      break;
    case ma_setDirty:   
      if (state == O)
        nstate = O;
      else
        nstate = M;
      break;
    case ma_setShared:   
      if (state == O || state == M)
        nstate = O;
      else
        nstate = S;
      break;
    case ma_setExclusive:    
      //I(state == I || state == E);
      nstate = E;
      break;
    default: I(0);
  }

  return nstate;
}
// }}}

void CCache::CState::adjustState(MemRequest *mreq, int16_t port, bool redundant) 
  // {{{1
{
  StateType ostate = state;
  state = calcAdjustState(mreq);
  if (redundant && ostate == state)
    return;

  //I(ostate != state); // only if we have full MSHR

  if (state == I) {
    invalidate();
    return;
  }

  I(state != I);
  if (port<0) {
    return;
  }

  I(port>=0); // port<0 means no port found
  if (nSharers==0) {
    share[0] = port;
    nSharers++;
    return;
  }

  for(int i=0;i<nSharers;i++) {
    if(share[i] == port) {
      return;
    }
  }

  addSharing(port);
}
// }}}

bool CCache::notifyLowerLevels(Line *l, MemRequest *mreq)
  // {{{1
{
  I(mreq->isReq());

  if (!needsCoherence)
    return false;

  if (mreq->isReqAck())
    return false;

  I(mreq->isReq());

  if (l->shouldNotifyLowerLevels(mreq->getAction()))
    return true;

  return false;
}
// }}}

bool CCache::notifyHigherLevels(Line *l, MemRequest *mreq)
// {{{1
{
  // Must do the notifyLowerLevels first
  I(l);
  I(!notifyLowerLevels(l,mreq));

  if (router->isTopLevel())
    return false;

  int16_t port = router->getCreatorPort(mreq);
  if (l->shouldNotifyHigherLevels(mreq, port)) {
    MsgAction ma = l->othersNeed(mreq->getAction());

    if (!directory || l->isBroadcastNeeded()) {
      router->sendSetStateOthers(mreq,ma,dyn_missDelay);
      //I(num); // Otherwise, the need coherent would be set
    }else{
      for(int16_t i=0;i<l->getSharingCount();i++) {
        int32_t j = router->sendSetStateOthersPos(l->getSharingPos(i),mreq,ma,dyn_missDelay);
        I(j);
      }
    }

    // If mreq has pending stateack, it should not complete the read now
    return(mreq->hasPendingSetStateAck());
  }

  return false;
}
// }}}

void CCache::doReq(MemRequest *mreq)
/* processor/upper level issued read/write/busread {{{1 */
{
  AddrType addr = mreq->getAddr();
  bool retrying = mreq->isRetrying();

  if (retrying) { // reissued operation
		mreq->clearRetrying();
  }else{
    curRequests++;
    if(!mshr->canIssue(addr)) {
      s_reqHalfMiss[mreq->getAction()]->inc(mreq->getStatsFlag());

      mshr->addEntry(addr, &mreq->doReqCB);
      mreq->setRetrying();
      return;
    }
    mshr->addEntry(addr);
  }

  Line *l = cacheBank->readLine(addr);
  if (l == 0) {
		mustForwardReqDown(mreq);
    return;
  }

  if (notifyLowerLevels(l,mreq)) {
    mustForwardReqDown(mreq);
    return; // Done (no retrying), and wait for the ReqAck
  }

  I(!mreq->hasPendingSetStateAck());
  if (notifyHigherLevels(l,mreq)) {
    I(mreq->hasPendingSetStateAck());
    mreq->setRetrying();
    return;
  }

	I(l);
	I(l->isValid());

  if (retrying)
    s_reqHalfMiss[mreq->getAction()]->inc(mreq->getStatsFlag());
  else
    s_reqHit[mreq->getAction()]->inc(mreq->getStatsFlag());

  avgMemLat.sample(mreq->getTimeDelay()+dyn_hitDelay);

	Time_t when = ackPort->nextSlot(mreq->getStatsFlag())+dyn_hitDelay;
	I(when>=globalClock);
  if(mreq->isHomeNode()) {
		mreq->ackAbs(when);
	}else{
    int16_t port = router->getCreatorPort(mreq);
    l->adjustState(mreq, port, true);
		mreq->convert2ReqAck(l->reqAckNeeds());
		router->scheduleReqAckAbs(mreq, when);
	}

  curRequests--;
  I(curRequests>=0);
  mshr->retire(addr);

}
// }}}

void CCache::doDisp(MemRequest *mreq)
/* CCache displaced from higher levels {{{1 */
{
  AddrType addr = mreq->getAddr();

  cacheBank->findLineDebug(addr);
  // I(l); // inclusion. We miss some due to timing. Can someone fix the bug?
#if 0
  // FIXME: some data race is happenning (may be the nesting)
  if (l) {
    I(!l->shouldNotifyLowerLevels(mreq->getAction()));
  }
#endif

  mreq->ack();
}
// }}}

void CCache::doReqAck(MemRequest *mreq)
/* CCache reqAck {{{1 */
{
  AddrType addr = mreq->getAddr();
  Line *l = cacheBank->readLine(addr);
  if (l == 0) {
    l = allocateLine(addr, mreq);
  }

  s_reqSetState[mreq->getAction()]->inc(mreq->getStatsFlag());

  int16_t port = router->getCreatorPort(mreq);
  GI(port<0,mreq->isHomeNode());
	l->adjustState(mreq, port);

  curRequests--;
  I(curRequests>=0);
  mshr->retire(addr);

  avgMissLat.sample(mreq->getTimeDelay()+1, mreq->getStatsFlag());
  avgMemLat.sample(mreq->getTimeDelay()+1);

  if(mreq->isHomeNode()) {
		mreq->ackAbs(ackPort->nextSlot(mreq->getStatsFlag())+dyn_hitDelay);
  }else {
    router->scheduleReqAck(mreq); 
  }
}
// }}}

void CCache::doSetState(MemRequest *mreq)
/* CCache invalidate request {{{1 */
{
  I(!mreq->isHomeNode());
  if (true || !inclusive || !needsCoherence) { // FIXME: remove true
		// If not inclusive, do whatever we want
		mreq->convert2SetStateAck(this);
    router->scheduleSetStateAck(mreq, 1); // invalidate ack with nothing else if not inclusive
    return;
  }

  AddrType addr    = mreq->getAddr();
  int32_t ls       = getLineSize();
  int32_t maxLines = mreq->getLineSize()/(ls+1)+1;
  bool allInvalid  = true;

  for(int32_t i =0 ; i < maxLines ; i++) {
    Line *l = cacheBank->readLine(addr+i*ls);
    if (l) {
      allInvalid = false;
      int16_t port = router->getCreatorPort(mreq);
			l->adjustState(mreq,port);
    }
  }
  if (allInvalid) {
		// Done!
		mreq->convert2SetStateAck(this);
    router->scheduleSetStateAck(mreq, 1);
    return;
  }

	// Propagate setState to all the upper levels
  if(router->sendSetStateAll(mreq, mreq->getAction(), 1)) {
		// TODO: We should wait until the messages come to propagate the ack
		//mreq->convert2SetStateAck(this);
    //router->scheduleSetStateAck(mreq, 1); 
  }else{
		mreq->convert2SetStateAck(this);
    router->scheduleSetStateAck(mreq, 1); 
  }
}
// }}}

void CCache::doSetStateAck(MemRequest *mreq)
/* CCache invalidate request Ack {{{1 */
{
  if(mreq->isHomeNode()) {
    int16_t port = router->getCreatorPort(mreq);
    Line *l = cacheBank->readLine(mreq->getAddr());
    if (l) {
      l->adjustState(mreq, port, true);
    }
		mreq->ack();
	}else{
		router->scheduleSetStateAck(mreq, dyn_hitDelay);
	}
}
// }}}

void CCache::req(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  // predicated ARM instructions canbe with zero address
  if(mreq->getAddr() == 0) {
    mreq->ack(dyn_hitDelay);
    return;
  }

	I(!mreq->isRetrying());
	mreq->scheduleReqAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void CCache::reqAck(MemRequest *mreq)
/* request Ack {{{1 */
{
  if (mreq->getStatsFlag())
	  s_reqAck[mreq->getAction()]->inc(true);

	I(!mreq->isRetrying());
	mreq->scheduleReqAckAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void CCache::setState(MemRequest *mreq)
/* set state {{{1 */
{
  if (mreq->getStatsFlag())
	  s_reqAck[mreq->getAction()]->inc(true);

	I(!mreq->isRetrying());
	mreq->scheduleSetStateAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void CCache::setStateAck(MemRequest *mreq)
/* set state ack {{{1 */
{
	I(!mreq->isRetrying());
	mreq->scheduleSetStateAckAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

void CCache::disp(MemRequest *mreq)
/* displace a CCache line {{{1 */
{
	I(!mreq->isRetrying());
	mreq->scheduleDispAbs(nextBankSlot(mreq->getAddr(), mreq->getStatsFlag()));
}
// }}}

bool CCache::isBusy(AddrType addr) const
/* check if CCache can accept more writes {{{1 */
{
  if(curRequests >= maxRequests)
    return true;

  if (!mshr->canAccept(addr))
    return true;

  if (router->isBusyPos(0,addr))
    return true;

  return false;
}
// }}}

void CCache::dump() const
/* Dump some CCache statistics {{{1 */
{

  mshr->dump();
}
// }}}

TimeDelta_t CCache::ffread(AddrType addr)
/* can accept reads? {{{1 */
{ 
  AddrType addr_r=0;

  if (cacheBank->readLine(addr))
    return hitDelay;   // done!

  Line *l = cacheBank->fillLine(addr, addr_r);
  l->setShared(); // WARNING, can create random inconsistencies (no inv others)

  return router->ffread(addr) + dyn_missDelay;
}
// }}}

TimeDelta_t CCache::ffwrite(AddrType addr)
/* can accept writes? {{{1 */
{ 
  AddrType addr_r=0;

  if (cacheBank->writeLine(addr))
    return hitDelay;   // done!

  Line *l = cacheBank->fillLine(addr, addr_r);
  l->setExclusive(); // WARNING, can create random inconsistencies (no inv others)

  return router->ffwrite(addr) + dyn_missDelay;
}
// }}}

void CCache::setTurboRatio(float r) 
  // {{{1
{
  I(coreCoupledFreq);
  dyn_hitDelay  = (hitDelay * r);
  dyn_missDelay = (missDelay * r);
}
// }}}

void CCache::setNeedsCoherence()   
  // {{{1
{ 
	needsCoherence = true; 
}
// }}}

void CCache::clearNeedsCoherence() 
  // {{{1
{ 
	needsCoherence = false; 
} 
// }}}

