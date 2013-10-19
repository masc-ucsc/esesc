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
#define ESESC_FUZE_DEBUG = 1

#include <iostream>
#include <iomanip>

#include "MegaOp.h"
#include "MegaOpConfig.h"
#include "InstOpcode.h"
#include "MegaOpTools.h"

/// <summary>
///   Constructor: Slightly more functional
/// </summary>
MegaOp::MegaOp(InstOpcode type) 
  : ready(false)
  , age(0)
  , numImm(0)
  , opcode(type)
  , instrs()
  , pcs()
  , srcs()
  , dsts()
{
  parentOp = NULL;
}
 
/// <summary>
///   Constructor: Slightly more functional
MegaOp::MegaOp(DInst dinst) 
  : ready(false)
  , age(0)
  , numImm(0)
  , instrs()
  , pcs()
  , srcs()
  , dsts()
{
  parentOp = NULL;
  const Instruction *inst = dinst.getInst();
  numImm = inst->hasImm() ? 1 : 0;
  opcode = inst->getOpcode();

  std::vector<RegType> vec = inst->getSrcs();
  for(std::vector<RegType>::iterator iter = vec.begin(); iter != vec.end(); iter++) {
    srcs.insert(*iter);
  }
  
  vec = inst->getDsts();
  for(std::vector<RegType>::iterator iter = vec.begin(); iter != vec.end(); iter++) {
    dsts.insert(*iter);
  }

  filterInvalidRegs(&srcs);
  filterInvalidRegs(&dsts);
  
  pcs.insert(dinst.getPC());
  instrs.push_back(dinst);

  // IanLee1521: Debug
#ifdef ESESC_FUZE_DEBUG
  //visualize("Construction: ");
#endif
}

/// <summary>
///   Check if a RegType is already in the MegaOp
/// </summary>
int MegaOp::addMegaOp(MegaOp newOp) {
  numImm += newOp.numImm;
  instrs.insert(instrs.end(), newOp.instrs.begin(), newOp.instrs.end());
  pcs.insert(newOp.pcs.begin(), newOp.pcs.end());

  for(std::set<RegType>::iterator iter = dsts.begin(); iter != dsts.end(); iter++) {
    newOp.srcs.erase(*iter);
  }
  srcs.insert(newOp.srcs.begin(), newOp.srcs.end());
  dsts.insert(newOp.dsts.begin(), newOp.dsts.end());

  return NO_ERROR;
}

/// <summary>
///   Check if there is a match to the src list
///   Not a perfect implementation, but due to the limited size of these
///     vectors, the number of actual comparisons is fairly small
/// </summary>
bool MegaOp::matchSrcs(std::set<RegType> inSet) {
  std::set<RegType> intersect;
  set_intersection(srcs.begin(), srcs.end(), inSet.begin(), inSet.end(), std::inserter(intersect, intersect.begin()));
  if((int)intersect.size() == 0) {
    return false;
  }
  return true;
}

/// <summary>
///   Check if there is a match to the dst list
///   Not a perfect implementation, but due to the limited size of these
///     vectors, the number of actual comparisons is fairly small
/// </summary>
bool MegaOp::matchDsts(std::set<RegType> inSet) {
  std::set<RegType> intersect;
  set_intersection(dsts.begin(), dsts.end(), inSet.begin(), inSet.end(), std::inserter(intersect, intersect.begin()));
  if((int)intersect.size() == 0) {
    return false;
  }
  return true;
}

/// <summary>
///   Update the megaops to check for ready-ness
/// </summary>
void MegaOp::update() {
  age++;
  if(age >= MegaOpConfig::maxAge) { ready = true; }
}

/// <summary>
///   Rename the source regs in MegaOp from oldReg to newReg
/// </summary>
void MegaOp::renameSrcs(RegType oldReg, RegType newReg) {
   if(srcs.erase(oldReg) == 1) {
     srcs.insert(newReg);
   }
}
/// <summary>
///   Rename the destination regs in MegaOp from oldReg to newReg
/// </summary>
void MegaOp::renameDsts(RegType oldReg, RegType newReg) {
   if(dsts.erase(oldReg) == 1) {
     dsts.insert(newReg);
   }
}

void MegaOp::pruneTmpRegs() {
  srcs.erase(srcs.find(LREG_FUZE0), srcs.end());
  dsts.erase(dsts.find(LREG_FUZE0), dsts.end());
}

void MegaOp::visualize(std::string text = "") {
    std::cout << text;
    std::cout << "#In(" << std::setw(2) << instrs.size() << ") ";
    std::cout << "Op("  << std::setw(2) << opcode << ") ";
    std::cout << "Imm(" << std::setw(2) << numImm << ") ";

    std::cout << "(";
    printRegSet("", srcs);
    std::cout << " -> ";
    printRegSet("", dsts);
    std::cout << ")";
    
    std::cout << std::endl;
}

MegaOp MegaOp::merge(std::list<MegaOp> &list) {
  MegaOp op = MegaOp(iAALU);
  for(std::list<MegaOp>::iterator it = list.begin(); it != list.end(); ++it) {
    op.addMegaOp( *it );
  }
  return op;
}


#endif
