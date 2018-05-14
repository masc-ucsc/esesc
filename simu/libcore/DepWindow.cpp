// Contributed by Jose Renau
//                Basilio Fraguela
//                Smruti Sarangi
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

#include "DepWindow.h"

#include "DInst.h"
#include "GProcessor.h"
#include "Resource.h"
#include "SescConf.h"

DepWindow::DepWindow(uint32_t cpuid, Cluster *aCluster, const char *clusterName, uint32_t pos)
    : srcCluster(aCluster)
    , Id(cpuid)
    , wrForwardBus("P(%d)_%s%d_wrForwardBus", cpuid, clusterName, pos) {
  char cadena[1024];

  sprintf(cadena, "P(%d)_%s%d_sched", cpuid, clusterName, pos);
  schedPort =
      PortGeneric::create(cadena, SescConf->getInt(clusterName, "SchedNumPorts"), SescConf->getInt(clusterName, "SchedPortOccp"));

  InterClusterLat = SescConf->getInt("cpusimu", "interClusterLat", cpuid);
  SchedDelay      = SescConf->getInt(clusterName, "schedDelay");

  // Constraints
  SescConf->isInt(clusterName, "schedDelay");
  SescConf->isBetween(clusterName, "schedDelay", 0, 1024);

  SescConf->isInt("cpusimu", "interClusterLat", cpuid);
  SescConf->isBetween("cpusimu", "interClusterLat", SchedDelay, 1024, cpuid);
}

DepWindow::~DepWindow() {
}

StallCause DepWindow::canIssue(DInst *dinst) const {

  return NoStall;
}

void DepWindow::addInst(DInst *dinst) {

  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field

  if(!dinst->hasDeps()) {
    preSelect(dinst);
  }
}

void DepWindow::preSelect(DInst *dinst) {
  // At the end of the wakeUp, we can start to read the register file
  I(!dinst->hasDeps());

  dinst->markIssued();
  I(dinst->getCluster());

  dinst->getCluster()->select(dinst);
}

void DepWindow::select(DInst *dinst) {

  Time_t schedTime = schedPort->nextSlot(dinst->getStatsFlag());
  if(dinst->hasInterCluster())
    schedTime += InterClusterLat;
  else
    schedTime += SchedDelay;

  I(srcCluster == dinst->getCluster());

  Resource::executingCB::scheduleAbs(schedTime, dinst->getClusterResource(), dinst);
}

// Called when dinst finished execution. Look for dependent to wakeUp
void DepWindow::executed(DInst *dinst) {
  //  MSG("execute [0x%x] @%lld",dinst, globalClock);

  I(!dinst->hasDeps());

  dinst->markExecuted();
  dinst->clearRATEntry();

  if(!dinst->hasPending())
    return;

  // NEVER HERE FOR in-order cores

  I(dinst->getCluster());
  I(srcCluster == dinst->getCluster());

  I(dinst->isIssued());
  while(dinst->hasPending()) {
    DInst *dstReady = dinst->getNextPending();
    I(dstReady);

    I(!dstReady->isExecuted());

    if(!dstReady->hasDeps()) {
      // Check dstRes because dstReady may not be issued
      I(dstReady->getCluster());
      const Cluster *dstCluster = dstReady->getCluster();
      I(dstCluster);

      if(dstCluster != srcCluster) {
        wrForwardBus.inc(dstReady->getStatsFlag());
        dstReady->markInterCluster();
      }

      preSelect(dstReady);
    }
  }
}
