// copyright and includes {{{1
// Contributed by Luis Ceze
//                Karin Strauss
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

#ifndef CCache_H
#define CCache_H

#include <vector>
#include "CacheCore.h"
#include "GProcessor.h"
#include "GStats.h"
#include "MSHR.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "SCTable.h"
#include "Snippets.h"
#include "TaskHandler.h"
#include "estl.h"
//#include "Prefetcher.h"
//#define ENABLE_LDBP

class PortManager;
class MemRequest;
/* }}} */

#define CCACHE_MAXNSHARERS 64

//#define ENABLE_PTRCHASE 1

class CCache : public MemObj {
protected:
  class CState : public StateGeneric<AddrType> { /*{{{*/
  private:
    enum StateType { M, E, S, I };
    StateType state;
    StateType shareState;

    int16_t nSharers;
    int16_t share[CCACHE_MAXNSHARERS]; // Max number of shares to remember. If nshares >=8, then broadcast

  public:
    CState(int32_t lineSize) {
      state = I;
      clearTag();
    }

    bool isModified() const {
      return state == M;
    }
    void setModified() {
      state = M;
    }
    bool isExclusive() const {
      return state == E;
    }
    void setExclusive() {
      state = E;
    }
    bool isShared() const {
      return state == S;
    }
    void setShared() {
      state = S;
    }
    bool isValid() const {
      return state != I || shareState != I;
    }
    bool isLocalInvalid() const {
      return state == I;
    }

    void forceInvalid() {
      state = I;
    }

    // If SNOOPS displaces E too
    // bool needsDisp() const { return state == M || state == E; }
    bool needsDisp() const {
      return state == M;
    }

    bool      shouldNotifyLowerLevels(MsgAction ma, bool incoherent) const;
    bool      shouldNotifyHigherLevels(MemRequest *mreq, int16_t port) const;
    StateType getState() const {
      return state;
    };
    StateType calcAdjustState(MemRequest *mreq) const;
    void      adjustState(MemRequest *mreq, int16_t port);

    static MsgAction othersNeed(MsgAction ma) {
      switch(ma) {
      case ma_setValid:
        return ma_setShared;
      case ma_setDirty:
        return ma_setInvalid;
      default:
        I(0);
      }
      I(0);
      return ma_setDirty;
    }
    MsgAction reqAckNeeds() const {
      switch(shareState) {
      case M:
        return ma_setDirty;
      case E:
        return ma_setExclusive;
      case S:
        return ma_setShared;
      case I:
        return ma_setInvalid;
      }
      I(0);
      return ma_setDirty;
    }

    // bool canRead()  const { return state != I; }
    // bool canWrite() const { return state == E || state == M; }

    void invalidate() {
      state      = I;
      nSharers   = 0;
      shareState = I;
      clearTag();
    }

    bool isBroadcastNeeded() const {
      return nSharers >= CCACHE_MAXNSHARERS;
    }

    int16_t getSharingCount() const {
      return nSharers; // Directory
    }
    void    removeSharing(int16_t id);
    void    addSharing(int16_t id);
    int16_t getFirstSharingPos() const {
      return share[0];
    }
    int16_t getSharingPos(int16_t pos) const {
      I(pos < nSharers);
      return share[pos];
    }
    void clearSharing() {
      nSharers = 0;
    }

    void set(const MemRequest *mreq);
  }; /*}}}*/

  typedef CacheGeneric<CState, AddrType>            CacheType;
  typedef CacheGeneric<CState, AddrType>::CacheLine Line;

  PortManager *port;
  CacheType *  cacheBank;
  MSHR *       mshr;
  MSHR *       pmshr;

  Time_t lastUpMsg; // can not bypass up messages (races)
  Time_t inOrderUpMessageAbs(Time_t when) {
#if 1
    if(lastUpMsg > when)
      when = lastUpMsg;
    else
      lastUpMsg = when;
#endif
    I(when >= globalClock);
    if(when == globalClock)
      when++;

    return when;
  }
  TimeDelta_t inOrderUpMessage(TimeDelta_t lat = 0) {
#if 1
    if(lastUpMsg > globalClock)
      lat += (lastUpMsg - globalClock);

    lastUpMsg = globalClock + lat;
#endif

    return lat;
  }

  int32_t lineSize;
  int32_t lineSizeBits;
  int32_t nlprefetch; // next line prefetch degree (0 == off)
  int32_t nlprefetchDistance;
  int32_t nlprefetchStride;
  int32_t prefetchDegree;

  int32_t moving_conf;
  double  megaRatio;

  bool coreCoupledFreq;
  bool inclusive;
  bool directory;
  bool needsCoherence;
  bool incoherent;
  bool victim;
  bool allocateMiss;
  bool justDirectory;

  // BEGIN Statistics
  GStatsCntr nTryPrefetch;
  GStatsCntr nSendPrefetch;

  GStatsCntr displacedSend;
  GStatsCntr displacedRecv;

  GStatsCntr invAll;
  GStatsCntr invOne;
  GStatsCntr invNone;

  GStatsCntr writeBack;

  GStatsCntr lineFill;

  GStatsAvg avgMissLat;
  GStatsAvg avgMemLat;
  GStatsAvg avgHalfMemLat;
  GStatsAvg avgSnoopLat;

  GStatsCntr capInvalidateHit;
  GStatsCntr capInvalidateMiss;
  GStatsCntr invalidateHit;
  GStatsCntr invalidateMiss;

  GStatsAvg  avgPrefetchLat;
  GStatsCntr nPrefetchUseful;
  GStatsCntr nPrefetchWasteful;
  GStatsCntr nPrefetchLineFill;
  GStatsCntr nPrefetchRedundant;
  GStatsCntr nPrefetchHitLine;
  GStatsCntr nPrefetchHitPending;
  GStatsCntr nPrefetchHitBusy;
  GStatsCntr nPrefetchDropped;

  GStatsCntr *s_reqHit[ma_MAX];
  GStatsCntr *s_reqMissLine[ma_MAX];
  GStatsCntr *s_reqMissState[ma_MAX];
  GStatsCntr *s_reqHalfMiss[ma_MAX];
  GStatsCntr *s_reqAck[ma_MAX];
  GStatsCntr *s_reqSetState[ma_MAX];

  // Statistics currently not used.
  // Only defined here to prevent bogus warnings from the powermodel.
  GStatsCntr writeExclusive;

  AddrType minMissAddr;
  AddrType maxMissAddr;

  // END Statistics
  void  displaceLine(AddrType addr, MemRequest *mreq, Line *l);
  Line *allocateLine(AddrType addr, MemRequest *mreq);
  void  mustForwardReqDown(MemRequest *mreq, bool miss, Line *l);

  bool notifyLowerLevels(Line *l, MemRequest *mreq);
  bool notifyHigherLevels(Line *l, MemRequest *mreq);

  void dropPrefetch(MemRequest *mreq);

  void
                                                  cleanup(); // FIXME: Expose this to MemObj and call it from core on ctx switch or syscall (move to public and remove callback)
  StaticCallbackMember0<CCache, &CCache::cleanup> cleanupCB;

public:
  CCache(MemorySystem *gms, const char *descr_section, const char *name = NULL);
  virtual ~CCache();

  int32_t getLineSize() const {
    return lineSize;
  }


  // Entry points to schedule that may schedule a do?? if needed
  void req(MemRequest *req);
  void blockFill(MemRequest *req);
  void reqAck(MemRequest *req);
  void setState(MemRequest *req);
  void setStateAck(MemRequest *req);
  void disp(MemRequest *req);

  void tryPrefetch(AddrType paddr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb = 0);

  // This do the real work
  void doReq(MemRequest *req);
  void doReqAck(MemRequest *req);
  void doSetState(MemRequest *req);
  void doSetStateAck(MemRequest *req);
  void doDisp(MemRequest *req);

  TimeDelta_t ffread(AddrType addr);
  TimeDelta_t ffwrite(AddrType addr);

  bool isBusy(AddrType addr) const;

  void setTurboRatio(float r);
  void dump() const;

  void setNeedsCoherence();
  void clearNeedsCoherence();

  bool isJustDirectory() const {
    return justDirectory;
  }

  bool Modified(AddrType addr) const {
    Line *cl = cacheBank->findLineNoEffect(addr);
    if(cl != 0)
      return cl->isModified();

    return false;
  }

  bool Exclusive(AddrType addr) const {
    Line *cl = cacheBank->findLineNoEffect(addr);
    if(cl != 0)
      return cl->isExclusive();

    return false;
  }

  bool Shared(AddrType addr) const {
    Line *cl = cacheBank->findLineNoEffect(addr);
    if(cl != 0)
      return cl->isShared();
    return false;
  }

  bool Invalid(AddrType addr) const {
    Line *cl = cacheBank->findLineNoEffect(addr);
    if(cl == 0)
      return true;
    return cl->isLocalInvalid();
  }

#ifdef DEBUG
  void trackAddress(MemRequest *mreq);
#else
  void trackAddress(MemRequest *mreq) {
  }
#endif
};

#endif
