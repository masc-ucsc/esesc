// Contributed by Jose Renau
//                Milos Prvulovic
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

#include "Cluster.h"

#include "GMemorySystem.h"
#include "GProcessor.h"
#include "Port.h"
#include "Resource.h"
#include "SescConf.h"
#include "TaskHandler.h"

#include "estl.h"

// Begin: Fields used during constructions

struct UnitEntry {
  PortGeneric *gen;
  int32_t      num;
  int32_t      occ;
};

// Class for comparison to be used in hashes of char * where the content is to be compared
class eqstr {
public:
  inline bool operator()(const char *s1, const char *s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};

// typedef HASH_MAP<const char *,UnitEntry , HASH<const char*>, eqstr>     UnitMapType;
// typedef HASH_MAP<const char *,Resource *, HASH<const char*>, eqstr> ResourceMapType;
// typedef HASH_MAP<const char *,Cluster  *, HASH<const char*>, eqstr>  ClusterMapType;
typedef std::map<string, UnitEntry>  UnitMapType;
typedef std::map<string, Resource *> ResourceMapType;
typedef std::map<string, Cluster *>  ClusterMapType;

static UnitMapType     unitMap;
static ResourceMapType resourceMap;
static ClusterMapType  clusterMap;

Cluster::~Cluster() {
  // Nothing to do
}

Cluster::Cluster(const char *clusterName, uint32_t pos, uint32_t _cpuid)
    : window(_cpuid, this, clusterName, pos)
    , MaxWinSize(SescConf->getInt(clusterName, "winSize"))
    , windowSize(SescConf->getInt(clusterName, "winSize"))
    , winNotUsed("P(%d)_%s%d_winNotUsed", _cpuid, clusterName, pos)
    , rdRegPool("P(%d)_%s%d_rdRegPool", _cpuid, clusterName, pos)
    , wrRegPool("P(%d)_%s%d_wrRegPool", _cpuid, clusterName, pos)
    , cpuid(_cpuid) {
  name = (char *)malloc(strlen(clusterName) + 32);
  sprintf(name, "%s%d", clusterName, pos);

  bzero(res, sizeof(Resource *) * iMAX);

  nready = 0;
}

void Cluster::buildUnit(const char *clusterName, uint32_t pos, GMemorySystem *ms, Cluster *cluster, uint32_t cpuid, InstOpcode type,
                        GProcessor *gproc) {
  const char *unitType = Instruction::opcode2Name(type);

  char utUnit[1024];
  char utLat[1024];
  sprintf(utUnit, "%sUnit", unitType);
  sprintf(utLat, "%sLat", unitType);

  if(!SescConf->checkCharPtr(clusterName, utUnit))
    return;

  int smt = 1;
  int id  = cpuid;
  if(SescConf->checkInt("cpusimu", "smt", id))
    smt = SescConf->getInt("cpusimu", "smt", id);
  int smt_ctx = id - (id % smt);

  const char *sUnitName = SescConf->getCharPtr(clusterName, utUnit);
  char        unitName[1024];
  sprintf(unitName, "P(%d)_%s%d_%s", smt_ctx, clusterName, pos, sUnitName);

  TimeDelta_t  lat = SescConf->getInt(clusterName, utLat);
  PortGeneric *gen;

  SescConf->isBetween(clusterName, utLat, 0, 1024);
  UnitMapType::const_iterator it = unitMap.find(unitName);

  if(it != unitMap.end()) {
    gen = it->second.gen;
  } else {
    UnitEntry e;
    e.num = SescConf->getInt(sUnitName, "Num");
    SescConf->isLT(sUnitName, "Num", 1024);

    e.occ = SescConf->getInt(sUnitName, "occ");
    SescConf->isBetween(sUnitName, "occ", 0, 1024);

    e.gen = PortGeneric::create(unitName, e.num, e.occ);

    unitMap[unitName] = e;
    gen               = e.gen;
  }
  char resourceName[1024];
  int  unitID = 0;
  switch(type) {
  case iBALU_LBRANCH:
  case iBALU_LJUMP:
  case iBALU_LCALL:
  case iBALU_RBRANCH:
  case iBALU_RJUMP:
  case iBALU_RCALL:
  case iBALU_RET:
    unitID = 1;
    break;
  case iLALU_LD:
    unitID = 2;
    break;
  case iSALU_LL:
  case iSALU_SC:
  case iSALU_ST:
  case iSALU_ADDR:
    unitID = 3;
    break;
  default:
    unitID = 0;
    break;
  }

  sprintf(resourceName, "P(%d)_%s%d_%d_%s", smt_ctx, clusterName, pos, unitID, sUnitName);
  ResourceMapType::const_iterator it2 = resourceMap.find(resourceName);

  Resource *r = 0;
  if(it2 != resourceMap.end()) {
    r = it2->second;
  } else {
    switch(type) {
    case iOpInvalid:
    case iRALU:
      r = new FURALU(type, cluster, gen, lat, cpuid);
      break;
    case iAALU:
    case iCALU_FPMULT:
    case iCALU_FPDIV:
    case iCALU_FPALU:
    case iCALU_MULT:
    case iCALU_DIV:
      r = new FUGeneric(type, cluster, gen, lat, cpuid);
      break;
    case iBALU_LBRANCH:
    case iBALU_LJUMP:
    case iBALU_LCALL:
    case iBALU_RBRANCH:
    case iBALU_RJUMP:
    case iBALU_RCALL:
    case iBALU_RET: {
      int32_t MaxBranches = SescConf->getInt("cpusimu", "maxBranches", cpuid);
      if(MaxBranches == 0)
        MaxBranches = INT_MAX;
      bool drainOnMiss = SescConf->getBool("cpusimu", "drainOnMiss", cpuid);

      r = new FUBranch(type, cluster, gen, lat, cpuid, MaxBranches, drainOnMiss);
    } break;
    case iLALU_LD: {
      TimeDelta_t ldstdelay = SescConf->getInt("cpusimu", "stForwardDelay", cpuid);
      SescConf->isInt("cpusimu", "maxLoads", cpuid);
      SescConf->isBetween("cpusimu", "maxLoads", 0, 256 * 1024, cpuid);
      int32_t maxLoads = SescConf->getInt("cpusimu", "maxLoads", cpuid);
      if(maxLoads == 0)
        maxLoads = 256 * 1024;

      r = new FULoad(type, cluster, gen, gproc->getLSQ(), gproc->getSS(), gproc->getPrefetcher(), ldstdelay, lat, ms, maxLoads,
                     cpuid, "specld");
    } break;
    case iSALU_LL:
    case iSALU_SC:
    case iSALU_ST:
    case iSALU_ADDR: {
      SescConf->isInt("cpusimu", "maxStores", cpuid);
      SescConf->isBetween("cpusimu", "maxStores", 0, 256 * 1024, cpuid);
      int32_t maxStores = SescConf->getInt("cpusimu", "maxStores", cpuid);
      if(maxStores == 0)
        maxStores = 256 * 1024;

      r = new FUStore(type, cluster, gen, gproc->getLSQ(), gproc->getSS(), gproc->getPrefetcher(), lat, ms, maxStores, cpuid,
                      Instruction::opcode2Name(type));
    } break;
    default:
      I(0);
      MSG("Unknown unit type [%d] [%s]", type, Instruction::opcode2Name(type));
    }
    I(r);
    resourceMap[resourceName] = r;
  }

  I(res[type] == 0);
  res[type] = r;
}

Cluster *Cluster::create(const char *clusterName, uint32_t pos, GMemorySystem *ms, uint32_t cpuid, GProcessor *gproc) {
  // Constraints
  SescConf->isCharPtr(clusterName, "recycleAt");
  SescConf->isInList(clusterName, "recycleAt", "Executing", "Execute", "Retire");
  const char *recycleAt = SescConf->getCharPtr(clusterName, "recycleAt");

  SescConf->isInt(clusterName, "winSize");
  SescConf->isBetween(clusterName, "winSize", 1, 262144);

  int smt = 1;
  int id  = cpuid;
  if(SescConf->checkInt("cpusimu", "smt", id))
    smt = SescConf->getInt("cpusimu", "smt", id);
  int smt_ctx = id - (id % smt);

  char cName[1024];
  sprintf(cName, "cluster(%d)_%s%d", smt_ctx, clusterName, pos);
  ClusterMapType::const_iterator it = clusterMap.find(cName);

  Cluster *cluster = 0;

  if(it != clusterMap.end()) {
    cluster = it->second;
  } else {

    if(strcasecmp(recycleAt, "retire") == 0) {
      cluster = new RetiredCluster(clusterName, pos, cpuid);
    } else if(strcasecmp(recycleAt, "executing") == 0) {
      cluster = new ExecutingCluster(clusterName, pos, cpuid);
    } else {
      I(strcasecmp(recycleAt, "execute") == 0);
      cluster = new ExecutedCluster(clusterName, pos, cpuid);
    }

    cluster->nRegs   = SescConf->getInt(clusterName, "nRegs");
    cluster->regPool = cluster->nRegs;
    if(SescConf->checkBool(clusterName, "lateAlloc"))
      cluster->lateAlloc = SescConf->getBool(clusterName, "lateAlloc");
    else
      cluster->lateAlloc = false;

    SescConf->isInt(clusterName, "nRegs");
    SescConf->isBetween(clusterName, "nRegs", 2, 262144);

    for(int32_t t = 0; t < iMAX; t++) {
      cluster->buildUnit(clusterName, pos, ms, cluster, cpuid, static_cast<InstOpcode>(t), gproc);
    }

    clusterMap[cName] = cluster;
  }

  return cluster;
}

void Cluster::select(DInst *dinst) {
  I(nready >= 0);
  nready++;
  window.select(dinst);
}

StallCause Cluster::canIssue(DInst *dinst) const {
  if(regPool <= 0)
    return SmallREGStall;

  if(windowSize <= 0)
    return SmallWinStall;

  StallCause sc = window.canIssue(dinst);
  if(sc != NoStall)
    return sc;

  return dinst->getClusterResource()->canIssue(dinst);
}

void Cluster::addInst(DInst *dinst) {

  rdRegPool.add(2, dinst->getStatsFlag()); // 2 reads

  if(!lateAlloc && dinst->getInst()->hasDstRegister()) {
    wrRegPool.inc(dinst->getStatsFlag());
    I(regPool > 0);
    regPool--;
  }
  // dinst->dump("add");

  newEntry();

  window.addInst(dinst);
}

//************ Executing Cluster

void ExecutingCluster::executing(DInst *dinst) {
  nready--;

  if(lateAlloc && dinst->getInst()->hasDstRegister()) {
    wrRegPool.inc(dinst->getStatsFlag());
    I(regPool > 0);
    regPool--;
  }
  dinst->getGProc()->executing(dinst);

  delEntry();
}

void ExecutingCluster::executed(DInst *dinst) {

  window.executed(dinst);
  dinst->getGProc()->executed(dinst);
}

bool ExecutingCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);

  if(!done)
    return false;

  bool hasDest = (dinst->getInst()->hasDstRegister());

  if(hasDest) {
    regPool++;
    I(regPool <= nRegs);
  }

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ Executed Cluster

void ExecutedCluster::executing(DInst *dinst) {
  nready--;

  if(lateAlloc && dinst->getInst()->hasDstRegister()) {
    wrRegPool.inc(dinst->getStatsFlag());
    I(regPool > 0);
    regPool--;
  }
  dinst->getGProc()->executing(dinst);
}

void ExecutedCluster::executed(DInst *dinst) {

  window.executed(dinst);
  dinst->getGProc()->executed(dinst);
  I(!dinst->hasPending());

  delEntry();
}

bool ExecutedCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);
  if(!done)
    return false;

  bool hasDest = (dinst->getInst()->hasDstRegister());
  if(hasDest) {
    regPool++;
    I(regPool <= nRegs);
  }
  // dinst->dump("ret");

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ RetiredCluster

void RetiredCluster::executing(DInst *dinst) {
  nready--;

  if(lateAlloc && dinst->getInst()->hasDstRegister()) {
    wrRegPool.inc(dinst->getStatsFlag());
    regPool--;
  }
  dinst->getGProc()->executing(dinst);
}

void RetiredCluster::executed(DInst *dinst) {

  window.executed(dinst);
  dinst->getGProc()->executed(dinst);
}

bool RetiredCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);
  if(!done)
    return false;

  I(dinst->getGProc()->getID() == dinst->getFlowId());

  bool hasDest = (dinst->getInst()->hasDstRegister());

  if(hasDest) {
    regPool++;
    I(regPool <= nRegs);
  }

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  delEntry();

  return true;
}
