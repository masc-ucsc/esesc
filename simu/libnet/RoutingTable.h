#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#include <vector>

#include "nanassert.h"
#include "Snippets.h"
#include "NetIdentifiers.h"

class RoutingTable {
public:
  class Wire {
  public:
    Wire(RouterID_t    r
	 ,PortID_t     p
	 ,TimeDelta_t  d
      ) {
      rID  = r;
      port = p;
      dist = d;
    }
    
    RouterID_t    rID;   // Next Router In that wire
    PortID_t      port;  // Local port to reach routerID
    TimeDelta_t   dist;  // distance to reach routerID

    bool operator==(const Wire &w2) {
      return rID == w2.rID && port == w2.port && dist == w2.dist;
    }

    void dump(const char *str) const;
  };

protected:
  const RouterID_t myID;
  const bool fixMessagePath;
  const PortID_t nPorts;

  class Wires4Router {
  public:
    Wires4Router() {
      prevTurn =0;
    }
    std::vector<Wire> succs;
    int32_t prevTurn;
  };
  
  std::vector<Wires4Router> nextHop;
  Wire* next; /* next in broadcast (see Message::Type or ask Karin) */

  const Wire *getPortWire(RouterID_t id, PortID_t port) const;

public:
  
  RoutingTable(const char *section, RouterID_t id, size_t size, PortID_t np);
  virtual ~RoutingTable();

  const Wire *getWire(RouterID_t destid);

  void setNextWire(Wire* w) { next = w; }
  const Wire *getNextWire() const { return next; }

  PortID_t getnPorts() const {
    return nPorts;
  }

  void addNeighborWire(RouterID_t id, const Wire &succs);

  const Wire *getUpWire(RouterID_t id) const { 
    return getPortWire(id,UP_PORT);
  }
  const Wire *getDownWire(RouterID_t id) const { 
    return getPortWire(id,DOWN_PORT);
  }
  const Wire *getLeftWire(RouterID_t id) const { 
    return getPortWire(id,LEFT_PORT);
  }
  const Wire *getRightWire(RouterID_t id) const { 
    return getPortWire(id,RIGHT_PORT);
  }

  void dump();
};

#endif // ROUTINGTABLE_H
