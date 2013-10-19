
#include <typeinfo>

#include "ProtocolBase.h"
#include "PMessage.h"
#include "InterConn.h"

ProtocolBase::NetDevType ProtocolBase::netDev;


ProtocolBase::ProtocolBase(InterConnection *net, RouterID_t rID,PortID_t pID)
  : network(net)
  , routerID(rID)
  , portID(pID)
{
  I(network);
 
  NetDevType::iterator netDevEntry = netDev.find(network);

  if(netDevEntry == netDev.end()) {
    // create a new device vector for network
    
    netDev[network] = new DevList;
    netDevEntry = netDev.find(network);
  }
  
  DevList *devList = netDevEntry->second;
  
  myID = devList->size();
  devList->push_back(this);
  //LOG("ProtocolBase: successfully added ProtocolBase id=%d (%p) to network entry %p",
  //    myID, this, network);
}

void ProtocolBase::registerHandler(ProtocolCBBase *pcb, MessageType msgType) 
{
  network->registerProtocol(pcb,msgType,routerID,portID,myID);
}

ProtocolBase *ProtocolBase::getProtocolBase(NetDevice_t netID) 
{
    I(network);
    
    NetDevType::iterator it = netDev.find(network);
    I(it != netDev.end());
    
    DevList *devList = (*it).second;

    I(netID < devList->size());
   
    return (*(devList))[netID];
}
