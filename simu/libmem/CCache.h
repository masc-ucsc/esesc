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

#include "estl.h"
#include "CacheCore.h"
#include "GStats.h"
#include "Port.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MSHR.h"
#include "Snippets.h"

class MemRequest;
/* }}} */

class CCache: public MemObj {
protected:
  class CState : public StateGeneric<AddrType> {
  private:
    enum StateType {
      M,
      E,
			O,
      S,
      I
    } state;
		int16_t nSharers;
		int16_t share[8]; // Max number of shares to remember. If nshares >=8, then broadcast
  public:
    CState(int32_t lineSize) {
      state  = I;
      clearTag();
    }

    bool isModified() const  { return state == M; }
    void setModified() {
      state = M;
    }
    bool isExclusive() const { return state == E; }
    void setExclusive() {
      state = E;
    }
    bool isOwner() const { return state == O; }
    void setOwner() {
      state = O;
    }
    bool isShared() const    { return state == S; }
    void setShared() {
      state = S;
    }
    bool isValid()   const   { return state != I; }
    bool isInvalid() const   { return state == I; }

    bool needsDisp() const { return state == M || state == O; }

		bool shouldNotifyLowerLevels(MsgAction ma) const;
		bool shouldNotifyHigherLevels(MemRequest *mreq, int16_t port) const;
		StateType getState() const { return state; };
		StateType calcAdjustState(MemRequest *mreq) const;
		void adjustState(MemRequest *mreq, int16_t port, bool redundant = false);

    MsgAction othersNeed(MsgAction ma) const {
			switch(ma) {
				case ma_setValid:     return ma_setShared;
				case ma_setInvalid:   return ma_setInvalid;
				case ma_setDirty:     return ma_setInvalid;
				case ma_setShared:    return ma_setShared;
				case ma_setExclusive: return ma_setInvalid;
        default:          I(0);
			}
      I(0);
			return ma_setDirty;
    }
    MsgAction reqAckNeeds() const {
			switch(state) {
				case M:   return ma_setDirty;
				case O:   return ma_setShared;
				case E:   return ma_setShared;
				case S:   return ma_setShared;
				case I:   return ma_setInvalid;
			}
      I(0);
			return ma_setDirty;
    }

		bool canRead()  const { return state != I; }
		bool canWrite() const { return state == E || state == M; }

    void invalidate() {
      state  = I;
			nSharers = 0;
      clearTag();
    }

		bool isBroadcastNeeded() const { return nSharers >= 8; }

		int16_t getSharingCount() const {
			return nSharers; // Directory
		}
		void addSharing(int16_t id) {
      I(id != -1);
			if (nSharers>=8)
				return;
			share[nSharers] = id;
			nSharers++;
		}
		int16_t getFirstSharingPos() const {
			return share[0];
		}
    int16_t getSharingPos(int16_t pos) const {
      I(pos<nSharers);
      return share[pos];
    }
  };

  typedef CacheGeneric<CState,AddrType> CacheType;
  typedef CacheGeneric<CState,AddrType>::CacheLine Line;

  const TimeDelta_t hitDelay;
  const TimeDelta_t missDelay;

  TimeDelta_t dyn_hitDelay; // DVFS adjusted
  TimeDelta_t dyn_missDelay;

  // CCache has 4 ports, read, write, bank, and lower level request (bus/Ln+1)
  PortGeneric **bkPort;
  PortGeneric *ackPort;
  CacheType   *cacheBank;
  MSHR        *mshr;

	uint32_t    numBanks;

  int32_t     maxRequests;
  int32_t     curRequests;

  int32_t     lineSize;
  int32_t     lineSizeBits;
  int32_t     numBanksMask;

  bool        coreCoupledFreq;
  bool        inclusive;
  bool        directory;
  bool        needsCoherence;

  // BEGIN Statistics
  GStatsCntr  displaced;

  GStatsCntr  invAll;
  GStatsCntr  invOne;
  GStatsCntr  invNone;

  GStatsCntr  writeBack;

  GStatsCntr  lineFill;

  GStatsAvg   avgMissLat;
  GStatsAvg   avgMemLat;

	GStatsCntr  capInvalidateHit;
	GStatsCntr  capInvalidateMiss;
	GStatsCntr  invalidateHit;
  GStatsCntr  invalidateMiss;

	GStatsCntr  *s_reqHit[ma_MAX];
	GStatsCntr  *s_reqMiss[ma_MAX];
	GStatsCntr  *s_reqHalfMiss[ma_MAX];
	GStatsCntr  *s_reqAck[ma_MAX];
	GStatsCntr  *s_reqSetState[ma_MAX];

  // END Statistics

  int32_t getLineSize() const          { return lineSize;   }

  Time_t nextBankSlot(AddrType addr, bool en) { 
    if (numBanksMask == 0)
      return bkPort[0]->nextSlot(en); 

    int32_t bank = (addr>>lineSizeBits) & numBanksMask;

    return bkPort[bank]->nextSlot(en); 
  }

	void displaceLine(AddrType addr, MemRequest *mreq, Line *l);
  Line *allocateLine(AddrType addr, MemRequest *mreq);
  void mustForwardReqDown(MemRequest *mreq);

  bool notifyLowerLevels(Line *l, MemRequest *mreq);
  bool notifyHigherLevels(Line *l, MemRequest *mreq);

public:
  CCache(MemorySystem *gms, const char *descr_section, const char *name = NULL);
  virtual ~CCache();

	// Entry points to schedule that may schedule a do?? if needed
	void req(MemRequest *req);
	void reqAck(MemRequest *req);
	void setState(MemRequest *req);
	void setStateAck(MemRequest *req);
	void disp(MemRequest *req);

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
};

#endif
