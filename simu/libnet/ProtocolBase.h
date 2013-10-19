#ifndef PROTOCOLBASE_H
#define PROTOCOLBASE_H

// Simpler and cleaner interface to manage the low level network
// interface
#include <vector>

#include "estl.h"
#include "InterConn.h"
#include "nanassert.h"
#include "ProtocolCB.h"
#include "PMessage.h"

class ProtocolBase {
private:
  typedef std::vector<ProtocolBase *> DevList;
  typedef HASH_MAP<InterConnection *, std::vector<ProtocolBase *> *, InterconnectionHashFunc> NetDevType;
  
  static NetDevType netDev;

protected:
  NetDevice_t myID; // position inside netDev (&netDev[myID] == this)

  // Low level Information where the device is mapped
  InterConnection *network;
  const RouterID_t  routerID;
  const PortID_t    portID;

  // Map a device to a local device. Unless the portID is specified,
  // it can use any of the local ports.
  ProtocolBase(InterConnection *net, 
	       RouterID_t rID, 
	       PortID_t pID = LOCAL_PORT1);

  void registerHandler(ProtocolCBBase *pcb, MessageType msgType);

  void sendMsg(PMessage *msg) {
    network->sendMsg(msg);
  }

  void sendMsg(TimeDelta_t xlat, PMessage *msg) {
    network->sendMsg(xlat, msg);
  }

  void sendMsgAbs(Time_t when, PMessage *msg) {
    network->sendMsgAbs(when,msg);
  }

public:
  
  NetDevice_t getNetDeviceID() const {
    return myID;
  }
  
  RouterID_t getRouterID() const {
    return routerID;
  }
  
  PortID_t getPortID() const {
    return portID;
  }

  ProtocolBase *getProtocolBase(NetDevice_t netID);
};

#endif // PROTOCOLBASE_H

