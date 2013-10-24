
#include <string>
#include <strings.h>

#include "SescConf.h"
#include "RoutingTable.h"

/*******************************
          RoutingTable
*******************************/

void RoutingTable::Wire::dump(const char *str) const 
{
  MSG("%s: next router %d through port %d in %d clk",str, rID, port, dist);
}

RoutingTable::RoutingTable(const char *section, RouterID_t id,  size_t size, PortID_t np)
  : myID(id)
    ,fixMessagePath(SescConf->getBool(section,"fixMessagePath"))
    ,nPorts(np)
{
  SescConf->isBool(section,"fixMessagePath");
  
  nextHop.resize(size);
}

RoutingTable::~RoutingTable()
{
}

void RoutingTable::addNeighborWire(RouterID_t id, const Wire &wire)
{
  while( nextHop.size() < id ) {
    nextHop.push_back(Wires4Router());
  }

  if( fixMessagePath ) {
    // Only keep one destination per router, but try to balance the number of ports used. So that
    // not all the packets go through the same port for all the destinations.
    if( !nextHop[id].succs.empty() ) {
      int32_t cntr[MAX_PORTS];
      bzero(cntr,sizeof(int)*MAX_PORTS);
      
      for(size_t i=0;i<nextHop.size();i++) {
	if( !nextHop[i].succs.empty() ) {
	  I(nextHop[i].succs[0].port>DISABLED_PORT);
	  I(nextHop[i].succs[0].port<=LOCAL_PORT1);
	  cntr[nextHop[i].succs[0].port]++;
	}
      }
      
      if( cntr[nextHop[id].succs[0].port] > cntr[wire.port] ) {
	// The new Wire uses a port less used
	nextHop[id].succs.clear();
      }else{
	// Too many of this port, ignore it
	return;
      }
    }
  }
  
  nextHop[id].succs.push_back(wire);
}

const RoutingTable::Wire *RoutingTable::getWire(RouterID_t destid) 
{
  size_t pos = nextHop[destid].prevTurn+1;
  I(!nextHop[destid].succs.empty());
  
  if( fixMessagePath || pos >= nextHop[destid].succs.size()  )
    pos=0;

  nextHop[destid].prevTurn = pos;
  
  return &nextHop[destid].succs[pos];
}

const RoutingTable::Wire *RoutingTable::getPortWire(RouterID_t id, PortID_t port) const 
{
  size_t nSuccs = nextHop[id].succs.size();
  
  for(size_t i=0 ; i<nSuccs ; i++ ) {
    if( nextHop[id].succs[i].port == port )
      return &nextHop[id].succs[i];
  }

  I(0);
  return 0;
}

void RoutingTable::dump()
{
  for(size_t i = 0; i < nextHop.size(); i++) {
    printf("From %d to %zu : ", myID, i);
    for (std::vector<Wire>::iterator iter = nextHop[i].succs.begin();
	 iter != nextHop[i].succs.end(); iter++) {
      printf(",%d port %d in %d clks ", iter->rID, iter->port, iter->dist);
    }
    printf("\n");
  }
}
