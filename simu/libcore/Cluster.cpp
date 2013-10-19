/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
   Updated by     Milos Prvulovic

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
  ,winNotUsed("P(%d)_%s_winNotUsed",gp->getId(), clusterName)
  ,rdRegPool("P(%d)_%s_rdRegPool",gp->getId(), clusterName)
  ,wrRegPool("P(%d)_%s_wrRegPool",gp->getId(), clusterName)
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
    sprintf(name,"%s(%d)", unitName, (int)gproc->getId());
    e.gen = PortGeneric::create(name,e.num,e.occ);

    unitMap[unitName] = e;
    gen = e.gen;
  }

  Resource *r=0;

  char name[100];
  sprintf(name, "Cluster(%d)", (int)gproc->getId());

  bool scooreMemory=false;
  if (SescConf->checkBool("cpusimu", "scooreMemory",gproc->getId()))
    scooreMemory=SescConf->getBool("cpusimu", "scooreMemory",gproc->getId());
  
  bool noMemSpec = SescConf->getBool("cpusimu", "noMemSpec",gproc->getId());

  switch(type) {
    case iOpInvalid: 
    case iRALU:
      r = new FURALU(cluster, gen, lat, scooreMemory, gproc->getId());
      break ;
    case iAALU:
    case iCALU_FPMULT:
    case iCALU_FPDIV:
    case iCALU_FPALU:
    case iCALU_MULT:
    case iCALU_DIV:
      r = new FUGeneric(cluster, gen, lat);
      break ;
    case iFUZE:
      //ESESC_FUZE FIXME: IANLEE1521
      r = new FUFuze(cluster, gen, lat);
      //r = new FUGeneric(cluster, gen, lat);
      break;
    case iBALU_LBRANCH:
    case iBALU_LJUMP:
    case iBALU_LCALL:
    case iBALU_RBRANCH:
    case iBALU_RJUMP:
    case iBALU_RCALL:
    case iBALU_RET:
      {
        int32_t MaxBranches = SescConf->getInt("cpusimu", "maxBranches", gproc->getId());
        if( MaxBranches == 0 )
          MaxBranches = INT_MAX;

        r = new FUBranch(cluster, gen, lat, MaxBranches);
      }
      break;
    case iLALU_LD: 
      {
        if(scooreMemory){
          r = new FUSCOORELoad(cluster, gen, gproc->getSS(), lat, ms, gproc->getId(), "scooreld");
        }else{
          TimeDelta_t ldstdelay=SescConf->getInt("cpusimu", "stForwardDelay",gproc->getId());
          SescConf->isInt("cpusimu", "maxLoads",gproc->getId());
          SescConf->isBetween("cpusimu", "maxLoads", 0, 256*1024, gproc->getId());
          int32_t maxLoads=SescConf->getInt("cpusimu", "maxLoads",gproc->getId());
          if( maxLoads == 0 )
            maxLoads = 256*1024;

          if(noMemSpec){        
            r = new FULoad_noMemSpec(cluster, gen, ldstdelay, lat, ms, maxLoads, gproc->getId(), "nonspecld");
          }else{
            r = new FULoad(cluster, gen, gproc->getLSQ(), gproc->getSS(), ldstdelay, lat, ms, maxLoads, gproc->getId(), "specld");
          }
        }
      }
      break;
    case iSALU_LL: 
    case iSALU_SC:
    case iSALU_ST:
    case iSALU_ADDR:
      {
        if(scooreMemory){
          r = new FUSCOOREStore(cluster, gen, gproc->getSS(), lat, ms, gproc->getId(), "scoorest");
        }else{
          SescConf->isInt("cpusimu", "maxStores",gproc->getId());
          SescConf->isBetween("cpusimu", "maxStores", 0, 256*1024, gproc->getId());
          int32_t maxStores=SescConf->getInt("cpusimu", "maxStores",gproc->getId());
          if( maxStores == 0 )
            maxStores = 256*1024;

          if(noMemSpec){
            r = new FUStore_noMemSpec(cluster, gen, lat, ms, maxStores, gproc->getId(), Instruction::opcode2Name(type));
          }else{
            r = new FUStore(cluster, gen, gproc->getLSQ(), gproc->getSS(), lat, ms, maxStores, gproc->getId(), Instruction::opcode2Name(type));
          }
        }
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

#ifdef ESESC_FUZE
  if (dinst->getInst()->hasDstRegister()) {
#else
  if (dinst->getInst()->hasDstRegister()) {
#endif

    wrRegPool.inc(dinst->getStatsFlag());
    regPool--;
  }

  window.addInst(dinst);

  newEntry();
}

//************ Executing Cluster

void ExecutingCluster::executing(DInst *dinst) {

  window.wakeUpDeps(dinst);
  delEntry();
}

void ExecutingCluster::executed(DInst *dinst) {

  window.executed(dinst);
}

bool ExecutingCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);

  if( !done )
    return false;

#ifdef ESESC_FUZE
  bool hasDest = (dinst->getInst()->hasDstRegister());
#else
  bool hasDest = (dinst->getInst()->hasDstRegister());
#endif

  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ Executed Cluster

void ExecutedCluster::executing(DInst *dinst) {

  window.wakeUpDeps(dinst);
}

void ExecutedCluster::executed(DInst *dinst) {

  window.executed(dinst);

  delEntry();
}

bool ExecutedCluster::retire(DInst *dinst, bool reply) {

  bool done  = dinst->getClusterResource()->retire(dinst, reply);
  if( !done )
    return false;

#ifdef ESESC_FUZE
  bool hasDest = (dinst->getInst()->hasDstRegister());
#else
  bool hasDest = (dinst->getInst()->hasDstRegister());
#endif
  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());

  return true;
}

//************ RetiredCluster

void RetiredCluster::executing(DInst *dinst) {

  window.wakeUpDeps(dinst);
}

void RetiredCluster::executed(DInst *dinst) {

  window.executed(dinst);
}

bool RetiredCluster::retire(DInst *dinst, bool reply) {

  bool done = dinst->getClusterResource()->retire(dinst, reply);
  if( !done )
    return false;

#ifdef ESESC_FUZE
  bool hasDest = (dinst->getInst()->hasDstRegister());
#else
  bool hasDest = (dinst->getInst()->hasDstRegister());
#endif

  if( hasDest )
    regPool++;

  winNotUsed.sample(windowSize, dinst->getStatsFlag());
  delEntry();

  return true;
}

