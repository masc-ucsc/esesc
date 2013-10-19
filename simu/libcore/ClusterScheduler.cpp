/* License & includes {{{1 */
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
#include "Resource.h"
#include "SescConf.h"
#include "ClusterScheduler.h"
/* }}} */
//#define DUMP_TRACE 1

ClusterScheduler::ClusterScheduler(const ResourcesPoolType ores) 
  : res(ores) {
}

ClusterScheduler::~ClusterScheduler() {
  
}

RoundRobinClusterScheduler::RoundRobinClusterScheduler(const ResourcesPoolType ores)
  : ClusterScheduler(ores) {
  
  nres.resize(res.size());
  pos.resize(res.size(),0);

  for(size_t i=0;i<res.size();i++) {
    nres[i] = res[i].size();
  }
}

RoundRobinClusterScheduler::~RoundRobinClusterScheduler() {
  
}

Resource *RoundRobinClusterScheduler::getResource(DInst *dinst) {
  const Instruction *inst=dinst->getInst();
  InstOpcode type = inst->getOpcode();

  unsigned int i = pos[type];
  if (i>=nres[type]) {
    i = 0;
    pos[type] = 1;
  }else{
    pos[type]++;
  }

  I(i<res[type].size());

  return res[type][i];
}

LRUClusterScheduler::LRUClusterScheduler(const ResourcesPoolType ores)
  : ClusterScheduler(ores) {
  
}

LRUClusterScheduler::~LRUClusterScheduler() {
  
}

Resource *LRUClusterScheduler::getResource(DInst *dinst) {
  const Instruction *inst=dinst->getInst();
  InstOpcode type = inst->getOpcode();
  Resource *touse = res[type][0];
  
  for(size_t i = 1; i<res[type].size(); i++) {
    if (touse->getUsedTime() > res[type][i]->getUsedTime())
      touse = res[type][i];
  }
  
  touse->setUsedTime(); 
  return touse;
}
