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

#ifndef TAGGEDPREFETCHER_H
#define TAGGEDPREFETCHER_H

#include <queue>

#include "Port.h"
#include "MemRequest.h"
#include "CacheCore.h"
#include "MemObj.h"

class TagBuffState : public StateGeneric<AddrType> {
};

class TaggedPrefetcher : public MemObj {
protected:
  //GMemorySystem *gms;
  PortGeneric *cachePort;

  typedef CacheGeneric<TagBuffState,AddrType> BuffType;
  typedef CacheGeneric<TagBuffState,AddrType>::CacheLine bLine;

  typedef HASH_MAP<AddrType, std::queue<MemRequest *> *> penReqMapper;
  typedef HASH_SET<AddrType> penFetchSet;

  penReqMapper pendingRequests;

  penFetchSet pendingFetches;

  void read(MemRequest *mreq);

  int32_t defaultMask;
  
  int32_t lineSize;

  BuffType *buff;
  PortGeneric *buffPort;
  PortGeneric *tablePort;  
  int32_t numBuffPorts;
  int32_t buffPortOccp;

  GStatsCntr halfMiss;
  GStatsCntr miss;
  GStatsCntr hit;
  GStatsCntr accesses;
  GStatsCntr predictions;

public:
  TaggedPrefetcher(MemorySystem* current, const char *device_descr_section,
      const char *device_name = NULL);
  ~TaggedPrefetcher() {}
  void access(MemRequest *mreq);
  void returnAccess(MemRequest *mreq);
  bool canAcceptStore(AddrType addr);
  virtual void invalidate(AddrType addr,uint16_t size,MemObj *oc);
  Time_t getNextFreeCycle() const;

  void prefetch(AddrType addr, Time_t lat);
  void learnHit(AddrType addr);

  void processAck(AddrType paddr);
  typedef CallbackMember1<TaggedPrefetcher, AddrType, &TaggedPrefetcher::processAck> processAckCB;

  Time_t nextBuffSlot() {
    return buffPort->nextSlot();
  }

  Time_t nextTableSlot() {
    return tablePort->nextSlot();
  }
};

#endif // TAGGEDPREFETCHER_H
#endif
