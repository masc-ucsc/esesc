#if 0
/* License & includes */ 
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela

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

#ifndef SP_H 
#define SP_H

#include "GStats.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"
#include "DDR2.h"

#include <queue>
#include <vector>
#include <deque>
#include <stack>
#include <map>
#include "callback.h"
#include "estl.h"
#include "CacheCore.h"


class BState : public StateGeneric<AddrType> {
  public:
    BState(int32_t lineSize) {
    }
};

class PfState : public StateGeneric <AddrType> {
  public:
    PfState(int32_t lineSize) {
    }

    uint32_t stride;
    bool goingUp;

    AddrType nextAddr(CacheGeneric<PfState,AddrType> *c) {
      AddrType preaddr = c->calcAddr4Tag(getTag());
      return (goingUp ? (preaddr + stride) : (preaddr - stride));
    }
};


class StridePrefetcher: public MemObj {

private:
  typedef CacheGeneric<BState,AddrType> BuffType;
  typedef CacheGeneric<BState,AddrType>::CacheLine bLine;

  typedef CacheGeneric<PfState,AddrType> PfTable;
  typedef CacheGeneric<PfState,AddrType>::CacheLine pEntry;


  struct hash_long_long {
    size_t operator()(const AddrType in)  const {
      //uint32_t ret = (in >> 32L) ^ (in & 0xFFFFFFFF);
      //return (uint32_t) ret;
      return (uint32_t) in;
    }
  };

  class AddrTypeHashFunc {
    public:
      size_t operator()(const AddrType p) const {
        return((int) p);
      }
  };

  class AddrTypeEqual {
    public:
      bool operator()(const AddrType &x, const AddrType &y) const{
        return (memcmp((const void*)x, (const void*)y, sizeof(AddrType)) == 0);
      }
  };
  
  typedef HASH_MAP<AddrType, std::queue<MemRequest *>, AddrTypeHashFunc> penReqMapper;
  typedef HASH_SET<AddrType, AddrTypeHashFunc> penFetchSet;

  penReqMapper pendingRequests;
  penFetchSet pendingFetches;

  BuffType *buff;
  PfTable  *table;

  std::deque<AddrType> lastMissesQ;

  PortGeneric *buffPort;
  PortGeneric *tablePort;

  int32_t numStreams;
  int32_t streamAssoc;
  int32_t depth;
  int32_t numBuffPorts;
  int32_t buffPortOccp;
  int32_t numTablePorts;
  int32_t tablePortOccp;
  int32_t hitDelay;
  int32_t missDelay;
  int32_t learnHitDelay;
  int32_t learnMissDelay;
  uint32_t missWindow;
  uint32_t maxStride;
  uint32_t MaxPendingRequests;
  static const int32_t pEntrySize = 8; // size of an entry in the prefetching table
  
  //int32_t defaultMask;
  AddrType defaultMask;

  GStatsCntr halfMiss;
  GStatsCntr miss;
  GStatsCntr hit;
  GStatsCntr predictions;
  GStatsCntr accesses;
  GStatsCntr unitStrideStreams;
  GStatsCntr nonUnitStrideStreams;
  GStatsCntr ignoredStreams;

protected:
  TimeDelta_t delay;
//  PortGeneric *dataPort;
//  PortGeneric *cmdPort;

  bool isMemoryBus;
//  DDR2 **DRAM;
//  int numChannels;

public:
  StridePrefetcher(MemorySystem* current, const char *device_descr_section, const char *device_name = NULL);
  ~StridePrefetcher() {}

  Time_t nextReadSlot(       const MemRequest *mreq);
  Time_t nextWriteSlot(      const MemRequest *mreq);
  Time_t nextBusReadSlot(    const MemRequest *mreq);
  Time_t nextPushDownSlot(   const MemRequest *mreq);
  Time_t nextPushUpSlot(     const MemRequest *mreq);
  Time_t nextInvalidateSlot( const MemRequest *mreq);

  // processor direct requests
  void read(MemRequest  *req);
  void write(MemRequest *req);
  void writeAddress(MemRequest *req);

  // DOWN
  void busRead(MemRequest *req);
  void pushDown(MemRequest *req);
  
  // UP
  void pushUp(MemRequest *req);
  void invalidate(MemRequest *req);

  // Status/state
  uint16_t getLineSize() const;

  bool canAcceptRead(DInst *dinst) const;
  bool canAcceptWrite(DInst *dinst) const;

  TimeDelta_t ffread(AddrType addr, DataType data);
  TimeDelta_t ffwrite(AddrType addr, DataType data);
  void        ffinvalidate(AddrType addr, int32_t lineSize);

  //Added from the old prefetcher
  void learnHit(AddrType addr,MemRequest* orig_mreq);
  void learnMiss(AddrType addr, MemRequest* orig_mreq);
  void prefetch(pEntry *pe, Time_t lat, MemRequest* orig_mreq);
  void processAck(AddrType addr);

  typedef CallbackMember1<StridePrefetcher, AddrType, &StridePrefetcher::processAck> processAckCB;

  Time_t nextBuffSlot() {
    return buffPort->nextSlot();
  }

  Time_t nextTableSlot() {
    return tablePort->nextSlot();
  }

};

#endif

#endif
