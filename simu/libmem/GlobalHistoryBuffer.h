#if 0
// Contributed by Jose Renau
//                Basilio Fraguela
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

#ifndef GHB_H
#define GHB_H

#include "MemorySystem.h"
#include "GStats.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"
#include "CacheCore.h"
#include "MSHR.h"
#include "FastQueue.h"
/* }}} */


class BState : public StateGeneric<AddrType> {
  public:
    BState(int32_t lineSize) {
    }
};


class GHB: public MemObj {

  class HistBuffObj {
    AddrType memaddr;
    class HistBuffObj* lastaccess;
  };

  class AddrTypeHashFunc {
    public:
      size_t operator()(const AddrType p) const {
        return((int) p);
      }
  };

protected:
  uint32_t depth;             // depth
  uint32_t maxPendingRequests;// MaxPendingRequests
  uint32_t pendingRequests;   // MaxPendingRequests
  uint32_t missWindow;        // missWindow
  uint32_t maxStride;         // maxStride
  uint32_t hitDelay;          // hitDelay
  uint32_t missDelay;         // missDelay
  uint32_t learnHitDelay;     // learnHitDelay
  uint32_t learnMissDelay;    // learnMissDelay
  
  HASH_MAP<AddrType, HistBuffObj* , AddrTypeHashFunc> indexTable; //IndexTable
  FastQueue<HistBuffObj*> *HistoryBuffer;                          //Global history buffer
  uint32_t GHBentries;          // No of entries in the History Buffer
  uint32_t GHBindexbits;        // Index bits (will affect the size of the index table)
  uint32_t GHBLocalizingMethod; // Localizing method: 0: PC 1: Mem Addr
  uint32_t GHBDetectionMethod;  // Detection mechanism: 0: Stride, 1: Markov, 2: Distance
  uint32_t defaultMask;         // defaultMask


  typedef CacheGeneric<BState,AddrType> BuffType;
  typedef CacheGeneric<BState,AddrType>::CacheLine bLine;

  BuffType *buff;
  uint32_t numBuffPorts;        // number of buffer ports
  uint32_t buffPortOccp;        // bufferPortOccp

  MSHR        *mshr;                                              //To make sure that there are no requests to the same address

  GStatsCntr halfMiss;
  GStatsCntr miss;
  GStatsCntr hit;
  GStatsCntr predictions;
  GStatsCntr accesses;
  GStatsCntr unitStrideStreams;
  GStatsCntr nonUnitStrideStreams;
  GStatsCntr ignoredStreams;

public:
  GHB(MemorySystem* current, 
      const char *section, 
      const char *name = NULL);
  ~GHB() {}

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
  void busRead(MemRequest *mreq);
  void pushDown(MemRequest *mreq);
  
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

  void        busReadAck(MemRequest *mreq);
  typedef CallbackMember1<GHB, MemRequest*, &GHB::busReadAck> busReadAckCB;

};

#endif
#endif
