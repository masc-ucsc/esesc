
#include <limits.h>

#include "SescConf.h"
#include "Router.h"
#include "InterConn.h"

Router::Router(const char *section, RouterID_t id, 
	       InterConnection *n, RoutingTable *rt)
  : myID(id)
   ,crossLat(SescConf->getInt(section, "crossLat"))
   ,localNum(SescConf->getInt(section, "localNum"))
   ,localLat(SescConf->getInt(section, "localLat"))
   ,localOcc(SescConf->getInt(section, "localOcc"))
   ,localPort(SescConf->getInt(section, "localPort"))
   ,congestionFree(SescConf->getBool(section, "congestionFree"))
   ,addFixDelay(SescConf->getBool(section, "addFixDelay"))
   ,net(n)
   ,rTable(rt)

{
  SescConf->isBetween(section , "crossLat",0,32700);

  SescConf->isBetween(section , "localNum",1,MAX_PORTS-LOCAL_PORT1);
  SescConf->isBetween(section , "localLat",0,32700);
  SescConf->isBetween(section , "localOcc",0,32700);
  SescConf->isBetween(section , "localPort",0,32700);

  SescConf->isInt(section, "addFixDelay");
  SescConf->isBetween(section, "addFixDelay",0,32768); // TimeDelta_t

  SescConf->isBool(section, "congestionFree");

  maxLocalPort = PortID_t(static_cast<int>(LOCAL_PORT1)+localNum);
  
  I(maxLocalPort>LOCAL_PORT1);
  I(maxLocalPort<MAX_PORTS);

  // Initialize ports
  l2rPort.resize(MAX_PORTS);
  r2lPort.resize(MAX_PORTS);
  r2rPort.resize(MAX_PORTS);

  for(PortID_t i=LOCAL_PORT1;i<maxLocalPort;i++) {
    char name[256];
    int32_t ret = snprintf(name,127,"l2rPort(%d-%d)",myID,i);
    I(ret > 0);
    l2rPort[i] = PortGeneric::create(name,localPort,localOcc);
    
    ret = snprintf(name,127,"r2lPort(%d-%d)",myID,i);
    I(ret > 0);
    r2lPort[i] = PortGeneric::create(name,localPort,localOcc);
  }
  for(PortID_t i=DISABLED_PORT;i<rTable->getnPorts();i++) {
    char name[256];
    int32_t ret = snprintf(name,127,"r2rPort(%d-%d)",myID,i+1);
    I(ret > 0);

    r2rPort[i+1] = PortGeneric::create(name,1,1);
  }
  

#ifdef DEBUG
  // Make sure that if other ports are used a segfault is generated
  for(PortID_t i=DISABLED_PORT;i<LOCAL_PORT1;i++) {
    l2rPort[i] = 0;
    r2lPort[i] = 0;
  }
#endif

}

Router::~Router()
{
  // nothing to do
}

void Router::launchMsg(Message *msg)
{

#ifdef DEBUG
#if 0
  MSG("router(%d)::launchMsg [%d:%d] to [%d:%d]",
      myID,
      msg->getSrcRouterID(),
      msg->getSrcPortID(),
      msg->getDstRouterID(),
      msg->getDstPortID());
#endif
#endif

  I(msg->getSrcRouterID() == myID);

  // bufferEnergy.inc() ;  // read

  msg->setInterConnection(net);

  PortID_t portid = msg->getSrcPortID();
  I(l2rPort[portid]);

  if (msg->getDelivery() == Message::RCV)
    msg->setDstRouterID(rTable->getNextWire()->rID);
  else if (msg->getDelivery() == Message::RCV_AND_PASS)
    msg->setDstRouterID(myID);

  Router *dstRouter = net->getRouter(msg->getDstRouterID());

  if (msg->getDstRouterID() == myID && msg->getDstPortID() == portid &&
      msg->getDelivery() == Message::PT_TO_PT) {

    I(msg->getDstPortID() == portid);

    // If same router and port, do not model contention for input
    // because it is modeled as contention for output in receiveMsg
    dstRouter->receiveMsg(msg);

  } else {

    Time_t when = l2rPort[portid]->occupySlots(calcNumFlits(msg));

    when+=addFixDelay;
  
    if (congestionFree)
      msg->receiveMsgAbs(when,dstRouter);
    else
      msg->forwardMsgAbs(when, this);
  }
}

void Router::forwardMsg(Message *msg)
{
   // routerEnergy.inc() ;
  
  // called when a packet head is already in the router, and it needs
  // to be forwarded to another router.

  // receive message if either this is the destination or it is a finished
  // RCV_AND_PASS message
  if( msg->getDstRouterID() == myID && 
      ( (msg->getDelivery() != Message::RCV_AND_PASS) || msg->isFinished() ) ) {
    receiveMsg(msg);
    return;
  }

  // linkEnergy.inc();

  const RoutingTable::Wire *wire;

  switch (msg->getDelivery()) {
  case Message::RCV_AND_PASS:
    if (msg->getDstRouterID() != myID) { /* don't receive message now;
					    wait until it makes a loop
					    then receive above */
      msg->duplicate();
      receiveMsg(msg);
    }
    wire = rTable->getNextWire();
    if (msg->getDstRouterID() == wire->rID) /* finished loop */
      msg->setFinished();
    break;
  case Message::RCV:                     /* RCV messages: dest set in launchMsg() */
  case Message::PT_TO_PT:
    wire = rTable->getWire(msg->getDstRouterID());
    break;
  default:
    LOG("Message delivery = %d", msg->getDelivery());
    I(0);
    wire = 0;
  }

  Time_t when = r2rPort[wire->port]->occupySlots(calcNumFlits(msg));

  // MSG("%lld router::forwardMsg %d->%d",globalClock,myID, wire->rID);

  msg->forwardMsgAbs(when+crossLat+wire->dist, net->getRouter(wire->rID));
}

void Router::receiveMsg(Message *msg)
{
  I(msg->getDstRouterID() == myID || msg->getDelivery() == Message::RCV_AND_PASS);

  // bufferEnergy.inc() ;  // write

  PortID_t portid = msg->getDstPortID();
  I(r2lPort[portid]);

  Time_t when = r2lPort[portid]->occupySlots(calcNumFlits(msg));

  // MSG("dstport:%d srcport:%d router:%d",portid,msg->getSrcPortID(),myID);
  msg->notifyMsgAbs(when + calcNumFlits(msg), this);
}

void Router::notifyMsg(Message *msg)
{
  I(msg->getDstRouterID() == myID || msg->getDelivery() == Message::RCV_AND_PASS);

  // Karin: remove the following if block to enable processing
  // of RCV_AND_PASS & RCV messages
  if (msg->getDelivery() != Message::PT_TO_PT) {
    LOG("[%zu] received %s message from %zu", myID, 
	(msg->getDelivery() == Message::RCV_AND_PASS) ? "RCV_AND_PASS" : "RCV", 
	 msg->getSrcRouterID());
    msg->garbageCollect();
    return;
  }

  // There is no queue because a "device" always swallows
  // messages. Currently, there is no API for the ProtocolBase to
  // notify the network that it does not want a message. (Too many
  // freaking details, sorry)

  ProtHandlersType::iterator it = localPortProtocol.find(msg->getUniqueProtID());
  GLOG(it==localPortProtocol.end(),
       "Router[%d]::receiveMsg no one accepts packet in router[%d:%d] (uniqueID=%ld)\n", 
       myID, 
       msg->getDstRouterID(),
       msg->getDstPortID(),
       msg->getUniqueProtID());

  it->second->call(msg);

  // TODO: decrease destroy. (GC destroy)
}

void Router::dump()
{
  fprintf(stderr, "Router #%d\n", myID);
}

void Router::registerProtocol(ProtocolCBBase *pcb, PortID_t pID, int32_t id)
{
  I(localPortProtocol.find(id) == localPortProtocol.end());
  localPortProtocol[id] = pcb;
}

ushort Router::calcNumFlits(Message *msg) const {
  return (ushort) ceil(msg->getSize() / net->getLinkBytes());
}

