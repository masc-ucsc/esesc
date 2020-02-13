// Contributed by Jose Renau
//                Basilio Fraguela
//                Smruti Sarangi
//                Karin Strauss
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

#ifndef RESOURCE_H
#define RESOURCE_H

#include "GStats.h"

#include "BloomFilter.h"
#include "FastQueue.h"
#include "callback.h"

#include "Prefetcher.h"
#include "StoreSet.h"
#include "nanassert.h"

class PortGeneric;
class DInst;
class MemObj;
class Cluster;

enum StallCause {
  NoStall = 0,
  SmallWinStall,
  SmallROBStall,
  SmallREGStall,
  DivergeStall,
  OutsLoadsStall,
  OutsStoresStall,
  OutsBranchesStall,
  ReplaysStall,
  SyscallStall,
  MaxStall,
  Suspend
};

class LSQ;

class Resource {
protected:
  Cluster *const     cluster;
  PortGeneric *const gen;

  GStatsAvg avgRenameTime;
  GStatsAvg avgIssueTime;
  GStatsAvg avgExecuteTime;
  GStatsAvg avgRetireTime;

  const TimeDelta_t lat;
  const int32_t     coreid;

  Time_t usedTime;
  bool   inorder;

  Resource(uint8_t type, Cluster *cls, PortGeneric *gen, TimeDelta_t l, uint32_t cpuid);

  void setStats(const DInst *dinst);

public:
  virtual ~Resource();

  const Cluster *getCluster() const {
    return cluster;
  }
  Cluster *getCluster() {
    return cluster;
  }

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

  virtual StallCause canIssue(DInst *dinst)                 = 0;
  virtual void       executing(DInst *dinst)                = 0;
  virtual void       executed(DInst *dinst)                 = 0;
  virtual bool       preretire(DInst *dinst, bool flushing) = 0;
  virtual bool       retire(DInst *dinst, bool flushing)    = 0;
  virtual void       performed(DInst *dinst)                = 0;

  typedef CallbackMember1<Resource, DInst *, &Resource::executing> executingCB;
  typedef CallbackMember1<Resource, DInst *, &Resource::executed>  executedCB;
  typedef CallbackMember1<Resource, DInst *, &Resource::performed> performedCB;

  Time_t getUsedTime() const {
    return usedTime;
  }
  void setUsedTime() {
    usedTime = globalClock;
  }
};

class GMemorySystem;

class MemReplay : public Resource {
protected:
  const uint32_t lfSize;

  StoreSet *storeset;
  void      replayManage(DInst *dinst);
  struct FailType {
    SSID_t     ssid;
    Time_t     id;
    AddrType   pc;
    AddrType   addr;
    AddrType   data;
    InstOpcode op;
  };
  FailType *lf;

public:
  MemReplay(uint8_t type, Cluster *cls, PortGeneric *gen, StoreSet *ss, TimeDelta_t l, uint32_t cpuid);
};

class MemResource : public MemReplay {
private:
protected:
  MemObj *       firstLevelMemObj;
  MemObj *       DL1;
  GMemorySystem *memorySystem;
  LSQ *          lsq;
  Prefetcher *   pref;

  GStatsCntr stldViolations;

  bool LSQlateAlloc;

  MemResource(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, Prefetcher *pref, TimeDelta_t l,
              GMemorySystem *ms, int32_t id, const char *cad);

public:
};

class FULoad : public MemResource {
private:
  const TimeDelta_t LSDelay;

  int32_t freeEntries;
  bool    enableDcache;
#ifdef MEM_TSO2
  GStatsCntr tso2Replay;
#endif

protected:
  void                                                               cacheDispatched(DInst *dinst);
  typedef CallbackMember1<FULoad, DInst *, &FULoad::cacheDispatched> cacheDispatchedCB;

public:
  FULoad(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, Prefetcher *pref, TimeDelta_t lsdelay,
         TimeDelta_t l, GMemorySystem *ms, int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst *dinst, bool flushing);
  void       performed(DInst *dinst);
};

class FUStore : public MemResource {
private:
  int32_t  freeEntries;
  bool     enableDcache;
  int32_t  scbSize;
  int32_t  scbEntries;
  int32_t  scbMerge[1024];
  uint32_t lineSizeBits;

  typedef std::list<DInst *> SCBQueueType;
  SCBQueueType               scbQueue;

public:
  FUStore(uint8_t type, Cluster *cls, PortGeneric *aGen, LSQ *lsq, StoreSet *ss, Prefetcher *pref, TimeDelta_t l, GMemorySystem *ms,
          int32_t size, int32_t id, const char *cad);

  StallCause canIssue(DInst *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst *dinst, bool flushing);
  void       performed(DInst *dinst);
};

class FUGeneric : public Resource {
private:
protected:
public:
  FUGeneric(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, uint32_t cpuid);

  StallCause canIssue(DInst *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst *dinst, bool flushing);
  void       performed(DInst *dinst);
};

class FUBranch : public Resource {
private:
  int32_t freeBranches;
  bool    drainOnMiss;

protected:
public:
  FUBranch(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, uint32_t cpuid, int32_t mb, bool dom);

  StallCause canIssue(DInst *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst *dinst, bool flushing);
  void       performed(DInst *dinst);
};

class FURALU : public Resource {
private:
  GStatsCntr dmemoryBarrier;
  GStatsCntr imemoryBarrier;
  Time_t     blockUntil;

protected:
public:
  FURALU(uint8_t type, Cluster *cls, PortGeneric *aGen, TimeDelta_t l, int32_t id);

  StallCause canIssue(DInst *dinst);
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);
  bool       preretire(DInst *dinst, bool flushing);
  bool       retire(DInst *dinst, bool flushing);
  void       performed(DInst *dinst);
};

#endif // RESOURCE_H
