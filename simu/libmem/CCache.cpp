

#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "nanassert.h"

#include "../libsampler/BootLoader.h"
#include "CCache.h"
#include "GProcessor.h"
#include "MSHR.h"
#include "MemRequest.h"
#include "MemorySystem.h"
#include "PortManager.h"
#include "SescConf.h"
#include "TaskHandler.h"

extern "C" uint64_t esesc_mem_read(uint64_t addr);

// {{{1 Debug information
#ifdef DEBUG
void CCache::trackAddress(MemRequest *mreq) {
  // I((mreq->getAddr() & 0xFFFF0) != 0x74f00);
  // I(mreq->getID() != 5);
  // I((mreq->getAddr() & 0xFFFF0) != 0x15e10);
}
#endif

//#define MTRACE(a...)   do{ if (strcasecmp(getName(),"DL1(0)") == 0) { fprintf(stderr,"@%lld %s %d 0x%x %s :",(long long
// int)globalClock,getName(), (int)mreq->getID(), (unsigned int)mreq->getAddr(), mreq->isPrefetch()?"pref":""); fprintf(stderr,##a);
// fprintf(stderr,"\n"); } }while(0)
#define MTRACE(a...)
//#define MTRACE(a...)   do{ if (mreq->getID()==49) { I(0); fprintf(stderr,"@%lld %s %d 0x%x:",(long long int)globalClock,getName(),
//(int)mreq->getID(), (unsigned int)mreq->getAddr()); fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0) #define MTRACE(a...)
//do{ if ((mreq->getAddr()>>4) == 0x100080a) { fprintf(stderr,"@%lld %s %d 0x%x:",(long long int)globalClock,getName(),
//(int)mreq->getID(), (unsigned int)mreq->getAddr()); fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0)
// 1}}}

CCache::CCache(MemorySystem *gms, const char *section, const char *name)
    // Constructor {{{1
    : MemObj(section, name)
    , nTryPrefetch("%s:nTryPrefetch", name)
    , nSendPrefetch("%s:nSendPrefetch", name)
    , displacedSend("%s:displacedSend", name)
    , displacedRecv("%s:displacedRecv", name)
    , invAll("%s:invAll", name)
    , invOne("%s:invOne", name)
    , invNone("%s:invNone", name)
    , writeBack("%s:writeBack", name)
    , lineFill("%s:lineFill", name)
    , avgMissLat("%s_avgMissLat", name)
    , avgMemLat("%s_avgMemLat", name)
    , avgHalfMemLat("%s_avgHalfMemLat", name)
    , avgPrefetchLat("%s_avgPrefetchLat", name)
    , avgSnoopLat("%s_avgSnoopLat", name)
    , capInvalidateHit("%s_capInvalidateHit", name)
    , capInvalidateMiss("%s_capInvalidateMiss", name)
    , invalidateHit("%s_invalidateHit", name)
    , invalidateMiss("%s_invalidateMiss", name)
    , writeExclusive("%s:writeExclusive", name)
    , nPrefetchUseful("%s:nPrefetchUseful", name)
    , nPrefetchWasteful("%s:nPrefetchWasteful", name)
    , nPrefetchLineFill("%s:nPrefetchLineFill", name)
    , nPrefetchRedundant("%s:nPrefetchRedundant", name)
    , nPrefetchHitLine("%s:nPrefetchHitLine", name)
    , nPrefetchHitPending("%s:nPrefetchHitPending", name)
    , nPrefetchHitBusy("%s:nPrefetchHitBusy", name)
    , nPrefetchDropped("%s:nPrefetchDropped", name)
    , cleanupCB(this) {

  // cleanupCB.scheduleAbs(globalClock+32768);

  s_reqHit[ma_setInvalid]   = new GStatsCntr("%s:setInvalidHit", name);
  s_reqHit[ma_setValid]     = new GStatsCntr("%s:readHit", name);
  s_reqHit[ma_setDirty]     = new GStatsCntr("%s:writeHit", name);
  s_reqHit[ma_setShared]    = new GStatsCntr("%s:setSharedHit", name);
  s_reqHit[ma_setExclusive] = new GStatsCntr("%s:setExclusiveHit", name);
  s_reqHit[ma_MMU]          = new GStatsCntr("%s:MMUHit", name);
  s_reqHit[ma_VPCWU]        = new GStatsCntr("%s:VPCMUHit", name);

  s_reqMissLine[ma_setInvalid]   = new GStatsCntr("%s:setInvalidMiss", name);
  s_reqMissLine[ma_setValid]     = new GStatsCntr("%s:readMiss", name);
  s_reqMissLine[ma_setDirty]     = new GStatsCntr("%s:writeMiss", name);
  s_reqMissLine[ma_setShared]    = new GStatsCntr("%s:setSharedMiss", name);
  s_reqMissLine[ma_setExclusive] = new GStatsCntr("%s:setExclusiveMiss", name);
  s_reqMissLine[ma_MMU]          = new GStatsCntr("%s:MMUMiss", name);
  s_reqMissLine[ma_VPCWU]        = new GStatsCntr("%s:VPCMUMiss", name);

  s_reqMissState[ma_setInvalid]   = new GStatsCntr("%s:setInvalidMissState", name);
  s_reqMissState[ma_setValid]     = new GStatsCntr("%s:readMissState", name);
  s_reqMissState[ma_setDirty]     = new GStatsCntr("%s:writeMissState", name);
  s_reqMissState[ma_setShared]    = new GStatsCntr("%s:setSharedMissState", name);
  s_reqMissState[ma_setExclusive] = new GStatsCntr("%s:setExclusiveMissState", name);
  s_reqMissState[ma_MMU]          = new GStatsCntr("%s:MMUMissState", name);
  s_reqMissState[ma_VPCWU]        = new GStatsCntr("%s:VPCMUMissState", name);

  s_reqHalfMiss[ma_setInvalid]   = new GStatsCntr("%s:setInvalidHalfMiss", name);
  s_reqHalfMiss[ma_setValid]     = new GStatsCntr("%s:readHalfMiss", name);
  s_reqHalfMiss[ma_setDirty]     = new GStatsCntr("%s:writeHalfMiss", name);
  s_reqHalfMiss[ma_setShared]    = new GStatsCntr("%s:setSharedHalfMiss", name);
  s_reqHalfMiss[ma_setExclusive] = new GStatsCntr("%s:setExclusiveHalfMiss", name);
  s_reqHalfMiss[ma_MMU]          = new GStatsCntr("%s:MMUHalfMiss", name);
  s_reqHalfMiss[ma_VPCWU]        = new GStatsCntr("%s:VPCMUHalfMiss", name);

  s_reqAck[ma_setInvalid]   = new GStatsCntr("%s:setInvalidAck", name);
  s_reqAck[ma_setValid]     = new GStatsCntr("%s:setValidAck", name);
  s_reqAck[ma_setDirty]     = new GStatsCntr("%s:setDirtyAck", name);
  s_reqAck[ma_setShared]    = new GStatsCntr("%s:setSharedAck", name);
  s_reqAck[ma_setExclusive] = new GStatsCntr("%s:setExclusiveAck", name);
  s_reqAck[ma_MMU]          = new GStatsCntr("%s:MMUAck", name);
  s_reqAck[ma_VPCWU]        = new GStatsCntr("%s:VPCMUAck", name);

  s_reqSetState[ma_setInvalid]   = new GStatsCntr("%s:setInvalidSetState", name);
  s_reqSetState[ma_setValid]     = new GStatsCntr("%s:setValidSetState", name);
  s_reqSetState[ma_setDirty]     = new GStatsCntr("%s:setDirtySetState", name);
  s_reqSetState[ma_setShared]    = new GStatsCntr("%s:setSharedSetState", name);
  s_reqSetState[ma_setExclusive] = new GStatsCntr("%s:setExclusiveSetState", name);
  s_reqSetState[ma_MMU]          = new GStatsCntr("%s:MMUSetState", name);
  s_reqSetState[ma_VPCWU]        = new GStatsCntr("%s:VPCMUSetState", name);

  // TODO: add support for coreCoupledFreq as part of mreq
  // if(SescConf->checkBool(section,"coreCoupledFreq")) {
  // MSG("WARNING: coreCoupledFreq does not work yet");
  //}

  bool nlp = false;
  if(SescConf->checkInt(section, "nlprefetch")) {
    nlp = SescConf->getInt(section, "nlprefetch")>0;
  }

  if(nlp) {
    nlprefetch         = SescConf->getInt(section, "nlprefetch");
    nlprefetchDistance = SescConf->getInt(section, "nlprefetchDistance");
    nlprefetchStride   = SescConf->getInt(section, "nlprefetchStride");
  } else {
    nlprefetch         = 0;
    nlprefetchDistance = 0;
    nlprefetchStride   = 1;
  }
  allocateMiss = true;
  if(SescConf->checkBool(section, "allocateMiss")) {
    allocateMiss = SescConf->getBool(section, "allocateMiss");
  }

  if(SescConf->checkBool(section, "incoherent")) {
    incoherent = SescConf->getBool(section, "incoherent");
  } else {
    incoherent = false;
  }
#if 1
  if(incoherent)
    clearNeedsCoherence();
#endif

  minMissAddr = ULONG_MAX;
  maxMissAddr = 0;

  if(SescConf->checkBool(section, "victim")) {
    victim = SescConf->getBool(section, "victim");
  } else {
    victim = false;
  }

  coreCoupledFreq = false;
  // if (coreCoupledFreq) {
  //  BootLoader::getPowerModelPtr()->addTurboCoupledMemory(this);
  //}

  char tmpName[512];
  sprintf(tmpName, "%s", name);

  cacheBank            = CacheType::create(section, "", tmpName);
  lineSize             = cacheBank->getLineSize();
  lineSizeBits         = log2i(lineSize);
  uint32_t MaxRequests = SescConf->getInt(section, "maxRequests");
  if(MaxRequests > 2000)
    MaxRequests = 2000;

  if(SescConf->checkInt(section, "prefetchDegree")) {
    prefetchDegree = SescConf->getInt(section, "prefetchDegree");
  } else {
    prefetchDegree = 4;
  }
  if(SescConf->checkDouble(section, "megaRatio")) {
    megaRatio = SescConf->getDouble(section, "megaRatio");
  } else {
    megaRatio = 2.0; // >1 means never active
  }

  if(!allocateMiss && megaRatio < 1.0) {
    MSG("ERROR: %s CCache megaRatio can be set only for allocateMiss caches", section);
    SescConf->notCorrect();
  }

  {
    int16_t lineSize2 = lineSize;
    if(!allocateMiss) {
      lineSize2 = 64; // FIXME: Do not hardcode upper level line size
    }

    mshr = new MSHR(tmpName, 128 * MaxRequests, lineSize2, MaxRequests);
    sprintf(tmpName, "%sp", name);
    pmshr = new MSHR(tmpName, 128 * MaxRequests, lineSize2, MaxRequests);
  }

  I(getLineSize() < 4096); // To avoid bank selection conflict (insane CCache line)
  I(gms);

  SescConf->checkBool(section, "inclusive");
  SescConf->checkBool(section, "directory");

  inclusive = SescConf->getBool(section, "inclusive");
  directory = SescConf->getBool(section, "directory");
  if(directory && !inclusive) {
    I(0);
    MSG("ERROR: %s CCache can not have a 'directory' without being 'inclusive'", section);
    SescConf->notCorrect();
  }
  if(SescConf->checkBool(section, "justDirectory")) {
    justDirectory = SescConf->getBool(section, "justDirectory");
  } else {
    justDirectory = false;
  }
  if(justDirectory && !inclusive) {
    MSG("ERROR: justDirectory option is only possible with inclusive=true");
    SescConf->notCorrect();
  }
  if(justDirectory && !allocateMiss) {
    MSG("ERROR: justDirectory option is only possible with allocateMiss=false");
    SescConf->notCorrect();
  }
  if(justDirectory && !directory) {
    MSG("ERROR: justDirectory option is only possible with directory=true");
    SescConf->notCorrect();
  }
  if(justDirectory && victim) {
    MSG("ERROR: justDirectory option is only possible with directory=true and no victim");
    SescConf->notCorrect();
  }

  MemObj *lower_level = gms->declareMemoryObj(section, "lowerLevel");
  if(lower_level)
    addLowerLevel(lower_level);

  port = PortManager::create(section, this);

  lastUpMsg = 0;
}
// }}}

CCache::~CCache()
/* destructor {{{1 */
{
  cacheBank->destroy();
}
// }}}


void CCache::cleanup() {

  int n = cacheBank->getNumLines();

  int nInvalidated = 0;

  for(int i = 0; i < n; i++) {
    Line *l = cacheBank->getPLine(n);
    if(!l->isValid())
      continue;

    nInvalidated++;
    l->invalidate();
  }

  MSG("Cleanup %d called for %s @%lld", nInvalidated, getName(), globalClock);

  cleanupCB.scheduleAbs(globalClock + 1000000);
}


void CCache::dropPrefetch(MemRequest *mreq)
// dropPrefetch {{{1
{
  I(mreq->isPrefetch());

  nPrefetchDropped.inc(mreq->getStatsFlag());
  mreq->setDropped();
  if(mreq->isHomeNode()) {
    mreq->ack();
  } else {
    router->scheduleReqAck(mreq, 0);
  }
}
// 1}}}

void CCache::displaceLine(AddrType naddr, MemRequest *mreq, Line *l) {
  /* displace a line from the CCache. writeback if necessary {{{1 */
  I(naddr != mreq->getAddr()); // naddr is the displace address, mreq is the trigger
  I(l->isValid());

  bool doStats = mreq->getStatsFlag();

  if(inclusive && !mreq->isTopCoherentNode()) {
    if(directory) {
      if(l->getSharingCount() == 0) {
        invNone.inc(doStats);

        // DONE! Nice directory tracking detected no higher level sharing
        if(l->isPrefetch())
          return; // No notification to lower level if prefetch (avoid overheads)
      } else if(l->getSharingCount() == 1) {
        invOne.inc(doStats);

        MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, doStats);
        trackAddress(inv_req);
        int32_t i = router->sendSetStateOthersPos(l->getFirstSharingPos(), inv_req, ma_setInvalid, inOrderUpMessage());
        if(i == 0)
          inv_req->ack();
      } else {
        // FIXME: optimize directory for 2 or more
        invAll.inc(doStats);

        MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, doStats);
        trackAddress(inv_req);
        int32_t i = router->sendSetStateAll(inv_req, ma_setInvalid, inOrderUpMessage());
        if(i == 0)
          inv_req->ack();
      }
    } else {
      invAll.inc(doStats);

      MemRequest *inv_req = MemRequest::createSetState(this, this, ma_setInvalid, naddr, doStats);
      int32_t     i       = router->sendSetStateAll(inv_req, ma_setInvalid, inOrderUpMessage());

      if(i == 0)
        inv_req->ack();
    }
  } else {
    if(l->isPrefetch()) {
      l->clearSharing();
      return; // No notification to lower level if prefetch (avoid overheads)
    }
  }

  displacedSend.inc(doStats);

  if(l->needsDisp()) {
    MTRACE("displace 0x%llx dirty", naddr);
    router->sendDirtyDisp(naddr, mreq->getStatsFlag(), 1);
    writeBack.inc(mreq->getStatsFlag());
  } else {
    MTRACE("displace 0x%llx clean", naddr);
    router->sendCleanDisp(naddr, l->isPrefetch(), mreq->getStatsFlag(), 1);
  }

  l->clearSharing();
}
// }}}

CCache::Line *CCache::allocateLine(AddrType addr, MemRequest *mreq)
/* find a new CCacheline for addr (it can return 0) {{{1 */
{
  AddrType rpl_addr = 0;
  I(mreq->getAddr() == addr);

  I(cacheBank->findLineDebug(addr) == 0);
  Line *l = cacheBank->fillLine_replace(addr, rpl_addr, mreq->getPC(), mreq->isPrefetch());
  lineFill.inc(mreq->getStatsFlag());

  I(l); // Ignore lock guarantees to find line

  if(l->isValid()) {
    if(l->isPrefetch() && !mreq->isPrefetch())
      nPrefetchWasteful.inc(mreq->getStatsFlag());

    // TODO: add a port for evictions. Schedule the displaceLine accordingly
    displaceLine(rpl_addr, mreq, l);
  }

  l->set(mreq);

  if(mreq->isPrefetch()) {
    nPrefetchLineFill.inc(mreq->getStatsFlag());
  }

#if 1
  if(megaRatio < 1) {
    static int conta = 0;
    AddrType   pendAddr[32];
    int        pendAddr_counter = 0;
    if(conta++ > 8) { // Do not try all the time
      conta = 0;

      AddrType page_addr = (addr >> 10) << 10;
      int      nHit      = 0;
      int      nMiss     = 0;

      for(int i = 0; i < (1024 >> lineSizeBits); i++) {
        if(!mshr->canIssue(page_addr + lineSize * i * nlprefetchStride)) {
          nHit++; // Pending request == hit, line already requested
        } else if(cacheBank->findLineNoEffect(page_addr + lineSize * i * nlprefetchStride)) {
          nHit++;
        } else {
          if(pendAddr_counter < 32) {
            pendAddr[pendAddr_counter++] = page_addr + lineSize * i * nlprefetchStride;
          }
          nMiss++;
        }
      }

      double ratio = nHit;
      ratio        = ratio / (double)(nHit + nMiss);
      // MSG("ratio %f %d %d trying 0x%llx",ratio,nHit,nMiss,addr);
      if(ratio > megaRatio && ratio < 1.0) {
#if 1
        for(int i = 0; i < pendAddr_counter; i++) {
          tryPrefetch(pendAddr[i], mreq->getStatsFlag(), 1, PSIGN_MEGA, mreq->getPC(), 0);
        }
#else
        // Mega next level
        for(int i = 0; i < pendAddr_counter; i++) {
          router->tryPrefetch(pendAddr[i], mreq->getStatsFlag(), 1, PSIGN_MEGA, mreq->getPC(), 0);
        }
        if(pendAddr[0] != page_addr)
          router->tryPrefetch(page_addr, mreq->getStatsFlag(), 1, PSIGN_MEGA, mreq->getPC(), 0);
#endif
      }
    }
  }
#endif

  I(l->getSharingCount() == 0);

  return l;
}
// }}}

void CCache::mustForwardReqDown(MemRequest *mreq, bool miss, Line *l)
// pass a req down the hierarchy Check with MSR {{{1
{
  if(!mreq->isPrefetch()) {
    s_reqMissLine[mreq->getAction()]->inc(miss && mreq->getStatsFlag());
    s_reqMissState[mreq->getAction()]->inc(!miss && mreq->getStatsFlag());
  }

  if(mreq->getAction() == ma_setDirty) {
    mreq->adjustReqAction(ma_setExclusive);
  }

  I(!mreq->isRetrying());

  router->scheduleReq(mreq, 0); // miss latency already charged
}
// }}}

bool CCache::CState::shouldNotifyLowerLevels(MsgAction ma, bool incoherent) const
// {{{1
{

  if(incoherent) {
    if(state == I) {
      return true;
    }
    return false;
  }

  switch(ma) {
  case ma_setValid:
    return state == I;
  case ma_setInvalid:
    return true;
  case ma_MMU: // ARM does not cache MMU translations???
  case ma_setDirty:
    return state != M && state != E;
  case ma_setShared:
    return state != S && state != E;
  case ma_setExclusive:
    return state != M && state != E;
  default:
    I(0);
  }
  I(0);
  return false;
}
// }}}

bool CCache::CState::shouldNotifyHigherLevels(MemRequest *mreq, int16_t portid) const
// {{{1
{
#if 0
  if uncoherent, return false
#endif
  if(nSharers == 0)
    return false;

  if(nSharers == 1 && share[0] == portid)
    return false; // Nobody but requester

#if 0
  if (incoherent){
    return false;
  }
#endif

  switch(mreq->getAction()) {
  case ma_setValid:
  case ma_setShared:
    return shareState != S;
  case ma_setDirty:
  case ma_setExclusive:
  case ma_setInvalid:
    return true;
  default:
    I(0);
  }
  I(0);
  return true;
}
// }}}

CCache::CState::StateType CCache::CState::calcAdjustState(MemRequest *mreq) const
// {{{1
{
  StateType nstate = state;
  switch(mreq->getAction()) {
  case ma_setValid:
    if(state == I)
      nstate = E; // Simpler not E state
    // else keep the same
    break;
  case ma_setInvalid:
    nstate = I;
    break;
  case ma_setDirty:
    nstate = M;
    break;
  case ma_setShared:
    nstate = S;
    break;
  case ma_setExclusive:
    // I(state == I || state == E);
    nstate = E;
    break;
  default:
    I(0);
  }

  return nstate;
}
// }}}

void CCache::CState::adjustState(MemRequest *mreq, int16_t portid)
// {{{1
{
  StateType ostate = state;
  state            = calcAdjustState(mreq);

  // I(ostate != state); // only if we have full MSHR
  GI(mreq->isReq(), mreq->getAction() == ma_setExclusive || mreq->getAction() == ma_setDirty || mreq->getAction() == ma_setValid);
  GI(mreq->isReqAck(),
     mreq->getAction() == ma_setExclusive || mreq->getAction() == ma_setDirty || mreq->getAction() == ma_setShared);
  GI(mreq->isDisp(), mreq->getAction() == ma_setDirty || mreq->getAction() == ma_setValid);
  GI(mreq->isSetStateAck(), mreq->getAction() == ma_setShared || mreq->getAction() == ma_setInvalid);
  GI(mreq->isSetState(), mreq->getAction() == ma_setShared || mreq->getAction() == ma_setInvalid);

  if(mreq->isDisp()) {
    I(state != I);
    removeSharing(portid);
    if(mreq->getAction() == ma_setDirty) {
      state = M;
    } else {
      I(mreq->getAction() == ma_setValid);
      if(nSharers == 0)
        state = I;
    }
  } else if(mreq->isSetStateAck()) {
    if(mreq->getAction() == ma_setInvalid) {
      if(isBroadcastNeeded())
        nSharers = CCACHE_MAXNSHARERS - 1; // Broadcast was sent, remove broadcast need
      removeSharing(portid);
    } else {
      I(mreq->getAction() == ma_setShared);
      if(shareState != I)
        shareState = S;
    }
    if(mreq->isHomeNode()) {
      state = ostate;
      if(mreq->isSetStateAckDisp()) {
        // Only if it is the one that triggered the setState
        state = M;
      }
    }
  } else if(mreq->isSetState()) {
    //    if (nSharers)
    //      shareState = state;
    // I(portid<0);
  } else {
    I(state != I);
    I(mreq->isReq() || mreq->isReqAck());
    if(ostate != I && !mreq->isTopCoherentNode()) {
      // I(!mreq->isHomeNode());
      state = ostate;
    }
    if(mreq->isReqAck() && !mreq->isTopCoherentNode()) {
      switch(mreq->getAction()) {
      case ma_setDirty:
        if(ostate == I)
          state = E;
        else
          state = ostate;
        break;
      case ma_setShared:
        state = S;
        break;
      case ma_setExclusive:
        state = E;
        break;
      default:
        I(0);
      }
    }
    addSharing(portid);
    if(nSharers == 0)
      shareState = I;
    else if(nSharers > 1)
      shareState = S;
    else {
      if(state == S)
        shareState = S;
      else {
        I(state != I);
        shareState = E;
      }
    }
  }

  GI(nSharers > 1, shareState != E && shareState != M);
  GI(shareState == E || shareState == M, nSharers == 1);
  GI(nSharers == 0, shareState == I);
  GI(shareState == I, nSharers == 0);

  if(state == I && shareState == I) {
    invalidate();
    return;
  }
}
// }}}

bool CCache::notifyLowerLevels(Line *l, MemRequest *mreq)
// {{{1
{
  if(justDirectory)
    return false;

  if(!needsCoherence)
    return false;

  if(mreq->isReqAck())
    return false;

  I(mreq->isReq());

  if(l->shouldNotifyLowerLevels(mreq->getAction(), incoherent))
    return true;

  return false;
}
// }}}

bool CCache::notifyHigherLevels(Line *l, MemRequest *mreq)
// {{{1
{
  // Must do the notifyLowerLevels first
  I(l);
  I(!notifyLowerLevels(l, mreq));

  if(mreq->isTopCoherentNode())
    return false;

  if(victim)
    return false;

  I(!mreq->isHomeNode());
  I(!router->isTopLevel());

  int16_t portid = router->getCreatorPort(mreq);
  if(l->shouldNotifyHigherLevels(mreq, portid)) {
    if(mreq->isPrefetch()) {
      dropPrefetch(mreq);
      return false;
    }
    MsgAction ma = l->othersNeed(mreq->getOrigAction());

    if(ma == ma_setShared && mreq->isReqAck()) {
      I(mreq->getAction() == ma_setShared || mreq->getAction() == ma_setExclusive);
      if(mreq->getAction() == ma_setExclusive)
        mreq->forceReqAction(ma_setShared);
    }

    trackAddress(mreq);
    // TODO: check that it is the correct DL1 IL1 request source

    if(!directory || l->isBroadcastNeeded()) {
      router->sendSetStateOthers(mreq, ma, inOrderUpMessage());
      // I(num); // Otherwise, the need coherent would be set
    } else {
      for(int16_t i = 0; i < l->getSharingCount(); i++) {
        int16_t pos = l->getSharingPos(i);
        if(pos != portid) {
#ifdef DEBUG
          int32_t j =
#endif
              router->sendSetStateOthersPos(l->getSharingPos(i), mreq, ma, inOrderUpMessage());
          I(j);
        }
      }
    }

    // If mreq has pending stateack, it should not complete the read now
    return (mreq->hasPendingSetStateAck());
  }

  return false;
}
// }}}

void CCache::CState::addSharing(int16_t id) { /*{{{*/
  if(nSharers >= CCACHE_MAXNSHARERS) {
    I(shareState == S);
    return;
  }
  if(id < 0) {
    if(nSharers == 0) {
      shareState = I;
    }
    return;
  }

  I(id >= 0); // portid<0 means no portid found
  if(nSharers == 0) {
    share[0] = id;
    nSharers++;
    GI(nSharers > 1, shareState == S);
    return;
  }

  for(int i = 0; i < nSharers; i++) {
    if(share[i] == id) {
      return;
    }
  }

  share[nSharers] = id;
  nSharers++;
} /*}}}*/

void CCache::CState::removeSharing(int16_t id)
// {{{1
{
  if(nSharers >= CCACHE_MAXNSHARERS)
    return; // not possible to remove if in broadcast mode

  for(int16_t i = 0; i < nSharers; i++) {
    if(share[i] == id) {
      for(int16_t j = i; j < (nSharers - 1); j++) {
        share[j] = share[j + 1];
      }
      nSharers--;
      if(nSharers == 0)
        shareState = I;
      return;
    }
  }
}
// }}}

void CCache::CState::set(const MemRequest *mreq) {
  // {{{1 set Cstate
  if(mreq->isPrefetch()) {
    setPrefetch(mreq->getPC(), mreq->getSign(), mreq->getDegree());
  } else {
    clearPrefetch(mreq->getPC());
  }
}
// 1}}}

void CCache::doReq(MemRequest *mreq)
/* processor/upper level issued read/write/busread {{{1 */
{
  MTRACE("doReq start");

  trackAddress(mreq);

  AddrType addr     = mreq->getAddr();
  bool     retrying = mreq->isRetrying();

  // MSG("%s 0x%x %s",getName(),  mreq->getAddr(), retrying?"R":"");

  if(retrying) { // reissued operation
    mreq->clearRetrying();
    // GI(mreq->isPrefetch() , !pmshr->canIssue(addr)); // the req is already queued if retrying
    GI(!mreq->isPrefetch(), !mshr->canIssue(addr)); // the req is already queued if retrying
    I(!mreq->isPrefetch());
  } else {
    if(!mshr->canIssue(addr)) {
      MTRACE("doReq queued");
#if 0
      if (mreq->isPrefetch()) {
        // merge prefetch
        MSG("%s @%lld merge preq 0x%llx (%d)",getName(), globalClock, mreq->getAddr(), mreq->getID());
      }
#endif

      if(mreq->isPrefetch()) {
        dropPrefetch(mreq);
      } else {
        mreq->setRetrying();
        mshr->addEntry(addr, &mreq->redoReqCB, mreq);
      }
      return;
    }
    mshr->blockEntry(addr, mreq);
  }

  if(mreq->isNonCacheable()) {
    router->scheduleReq(mreq, 0); // miss latency already charged
    return;
  }

  Line *l = 0;
  if(mreq->isPrefetch())
    l = cacheBank->findLineNoEffect(addr, mreq->getPC());
  else
    l = cacheBank->readLine(addr, mreq->getPC());

  if(!allocateMiss && l == 0) {
    AddrType page_addr = (addr >> 10) << 10;
    if(page_addr == addr && mreq->getSign() == PSIGN_MEGA) {
      // Time to allocate a megaLine
      MSG("Mega allocate 0x%llx", addr);
    } else {
      router->scheduleReq(mreq, 0); // miss latency already charged
      return;
    }
  }

  if(nlprefetch != 0 && !retrying && !mreq->isPrefetch()) {
    AddrType base = addr + nlprefetchDistance * lineSize;
    for(int i = 0; i < nlprefetch; i++) {
#if 1
      // Geometric stride
      // static int prog[] = {1,3,7,15,31,63,127,255,511};
      static int prog[] = {1, 3, 6, 25, 15, 76, 63, 229, 127, 458};
      int        delta;
      if(i < 8)
        delta = prog[i];
      else
        delta = i * 67;
      delta = delta * nlprefetchStride;
      tryPrefetch(base + (delta * lineSize), mreq->getStatsFlag(), i, PSIGN_NLINE, mreq->getPC());
#else
      tryPrefetch(base + (i * nlprefetchStride * lineSize), mreq->getStatsFlag(), i, PSIGN_NLINE, mreq->getPC());
#endif
    }
  }

  if(l && mreq->isPrefetch() && mreq->isHomeNode()) {
    nPrefetchDropped.inc(mreq->getStatsFlag());
    mreq->setDropped(); // useless prefetch, already a hit
  }

  if(justDirectory && retrying && l == 0) {
    l = allocateLine(addr, mreq);
  }

  if(l == 0) {
#if 0
    if (!retrying && strcasecmp(getName(),"DL1(0)") == 0) {
      MSG("miss cl:%llx",mreq->getAddr());
    }
#endif
    // if (nlprefetch)
    // MSG("%s @%lld miss req 0x%llx (%d)",getName(), globalClock, mreq->getAddr(), mreq->getID());
    MTRACE("doReq cache miss");
    mustForwardReqDown(mreq, true, 0);
    return;
  }

  if(l->isPrefetch() && !mreq->isPrefetch()) {
    nPrefetchUseful.inc(mreq->getStatsFlag());
    I(!victim); // Victim should not have prefetch lines
  }

  if(l->isPrefetch() && mreq->isPrefetch()) {
    nPrefetchRedundant.inc(mreq->getStatsFlag());
  }

  l->clearPrefetch(mreq->getPC());

  if(notifyLowerLevels(l, mreq)) {
    MTRACE("doReq change state down");
    mustForwardReqDown(mreq, false, l);
    return; // Done (no retrying), and wait for the ReqAck
  }

  I(!mreq->hasPendingSetStateAck());
  GI(mreq->isPrefetch(), !mreq->hasPendingSetStateAck());
  bool waitupper = notifyHigherLevels(l, mreq);
  if(waitupper) {
    MTRACE("doReq change state up others");
    I(mreq->hasPendingSetStateAck());
    // Not easy to drop prefetch
    mreq->setRetrying();
    return;
  }
  if(mreq->isDropped()) {
    // NotifyHigherLevel can trigger and setDropped
    port->reqRetire(mreq);
    mshr->retire(addr, mreq);
    return;
  }

  I(l);
  I(l->isValid() || justDirectory); // JustDirectory can have newly allocated line with invalid state

  int16_t portid = router->getCreatorPort(mreq);
  GI(portid < 0, mreq->isTopCoherentNode());
  l->adjustState(mreq, portid);

  Time_t when = port->reqDone(mreq, retrying);
  if(when == 0) {
    // I(0);
    MTRACE("doReq restartReq");
    // Must restart request
    if(mreq->isPrefetch()) {
      dropPrefetch(mreq);
      port->reqRetire(mreq);
      mshr->retire(addr, mreq);
    } else {
      mreq->setRetrying();
      mreq->restartReq();
    }
    return;
  }

  when = inOrderUpMessageAbs(when);

  if(justDirectory) {
    if(l->needsDisp())
      router->sendDirtyDisp(mreq->getAddr(), mreq->getStatsFlag(), 1);
    l->forceInvalid();
  }

  if(!mreq->isPrefetch()) {
    if(retrying) {
      s_reqHalfMiss[mreq->getAction()]->inc(mreq->getStatsFlag());
    } else {
      s_reqHit[mreq->getAction()]->inc(mreq->getStatsFlag());
    }
  }

  if(mreq->isDemandCritical()) {
    I(!mreq->isPrefetch());
#ifdef ENABLE_LDBP
    if(mreq->isTriggerLoad() && mreq->isHomeNode()) {
      //mreq->getHomeNode()->find_cir_queue_index(mreq);
      //mreq->getHomeNode()->lor_find_index(mreq);
      mreq->getHomeNode()->lor_find_index(mreq->getAddr());
    }
#endif
    double lat = mreq->getTimeDelay() + (when - globalClock);
    avgMemLat.sample(lat, mreq->getStatsFlag());
    if(retrying)
      avgHalfMemLat.sample(lat, mreq->getStatsFlag());

#if 0
    if (mreq->getPC())
      MSG("%s pc=%llx addr=%llx lat=%g @%lld", getName(), mreq->getPC(), mreq->getAddr(), lat, when);
#endif
  } else if(mreq->isPrefetch()) {
    double lat = mreq->getTimeDelay() + (when - globalClock);
    avgPrefetchLat.sample(lat, mreq->getStatsFlag());
  }

  I(when > globalClock);
  if(mreq->isHomeNode()) {
    mreq->ackAbs(when);
    DInst *dinst = mreq->getDInst();
    if(dinst)
      dinst->setFullMiss(false);
  } else {
    mreq->convert2ReqAck(l->reqAckNeeds());
    router->scheduleReqAckAbs(mreq, when);
  }

  MTRACE("doReq done %lld", when);

  port->reqRetire(mreq);
  mshr->retire(addr, mreq);
}
// }}}

void CCache::doDisp(MemRequest *mreq)
/* CCache displaced from higher levels {{{1 */
{
  trackAddress(mreq);

  AddrType addr = mreq->getAddr();

  // A disp being prefetch means that the line was prefetched but never used

  Line *l = cacheBank->findLineNoEffect(addr, mreq->getPC());
  if(l == 0 && victim && allocateMiss && !mreq->isPrefetch()) {
    MTRACE("doDisp allocateLine");
    l = allocateLine(addr, mreq);
  }
  if(l) {
    int16_t portid = router->getCreatorPort(mreq);
    l->adjustState(mreq, portid);
  }
  if(justDirectory) { // Directory info kept, invalid line to trigger misses
    // router->sendDirtyDisp(addr, mreq->getStatsFlag(), 1);
    if(l) {
      if(l->getSharingCount() == 0)
        l->invalidate();
    }
    router->scheduleDisp(mreq, 1);
    writeBack.inc(mreq->getStatsFlag());
  } else if(!allocateMiss && l == 0) {
    router->scheduleDisp(mreq, 1);
    writeBack.inc(mreq->getStatsFlag());
  } else {
    mreq->ack();
  }
}
// }}}

void CCache::blockFill(MemRequest *mreq)
// Block cache line fill {{{1
{
  port->blockFill(mreq);
}
// }}}

void CCache::doReqAck(MemRequest *mreq)
/* CCache reqAck {{{1 */
{
  MTRACE("doReqAck start");
  trackAddress(mreq);

  mreq->recoverReqAction();

  Time_t when = globalClock;
  if(!mreq->isDropped()) {
    if(!mreq->isNonCacheable()) {
      AddrType addr = mreq->getAddr();
      Line *   l    = 0;
      if(mreq->isPrefetch())
        l = cacheBank->findLineNoEffect(addr, mreq->getPC());
      else {
        l = cacheBank->readLine(addr, mreq->getPC());

#ifdef ENABLE_PTRCHASE
        if(mreq->getAddr() < 0x4000000000ULL) { // filter stack addresses
          if(minMissAddr > mreq->getAddr())
            minMissAddr = mreq->getAddr();
          if(maxMissAddr < mreq->getAddr())
            maxMissAddr = mreq->getAddr();
        }

        AddrType base = (addr >> 6) << 6;
        for(int i = 0; i < 16; i++) {
          AddrType data = esesc_mem_read(base + i);
          if(data > minMissAddr && data < maxMissAddr)
            tryPrefetch(data, mreq->getStatsFlag(), 1, PSIGN_CHASE, mreq->getPC(), 0);
        }
#endif
      }

      // It could be l!=0 if we requested a check in the lower levels to change state.
      if(l == 0) {
        if(!victim && allocateMiss) {
          MTRACE("doReqAck allocateLine");
          l = allocateLine(addr, mreq);
        } else if(!allocateMiss && mreq->isHomeNode()) {
          MTRACE("doReqAck allocateLine");
          l = allocateLine(addr, mreq);
        }
      } else {
        l->set(mreq);

        if(notifyHigherLevels(l, mreq)) {
          // FIXME I(0);
          MTRACE("doReqAck Notifying Higher Levels");
          I(mreq->hasPendingSetStateAck());
          return;
        }
      }

      // s_reqSetState[mreq->getAction()]->inc(mreq->getStatsFlag());

      int16_t portid = router->getCreatorPort(mreq);
      GI(portid < 0, mreq->isTopCoherentNode());
      if(l) {
        l->adjustState(mreq, portid);

        if(justDirectory) { // Directory info kept, invalid line to trigger misses
          l->forceInvalid();
        }
      } else {
        I(victim || !allocateMiss); // Only victim can pass through
      }
    }
  }

  when = port->reqAckDone(mreq);
  Time_t when_copy = when;
  if(when == 0) {
    MTRACE("doReqAck restartReqAck");
    // Must restart request
    if(mreq->isPrefetch()) {
      dropPrefetch(mreq);
      port->reqRetire(mreq);
      mshr->retire(mreq->getAddr(), mreq);
    } else {
      mreq->setRetrying();
      mreq->restartReqAckAbs(globalClock + 3);
    }
    return;
  }
  if(!mreq->isWarmup())
    when = inOrderUpMessageAbs(when);

  port->reqRetire(mreq);

  if(mreq->isDemandCritical()) {
    I(!mreq->isPrefetch());
#ifdef ENABLE_LDBP
    if(mreq->isTriggerLoad() && mreq->isHomeNode()) {
      //mreq->getHomeNode()->find_cir_queue_index(mreq);
      //mreq->getHomeNode()->lor_find_index(mreq);
      mreq->getHomeNode()->lor_find_index(mreq->getAddr());
    }
#endif
    double lat = mreq->getTimeDelay(when);
    avgMissLat.sample(lat, mreq->getStatsFlag() && !mreq->isPrefetch());
    avgMemLat.sample(lat, mreq->getStatsFlag() && !mreq->isPrefetch());
#if 0
    if (lat>30 && strcmp(getName(),"L2(0)")==0)
      MSG("%s id=%lld addr=%llx lat=%g @%lld when=%lld", getName(), mreq->getID(), mreq->getAddr(), lat, globalClock, when);
#endif
  } else if(mreq->isPrefetch()) {
    double lat = mreq->getTimeDelay(when);
    avgPrefetchLat.sample(lat, mreq->getStatsFlag());
  }

  if(mreq->isHomeNode()) {
    MTRACE("doReqAck isHomeNode, calling ackAbs %lld", when);
    DInst *dinst = mreq->getDInst();
    if(dinst)
      dinst->setFullMiss(true);
    mreq->ackAbs(when);
  } else {
    MTRACE("doReqAck is Not HomeNode, calling ackAbsCB %u", when);
    router->scheduleReqAckAbs(mreq, when);
  }

  mshr->retire(mreq->getAddr(), mreq);
}
// }}}

void CCache::doSetState(MemRequest *mreq)
/* CCache invalidate request {{{1 */
{
  trackAddress(mreq);
  I(!mreq->isHomeNode());

  GI(victim, needsCoherence);

  if(!inclusive || !needsCoherence) {
    // If not inclusive, do whatever we want
    I(mreq->getCurrMem() == this);
    mreq->convert2SetStateAck(ma_setInvalid, false);
    router->scheduleSetStateAck(mreq, 0); // invalidate ack with nothing else if not inclusive

    MTRACE("scheduleSetStateAck without disp (incoherent cache)");
    return;
  }

  Line *l = cacheBank->findLineNoEffect(mreq->getAddr());
  if(victim) {
    I(needsCoherence);
    invAll.inc(mreq->getStatsFlag());
    int32_t nmsg = router->sendSetStateAll(mreq, mreq->getAction(), inOrderUpMessage());
    if(l)
      l->invalidate();
    I(nmsg);
    return;
  }

  if(l == 0) {
    // Done!
    mreq->convert2SetStateAck(ma_setInvalid, false);
    router->scheduleSetStateAck(mreq, 0);

    MTRACE("scheduleSetStateAck without disp (local miss)");
    return;
  }

  // FIXME: add hit/mixx delay

  int16_t portid = router->getCreatorPort(mreq);
  if(l->getSharingCount()) {
    if(portid >= 0)
      l->removeSharing(portid);
    if(directory) {
      if(l->getSharingCount() == 1) {
        invOne.inc(mreq->getStatsFlag());
        int32_t i = router->sendSetStateOthersPos(l->getFirstSharingPos(), mreq, mreq->getAction(), inOrderUpMessage());
        I(i);
      } else {
        invAll.inc(mreq->getStatsFlag());
        // FIXME: optimize directory for 2 or more
        int32_t nmsg = router->sendSetStateAll(mreq, mreq->getAction(), inOrderUpMessage());
        I(nmsg);
      }
    } else {
      invAll.inc(mreq->getStatsFlag());
      int32_t nmsg = router->sendSetStateAll(mreq, mreq->getAction(), inOrderUpMessage());
      I(nmsg);
    }
  } else {
    invNone.inc(mreq->getStatsFlag());
    // We are done
    bool needsDisp = l->needsDisp();
    l->adjustState(mreq, portid);
    GI(mreq->getAction() == ma_setInvalid, !l->isValid());
    GI(mreq->getAction() == ma_setShared, l->isShared());

    mreq->convert2SetStateAck(mreq->getAction(), needsDisp); // Keep shared or invalid
    router->scheduleSetStateAck(mreq, 0);

    MTRACE("scheduleSetStateAck %s disp", needsDisp ? "with" : "without");
  }
}
// }}}

void CCache::doSetStateAck(MemRequest *mreq)
/* CCache invalidate request Ack {{{1 */
{
  trackAddress(mreq);

  Line *l = cacheBank->findLineNoEffect(mreq->getAddr());
  if(l) {
    bool    needsDisp = l->needsDisp();
    int16_t portid    = router->getCreatorPort(mreq);

    l->adjustState(mreq, portid);
    if(needsDisp)
      mreq->setNeedsDisp();
  }

  if(mreq->isHomeNode()) {
    MTRACE("scheduleSetStateAck %s disp (home) line is %d", mreq->isDisp() ? "with" : "without", l ? l->getState() : -1);

    avgSnoopLat.sample(mreq->getTimeDelay() + 0, mreq->getStatsFlag());
    mreq->ack(); // same cycle
  } else {
    router->scheduleSetStateAck(mreq, 1);
    MTRACE("scheduleSetStateAck %s disp (forward)", mreq->isDisp() ? "with" : "without");
  }
}
// }}}

void CCache::req(MemRequest *mreq)
/* main processor read entry point {{{1 */
{
  // predicated ARM instructions can be with zero address
  if(mreq->getAddr() == 0) {
    mreq->ack(1);
    return;
  }

  if(!incoherent)
    mreq->trySetTopCoherentNode(this);

  port->req(mreq);
}
// }}}

void CCache::reqAck(MemRequest *mreq)
/* request Ack {{{1 */
{
  // I(!mreq->isRetrying());
  //
  MTRACE("Received reqAck request");
  if(mreq->isRetrying()) {
    MTRACE("reqAck clearRetrying");
    mreq->clearRetrying();
  } else {
    s_reqAck[mreq->getAction()]->inc(mreq->getStatsFlag());
  }

  port->reqAck(mreq);
}
// }}}

void CCache::setState(MemRequest *mreq)
/* set state {{{1 */
{

  if(incoherent) {
    mreq->convert2SetStateAck(ma_setInvalid, true);
  }

  s_reqSetState[mreq->getAction()]->inc(mreq->getStatsFlag());

  I(!mreq->isRetrying());
  port->setState(mreq);
}
// }}}

void CCache::setStateAck(MemRequest *mreq)
/* set state ack {{{1 */
{
  I(!mreq->isRetrying());
  port->setStateAck(mreq);
}
// }}}

void CCache::disp(MemRequest *mreq)
/* displace a CCache line {{{1 */
{
  displacedRecv.inc(mreq->getStatsFlag());
  I(!mreq->isRetrying());
  port->disp(mreq);
}
// }}}

void CCache::tryPrefetch(AddrType paddr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* Try to prefetch this address (no guarantees) {{{1 */
{
  if((paddr >> 8) == 0) {
    if(cb)
      cb->destroy();
    return;
  }

  I(degree < 40);

  nTryPrefetch.inc(doStats);

  if(cacheBank->findLineNoEffect(paddr)) {
    nPrefetchHitLine.inc(doStats);
    if(cb) {
      // static_cast<IndirectAddressPredictor::performedCB *>(cb)->setParam1(this);
      cb->call();
    }
    return;
  }

  if(!mshr->canIssue(paddr)) {
    nPrefetchHitPending.inc(doStats);
    if(cb)
      cb->destroy();
    return;
  }

  if(!allocateMiss) {
    AddrType page_addr = (paddr >> 10) << 10;
    if(pref_sign != PSIGN_MEGA || page_addr != paddr) {
      nPrefetchHitBusy.inc(doStats);
      router->tryPrefetch(paddr, doStats, degree, pref_sign, pc, cb);
      return;
    }
  }

#ifdef DEBUG3
  fprintf(stderr, "%s %s %s %s ", getName(), cb ? "x" : "", port->isBusy(paddr) ? "B" : "", degree > prefetchDegree ? "y" : "");
#endif

  if(port->isBusy(paddr) || degree > prefetchDegree || victim) {
    nPrefetchHitBusy.inc(doStats);
    router->tryPrefetch(paddr, doStats, degree, pref_sign, pc, cb);
    return;
  }

  nSendPrefetch.inc(doStats);
  if(cb) {
    // I(pref_sign==PSIGN_STRIDE);
    // static_cast<IndirectAddressPredictor::performedCB *>(cb)->setParam1(this);
  }
  MemRequest *preq = MemRequest::createReqReadPrefetch(this, doStats, paddr, pref_sign, degree, pc, cb);
  preq->trySetTopCoherentNode(this);
  // MSG("%s @%lld try 0x%llx (%d)",getName(), globalClock, preq->getAddr(), preq->getID());
  port->startPrefetch(preq);
  router->scheduleReq(preq, 1);
  mshr->blockEntry(paddr, preq);
}
// }}}

bool CCache::isBusy(AddrType addr) const
/* check if CCache can accept more writes {{{1 */
{
  if(port->isBusy(addr))
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
  AddrType addr_r = 0;

  Line *l = cacheBank->readLine(addr);
  if(l)
    return 1; // done!

  l = cacheBank->fillLine_replace(addr, addr_r, 0xbeefbeef);
  l->setExclusive(); // WARNING, can create random inconsistencies (no inv others)

  return router->ffread(addr) + 1;
}
// }}}

TimeDelta_t CCache::ffwrite(AddrType addr)
/* can accept writes? {{{1 */
{
  AddrType addr_r = 0;

  Line *l = cacheBank->writeLine(addr);
  if(l == 0) {
    l = cacheBank->fillLine_replace(addr, addr_r, 0xbeefbeef);
  }
  if(router->isTopLevel())
    l->setModified(); // WARNING, can create random inconsistencies (no inv others)
  else
    l->setExclusive();

  return router->ffwrite(addr) + 1;
}
// }}}

void CCache::setTurboRatio(float r)
// {{{1
{
  I(coreCoupledFreq);
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
