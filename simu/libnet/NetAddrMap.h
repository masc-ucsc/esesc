#ifndef NETADDRMAP_H
#define NETADDRMAP_H

#include "SescConf.h"
//#include "ThreadContext.h" Does not appear in esesc
#include "PMessage.h"

class NetAddrMap {
protected:
  // This should correspond to the number of physical addresses mapped
  // to a particular node, this is independent of the cache size on the
  // chip. It is only a function of the size of physical memory

  typedef enum { blocked, strided } MapType;

  static MapType type;

  static uint32_t log2TileSize;
  static uint32_t numNodes;
  static uint32_t numNodesMask;

public:
  static void InitMap(char *descr_section) {
    /*
     * useNetwork
     */
    bool useNetwork = SescConf->getBool(descr_section, "useNetwork");

    if(useNetwork) {
      numNodes = SescConf->getInt("gNetwork", "width");
      numNodes *= numNodes;
    } else {
      // FIXME: procsPerNode removed from config file
      // int32_t nProcsPerNode = SescConf->getInt("","procsPerNode");
      /* Commented out because procPerNode and NProcesperNode not in config file
      LOG("NetAddrMap: nProcsPerNode = %d", nProcsPerNode);
      numNodes = SescConf->getRecordSize("","cpucore");
      LOG("NetAddrMap: numNodes = %ld", numNodes);
      numNodes = numNodes / nProcsPerNode;
      if (numNodes==0)
  numNodes = 1;

      LOG("NetAddrMap: final numNodes = %ld", numNodes);
     */
      printf("Need procsPerNode and nProcsPerNode here\n");
    }

    uint32_t temp = 1;
    while(temp < numNodes - 1) {
      temp = temp << 1;
      temp++;
    }
    numNodesMask = temp;

    log2TileSize = SescConf->getInt(descr_section, "log2TileSize");

    if(strcmp(SescConf->getCharPtr(descr_section, "MapType"), "blocked") == 0) {
      type = blocked;
    } else {
      type = strided;
    }
  }

  static NetDevice_t getMapping(AddrType paddr) {
    if(type == blocked) {
      return paddr >> log2TileSize;
    } else {
      uint32_t temp;
      temp = (paddr >> log2TileSize);
      temp = (numNodesMask & temp);
      if(temp >= numNodes) {
        temp -= numNodes;
      }
      return NetDevice_t(temp);
    }
  }
};

#endif
