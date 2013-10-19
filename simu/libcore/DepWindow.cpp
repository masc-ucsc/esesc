/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California Santa Cruz

   Contributed by Jose Renau
                  Basilio Fraguela
                  Smruti Sarangi
                  Luis Ceze

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

#include "DepWindow.h"

#include "DInst.h"
#include "Resource.h"
#include "SescConf.h"
#include "GProcessor.h"

DepWindow::DepWindow(GProcessor *gp, Cluster *aCluster, const char *clusterName)
  :gproc(gp)
  ,srcCluster(aCluster)
  ,Id(gp->getId())
  ,InterClusterLat(SescConf->getInt("cpusimu", "interClusterLat",gp->getId()))
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
    dinst->setWakeUpTime(wakeUpPort->nextSlot() + WakeUpDelay);
    preSelect(dinst);
  }
}

// Look for dependent instructions on the same cluster (do not wakeup,
// just get the time)
void DepWindow::wakeUpDeps(DInst *dinst) {
  I(!dinst->hasDeps());

  // Even if it does not wakeup instructions the port is used
  Time_t wakeUpTime= wakeUpPort->nextSlot();
  //dinst->dump("Clearing:");
  dinst->clearRATEntry(); // Not much impact for OoO, mostly for InOrder

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
  dinst->clearRATEntry(); 

  Resource::selectCB::scheduleAbs(wakeUpTime, dinst->getClusterResource(), dinst);
}

void DepWindow::select(DInst *dinst) {
  I(!dinst->getWakeUpTime());

  Time_t schedTime = schedPort->nextSlot() + SchedDelay;

  I(srcCluster == dinst->getCluster());
  //dinst->executingCB.scheduleAbs(schedTime);
  Resource::executingCB::scheduleAbs(schedTime, dinst->getClusterResource(), dinst);
}

// Called when dinst finished execution. Look for dependent to wakeUp
void DepWindow::executed(DInst *dinst) {
  //  MSG("execute [0x%x] @%lld",dinst, globalClock);

  I(!dinst->hasDeps());

  //dinst->dump("Clearing2:");
  dinst->clearRATEntry();

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

      Time_t when = wakeUpPort->nextSlot();
      if (dstCluster != srcCluster) {
        wrForwardBus.inc(dinst->getStatsFlag());
        when += InterClusterLat;
      }

      dstReady->setWakeUpTime(when);

      preSelect(dstReady);
    }
  }
}

