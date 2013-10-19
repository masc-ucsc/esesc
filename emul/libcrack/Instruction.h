/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.
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

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "nanassert.h"

#include <stdint.h>
#include <algorithm>
#include <vector>

#include "InstOpcode.h"

class Instruction {
private:
protected:
  InstOpcode  opcode;
#ifdef ESESC_FUZE
  std::vector<RegType> srcs;
  std::vector<RegType> dsts;
  bool imm;
#else
  RegType     src1;
  RegType     src2;
  RegType     dst1;
  RegType     dst2;
#endif
  
  public:
#ifdef ESESC_FUZE
  Instruction();
#endif

  static const char *opcode2Name(InstOpcode type);
  void set(InstOpcode op, RegType src1, RegType src2, RegType dst1, RegType dst2, bool useImm);
  bool doesJump2Label() const  { return opcode == iBALU_LBRANCH || opcode == iBALU_LJUMP || opcode == iBALU_LCALL; }
  InstOpcode getOpcode() const { return opcode; }
  void forcemult() {opcode = iCALU_FPMULT; }

#ifdef ESESC_FUZE
  void set(InstOpcode opcode_, std::vector<RegType> srcs_, std::vector<RegType> dsts_, bool useImm);

  RegType getSrc(uint32_t i) const { return srcs[i]; }
  RegType getDst(uint32_t i) const { return dsts[i]; }
  std::vector<RegType> getSrcs() const { return srcs; }
  std::vector<RegType> getDsts() const { return dsts; }

  bool hasDstRegister() const;
  bool hasSrcRegister() const;
  bool hasImm() const { return imm; };

#else
  RegType getSrc1() const { return src1;  }
  RegType getSrc2() const { return src2;  }
  RegType getDst1() const { return dst1;  }
  RegType getDst2() const { return dst2;  }

  // if dst == Invalid -> dst2 == invalid
  bool hasDstRegister() const { return dst1 != LREG_InvalidOutput || dst2 != LREG_InvalidOutput; }
  bool hasSrc1Register() const { return src1 != LREG_NoDependence;  }
  bool hasSrc2Register() const { return src2 != LREG_NoDependence;  }

  bool hasImm() const { I(0); return false; };
#endif

  bool isFuncCall() const { return opcode == iBALU_RCALL   || opcode == iBALU_LCALL;   }
  bool isFuncRet()  const { return opcode == iBALU_RET;    }
  // All the conditional control flow instructions are branches
  bool isBranch()   const { return opcode == iBALU_RBRANCH || opcode == iBALU_LBRANCH; }
  // All the unconditional but function return are jumps
  bool isJump()     const { return opcode == iBALU_RJUMP   || opcode == iBALU_LJUMP || isFuncCall(); }

  bool isControl()  const { 
    GI(opcode >= iBALU_LBRANCH && opcode <= iBALU_RET, isJump() || isBranch() || isFuncCall() || isFuncRet());

    return opcode >= iBALU_LBRANCH && opcode <= iBALU_RET; 
  }

  bool isLoad() const         { return opcode == iLALU_LD; }
  bool isStore() const        { return opcode == iSALU_ST; }
  bool isStoreAddress() const { return opcode == iSALU_ADDR; }

  bool isMemory() const   { return opcode == iSALU_ST || opcode == iLALU_LD; }

  void dump(const char *str) const;
};

#endif   // INSTRUCTION_H

