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

#include "ClusterManager.h"
#include "ClusterScheduler.h"

#include "Cluster.h"
#include "GMemorySystem.h"
#include "GProcessor.h"
#include "Resource.h"
#include "SescConf.h"


ClusterManager::ClusterManager(GMemorySystem *ms, GProcessor *gproc) {

  ResourcesPoolType res(iMAX);

  IN(forall((size_t i=1;i<static_cast<size_t>(iMAX);i++),res[i].empty()));

  const char *coreSection = SescConf->getCharPtr("","cpusimu",gproc->getId());
  if(coreSection == 0) 
    return;  // No core section, bad conf

  int32_t nClusters = SescConf->getRecordSize(coreSection,"cluster");
  
  for(int32_t i=0;i<nClusters;i++) {
    const char *clusterName = SescConf->getCharPtr(coreSection,"cluster",i);
    SescConf->isCharPtr(coreSection,"cluster",i);
    
    Cluster *cluster = Cluster::create(clusterName, ms, gproc);

    for(int32_t t = 0; t < iMAX; t++) {
      Resource *r = cluster->getResource(static_cast<InstOpcode>(t));

      if (r)
        res[t].push_back(r);
    }
  }

  const char *clusterScheduler = SescConf->getCharPtr(coreSection,"clusterScheduler");

  if (strcasecmp(clusterScheduler,"RoundRobin")== 0) {
    scheduler = new RoundRobinClusterScheduler(res);
  }else if (strcasecmp(clusterScheduler,"LRU")== 0) {
    scheduler = new LRUClusterScheduler(res);
  }else{
    MSG("ERROR: Invalid clusterScheduler [%s]",clusterScheduler);
    SescConf->notCorrect();
    return;
  }
  
  // 0 is an invalid opcde. All the other should be defined
  for(InstOpcode i=static_cast<InstOpcode>(1);i<iMAX;i = static_cast<InstOpcode>((int)i + 1)) {
    if(!res[i].empty())
      continue;

    MSG("ERROR: missing %s instruction type support",Instruction::opcode2Name(i));

    SescConf->notCorrect();
  }

}

