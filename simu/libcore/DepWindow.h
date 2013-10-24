/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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

#ifndef DEPWINDOW_H
#define DEPWINDOW_H

#include "nanassert.h"

#include "Resource.h"
#include "Port.h"

class DInst;
class GProcessor;
class Cluster;

class DepWindow {
private:
  GProcessor *gproc;
  Cluster    *srcCluster;

  const int32_t Id;

  const TimeDelta_t InterClusterLat;
  const TimeDelta_t WakeUpDelay;
  const TimeDelta_t SchedDelay;
  const TimeDelta_t RegFileDelay;

  GStatsCntr wrForwardBus;

  PortGeneric *wakeUpPort;
  PortGeneric *schedPort;

protected:
  void preSelect(DInst *dinst);

public:
  ~DepWindow();
  DepWindow(GProcessor *gp, Cluster *aCluster, const char *clusterName);

  void wakeUpDeps(DInst *dinst);

  void select(DInst *dinst);

  StallCause canIssue(DInst *dinst) const;
  void addInst(DInst *dinst);
  void executed(DInst *dinst);
};

#endif // DEPWINDOW_H
