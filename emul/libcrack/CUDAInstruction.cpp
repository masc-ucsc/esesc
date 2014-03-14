// Contributed by   Alamelu Sankaranarayanan
//                  Jose Renau
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

#ifdef ENABLE_CUDA

#include <stdio.h>
#include <stdlib.h>

#include "nanassert.h"
#include "CUDAInstruction.h"

// This function uses non-native bit order
#define GET_FIELD(X, FROM, TO) ((X) >> (31 - (TO)) & ((1 << ((TO) - (FROM) + 1)) - 1))

// This function uses the order in the manuals, i.e. bit 0 is 2^0
#define GET_FIELD_SP(X, FROM, TO) GET_FIELD(X, 31 - (TO), 31 - (FROM))

#define IS_IMM (insn & (1<<13))
/*
static uint32_t max_dfl_reg32 = 0;
static uint32_t max_dfl_reg64 = 0;
static uint32_t max_dfl_regFP = 0;
static uint32_t max_sm_reg32  = 0;
static uint32_t max_sm_reg64  = 0;
static uint32_t max_sm_regFP  = 0;
static uint32_t max_sm_addr   = 0;
static uint32_t max_tracesize = 0;
*/



void esesc_disas_cuda_inst(RAWDInst *rinst, uint64_t addr)
{
  uint32_t pe_id   = 0;
  uint32_t warp_id = 0;
  uint32_t peid_warpid = 0;

  
  //MSG (" ---------> ADDR is 0x%016llX",addr);
  peid_warpid = (addr >> 32);
  warp_id     = (peid_warpid & 0x0000FFFF);
  //  I(warp_id <= 24*32);
  //MSG (" ---------> WARPID is 0x%04X",warp_id);
  pe_id       = ((peid_warpid >> 16) & 0x0000FFFF);
  //MSG (" ---------> PE_ID is 0x%04X",pe_id);

  if (((rinst->getInsn() == 0xFFFF) || (rinst->getInsn() == 0xFFFB))){

    InstOpcode decode0_opcode;
    RegType    decode0_src1;  
    RegType    decode0_src2;  
    RegType    decode0_dst1;  

    if (rinst->getInsn() == 0xFFFF){
      decode0_opcode    = iLALU_LD;
      decode0_src1      = LREG_NoDependence;
      decode0_src2      = LREG_NoDependence;
      decode0_dst1      = LREG_TMP1;
    } else {
      decode0_opcode    = iSALU_ST;
      decode0_src1      = LREG_TMP1;
      decode0_src2      = LREG_NoDependence;
      decode0_dst1      = LREG_NoDependence;
    }

    RegType    decode0_dst2      = LREG_InvalidOutput;
    bool       decode0_jumpLabel = false;

    rinst->setMemaccesstype(GlobalMem); //Global memory 
    Instruction *inst = rinst->getNewInst();
    inst->set(decode0_opcode, decode0_src1, decode0_src2, decode0_dst1, decode0_dst2, false);

  } else {

    CUDAKernel* kernel = kernels[current_CUDA_kernel];
    CudaInst *cinst = kernel->getInst(rinst->getInsn());
    //RAWInstType insn = (uint32_t)rinst->getInsn();
    InstOpcode decode0_opcode    = cinst->opcode;

    RegType    decode0_src1; 
    if (cinst->src1 != LREG_NoDependence){
      decode0_src1      = RegType(warp_id*LREG_MAX + pe_id*LREG_MAX + cinst->src1 );
    } else {
      decode0_src1      = LREG_NoDependence; 
    }

    RegType    decode0_src2;
    if (cinst->src2 != LREG_NoDependence){
      decode0_src2      = RegType(warp_id*LREG_MAX + pe_id*LREG_MAX + cinst->src2 );
    } else {
      decode0_src2      = LREG_NoDependence; 
    }

    RegType    decode0_dst1; 
    if(cinst->dest1 != LREG_InvalidOutput){
      decode0_dst1      = RegType(warp_id*LREG_MAX + pe_id*LREG_MAX + cinst->dest1);
    } else {
      decode0_dst1      = LREG_InvalidOutput;
    }

    RegType    decode0_dst2      = LREG_InvalidOutput; 
    bool       decode0_jumpLabel = cinst->jumpLabel;// CUDA 3.1 supports jump register (CHECK)
    rinst->setMemaccesstype(cinst->memaccesstype); 

    // FIXME: fix the following code , remove jumpLabel from all over, use R vs L
    if (!decode0_jumpLabel) {
      switch(decode0_opcode) {
        case iBALU_LBRANCH: decode0_jumpLabel = iBALU_RBRANCH; break;
        case iBALU_LJUMP  : decode0_jumpLabel = iBALU_RJUMP;   break;
        case iBALU_LCALL  : decode0_jumpLabel = iBALU_RCALL;   break;
        default: ;
      }
    }

    //rinst->clearInst();
    Instruction *inst = rinst->getNewInst();
    //inst->set(decode0_opcode, decode0_src1, decode0_src2, decode0_dst1, decode0_dst2);
    inst->set(decode0_opcode, decode0_src1, decode0_src2, decode0_dst1, decode0_dst2, false);
    //inst->dump("esesc_disas_cuda->");

  }
}


void crack_cuda(uint32_t pc, uint32_t insn) {

  MSG("PC=%x %x", pc, insn);

}
#endif
