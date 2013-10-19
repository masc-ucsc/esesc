/*
 ESESC: Enhanced Super ESCalar simulator
 Copyright (C) 2012 University of California, Santa Cruz.

 Contributed by  Ian Lee

 This file is part of ESESC.

 ESESC is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2, or (at your option) any later version.

 ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should  have received a copy of  the GNU General  Public License along with
 ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef MEGAOPCLUSTER_H
#define MEGAOPCLUSTER_H

#include <vector>
#include <set>

#include "DInst.h"
#include "Instruction.h"
#include "MegaOp.h"

class MegaOpCluster {
  private:
    std::list<MegaOp> pendingFusions;
    std::list<MegaOp> pendingLoads;
    std::list<MegaOp> pendingStores;
    std::list<MegaOp> pendingFPUs;

  public:
    std::list<MegaOp> pendingMegaOps;
    std::list<MegaOp> readyMegaOps;

    std::set<RegType> ldq_regs;
    std::set<RegType> stq_regs;
    std::set<RegType> fpq_regs;

    MegaOpCluster();
    
    void rename();

    // Control Codes:
    //  B = Baseline
    //  N = Naive In-Order
    //  Q = Queue
    //  D = Dynamic
    //  I = Perfect # Instructions
    //  R = Perfect # Regs
    void fuzion(char control = 'B');
    void clear() {
      pendingFusions.clear();
      pendingLoads.clear();
      pendingStores.clear();
      pendingFPUs.clear();

      readyMegaOps.clear();
      ldq_regs.clear();
      stq_regs.clear();
      fpq_regs.clear();
    }
    void reset() {
      this->clear();
      pendingMegaOps.clear();
    }
    void visualize(std::string text = "");
};

#endif // MEGAOPCLUSTER_H



