#if 0
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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

#ifndef MARKOVPREFETCHER_H
#define MARKOVPREFETCHER_H

#include <queue>

#include "Port.h"
#include "MemRequest.h"
#include "CacheCore.h"
#include "MemObj.h"

class MarkovPfState : public StateGeneric<AddrType> {
 public:
  AddrType predAddr1;
  AddrType predAddr2;
  AddrType predAddr3;
  AddrType predAddr4;
};

class MarkovQState : public StateGeneric<AddrType> {
};

/*
class MarkovTState : public StateGeneric<AddrType> {
 public:
  AddrType missAddr;
  AddrType predAddr1;
  AddrType predAddr2;
  AddrType predAddr3;
  int32_t tag;
  };*/

class MarkovPrefetcher : public MemObj {
protected:
  GMemorySystem *gms;
  PortGeneric *cachePort;

  typedef CacheGeneric<MarkovPfState,AddrType> MarkovTable;
  MarkovTable::CacheLine *tEntry;
  AddrType lastAddr;

  typedef CacheGeneric<MarkovQState,AddrType> BuffType;
  typedef CacheGeneric<MarkovQState,AddrType>::CacheLine bLine;

  typedef HASH_MAP<AddrType, std::queue<MemRequest *> *> penReqMapper;
  typedef HASH_SET<AddrType> penFetchSet;

  penReqMapper pendingRequests;

  penFetchSet pendingFetches;

  void read(MemRequest *mreq);

  int32_t defaultMask;
  
  int32_t lineSize;

  BuffType *buff;
  MarkovTable *table;

  PortGeneric *buffPort;
  PortGeneric *tablePort;

  int32_t numBuffPorts;
  int32_t numTablePorts;
  int32_t buffPortOccp;
  int32_t tablePortOccp;
  int32_t hitDelay;
  int32_t missDelay;
  int32_t depth;
  int32_t width;
  int32_t ptr;
  int32_t age;

  GStatsCntr halfMiss;
  GStatsCntr miss;
  GStatsCntr hit;
  GStatsCntr predictions;
  GStatsCntr accesses;

  static const int32_t tEntrySize = 8; // size of an entry in the prefetching table

public:
  MarkovPrefetcher(MemorySystem* current, const char *device_descr_section,
      const char *device_name = NULL);
  ~MarkovPrefetcher() {}
  void access(MemRequest *mreq);
  void returnAccess(MemRequest *mreq);
  bool canAcceptStore(AddrType addr);
  virtual void invalidate(AddrType addr,uint16_t size,MemObj *oc);
  Time_t getNextFreeCycle() const;
  void prefetch(AddrType addr, Time_t lat);
  void insertTable(AddrType paddr);
  void TESTinsertTable(AddrType paddr);
  void processAck(AddrType paddr);
  typedef CallbackMember1<MarkovPrefetcher, AddrType, &MarkovPrefetcher::processAck> processAckCB;

  Time_t nextBuffSlot() {
    return buffPort->nextSlot();
  }

  Time_t nextTableSlot() {
    return tablePort->nextSlot();
  }

};

#endif // MARKOVPREFETCHER_H
#endif
