/* License & includes {{{1 */
/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Luis Ceze
  Karin Strauss
  Jose Renau
  David Munday

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

#ifndef VPC_H
#define VPC_H

#include "estl.h"
#include "CacheCore.h"
#include "GStats.h"
#include "Port.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MSHR.h"
#include "Snippets.h"
#include "WCB.h"
#include "LSQ.h"


#define FORWARD_PROGRESS_THRESHOLD 20
#define WCB_MISPREDICT_CLEAR_FREQ 1000

class MemRequest;
/* }}} */

class VPC: public MemObj {
 protected:
  class CState : public StateGeneric<AddrType> {
  private:
    std::vector<DataType> data;
    std::vector<DataType> storepc;
    std::vector<Time_t>   stid;
    int32_t   wordMask;
    bool     correct_override;
    std::vector<bool>      xCoreDataPresent;
    enum {
      M,
      E,
      S,
      I
    } state;
  public:
    CState(int32_t lineSize) {
      //MSG("line size = %d",lineSize);
      state  = I;
      wordMask = (1<<lineSize)-1;
      stid.resize(lineSize);
      data.resize(lineSize);
      storepc.resize(lineSize);
      correct_override = false;
      xCoreDataPresent.resize(lineSize);
      clearTag();
    }

    uint32_t getWord(AddrType addr) const { return ((uint32_t)addr & wordMask); }
    DataType getData(AddrType addr) const { return data[getWord(addr)]; }
    DataType getStorePC(AddrType addr) const { return storepc[getWord(addr)]; }
    void setData(AddrType addr, DataType write_data) {
      data[getWord(addr)] = write_data;
    }
      
    bool getXCoreStore(AddrType addr){
      return xCoreDataPresent[getWord(addr)];
    }

    void clearXCoreStore(AddrType addr) {
      xCoreDataPresent[getWord(addr)] = false;
    } 

    void setXCoreStore(AddrType addr) {
      xCoreDataPresent[getWord(addr)] = true;
      //data[getWord(addr)] = 0xDEADBEEF; //force a replay
    } 

    void setStorePC(AddrType addr, AddrType pc) {
      storepc[getWord(addr)] = pc;
    }


    Time_t getSTID(AddrType addr) { return stid[getWord(addr)]; }
    void   setSTID(AddrType addr, Time_t new_stid) {
      stid[getWord(addr)] = new_stid;
    } 

    bool getCorrect_Override() { return (correct_override); }
    void setCorrect_Override(bool flag) { 
      correct_override  = flag;
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

  typedef CacheGeneric<CState,AddrType> VPCType;
  typedef CacheGeneric<CState,AddrType>::CacheLine VPCLine;

  WCB wcb;
  AddrType wcb_lineSize;

  static LSQVPC *replayCheckLSQ;

  static std::vector<VPC*> VPCvec;

  const TimeDelta_t hitDelay;
  const TimeDelta_t missDelay;

  MemorySystem *ms;

  // Cache has 4 ports, read, write, bank, and lower level request (bus/Ln+1)  PortGeneric *rdPort;
  PortGeneric *rdPort;
  PortGeneric *wrPort;
  PortGeneric *ctPort;
  PortGeneric *bkPort;
  VPCType     *VPCBank;
  MSHR        *mshr;
  MemObj      *lower_level;

  int32_t     maxPendingWrites;
  int32_t     pendingWrites;
  int32_t     maxPendingReads;
  int32_t     pendingReads;

  int32_t     lineSize;

  // BEGIN Statistics
  GStatsCntr   wcbHit;
  GStatsCntr   wcbMiss;

  GStatsCntr   wcbPass;
  GStatsCntr   wcbFail;

  GStatsCntr   vpcPass;
  GStatsCntr   vpcFail;
  GStatsCntr   vpcOverride;

  GStatsCntr  readHit;
  GStatsCntr  readMiss;
  GStatsCntr  readHalfMiss;

  GStatsCntr  writeHit;
  GStatsCntr  writeMiss;
  GStatsCntr  writeHalfMiss;

  GStatsCntr  writeBack;
  GStatsCntr  writeExclusive;

  GStatsCntr  lineFill;
  GStatsAvg   avgMissLat;

  GStatsAvg   avgMemLat;

  GStatsCntr  invalidateHit;
  GStatsCntr  invalidateMiss;

  GStatsCntr  replayInflightStore;
  GStatsCntr  VPCreplayXcoreStore;
  
 
  uint64_t  mispredictions;
  uint64_t  committed_at_last_replay;
  uint64_t  skips;
  uint64_t  wcb_replays;
  uint64_t  vpc_replays;


  // END Statistics

  Time_t max(Time_t a, Time_t b) const { return a<b? b : a; }
  int32_t getLineSize() const          { return lineSize;   }

  Time_t nextBankSlot()   { return bkPort->nextSlot(); }
  Time_t nextCtrlSlot()   { return max(ctPort->nextSlot(),nextBankSlot()); }
  Time_t nextReadSlot()   { return max(rdPort->nextSlot(),nextBankSlot()); }
  Time_t nextWriteSlot()  { return max(wrPort->nextSlot(),nextBankSlot()); }

  VPC::VPCLine* allocateLine(AddrType addr, MemRequest *mreq);


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
  void doBankUpdate(MemRequest *mreq);
  void doCacheWrite(DInst * dinst);

  void writeMissHandler(MemRequest *mreq, bool retrying);
  typedef CallbackMember2<VPC, MemRequest *, bool,     &VPC::writeMissHandler>    writeMissHandlerCB;

  void debug(AddrType addr);
  typedef CallbackMember1<VPC, AddrType,               &VPC::debug>        debugCB;
   
  typedef CallbackMember1<VPC, AddrType,               &VPC::doWriteback>  doWritebackCB;
  typedef CallbackMember2<VPC, MemRequest *, bool,     &VPC::doRead>       doReadCB;
  typedef CallbackMember2<VPC, MemRequest *, bool,     &VPC::doWrite>      doWriteCB;
  typedef CallbackMember2<VPC, MemRequest *, bool,     &VPC::doBusRead>    doBusReadCB;
  typedef CallbackMember1<VPC, MemRequest *,           &VPC::doPushDown>   doPushDownCB;
  typedef CallbackMember1<VPC, MemRequest *,           &VPC::doPushUp>     doPushUpCB;
  typedef CallbackMember1<VPC, MemRequest *,           &VPC::doInvalidate> doInvalidateCB;

 public:
  VPC(MemorySystem *gms, const char *descr_section, const char *name = NULL);
  virtual ~VPC();

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

  void replayflush();
  void replayCheckLSQ_removeStore(DInst *);
  
  bool probe(DInst *dinst);
  void checkSetXCoreStore(AddrType addr);
  void updateXCoreStores(AddrType addr);
  

};

#endif
