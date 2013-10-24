#ifndef ROUTINGPOLICY_H
#define ROUTINGPOLICY_H

#include <vector>

#include "nanassert.h"

#include "Snippets.h"

#include "RoutingTable.h"

// This class builds the routing tables. There should be an instance per network. The reason why is
// separated from the network is that it makes it easier to have different policies like mesh,
// torus, hyper-cube...

class RoutingPolicy {
protected:
  const size_t nRouters;
  const size_t nPorts;
  const TimeDelta_t crossLat;
  const TimeDelta_t wireLat;
  std::vector<RoutingTable *> table;

  class MyWire : public RoutingTable::Wire {
  public:
    MyWire() : RoutingTable::Wire(0,DISABLED_PORT,0) {
    }
  };
  
  std::vector< std::vector< std::vector<MyWire> > > adjacent;
  std::vector<MyWire*> next;

  void shortestPaths(RouterID_t dst);

  RoutingPolicy(const char *section, size_t ports);

  void make(const char* section);

  virtual void create() = 0;
public:
  virtual ~RoutingPolicy() {
  }

  RoutingTable *getRoutingTable(RouterID_t id) {
    I(id<table.size());
    return table[id];
  }

  PortID_t getnRemotePorts() const { return (PortID_t)nPorts; }

  void dump() const;
  size_t getnRouters() const {
    return nRouters;
  }
  
};

class FullyConnectedRoutingPolicy : public RoutingPolicy {
protected:
  void create();

public:
  FullyConnectedRoutingPolicy(const char *section)
    : RoutingPolicy(section,SescConf->getRecordSize("","cpucore") - 1)
    { make(section); }
};

//unidirection ring
class UniRingRoutingPolicy : public RoutingPolicy {
protected:
  void create();

public:
  UniRingRoutingPolicy(const char *section) : RoutingPolicy(section, 1) 
  { make(section); }
  UniRingRoutingPolicy(const char *section, size_t ports) : RoutingPolicy(section, ports)
  { /* called from bidirectional ring */ }
};

//bidirectional ring
class BiRingRoutingPolicy : public UniRingRoutingPolicy {
protected:
  void create();

public:
  BiRingRoutingPolicy(const char *section) : UniRingRoutingPolicy(section,2) 
  { make(section); }
};

class MeshMultiPathRoutingPolicy : public RoutingPolicy {
private:
  const size_t width;

protected:
  void create();

public:
  MeshMultiPathRoutingPolicy(const char *section)
    : RoutingPolicy(section,4)
    ,width(SescConf->getInt(section, "width")) {
    SescConf->isBetween(section, "width",1,128);
    make(section);
  }
};


class HypercubeRoutingPolicy : public RoutingPolicy {
protected:
  void create();

public:
  HypercubeRoutingPolicy(const char *section)
    : RoutingPolicy(section, log2i((int)SescConf->getRecordSize("","cpucore")))
  {
    SescConf->isPower2(section,"hyperNumProcs");
    make(section);
  }
};

#endif // ROUTINGPOLICY_H
