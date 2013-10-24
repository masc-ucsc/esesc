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

#ifndef CLUSTERMANAGER_H
#define CLUSTERMANAGER_H

#include "GStats.h"
#include "DepWindow.h"
#include "Instruction.h"
#include "ClusterScheduler.h"

#include "nanassert.h"

class Resource;
class GMemorySystem;
class GProcessor;

class ClusterManager {
 private:
  ClusterScheduler *scheduler;

 protected:
 public:
  ClusterManager(GMemorySystem *ms, GProcessor *gproc);

  Resource *getResource(DInst *dinst) const {
    return scheduler->getResource(dinst);
  }

};

#endif // CLUSTERMANAGER_H
