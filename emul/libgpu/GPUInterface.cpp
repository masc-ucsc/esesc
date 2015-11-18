/*
ESESC: Super ESCalar simulator
Copyright (C) 2005 University California, Santa Cruz.

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
#define DO_MEMCPY 1
#define CPU_DOES_MEMCPY 1
#define SPECRATE_SYNC 0

#include <pthread.h>
#include "GPUInterface.h"

#include "InstOpcode.h"
#include "Instruction.h"
#include "Snippets.h"
#include "GPUReader.h"
#include "EmuSampler.h"
#include "CUDAInstruction.h"
#include "Report.h"
#include "GStats.h"

timeval endTime, stTime;
GStatsCntr msecGpu("OSSim:msecs_gpu");
GStatsCntr totalThreadCount("PTXStats:totalThreadCount");
GStatsCntr totalTimingThreadCount("PTXStats:totalTimingThreadCount");
GStatsCntr memcpy2device("PTXStats:memcpy2device");
GStatsCntr memcpy2host("PTXStats:memcpy2host");
GStatsCntr cudamalloc("PTXStats:cudamalloc");
GStatsHist avgMemcpy("PTXStats:avgMemcpy");


extern bool cuda_go_ahead;
CUDAKernel *kernel       = NULL;
EmuSampler *gsampler     = NULL;
//extern EmuSampler *qsampler;
GPUThreadManager *gpuTM  = NULL;
ThreadBlockStatus *block = NULL;

uint8_t cuda_execution_complete = 0;
uint8_t cuda_execution_started  = 0;
uint64_t oldqemuid          = 0;
uint64_t newqemuid          = 0;
uint64_t  GPUFlowsAlive     = 0;
bool* GPUFlowStatus         = 0;
bool* GPUSMDoneStatus       = 0;

uint16_t SM_all_timing_threads_complete = 0;

uint64_t numThreads         = 0;
uint64_t blockThreads       = 0;
int64_t maxSamplingThreads  = 0;
uint32_t traceSize          = 0;

uint64_t blockSize          = 0;
uint32_t warpsize           = 0;
uint32_t halfwarpsize       = 0;
uint32_t thread_granularity = 0;
uint32_t shmem_granularity  = 0;
uint32_t max_shmem_sm       = 0;
uint32_t max_regs_sm        = 0;
uint32_t max_warps_sm       = 0;
uint32_t max_blocks_sm      = 0;
bool unifiedCPUGPUmem       = false;
extern bool MIMDmode;
div_type branch_div_mech    = serial;
uint64_t loopcounter = 0;
uint64_t relaunchcounter = 1;
uint64_t memref_interval = 0;

extern EmuSampler *qsamplerlist[];
extern EmuSampler *qsampler;
extern "C" void loadSampler(FlowID fid);

bool* istsfifoBlocked           = NULL;
bool finishkernel               = false;
bool rabbit_threads_executing   = false;
uint32_t gpuid                  = 0;
uint32_t nextSM                 = 0;
bool firstevertime = true;
bool different_kernel = false;

string current_CUDA_kernel = "";
std::map < string, class CUDAKernel * >kernels;
std::map < string, uint32_t > kernelsId;
//std::vector <FlowID> SM_temp_pause;
std::vector <AddrRange> Addrmap;

uint32_t reexecute = 0;


extern "C" void GPUReader_declareLaunch() {
  IS(MSG ("****************************************************************"));
  IS(MSG("Launching Kernel %s", current_CUDA_kernel.c_str()));
  IS(MSG ("****************************************************************"));

  if (gsampler->getThreads2Simulate() > 0){
    if ((int64_t) numThreads < gsampler->getThreads2Simulate()){
      totalTimingThreadCount.add(numThreads);
    } else {
      totalTimingThreadCount.add(gsampler->getThreads2Simulate());
    }
  } else if (gsampler->getThreads2Simulate() < 0){
    totalTimingThreadCount.add(numThreads);
  }

  totalThreadCount.add(numThreads);
}

extern "C" void GPUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, char op, uint64_t icount, void *env) {
  gsampler->queue(insn, pc, addr, fid, op, icount, env);
}

extern "C" uint64_t GPUReader_getNumThreads() {
  /*This function is always called before each kernel
   * Therefore a good place to add initializations
   * like the one for maxSamplingThreads */
  //maxSamplingThreads = gsampler->getThreads2Simulate();
  return numThreads;
}

extern "C" uint64_t GPUReader_getBlockThreads (){
  /*This function is always called before each kernel
   * Therefore a good place to add initializations
   * like the one for maxSamplingThreads */
  return blockSize;
}

extern "C" uint32_t GPUReader_setNumThreads (uint64_t binnumthreads, uint64_t binblockthreads){
  //At this point, kernelname has already been set.

  if ((different_kernel) || (numThreads != binnumthreads) || (blockSize != binblockthreads)){
    numThreads  = binnumthreads;
    blockSize   = binblockthreads;
    gpuTM->reOrganize();
    IS(MSG("Calling Reorganize"));
  } else {
    IS(MSG(" NOT Calling Reorganize"));
  }

  IS(MSG("Numthreads set to %llu blockSize set to %llu, maxthreadstobesampled is %lld",
        numThreads,
        blockSize,
        maxSamplingThreads));
  return 0;
}

extern "C" uint32_t GPUReader_getTracesize() {
  return traceSize;
}

extern "C" void GPUReader_mallocAddress(uint32_t dev_addr, uint32_t size, uint32_t* cpufid)
{/*{{{*/

  cudamalloc.add(size);
  if (unifiedCPUGPUmem){
    MSG("Mapping %d bytes of memory on the GPU starting from address %x ", (int)size, dev_addr);
    AddrRange a;

    a.dev_start = dev_addr;
    a.size  = size;
    a.dev_end   = dev_addr + size - 1;

    a.host_start  = 0;
    a.host_end    = 0;

    Addrmap.push_back(a);
  }
}/*}}}*/

extern "C" void GPUReader_mapcudaMemcpy(uint32_t addr0, uint32_t addr1, uint32_t size, uint32_t kind,void *env, uint32_t* cpufid)
{/*{{{*/
  uint32_t dev_addr  = 0;
  uint32_t host_addr = 0;
  //avgMemcpy.sample(size);

  if (firstevertime == true){

#if SPECRATE_SYNC

    //Pause the CPU
    qsamplerlist[*cpufid]->pauseThread(*cpufid);
    oldqemuid = *cpufid;
    gpuTM->putinfile(qsamplerlist[*cpufid]->totalnInst);
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //MSG("BYEEEEEEEEEEEEEEEE!!!!!");
    //exit(-1);

    //Wait till the other threads reach the nInstSkipThreads mark.
    while (cuda_go_ahead == false){
      fprintf(stderr,".");
      pthread_yield();
      sleep(10);
    }

    //Resume the CPU
    newqemuid = qsamplerlist[oldqemuid]->getFid(oldqemuid);
    newqemuid = qsamplerlist[newqemuid]->resumeThread(newqemuid, newqemuid);
    *cpufid = newqemuid;

#endif

    //totalThreadCount.setIgnoreSampler();
    //totalTimingThreadCount.setIgnoreSampler();
    firstevertime = false;
    MSG("\n\n\n\n\n\n\n************************************************************** GO AHEAD ****************************************************************\n\n\n\n\n\n\n");
  }


  if (unifiedCPUGPUmem){
    /***************************************************/
    //  enum cudaMemcpyKind
    //  {
    //      cudaMemcpyHostToHost     = 0, /* *< Host   -> Host   */
    //      cudaMemcpyHostToDevice   = 1, /* *< Host   -> Device */
    //      cudaMemcpyDeviceToHost   = 2, /* *< Device -> Host   */
    //      cudaMemcpyDeviceToDevice = 3  /**< Device -> Device */
    //  };
    /***************************************************/

    if (kind == 1){
      //cudaMemcpyHostToDevice   = 1, /* *< Host   -> Device */
      dev_addr           = addr0;
      host_addr          = addr1;
      memcpy2device.add(size);
    } else if (kind == 2){
      //cudaMemcpyDeviceToHost   = 2, /* *< Device -> Host   */
      dev_addr           = addr1;
      host_addr          = addr0;
      memcpy2host.add(size);
    }

    IS(MSG("Map %d bytes of memory between CPU address %x, and GPU address %x", (int)size, host_addr, dev_addr));

    bool notfound = true;
    for (uint32_t i = 0; i < Addrmap.size(); i++){
      if ((dev_addr >= Addrmap[i].dev_start) && (dev_addr <= Addrmap[i].dev_end)){
        notfound = false;
        Addrmap[i].host_start = host_addr;
        Addrmap[i].host_end   = host_addr+size;
        I(size <= Addrmap[i].size);
        i                     = Addrmap.size()+1;
      }
    }

    if (notfound){
      I(0);
      IS(MSG("ERROR!!!"));
    }
  } else {
#if DO_MEMCPY
#if CPU_DOES_MEMCPY
///////////////////////////////////////////////////////////////////
//    RAWInstType loadinsn  = 0xe4917004; //DO NOT CHANGE
//    RAWInstType storeinsn = 0xe4804004; //DO NOT CHANGE
//    uint32_t    loadpc    = 0xdeaddead; // Dummy PC
//    uint32_t    storepc   = 0xdeaddeb1; // deaddead+4
///////////////////////////////////////////////////////////////////
//    RAWInstType loadinsn1  = 0xe5912000; //DO NOT CHANGE
//    RAWInstType loadinsn2  = 0xe2811010; //DO NOT CHANGE
//
//    RAWInstType storeinsn1 = 0xe5834000; //DO NOT CHANGE
//    RAWInstType storeinsn2 = 0xe2833010; //DO NOT CHANGE
//
//    DataType    data      = 0;
//    uint32_t    loadpc1    = 0xdeaddead; // Dummy PC
//    uint32_t    loadpc2    = 0xdeaddeb1; // Dummy PC
//    uint32_t    storepc1   = 0xdeaddeb5; // deaddead+4
//    uint32_t    storepc2   = 0xdeaddeb9; // deaddead+4
///////////////////////////////////////////////////////////////////


    //op = 4  (alu+br)    fcf4: f5d1f07c        pld     [r1, #124];       0x7c                                                                       │~
    //op = 1  (8ld+9alu)  fcf8: e8b151f8        ldm     r1!, {r3, r4, r5, r6, r7, r8, ip, lr}                                                        │~
    //op = 4  (2alu)      fcfc: e2522020        subs    r2, r2, #32                                                                                  │~
    //op = 2  (8st+9alu)  fd00: e8a051f8        stmia   r0!, {r3, r4, r5, r6, r7, r8, ip, lr}                                                        │~
    //op =3  (1br)       fd04: aafffffa        bge     fcf4 <memcpy+0x44>
    RAWInstType insn1 = 0xf5d1f07c; // DO NOT CHANGE
    RAWInstType insn2 = 0xe8b151f8; // DO NOT CHANGE
    RAWInstType insn3 = 0xe2522020; // DO NOT CHANGE
    RAWInstType insn4 = 0xe8a051f8; // DO NOT CHANGE
    RAWInstType insn5 = 0xaafffffa; // DO NOT CHANGE

    uint32_t    pc1   = 0xf00dfcf4; // DO NOT CHANGE
    uint32_t    pc2   = 0xf00dfcf8; // DO NOT CHANGE
    uint32_t    pc3   = 0xf00dfcfc; // DO NOT CHANGE
    uint32_t    pc4   = 0xf00dfd00; // DO NOT CHANGE
    uint32_t    pc5   = 0xf00dfd04; // DO NOT CHANGE


    char        op1        = 4 | 0xc0; // Thumb32
    char        op2        = 1; // ARM32
    char        op3        = 4; // ARM32
    char        op4        = 2; // ARM32
    char        op5        = 3; // ARM32

    //DataType    data      = 0;

    uint64_t    icount    = 1;
    int32_t bytecount     = size;
    uint32_t datachunk    = 32;          // Bytes fetched at a time

    if (kind == 1){
      //cudaMemcpyHostToDevice   = 1, /* *< Host   -> Device */
      dev_addr           = addr0;
      host_addr          = addr1;
      memcpy2device.add(bytecount);
//      qsamplerlist[*cpufid]->setyesStats(*cpufid);
      do {
        //loadSampler(cpufid);

        qsamplerlist[*cpufid]->queue(insn1,pc1, 0xdeadf00d ,*cpufid,op1,icount,env);
        for(int i=0;i<8;i++) // 8 LD
          qsamplerlist[*cpufid]->queue(insn2,pc2, host_addr + i*4,*cpufid,op2,icount,env);
        qsamplerlist[*cpufid]->queue(insn3,pc3, 0,*cpufid,op3,icount,env);
        for(int i=0;i<8;i++) // 8 ST
          qsamplerlist[*cpufid]->queue(insn4,pc4, dev_addr + i*4,*cpufid,op4,icount,env);
        qsamplerlist[*cpufid]->queue(insn5,pc5, pc1 ,*cpufid,op5,icount,env);

        //op        = 1; //Load from host
        //qsamplerlist[*cpufid]->queue(loadinsn1,loadpc1,host_addr,*cpufid,op,icount,env);
        //qsamplerlist[*cpufid]->queue(loadinsn2,loadpc1,host_addr,*cpufid,op,icount,env);
        //gsampler->queue(insn,pc,addr,data,*cpufid,op,icount,env);

        //op        = 2; // Store to device
        //qsamplerlist[*cpufid]->queue(storeinsn1,storepc1,dev_addr,*cpufid,op,icount,env);
        //qsamplerlist[*cpufid]->queue(storeinsn2,storepc2,dev_addr,*cpufid,op,icount,env);
        //gsampler->queue(insn,pc,addr,data,*cpufid,op,icount,env);
        host_addr += datachunk;
        dev_addr  += datachunk;
        bytecount = bytecount - datachunk;

      } while (bytecount > 0);
   //   qsamplerlist[*cpufid]->setnoStats(*cpufid);
    } else if (kind == 2){
      //cudaMemcpyDeviceToHost   = 2, /* *< Device -> Host   */
      memcpy2host.add(bytecount);
      dev_addr           = addr1;
      host_addr          = addr0;
//      qsamplerlist[*cpufid]->setyesStats(*cpufid);
      do {
        //loadSampler(cpufid);

        qsamplerlist[*cpufid]->queue(insn1,pc1, 0xdeadf00d ,*cpufid,op1,icount,env);
        for(int i=0;i<8;i++) // 8 LD
          qsamplerlist[*cpufid]->queue(insn2,pc2, dev_addr + i*4,*cpufid,op2,icount,env);
        qsamplerlist[*cpufid]->queue(insn3,pc3, 0,*cpufid,op3,icount,env);
        for(int i=0;i<8;i++) // 8 LD
          qsamplerlist[*cpufid]->queue(insn4,pc4, host_addr + i*4,*cpufid,op4,icount,env);
        qsamplerlist[*cpufid]->queue(insn5,pc5, pc1 ,*cpufid,op5,icount,env);

        //op        = 1; // Load from device
        //qsamplerlist[*cpufid]->queue(loadinsn1,loadpc1,dev_addr,*cpufid,op,icount,env);
        //qsamplerlist[*cpufid]->queue(loadinsn2,loadpc2,dev_addr,*cpufid,op,icount,env);
        //gsampler->queue(insn,pc,addr,*cpufid,op,icount,env);

        //op        = 2; //Store to host
        //qsamplerlist[*cpufid]->queue(storeinsn1,storepc1,host_addr,*cpufid,op,icount,env);
        //qsamplerlist[*cpufid]->queue(storeinsn2,storepc2,host_addr,*cpufid,op,icount,env);
        //gsampler->queue(insn,pc,addr,*cpufid,op,icount,env);

        dev_addr += datachunk;
        host_addr += datachunk;
        bytecount = bytecount - datachunk;

      } while (bytecount > 0);
   //   qsamplerlist[*cpufid]->setnoStats(*cpufid);
    }
#else
#if 1
    //IF CPU DOES NOT DO MEMCPY, instead GPU fetched the memory address.

    uint32_t smid        = 0;
    uint32_t glofid      = 0;

    RAWInstType loadinsn = 0xFFFF;
    RAWInstType storeinsn= 0xFFFB;
    uint32_t    loadpc   = 0xf00ddead;
    uint32_t    storepc  = 0xf00ddeb1;

    AddrType    addr     = 0;
    char        op       = 0;
    uint64_t    icount   = 1;

    int32_t bytecount     = size;
    uint32_t datachunk    = 64;          // Bytes fetched at a time, equal to the cache linesize of DL1G

    gsampler->startTiming(glofid);
    if (kind == 1) {
      //cudaMemcpyHostToDevice   = 1, /* *< Host   -> Device */

      memcpy2device.add(bytecount);
      dev_addr           = addr0;
      host_addr          = addr1;


      //Pause the CPU
      oldqemuid = *cpufid;
      qsamplerlist[*cpufid]->pauseThread(*cpufid);

      //Memcpy
      do{
        glofid          = gpuTM->mapLocalID(smid);

        //Load
        gsampler->queue(loadinsn,loadpc,host_addr,glofid,op,1,env);

        if (istsfifoBlocked[glofid]){
          gsampler->resumeThread(glofid);
        } else {
          //Store
          gsampler->queue(storeinsn,storepc,dev_addr,glofid,op,1,env);
          host_addr += datachunk;
          dev_addr  += datachunk;
          bytecount -= datachunk;
        }
        smid++;
        if (smid == gpuTM->ret_numSM()){
          smid = 0;
        }
      } while (bytecount > 0);

      //Resume the CPU
      newqemuid = qsamplerlist[oldqemuid]->getFid(oldqemuid);
      newqemuid = qsamplerlist[newqemuid]->resumeThread(newqemuid, newqemuid);
      *cpufid = newqemuid;

      //Pause the GPUs
      for (smid = 0; ((smid < gpuTM->ret_numSM())); smid++){
        gsampler->pauseThread(glofid);
      }

    } else if (kind == 2){
      //cudaMemcpyDeviceToHost   = 2, /* *< Device -> Host   */

      memcpy2host.add(bytecount);
      dev_addr           = addr1;
      host_addr          = addr0;

      //Pause the CPU
      oldqemuid = *cpufid;
      qsamplerlist[*cpufid]->pauseThread(*cpufid);

      //Memcpy
      do{
        glofid          = gpuTM->mapLocalID(smid);

        //Load
        gsampler->queue(loadinsn,loadpc,host_addr,glofid,op,1,env);
        if (istsfifoBlocked[glofid]){
          gsampler->resumeThread(glofid);
        } else {
          //Store
          gsampler->queue(storeinsn,storepc,dev_addr,glofid,op,1,env);
          host_addr += datachunk;
          dev_addr  += datachunk;
          bytecount -= datachunk;
        }
        smid++;
        if (smid == gpuTM->ret_numSM()){
          smid = 0;
        }
      } while (bytecount > 0);

      //Resume the CPU
      newqemuid = qsamplerlist[oldqemuid]->getFid(oldqemuid);
      newqemuid = qsamplerlist[newqemuid]->resumeThread(newqemuid, newqemuid);
      *cpufid = newqemuid;

      //Pause the GPUs
      for (smid = 0; ((smid < gpuTM->ret_numSM())); smid++){
        gsampler->pauseThread(glofid);
      }

    }

    gsampler->stop();
#endif
#endif
#endif
  }
}/*}}}*/


extern "C" void GPUReader_setCurrentKernel(const char *local_kernel_name)
{/*{{{*/
  CUDAKernelMapType::iterator iter = kernels.begin();
  uint32_t size = 0;
  bool found = false;

  //fprintf(stderr,"\nLooking for... %s\n",local_kernel_name);
  do {
    string tempstr = iter->second->kernel_name;
    //fprintf(stderr,"\tComparing with... %s\n",tempstr.c_str());
    if (iter->second->kernel_name.find(local_kernel_name) != string::npos) {
      size = kernels.size();
      found = true;
      //fprintf(stderr,"Found!!\n\n");
    } else {
      size++;
      iter++;
    }
  } while (size < kernels.size());

  if (found) {
    if (current_CUDA_kernel == iter->second->kernel_name){
      different_kernel = false;
    } else{
      different_kernel = true;
    }
    current_CUDA_kernel = iter->second->kernel_name;
    cuda_execution_complete = 0;
    traceSize = ((CUDAKernel *) (kernels[current_CUDA_kernel]))->tracesize;
    return;
  } else {
    MSG("Cannot find details for kernel %s. Verify the .info file \n Terminating...", local_kernel_name);
    exit(-1);
  }
}/*}}}*/

extern "C" void GPUReader_CUDA_exec_done()
{/*{{{*/
  gettimeofday(&endTime, 0);  // WARNING: This does not support
  static double msecs = 0.0;
  double msec = (endTime.tv_sec - stTime.tv_sec) * 1000 + (endTime.tv_usec - stTime.tv_usec) / 1000;
  msecs += msec;
  msecGpu.add(msec);
  //msecGpu.stop();
  //totalThreadCount.stop();
}/*}}}*/

extern "C" void GPUReader_getKernelParameters(uint32_t * DFL_REG32, uint32_t * DFL_REG64, uint32_t * DFL_REGFP, uint32_t * DFL_REGFP64, uint32_t * SM_REG32, uint32_t * SM_REG64, uint32_t * SM_REGFP, uint32_t * SM_REGFP64, uint32_t * SM_ADDR, uint32_t * TRACESIZE)
{/*{{{*/

  CUDAKernelMapType::iterator iter = kernels.begin();
  iter = kernels.find(current_CUDA_kernel);
  if (iter != kernels.end()) {

    *DFL_REG32 = iter->second->dfl_reg32;
    *DFL_REG64 =iter->second->dfl_reg64;
    *DFL_REGFP = iter->second->dfl_regFP;
    *DFL_REGFP64 = iter->second->dfl_regFP64;

    *SM_REG32 = iter->second->sm_reg32;
    *SM_REG64 = iter->second->sm_reg64;
    *SM_REGFP = iter->second->sm_regFP;
    *SM_REGFP64 = iter->second->sm_regFP64;
    *SM_ADDR = iter->second->sm_addr;

    *TRACESIZE = iter->second->tracesize;
    /*
     *DFL_REG32 = CUDAKernel::max_dfl_reg32;
     *DFL_REG64 = CUDAKernel::max_dfl_reg64;
     *DFL_REGFP = CUDAKernel::max_dfl_regFP;
     *DFL_REGFP64 = CUDAKernel::max_dfl_regFP64;

     *SM_REG32 = CUDAKernel::max_sm_reg32;
     *SM_REG64 = CUDAKernel::max_sm_reg64;
     *SM_REGFP = CUDAKernel::max_sm_regFP;
     *SM_REGFP64 = CUDAKernel::max_sm_regFP64;
     *SM_ADDR = CUDAKernel::max_sm_addr;

     *TRACESIZE = CUDAKernel::max_tracesize;
     */
  } else {
    MSG("No such kernel exists");
    exit(-1);
  }

}/*}}}*/

extern "C" void GPUReader_init_trace(uint32_t * h_trace)
{/*{{{*/
  gpuTM->init_trace(h_trace);
  cuda_execution_complete = 0;
  cuda_execution_started = 0;
}/*}}}*/

extern "C" uint32_t GPUReader_decode_trace(uint32_t * h_trace, uint32_t * qemuid, void *env)
{/*{{{*/

  if (firstevertime == true){
  }


  uint32_t *h_trace_qemu = h_trace;
  //uint64_t global_fid;
  uint64_t local_fid;

  bool alldone = true;

  if (!cuda_execution_started && (cuda_execution_complete == 0)) {
    MSG("\n\n\nResuming the GPU Threads, Pausing the CPU Threads");
    cuda_execution_started = 1;
    oldqemuid = *qemuid;

    IS(MSG("Current QEMU ID is %d i (%d) ",(int)( *qemuid),(int)oldqemuid));

    uint64_t numSM = gpuTM->ret_numSM();

    if (numSM > gsampler->getNumFlows()){
      (numSM = gsampler->getNumFlows());
    }

    gettimeofday(&stTime, 0);

    bool onestarted = false;
    GPUFlowsAlive=0;

    for (local_fid = 0; (local_fid < numSM) && (onestarted == false); local_fid++) {
      if (gpuTM->isSMactive(local_fid) == true) {
        GPUFlowsAlive++;
        GPUFlowStatus[local_fid] = true;
        onestarted = true;
      } else {
        GPUFlowStatus[local_fid] = false;
      }
    }

    fprintf(stderr,"PAUSING CPU");
    qsamplerlist[oldqemuid]->pauseThread(oldqemuid);
    //gsampler->pauseThread(oldqemuid);

    while (local_fid < numSM) {
      if (gpuTM->isSMactive(local_fid) == true) {
        GPUFlowsAlive++;
        GPUFlowStatus[local_fid] = true;
      } else{
        GPUFlowStatus[local_fid] = false;
      }
      local_fid++;
    }
#if 0
    //Resume all the GPUThreads
    if (MIMDmode){
      for (uint64_t i = 0; i < numSM; i++) {
        for (uint64_t j = 0; j < gpuTM->ret_numSP(); j++) {
          gsampler->resumeThread(gpuTM->mapLocalID(i,j));
        }
      }
    } else {
    }
#endif
  }

  if (!cuda_execution_complete && h_trace_qemu != NULL) {
    I(cuda_execution_complete == 0);
    alldone = gpuTM->decode_trace(gsampler, h_trace_qemu, env, qemuid);
  }

  if (!alldone) {
    // Kernel has completed execution.
    GPUFlowsAlive = 0;
    MSG("All GPU threads for this kernel done! STOP!");

    //gpuTM->decodeSignature(h_trace_qemu);

    cuda_execution_complete = 1;
    cuda_execution_started = 0;
    gpuTM->reset_nextkernel();

    gsampler->stop();

    newqemuid = qsamplerlist[oldqemuid]->getFid(oldqemuid);
    //loadSampler(newqemuid);
    IS(fprintf(stderr,"RESUMING CPU"));
    newqemuid = qsamplerlist[newqemuid]->resumeThread(newqemuid, newqemuid);

    *qemuid = newqemuid;
    if (oldqemuid != newqemuid)
      oldqemuid = newqemuid;
  }  else {
    relaunchcounter++;
  }

  return (alldone);    // no pause

}/*}}}*/

#ifdef USE_OUTDATED_CUDE
extern "C" uint64_t GPUReader_translate_d2h(AddrType addr_ptr)
{/*{{{*/
  if (unifiedCPUGPUmem){
    //Translate device address to the host address.
    uint64_t devaddr = addr_ptr;
    uint64_t hostaddr = 0;

    bool notfound = true;
    for (uint32_t i = 0; i < Addrmap.size(); i++){
      if ((devaddr >= Addrmap[i].dev_start) && (devaddr <= Addrmap[i].dev_end)){
        notfound = false;
        uint32_t offset = devaddr - Addrmap[i].dev_start;
        hostaddr = Addrmap[i].host_start + offset;
        i = Addrmap.size()+1;
      }
    }

    if (notfound){
      I(0);
      IS(MSG("Addr %llx not found!!!",addr_ptr));
    }

    return hostaddr;

  } else {
    //If it not unified memory address, then just mark the memory as GPU memory and we are good.
    uint64_t ret_addr_ptr  = (0x0FFFFFFFFFFFFFFFULL & addr_ptr);
    ret_addr_ptr  = (0xA000000000000000ULL | ret_addr_ptr);
    //IS(MSG("Returning Addr %llx",(ret_addr_ptr)));
    return ret_addr_ptr;
  }
}/*}}}*/

extern "C" uint64_t GPUReader_translate_shared(AddrType addr_ptr, uint32_t blockid, uint32_t warpid)
{/*{{{*/

  uint64_t raw_addr = addr_ptr;    //Upper 32 bits will be zero.
  uint16_t bid      = 0xFFFF & blockid;
  uint8_t wid       = 0xFF & warpid;


  /*******************************************************************
   * The 64 bits are decoded as follows
   * Bits 31-0  -> raw_addr
   * Bits 47-32 -> blockid
   * Bits 48-55 -> warpid
   *
   * The upper three bits are then set to  101 to denote GPU memory
   * ******************************************************************/

  uint64_t upper = wid<<16;
  upper          = upper | bid;
  raw_addr       = (upper<<32) | (0x00000000FFFFFFFFULL & raw_addr);
  raw_addr  = (0xC000000000000000ULL | raw_addr);

  //IS(MSG("Returning Addr %llx",raw_addr));
  return raw_addr;

}/*}}}*/
#else
extern "C" uint64_t GPUReader_encode_rawaddr(AddrType addr_ptr, uint32_t blockid, uint32_t warpid, uint32_t pe_id, bool sharedAddr)
{/*{{{*/

  /*******************************************************************
   * The 64 bits are decoded as follows
   * Bits 31-0  -> raw_addr
   * Bits 36-32 -> warpid (5 bits)
   * Bits 43-37 -> pe_id (7 bits) 
   * Bits 59-44 -> block_id (16 bits)
   * Bit  60    -> 1-Shared/0-Global
   * Bits 61-63 -> Bits differentiating CPU and GPU memory, set to  101 to
   *               denote GPU memory
   *               This means that the upper 4 bits will be 0b1010 i.e. 0xA for global addresses
   *               This means that the upper 4 bits will be 0b1011 i.e. 0xB for shared addresses
   *
   * ******************************************************************/


  if (unifiedCPUGPUmem){
    I(0);
    MSG("Not figured a way out to pass peid and blockid for a unified address");
    exit(1);
  }

  uint64_t raw_addr = addr_ptr;                                         // Upper 32 bits will be zero.
  //raw_addr = setspBits64(raw_addr,warpid,32,47);
  //raw_addr = setspBits64(raw_addr,pe_id,48,55);
  
  raw_addr = setspBits64(raw_addr,warpid ,32,36);
  raw_addr = setspBits64(raw_addr,pe_id  ,37,43);
  raw_addr = setspBits64(raw_addr,blockid,44,59);

  if(sharedAddr){
    raw_addr = setspBits64(raw_addr,1,60,60);
  } else {
    raw_addr = setspBits64(raw_addr,0,60,60);
  }

  raw_addr = setspBits64(raw_addr,5,61,63);

  return raw_addr;

}/*}}}*/
#endif

extern "C" uint32_t GPUReader_reexecute_same_kernel() {
  return reexecute;
}
