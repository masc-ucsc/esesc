#if 1
// Contributed by Jose Renau
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

#ifndef MARKOVPREFETCHER_H
#define MARKOVPREFETCHER_H

#include <queue>

#include "Port.h"
#include "MemRequest.h"
#include "CacheCore.h"
#include "MemObj.h"
/* }}} */


class MarkovPfState : public StateGeneric<AddrType> {
 public:
  AddrType predAddr1;
  AddrType predAddr2;
  AddrType predAddr3;
  AddrType predAddr4;
  MarkovPfState(int32_t linesize){
  }
};


class MarkovTState : public StateGeneric<AddrType> {
 public:
  AddrType missAddr;
  AddrType predAddr1;
  AddrType predAddr2;
  AddrType predAddr3;
  int32_t tag;
  MarkovTState(int32_t linesize){
  }
  };




class MarkovPrefetcher : public MemObj {

private:
  typedef CacheGeneric<MarkovPfState,AddrType> MarkovTable;
  MarkovTable::CacheLine *tEntry;
  AddrType lastAddr;

  typedef CacheGeneric<MarkovTState,AddrType> BuffType;
  typedef CacheGeneric<MarkovTState,AddrType>::CacheLine bLine;

  typedef HASH_MAP<AddrType, std::queue<MemRequest *> *> penReqMapper;
  typedef HASH_SET<AddrType> penFetchSet;

#if 0
public:
/*
  TimeDelta_t delay;
  GMemorySystem *gms;
  PortGeneric *cachePort;
  PortGeneric *dataPort;
  PortGeneric *cmdPort;
*/

  uint32_t pendingRequests;
  uint32_t pendingFetches;

  //void read(MemRequest *mreq);

  int32_t defaultMask;
  
  int32_t lineSize;



  PortGeneric *buffPort;
  PortGeneric *tablePort;

  int32_t numBuffPorts;
  int32_t numTablePorts;
  int32_t buffPortOccp;
  int32_t tablePortOccp;
  int32_t hitDelay;
  int32_t missDelay;
  int32_t depth;
  
  int32_t age;


  static const int32_t tEntrySize = 8; // size of an entry in the prefetching table

#endif

penReqMapper pendingRequests;
penFetchSet pendingFetches;


  BuffType *buff;
  MarkovTable *table;

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
int32_t lineSize;
int32_t width;
int32_t ptr;
static const int32_t tEntrySize = 8; // size of an entry in the prefetching table

AddrType defaultMask;

  GStatsCntr halfMiss;
  GStatsCntr miss;
  GStatsCntr hit;
  GStatsCntr predictions;
  GStatsCntr accesses;

protected:
TimeDelta_t delay;
 PortGeneric *dataPort;
 PortGeneric *cmdPort;

bool isMemoryBus;


public:
  MarkovPrefetcher(MemorySystem* current, const char *device_descr_section, const char *device_name = NULL);
  ~MarkovPrefetcher() {}
  void doReq(MemRequest *mreq);
  void doDisp(MemRequest *mreq);
  void doReqAck(MemRequest *mreq);

  void doSetState(MemRequest *mreq);
  void doSetStateAck(MemRequest *mreq);
  bool isBusy(AddrType addr) const;
  TimeDelta_t ffread(AddrType addr);
  TimeDelta_t ffwrite(AddrType addr);
  void prefetch(AddrType prefAddr, Time_t lat);
  void insertTable(AddrType addr);
  void TESTinsertTable(AddrType addr);


  Time_t nextBuffSlot() {
    return buffPort->nextSlot(true);
  }

  Time_t nextTableSlot() {
    return tablePort->nextSlot(true);
  }

  typedef CallbackMember1<MarkovPrefetcher, MemRequest*, &MarkovPrefetcher::doSetState> processAckCB;
};

#endif // MARKOVPREFETCHER_H
#endif
