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

#ifndef _INTER_CONN_H
#define _INTER_CONN_H

#include <vector>

#include "GStats.h"
#include "PMessage.h"
#include "Router.h"
#include "Snippets.h"
#include "estl.h"
#include "nanassert.h"

class ProtocolCBBase;
class RoutingPolicy;

// Class for comparison to be used in hashes of char * where the
// content is to be compared
class SectionComp {
public:
  inline bool operator()(const char *s1, const char *s2) const {
    return (strcasecmp(s1, s2) == 0);
  }
};

class InterConnection {
private:
  const char *descrSection;
  GStatsAvg   msgLatency;

  unsigned short linkBits;  // link width in bits [1..32700)
  float  linkBytes; // linkBits / 8

  // stores how many routers were already attached to by type of
  // object in the network
  typedef HASH_MAP<const char *, uint32_t, HASH<const char *>, SectionComp> ValHash;

  ValHash routersCtr;

  // stores which port to use based on the object that is attached to
  // the network
  typedef HASH_MAP<const char *, PortID_t, HASH<const char *>, SectionComp> PortHash;

  PortHash portsCtr;

  PortID_t portCtr;

protected:
  const char *netType;

  RoutingPolicy *rPolicy;

  std::vector<Router *> routers;

  void createRouters();
  void destroyRouters();

public:
  InterConnection(const char *section);
  ~InterConnection();

  const char *getDescrSection() const {
    return descrSection;
  }

  uint32_t getMaxRouters() const {
    return (uint32_t)routers.size();
  }

  uint32_t getNextFreeRouter(const char *section);

  unsigned short getLinkBits() const {
    return linkBits;
  }

  PortID_t getPort(const char *section);

  float getLinkBytes() const {
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
                                            // arrives to the "device" netID
                        ,
                        MessageType msgType //!< \param message type that netID is listening
                        ,
                        RouterID_t rID //!< \param router where netID is mapped
                        ,
                        PortID_t pID //!< \param port where netID is mapped
                        ,
                        NetDevice_t netID //!< \param device (== protocol) identifier netID
  );

  void sendMsg(Message *msg) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsg(routers[rID]);
  }

  void sendMsg(TimeDelta_t xlat, Message *msg) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsg(xlat, routers[rID]);
  }

  void sendMsgAbs(Time_t when, Message *msg) {
    RouterID_t rID = msg->getSrcRouterID();
    msg->launchMsgAbs(when, routers[rID]);
  }

  void dumpRouters();
  void dump();
};

class InterconnectionHashFunc {
public:
  size_t operator()(const InterConnection *ip) const {
    HASH<uint64_t> H;
    return H((uint64_t)ip);
  }
};

#endif //_INTER_CONN_H
