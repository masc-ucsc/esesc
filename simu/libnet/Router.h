#ifndef _ROUTER_H
#define _ROUTER_H

#include <vector>
#include <list>

#include "EnergyMgr.h"
#include "estl.h"
#include "nanassert.h"
#include "Port.h"
#include "ProtocolCB.h"
#include "RoutingTable.h"

class Message;
class InterConnection;

/*! \class Router
 * \brief Low level routing of packets (level 2 in ISO)
 *
 * Class that routes packets. Each router can have a collection of NetDevices attached. Each packet
 * goes through routers using the RoutingTable to select the path. A message is injected with
 * launchMsg, if the destination is in another router forwardMsg is called as many times a routers
 * involved. Once the message arrives to the destination, receiveMsg is invoked.
 *
 * Routers have virtual channels with turn based port selection. (The best according to Martinez)
 *
 * This implementation of Router assume in many places that the links are unidirectional. This means
 * that for each port there are two dedicated links (input and output).
 *
 */

class Router {
private:
  const RouterID_t myID;

  // Begin Configuration parameters
  const TimeDelta_t crossLat; //!< Router crossing latency [0..32700)

  const ushort      localNum; //!< Number of addressable local ports [1..MAX_PORTS-LOCAL_PORT1)
  const TimeDelta_t localLat; //!< Local port latency [0..32700)
  const TimeDelta_t localOcc; //!< Local port occupancy [0..32700)
  const ushort      localPort;//!< Number of ports for each addressable local port [0..32700)

  const bool congestionFree;     //!< Skip the router modeling (just local ports)
  const TimeDelta_t addFixDelay; //!< fix delay to add to the network forwarding
  // End Configuration parameters

  PortID_t maxLocalPort;

  InterConnection *net; //!< The network where the router is mapped

  RoutingTable *rTable;

  typedef HASH_MAP<int32_t,ProtocolCBBase*> ProtHandlersType;

  ProtHandlersType localPortProtocol;

  std::vector<PortGeneric *> l2rPort; //!< ports from local device to router
  std::vector<PortGeneric *> r2lPort; //!< ports from router to local device
  std::vector<PortGeneric *> r2rPort; //!< ports from router to router (output)

protected:

  ushort calcNumFlits(Message *msg) const;

public:	
  Router(const char *section, RouterID_t id, InterConnection *n, RoutingTable *rt);
  virtual ~Router();

  void launchMsg(Message *msg);  //!< Called when a new message is injected in the router.
  void forwardMsg(Message *msg); //!< Called to forward a message from router to router
  void receiveMsg(Message *msg); //!< Called when the first flit can arrive to the destination
  void notifyMsg(Message *msg);  //!< Called when the notification flit arrives to the destination

  //!< Register a protocol callback with unique id in the portID
  void registerProtocol(ProtocolCBBase *pcb, PortID_t pID, int32_t id);
  
  void dump();

  RouterID_t getMyID() const { 
    return myID; 
  }

};

#endif //_ROUTER_H
