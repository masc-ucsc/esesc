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

#ifndef INSTOPCODE_H
#define INSTOPCODE_H
#include <stdint.h>
#include <sys/types.h>

enum InstOpcode {
  iOpInvalid = 0,
  //-----------------
  iRALU,
  //-----------------
  iAALU, // Can got to BUNIT/AUNIT/SUNIT in scoore
  //-----------------
  iBALU_LBRANCH, // branch label/immediate
  iBALU_RBRANCH, // branch register
  iBALU_LJUMP,   // jump label/immediate
  iBALU_RJUMP,   // jump register
  iBALU_LCALL,   // call label/immediate
  iBALU_RCALL,   // call register (same as RBRANCH, but notify predictor)
  iBALU_RET,     // func return (same as RBRANCH, but notify predictor)
  //-----------------
  iLALU_LD,
  //-----------------
  iSALU_ST,
  iSALU_LL,
  iSALU_SC,
  iSALU_ADDR, // plain add, but it has a store address (break down st addr and data)
  //-----------------
  iCALU_FPMULT,
  iCALU_FPDIV,
  iCALU_FPALU,
  iCALU_MULT,
  iCALU_DIV,
  //-----------------
  iMAX
};

// enum RegType:short {
enum RegType {
  LREG_R0 = 0, // No dependence
  LREG_R1,
  LREG_R2,
  LREG_R3,
  LREG_R4,
  LREG_R5,
  LREG_R6,
  LREG_R7,
  LREG_R8,
  LREG_R9,
  LREG_R10,
  LREG_R11,
  LREG_R12,
  LREG_R13,
  LREG_R14,
  LREG_R15,
  LREG_R16,
  LREG_R17,
  LREG_R18,
  LREG_R19,
  LREG_R20,
  LREG_R21,
  LREG_R22,
  LREG_R23,
  LREG_R24,
  LREG_R25,
  LREG_R26,
  LREG_R27,
  LREG_R28,
  LREG_R29,
  LREG_R30,
  LREG_R31,

  LREG_FP0, // FP Boundary
  LREG_FP1,
  LREG_FP2,
  LREG_FP3,
  LREG_FP4,
  LREG_FP5,
  LREG_FP6,
  LREG_FP7,
  LREG_FP8,
  LREG_FP9,
  LREG_FP10,
  LREG_FP11,
  LREG_FP12,
  LREG_FP13,
  LREG_FP14,
  LREG_FP15,
  LREG_FP16,
  LREG_FP17,
  LREG_FP18,
  LREG_FP19,
  LREG_FP20,
  LREG_FP21,
  LREG_FP22,
  LREG_FP23,
  LREG_FP24,
  LREG_FP25,
  LREG_FP26,
  LREG_FP27,
  LREG_FP28,
  LREG_FP29,
  LREG_FP30,
  LREG_FP31,

  LREG_VECTOR0, // FP Boundary
  LREG_VECTOR1,
  LREG_VECTOR2,
  LREG_VECTOR3,
  LREG_VECTOR4,
  LREG_VECTOR5,
  LREG_VECTOR6,
  LREG_VECTOR7,
  LREG_VECTOR8,
  LREG_VECTOR9,
  LREG_VECTOR10,
  LREG_VECTOR11,
  LREG_VECTOR12,
  LREG_VECTOR13,
  LREG_VECTOR14,
  LREG_VECTOR15,
  LREG_VECTOR16,
  LREG_VECTOR17,
  LREG_VECTOR18,
  LREG_VECTOR19,
  LREG_VECTOR20,
  LREG_VECTOR21,
  LREG_VECTOR22,
  LREG_VECTOR23,
  LREG_VECTOR24,
  LREG_VECTOR25,
  LREG_VECTOR26,
  LREG_VECTOR27,
  LREG_VECTOR28,
  LREG_VECTOR29,
  LREG_VECTOR30,
  LREG_VECTOR31,

  LREG_RND = 64 + 32, // FP Rounding Register

  // Begin SPARC/ARM names
  LREG_ARCH0 = 65 + 32,
  LREG_ARCH1,
  LREG_ARCH2,
  LREG_ARCH3,
  LREG_ARCH4,
  LREG_ARCH5,
  LREG_ARCH6,
  LREG_ARCH7,
  LREG_ARCH8,
  LREG_ARCH9,
  LREG_ARCH10,

  // General names
  LREG_TMP1 = 76 + 32,
  LREG_TMP2,
  LREG_TMP3,
  LREG_TMP4,
  LREG_TMP5,
  LREG_TMP6,
  LREG_TMP7,
  LREG_TMP8,
  LREG_SYSMEM, // syscall memory
  LREG_TTYPE,  // Translation type

  LREG_SCLAST = 86 + 32,
  // This is the end of the RAT for SCOORE

  LREG_INVALID,       // For debug reasons, nobody should use this ID
  LREG_InvalidOutput, // To optimize the RAT code, nobody can read this one, but they can write
  LREG_MAX
};

// enum ClusterType { AUnit = 0, BUnit, CUnit, LUnit, SUnit };
enum TranslationType { ARM = 0, THUMB, THUMB32, SPARC32 };

// Common alias
#define LREG_ZERO LREG_R0
#define LREG_NoDependence LREG_R0
#define NoDependence LREG_R0

// SPARC Mappings
#define LREG_PSR LREG_ARCH0
#define LREG_ICC LREG_ARCH1
#define LREG_CWP LREG_ARCH2
#define LREG_Y LREG_ARCH3
#define LREG_TBR LREG_ARCH4
#define LREG_WIM LREG_ARCH5
#define LREG_FSR LREG_ARCH6
#define LREG_FCC LREG_ARCH7
#define LREG_CEXC LREG_ARCH8
#define LREG_FRS1 LREG_FRN
#define LREG_FRS2 LREG_FRS

// ARM Mappings
#define LREG_CPSR LREG_ARCH0
#define LREG_GE_FLAG LREG_ARCH3
#define LREG_Q_FLAG LREG_ARCH4
#define LREG_PC LREG_R16
#define LREG_LINK LREG_R15
#define LREG_SP LREG_R14
#define LREG_IP LREG_R13

#endif
