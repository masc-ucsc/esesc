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

#ifdef ESESC_FUZE

#include <iostream>
#include "MegaOp.h"
#include "MegaOpCluster.h"
#include "MegaOpConfig.h"

/// <summary>
///   Default Constructor
/// </summary>
MegaOpCluster::MegaOpCluster()
  : pendingFusions()
  , pendingMegaOps()
  , readyMegaOps()
  , ldq_regs()
  , stq_regs()
  , fpq_regs()
{
    //
}

/// <summary>
///   Rename the MegaOp registers in the cluster. So that internal Regs are replaced with Tmp Regs
/// </summary>
void MegaOpCluster::rename() {
  RegType tmpReg = LREG_FUZE0;
  std::set<RegType> fuzeDsts;
  
  std::list<MegaOp>::iterator renameFocus = pendingMegaOps.end();
  std::list<MegaOp>::iterator renameUpTo;
  std::list<MegaOp>::iterator renameComplete = pendingMegaOps.begin();

  do {
    renameFocus--;
    if(renameFocus->matchDsts(fuzeDsts)) {
      for(std::set<RegType>::iterator chng = fuzeDsts.begin(); chng != fuzeDsts.end(); chng++) {
        renameFocus->renameDsts(*chng, tmpReg);
        
        renameUpTo = renameFocus;
        renameUpTo++;
        std::set<RegType> modded;
        modded.insert(*chng);
        
        for(;renameUpTo != pendingMegaOps.end();renameUpTo++) {
          renameUpTo->renameSrcs(*chng, tmpReg);
          if( renameUpTo->matchDsts(modded) ) {
            break;
          }
        }
      }
      tmpReg = (RegType) (tmpReg + 1);
    } else {
      std::set<RegType> tmpDsts = (*renameFocus).dsts;
      fuzeDsts.insert(tmpDsts.begin(), tmpDsts.end());
    }
  } while (renameFocus != pendingMegaOps.begin());
}

/// <summary>
///   Fuze the MegaOps in the cluster.
/// </summary>
void MegaOpCluster::fuzion(char control) {
  this->clear();

  for(std::list<MegaOp>::iterator iter = pendingMegaOps.begin(); iter != pendingMegaOps.end(); ++iter) {
    MegaOp newMegaOp = *iter;

    // FIXME: Add size checking 
    //
    //if(too many srcs)
    //  if(too many dsts)
    //    if(too many total regs)
    //      aa_MegaOp = readyMegaOps.end();
    //      readyMegaOps.push_back(MegaOp(iAALU));


    switch( control ) {
      case 'B':
        readyMegaOps.push_back( newMegaOp );
        continue;
      
      case 'N':
        switch(newMegaOp.opcode) {
          case iAALU:
          case iSALU_ADDR:
          case iBALU_LBRANCH:
          case iBALU_RBRANCH:
          case iBALU_LJUMP:
          case iBALU_RJUMP:
          case iBALU_LCALL:
          case iBALU_RCALL:
          case iBALU_RET:
            if(pendingFusions.empty()) {
              pendingFusions.push_back( newMegaOp );
            } else {
              pendingFusions.front().addMegaOp( newMegaOp );
            }
            break;
          
          default:
            if(!pendingFusions.empty()) {
              readyMegaOps.push_back(pendingFusions.front());
              pendingFusions.clear();
            }
            readyMegaOps.push_back( newMegaOp );
            break;
        }
        break;
      
      case 'Q':
        switch(newMegaOp.opcode) {
          case iAALU:
          case iSALU_ADDR:
          case iBALU_LBRANCH:
          case iBALU_RBRANCH:
          case iBALU_LJUMP:
          case iBALU_RJUMP:
          case iBALU_LCALL:
          case iBALU_RCALL:
          case iBALU_RET:
            if(newMegaOp.matchSrcs(ldq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingLoads);
              pendingFusions.push_back( MegaOp(iAALU) );
              ldq_regs.clear();
            }
            if(newMegaOp.matchDsts(stq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingStores);
              pendingFusions.push_back( MegaOp(iAALU) );
              stq_regs.clear();
            }
            if(newMegaOp.matchSrcs(fpq_regs) || newMegaOp.matchDsts(fpq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingFPUs);
              pendingFusions.push_back( MegaOp(iAALU) );
              fpq_regs.clear();
            }
            if(pendingFusions.empty()) {
              pendingFusions.push_back( newMegaOp );
            } else {
              pendingFusions.front().addMegaOp( newMegaOp );
            }
            break;
        
          case iLALU_LD:
            pendingLoads.push_back( newMegaOp );
            ldq_regs.insert(newMegaOp.dsts.begin(), newMegaOp.dsts.end());
            break;

          case iSALU_ST:
          case iSALU_LL:
          case iSALU_SC:
            pendingStores.push_back( newMegaOp );
            stq_regs.insert(newMegaOp.srcs.begin(), newMegaOp.srcs.end());
            break;

          case iCALU_FPMULT:
          case iCALU_FPDIV:
          case iCALU_FPALU:
          case iCALU_MULT:
          case iCALU_DIV:
            pendingFPUs.push_back( newMegaOp );
            fpq_regs.insert(newMegaOp.srcs.begin(), newMegaOp.srcs.end());
            fpq_regs.insert(newMegaOp.dsts.begin(), newMegaOp.dsts.end());
            break;

          default:
            // FIXME IanLee1521 -- Throw Error
            //readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
            //pendingFusions.push_back( MegaOp(iAALU) );
            break;
        }
        break;

      case 'D':
        switch(newMegaOp.opcode) {
          case iAALU:
          case iSALU_ADDR:
          case iBALU_LBRANCH:
          case iBALU_RBRANCH:
          case iBALU_LJUMP:
          case iBALU_RJUMP:
          case iBALU_LCALL:
          case iBALU_RCALL:
          case iBALU_RET:
            if(newMegaOp.matchSrcs(ldq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingLoads);
              ldq_regs.clear();
            }
            if(newMegaOp.matchDsts(stq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingStores);
              stq_regs.clear();
            }
            if(newMegaOp.matchSrcs(fpq_regs) || newMegaOp.matchDsts(fpq_regs)) {
              readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
              readyMegaOps.splice(readyMegaOps.end(), pendingFPUs);
              fpq_regs.clear();
            }

            {
              std::list<MegaOp> matches;
              for(std::list<MegaOp>::iterator opIt = pendingFusions.begin(); opIt != pendingFusions.end(); ++opIt) {
                if( newMegaOp.matchSrcs( opIt->srcs ) || 
                    newMegaOp.matchDsts( opIt->dsts ) || 
                    newMegaOp.matchSrcs( opIt->dsts ) ) {
                  matches.push_back( *opIt );
                  opIt = pendingFusions.erase( opIt );
                }
              }

              matches.push_back( newMegaOp );
              MegaOp mergedOp = MegaOp::merge( matches );
              pendingFusions.push_back( mergedOp );
            }
            break;

          case iLALU_LD:
            pendingLoads.push_back( newMegaOp );
            ldq_regs.insert(newMegaOp.dsts.begin(), newMegaOp.dsts.end());
            break;

          case iSALU_ST:
          case iSALU_LL:
          case iSALU_SC:
            pendingStores.push_back( newMegaOp );
            stq_regs.insert(newMegaOp.srcs.begin(), newMegaOp.srcs.end());
            break;

          case iCALU_FPMULT:
          case iCALU_FPDIV:
          case iCALU_FPALU:
          case iCALU_MULT:
          case iCALU_DIV:
            pendingFPUs.push_back( newMegaOp );
            fpq_regs.insert(newMegaOp.srcs.begin(), newMegaOp.srcs.end());
            fpq_regs.insert(newMegaOp.dsts.begin(), newMegaOp.dsts.end());
            break;

          default:
            // FIXME IanLee1521 -- Throw Error
            //readyMegaOps.splice(readyMegaOps.end(), pendingFusions);
            //pendingFusions.push_back( MegaOp(iAALU) );
            break;
        }
        break;

      default:
        break;
    }
  }

  //FIXME IanLee1521 -- Verify that these lists drain in the correct order.
  if(!pendingFusions.empty()) { readyMegaOps.splice(readyMegaOps.end(), pendingFusions); }
  if(!pendingLoads.empty())   { readyMegaOps.splice(readyMegaOps.end(), pendingLoads); }
  if(!pendingStores.empty())  { readyMegaOps.splice(readyMegaOps.end(), pendingStores); }
  if(!pendingFPUs.empty())    { readyMegaOps.splice(readyMegaOps.end(), pendingFPUs); }
}

void MegaOpCluster::visualize(std::string text) {

    std::cout << text << "Cluster::pendingMegaOps.size() = " << pendingMegaOps.size() << std::endl;
    for(std::list<MegaOp>::iterator iter = pendingMegaOps.begin(); iter != pendingMegaOps.end(); iter++) {
      iter->visualize(text + "  ");
    }

    std::cout << text << "Cluster::readyMegaOps.size() = " << readyMegaOps.size() << std::endl;
    for(std::list<MegaOp>::iterator iter = readyMegaOps.begin(); iter != readyMegaOps.end(); iter++) {
      iter->visualize(text + "  ");
    }
    std::cout << std::endl;

}


#endif
