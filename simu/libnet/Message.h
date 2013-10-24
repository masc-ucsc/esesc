#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "estl.h"
#include <math.h>

#include "nanassert.h"
#include "Snippets.h"
#include "callback.h"

#include "NetIdentifiers.h"

class Router;
class InterConnection;

class Message {
public:
  enum DeliveryType_t {PT_TO_PT, RCV_AND_PASS, RCV};
  /*
    PT_TO_PT is a normal point-to-point message
    RCV_AND_PASS is for broadcast messages -- Router sends message up
      to cache and to neighbor
    RCV -- Router sends message to subsequent processor (ask Karin)
  */

private:
  static size_t gMsgCount;   //!< Counter used to assign unique msgID

  // Begin of callback interface used by Router. 

  // Callbacks are here instead of Router because of performance
  // reasons. Being here, it is possible to use static
  // callbacks. (Same performance reasons than DInst)

protected:
  // launchMsg is directly accesible by InterConnection
  void launchMsg(Router *router);

private:
  StaticCallbackMember1<Message,Router *,&Message::launchMsg> launchMsgCB;

  void forwardMsg(Router *router);
  StaticCallbackMember1<Message,Router *,&Message::forwardMsg> forwardMsgCB;

  void receiveMsg(Router *router);
  StaticCallbackMember1<Message,Router *,&Message::receiveMsg> receiveMsgCB;

  void notifyMsg(Router *router);
  StaticCallbackMember1<Message,Router *,&Message::notifyMsg> notifyMsgCB;

  // End of callback interface used by Router

protected:

  InterConnection *interConnection;

  DeliveryType_t delivery;
  bool   finished;

  RouterID_t srcRouterID;
  PortID_t   srcPortID;

  RouterID_t dstRouterID;
  PortID_t   dstPortID;

  size_t msgID;  //!< Msg unique ID. Notice that a message can be reused, 
                 // and the same ID can be found in different messages 
                 // through time.

  Time_t launchTime; //!< Time when the msg was launched
  
  size_t nSize;

  uint32_t refCount; // number of references counter 

  Message();
  virtual ~Message() {
    // Useless, but removes a warning in gcc
  }

   // Should be called through setupMessage or similars in PMessage or descendents
  void init(RouterID_t src, PortID_t srcPort, RouterID_t dst, PortID_t dstPort,
	    DeliveryType_t d = PT_TO_PT) {
    srcRouterID = src;
    srcPortID   = srcPort;
    
    dstRouterID = dst;
    dstPortID   = dstPort;

    delivery = d;
    finished = false;
    refCount = 1;

    IS(nSize   = 0); // setSize must be called later
  }


  // Begin interface used by Router

  friend class Router;
  friend class InterConnection;

  void launchMsg(TimeDelta_t lat, Router *r)  { 
    launchMsgCB.schedule(lat,r);      
  }
  void launchMsgAbs(Time_t time , Router *r)  { 
    launchMsgCB.scheduleAbs(time,r);  
  }
  void receiveMsg(TimeDelta_t lat, Router *r) { 
    receiveMsgCB.schedule(lat,r);     
  }
  void receiveMsgAbs(Time_t time , Router *r) { 
    receiveMsgCB.scheduleAbs(time,r); 
  }
  void forwardMsg(TimeDelta_t lat, Router *r);
  void forwardMsgAbs(Time_t time , Router *r);
  void notifyMsg(TimeDelta_t lat, Router *r)  { 
    notifyMsgCB.schedule(lat,r);      
  }
  void notifyMsgAbs(Time_t time , Router *r)  { 
    notifyMsgCB.scheduleAbs(time,r);  
  }

  // End interface used by Router

public:

  // Standard way to destroy messages. The inheriting class should define it.
  virtual void garbageCollect() = 0;
  Message* duplicate() { refCount++; return this; }
  virtual void trace(const char *format,...) = 0;

#ifdef DEBUG
  size_t getMsgID() const { 
    return msgID; 
  }
#endif

  virtual int32_t getUniqueProtID() const = 0;
  
  void setSize(uint32_t size);

  size_t getSize() const {
    return nSize;
  }

  RouterID_t getDstRouterID() const {
    return dstRouterID;
  }

  void setDstRouterID(RouterID_t id) {
    dstRouterID = id;
  }

  PortID_t getDstPortID() const {
    return dstPortID;
  }

  RouterID_t getSrcRouterID() const {
    return srcRouterID;
  }

  PortID_t getSrcPortID() const {
    return srcPortID;
  }

  DeliveryType_t getDelivery() const {
    return delivery;
  }

  bool isFinished() const { return finished; }
  void setFinished() { finished = true; }

  virtual void setInterConnection(InterConnection *intercon);
  void dump();
};

class MsgPtrHashFunc {
public: 
  size_t operator()(const Message *msg) const {
    HASH<const char *> H;
    return H((const char *) msg);
  }
};


#endif //_MESSAGE_H
