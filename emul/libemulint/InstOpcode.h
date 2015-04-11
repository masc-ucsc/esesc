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
#include <sys/types.h>
#include <stdint.h>

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

enum Scopcode {
  OP_nop

  ,OP_S64_ADD
  ,OP_S32_ADD
  ,OP_S16_ADD
  ,OP_S08_ADD
  ,OP_U64_ADD
  ,OP_U32_ADD
  ,OP_U16_ADD
  ,OP_U08_ADD
  ,OP_S64_ADDXX
  ,OP_S32_ADDXX
  ,OP_S64_SUB
  ,OP_S32_SUB
  ,OP_S16_SUB
  ,OP_S08_SUB
  ,OP_U64_SUB
  ,OP_U32_SUB
  ,OP_U16_SUB
  ,OP_U08_SUB
  ,OP_S64_SLL
  ,OP_S32_SLL
  ,OP_S16_SLL
  ,OP_S08_SLL
  ,OP_S64_SRL
  ,OP_S32_SRL
  ,OP_S16_SRL
  ,OP_S08_SRL
  ,OP_S64_SRA
  ,OP_S32_SRA
  ,OP_S16_SRA
  ,OP_S08_SRA
  ,OP_S64_ROTR
  ,OP_S32_ROTR
  ,OP_S16_ROTR
  ,OP_S08_ROTR
  ,OP_S64_ROTRXX
  ,OP_S32_ROTRXX
  ,OP_S64_ANDN
  ,OP_S64_AND
  ,OP_S64_OR
  ,OP_S64_XOR
  ,OP_S64_ORN
  ,OP_S64_XNOR
  ,OP_S64_SADD
  ,OP_S32_SADD
  ,OP_S16_SADD
  ,OP_S08_SADD
  ,OP_U64_SADD
  ,OP_U32_SADD
  ,OP_U16_SADD
  ,OP_U08_SADD
  ,OP_S64_SSUB
  ,OP_S32_SSUB
  ,OP_S16_SSUB
  ,OP_S08_SSUB
  ,OP_U64_SSUB
  ,OP_U32_SSUB
  ,OP_U16_SSUB
  ,OP_U08_SSUB
  ,OP_S64_COPY
  ,OP_S64_FCC2ICC
  ,OP_S64_JOINPSR
  //NOTUSED
  ,OP_S64_COPYICC
  ,OP_S64_GETICC
  ,OP_S64_PUTICC
  ,OP_S64_NOTICC
  ,OP_S64_ANDICC
  ,OP_S64_ORICC  
  ,OP_S64_XORICC
  //NOTUSED
  //NOTUSED
  //NOTUSED
  ,OP_S64_COPYPCL
  ,OP_S64_COPYPCH
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
 
  ///////////////////////////////////////////////////////////////////////
  //CONDITIONAL MOVES
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_CMOV_A //not passed
  ,OP_U64_CMOV_E
  ,OP_U64_CMOV_NE
  ,OP_U64_CMOV_CS
  ,OP_U64_CMOV_CC
  ,OP_U64_CMOV_NEG
  ,OP_U64_CMOV_POS
  ,OP_U64_CMOV_VS
  ,OP_U64_CMOV_VC
  ,OP_U64_CMOV_C_AND_NOTZ
  ,OP_U64_CMOV_NOTC_OR_Z
  ,OP_U64_CMOV_GE
  ,OP_U64_CMOV_L
  ,OP_U64_CMOV_G
  ,OP_U64_CMOV_LE
  ,OP_U64_CMOV_LGU
  ,OP_U64_CMOV_EGU
  ,OP_U64_CMOV_LEG
  ,OP_U64_CMOV_UG
  ,OP_U64_CMOV_EL
  ,OP_U64_CMOV_EG
  ,OP_U64_CMOV_LU
  ,OP_U64_CMOV_LUE
  ,OP_U64_CMOV_LG
  ,OP_U64_CMOV_EU
  //NOTUSED
  //NOTUSED
  //NOTUSED

  ///////////////////////////////////////////////////////////////////////
  //BRANCH ALU (BALU)
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_JMP_REG
  ,OP_U64_JMP_IMM

  ///////////////////////////////////////////////////////////////////////
  //BRANCH ALU (BALU) BRANCH REGISTER 
  ////////////////////////////////////////////////////////////////////////
  //OP_U64_RBA //not passed
  ,OP_U64_RBE
  ,OP_U64_RBNE
  ,OP_U64_RBCS
  ,OP_U64_RBCC
  ,OP_U64_RBNEG
  ,OP_U64_RBPOS
  ,OP_U64_RBVS
  ,OP_U64_RBVC
  ,OP_U64_RBC_OR_Z_NOT
  ,OP_U64_RBC_OR_Z
  ,OP_U64_RBC_AND_NOTZ
  ,OP_U64_RBNOTC_OR_Z
  ,OP_U64_RBGE
  ,OP_U64_RBL
  ,OP_U64_RBG
  ,OP_U64_RBLE
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED

  ///////////////////////////////////////////////////////////////////////
  //BRANCH ALU (BALU) BRANCH IMMEDIATE
  ////////////////////////////////////////////////////////////////////////
  //OP_U64_LBA //not passed
  ,OP_U64_LBE
  ,OP_U64_LBNE
  ,OP_U64_LBCS
  ,OP_U64_LBCC
  ,OP_U64_LBNEG
  ,OP_U64_LBPOS
  ,OP_U64_LBVS
  ,OP_U64_LBVC
  ,OP_U64_LBC_OR_Z_NOT
  ,OP_U64_LBC_OR_Z
  ,OP_U64_LBC_AND_NOTZ
  ,OP_U64_LBNOTC_OR_Z
  ,OP_U64_LBGE
  ,OP_U64_LBL
  ,OP_U64_LBG
  ,OP_U64_LBLE
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  
  ///////////////////////////////////////////////////////////////////////
  //BRANCH ALU (BALU) FLOATING POINT BRANCH BASED ON FCC
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_FBA //not passed
  ,OP_U64_FBUG
  ,OP_U64_FBLU
  ,OP_U64_FBLG
  ,OP_U64_FBLGU
  ,OP_U64_FBEU
  ,OP_U64_FBEG
  ,OP_U64_FBEGU
  ,OP_U64_FBEL
  ,OP_U64_FBLUE
  ,OP_U64_FBLEG
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  //NOTUSED
  
  ///////////////////////////////////////////////////////////////////////
  //LOAD/STORE ALU (MEMORY ALU)
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_LD_L //Little Endian
  ,OP_S32_LD_L //Little Endian
  ,OP_S16_LD_L //Little Endian
  ,OP_S08_LD
  ,OP_U32_LD_L //Little Endian
  ,OP_U16_LD_L //Little Endian
  ,OP_U08_LD
 
  ,OP_U64_LD_B //Big Endian
  ,OP_S32_LD_B //Big Endian
  ,OP_S16_LD_B //Big Endian
  ,OP_U32_LD_B //Big Endian
  ,OP_U16_LD_B //Big Endian
  ///////////////////////////////////////////////////////////////////////
  //STORE ALU (SALU)
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_ST_L //Little Endian
  ,OP_U32_ST_L //Little Endian
  ,OP_U16_ST_L //Little Endian

  ,OP_U64_ST_B //Big Endian
  ,OP_U32_ST_B //Big Endian
  ,OP_U16_ST_B //Big Endian

  ,OP_U08_ST
  ,OP_U32_STSC
  
  ///////////////////////////////////////////////////////////////////////
  //STORE CALCULATE ADDRESS
  ///////////////////////////////////////////////////////////////////////
  ,OP_U64_CADD_E
  ,OP_U64_CADD_NE
  ,OP_U64_CADD_CS
  ,OP_U64_CADD_CC
  ,OP_U64_CADD_NEG
  ,OP_U64_CADD_POS
  ,OP_U64_CADD_VS
  ,OP_U64_CADD_VC
  ,OP_U64_CADD_C_OR_Z_NOT
  ,OP_U64_CADD_C_OR_Z
  ,OP_U64_CADD_C_AND_NOTZ
  ,OP_U64_CADD_NOTC_OR_Z
  ,OP_U64_CADD_GE
  ,OP_U64_CADD_L
  ,OP_U64_CADD_G
  ,OP_U64_CADD_LE
  
  ,OP_U64_CSUB_E
  ,OP_U64_CSUB_NE
  ,OP_U64_CSUB_CS
  ,OP_U64_CSUB_CC
  ,OP_U64_CSUB_NEG
  ,OP_U64_CSUB_POS
  ,OP_U64_CSUB_VS
  ,OP_U64_CSUB_VC
  ,OP_U64_CSUB_C_OR_Z_NOT
  ,OP_U64_CSUB_C_OR_Z
  ,OP_U64_CSUB_C_AND_NOTZ
  ,OP_U64_CSUB_NOTC_OR_Z
  ,OP_U64_CSUB_GE
  ,OP_U64_CSUB_L
  ,OP_U64_CSUB_G
  ,OP_U64_CSUB_LE
  
  ///////////////////////////////////////////////////////////////////////
  //ATOMICITY Instructions
  ////////////////////////////////////////////////////////////////////////
  ,OP_U64_MARK_EX
  ,OP_U64_CHECK_EX
  ,OP_U64_ST_CHECK_EX
  ,OP_U64_CLR_ADDR //NOTE: If addr==0, then clear everything
  ,OP_U64_ST_COND
  ,OP_U32_ST_COND
  ,OP_U16_ST_COND
  ,OP_U08_ST_COND

  ///////////////////////////////////////////////////////////////////////
  //FPU ALU (CALU)
  ////////////////////////////////////////////////////////////////////////
  ,OP_C_JOINFSR
  
  ,OP_C_FSTOI
  ,OP_C_FSTOD
  ,OP_C_FMOVS
  ,OP_C_FNEGS
  ,OP_C_FABSS
  ,OP_C_FSQRTS
  ,OP_C_FADDS
  ,OP_C_FSUBS
  ,OP_C_FMULS
  ,OP_C_FDIVS
  ,OP_C_FSMULD
  ,OP_C_FCMPS
  ,OP_C_FCMPES

  ,OP_C_FDTOI
  ,OP_C_FDTOS
  ,OP_C_FMOVD
  ,OP_C_FNEGD
  ,OP_C_FABSD
  
  ,OP_C_FSQRTD
  ,OP_C_FADDD
  ,OP_C_FSUBD
  ,OP_C_FMULD
  ,OP_C_FDIVD
  ,OP_C_FCMPD
  ,OP_C_FCMPED

  ,OP_C_UMUL
  ,OP_C_UDIV
  ,OP_C_UDIVCC
  ,OP_C_UMULCC

  ,OP_C_FITOS
  ,OP_C_FITOD

  ,OP_C_CONCAT
  ,OP_C_MULSCC

  ,OP_C_SMUL
  ,OP_C_SDIV
  ,OP_C_SDIVCC
  ,OP_C_SMULCC
 
// FIXME:IanLee1521: Instr's in this comment block don't match to anything in .../scips/rtl/scoore_isa.v 
  ,OP_iRALU_move
};

//enum RegType:short {
enum RegType {
  LREG_R0=0, // No dependence
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

  LREG_RND =64, // FP Rounding Register

  // Begin SPARC/ARM names
  LREG_ARCH0=65, 
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
  LREG_TMP1=76,
  LREG_TMP2,
  LREG_TMP3,
  LREG_TMP4,
  LREG_TMP5,
  LREG_TMP6,
  LREG_TMP7,
  LREG_TMP8,
  LREG_SYSMEM,   //syscall memory 
  LREG_TTYPE,    //Translation type

  LREG_SCLAST=86,
  // This is the end of the RAT for SCOORE

  LREG_INVALID, // For debug reasons, nobody should use this ID
  LREG_InvalidOutput, // To optimize the RAT code, nobody can read this one, but they can write
  LREG_MAX
};

//enum ClusterType { AUnit = 0, BUnit, CUnit, LUnit, SUnit };
enum TranslationType { ARM = 0, THUMB, THUMB32, SPARC32};

// Common alias
#define LREG_ZERO  LREG_R0
#define LREG_NoDependence LREG_R0
#define NoDependence LREG_R0

// SPARC Mappings
#define LREG_PSR         LREG_ARCH0
#define LREG_ICC         LREG_ARCH1
#define LREG_CWP         LREG_ARCH2
#define LREG_Y           LREG_ARCH3
#define LREG_TBR         LREG_ARCH4
#define LREG_WIM         LREG_ARCH5
#define LREG_FSR         LREG_ARCH6
#define LREG_FCC         LREG_ARCH7
#define LREG_CEXC        LREG_ARCH8
#define LREG_FRS1        LREG_FRN
#define LREG_FRS2        LREG_FRS

// ARM Mappings
#define LREG_CPSR        LREG_ARCH0
#define LREG_GE_FLAG     LREG_ARCH3
#define LREG_Q_FLAG      LREG_ARCH4
#define LREG_PC          LREG_R16
#define LREG_LINK        LREG_R15 
#define LREG_SP          LREG_R14
#define LREG_IP          LREG_R13


#endif
