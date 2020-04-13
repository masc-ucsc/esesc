// Contributed by Jose Renau
//	              Karin Strauss
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

#include <sys/types.h>
#include <time.h>

#include "SescConf.h"

#include "InterConn.h"
#include "RoutingPolicy.h"

InterConnection::InterConnection(const char *section)
    : descrSection(strdup(section))
    , msgLatency("%s_msgLatency", section)
    , netType(strdup(SescConf->getCharPtr(section, "type"))) {
  if(strcasecmp(netType, "mesh") == 0) {
    rPolicy = new MeshMultiPathRoutingPolicy(section);
  } else if(strcasecmp(netType, "hypercube") == 0) {
    rPolicy = new HypercubeRoutingPolicy(section);
  } else if(strcasecmp(netType, "uniring") == 0) {
    rPolicy = new UniRingRoutingPolicy(section);
  } else if(strcasecmp(netType, "biring") == 0) {
    rPolicy = new BiRingRoutingPolicy(section);
  } else if(strcasecmp(netType, "full") == 0) {
    rPolicy = new FullyConnectedRoutingPolicy(section);
  } else {
    MSG("Unknown network Type <%s>", netType);
    I(0);
  }
  createRouters();

  portCtr = LOCAL_PORT1;

  SescConf->isBetween(descrSection, "linkBits", 1, 32700);

  linkBits  = SescConf->getInt(section, "linkBits");
  linkBytes = (float)linkBits / 8;
}

InterConnection::~InterConnection() {
  destroyRouters();
}

void InterConnection::createRouters() {
  routers.resize(rPolicy->getnRouters());

  // new Router objects
  for(RouterID_t i = 0; i < routers.size(); i++)
    routers[i] = new Router(descrSection, i, this, rPolicy->getRoutingTable(i));
}

void InterConnection::destroyRouters() {
  for(RouterID_t i = 0; i < routers.size(); i++)
    delete routers[i];
}

void InterConnection::registerProtocol(ProtocolCBBase *pcb, MessageType msgType, RouterID_t rID, PortID_t pID, NetDevice_t netID) {
  routers[rID]->registerProtocol(pcb, pID, PMessage::getUniqueProtID(msgType, netID));
}

uint32_t InterConnection::getNextFreeRouter(const char *section) {
  ValHash::iterator it = routersCtr.find(section);

  if(it == routersCtr.end())
    routersCtr[section] = 0;
  else
    routersCtr[section]++;

  return routersCtr[section];
}

PortID_t InterConnection::getPort(const char *section) {

  PortHash::iterator it = portsCtr.find(section);

  if(it == portsCtr.end()) {
    portsCtr[section] = portCtr;
    portCtr++;

    if(portCtr >= MAX_PORTS) {
      // this can't be write, you cannot use more ports than available
      MSG("Too many ports being used, aborting simulation");
      abort();
    }
  }

  return portsCtr[section];
}

void InterConnection::dumpRouters() {
  for(RouterID_t i = 0; i < routers.size(); i++)
    routers[i]->dump();
}

PortID_t InterConnection::getnRemotePorts() const {
  return rPolicy->getnRemotePorts();
}

void InterConnection::dump() {
  //   for (RouterID_t i = 0; i < routers.size(); i++) {
  //     Router *r = routers[i];
  //     for (PortID_t j = DISABLED_PORT; j < MAX_PORTS ; j++) {
  //       if (r->getChan(j)->getVChan(0)->isOccupied())
  // 	MSG("router %d has port %d occupied", i, j);
  //     }
  //   }
}

void InterConnection::updateAvgMsgLatency(Time_t launchTime) {
  // FIXME: add getstatsflag
  // msgLatency.sample(globalClock - launchTime);
}

