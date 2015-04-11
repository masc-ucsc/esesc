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
#include "Resource.h"
#include "SescConf.h"
#include "GProcessor.h"

DepWindow::DepWindow(GProcessor *gp, Cluster *aCluster, const char *clusterName)
  :srcCluster(aCluster)
  ,Id(gp->getID())
  ,InterClusterLat(SescConf->getInt("cpusimu", "interClusterLat",gp->getID()))
  ,WakeUpDelay(SescConf->getInt(clusterName, "wakeupDelay"))
  ,SchedDelay(SescConf->getInt(clusterName, "schedDelay"))
  ,RegFileDelay(SescConf->getInt(clusterName, "regFileDelay"))
  ,wrForwardBus("P(%d)_%s_wrForwardBus",Id, clusterName)
{
  char cadena[100];
  sprintf(cadena,"P(%d)_%s_wakeUp", Id, clusterName);
  wakeUpPort = PortGeneric::create(cadena
                                 ,SescConf->getInt(clusterName, "wakeUpNumPorts")
                                 ,SescConf->getInt(clusterName, "wakeUpPortOccp"));

  SescConf->isInt(clusterName, "wakeupDelay");
  SescConf->isBetween(clusterName, "wakeupDelay", 0, 1024);

  sprintf(cadena,"P(%d)_%s_sched", Id, clusterName);
  schedPort = PortGeneric::create(cadena
                                  ,SescConf->getInt(clusterName, "SchedNumPorts")
                                  ,SescConf->getInt(clusterName, "SchedPortOccp"));

  // Constraints
  SescConf->isInt(clusterName    , "schedDelay");
  SescConf->isBetween(clusterName , "schedDelay", 0, 1024);

  SescConf->isInt("cpusimu"    , "interClusterLat",Id);
  SescConf->isBetween("cpusimu" , "interClusterLat", 0, 1024,Id);

  SescConf->isInt(clusterName    , "regFileDelay");
  SescConf->isBetween(clusterName , "regFileDelay", 0, 1024);
}

DepWindow::~DepWindow() {
}

StallCause DepWindow::canIssue(DInst *dinst) const {

  return NoStall;
}

void DepWindow::addInst(DInst *dinst) {

  I(dinst->getCluster() != 0); // Resource::schedule must set the resource field

  if (!dinst->hasDeps()) {
    dinst->setWakeUpTime(wakeUpPort->nextSlot(dinst->getStatsFlag()) + WakeUpDelay);
    preSelect(dinst);
  }
}

// Look for dependent instructions on the same cluster (do not wakeup,
// just get the time)
void DepWindow::wakeUpDeps(DInst *dinst) {
  I(!dinst->hasDeps());

  // Even if it does not wakeup instructions the port is used
  Time_t wakeUpTime= wakeUpPort->nextSlot(dinst->getStatsFlag());
  //dinst->dump("Clearing:");

  if (!dinst->hasPending())
    return;

  // NEVER HERE FOR in-order cores

  wakeUpTime += WakeUpDelay;

  I(dinst->getCluster());
  I(srcCluster == dinst->getCluster());

  I(dinst->hasPending());
  for(const DInstNext *it = dinst->getFirst();
       it ;
       it = it->getNext() ) {
    DInst *dstReady = it->getDInst();

    const Cluster *dstCluster = dstReady->getCluster();
    I(dstCluster); // all the instructions should have a resource after rename stage

    if (dstCluster == srcCluster && dstReady->getWakeUpTime() < wakeUpTime)
      dstReady->setWakeUpTime(wakeUpTime);
  }
}

void DepWindow::preSelect(DInst *dinst) {
  // At the end of the wakeUp, we can start to read the register file
  I(dinst->getWakeUpTime());
  I(!dinst->hasDeps());

  Time_t wakeUpTime = dinst->getWakeUpTime() + RegFileDelay;

  IS(dinst->setWakeUpTime(0));
  dinst->markIssued();
  I(dinst->getCluster());
  //dinst->clearRATEntry(); 

  Resource::selectCB::scheduleAbs(wakeUpTime, dinst->getClusterResource(), dinst);
}

void DepWindow::select(DInst *dinst) {
  I(!dinst->getWakeUpTime());

  Time_t schedTime = schedPort->nextSlot(dinst->getStatsFlag()) + SchedDelay;

  I(srcCluster == dinst->getCluster());
  //dinst->executingCB.scheduleAbs(schedTime);
  Resource::executingCB::scheduleAbs(schedTime, dinst->getClusterResource(), dinst);
}

// Called when dinst finished execution. Look for dependent to wakeUp
void DepWindow::executed(DInst *dinst) {
  //  MSG("execute [0x%x] @%lld",dinst, globalClock);

  I(!dinst->hasDeps());

  //dinst->dump("Clearing2:");

  if (!dinst->hasPending())
    return;

  // NEVER HERE FOR in-order cores

  I(dinst->getCluster());
  I(srcCluster == dinst->getCluster());

  // Only until reaches last. The instructions that are from another processor
  // should be added again to the dependence chain so that MemRequest::ack can
  // awake them (other processor instructions)

  const DInst *stopAtDst = 0;

  I(dinst->isIssued());
  while (dinst->hasPending()) {

    if (stopAtDst == dinst->getFirstPending())
      break;
    DInst *dstReady = dinst->getNextPending();
    I(dstReady);

#if 0
    if (!dstReady->isIssued()) {
      I(dinst->getInst()->isStore());

      I(!dstReady->hasDeps());
      continue;
    }
#endif
    I(!dstReady->isExecuted());

    if (!dstReady->hasDeps()) {
      // Check dstRes because dstReady may not be issued
      I(dstReady->getCluster());
      const Cluster *dstCluster = dstReady->getCluster();
      I(dstCluster);

      Time_t when = wakeUpPort->nextSlot(dinst->getStatsFlag());
      if (dstCluster != srcCluster) {
        wrForwardBus.inc(dinst->getStatsFlag());
        when += InterClusterLat;
      }

      dstReady->setWakeUpTime(when);

      preSelect(dstReady);
    }
  }
}

