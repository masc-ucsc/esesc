// Copyright and includes {{{1
// Contributed by Jose Renau
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

#ifndef MEMREQUEST_H
#define MEMREQUEST_H

#include "MemObj.h"
#include "MRouter.h"

#include "nanassert.h"

/* }}} */

#ifdef DEBUG
#define DEBUG_CALLPATH 1
#endif

class MemRequest {
private:
  void setNextHop(MemObj *m);
  void startReq();
  void startReqAck();
  void startSetState();
  void startSetStateAck();
  void startDisp();

//#ifdef DEBUG
  uint64_t     id;
//#endif
	// memRequest pool {{{1
  static pool<MemRequest> actPool;
  friend class pool<MemRequest>;
	// }}}
 protected:

  /* MsgType declarations {{{1 */
  enum MsgType {
		mt_req,
		mt_reqAck,
		mt_setState,
		mt_setStateAck,
		mt_disp
	};

#ifdef DEBUG_CALLPATH
  class CallEdge {
  public:
    const MemObj *s;     // start
    const MemObj *e;     // end
    Time_t        tismo; // Time In Start Memory Object
    MsgType       mt;
    MsgAction     ma;
  };
#endif
  /* }}} */

  /* Local variables {{{1 */
  AddrType     addr;
  MsgType      mt;
  MsgAction    ma;
  MsgAction    ma_orig;

  MemObj       *creatorObj;
  MemObj       *homeMemObj; // Starting home node
  MemObj       *currMemObj;
  MemObj       *prevMemObj;
  MemObj       *firstCache;
  MsgAction     firstCache_ma;

#ifdef DEBUG_CALLPATH
  std::vector<CallEdge> calledge;
  Time_t        lastCallTime;
#endif

  CallbackBase *cb;

  int16_t       pendingSetStateAck;
  MemRequest   *setStateAckOrig;

  Time_t        startClock;

  bool          retrying;
  bool          needsDisp; // Once set, it keeps the value
  bool          doStats;
  bool          warmup;

  AddrType     pc;
  /* }}} */

  MemRequest();
  virtual ~MemRequest();

  void memReq();    // E.g: L1 -> L2
  void memReqAck(); // E.gL L2 -> L1 ack

  void memSetState();    // E.g: L2 -> L1
  void memSetStateAck(); // E.gL L1 -> L2 ack

  void memDisp();  // E.g: L1 -> L2


	friend class MRouter; // only mrouter can call the req directly
  void redoReq(TimeDelta_t       lat)  { redoReqCB.schedule(lat); }
  void redoReqAck(TimeDelta_t    lat)  { redoReqAckCB.schedule(lat); }
  void redoSetState(TimeDelta_t  lat)  { redoSetStateCB.schedule(lat);          }
  void redoSetStateAck(TimeDelta_t   lat)  { redoSetStateAckCB.schedule(lat);          }
  void redoDisp(TimeDelta_t   lat)  { redoDispCB.schedule(lat); }

  void startReq(MemObj *m, TimeDelta_t       lat)    { setNextHop(m); startReqCB.schedule(lat); }
  void startReqAck(MemObj *m, TimeDelta_t    lat)    { setNextHop(m); startReqAckCB.schedule(lat); }
  void startSetState(MemObj *m, TimeDelta_t  lat)    { setNextHop(m); startSetStateCB.schedule(lat);          }
  void startSetStateAck(MemObj *m, TimeDelta_t   lat){ setNextHop(m); startSetStateAckCB.schedule(lat);          }
  void startDisp(MemObj *m, TimeDelta_t   lat)       { setNextHop(m); startDispCB.schedule(lat); }



  void setStateAckDone(TimeDelta_t lat);

#ifdef DEBUG_CALLPATH
public:
	static void dump_all();
  void rawdump_calledge(TimeDelta_t lat=0, Time_t total=0);
protected:
  void dump_calledge(TimeDelta_t lat, bool interesting=false);
  void upce();
#else
  void rawdump_calledge(TimeDelta_t lat=0, Time_t total=0) { };
  void dump_calledge(TimeDelta_t lat) { }
  void upce() { };
#endif
  static MemRequest *create(MemObj *m, AddrType addr, bool doStats, CallbackBase *cb);
 public:
#ifdef ENABLE_NBSD
  void *param;
#endif
  void redoReq();
  void redoReqAck();
  void redoSetState();
  void redoSetStateAck();
  void redoDisp();

  StaticCallbackMember0<MemRequest, &MemRequest::redoReq>          redoReqCB;
  StaticCallbackMember0<MemRequest, &MemRequest::redoReqAck>       redoReqAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::redoSetState>     redoSetStateCB;
  StaticCallbackMember0<MemRequest, &MemRequest::redoSetStateAck>  redoSetStateAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::redoDisp>         redoDispCB;

  StaticCallbackMember0<MemRequest, &MemRequest::startReq>          startReqCB;
  StaticCallbackMember0<MemRequest, &MemRequest::startReqAck>       startReqAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::startSetState>     startSetStateCB;
  StaticCallbackMember0<MemRequest, &MemRequest::startSetStateAck>  startSetStateAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::startDisp>         startDispCB;

  void redoReqAbs(Time_t       when)  { redoReqCB.scheduleAbs(when); }
  void startReqAbs(MemObj *m, Time_t       when) { setNextHop(m); startReqCB.scheduleAbs(when); }
  void restartReq() { startReq(); }

  void redoReqAckAbs(Time_t     when) { redoReqAckCB.scheduleAbs(when); }
  void startReqAckAbs(MemObj *m, Time_t     when)      { setNextHop(m); startReqAckCB.scheduleAbs(when); }
  void restartReqAck() { startReqAck(); }
  void restartReqAckAbs(Time_t     when)      { startReqAckCB.scheduleAbs(when); }

  void redoSetStateAbs(Time_t   when) { redoSetStateCB.scheduleAbs(when);          }
  void startSetStateAbs(MemObj *m, Time_t   when)      { setNextHop(m); startSetStateCB.scheduleAbs(when);          }

  void redoSetStateAckAbs(Time_t    when) { redoSetStateAckCB.scheduleAbs(when);          }
  void startSetStateAckAbs(MemObj *m, Time_t    when)      { setNextHop(m); startSetStateAckCB.scheduleAbs(when);          }

  void redoDispAbs(Time_t    when) { redoDispCB.scheduleAbs(when); }
  void startDispAbs(MemObj *m, Time_t    when)      { setNextHop(m); startDispCB.scheduleAbs(when); }

  static void sendReqVPCWriteUpdate(MemObj *m, bool doStats, AddrType addr) {
    MemRequest *mreq = create(m,addr,doStats, 0);
    mreq->mt         = mt_req;
    mreq->ma         = ma_VPCWU;
    mreq->ma_orig    = mreq->ma;
		m->req(mreq);
  }
  static MemRequest *createReqRead(MemObj *m, bool doStats, AddrType addr, CallbackBase *cb=0) {
    MemRequest *mreq = create(m,addr, doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setValid; // For reads, MOES are valid states
    mreq->ma_orig    = mreq->ma;
    return mreq;
  }

  static void sendReqRead(MemObj *m, bool doStats, AddrType addr, CallbackBase *cb=0
#ifdef ENABLE_NBSD
      ,void *param=0
#endif
      ) {
    MemRequest *mreq = create(m,addr, doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setValid; // For reads, MOES are valid states
    mreq->ma_orig    = mreq->ma;
#ifdef ENABLE_NBSD
    mreq->param      = param;
#endif
		m->req(mreq);
  }

  static void sendReqRead(MemObj *m, bool doStats, AddrType addr, AddrType pc, CallbackBase *cb=0
#ifdef ENABLE_NBSD
      ,void *param=0
#endif
      ) {
    MemRequest *mreq = create(m,addr, doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setValid; // For reads, MOES are valid states
    mreq->ma_orig    = mreq->ma;
#ifdef ENABLE_NBSD
    mreq->param      = param;
#endif
    mreq->pc         = pc;
		m->req(mreq);
  }

  static void sendReqReadWarmup(MemObj *m, AddrType addr) {
    MemRequest *mreq = create(m,addr, false, 0);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setValid; // For reads, MOES are valid states
    mreq->ma_orig    = mreq->ma;
    mreq->warmup     = true;
#ifdef ENABLE_NBSD
    mreq->param      = 0;
#endif
		m->req(mreq);
  }

  static void sendReqWriteWarmup(MemObj *m, AddrType addr) {
    MemRequest *mreq = create(m,addr,false, 0);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setDirty; // For writes, only MO are valid states
    mreq->ma_orig    = mreq->ma;
    mreq->warmup     = true;
#ifdef ENABLE_NBSD
    mreq->param      = 0;
#endif
		m->req(mreq);
  }

  static void sendReqWrite(MemObj *m, bool doStats, AddrType addr, CallbackBase *cb=0
#ifdef ENABLE_NBSD
      ,void *param=0
#endif
      ) {
    MemRequest *mreq = create(m,addr,doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setDirty; // For writes, only MO are valid states
    mreq->ma_orig    = mreq->ma;
#ifdef ENABLE_NBSD
    mreq->param      = param;
#endif
		m->req(mreq);
  }
  static void sendReqWritePrefetch(MemObj *m, bool doStats, AddrType addr, CallbackBase *cb=0) {
    MemRequest *mreq = create(m,addr,doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setDirty;
    mreq->ma_orig    = mreq->ma;
		m->req(mreq);
  }

  void forceReqAction(MsgAction _ma) {
    ma = _ma;
  }

  void adjustReqAction(MsgAction _ma) {
    if (firstCache)
      return;

    firstCache = currMemObj;
    firstCache_ma = ma;
    I(mt == mt_req);
    I(_ma == ma_setExclusive);
    ma = _ma;
  }
  void recoverReqAction() {
    I(mt == mt_reqAck);
    if (firstCache != currMemObj)
      return;
    firstCache = 0;
    ma         = firstCache_ma;
  }

  void convert2ReqAck(MsgAction _ma) {
		I(mt == mt_req);
		ma = _ma;
		mt = mt_reqAck;
  }
  void convert2SetStateAck(MsgAction _ma, bool _needsDisp) {
		I(mt == mt_setState);
		mt = mt_setStateAck;
		ma = _ma;
    creatorObj = currMemObj;
    needsDisp = _needsDisp;
  }

  static void sendDirtyDisp(MemObj *m, MemObj *creator, AddrType addr, bool doStats) {
    MemRequest *mreq = create(m,addr,doStats, 0);
    mreq->mt         = mt_disp;
    mreq->ma         = ma_setDirty;
    mreq->ma_orig    = mreq->ma;
    I(creator);
    mreq->creatorObj = creator;
		m->disp(mreq);
  }
  static void sendCleanDisp(MemObj *m, MemObj *creator, AddrType addr, bool doStats) {
    MemRequest *mreq = create(m,addr,doStats, 0);
    mreq->mt         = mt_disp;
    mreq->ma         = ma_setValid;
    mreq->ma_orig    = mreq->ma;
    I(creator);
    mreq->creatorObj = creator;
		m->disp(mreq);
  }

  static MemRequest *createSetState(MemObj *m, MemObj *creator, MsgAction ma, AddrType naddr, bool doStats) {
    MemRequest *mreq = create(m,naddr,doStats, 0);
    mreq->mt         = mt_setState;
    mreq->ma         = ma;
    mreq->ma_orig    = mreq->ma;
    I(creator);
    mreq->creatorObj = creator;
    return mreq;
  }

	bool isReq() const         { return mt == mt_req; }
	bool isReqAck() const      { return mt == mt_reqAck; }
	bool isSetState() const    { return mt == mt_setState; }
	bool isSetStateAck() const { return mt == mt_setStateAck; }
	bool isSetStateAckDisp() const { return mt == mt_setStateAck && needsDisp; }
	bool isDisp() const        { return mt == mt_disp; }

  void setNeedsDisp() {
    I(mt == mt_setStateAck);
    needsDisp = true;
  }

//#ifdef DEBUG
  uint64_t getID() { return id;}
//#endif
  void destroy();

  MemObj *getHomeNode() const  { return homeMemObj; }
  MemObj *getCreator()  const  { return creatorObj; }
  MemObj *getCurrMem()  const  { return currMemObj; }
  MemObj *getPrevMem()  const  { return prevMemObj; }
  bool isHomeNode()     const  { return homeMemObj == currMemObj; }
	MsgAction getAction() const  { return ma; }
	MsgAction getOrigAction() const  { return ma_orig; }

  bool isMMU() const {return ma == ma_MMU; }
  bool isVPCWriteUpdate() const {return ma == ma_VPCWU; }

  void ack() {
    if(cb)
      cb->call();
    if(mt == mt_setStateAck)
      setStateAckDone(0);

    dump_calledge(0);
    destroy();
  }
  void ack(TimeDelta_t lat) {
    I(lat);
    if(cb) { // Not all the request require a completion notification
      cb->schedule(lat);
    }

    if(mt == mt_setStateAck)
      setStateAckDone(lat);

    dump_calledge(lat);
    destroy();
  }
  void ackAbs(Time_t when) {
    I(when);
    if(cb)
      cb->scheduleAbs(when);
    if(mt == mt_setStateAck)
      setStateAckDone(when-globalClock);

    dump_calledge(when-globalClock);
    destroy();
  }

  Time_t  getTimeDelay()  const { return globalClock-startClock; }
  Time_t  getTimeDelay(Time_t when)  const { return when-startClock; }

  AddrType getAddr() const { return addr; }

  AddrType getPC() const { return pc; }
  void setPC(AddrType _pc) { pc = _pc; }

  bool getStatsFlag() const { return doStats; }
  bool isWarmup() const { return warmup; }
  bool isRetrying() const { return retrying; }
  void setRetrying() { retrying = true; }
  void clearRetrying() { retrying = false; }

  void addPendingSetStateAck(MemRequest *mreq);
  bool hasPendingSetStateAck() const { return pendingSetStateAck>0; }
};

class MemRequestHashFunc {
 public:
  size_t operator()(const MemRequest *mreq) const {
    size_t val = (size_t)mreq;
    return val>>2;
  }
};


#endif   // MEMREQUEST_H
