// Contributed by  Alamelu Sankaranarayanan
//                 Jose Renau
//                 Gabriel Southern
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
}

#endif
