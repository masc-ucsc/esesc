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

#ifndef _INTER_CONN_H
#define _INTER_CONN_H

#include <vector>

#include "estl.h"
#include "GStats.h"
#include "nanassert.h"
#include "PMessage.h"
#include "Router.h"
#include "Snippets.h"

class ProtocolCBBase;
class RoutingPolicy;

// Class for comparison to be used in hashes of char * where the
// content is to be compared
class SectionComp {
public:
  inline bool operator()(const char* s1, const char* s2) const {
    return (strcasecmp(s1, s2) == 0);
  }
}; 

class InterConnection {
private:
  const char *descrSection;
  GStatsAvg msgLatency;
  
  ushort linkBits; // link width in bits [1..32700)
  float linkBytes; // linkBits / 8
  
  // stores how many routers were already attached to by type of
  // object in the network
  typedef HASH_MAP<const char *, uint32_t, 
                   HASH<const char*>, SectionComp> ValHash;

  ValHash routersCtr; 

  // stores which port to use based on the object that is attached to
  // the network
  typedef HASH_MAP<const char *, PortID_t, 
                   HASH<const char*>, SectionComp> PortHash;

  PortHash portsCtr;

  PortID_t portCtr;

protected:
  const char   *netType;

  RoutingPolicy *rPolicy;

  std::vector<Router *> routers;

  void createRouters();
  void destroyRouters();

public:
  InterConnection(const char *section);
  ~InterConnection();

  const char *getDescrSection() {
    return descrSection;
  }

  uint32_t getMaxRouters() {
    return (uint32_t) routers.size();
  }

  uint32_t getNextFreeRouter(const char *section); 

  ushort getLinkBits() {
    return linkBits;
  }

  PortID_t getPort(const char *section);

  float getLinkBytes() {
    return linkBytes;
  }

  void updateAvgMsgLatency(Time_t launchTime);

  Router *getRouter(RouterID_t id) const {
    I(id < routers.size());
    return routers[id];
  }

  // To be removed
  size_t getnRouters() const {
    return routers.size();
  }
  
  // To be removed
  PortID_t getnRemotePorts() const;

  void registerProtocol(ProtocolCBBase *pcb //!< \param protocol callback to be invoqued when a msg
					    //arrives to the "device" netID
			,MessageType msgType//!< \param message type that netID is listening
			,RouterID_t rID     //!< \param router where netID is mapped
			,PortID_t pID       //!< \param port where netID is mapped
			,NetDevice_t netID  //!< \param device (== protocol) identifier netID
    );

  void sendMsg(Message *msg) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsg(routers[rID]);
  }

  void sendMsg(TimeDelta_t xlat, Message *msg ) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsg(xlat,routers[rID]);
  }

  void sendMsgAbs(Time_t when, Message *msg) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsgAbs(when,routers[rID]);
  }

  void dumpRouters();
  void dump();

};

class InterconnectionHashFunc {
public: 
  size_t operator()(const InterConnection *ip) const {
	 HASH<uint32_t> H;
	 return H((uint32_t) ip);
  }
};


#endif //_INTER_CONN_H
