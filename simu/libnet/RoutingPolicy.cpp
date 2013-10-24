#include <math.h>
#include "SescConf.h"
#include "RoutingPolicy.h"

/*******************************
       RoutingPolicy
*******************************/

RoutingPolicy::RoutingPolicy(const char *section, size_t ports) 
  : nRouters(SescConf->getRecordSize("","cpucore"))
  ,nPorts(ports)
  ,crossLat(SescConf->getInt(section, "crossLat"))
  ,wireLat(SescConf->getInt(section,"wireLat"))
{
  table.resize(nRouters);
}

void RoutingPolicy::make(const char* section)
{
  adjacent.resize(nRouters);
  next.resize(nRouters);
  for(size_t i=0; i < nRouters;i++){
    adjacent[i].resize(nRouters);
    for(size_t j = 0; j < nRouters;j++){
      adjacent[i][j].resize(1);
      adjacent[i][j][0].dist = INT_MAX;
    }
  }  

  create();

  for(RouterID_t i = 0; i < nRouters; i++) {
    shortestPaths(i);
  }

  for(RouterID_t i = 0; i < nRouters; i++) {
    table[i] = new RoutingTable(section, i, nRouters, getnRemotePorts());
    for(RouterID_t j = 0; j < nRouters; j++) {
      for(size_t k = 0; k < adjacent[i][j].size(); k++) {
	// i is source Router (or RoutingTable), j is destination, k are all
	// the paths available
	table[i]->addNeighborWire(j,adjacent[i][j][k]);
      }
    }
    if (next[i])
      table[i]->setNextWire(next[i]);
  }
#ifdef DEBUG
  //dump();
#endif
}

// Enhanced Dijkstra algorithm 
// It can find all best paths in the mean time
void RoutingPolicy::shortestPaths(RouterID_t dest)
{
  std::vector<bool> tentative(nRouters);

  uint32_t i;
  int32_t k,min;

  for(size_t i = 0; i < nRouters; i++) {
    tentative[i] = true;
  }

  if (adjacent[dest][dest].empty()) {
    adjacent[dest][dest].resize(1);
  }
  
  adjacent[dest][dest][0].rID  = dest;
  adjacent[dest][dest][0].dist = 0;
  adjacent[dest][dest][0].port = LOCAL_PORT1;
  tentative[dest] = false;
  k = dest;

  do {
    for (i = 0; i < nRouters; i++) {
      int32_t dist = adjacent[k][i][0].dist; // All the wires should have same dist
      
      if (dist != 0 && tentative[i]) {
	if (adjacent[dest][k][0].dist + dist + crossLat <= adjacent[dest][i][0].dist) {

	  if (adjacent[dest][k][0].dist + dist + crossLat < adjacent[dest][i][0].dist) {
	    adjacent[dest][i].clear();
	  }

	  MyWire wire;
	  I(k!=dest); // dist!=0 excludes this option

	  for (size_t opt = 0; opt < adjacent[dest][k].size(); opt++) {
	    wire.dist = adjacent[dest][k][opt].dist + dist + crossLat;
	    wire.rID  = adjacent[dest][k][opt].rID;
	    wire.port = adjacent[dest][k][opt].port;

	    if (adjacent[dest][k][0].port >= UP_PORT  && 
		adjacent[dest][k][0].port < LOCAL_PORT1) {
	      bool found = false;
	      for (size_t conta = 0 ; conta < adjacent[dest][i].size() ; conta++ ){
		if (adjacent[dest][i][conta] == wire ) {
		  found = true;
		  break;
		}
	      }
	      if (!found ) {
		adjacent[dest][i].push_back(wire);
	      }
	    }
	  }
	}
      }
    }
    
    k = -1; 
    min = INT_MAX;
    for (i = 0; i < nRouters; i++) {
      if (tentative[i] && adjacent[dest][i][0].dist < min) {
	min = adjacent[dest][i][0].dist;
	k = i;
      }
    }
    if(k != -1)
      tentative[k] = false;
    
  }while(k != -1);
}  

void RoutingPolicy::dump() const
{
  LOG("%d routers, %d ports/router", nRouters, nPorts);
  for(size_t i = 0; i < nRouters; i++) {
    char chain[256];

    sprintf(chain, "Next from %zu is %d", i, (int)next[i]->rID);

    next[i]->dump(chain);
    for(size_t j = 0; j < nRouters; j++) {
      for(size_t k = 0; k < adjacent[i][j].size(); k++) {
	char cadena[256];
	sprintf(cadena, "Route from %zu to %zu", i, j);
	adjacent[i][j][k].dump(cadena);
      }
    }
  }
}

/*******************************
  Specialized  RoutingPolicies
*******************************/

// Adjacent Matrix for mesh network
// '0' represents no link between the two nodes


void FullyConnectedRoutingPolicy::create()
{
  for(size_t i = 0; i < nRouters; i++) {
    PortID_t last = PortID_t(0);
    for(size_t j = 0; j < nRouters; j++) {
      if (i != j) {
	adjacent[i][j][0].rID = j;
	adjacent[i][j][0].port = last++;
	adjacent[i][j][0].dist = wireLat;
      }
    }
    int32_t k = (i + 1) % nRouters; /* next for broadcasts (see Message::type) */ 
    next[i] = &adjacent[i][k][0];
  }
}

void UniRingRoutingPolicy::create()
{
  for(size_t i = 0; i < nRouters; i++) {
    int32_t a;

    if (i == 0)
      a = nRouters - 1;
    else
      a = i - 1;

    adjacent[a][i][0].rID  = i;
    adjacent[a][i][0].port = UP_PORT;
    adjacent[a][i][0].dist = wireLat;

    next[a] = &adjacent[a][i][0];
  }
}

void BiRingRoutingPolicy::create()
{ 
  UniRingRoutingPolicy::create();

  for(size_t i = 0; i < nRouters; i++) {
    int32_t b;

    if (i == nRouters - 1)
      b = 0;
    else
      b = i + 1;

    adjacent[b][i][0].rID  = i;
    adjacent[b][i][0].port = DOWN_PORT;
    adjacent[b][i][0].dist = wireLat;
  }
}

// Adjacent Matrix for mesh network
// '0' represents no link between the two nodes
void MeshMultiPathRoutingPolicy::create()
{ 
  if(width == 2) {
    // 2x2 mesh
    adjacent[0][1][0].rID  = 1;
    adjacent[0][2][0].rID  = 2;
    adjacent[1][0][0].rID  = 0;
    adjacent[1][3][0].rID  = 3;
    adjacent[2][0][0].rID  = 0;
    adjacent[2][3][0].rID  = 3;
    adjacent[3][1][0].rID  = 1;
    adjacent[3][2][0].rID  = 2;

    adjacent[1][0][0].port = RIGHT_PORT;
    adjacent[2][0][0].port = DOWN_PORT;
    adjacent[0][1][0].port = LEFT_PORT;
    adjacent[3][1][0].port = DOWN_PORT;
    adjacent[0][2][0].port = UP_PORT;
    adjacent[3][2][0].port = RIGHT_PORT;
    adjacent[1][3][0].port = UP_PORT;
    adjacent[2][3][0].port = LEFT_PORT;

    adjacent[0][1][0].dist = 1;
    adjacent[0][2][0].dist = 1;
    adjacent[1][0][0].dist = 1;
    adjacent[1][3][0].dist = 1;
    adjacent[2][0][0].dist = 1;
    adjacent[2][3][0].dist = 1;
    adjacent[3][1][0].dist = 1;
    adjacent[3][2][0].dist = 1;
  }else{
    for(size_t i = 0; i < nRouters; i++) {
      size_t row = i/width;
      size_t col = i%width;

      if(row == 0) {
  // the first row
  adjacent[(width-1)*width+i][i][0].rID  = i;
  adjacent[(width-1)*width+i][i][0].port = UP_PORT;
  adjacent[(width-1)*width+i][i][0].dist = wireLat;

  adjacent[i+width][i][0].rID  = i;
  adjacent[i+width][i][0].port = DOWN_PORT;
  adjacent[i+width][i][0].dist = wireLat;
      }else if (row == width-1) {
  // the last row
  adjacent[i-width][i][0].rID  = i;
  adjacent[i-width][i][0].port = UP_PORT;
  adjacent[i-width][i][0].dist = wireLat;

  adjacent[col][i][0].rID  = i;
  adjacent[col][i][0].port = DOWN_PORT;
  adjacent[col][i][0].dist = wireLat;
      }else{
  adjacent[i-width][i][0].rID  = i;
  adjacent[i-width][i][0].port = UP_PORT;
  adjacent[i-width][i][0].dist = wireLat;

  adjacent[i+width][i][0].rID  = i;
  adjacent[i+width][i][0].port = DOWN_PORT;
  adjacent[i+width][i][0].dist = wireLat;
      }

      if(col == 0) {
  // the left column
  adjacent[i+width-1][i][0].rID  = i;
  adjacent[i+width-1][i][0].port = LEFT_PORT;
  adjacent[i+width-1][i][0].dist = wireLat;

  adjacent[i+1][i][0].rID  = i;
  adjacent[i+1][i][0].port = RIGHT_PORT;
  adjacent[i+1][i][0].dist = wireLat;
      }else if(col == width-1) {
  // the right column
  adjacent[i-1][i][0].rID  = i;
  adjacent[i-1][i][0].port = LEFT_PORT;
  adjacent[i-1][i][0].dist = wireLat;

  adjacent[i-width+1][i][0].rID  = i;
  adjacent[i-width+1][i][0].dist = wireLat;
  adjacent[i-width+1][i][0].port = RIGHT_PORT;
      }else{
  adjacent[i-1][i][0].rID = i;
  adjacent[i-1][i][0].port = LEFT_PORT;
  adjacent[i-1][i][0].dist = wireLat;

  adjacent[i+1][i][0].rID  = i;
  adjacent[i+1][i][0].port = RIGHT_PORT;
  adjacent[i+1][i][0].dist = wireLat;
      }
    } //for
  }
}

void HypercubeRoutingPolicy::create()
{
  for(size_t i = 0; i < nRouters; i++){
    for(size_t j = 0; j < (size_t)nPorts ; j++){
      size_t mask = 1 << j;
      size_t bit = i & mask;
      size_t nbor;
      MSG("rstats = %ud %ud %d %d\n",i,j,mask,bit);
      if(bit != 0){
        nbor = (size_t)(i - mask);
      }else{
        nbor = (size_t)(i + mask);
      }
      adjacent[i][nbor][0].rID = nbor;
      adjacent[i][nbor][0].port = PortID_t(j+1);
      adjacent[i][nbor][0].dist = wireLat;
    }
  }  
}
