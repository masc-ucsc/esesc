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
#include "DInst.h"

#include "nanassert.h"

/* }}} */

#ifdef DEBUG
//#define DEBUG_CALLPATH 1
#endif

class MemRequest {
 private:
#ifdef DEBUG
  uint64_t     id;
#endif
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
    TimeDelta_t   tismo; // Time In Start Memory Object
    MsgType       mt;    
    MsgAction     ma;    
  };
#endif
  /* }}} */

  /* Local variables {{{1 */
  AddrType     addr;
#ifdef SCOORE_VPC
  AddrType     pc; //for VPC updates
#endif
  DataType     data;  
  MsgType      mt;
  MsgAction    ma;

  MemObj       *creatorObj;
  MemObj       *homeMemObj; // Starting home node
  MemObj       *currMemObj;

#ifdef DEBUG_CALLPATH
  MemObj       *prevMemObj;
  std::vector<CallEdge> calledge;
  Time_t        lastCallTime;
#endif

  uint16_t      lineSize; // Request size, originally same as MemObj unless overruled

  CallbackBase *cb;

  int16_t       pendingSetStateAck;
  MemRequest   *setStateAckOrig;

  Time_t        startClock;

#ifdef SCOORE_VPC
  bool          vpc_update;
#endif
  bool          doStats;
  bool          retrying;

  DInst        *dinst;
  /* }}} */

  MemRequest();
  virtual ~MemRequest();

  void memReq();    // E.g: L1 -> L2
  void memReqAck(); // E.gL L2 -> L1 ack

  void memSetState();    // E.g: L2 -> L1
  void memSetStateAck(); // E.gL L1 -> L2 ack

  void memDisp();  // E.g: L1 -> L2

	friend class MRouter; // only mrouter can call the req directly
  void scheduleReq(TimeDelta_t       lat)  { doReqCB.schedule(lat); }
  void scheduleReqAck(TimeDelta_t    lat)  { doReqAckCB.schedule(lat); }
  void scheduleSetState(TimeDelta_t  lat)  { doSetStateCB.schedule(lat);          }
  void scheduleSetStateAck(TimeDelta_t   lat)  { doSetStateAckCB.schedule(lat);          }
  void scheduleDisp(TimeDelta_t   lat)  { doDispCB.schedule(lat); }

  void doReq();
  void doReqAck();
  void doSetState();
  void doSetStateAck();
  void doDisp();
  void setStateAckDone();

#ifdef DEBUG_CALLPATH
public:
  void rawdump_calledge(TimeDelta_t lat=0, Time_t total=0);
protected:
  void dump_calledge(TimeDelta_t lat, bool interesting=false);
  void upce();
#else
  void rawdump_calledge(TimeDelta_t lat=0, Time_t total=0) { };
  void dump_calledge(TimeDelta_t lat) { }
  void upce() { };
#endif
 public:
  static MemRequest *create(MemObj *m, AddrType addr, bool doStats, CallbackBase *cb);
  StaticCallbackMember0<MemRequest, &MemRequest::doReq>          doReqCB;
  StaticCallbackMember0<MemRequest, &MemRequest::doReqAck>       doReqAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::doSetState>     doSetStateCB;
  StaticCallbackMember0<MemRequest, &MemRequest::doSetStateAck>  doSetStateAckCB;
  StaticCallbackMember0<MemRequest, &MemRequest::doDisp>         doDispCB;

  void req();
  void scheduleReqAbs(Time_t       when)   { doReqCB.scheduleAbs(when); }

  void reqAck();
  void scheduleReqAckAbs(Time_t     when)  { doReqAckCB.scheduleAbs(when); }

  void setState();
  void scheduleSetStateAbs(Time_t   when)  { doSetStateCB.scheduleAbs(when);          }

  void setStateAck();
  void scheduleSetStateAckAbs(Time_t    when)  { doSetStateAckCB.scheduleAbs(when);          }

  void disp();
  void scheduleDispAbs(Time_t    when)  { doDispCB.scheduleAbs(when); }

  static void sendReqMMU(MemObj *m, bool doStats, AddrType addr, CallbackBase *cb=0) { 
    MemRequest *mreq = create(m,addr,doStats, cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_MMU; 
    mreq->dinst      = 0;
		mreq->req();
  }
  static void sendReqVPCWriteUpdate(MemObj *m, bool doStats, AddrType addr) { 
    MemRequest *mreq = create(m,addr,doStats, 0);
    mreq->mt         = mt_req;
    mreq->ma         = ma_VPCWU; 
		mreq->req();
  }
  static void sendReqRead(MemObj *m, DInst *dinst, AddrType addr, CallbackBase *cb=0) { 
    MemRequest *mreq = create(m,addr,dinst->getStatsFlag(), cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setValid; // For reads, MOES are valid states
    mreq->dinst      = dinst;
		mreq->req();
  }
  static void sendReqWrite(MemObj *m, DInst *dinst, AddrType addr, CallbackBase *cb=0) { 
    MemRequest *mreq = create(m,addr,dinst->getStatsFlag(), cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setDirty; // For writes, only MO are valid states
    mreq->dinst      = dinst;
		mreq->req();
  }
  static void sendReqWritePrefetch(MemObj *m, DInst *dinst, AddrType addr, CallbackBase *cb=0) { 
    MemRequest *mreq = create(m,addr,dinst->getStatsFlag(), cb);
    mreq->mt         = mt_req;
    mreq->ma         = ma_setDirty; 
    mreq->dinst      = dinst;
		mreq->req();
  }

  void convert2ReqAck(MsgAction _ma) {
		I(mt == mt_req);
		ma = _ma;
		mt = mt_reqAck;
  }
  void convert2SetStateAck(MemObj *obj) {
		I(mt == mt_setState);
		mt = mt_setStateAck;
    creatorObj = obj;
  }

  static void sendDisp(MemObj *m, AddrType addr, bool doStats) {
    MemRequest *mreq = create(m,addr,doStats, 0);
    mreq->mt         = mt_disp;
    mreq->ma         = ma_setDirty;
    mreq->dinst      = 0;
		mreq->disp();
  }

  static MemRequest *createSetState(MemObj *m, MemObj *creator, MsgAction ma, AddrType naddr, uint16_t size, bool doStats) {
    MemRequest *mreq = create(m,naddr,doStats, 0);
    mreq->mt         = mt_setState;
    mreq->ma         = ma;
    mreq->lineSize   = size;
    mreq->dinst      = 0;
    I(creator);
    mreq->creatorObj = creator;
    return mreq;
  }

	bool isReq() const         { return mt == mt_req; }
	bool isReqAck() const      { return mt == mt_reqAck; }
	bool isSetState() const    { return mt == mt_setState; }
	bool isSetStateAck() const { return mt == mt_setStateAck; }
	bool isDisp() const        { return mt == mt_disp; }

#ifdef DEBUG
  uint64_t getid() { return id;}
#endif
  void setNextHop(MemObj *newMemObj);

  void destroy();

  MemObj *getHomeNode() const  { return homeMemObj; }
  MemObj *getCreator() const   { return creatorObj; }
  bool isHomeNode()     const  { return homeMemObj == currMemObj; }
	MsgAction getAction() const  { return ma; }

  bool isMMU() const {return ma == ma_MMU; }
  bool isVPCWriteUpdate() const {return ma == ma_VPCWU; }

  void ack() {
    if(cb)
      cb->call();
    if(mt == mt_setStateAck)
      setStateAckDone();
          
    dump_calledge(0);
    destroy();
  }
  void ack(TimeDelta_t lat) {
    I(lat);

    if(cb) // Not all the request require a completion notification
      cb->schedule(lat);
    if(mt == mt_setStateAck)
      setStateAckDone();

    dump_calledge(lat);
    destroy();
  }
  void ackAbs(Time_t when) {
    I(when);
    if(cb)
      cb->scheduleAbs(when);
    dump_calledge(when-globalClock);
    destroy();
  }

  Time_t  getTimeDelay()  const { return globalClock-startClock; }

  AddrType getAddr() const { return addr; }
  DataType getData() const { return data; }
#ifdef SCOORE_VPC
  AddrType getPC()   const { return pc; }
#endif

  uint16_t getLineSize() const { return lineSize; };

  DInst *getDInst() const       { return dinst; }

#ifdef ENABLE_CUDA
  bool isSharedAddress() const { 
    if (dinst == 0x0) {
      if ((addr >> 61) == 6){
        return true;
      } 
      return false;
    }
    return dinst->isSharedAddress(); 
  } 
#endif

  bool getStatsFlag() const { return doStats; }
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
