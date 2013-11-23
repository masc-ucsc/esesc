// Contributed by Jose Renau
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

#ifndef CUDAINSTRUCTION_H
#define CUDAINSTRUCTION_H

#include <map>
#include "RAWDInst.h"

#include <string>
using namespace std;

class CudaInst {
  public:
    InstOpcode opcode;
    AddrType pc;
    int      opkind;
    bool     jumpLabel;
    CUDAMemType    memaccesstype;
    /*
       memaccesstype = 6 --> Texture memory
       memaccesstype = 5 --> Constant memory
       memaccesstype = 4 --> Shared memory
       memaccesstype = 3 --> Local memory
       memaccesstype = 2 --> Param memory
       memaccesstype = 1 --> Global memory
       memaccesstype = 0 --> Register
     */

    RegType  src1;
    RegType  src2;
    RegType  src3; //No Support for 3 registers in Instruction/DInst/RawDInst anyway
    RegType  dest1;

    void setOpcode(std::string opstring){

      opkind = 0;
      if (opstring == "iOpInvalid"){
        opcode = iOpInvalid;
      } else if (opstring == "iRALU"){
        opcode = iRALU;
      } else if (opstring == "iAALU"){
        opcode = iAALU;
      } else if (opstring == "iBALU_BRANCH") { // FIXME? Do we ever branch/jump to a register?
        opcode = iBALU_LBRANCH;
        opkind = 3;
      } else if (opstring == "iBALU_JUMP") {
        opcode = iBALU_LJUMP;
        opkind = 3;
      } else if (opstring == "iBALU_CALL") {
        opcode = iBALU_LCALL;
        opkind = 3;
      } else if (opstring == "iBALU_RET") {
        opcode = iBALU_RET;
        opkind = 3;
      }
      else if (opstring == "iLALU_LD") {
        opcode = iLALU_LD;
        opkind = 1;
      }
      else if (opstring == "iSALU_ST") {
        opcode = iSALU_ST;
        opkind = 2;
      } else if (opstring == "iSALU_LL") {
        opcode = iSALU_LL;
        opkind = 2;
      } else if (opstring == "iSALU_SC") {
        opcode = iSALU_SC;
        opkind = 2;
      } else if (opstring == "iSALU_ADDR") {
        opcode = iSALU_ADDR;
        opkind = 2;
      } else if (opstring == "iSALU_ST or iSALU_ADDR") {
        opcode = iSALU_ST;
        opkind = 2;
      }
      else if (opstring == "iCALU_FPMULT") {
        opcode = iCALU_FPMULT;
      } else if (opstring == "iCALU_FPDIV") {
        opcode = iCALU_FPDIV;
      } else if (opstring == "iCALU_FPALU") {
        opcode = iCALU_FPALU;
      } else if (opstring == "iCALU_MULT") {
        opcode = iCALU_MULT;
      } else if (opstring == "iCALU_DIV") {
        opcode = iCALU_DIV;
      } else if (opstring == "iMAX") {
        opcode = iMAX;
      } else {
        MSG("!!!%s!!!",opstring.c_str());
        exit(-1);
        opcode = iOpInvalid;
      }
    }
};

class BB {
  public:
    CudaInst *insts;
    int       number_of_insts;
    string    label;
    int       id;
    bool      barrier;
};

typedef uint32_t GMEM;
typedef uint32_t SMEM;
typedef class CMEMType{
  public:
    uint32_t bank;
    uint32_t size;
} CMEM;

class kernelstats {
  public:
  GMEM regs_per_thread;
  SMEM shmem_per_kernel;
  vector <CMEM> cmem;
};

class CUDAKernel {
  public:
    
    static uint32_t max_dfl_reg32;
    static uint32_t max_dfl_reg64;
    static uint32_t max_dfl_regFP;
    static uint32_t max_dfl_regFP64;

    static uint32_t max_sm_reg32;
    static uint32_t max_sm_reg64;
    static uint32_t max_sm_regFP;
    static uint32_t max_sm_regFP64;
    static uint32_t max_sm_addr;
    static uint32_t max_tracesize;

    kernelstats sm20;
    kernelstats sm10;

    BB *bb;
    string kernel_name;
    uint16_t numBBs;

    uint32_t dfl_reg32;
    uint32_t dfl_reg64;
    uint32_t dfl_regFP;
    uint32_t dfl_regFP64;

    uint32_t sm_reg32;
    uint32_t sm_reg64;
    uint32_t sm_regFP;
    uint32_t sm_regFP64;

    uint32_t sm_addr;
    uint32_t tracesize;
    uint32_t predicates;

    CUDAKernel(){
      sm20.regs_per_thread = 0;
      sm20.shmem_per_kernel = 0;
      sm20.cmem.clear();

      sm10.regs_per_thread = 0;
      sm10.shmem_per_kernel = 0;
      sm10.cmem.clear();

    }



    CudaInst *getInst(uint16_t bbid, uint16_t instid) const {
      if (bbid == 0){
        MSG("ERROR-----> Askign for data from bbid 0");
      }

      I(bb[bbid].number_of_insts > instid);
      if(bb[bbid].number_of_insts <= instid){
        MSG("Error in BBID[%d] (ID=%d) (size %d) instruction [%d]",bbid,bb[bbid].id,bb[bbid].number_of_insts,instid);
        exit(-1);
      }
      return &bb[bbid].insts[instid];
    }

    bool isDummyBB(uint16_t bbid){
      if ((bbid == 0) ||(bb[bbid].number_of_insts > 0))
        return false;
      return true;
    }

    bool isBarrierBlock(uint16_t bbid){
      //return false; 
      return bb[bbid].barrier; 
    }


    uint16_t getLastBB(){
      return numBBs;
    }

    CudaInst *getInst(uint32_t packedid) const {
      return getInst(packedid>>16,packedid&0xFFFF);
    }

    static uint32_t packID(uint16_t bbid, uint16_t instid) {
      if (bbid == 0){
        MSG("Invalid BBID");
      }
      uint32_t id = bbid;
      return id<<16 | instid;
    }

    static uint64_t pack_block_ID(uint64_t addr, uint32_t blockid){
      //The top 32 bits of addr are 0, we are going to store the blockid there. This blockid is the native block id
      uint64_t tempaddr = blockid;
      return tempaddr<<32|addr;
    }

};


//extern class CUDAKernel *kernel;
extern std::map<string, uint32_t> kernelsId;
extern std::map<string, class CUDAKernel*>kernels;
typedef std::map<string, class CUDAKernel*> CUDAKernelMapType;

extern string current_CUDA_kernel;


//void esesc_disas_cuda_inst(RAWDInst *rinst);
void esesc_disas_cuda_inst(RAWDInst *rinst, uint64_t warpid_peid);

#endif
#endif
