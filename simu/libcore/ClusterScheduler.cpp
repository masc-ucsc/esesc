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

#include "ClusterScheduler.h"
#include "Cluster.h"
#include "Resource.h"
#include "SescConf.h"
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
  pos.resize(res.size(), 0);

  for(size_t i = 0; i < res.size(); i++) {
    nres[i] = res[i].size();
  }
}

RoundRobinClusterScheduler::~RoundRobinClusterScheduler() {
}

Resource *RoundRobinClusterScheduler::getResource(DInst *dinst) {
  const Instruction *inst = dinst->getInst();
  InstOpcode         type = inst->getOpcode();

  unsigned int i = pos[type];
  if(i >= nres[type]) {
    i         = 0;
    pos[type] = 1;
  } else {
    pos[type]++;
  }

  I(i < res[type].size());

  return res[type][i];
}

LRUClusterScheduler::LRUClusterScheduler(const ResourcesPoolType ores)
    : ClusterScheduler(ores) {
}

LRUClusterScheduler::~LRUClusterScheduler() {
}

Resource *LRUClusterScheduler::getResource(DInst *dinst) {
  const Instruction *inst  = dinst->getInst();
  InstOpcode         type  = inst->getOpcode();
  Resource *         touse = res[type][0];

  for(size_t i = 1; i < res[type].size(); i++) {
    if(touse->getUsedTime() > res[type][i]->getUsedTime())
      touse = res[type][i];
  }

  touse->setUsedTime();
  return touse;
}

UseClusterScheduler::UseClusterScheduler(const ResourcesPoolType ores)
    : ClusterScheduler(ores) {

  nres.resize(res.size());
  pos.resize(res.size(), 0);

  for(size_t i = 0; i < res.size(); i++) {
    nres[i] = res[i].size();
  }
  for(size_t i = 0; i < LREG_MAX; i++) {
    cused[i] = 0;
  }
}

UseClusterScheduler::~UseClusterScheduler() {
}

Resource *UseClusterScheduler::getResource(DInst *dinst) {
  const Instruction *inst = dinst->getInst();
  InstOpcode         type = inst->getOpcode();

  unsigned int p = pos[type];
  if(p >= nres[type]) {
    p         = 0;
    pos[type] = 1;
  } else {
    pos[type]++;
  }

  Resource *touse = res[type][p];

  int touse_nintra = (cused[inst->getSrc1()] != res[type][p]->getCluster() && cused[inst->getSrc1()]) +
                     (cused[inst->getSrc2()] != res[type][p]->getCluster() && cused[inst->getSrc2()]);

#if 0
  if (touse_nintra==0 && touse->getCluster()->getAvailSpace()>0 && touse->getCluster()->getNReady() <= 2)
    return touse;
#endif

  for(size_t i = 0; i < res[type].size(); i++) {
    uint16_t n = p + i;
    if(n >= res[type].size())
      n -= res[type].size();

    if(res[type][n]->getCluster()->getAvailSpace() <= 0)
      continue;

    int nintra = (cused[inst->getSrc1()] != res[type][n]->getCluster() && cused[inst->getSrc1()]) +
                 (cused[inst->getSrc2()] != res[type][n]->getCluster() && cused[inst->getSrc2()]);

#if 1
    if(nintra == touse_nintra && touse->getCluster()->getNReady() < res[type][n]->getCluster()->getNReady()) {
      // if (nintra == touse_nintra && touse->getCluster()->getAvailSpace() < res[type][n]->getCluster()->getAvailSpace()) {
      // Same # deps, pick the less busy
      touse        = res[type][n];
      touse_nintra = nintra;
    } else if(nintra < touse_nintra) {
      touse        = res[type][n];
      touse_nintra = nintra;
    }
#else
    if(touse->getCluster()->getAvailSpace() > res[type][n]->getCluster()->getAvailSpace()) {
      touse        = res[type][n];
      touse_nintra = nintra;
    }
#endif
  }

  cused[inst->getDst1()] = touse->getCluster();
  cused[inst->getDst2()] = touse->getCluster();
  I(cused[LREG_NoDependence] == 0);

  return touse;
}
