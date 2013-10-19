/* License & includes {{{1 */
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
