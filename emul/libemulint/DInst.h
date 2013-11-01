/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Luis Ceze

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef DINST_H
#define DINST_H

#include "Instruction.h"
#include "pool.h"
#include "nanassert.h"

#include "RAWDInst.h"
#include "callback.h"
#include "Snippets.h"
#include "CUDAInstruction.h"

typedef int32_t SSID_t;

class DInst;
class FetchEngine;
class BPredictor;
class Cluster;
class Resource;
class EmulInterface;

// FIXME: do a nice class. Not so public
class DInstNext {
 private:
  DInst *dinst;
#ifdef DINST_PARENT
  DInst *parentDInst;
#endif
 public:
  DInstNext() {
    dinst = 0;
  }

  DInstNext *nextDep;
  bool       isUsed; // true while non-satisfied RAW dependence
  
  const DInstNext *getNext() const { return nextDep; }
  DInstNext *getNext() { return nextDep; }

  void setNextDep(DInstNext *n) {
    nextDep = n;
  }

  void init(DInst *d) {
    I(dinst==0);
    dinst = d;
  }

  DInst *getDInst() const { return dinst; }

#ifdef DINST_PARENT
  DInst *getParentDInst() const { return parentDInst; }
  void setParentDInst(DInst *d) {
    GI(d,isUsed);
    parentDInst = d;
  }
#else
  void setParentDInst(DInst *d) { }
#endif
};

class DInst {
private:
  // In a typical RISC processor MAX_PENDING_SOURCES should be 2
  static const int32_t MAX_PENDING_SOURCES=3;

  static pool<DInst> dInstPool;

  DInstNext pend[MAX_PENDING_SOURCES];
  DInstNext *last;
  DInstNext *first;

  FlowID fid;

  // BEGIN Boolean flags
  bool loadForwarded;
  bool issued;
  bool executed;
  bool replay;

  bool performed;

  bool keepStats;
  float L1clkRatio;
  float L3clkRatio;

#ifdef ENABLE_CUDA
  CUDAMemType memaccess;
  uint32_t pe_id;
#endif

  // END Boolean flags

  // BEGIN Time counters
  Time_t wakeUpTime;
  // END Time counters

  SSID_t       SSID;
  AddrType     conflictStorePC;
  Instruction  inst;
  AddrType     pc;    // PC for the dinst
  AddrType     addr;  // Either load/store address or jump/branch address
  DataType     data;  // load/store data value
  Cluster    *cluster;
  Resource   *resource;
  DInst      **RAT1Entry;
  DInst      **RAT2Entry;
  DInst      **serializeEntry;
  FetchEngine *fetch;
  Time_t fetchTime;

  char nDeps;              // 0, 1 or 2 for RISC processors

  static Time_t currentID;
  Time_t ID; // static ID, increased every create (currentID). pointer to the
#ifdef DEBUG
   uint64_t mreq_id;
#endif
  void setup() {
    ID            = currentID++;
#ifdef DEBUG
    mreq_id       = 0;
#endif
    wakeUpTime    = 0;
    first         = 0;

    cluster         = 0;
    resource        = 0;
    RAT1Entry       = 0;
    RAT2Entry       = 0;
    serializeEntry  = 0;
    fetch           = 0;
    SSID            = -1;
    conflictStorePC = 0;

    loadForwarded = false;
    issued        = false;
    executed      = false;
    replay        = false;
    performed     = false;
#ifdef ENABLE_CUDA
    memaccess   = GlobalMem;
#endif
    fetchTime = 0;
#ifdef DINST_PARENT
    pend[0].setParentDInst(0);
    pend[1].setParentDInst(0);
    pend[2].setParentDInst(0);
#endif

    pend[0].isUsed = false;
    pend[1].isUsed = false;
    pend[2].isUsed = false;

  }
protected:
public:
  DInst();

  DInst *clone();

  bool getStatsFlag() const { return keepStats; }

  float getL1clkRatio() const { return L1clkRatio; }
  float getL3clkRatio() const { return L3clkRatio; }

  static DInst *create(const Instruction *inst, RAWDInst *rinst, AddrType address, FlowID fid) {
    DInst *i = dInstPool.out(); 

    i->fid           = fid;
    i->inst          = *inst;
    I(inst);
    i->pc            = rinst->getPC();
    i->addr          = address;
    i->data          = rinst->getData();
    i->fetchTime = 0;
#ifdef ENABLE_CUDA
    i->memaccess = rinst->getMemaccesstype();
    i->pe_id =((rinst->getData() >> 32) & 0x0000FFFF);
#endif
    i->L3clkRatio = rinst->getL3clkRatio();
    i->keepStats  = rinst->getStatsFlag();
    i->L1clkRatio = rinst->getL1clkRatio();

    //GI(inst->isMemory(), (i->getAddr() & 0x3) == 0); // Always word aligned access
    //GI(i->getAddr(), (i->getAddr() & 0x3) == 0); // Even branches should be word aligned

    i->setup();
    I(i->getInst()->getOpcode());


    return i;
  }

  void scrap(EmulInterface *eint); // Destroys the instruction without any other effects
  void destroy(EmulInterface *eint);
  void recycle();

  void setCluster(Cluster *cls, Resource *res) {
    cluster  = cls;
    resource = res;
  }
  Cluster  *getCluster()         const { return cluster;  }
  Resource *getClusterResource() const { return resource; }

  void clearRATEntry();
  void setRAT1Entry(DInst **rentry) {
    I(!RAT1Entry);
    RAT1Entry = rentry;
  }
  void setRAT2Entry(DInst **rentry) {
    I(!RAT2Entry);
    RAT2Entry = rentry;
  }
  void setSerializeEntry(DInst **rentry) {
    I(!serializeEntry);
    serializeEntry = rentry;
  }

  void setSSID(SSID_t ssid) {
    SSID = ssid;
  }
  SSID_t getSSID() const { return SSID; }

  void setConflictStorePC(AddrType storepc){
    I(storepc);
    I(this->getInst()->isLoad());
    conflictStorePC = storepc;
  }
  AddrType getConflictStorePC() const { return conflictStorePC; }

#ifdef DINST_PARENT
  DInst *getParentSrc1() const {
    if (pend[0].isUsed)
      return pend[0].getParentDInst();
    return 0;
  }
  DInst *getParentSrc2() const {
    if (pend[1].isUsed)
      return pend[1].getParentDInst();
    return 0;
  }
  DInst *getParentSrc3() const {
    if (pend[2].isUsed)
      return pend[2].getParentDInst();
    return 0;
  }
#endif

#if 0
  void setFetch(FetchEngine *fe) {
    fetch = fe;
  }
#else

  void lockFetch(FetchEngine *fe) {
    fetch     = fe;
    fetchTime = globalClock;
  }

  FetchEngine *getFetch() const {
    return fetch;
  }
  Time_t getFetchTime() const {
    return fetchTime;
  }
#endif

  DInst *getNextPending() {
    I(first);
    DInst *n = first->getDInst();

    I(n);

    I(n->nDeps > 0);
    n->nDeps--;

    first->isUsed = false;
    first->setParentDInst(0);
    first = first->getNext();

    return n;
  }

  void addSrc1(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;

    DInstNext *n = &d->pend[0];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }

  void addSrc2(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;
    DInstNext *n = &d->pend[1];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }

  void addSrc3(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;
    DInstNext *n = &d->pend[2];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }


  void setAddr(AddrType a)     { addr = a;               }
  AddrType getPC()       const { return pc;              }
  AddrType getAddr()     const { return addr;            }
  DataType getData()     const { return data;            }
  void     setData(DataType newData) { data = newData;   }
  FlowID   getFlowId()   const { return fid;             }

  char     getnDeps()    const { return nDeps;           }
  bool     isSrc1Ready() const { return !pend[0].isUsed; }
  bool     isSrc2Ready() const { return !pend[1].isUsed; } 
  bool     isSrc3Ready() const { return !pend[2].isUsed; } 
  bool     hasPending()  const { return first != 0;      }

  bool hasDeps()     const {
    GI(!pend[0].isUsed && !pend[1].isUsed && !pend[2].isUsed, nDeps==0);
    return nDeps!=0;
  }

  const DInst       *getFirstPending() const { return first->getDInst(); }
  const DInstNext   *getFirst()        const { return first;             }

  const Instruction *getInst()         const  { return &inst;             }

  void dump(const char *id);

  // methods required for LDSTBuffer
  bool isLoadForwarded() const { return loadForwarded; }
  void setLoadForwarded() {
    I(!loadForwarded);
    loadForwarded=true;
  }

  bool isIssued() const { return issued; }

  void markIssued() {
    I(!issued);
    I(!executed);
    issued = true;
  }

  bool isExecuted() const { return executed; }
  void markExecuted() {
    I(issued);
    I(!executed);
    executed = true;
  }

  bool isReplay() const { return replay; }
  void markReplay() {
    replay= true;
  }

  bool isTaken()    const {
    I(getInst()->isControl());
    return addr!=0;
  }

  bool isPerformed() const { return performed; }
  void markPerformed() {
    // Loads get performed first, and then executed
    GI(!inst.isLoad(),executed);
    I(inst.isLoad() || inst.isStore());
    performed = true;
  }

  void setWakeUpTime(Time_t t)  {
    //I(wakeUpTime <= t || t == 0);
    wakeUpTime = t;
  }

  Time_t getWakeUpTime() const { return wakeUpTime; }

  Time_t getID() const { return ID; }

#ifdef DEBUG
  uint64_t getmreq_id() { return mreq_id; }
  void setmreq_id(uint64_t _mreq_id) { mreq_id = _mreq_id; }
#endif

#ifdef ENABLE_CUDA
  bool isSharedAddress(){
    // FIXME: This is not a check, it changes the memaccess value. It should not do so (bad coding)
    if ((addr >> 61) == 6){
      memaccess = SharedMem;
      return true;
    } 
    return false;
  }
  bool isGlobalAddress() const {
    return (memaccess==GlobalMem);
  }

  bool isParamAddress() const {
    return (memaccess==ParamMem);
  }

  bool isLocalAddress() const {
    return (memaccess==LocalMem);
  }

  bool isConstantAddress() const {
    return (memaccess==ConstantMem);
  }

  bool isTextureAddress() const {
    return (memaccess==TextureMem);
  }

  bool useSharedAddress(){ // SHould be phased out.. use isSharedAddress instead
    return (memaccess==SharedMem);
  }

  void setPE(DataType data){
    pe_id = ((data >> 32) & 0x0000FFFF);
  }

  void markAddressLocal(FlowID fid){
    memaccess = LocalMem;
  }
#endif
  uint32_t getPE() const {
#ifdef ENABLE_CUDA
    return pe_id;
#else
    return 0;
#endif
  }

};

class Hash4DInst {
 public:
  size_t operator()(const DInst *dinst) const {
    return (size_t)(dinst);
  }
};

#endif   // DINST_H
