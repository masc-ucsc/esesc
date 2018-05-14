// Contributed by Jose Renau
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

#include "Instruction.h"
#include "nanassert.h"
#include <stdio.h>

static const char *opcode2NameTable[] = {
    "iSubInvalid",
    //-----------------
    "iRALU",
    //-----------------
    "iAALU",
    //-----------------
    "iBALU_LBRANCH", "iBALU_RBRANCH", "iBALU_LJUMP", "iBALU_RJUMP", "iBALU_LCALL", "iBALU_RCALL", "iBALU_RET",
    //-----------------
    "iLALU_LD",
    //-----------------
    "iSALU_ST", "iSALU_LL", "iSALU_SC", "iSALU_ADDR",
    //-----------------
    "iCALU_FPMULT", "iCALU_FPDIV", "iCALU_FPALU", "iCALU_MULT", "iCALU_DIV",
    //-----------------
};

const char *Instruction::opcode2Name(InstOpcode op) {
  return opcode2NameTable[op];
}

void Instruction::dump(const char *str) const {
  fprintf(stderr, "%s: reg%d, reg%d = reg%d %11s reg%d", str, dst1, dst2, src1, opcode2NameTable[opcode], src2);
}

void Instruction::set(InstOpcode opcode_, RegType src1_, RegType src2_, RegType dst1_, RegType dst2_) {
  I(opcode_ != iOpInvalid);

  opcode = opcode_;
  src1   = src1_;
  src2   = src2_;
  if(dst1_ == LREG_NoDependence) {
    dst1 = LREG_InvalidOutput;
  } else {
    dst1 = dst1_;
  }
  if(dst2_ == LREG_NoDependence) {
    dst2 = LREG_InvalidOutput;
  } else {
    dst2 = dst2_;
  }
}
