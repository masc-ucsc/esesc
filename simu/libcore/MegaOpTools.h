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

#ifndef MEGAOPTOOLS_H
#define MEGAOPTOOLS_H

#include <algorithm>
#include <set>
#include <iostream>

#include "DInst.h"
#include "Instruction.h"

void printRegSet(std::string text, std::set<RegType> myset) {
  std::cout << text;
  for(std::set<RegType>::iterator iter = myset.begin(); iter != myset.end(); iter++) {
    std::cout << " " << *iter;
  }
  //std::cout << std::endl;
}

void filterInvalidRegs(std::set<RegType> *myset) {
  myset->erase((RegType) 0);
  for(RegType i = LREG_INVALID; i <= LREG_MAX; i = (RegType) (i + 1)) {
    myset->erase((RegType) i);
  }
}

std::set<RegType> convertVecToSet(std::vector<RegType> vec) {
  std::set<RegType> myset;
  for(std::vector<RegType>::iterator iter = vec.begin(); iter != vec.end(); iter++) {
    myset.insert(*iter);
  }
  return myset;
}

bool regSetIntersect(std::set<RegType> set1, std::set<RegType> set2) {
  std::set<RegType> intersect;
  //set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(), std::inserter(intersect, intersect.begin()));
  if((int)intersect.size() == 0) {
    return false;
  }
  return true;
  
}

#endif // MEGAOPTOOLS_H



