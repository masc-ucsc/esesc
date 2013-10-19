#include <stdlib.h>
#include <list>

#include "SescConf.h"

#include "InterConn.h"
#include "Message.h"
#include "Router.h"

size_t     Message::gMsgCount = 0;

Message::Message() 
  : launchMsgCB(this)
    ,forwardMsgCB(this)
    ,receiveMsgCB(this)
    ,notifyMsgCB(this)
{
#ifdef DEBUG
  msgID = gMsgCount++;
#endif
}

void Message::forwardMsg(TimeDelta_t lat, Router *r) { 
  forwardMsgCB.schedule(lat,r);     
}

void Message::forwardMsgAbs(Time_t time , Router *r) { 
  forwardMsgCB.scheduleAbs(time,r); 
}

void Message::dump()
{
  MSG("MESSAGE #%d: src[%d:%d] dst[%d:%d] size[%d] latency[%lld]"
      ,msgID
      ,srcRouterID,srcPortID
      ,dstRouterID,dstPortID
      ,nSize
      ,globalClock - launchTime
      );
}

void Message::launchMsg(Router *router)
{
  launchTime = globalClock;
  
  router->launchMsg(this);
}

void Message::forwardMsg(Router *router)
{
  router->forwardMsg(this);
}

void Message::receiveMsg(Router *router)
{
  router->receiveMsg(this);
}

void Message::notifyMsg(Router *router)
{
  I(interConnection);
  interConnection->updateAvgMsgLatency(launchTime);
  
  router->notifyMsg(this);
}

void Message::setSize(uint32_t size) 
{
  nSize  = size;
}

void Message::setInterConnection(InterConnection *intercon) 
{
  I(intercon);
  interConnection = intercon;
}

