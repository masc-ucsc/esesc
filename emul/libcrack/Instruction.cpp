/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Luis Ceze

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

#include <stdio.h>
#include "Instruction.h"
#include "nanassert.h"

static const char *opcode2NameTable[] = {
  "iSubInvalid",
  //-----------------
  "iRALU",
  //-----------------
  "iAALU",
  //-----------------
  "iBALU_LBRANCH",
  "iBALU_RBRANCH",
  "iBALU_LJUMP",
  "iBALU_RJUMP",
  "iBALU_LCALL",
  "iBALU_RCALL",
  "iBALU_RET",
  //-----------------
  "iLALU_LD",
  //-----------------
  "iSALU_ST",
  "iSALU_LL",
  "iSALU_SC",
  "iSALU_ADDR",
  //-----------------
  "iCALU_FPMULT",
  "iCALU_FPDIV",
  "iCALU_FPALU",
  "iCALU_MULT",
  "iCALU_DIV",
  //-----------------
  "iFUZE"
};

const char *Instruction::opcode2Name(InstOpcode op) {
  return opcode2NameTable[op];
}



#ifdef ESESC_FUZE
Instruction::Instruction() :
  // Ideally these should be equal to DInst::MAX_PENDING_SOURCES
  // Here just set to some large number
  srcs(16),
  dsts(16)
  {
}


void Instruction::dump(const char *str) const {
  //printf("%s: reg%d, reg%d = reg%d %11s reg%d", str, dst1, dst2, src1, opcode2NameTable[opcode], src2);
  printf("%s: ", str);
  
  for( std::vector<RegType>::const_iterator iter = dsts.begin(); iter != dsts.end(); iter++ ) {
    printf("dreg%d, ", *iter);
  }
  
  printf("%11s ", opcode2NameTable[opcode]);

  for( std::vector<RegType>::const_iterator iter = dsts.begin(); iter != dsts.end(); iter++ ) {
    printf("sreg%d, ", *iter);
  }
}

bool Instruction::hasDstRegister() const { 
  bool hasReg = false;
  for( std::vector<RegType>::const_iterator iter = dsts.begin(); iter != dsts.end(); iter++ ) {
    hasReg = hasReg || (*iter != LREG_InvalidOutput);
  }
  return hasReg;
}

bool Instruction::hasSrcRegister() const { 
  bool hasReg = false;
  for(std::vector<RegType>::const_iterator iter = srcs.begin(); iter != srcs.end(); iter++) {
    hasReg = hasReg || (*iter != LREG_NoDependence);
  }
  return hasReg;
}

void Instruction::set(InstOpcode opcode_, std::vector<RegType> srcs_, std::vector<RegType> dsts_, bool useImm) {
  I(opcode_ != iOpInvalid);

  opcode    = opcode_;
  srcs      = srcs_;
  dsts      = dsts_;
  imm       = useImm;

  for(std::vector<RegType>::iterator iter = dsts.begin(); iter != dsts.end(); iter++) {
    if(*iter == LREG_NoDependence) {
      *iter = LREG_InvalidOutput;
    }
  }
}

void Instruction::set(InstOpcode opcode_, RegType src0_, RegType src1_, RegType dst0_, RegType dst1_, bool useImm) {
  I(opcode_ != iOpInvalid);

  opcode    = opcode_;
  srcs[0]   = src0_;
  srcs[1]   = src1_;
  imm       = useImm;
  
  if (dst0_ == LREG_NoDependence) { 
    dsts[0] = LREG_InvalidOutput;
  }else{
    dsts[0] = dst0_;
  }
  if (dst1_ == LREG_NoDependence) { 
    dsts[1] = LREG_InvalidOutput;
  }else{
    dsts[1] = dst1_;
  }
}


#else

void Instruction::dump(const char *str) const {
  printf("%s: reg%d, reg%d = reg%d %11s reg%d",
      str, dst1, dst2, src1, opcode2NameTable[opcode], src2);
}

void Instruction::set(InstOpcode opcode_, RegType src1_, RegType src2_, RegType dst1_, RegType dst2_, bool useImm) 
{
  I(opcode_ != iOpInvalid);

  opcode    = opcode_;
  src1      = src1_;
  src2      = src2_;
  if (dst1_ == LREG_NoDependence) { 
    dst1 = LREG_InvalidOutput;
  }else{
    dst1 = dst1_;
  }
  if (dst2_ == LREG_NoDependence) { 
    dst2 = LREG_InvalidOutput;
  }else{
    dst2 = dst2_;
  }
}

#endif
