// Contributed by Luis Ceze
//                Karin Strauss
//                Jose Renau
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

#ifndef CACHE_H
#define CACHE_H

#include "estl.h"
#include "CacheCore.h"
#include "GStats.h"
#include "Port.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MSHR.h"
#include "Snippets.h"

class MemRequest;
/* }}} */

class Cache: public MemObj {
protected:
  class CState : public StateGeneric<AddrType> {
  private:
    enum {
      M,
      E,
      S,
      I
    } state;
  public:
    CState(int32_t lineSize) {
      state  = I;
      clearTag();
    }

    bool isModified() const  { return state == M; }
    void setModified() {
      state = M;
    }
    bool isExclusive() const { return state == E; }
    void setExclusive() {
      state = E;
    }
    bool isShared() const    { return state == S; }
    void setShared() {
      state = S;
    }
    bool isValid()   const   { return state != I; }
    bool isInvalid() const   { return state == I; }

    void invalidate() {
      state  = I;
      clearTag();
    }
  };

  typedef CacheGeneric<CState,AddrType> CacheType;
  typedef CacheGeneric<CState,AddrType>::CacheLine Line;

  const TimeDelta_t hitDelay;
  const TimeDelta_t invDelay;
  const TimeDelta_t missDelay;

  MemorySystem *ms;

  // Cache has 4 ports, read, write, bank, and lower level request (bus/Ln+1)
  PortGeneric *rdPort;
  PortGeneric *wrPort;
  PortGeneric *ctPort;
  PortGeneric **bkPort;
  CacheType   *cacheBank;
  MSHR        *mshr;

  int32_t     maxPendingWrites;
  int32_t     pendingWrites;
  int32_t     maxPendingReads;
  int32_t     pendingReads;

  int32_t     lineSize;
  int32_t     log2nBanks;

  bool        inclusive;
  bool        llc;

  // BEGIN Statistics
  GStatsCntr  readHit;
  GStatsCntr  readMiss;
  GStatsCntr  readHalfMiss;

  GStatsCntr  writeHit;
  GStatsCntr  writeMiss;
  GStatsCntr  writeHalfMiss;

  GStatsCntr  displaced;

  GStatsCntr  writeBack;
  GStatsCntr  writeExclusive;

  GStatsCntr  lineFill;

  GStatsAvg   avgMissLat;
  GStatsAvg   avgMemLat;

  GStatsCntr  capInvalidateHit;
  GStatsCntr  capInvalidateMiss;
  GStatsCntr  invalidateHit;
  GStatsCntr  invalidateMiss;
  // END Statistics

  Time_t max(Time_t a, Time_t b) const { return a<b? b : a; }
  int32_t getLineSize() const          { return lineSize;   }

  Time_t nextBankSlot(AddrType addr, bool en)   { 
    if (log2nBanks == 0)
      return bkPort[0]->nextSlot(en); 

    // FIXME: For bank selection use b12..+n ^ b20..+n (n=1,2,3,4 0 means no bank)
    AddrType part1 = (addr >> 12) & ((2<<(log2nBanks-1))-1);
    AddrType part2 = (addr >> 20) & ((2<<(log2nBanks-1))-1);
    int bank       = part1 ^ part2;

    //printf ("CHoosing bank %d\n", bank);
    return bkPort[bank]->nextSlot(en); 
  }

  Time_t nextCtrlSlot(AddrType addr, bool en)   { return max(ctPort->nextSlot(en),nextBankSlot(addr,en)); }
  Time_t nextReadSlot(AddrType addr, bool en)   { return max(rdPort->nextSlot(en),nextBankSlot(addr,en)); }
  Time_t nextWriteSlot(AddrType addr, bool en)  { return max(wrPort->nextSlot(en),nextBankSlot(addr,en)); }

  Line *allocateLine(AddrType addr, MemRequest *mreq);

  TimeDelta_t getHitDelay(MemRequest *mreq);
  TimeDelta_t getMissDelay(MemRequest *mreq);

  /* BEGIN Private Cache customization functions */
  virtual void displaceLine(AddrType addr, MemRequest *mreq, bool modified);
  virtual void doWriteback(AddrType addr);

  virtual void doRead(MemRequest  *req, bool retrying=false);
  virtual void doWrite(MemRequest *req, bool retrying=false);

  virtual void doBusRead(MemRequest *req, bool retrying=false);
  virtual void doPushDown(MemRequest *req);

  virtual void doPushUp(MemRequest *req);
  virtual void doInvalidate(MemRequest *req);
  /* END Private Cache customization functions */

  typedef CallbackMember1<Cache, AddrType,               &Cache::doWriteback>  doWritebackCB;
  typedef CallbackMember2<Cache, MemRequest *, bool,     &Cache::doRead>       doReadCB;
  typedef CallbackMember2<Cache, MemRequest *, bool,     &Cache::doWrite>      doWriteCB;
  typedef CallbackMember2<Cache, MemRequest *, bool,     &Cache::doBusRead>    doBusReadCB;
  typedef CallbackMember1<Cache, MemRequest *,           &Cache::doPushDown>   doPushDownCB;
  typedef CallbackMember1<Cache, MemRequest *,           &Cache::doPushUp>     doPushUpCB;
  typedef CallbackMember1<Cache, MemRequest *,           &Cache::doInvalidate> doInvalidateCB;

public:
  Cache(MemorySystem *gms, const char *descr_section, const char *name = NULL);
  virtual ~Cache();

  Time_t nextReadSlot(const MemRequest *mreq);
  Time_t nextWriteSlot(const MemRequest *mreq);
  Time_t nextBusReadSlot(const MemRequest *mreq);
  Time_t nextPushDownSlot(const MemRequest *mreq);
  Time_t nextPushUpSlot(const MemRequest *mreq);
  Time_t nextInvalidateSlot(const MemRequest *mreq);

  // processor direct requests
  void read(MemRequest  *req);
  void write(MemRequest *req);
  virtual void writeAddress(MemRequest *req);
  
  // DOWN
  void busRead(MemRequest *req);
  void pushDown(MemRequest *req);
  
  // UP
  void pushUp(MemRequest *req);
  void invalidate(MemRequest *req);
  
  virtual bool canAcceptRead(DInst *dinst) const;
  virtual bool canAcceptWrite(DInst *dinst) const;

  virtual void dump() const;

  virtual TimeDelta_t ffread(AddrType addr, DataType data);
  virtual TimeDelta_t ffwrite(AddrType addr, DataType data);
  virtual void        ffinvalidate(AddrType addr, int32_t lineSize);
};

#endif
