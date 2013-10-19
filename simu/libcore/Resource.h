/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Smruti Sarangi
                  Karin Strauss

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

#ifndef RESOURCE_H
#define RESOURCE_H

#include "GStats.h"

#include "callback.h"
#include "BloomFilter.h"
#include "FastQueue.h"

#include "nanassert.h"
#include "StoreSet.h"


class PortGeneric;
class DInst;
class MemObj;
class Cluster;

enum StallCause {
  NoStall=0,
  SmallWinStall,
  SmallROBStall,
  SmallREGStall,
  OutsLoadsStall,
  OutsStoresStall,
  OutsBranchesStall,
  ReplaysStall,
  SyscallStall,
  MaxStall,
  Suspend 
};

class GProcessor;
class LSQ;

class Resource {
protected:
  Cluster     *const cluster;
  PortGeneric *const gen;
  GProcessor  *const gproc;

  const TimeDelta_t lat;

  Time_t       usedTime;

  Resource(Cluster *cls, PortGeneric *gen, TimeDelta_t l);


public:
  virtual ~Resource();

  const Cluster *getCluster() const { return cluster; }
  Cluster *getCluster() { return cluster; }

  // Sequence:
  //
  // 1st) A canIssue check is done with "canIssue". This checks that the cluster
  // can accept another request (cluster window size), and that additional
  // structures (like the LD/ST queue entry) also have enough resources.
  //
  // 2nd) The timing to calculate when the inputs are ready is done at
  // executing.
  //
  // 3rd) executed is called the instructions has been executed. It may be
  // called through DInst::doAtExecuted
  //
  // 4th) When the instruction is retired from the ROB retire is called

  virtual StallCause canIssue(DInst  *dinst) =    0;
  virtual void       executing(DInst *dinst) =    0;
  virtual void       executed(DInst  *dinst) =    0;
  virtual bool       preretire(DInst  *dinst,bool flushing) = 0;
  virtual bool       retire(DInst    *dinst, bool flushing) = 0;
  virtual void       performed(DInst *dinst) =    0;

  void select(DInst *dinst);

  typedef CallbackMember1<Resource, DInst *, &Resource::select>    selectCB;
  typedef CallbackMember1<Resource, DInst *, &Resource::executing> executingCB;
  typedef CallbackMember1<Resource, DInst *, &Resource::executed>  executedCB;
  typedef CallbackMember1<Resource, DInst *, &Resource::performed> performedCB;

  Time_t getUsedTime() const { return usedTime; }
  void setUsedTime() { usedTime = globalClock;  }
};

class GMemorySystem;

class MemReplay : public Resource {
protected:
  const uint32_t lfSize;

  StoreSet      *storeset;
  void replayManage(DInst *dinst);
  struct FailType {
    SSID_t ssid;
    Time_t id;
    AddrType pc;
    AddrType addr;
    AddrType data;
    InstOpcode op;
  };
  FailType *lf;
public:
  MemReplay(Cluster *cls, PortGeneric *gen, StoreSet *ss, TimeDelta_t l);

};

class MemResource : public MemReplay {
private:
protected:
  MemObj        *DL1;
  GMemorySystem *memorySystem;
  LSQ           *lsq;

  GStatsCntr    ldldViolations;
  GStatsCntr    stldViolations;
  GStatsCntr    ststViolations;


  MemResource(Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, TimeDelta_t l, GMemorySystem *ms, int32_t id, const char *cad);
public:
};

class MemResource_noMemSpec : public Resource {
private:
protected:
  MemObj        *DL1;
  GMemorySystem *memorySystem;
  MemResource_noMemSpec(Cluster *cls, PortGeneric *aGen, TimeDelta_t l, GMemorySystem *ms, int32_t id, const char *cad);
public:
};

class SCOOREMem : public MemReplay {
private:
protected:
  MemObj        *DL1;
  MemObj        *vpc;
  GMemorySystem *memorySystem;
  //GStatsCntr    ScooreUpperHierReplays;
  //std::map<AddrType,DInst> pendLoadBuff;

  const bool enableDcache;

  TimeDelta_t    vpcDelay;
  TimeDelta_t    DL1Delay;
  
  SCOOREMem(Cluster *cls, PortGeneric *aGen, StoreSet *ss, TimeDelta_t l, GMemorySystem *ms, int32_t id, const char *cad);

public:
};

class FUSCOORELoad : public SCOOREMem {
private:
protected:
  void retryvpc(DInst *dinst);
  typedef CallbackMember1<FUSCOORELoad, DInst *, &FUSCOORELoad::retryvpc> retryvpcCB;
public:
  FUSCOORELoad(Cluster *cls, PortGeneric *aGen, StoreSet *ss, TimeDelta_t l, GMemorySystem *ms, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
  int32_t    maxLoads;
  int32_t    free_entries;

};

class FUSCOOREStore : public SCOOREMem {
private:
protected:
  void retryvpc(DInst *dinst);
  typedef CallbackMember1<FUSCOOREStore, DInst *, &FUSCOOREStore::retryvpc> retryvpcCB;

public:
  FUSCOOREStore(Cluster *cls, PortGeneric *aGen, StoreSet *ss, TimeDelta_t l, GMemorySystem *ms, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
  int32_t    maxStores;
  int32_t    free_entries;


};

class FULoad : public MemResource {
private:
  const TimeDelta_t LSDelay;

  int32_t freeEntries;
  bool enableDcache;

protected:
  void cacheDispatched(DInst *dinst);
  typedef CallbackMember1<FULoad, DInst *, &FULoad::cacheDispatched> cacheDispatchedCB;

public:
  FULoad(Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, TimeDelta_t lsdelay, TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FUStore : public MemResource {
private:

  int32_t freeEntries;
  bool    enableDcache;

public:
  FUStore(Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FUGeneric : public Resource {
private:

protected:
public:
  FUGeneric(Cluster *cls, PortGeneric *aGen, TimeDelta_t l);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FUFuze : public Resource {
private:

protected:
public:
  FUFuze(Cluster *cls, PortGeneric *aGen, TimeDelta_t l);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FUBranch : public Resource {
private:
  int32_t freeBranches;

protected:
public:
  FUBranch(Cluster *cls, PortGeneric *aGen, TimeDelta_t l, int32_t mb);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FURALU : public Resource {
private:
  GStatsCntr memoryBarrier;
  Time_t blockUntil;
  bool scooreMemory;

protected:
public:
  FURALU(Cluster *cls, PortGeneric *aGen, TimeDelta_t l, bool scooreMemory, int32_t id);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst    *dinst, bool flushing);
  void       performed(DInst *dinst);
};

class FULoad_noMemSpec : public MemResource_noMemSpec {
private:
  const TimeDelta_t LSDelay;

  int32_t freeEntries;
  bool enableDcache;

protected:
  void cacheDispatched(DInst *dinst);
  typedef CallbackMember1<FULoad_noMemSpec, DInst *, &FULoad_noMemSpec::cacheDispatched> cacheDispatchedCB;

public:
  FULoad_noMemSpec(Cluster *cls, PortGeneric *aGen, TimeDelta_t lsdelay, TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};

class FUStore_noMemSpec : public MemResource_noMemSpec {
private:

  int32_t freeEntries;
  bool    enableDcache;

public:
  FUStore_noMemSpec(Cluster *cls, PortGeneric *aGen, TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst  *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst  *dinst);
  bool       preretire(DInst  *dinst, bool flushing);
  bool       retire(DInst    *dinst,  bool flushing);
  void       performed(DInst *dinst);
};


#endif   // RESOURCE_H


