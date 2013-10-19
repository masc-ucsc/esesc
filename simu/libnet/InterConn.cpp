/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
		              Karin Strauss

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <time.h>
#include <sys/types.h>

#include "SescConf.h"

#include "InterConn.h"
#include "RoutingPolicy.h"

InterConnection::InterConnection(const char *section)
  : descrSection(strdup(section))
  ,msgLatency("%s_msgLatency",section)
  ,netType(strdup(SescConf->getCharPtr(section, "type")))
{
  if (strcasecmp(netType, "mesh") == 0) {
    rPolicy = new MeshMultiPathRoutingPolicy(section);
  } else if(strcasecmp(netType, "hypercube")==0){
    rPolicy = new HypercubeRoutingPolicy(section);
  } else if(strcasecmp(netType, "uniring")==0){
    rPolicy = new UniRingRoutingPolicy(section);
  } else if(strcasecmp(netType, "biring")==0){
    rPolicy = new BiRingRoutingPolicy(section);
  } else if(strcasecmp(netType, "full")==0){
    rPolicy = new FullyConnectedRoutingPolicy(section);
  } else {
    MSG("Unknown network Type <%s>", netType);
    I(0);
  }	
  createRouters();

  portCtr = LOCAL_PORT1;

  SescConf->isBetween(descrSection, "linkBits", 1, 32700);

  linkBits  = SescConf->getInt(section, "linkBits");
  linkBytes = (float) linkBits/8;

}

InterConnection::~InterConnection()
{
  destroyRouters();
}

void InterConnection::createRouters()
{
  routers.resize(rPolicy->getnRouters());
  
  // new Router objects
  for (RouterID_t i = 0; i < routers.size(); i++)
    routers[i] = new Router(descrSection, i, this, 
			    rPolicy->getRoutingTable(i));
}

void InterConnection::destroyRouters()
{
  for (RouterID_t i = 0; i < routers.size(); i++)
    delete routers[i];
}

void InterConnection::registerProtocol(ProtocolCBBase *pcb
				       ,MessageType msgType
				       ,RouterID_t rID
				       ,PortID_t pID
				       ,NetDevice_t netID)
{ 
  routers[rID]->registerProtocol(pcb,pID,PMessage::getUniqueProtID(msgType,netID));
}

uint32_t InterConnection::getNextFreeRouter(const char *section) 
{
  ValHash::iterator it = routersCtr.find(section);
  
  if (it == routersCtr.end()) 
    routersCtr[section] = 0;
  else
    routersCtr[section]++;
  
  return routersCtr[section];
}

PortID_t InterConnection::getPort(const char *section)
{
  
  PortHash::iterator it = portsCtr.find(section);
  
  if (it == portsCtr.end()) {
    portsCtr[section] = portCtr;
    portCtr++;

    if (portCtr >= MAX_PORTS) {
      // this can't be write, you cannot use more ports than available
      MSG("Too many ports being used, aborting simulation");
      abort();
    }  
  }  
 
  return portsCtr[section];
}

void InterConnection::dumpRouters()
{
  for (RouterID_t i = 0; i < routers.size(); i++)
    routers[i]->dump();
}		

PortID_t InterConnection::getnRemotePorts() const
{
  return rPolicy->getnRemotePorts();
}

void InterConnection::dump()
{
//   for (RouterID_t i = 0; i < routers.size(); i++) {
//     Router *r = routers[i];
//     for (PortID_t j = DISABLED_PORT; j < MAX_PORTS ; j++) {
//       if (r->getChan(j)->getVChan(0)->isOccupied())
// 	MSG("router %d has port %d occupied", i, j);
//     }
//   }	
}

void InterConnection::updateAvgMsgLatency(Time_t launchTime) 
{
  msgLatency.sample(globalClock - launchTime);
}

#if 0
class TestMessage : public Message {
  void garbageCollect();
  void trace(const char *format,...) {};
  int32_t getUniqueProtID() const {return 0;}
};
pool<TestMessage> tmpool;

void TestMessage::garbageCollect() 
{
    refCount--;
    if (refCount == 0) {
      LOG("destroying TestMessage");
      tmpool.in(this); 
    }
}

void InterConnection::test() 
{
  TestMessage* t;
  for (ushort i = 0; i < rPolicy->getnRouters(); i++) {
    t = tmpool.out();
    t->init(i, PortID_t(100), 0, PortID_t(101), Message::RCV_AND_PASS);
    t->setSize(100);
    routers[i]->launchMsg(t);
    t = tmpool.out();
    t->init(i, PortID_t(100), 0, PortID_t(101), Message::RCV);
    t->setSize(100);
    routers[i]->launchMsg(t);
  }
}
#endif
