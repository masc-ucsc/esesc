#include "NetAddrMap.h"

NetAddrMap::MapType NetAddrMap::type;
uint32_t NetAddrMap::log2TileSize = 10;
uint32_t NetAddrMap::numNodes = 1;
uint32_t NetAddrMap::numNodesMask = 0x0;
