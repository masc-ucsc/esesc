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

#include "ClusterManager.h"
#include "ClusterScheduler.h"

#include "Cluster.h"
#include "GMemorySystem.h"
#include "Resource.h"
#include "SescConf.h"

ClusterManager::ClusterManager(GMemorySystem *ms, uint32_t cpuid, GProcessor *gproc) {

  ResourcesPoolType res(iMAX);

  IN(forall((size_t i = 1; i < static_cast<size_t>(iMAX); i++), res[i].empty()));

  const char *coreSection = SescConf->getCharPtr("", "cpusimu", cpuid);
  if(coreSection == 0)
    return; // No core section, bad conf

  int32_t nClusters = SescConf->getRecordSize(coreSection, "cluster");

  for(int32_t i = 0; i < nClusters; i++) {
    const char *clusterName = SescConf->getCharPtr(coreSection, "cluster", i);
    SescConf->isCharPtr(coreSection, "cluster", i);

    Cluster *cluster = Cluster::create(clusterName, i, ms, cpuid, gproc);

    for(int32_t t = 0; t < iMAX; t++) {
      Resource *r = cluster->getResource(static_cast<InstOpcode>(t));

      if(r)
        res[t].push_back(r);
    }
  }

  const char *clusterScheduler = SescConf->getCharPtr(coreSection, "clusterScheduler");

  if(strcasecmp(clusterScheduler, "RoundRobin") == 0) {
    scheduler = new RoundRobinClusterScheduler(res);
  } else if(strcasecmp(clusterScheduler, "LRU") == 0) {
    scheduler = new LRUClusterScheduler(res);
  } else if(strcasecmp(clusterScheduler, "Use") == 0) {
    scheduler = new UseClusterScheduler(res);
  } else {
    MSG("ERROR: Invalid clusterScheduler [%s]", clusterScheduler);
    SescConf->notCorrect();
    return;
  }

  // 0 is an invalid opcde. All the other should be defined
  for(InstOpcode i = static_cast<InstOpcode>(1); i < iMAX; i = static_cast<InstOpcode>((int)i + 1)) {
    if(!res[i].empty())
      continue;

    MSG("ERROR: missing %s instruction type support", Instruction::opcode2Name(i));

    SescConf->notCorrect();
  }
}
