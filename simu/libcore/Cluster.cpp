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

#include "estl.h"

// Begin: Fields used during constructions

struct UnitEntry {
  PortGeneric *gen;
  int32_t num;
  int32_t occ;
};

//Class for comparison to be used in hashes of char * where the content is to be compared
class eqstr {
public:
  inline bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) == 0;
  }
};

typedef HASH_MAP<const char *,UnitEntry, HASH<const char*>, eqstr> UnitMapType;

static UnitMapType unitMap;

Cluster::~Cluster() {
    // Nothing to do
}

Cluster::Cluster(const char *clusterName, GProcessor *gp)
  : window(gp,this,clusterName)
  ,MaxWinSize(SescConf->getInt(clusterName,"winSize"))
  ,windowSize(SescConf->getInt(clusterName,"winSize")) 
  ,gproc(gp)
  ,winNotUsed("P(%d)_%s_winNotUsed",gp->getID(), clusterName)
  ,rdRegPool("P(%d)_%s_rdRegPool",gp->getID(), clusterName)
  ,wrRegPool("P(%d)_%s_wrRegPool",gp->getID(), clusterName)
  ,name(strdup(clusterName))
{
  bzero(res,sizeof(Resource *)*iMAX);  
}

void Cluster::buildUnit(const char *clusterName
      ,GMemorySystem *ms
      ,Cluster *cluster
      ,InstOpcode type)
{
  const char *unitType = Instruction::opcode2Name(type);
  
  char utUnit[1024];
  char utLat[1024];
  sprintf(utUnit,"%sUnit",unitType);
  sprintf(utLat,"%sLat",unitType);
  
  if( !SescConf->checkCharPtr(clusterName,utUnit) )
    return;

  const char *unitName = SescConf->getCharPtr(clusterName,utUnit);
  
  TimeDelta_t lat = SescConf->getInt(clusterName,utLat);
  PortGeneric *gen;

  SescConf->isBetween(clusterName,utLat,0,1024);
  UnitMapType::const_iterator it = unitMap.find(unitName);
    
  if( it != unitMap.end() ) {
    gen = it->second.gen;
  }else{
    UnitEntry e;
    e.num = SescConf->getInt(unitName,"Num");
    SescConf->isLT(unitName,"Num",1024);
      
    e.occ = SescConf->getInt(unitName,"occ");
    SescConf->isBetween(unitName,"occ",0,1024);
      
    char name[1024];
    sprintf(name,"%s(%d)", unitName, (int)gproc->getID());
    e.gen = PortGeneric::create(name,e.num,e.occ);

    unitMap[unitName] = e;
    gen = e.gen;
  }

  Resource *r=0;

  char name[100];
  sprintf(name, "Cluster(%d)", (int)gproc->getID());

  bool scooreMemory=false;
  if (SescConf->checkBool("cpusimu", "scooreMemory",gproc->getID()))
    scooreMemory=SescConf->getBool("cpusimu", "scooreMemory",gproc->getID());
  
  switch(type) {
    case iOpInvalid: 
    case iRALU:
      r = new FURALU(type, cluster, gen, lat, scooreMemory, gproc->getID());
      break ;
    case iAALU:
    case iCALU_FPMULT:
    case iCALU_FPDIV:
    case iCALU_FPALU:
    case iCALU_MULT:
    case iCALU_DIV:
      r = new FUGeneric(type, cluster, gen, lat);
      break ;
    case iBALU_LBRANCH:
    case iBALU_LJUMP:
    case iBALU_LCALL:
    case iBALU_RBRANCH:
    case iBALU_RJUMP:
    case iBALU_RCALL:
    case iBALU_RET:
      {
        int32_t MaxBranches = SescConf->getInt("cpusimu", "maxBranches", gproc->getID());
        if( MaxBranches == 0 )
          MaxBranches = INT_MAX;
        bool drainOnMiss = SescConf->getBool("cpusimu", "drainOnMiss", gproc->getID());

        r = new FUBranch(type, cluster, gen, lat, MaxBranches, drainOnMiss);
      }
      break;
    case iLALU_LD: 
      {
        TimeDelta_t ldstdelay=SescConf->getInt("cpusimu", "stForwardDelay",gproc->getID());
        SescConf->isInt("cpusimu", "maxLoads",gproc->getID());
        SescConf->isBetween("cpusimu", "maxLoads", 0, 256*1024, gproc->getID());
        int32_t maxLoads=SescConf->getInt("cpusimu", "maxLoads",gproc->getID());
        if( maxLoads == 0 )
          maxLoads = 256*1024;

        r = new FULoad(type, cluster, gen, gproc->getLSQ(), gproc->getSS(), ldstdelay, lat, ms, maxLoads, gproc->getID(), "specld");
      }
      break;
    case iSALU_LL: 
    case iSALU_SC:
    case iSALU_ST:
    case iSALU_ADDR:
      {
        SescConf->isInt("cpusimu", "maxStores",gproc->getID());
        SescConf->isBetween("cpusimu", "maxStores", 0, 256*1024, gproc->getID());
        int32_t maxStores=SescConf->getInt("cpusimu", "maxStores",gproc->getID());
        if( maxStores == 0 )
          maxStores = 256*1024;

        r = new FUStore(type, cluster, gen, gproc->getLSQ(), gproc->getSS(), lat, ms, maxStores, gproc->getID(), Instruction::opcode2Name(type));
      }
      break;
    default:
      I(0);
      MSG("Unknown unit type [%d] [%s]",type,Instruction::opcode2Name(type));
  }
  I(r);
  I(res[type] == 0);
  res[type] = r;
}

Cluster *Cluster::create(const char *clusterName, GMemorySystem *ms, GProcessor *gproc) {
  // Note: Clusters do NOT share functional units. This breaks the cluster
  // principle. If someone were interested in doing that, the UnitMap sould be
  // cleared (unitMap.clear()) by the PendingWindow instead of buildCluster. If
  // the objective is to share the FUs even between Cores, UnitMap must be
  // declared static in that class (what a mess!)

  unitMap.clear();

  // Constraints
  SescConf->isCharPtr(clusterName,"recycleAt");
  SescConf->isInList(clusterName,"recycleAt","Executing","Execute","Retire");
  const char *recycleAt = SescConf->getCharPtr(clusterName,"recycleAt");
  
  SescConf->isInt(clusterName,"winSize");
  SescConf->isBetween(clusterName,"winSize",1,262144);
  
  Cluster *cluster;
  if( strcasecmp(recycleAt,"retire") == 0) {
    cluster = new RetiredCluster(clusterName, gproc);
  }else if( strcasecmp(recycleAt,"executing") == 0) {
    cluster = new ExecutingCluster(clusterName, gproc);
  }else{
    I( strcasecmp(recycleAt,"execute") == 0);
    cluster = new ExecutedCluster(clusterName, gproc);
  }

  cluster->regPool = SescConf->getInt(clusterName, "nRegs");

  SescConf->isInt(clusterName     , "nRegs");
  SescConf->isBetween(clusterName , "nRegs", 16, 262144);

  for(int32_t t = 0; t < iMAX; t++) {
    cluster->buildUnit(clusterName,ms,cluster,static_cast<InstOpcode>(t));
  }

 // Do not leave useless garbage
  unitMap.clear();

  return cluster;
}


void Cluster::select(DInst *dinst) {
  window.select(dinst);
}


StallCause Cluster::canIssue(DInst *dinst) const { 
  if (regPool<=0)
    return SmallREGStall;

  if (windowSize<=0)
    return SmallWinStall;

  StallCause sc = window.canIssue(dinst);
  if (sc != NoStall)
    return sc;

  return dinst->getClusterResource()->canIssue(dinst);
}

void Cluster::addInst(DInst *dinst) {
  
  rdRegPool.add(2, dinst->getStatsFlag()); // 2 reads

  if (dinst->getInst()->hasDstRegister()) {
    wrRegPool.inc(dinst->getStatsFlag());
    regPool--;
  }

  newEntry();

  window.addInst(dinst);
}

//************ Executing Cluster

void ExecutingCluster::executing(DInst *dinst) {

  delEntry();
}

void ExecutingCluster::executed(DInst *dinst) {

  window.executed(dinst);
}

bool ExecutingCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);

  if( !done )
    return false;

  bool hasDest = (dinst->getInst()->hasDstRegister());

  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ Executed Cluster

void ExecutedCluster::executing(DInst *dinst) {

}

void ExecutedCluster::executed(DInst *dinst) {

  window.executed(dinst);
  I(!dinst->hasPending());

  delEntry();
}

bool ExecutedCluster::retire(DInst *dinst, bool reply) {

  bool done  = dinst->getClusterResource()->retire(dinst, reply);
  if( !done )
    return false;

  bool hasDest = (dinst->getInst()->hasDstRegister());
  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ RetiredCluster

void RetiredCluster::executing(DInst *dinst) {

}

void RetiredCluster::executed(DInst *dinst) {

  window.executed(dinst);
}

bool RetiredCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);
  if( !done )
    return false;

  bool hasDest = (dinst->getInst()->hasDstRegister());

  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());
  delEntry();

  return true;
}

