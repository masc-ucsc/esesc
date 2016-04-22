// Contributed by Jose Renau
//                Luis Ceze
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

#ifndef DINST_H
#define DINST_H

#include "Instruction.h"
#include "pool.h"
#include "nanassert.h"

#include "RAWDInst.h"
#include "callback.h"
#include "Snippets.h"

typedef int32_t SSID_t;

class DInst;
class FetchEngine;
class BPredictor;
class Cluster;
class Resource;
class EmulInterface;

//#define ESESC_TRACE 1

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
  Time_t fetched;
  Time_t renamed;
  Time_t issued;
  Time_t executed;

  bool  retired;

  bool loadForwarded;
  bool delayedDispatch;

  bool replay;

  bool performed;

  bool interCluster;
  bool keepStats;
  bool biasBranch;

  // END Boolean flags

  SSID_t       SSID;
  AddrType     conflictStorePC;
  Instruction  inst;
  AddrType     pc;    // PC for the dinst
  AddrType     addr;  // Either load/store address or jump/branch address
  Cluster    *cluster;
  Resource   *resource;
  DInst      **RAT1Entry;
  DInst      **RAT2Entry;
  DInst      **serializeEntry;
  FetchEngine *fetch;

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
    first         = 0;

    cluster         = 0;
    resource        = 0;
    RAT1Entry       = 0;
    RAT2Entry       = 0;
    serializeEntry  = 0;
    fetch           = 0;
    SSID            = -1;
    conflictStorePC = 0;

    fetched       = 0;
    renamed       = 0;
    issued        = 0;
    executed      = 0;
    retired       = false;

    delayedDispatch = false;
    loadForwarded   = false;

    replay        = false;
    performed     = false;
    interCluster  = false;

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

  static DInst *create(const Instruction *inst, AddrType pc, AddrType address, FlowID fid, bool keepStats) {
    DInst *i = dInstPool.out(); 

    I(inst);

    i->fid       = fid;
    i->inst      = *inst;
    i->pc        = pc;
    i->addr      = address;
    i->fetched   = 0;
    i->keepStats = keepStats;

    i->setup();
    I(i->getInst()->getOpcode());

    return i;
  }

  void scrap(EmulInterface *eint); // Destroys the instruction without any other effects
  void destroy(EmulInterface *eint);
  void recycle();

  void clearCluster() {
    cluster  = 0;
    resource = 0;
  }

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

  void lockFetch(FetchEngine *fe) {
    I(fetch==0);
    fetch     = fe;
    fetched = globalClock;
  }

  void setFetchTime() {
    I(fetch==0);
    fetched = globalClock;
  }

  FetchEngine *getFetch() const {
    return fetch;
  }
  Time_t getFetchTime() const {
    return fetched;
  }

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

    I(executed==0);
    I(d->executed==0);
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
    I(executed==0);
    I(d->executed==0);

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
    I(executed==0);
    I(d->executed==0);

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
  bool isDelayedDispatch() const { return delayedDispatch; }
  void clearDelayedDispatch() {
    I(delayedDispatch);
    delayedDispatch=false;
  }
  void setDelayedDispatch() {
    I(!delayedDispatch);
    delayedDispatch=true;
  }
  bool isLoadForwarded() const { return loadForwarded; }
  void setLoadForwarded() {
    I(!loadForwarded);
    loadForwarded=true;
  }

  bool hasInterCluster() const { return interCluster; }
  void markInterCluster() {
    interCluster = true;
  }

  bool isIssued() const { return issued; }

  void markRenamed() {
    I(renamed==0);
    renamed = globalClock;
  }
  bool isRenamed() const { return renamed!=0; }

  void markIssued() {
    I(issued==0);
    I(executed==0);
    issued = globalClock;
  }

  bool isExecuted() const { return executed; }
  void markExecuted() {
    I(issued!=0);
    I(executed==0);
    executed = globalClock;
  }

  bool isReplay() const { return replay; }
  void markReplay() {
    replay= true;
  }

  void setBiasBranch(bool b) { biasBranch = b; }
  bool isBiasBranch() const { return biasBranch; }

  bool isTaken()    const {
    I(getInst()->isControl());
    return addr!=0;
  }

  bool isPerformed() const { return performed; }
  void markPerformed() {
    // Loads get performed first, and then executed
    GI(!inst.isLoad(),executed!=0);
    performed = true;
  }

  bool isRetired() const { return retired; }
  void markRetired() {
    I(inst.isStore());
    retired = true;
  }

  Time_t getFetchedTime() const { return fetched; }
  Time_t getRenamedTime() const { return renamed; }
  Time_t getIssuedTime() const { return issued; }
  Time_t getExecutedTime() const { return executed; }

  Time_t getID() const { return ID; }

#ifdef DEBUG
  uint64_t getmreq_id() { return mreq_id; }
  void setmreq_id(uint64_t _mreq_id) { mreq_id = _mreq_id; }
#endif

};

class Hash4DInst {
 public:
  size_t operator()(const DInst *dinst) const {
    return (size_t)(dinst);
  }
};


#endif   // DINST_H
