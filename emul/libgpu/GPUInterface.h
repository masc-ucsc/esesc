/*
ESESC: Super ESCalar simulator
Copyright (C) 2006 University California, Santa Cruz.

Contributed by  Alamelu Sankaranarayanan
Jose Renau
Gabriel Southern

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

#ifndef GPU_INTERFACE_H
#define GPU_INTERFACE_H

#include <stdint.h>
#include "nanassert.h"
#include <string>
#include "Instruction.h"
#include "GPUThreadManager.h"

class EmuSampler;

class GPUThreadManager;

struct ThreadBlockStatus;

struct AddrRange {
  uint64_t dev_start;
  uint64_t dev_end;
  size_t size;
  uint64_t host_start;
  uint64_t host_end;
};

// These are all variables that will control how the traces
// are fed to each tsfifo.

extern uint64_t numThreads;
extern uint64_t blockThreads;

//extern WarpStyle warpStyle;
//extern InterLeaveStyle interleaveStyle;

extern uint32_t traceSize;
extern uint64_t blockSize;
extern uint32_t warpsize;
extern uint32_t halfwarpsize;
extern uint32_t thread_granularity;
extern uint32_t shmem_granularity;
extern uint32_t max_shmem_sm;
extern uint32_t max_regs_sm;
extern uint32_t max_warps_sm;
extern uint32_t max_blocks_sm;

typedef enum Branch_divergence_type {
  serial   = 0,
  post_dom = 1,
  sbi      = 2,
  sbi_swi  = 3,
  original = 4
} div_type;
extern div_type branch_div_mech;

/*
extern uint64_t oldqemuid;
extern uint64_t newqemuid;
extern uint32_t gpuid;
*/

extern uint8_t cuda_execution_complete;
extern uint8_t cuda_execution_started;

extern EmuSampler *gsampler;
extern GPUThreadManager *gpuTM;
extern ThreadBlockStatus *block;
extern std::string current_CUDA_kernel;

extern uint32_t reexecute;

extern "C" {

  void GPUReader_finish (uint32_t fid);
  void GPUReader_declareLaunch ();

  uint32_t GPUReader_decode_trace (uint32_t * h_trace, uint32_t * qemuid, void *env);
  void GPUReader_init_trace (uint32_t * h_trace);

  uint64_t GPUReader_getNumThreads ();
  uint64_t GPUReader_getBlockThreads ();
  //uint32_t GPUReader_setNumThreads (uint64_t binnumthreads);
  uint32_t GPUReader_setNumThreads (uint64_t binnumthreads, uint64_t binblockthreads);
  uint32_t GPUReader_getTracesize ();

  void esesc_set_rabbit_gpu (void );
  void esesc_set_warmup_gpu (void );
  void esesc_set_timing_gpu (void );

  void GPUReader_setCurrentKernel (const char *local_kernel_name);

  void GPUReader_CUDA_exec_done();
  //void GPUReader_getKernelParameters(uint32_t* DFL_REG32, uint32_t* DFL_REG64, uint32_t* DFL_REGFP, uint32_t* SM_REG32, uint32_t* SM_REG64, uint32_t* SM_REGFP, uint32_t* SM_ADDR , uint32_t* TRACESIZE);
  void GPUReader_getKernelParameters (uint32_t * DFL_REG32,
                                      uint32_t * DFL_REG64,
                                      uint32_t * DFL_REGFP,
                                      uint32_t * DFL_REGFP64,
                                      uint32_t * SM_REG32,
                                      uint32_t * SM_REG64,
                                      uint32_t * SM_REGFP,
                                      uint32_t * SM_REGFP64,
                                      uint32_t * SM_ADDR, uint32_t * TRACESIZE);


  void GPUReader_mallocAddress(uint32_t dev_addr, uint32_t size, uint32_t* cpufid);
  void GPUReader_mapcudaMemcpy(uint32_t addr0, uint32_t addr1, uint32_t size, uint32_t kind, void *env, uint32_t* cpufid);
  uint64_t GPUReader_translate_d2h(uint64_t addr_ptr);
  uint64_t GPUReader_translate_shared(uint64_t addr_ptr, uint32_t blockid, uint32_t warpid);
	uint64_t GPUReader_encode_rawaddr(uint64_t addr_ptr, uint32_t blockid, uint32_t warpid, uint32_t pe_id, bool sharedAddr);
}

#endif
