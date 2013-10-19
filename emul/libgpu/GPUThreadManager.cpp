/*
ESESC: Super ESCalar simulator
Copyright (C) 2006 University California, Santa Cruz.

Contributed by  Alamelu Sankaranarayanan
Jose Renau

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

#define TRACKGPU_MEMADDR 0
#include "GPUThreadManager.h"
bool printnow = false;
extern   uint64_t numThreads;
extern    int64_t maxSamplingThreads;

extern   uint64_t GPUFlowsAlive;
extern   bool*    GPUFlowStatus;

extern   uint64_t oldqemuid;
extern   uint64_t newqemuid;

extern   uint32_t traceSize;
extern   bool     unifiedCPUGPUmem;
extern   bool     istsfifoBlocked;
extern   bool     rabbit_threads_executing;
extern ThreadBlockStatus *block;

GStatsCntr*  GPUThreadManager::totalPTXinst_T;
GStatsCntr*  GPUThreadManager::totalPTXinst_R;
#if 1 
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Shared;
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Global;
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Rest;
#endif


extern uint64_t GPUReader_translate_d2h(AddrType addr_ptr);
extern uint64_t GPUReader_translate_shared(AddrType addr_ptr, uint32_t blockid, uint32_t warpid);
extern uint32_t roundUp(uint32_t numToRound, uint32_t multiple);
extern uint32_t roundDown(uint32_t numToRound, uint32_t multiple);

GPUThreadManager::GPUThreadManager(uint64_t numFlows, uint64_t numPEs){
  totalPTXinst_T = new GStatsCntr("PTXStats:totalPTXinst_T");
  totalPTXinst_T->setIgnoreSampler();
  //totalPTXinst_T->start();

  totalPTXinst_R = new GStatsCntr("PTXStats:totalPTXinst_R");
  totalPTXinst_R->setIgnoreSampler();
  //totalPTXinst_R->start();

#if 1
  totalPTXmemAccess_Shared = new GStatsCntr("PTXStats:totalmemAccess_Shared");
  totalPTXmemAccess_Shared->setIgnoreSampler();
  //totalPTXmemAccess_Shared->start();

  totalPTXmemAccess_Global = new GStatsCntr("PTXStats:totalmemAccess_Global");
  totalPTXmemAccess_Global->setIgnoreSampler();
  //totalPTXmemAccess_Global->start();

  totalPTXmemAccess_Rest = new GStatsCntr("PTXStats:totalmemAccess_Rest");
  totalPTXmemAccess_Rest->setIgnoreSampler();
  //totalPTXmemAccess_Rest->start();
#endif

  maxSMID = 0;
  numSM = numFlows;
  numSP = numPEs;
  SM = new esescSM[numSM];
  GPUFlowStatus = new bool[numSM];
  SM_fid_revmap = NULL;

  for(uint32_t smid = 0; smid < numSM; smid++){
    SM[smid].setnumPE(numSP);
  }


#if TRACKGPU_MEMADDR
  memdata.open("memaccesses_pe.csv"); // opens the file

  if( !memdata ) { // file couldn't be opened
    cerr << "Error: file could not be opened" << endl;
    exit(1);
  }
#endif 

}

GPUThreadManager::~GPUThreadManager(){
#if TRACKGPU_MEMADDR
  memdata.close();
#endif
  delete []SM;
  delete SM_fid_revmap;
}

inline uint32_t GPUThreadManager::calcOccupancy(){
  CUDAKernel* kernel = kernels[current_CUDA_kernel];

  uint32_t warps_per_block = blockSize/warpsize;
  if (blockSize % warpsize > 0)
    warps_per_block++;

  uint64_t regs_per_block  = (roundUp(kernel->sm10.regs_per_thread * warpsize, thread_granularity)) * warps_per_block;
  uint64_t shmem_per_block  = roundUp(kernel->sm10.shmem_per_kernel, shmem_granularity);

  uint32_t i = 0;
  for (i = 1; i <= max_blocks_sm; i++) {

    //Is the number of warps lesser than the maximum allowed?
    if (warps_per_block*i > max_warps_sm) {
      return (i-1)*warps_per_block; 
    } 

    //Is the amount of shared memory lesser than the maximum allowed?
    if (shmem_per_block*i > max_shmem_sm) {
      return (i-1)*warps_per_block; 
    } 

    //Is the number of registers lesser than the maximum allowed?
    if (regs_per_block*i > max_regs_sm) {
      return (i-1)*warps_per_block; 
    } 
  }

  return (i-1)*warps_per_block; 
}

inline void GPUThreadManager::pauseAllThreads(uint32_t* h_trace, bool init, bool isRabbit){
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;


  if (init){
    if (isRabbit) {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //SkipInst to a big number
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    } else {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
  } else {
    if (isRabbit) {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+2]=32760; //SkipInst to a big number
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    } else {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
  }
}

inline void GPUThreadManager::resumeAllThreads(uint32_t* h_trace, bool init, bool isRabbit){

  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;


  if (init){
    if (isRabbit) {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume);
      }
    } else {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume);
      }
    }
  } else {
    if (isRabbit){
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)   
      }
    } else {
      for(active_thread = 0; active_thread < (int64_t)numThreads; active_thread++){
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  }
}

inline void GPUThreadManager::pauseThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit){
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;

  if (init){
    if (isRabbit){
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+0]=1; //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    } else {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+0]=1; //First BB to 1
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
  } else {
    if (isRabbit){
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    } else {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
  }
}

inline void GPUThreadManager::resumeThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit){
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;
  if(init){
    if (isRabbit) {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    } else  {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  } else {
    if (isRabbit) {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    } else  {
      for(active_thread = startRange; active_thread < (int64_t)endRange; active_thread++){
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  }
}

inline void GPUThreadManager::resumeSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit){
  int64_t active_thread = 0;
  uint64_t maxcount = numSP;

  if (maxcount > SM[smid].totalthreadsinSM) {
    maxcount = SM[smid].totalthreadsinSM;
  }


  if (init) {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+0]=1;    
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+0]=1;    
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    }
  } else {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    }
  }

}

inline void GPUThreadManager::pauseSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit){
  int64_t active_thread = 0;
  uint64_t maxcount = numSP;

  if (maxcount > SM[smid].totalthreadsinSM) {
    maxcount = SM[smid].totalthreadsinSM;
  }

  if (init) {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+0]=1;    
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=1; //Pause
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+0]=1;    
          h_trace[active_thread*traceSize+4]=1; //Pause 
        }
      }
    }
  } else {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)){
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=1; //Pause 
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++){
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        h_trace[active_thread*traceSize+4] = 1; //Pause 
      }
    }
  }

}


void GPUThreadManager::dump(){
#if 0
  for(uint32_t smid = 0; smid < numSM; smid++){
    MSG("*************************************************************************");
    MSG("*******************************SM %d**************************************",smid);
    //SM[smid].dump();
    SM[smid].warpsetdump();
    MSG(" ");

  }

#if 0
  MSG("*******************************BLOCK Status******************************************");

  uint32_t numblks = 0;
  if (maxSamplingThreads >= 0) {
    numblks = maxSamplingThreads/blockSize;
    if (maxSamplingThreads%blockSize != 0)
      numblks++;
  } else {
    uint32_t numblks = numThreads/blockSize;
    if (numThreads%blockSize > 0)
      numblks++;
  }

  for (uint32_t i =0; i < numblks; i++){
    MSG(" Block ID [%d], Status = [%d], threads_at_barrier = [%d]", (block[i].id), block[i].status,block[i].threads_at_barrier);
  }
#endif
#endif
}

inline void GPUThreadManager::clearAll(){


  for (uint32_t localsmid =0; localsmid < numSM; localsmid++){
    SM[localsmid].clearAll();
  }


}

void GPUThreadManager::reOrganize(){
  clearAll();

  static bool first = true;
  if (first) {
    for(uint32_t i=0; i<getNumKernels(); i++)
      kernelsNumSM.push_back(numSM);
  }
  first = false;
  setNumSM(kernelsId[current_CUDA_kernel]);

  for(uint32_t smid = 0; smid < numSM; smid++){
    SM[smid].setnumPE(numSP);
  }

  // This is the step where you determine how many concurrent warps/blocks
  // for the given kernel can be run on the SM.
  uint32_t conc_warps = calcOccupancy(); 
  if (conc_warps == 0) {
    fprintf(stderr, "\n\nThe parameters of the kernel and the specifications of the GPU do not make it possible to launch even a single block on the GPGPU\n This kernel cannot be launched\n\n Exiting...\n");
    exit (-1);
  }

  uint32_t warps_per_block = blockSize/warpsize;
  if (blockSize % warpsize > 0)
    warps_per_block++;

  uint32_t blocks_per_sm = conc_warps/warps_per_block;
  uint32_t th_sb_sw = blocks_per_sm * blockSize;

  for(uint64_t h =0; h <numSM; h++){
    for(uint64_t i =0; i <numSP; i++){
      SM[h].threads_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_instoffset_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_traceoffset_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_status_per_pe[i].reserve(numThreads/numSP/numSM);
    }
  }

  // Add threads to each SM
  uint64_t tmpnum = 0;
  while (tmpnum < numThreads){
    uint64_t blockid = tmpnum/(blocks_per_sm*blockSize);
    uint32_t losmid    = blockid % numSM;
    if ((blocks_per_sm*blockSize) > (numThreads-tmpnum))
      SM[losmid].addThreads(tmpnum,(numThreads - tmpnum),blockSize);
    else{
      SM[losmid].addThreads(tmpnum,(blocks_per_sm*blockSize),blockSize);
    }
    tmpnum += blocks_per_sm*blockSize;
  }

  for(uint32_t smid = 0; smid < numSM; smid++){

    if (SM[smid].totalthreadsinSM > 0){
      SM[smid].setActive();
      uint32_t num_warps = 0;

      num_warps =  (uint32_t)(SM[smid].threads_per_pe[0].size());

      uint32_t nWarps = num_warps;
      uint32_t mybs = blockSize;

      SM[smid].warp_status.reserve(num_warps);
      SM[smid].warp_last_active_pe.reserve(num_warps);
      SM[smid].warp_complete.reserve(num_warps);
      SM[smid].warp_blocked.reserve(num_warps);
      SM[smid].warp_block_map.reserve(num_warps);
      SM[smid].active_sps.reserve(num_warps);
      SM[smid].warp_threadcount.reserve(num_warps);


      do{

        SM[smid].warp_status.push_back(traceReady);
        SM[smid].warp_last_active_pe.push_back(0);
        SM[smid].warp_complete.push_back(0);
        SM[smid].warp_blocked.push_back(0);
        SM[smid].warp_block_map.push_back((SM[smid].threads_per_pe[0][nWarps - num_warps])/blockSize);
        SM[smid].active_sps.push_back(0);


        if (blockSize < numSP){
          SM[smid].warp_threadcount.push_back(blockSize);
        } else{
          if (mybs > numSP){
            SM[smid].warp_threadcount.push_back(numSP);
            mybs = mybs - numSP;
          } else {
            SM[smid].warp_threadcount.push_back(mybs);
            mybs = blockSize;
          }
        }

        num_warps--;
      } while (num_warps > 0);

      num_warps = nWarps;
      do{
        SM[smid].warp_last_active_pe[num_warps-1] = 0;
        num_warps--;
      } while (num_warps > 0);

      SM[smid].cycledthru_currentwarpset =  false;

    } else if (SM[smid].totalthreadsinSM == 0){
      SM[smid].setInactive();
      SM[smid].cycledthru_currentwarpset = true;
    }

    SM[smid].resetCurrentWarp();
    SM[smid].resetCurrentWarpSet();
  }// For each SM loop

  //Update the concurrent warps per SM. Depending on how many threads are there, this number might be different. 
  IS(cout << "concurrent warps per sm = " <<conc_warps<<endl);
  for(uint32_t smid = 0; smid < numSM; smid++){
    SM[smid].updateCurrentWarps(conc_warps,max_warps_sm,th_sb_sw);
  }


  /* Set maxSamplingThreads */
  maxSamplingThreads = gsampler->getThreads2Simulate();

  if (maxSamplingThreads > 0){

    IS(cout <<"maxSamplingThreads defined as "<< maxSamplingThreads<<endl);
    maxSamplingThreads = roundUp(maxSamplingThreads, th_sb_sw);
    IS(cout <<"maxSamplingThreads rounded to "<<conc_warps*warpsize<< " to be " <<  maxSamplingThreads<<endl);

    if ((int64_t) numThreads < maxSamplingThreads){
      maxSamplingThreads = -1; 
    } 

  }

  IS(cout<<endl<<endl<<"maxSamplingThreads = "<< maxSamplingThreads<<endl);

  if (maxSamplingThreads != 0){
    uint64_t total_timing_threads  = maxSamplingThreads; 
    if ((int64_t) total_timing_threads == -1){
      total_timing_threads = numThreads;
    }
    uint32_t smid = 0;

    do {
      SM[smid].addTimingThreads(&total_timing_threads, warpsize);
      smid++;
      if (smid == numSM){
        smid = 0;
      }
    }  while (total_timing_threads > 0);
  }

  for(uint32_t smid = 0; smid < numSM; smid++){
    SM[smid].warpsets = SM[smid].timing_warps/conc_warps;
    if (SM[smid].timing_warps%conc_warps != 0){
      SM[smid].warpsets++;
    }
  }


  int64_t active_sp_in_warp      = 0;
  for(uint32_t smid = 0; smid < numSM; smid++){
    uint64_t warpid = 0; 
    while (warpid < SM[smid].warp_status.size()) {
      active_sp_in_warp      = 0;
      for (uint32_t peid = 0; peid < numSP; peid++) {
        if (warpid < SM[smid].threads_per_pe[peid].size() && (SM[smid].thread_status_per_pe[peid][warpid] != invalid)){
          active_sp_in_warp++;
        }
      }
      //IS(MSG("There are %d active SPs in this warp", (int) active_sp_in_warp));
      SM[smid].active_sps[warpid] = active_sp_in_warp;
      warpid++;
    }
  }

  // Initialize the barrier status data structure
  if (block != NULL){
    delete [] block;
  }

  uint32_t numblks = 0;
  if (maxSamplingThreads >= 0) {
    numblks = maxSamplingThreads/blockSize;
    if (maxSamplingThreads%blockSize!= 0)
      numblks++;
  } else {
    numblks = numThreads/blockSize;
    if (numThreads%blockSize != 0)
      numblks++;
  }

  block = new ThreadBlockStatus[numblks];

  for (uint32_t i = 0; i < numblks ; i++) {
    block[i].init(i);
  }

  for (uint64_t sm = 0; sm < numSM; sm++){
    uint64_t wid = 0; 
    uint64_t blk = 0;
    while (wid < SM[sm].timing_warps) {
      blk = SM[sm].warp_block_map[wid];
      block[blk].addWarp(wid);
      wid++;
    }
  }

  IS(dump());
  //exit(-1);
  ////////////////
}

void GPUThreadManager::link(uint64_t glosmid, uint64_t smid){
  I(smid < numSM);
  SM[smid].setglobal_smid(glosmid);
  if (glosmid > maxSMID)
    maxSMID = glosmid;
  else
    I(0);
}

void GPUThreadManager::createRevMap(){
  //How do we check if a gloid actually is a valid GPU flow? :-)
  I(SM_fid_revmap == NULL);

  SM_fid_revmap = new uint64_t[maxSMID+1];
  for (uint64_t i = 0; i < numSM; i++){
    SM_fid_revmap[SM[i].getglobal_smid()] = i;
  }
}

void GPUThreadManager::init_trace(uint32_t* h_trace){
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  bool init   = true;
  bool rabbit = true;

  //Clear the trace arrays
  memset((void *) h_trace,0,numThreads*traceSize*4);

  if (maxSamplingThreads != 0){

    //FEW OR ALL TIMING
    pauseAllThreads(h_trace,init,(!rabbit));

    for(uint32_t smid = 0; smid < numSM; smid++){
      if (SM[smid].totalthreadsinSM > 0){
        IS(fprintf(stderr, "\n\nSM [%u] has %u warps in timing and %u warps fastforwarded",smid, SM[smid].timing_warps, SM[smid].warp_status.size()-SM[smid].timing_warps));
        uint64_t warpid = 0;
        //SM[smid].smstatus = SMTiming;
        SM[smid].smstatus = SMalive;

        if (SM[smid].timing_warps > 0){
          SM[smid].resetCurrentWarpSet();
          warpid = SM[smid].getCurrentWarp();
          I(warpid<SM[smid].timing_warps);
          resumeSelectivethreads(h_trace,smid,warpid,init,(!rabbit)); //Unpause the timingthreads (one warp only)
        } else {
          //FIXME: Anything to be done if the SM has no timing threads???
        }

        for (warpid = SM[smid].timing_warps; warpid<SM[smid].warp_status.size() ;warpid++){
          pauseSelectivethreads(h_trace,smid,warpid,init,rabbit); 
        }
      } else {
        fprintf(stderr, "\nSM[%d] : No Threads\n\n",smid);
        SM[smid].smstatus = SMcomplete;
      }
    }
    rabbit_threads_executing = false;
  } else {
    //ALL RABBIT 
    resumeAllThreads(h_trace,init,rabbit);
    rabbit_threads_executing = true;
    for(uint32_t smid = 0; smid < numSM; smid++){
      SM[smid].smstatus = SMalive;
    }
  }
}


bool GPUThreadManager::Rabbit(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid){
  /*
   * In this loop we start the Rabbit mode and then  insert the instructions for the threads that
   * we have decided to skip.
   */
  CUDAKernel* kernel = kernels[current_CUDA_kernel];

  uint32_t trace_currentbbid;
  uint32_t trace_nextbbid;
  uint32_t trace_ffwd_insts;

  RAWInstType insn;
  AddrType    addr = 0;
  DataType    data = 0;
  uint32_t    pc = 0;
  char        op = 0;
  uint64_t    gloicount = 0;
  int64_t     active_thread = 0;

  uint64_t glofid = 1;
  gsampler->startRabbit(glofid); //startRabbit calls emul->startRabbit, which does not really use the flowID, so we are okay

  //insn            = CUDAKernel::packID(trace_currentbbid, instoffset);
  //CudaInst *cinst = kernel->getInst(trace_currentbbid, instoffset);
  insn            = CUDAKernel::packID(1, 0);
  CudaInst *cinst = kernel->getInst(1, 0);
  pc              = cinst->pc;
  op              = cinst->opkind;
  data            = 0;


  for(uint32_t smid = 0; smid < numSM; smid++){
    if (SM[smid].totalthreadsinSM > 0){

      if (SM[smid].smstatus == SMalive){

        for (uint64_t warpid = SM[smid].timing_warps; warpid<SM[smid].warp_status.size() ;warpid++){

          for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){

            // data            = pe_id; 
            if (SM[smid].thread_status_per_pe[pe_id][warpid] == running){
              active_thread     = SM[smid].threads_per_pe[pe_id][warpid];
              I(active_thread >= 0);
              trace_nextbbid    = h_trace_qemu[(active_thread*traceSize)+0];
              trace_currentbbid = h_trace_qemu[(active_thread*traceSize)+1];
              trace_ffwd_insts  = h_trace_qemu[(active_thread*traceSize)+9];

              if (trace_currentbbid != 0){

                //MSG("TID: %llu Current BBID = %d, Next BBID = %d",active_thread,trace_currentbbid,trace_nextbbid);
                gloicount += trace_ffwd_insts;

                if (trace_nextbbid == 0){
                  SM[smid].thread_status_per_pe[pe_id][warpid] = execution_done; // OK
                  h_trace_qemu[active_thread*traceSize+4]      = 1;              // Pause
                  h_trace_qemu[(active_thread*traceSize)+1]    = 0;              // Mark the currentbbid to 0 so that u don't keep inserting the instruction
                  SM[smid].rabbitthreadsComplete++;
                  SM[smid].warp_complete[warpid]++;
                }
              } 
            } else {
              //IS(MSG("Why is this happening"));
            }
          }
        }
      }
    }

    if (SM[smid].totalthreadsinSM > 0){
      //if (maxSamplingThreads == 0){ // all Rabbit 
      if (canTurnOff(smid) && GPUFlowStatus[smid]) {
        SM[smid].setInactive();
        if (GPUFlowStatus[smid]){ 
          GPUFlowStatus[smid] = false; 
          GPUFlowsAlive--;
        }
      }
      //}
    }//End of if (SM[smid].totalthreadsinSM > 0)
  }

  gsampler->queue(insn,pc,addr,data,glofid,op,gloicount,env); //None of the arguments except icount, env really matter.
  totalPTXinst_R->add(gloicount);
  //IS(cerr <<"Adding " <<gloicount<< " instructions starting from Basic Block "<<h_trace_qemu[1024*46+1]<< " going to basicBlock " <<h_trace_qemu[1024*46+0]<<endl);

  //gsampler->stop(1); // We force weight 1 to avoid loss of any stats remaining from timing.
  gsampler->stop(); // We force weight 1 to avoid loss of any stats remaining from timing.

  if (GPUFlowsAlive == 0)
    return true;
  else
    return false;
}

bool GPUThreadManager::Timing(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid){

  bool atleast_oneSM_alive                 = false;
  uint64_t threads_in_current_warp_done    = 0;
  uint64_t threads_in_current_warp_barrier = 0;
  uint64_t timing_threads_complete         = 0;                  // USEME
  int64_t total_timing_threads             = maxSamplingThreads;

  if ((int64_t) total_timing_threads == -1){
    total_timing_threads = numThreads;
  }

  uint32_t smid                            = 0;
  uint64_t warpid                          = 0;
  uint64_t glofid                          = 0;
  int64_t  active_thread                   = 0;
  uint32_t trace_currentbbid               = 0;
  uint32_t trace_nextbbid                  = 0;
  uint32_t trace_skipinstcount             = 0;
  uint32_t trace_branchtaken               = 0;
  uint32_t trace_unifiedBlockID            = 0;
  //uint32_t trace_warpid;

  CUDAKernel* kernel   = kernels[current_CUDA_kernel];
  traceSize            = kernel->tracesize;
  uint32_t tracestart  = (12 + kernel->predicates);    // Fixed.
  RAWInstType insn     = 0;
  AddrType    addr     = 0;
  DataType    data     = 0;
  uint32_t    pc       = 0;
  char        op       = 0;
  uint64_t    icount   = 1;
  bool memoryInstfound = false;
  bool changewarpnow   = false;
  bool tsfifoblocked = false;

  gsampler->startTiming(glofid); //startTiming calls emul->startTiming, which does not really use the flowID, so we are okay

  do{ //for each SM

    smid                    = 0;
    atleast_oneSM_alive     = false;
    timing_threads_complete = 0;

    for (smid = 0; smid < numSM; smid++){

      glofid          = mapLocalID(smid);
      memoryInstfound = false;
      changewarpnow   = false;

      if ((SM[smid].smstatus == SMalive))  { 
        I(SM[smid].timing_warps > 0); //Otherwise the status should be something else
        warpid = SM[smid].getCurrentWarp();

        if (SM[smid].warp_status[warpid] == waiting_barrier) {
          uint32_t blockid = SM[smid].warp_block_map[warpid];
          uint32_t compl_th = 0;
          for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++) { 
            uint32_t localwid = block[blockid].warps[w_per_b];
            compl_th +=  SM[smid].warp_complete[localwid];
          }

          if (block[blockid].checkThreadCount(blockSize,compl_th) == true) {
            //if (printnow) fprintf (stderr, "SM[%d] Warp [%d] Have to clear warps in block %d :", (int) smid, (int) warpid, (int) blockid);
            block[blockid].clearBarrier();
            uint32_t localwid = 0;
            for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++){
              localwid = block[blockid].warps[w_per_b];
              //if (printnow) IS(fprintf(stderr, "%d ,", localwid));
              SM[smid].warp_status[localwid]         = relaunchReady;
              SM[smid].warp_last_active_pe[localwid] = 0;
              SM[smid].warp_blocked[localwid]        = 0;
              for (uint32_t spid = 0; spid < numSP; spid++) {
                if ((localwid < SM[smid].threads_per_pe[spid].size()) && (SM[smid].thread_status_per_pe[spid][localwid] != invalid)){
                  SM[smid].thread_status_per_pe[spid][localwid] = block_done;
                }
              }
            }
            SM[smid].warp_status[warpid]      = relaunchReady;
            SM[smid].warp_blocked[warpid]     = 0;
          } else {
            I(0); // Something is seriously incorrect!!
          }

        } else if (SM[smid].warp_status[warpid] == waiting_memory){
          //I(0); // Just to see if we ever reach here. 
          SM[smid].warp_status[warpid] = traceReady;

        } else if (SM[smid].warp_status[warpid] == waiting_TSFIFO){
          //I(0); // Just to see if we ever reach here. 
          SM[smid].warp_status[warpid] = traceReady;

        } else if (SM[smid].warp_status[warpid] == relaunchReady){
          // I(0); // Just to see if we ever reach here. 
          // No more instructions to insert from this warp.
          // Mark the SM as done and wait for the kernel to be respawned for more instructions
          SM[smid].smstatus = SMdone;

        } else if (SM[smid].warp_status[warpid] == warpComplete){
          I(0); //Just to see if we ever reach here. 
          //Move to the next warp. 
          //Not sure if this needs to be there.
        }

        if (SM[smid].warp_status[warpid] == traceReady){

          // This state indicates there is data from at least some threads to insert into the queues
          // IS(MSG("********* Starting SM [%d] *********", smid));

          threads_in_current_warp_done    = 0;
          threads_in_current_warp_barrier = 0;

          bool start_halfway              = false;
          if(SM[smid].warp_last_active_pe[warpid]){
            start_halfway                 = true;
          }

          tsfifoblocked = false;
          for (int32_t pe_id = SM[smid].warp_last_active_pe[warpid]; ((pe_id < (int32_t)numSP) || (start_halfway)) && !(tsfifoblocked); pe_id++) {
            if (warpid < SM[smid].threads_per_pe[pe_id].size()){

              if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_memaccess){
                I(0);
                SM[smid].thread_status_per_pe[pe_id][warpid] = running; //Change it to running or whatever.
              } else if (SM[smid].thread_status_per_pe[pe_id][warpid] == paused_on_a_barrier){
                //I(0);
                threads_in_current_warp_barrier++;
              }

              if (SM[smid].thread_status_per_pe[pe_id][warpid] == running){
                active_thread        = SM[smid].threads_per_pe[pe_id][warpid];
                I(active_thread >= 0);
                trace_nextbbid       = h_trace_qemu[(active_thread*traceSize)+0];
                trace_currentbbid    = h_trace_qemu[(active_thread*traceSize)+1];
                trace_skipinstcount  = h_trace_qemu[(active_thread*traceSize)+2];
                trace_branchtaken    = h_trace_qemu[(active_thread*traceSize)+3];
                trace_unifiedBlockID = h_trace_qemu[(active_thread*traceSize)+5];

                uint32_t numbbinst   = kernel->bb[trace_currentbbid].number_of_insts;
                uint32_t instoffset  = 0;
                uint32_t traceoffset = 0;

                if (trace_currentbbid == 0 ){
                  SM[smid].thread_status_per_pe[pe_id][warpid] = block_done; //or should it be execution_done?
                  threads_in_current_warp_done++;

                } else {

                  if (kernel->isBarrierBlock(trace_currentbbid)){
                    uint32_t blockid = SM[smid].warp_block_map[warpid];

                    //if (printnow)
                    //IS(MSG ("BARRIER: SM %d PE %d Thread %d Block %d blocked on BB[%d], will resume from BB[%d]",(int)glofid, (int)pe_id, (int)active_thread, blockid, (int)trace_currentbbid, (int) trace_nextbbid));

                    block[blockid].incthreadcount();
                    SM[smid].thread_status_per_pe[pe_id][warpid] = paused_on_a_barrier;

                    h_trace_qemu[active_thread*traceSize+4]=1;  //  Pause the current thread.
                    threads_in_current_warp_barrier++;          //Increment the block_barrier_count by one. 
                    threads_in_current_warp_done++;             //Incremented when the thread is done, or hits a barrier
                    SM[smid].warp_blocked[warpid]++;

                    //  Insert a RALU instruction
                    //  FIXME-Alamelu: 

                    uint32_t compl_th = 0;
                    for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++){
                      uint32_t localwid = block[blockid].warps[w_per_b];
                      compl_th         +=  SM[smid].warp_complete[localwid];
                    }

                    if (block[blockid].checkThreadCount(blockSize,compl_th) == true){
                      //if (printnow) fprintf (stderr, "SM[%d] Warp [%d] Have to clear warps in block %d :", (int) smid, (int) warpid, (int) blockid);

                      block[blockid].clearBarrier();
                      uint32_t localwid = 0;
                      for (uint32_t w_per_b = 0; w_per_b<block[blockid].warps.size(); w_per_b++){
                        localwid = block[blockid].warps[w_per_b];
                        //if (printnow) IS(fprintf(stderr, "%d ,", localwid));
                        SM[smid].warp_status[localwid]         = relaunchReady;
                        SM[smid].warp_last_active_pe[localwid] = 0;
                        SM[smid].warp_blocked[localwid]        = 0;
                        for (uint32_t spid = 0; spid < numSP; spid++) {
                          if ((localwid < SM[smid].threads_per_pe[spid].size()) && (SM[smid].thread_status_per_pe[spid][localwid] != invalid)){
                            SM[smid].thread_status_per_pe[spid][localwid] = block_done;
                          }
                        }
                      }
                      // MSG(" ");
                      SM[smid].warp_blocked[warpid] = blockSize;

                    }

                  } else {

                    SM[smid].warp_last_active_pe[warpid] = 0;
                    // Not a barrier block
                    if (!(kernel->isDummyBB(trace_currentbbid))){
                      // Not a Dummy BBID
                      instoffset      = SM[smid].thread_instoffset_per_pe[pe_id][warpid];
                      traceoffset     = SM[smid].thread_traceoffset_per_pe[pe_id][warpid];
                      insn            = CUDAKernel::packID(trace_currentbbid, instoffset);
                      CudaInst *cinst = kernel->getInst(trace_currentbbid, instoffset);
                      pc              = cinst->pc;
                      op              = cinst->opkind;
                      CUDAMemType memaccesstype;

                      if (op == 0) {// ALU ops
                        addr = 0;

                      } else if (op == 3){// Branches
                        if (!(trace_branchtaken)){
                          addr = 0;//Not Address of h_trace_qemu[(tid*traceSize+0)];
                        } else {
                          if (trace_nextbbid != 0){
                            if (kernel->isDummyBB(trace_nextbbid)){
                              uint16_t tempnextbb = trace_nextbbid+1;

                              while ((kernel->isDummyBB(tempnextbb)) && (tempnextbb <= kernel->getLastBB()) ){
                                tempnextbb++;
                              }

                              if (tempnextbb <= kernel->getLastBB()){
                                addr = kernel->getInst(tempnextbb,0)->pc;
                              } else {
                                I(0);       //next blocks are all 0!
                                addr = 0x0; //ERROR
                              }
                            } else {
                              addr = kernel->getInst(trace_nextbbid,0)->pc;
                            }
                          }
                        }
                      } else {//Load Store

                        memaccesstype = kernel->bb[trace_currentbbid].insts[instoffset].memaccesstype;

                        if ((memaccesstype == ParamMem) || (memaccesstype == RegisterMem) || (memaccesstype == ConstantMem) || (memaccesstype == TextureMem)){
                          addr = 1000; //FIXME: Constant MEM
                        } else {

                          addr = h_trace_qemu[active_thread*traceSize+tracestart+traceoffset];
                          traceoffset++;
                        }

                        if (addr){
                          I(op == 1 || op == 2); //Sanity check;

                          if (memaccesstype == GlobalMem){
                            /*
                               if (op == 1){
                               MSG("Load Basic Block %d Inst %d traceoffset = %d", trace_currentbbid, instoffset, traceoffset);
                               } else {
                               MSG("Store Basic Block %d Inst %d traceoffset = %d", trace_currentbbid, instoffset, traceoffset);
                               }
                               */
                            addr = GPUReader_translate_d2h(addr);
                            //if ((active_thread == 0) || (active_thread == 1)) {
#if TRACKGPU_MEMADDR
                            int acctype = 0;
                            if (op == 1){
                              //Global Load
                              acctype = 0
                            } else {
                              //Global Store
                              acctype = 1
                            }
                            if ((pe_id == 1)) {
                              memdata <<active_thread<< ","<<type<<","<< hex << pc <<","<< hex << addr <<endl;
                            }
#endif
                          } else if (memaccesstype == SharedMem){
                            //MSG("Shared PC %llx OP %d Addr %llu", pc, op, addr); 
                            //addr = GPUReader_translate_shared(addr, trace_unifiedBlockID, warpid);
                            addr = GPUReader_translate_shared(addr, trace_unifiedBlockID,0);
                            //MSG("Shared Addr %llx",addr); 
                            //addr += 1000;
                            //addr = CUDAKernel::pack_block_ID(addr,trace_unifiedBlockID); 
                            //if ((active_thread == 0) || (active_thread == 1)) {
#if TRACKGPU_MEMADDR
                            if (op == 1){
                              //Shared Load
                              acctype = 2
                            } else {
                              //Shared Store
                              acctype = 3
                            }

                            if ((pe_id == 1)) {
                              memdata <<active_thread<< ","<<type<<","<< hex << pc <<","<< hex << addr <<endl;
                              //memdata << "("<<active_thread<< ") Shared: 0x" << hex <<addr <<endl;
                            }
#endif
                          }

                        }

                      }

                      //Data is not used for anything at the moment. (No Warmup) So I am storing the pe-information in this field.
                      //Setting the PE (dinst::pe_id) for each instruction
                      //FIXME: (Need to multiply the registers by the number of concurrent warps!) i.e. pass the warp information. 
                      data = pe_id;
                      data = data<<32|((SM[smid].getCurrentLocalWarp())*numSP); //Causes segmentation

                      icount = 1;
                      gsampler->queue(insn,pc,addr,data,glofid,op,icount,env);

                      if (istsfifoBlocked){
                        //MSG("\t\t\t TSFIFO[%llu] BLOCKED, (SM[%u], Warp[%u]). Moving to Next SM",glofid,smid,warpid);
                        //MSG("Last Active PE                = %d", (int) pe_id);
                        gsampler->resumeThread(glofid);
                        istsfifoBlocked                      = false;
                        SM[smid].warp_status[warpid]         = waiting_TSFIFO;
                        SM[smid].warp_last_active_pe[warpid] = pe_id;
                        pe_id                                = numSP+1;           // move on to the next sm.
                        tsfifoblocked                        = true;
                      } else {
                        //if (printnow)
                        //MSG("\t\t\t\t\t\t\t\t\t\tThread %d at SM[%d] PE [%d][%llu] BB [%d] Inst [%d] Trace[%d]",(int)active_thread,(int)smid, (int)pe_id, warpid,trace_currentbbid, instoffset,traceoffset);
                        //MSG("Thread %d Adding inst to CPU %d, PE %d---->PC %x",(int)active_thread, (int) glofid, (int) data, pc);
                        SM[smid].thread_instoffset_per_pe[pe_id][warpid]++;

                        totalPTXinst_T->add(icount);
                        if ((op == 1) ||(op == 2)) {  // Memory ops
                          if ((memaccesstype == SharedMem)){
                            totalPTXmemAccess_Shared->inc();
                          } else if ((memaccesstype == GlobalMem)){
                            totalPTXmemAccess_Global->inc();
                          } else {
                            totalPTXmemAccess_Rest->inc();
                          }
                          SM[smid].thread_traceoffset_per_pe[pe_id][warpid]++;
                          //MSG("Memory inst");
                          memoryInstfound = true;
                        }

                        //if all the instructions in the block have been inserted.
                        if (SM[smid].thread_instoffset_per_pe[pe_id][warpid] >= (int32_t) numbbinst) {
                          threads_in_current_warp_done++;
                          if (trace_nextbbid == 0){
                            SM[smid].thread_status_per_pe[pe_id][warpid] = execution_done;                                                                                      // OK
                            h_trace_qemu[active_thread*traceSize+4]      = 1;                                                                                                   // Pause
                            h_trace_qemu[(active_thread*traceSize)+1]    = 0;                                                                                                   // Mark the currentbbid to 0 so that u don't keep inserting the instruction
                            SM[smid].timingthreadsComplete++;
                            I(SM[smid].warp_status[warpid] != paused_on_a_barrier);
                            SM[smid].warp_complete[warpid]++;
                            //if (printnow)
                            //MSG("Thread %d at SM[%d] PE [%d][%llu] BB [%d] Inst [%d] COMPLETE",(int)active_thread,(int)smid, (int)pe_id, warpid,trace_currentbbid, instoffset);

                            if (SM[smid].warp_complete[warpid] >= numSP){
                              //if (printnow)
                              //MSG("SM[%d] PE [%d] WARP [%d] COMPLETE.. Should change warp!!",(int)smid, (int)pe_id, (int)warpid);
                              SM[smid].warp_status[warpid]               = warpComplete;
                            }

                          } else {
                            SM[smid].thread_status_per_pe[pe_id][warpid] = block_done;
                            SM[smid].warp_last_active_pe[warpid]         = 0;
                          }
                        }
                      }//End of if tsfifo not blocked
                    } else {
                      //Dummy BBID
                      threads_in_current_warp_done++;
                    }
                  } //Not a Barrier Block
                } 
              } else {
                //If thread_status != running or any other status that does not allow you to insert instructions
                if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)){
                  threads_in_current_warp_done++;
                }
              }
            } // End of if (warp_id < threads_per_pe[pe_id].size())

            if ((pe_id == (int32_t)(numSP - 1)) && (start_halfway)){
              //MSG("------>>>>>>>>> Reached here <<<<<<<<------[%d]",pe_id);
              SM[smid].warp_last_active_pe[warpid] = 0;
              pe_id                                = -1;
              start_halfway                        = false;
              threads_in_current_warp_done         = 0;
            }

          } //End of "for each pe" loop

          //Now that I have attempted to insert instructions from all the pes in the SM,

          if (SM[smid].warp_blocked[warpid] > 0){
            if ((SM[smid].warp_blocked[warpid]+SM[smid].warp_complete[warpid]) >= SM[smid].active_sps[warpid]) {
              //if (printnow)
              //MSG("All threads in current warp blocked");
              SM[smid].warp_status[warpid] = waiting_barrier;
              //if (printnow)
              //MSG("Marking SM[%d] warp [%d] as wating_barrier", (int)smid, (int)warpid);
              if ((SM[smid].warp_blocked[warpid]+SM[smid].warp_complete[warpid]) >= blockSize){
                SM[smid].warp_status[warpid]      = relaunchReady;
                SM[smid].warp_blocked[warpid]     = 0;
                //if (printnow)
                //MSG("UnMarking SM[%d] warp [%d] as wating_barrier", (int)smid, (int)warpid);
              }
              changewarpnow   = true; 
            } else if (threads_in_current_warp_done >= SM[smid].active_sps[warpid]) {
              SM[smid].warp_status[warpid] = relaunchReady;
              SM[smid].smstatus = SMdone;
            }
          } else if (SM[smid].warp_complete[warpid] >= SM[smid].active_sps[warpid]) {
            //if (printnow)
            //MSG("All threads in current warp complete");
            SM[smid].warp_status[warpid] = warpComplete;
            changewarpnow   = true; //Move to the next warp.
            SM[smid].smstatus = SMdone;
          } else if (threads_in_current_warp_done >= SM[smid].active_sps[warpid]) {
            //if (printnow)
            //MSG("All threads in current warp done");
            SM[smid].warp_status[warpid] = relaunchReady;
            SM[smid].smstatus = SMdone;
          } else {
            //MSG("[%d] threads in current warp blocked\n [%d] threads in current warp complete\n [%d] threads in current warp done\n " ,(int)SM[smid].warp_blocked[warpid] ,(int)SM[smid].warp_complete[warpid] ,(int)threads_in_current_warp_done);
          } 

          if (memoryInstfound || changewarpnow ){
            //Move to the next warp. 
            //Make sure that the threads in the current warp are paused
            //Switch to the next warp in the warpset which is not "Complete"
            //See if the new warp has instructions to insert. If yes, then good, otherwise mark SMstatus as SMDone, and be ready for relaunch
            //SM[smid].smstatus = SMdone;

            if (memoryInstfound) 
            {
              //MSG("ENCOUNTERED MEMORY INSTRUCTION!!!!!!!! SHOULD BE SWITCHING TO THE NEXT WARP");
              SM[smid].warp_status[warpid] = waiting_memory; // This has to be done at this point, so that all the threads can insert that one instruction
            } else if (changewarpnow ){
              I((SM[smid].warp_status[warpid] == warpComplete) || (SM[smid].warp_status[warpid] == relaunchReady) || (SM[smid].warp_status[warpid] == waiting_barrier));
            }

            bool warpSwitchsucc = switch2nextwarp(smid,h_trace_qemu);
            if (!warpSwitchsucc){
              //if (printnow)
              //MSG("Switch2nextwarp returned false");
              SM[smid].smstatus = SMcomplete;

            } else {
              uint64_t newwarpid = 0;
              newwarpid = SM[smid].getCurrentWarp();
              if (   (SM[smid].warp_status[newwarpid] == traceReady)) {
                //if (printnow)
                //MSG("Yes,traceReady new warp has instructions to insert");
                SM[smid].smstatus = SMdone;

              } else if (SM[smid].warp_status[newwarpid] == waiting_barrier) {
                //if (printnow)
                //MSG("Yes,new waiting_barrier warp has instructions to insert");
                //I(0);

              } else if (SM[smid].warp_status[newwarpid] == waiting_memory) {
                //if (printnow)
                //MSG("Yes,new waiting_memory warp has instructions to insert");
                SM[smid].smstatus = SMalive;

              } else {
                switch(SM[smid].warp_status[newwarpid]){
                  case waiting_TSFIFO:
                    //if (printnow)
                    //MSG("1: No, new warp has NO instructions to insert, warpstatus is waiting_tsfifo");
                    break;
                  case relaunchReady:
                    //if (printnow)
                    //MSG("2: No, new warp has NO instructions to insert, warpstatus is relaunchready");
                    break;
                  case warpComplete:
                    //if (printnow)
                    //MSG("3: No, new warp has NO instructions to insert, warpstatus is warpComplete");
                    break;
                  default:
                    I(0);
                    break;
                }
                SM[smid].smstatus = SMdone;
              }
            }
            memoryInstfound = false;
            changewarpnow   = false;
            threads_in_current_warp_done  = 0;
          }
        }

        possiblyturnOFFSM(smid);

        if (SM[smid].smstatus == SMalive){
          atleast_oneSM_alive = true;
        }
      } else if (SM[smid].smstatus == SMdone ) {
        //MSG("SM[%d] is SMDone",(int)smid);
        //Nothing for now.
        //possiblyturnOFFSM(smid);
      } else if (SM[smid].smstatus == SMcomplete) {
        //MSG("SM[%d] is SMComplete",(int)smid);
        //Should already be turned off. 
        //possiblyturnOFFSM(smid);
      } else if (SM[smid].smstatus == SMRabbit) { 
        //MSG("SM[%d] is SMRabbit",(int)smid);
        //Nothing for now.
        //possiblyturnOFFSM(smid);
      } 

      timing_threads_complete += SM[smid].timingthreadsComplete ;

    } // End of "for each SM" loop

  } while (atleast_oneSM_alive);

  gsampler->stop();

  //if (printnow)
  //IS(MSG("Number of Timing threads completed = %llu out of %llu\n\n\n",timing_threads_complete, total_timing_threads)); 

  if (timing_threads_complete < total_timing_threads)
    return true;
  else {
    float ratio = static_cast<float>(numThreads)/static_cast<float>(total_timing_threads);
    gsampler->getGPUCycles(*qemuid,ratio);
    return false;
  }
}

inline void GPUThreadManager::possiblyturnOFFSM(uint32_t smid){
  //MSG("Checking to see if I can turn off SM [%d]", (int) smid);
  if (canTurnOffTiming(smid) && GPUFlowStatus[smid]) {
    IS(MSG ("SM %d Timing DONE, Pausing global flow %d ",(int) smid, (int)SM[smid].getglobal_smid()));
    //gsampler->pauseThread(glofid);
    GPUFlowStatus[smid] = false;
    //MSG("Yes I can turn it off" );
    GPUFlowsAlive--;
  }
}

inline bool GPUThreadManager::switch2nextwarp(uint32_t smid, uint32_t* h_trace){
  int64_t active_thread;
  uint32_t warpid = SM[smid].getCurrentWarp();

  //MSG("Pausing threads in the old warp");
  // Pause all the threads in the current warp
  for (uint32_t pe_id = 0; pe_id < numSP; pe_id++){
    if (warpid < SM[smid].threads_per_pe[pe_id].size()){
      active_thread                      = SM[smid].threads_per_pe[pe_id][warpid];
      if (likely(active_thread >= 0)){
        h_trace[active_thread*traceSize+4] = 1;                                      // Pause
      }
    }
  }

  // Move to the next warp whose status is not warp_complete or warp_done. 
  bool warpfound          = false;
  bool nomoreWarpSets     = false;
  uint32_t newwarpid      = 0;
  uint32_t localwarpcount = 0;

  do {
    localwarpcount = 1;
    newwarpid      = SM[smid].getCurrentWarp();

    do{
      if (localwarpcount > SM[smid].concurrent_warp_count){
        break;
      }
      SM[smid].move2nextwarp();
      localwarpcount++;
      newwarpid      = SM[smid].getCurrentWarp();
      /*
         if (SM[smid].warp_status[newwarpid] == warpComplete) {
      //if (printnow)
      //MSG ("SM[%d] Warp [%d] : warpComplete",smid, newwarpid);
      } else if (SM[smid].warp_status[newwarpid] == waiting_barrier) {
      //if (printnow)
      //MSG ("SM[%d] Warp [%d] : waiting_barrier",smid, newwarpid);
      } else if (SM[smid].warp_status[newwarpid] == waiting_TSFIFO) {
      //if (printnow)
      //MSG ("SM[%d] Warp [%d] : waiting_TSFIFO",smid, newwarpid);
      } else {
      //if (printnow)
      //MSG ("SM[%d] Warp [%d] : dunno what",smid, newwarpid);
      }
      */ 
    } while ((SM[smid].warp_status[newwarpid] == warpComplete)
        || (SM[smid].warp_status[newwarpid] == waiting_barrier));

    if ((newwarpid == warpid) && (SM[smid].warp_status[newwarpid] != warpComplete)) {
      //if (printnow)
      //IS (MSG("New warp switched to is the same as the old one"));
      warpfound = true;
    } else if (localwarpcount <= SM[smid].concurrent_warp_count){
      //if (printnow)
      //IS (MSG("1: SM[%d] : Switching from warp %d to warp %d ",(int)smid, warpid, newwarpid));
      warpfound = true;
    } else {
      //if (printnow)
      //IS (MSG("No warp to switch to, try to switch to a warp stuck on a barrier!"));
      warpfound = false;
    }

    if (warpfound == false) {
      /********** CHECKME ***********/
      localwarpcount = 1;
      newwarpid      = SM[smid].getCurrentWarp();

      do{
        if (localwarpcount > SM[smid].concurrent_warp_count){
          break;
        }
        SM[smid].move2nextwarp();
        localwarpcount++;
        newwarpid = SM[smid].getCurrentWarp();
        /*
           if (SM[smid].warp_status[newwarpid] == warpComplete) {
        //MSG ("SM[%d] Warp [%d] : warpComplete",smid, newwarpid);
        } else if (SM[smid].warp_status[newwarpid] == waiting_barrier) {
        //MSG ("SM[%d] Warp [%d] : waiting_barrier",smid, newwarpid);
        } else if (SM[smid].warp_status[newwarpid] == waiting_TSFIFO) {
        //MSG ("SM[%d] Warp [%d] : waiting_TSFIFO",smid, newwarpid);
        } else {
        //MSG ("SM[%d] Warp [%d] : dunno what",smid, newwarpid);
        }
        */

      } while (SM[smid].warp_status[newwarpid] == warpComplete);

      if ((newwarpid == warpid) && (SM[smid].warp_status[newwarpid] != warpComplete)) {
        //IS (MSG("New warp switched to is the same as the old one"));
        warpfound = true;
      } else if (localwarpcount <= SM[smid].concurrent_warp_count){
        //IS (MSG("2: SM[%d] : Switching from warp %d to warp %d ",(int)smid, warpid, newwarpid));
        warpfound = true;
      } else {
        //IS (MSG("No warp to switch to !"));
        warpfound = false;
      }

      /********** CHECKME ***********/

      if (warpfound == false) {
        //All warps in the current warpset are complete. So move to the next warpSet
        nomoreWarpSets = (SM[smid].move2nextwarpset() == false);
        //printnow = true;
        if (nomoreWarpSets){ 
          // If you cannot move to the next warpset, then that 
          // means that you were called  from the last warp in 
          // the last warpset
          return false;
        } else {
          newwarpid = SM[smid].getCurrentWarp();
          if (SM[smid].warp_status[newwarpid] != warpComplete){ //Has to be true.
            warpfound = true; 
          } else {
            I(0);
            return false;
          }
        } 
      }
    }
  } while (!warpfound);

  // Resume all the threads in the new warp
  newwarpid = SM[smid].getCurrentWarp();
  //MSG("SM[%d]: Unpausing threads in new warp %d",smid, newwarpid);
  for (uint32_t pe_id = 0; pe_id < numSP; pe_id++){
    if (newwarpid < SM[smid].threads_per_pe[pe_id].size()){
      active_thread = SM[smid].threads_per_pe[pe_id][newwarpid];
      if (likely(active_thread >= 0)){
        h_trace[active_thread*traceSize+4]=0; //Resume 
      }
    }
  }
  return true;
}

bool GPUThreadManager::decode_trace(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid){
  static int conta = 0;
  bool continue_samestate;
  if (rabbit_threads_executing == false){
    //TIMING
    if (conta > 100){
      IS(fprintf(stderr,"T"));
      conta = 0;
    }else {
      conta++;
    }
    continue_samestate = Timing(gsampler, h_trace_qemu, env,qemuid);
    if (!continue_samestate){
      reset_nextRabbitBB(h_trace_qemu);
      rabbit_threads_executing = true;
    } else {
      reset_nextTimingBB(h_trace_qemu);
    }

    if (maxSamplingThreads < 0){ // all Timing
      if (GPUFlowsAlive > 0){
        return true;
      } else {
        return false; //Kernel execution complete
      }
    } 
    return true;
  } else {
    //RABBIT
    if (conta < 100){
      IS(fprintf(stderr,"R"));
      conta++;
    }else {
      conta = 0;
    }

    bool rabbitdone = Rabbit(gsampler,h_trace_qemu,env,qemuid);
    if (rabbitdone == true){
      //MSG("*************************************************** \n KERNEL DONE \n ****************************************************");
      return false; //Kernel execution complete
    } else {
      return true;
    }
  }

}

void GPUThreadManager::reset_nextTimingBB(uint32_t* h_trace){
  for(uint32_t smid = 0; smid < numSM; smid++){
    if (canTurnOffTiming(smid)) {
      SM[smid].smstatus = SMcomplete;
    } else {
      if (SM[smid].totalthreadsinSM > 0){
        uint32_t warpid = SM[smid].getCurrentWarp();
        for (uint32_t pe_id = 0; pe_id < numSP; pe_id++){
          if (warpid < SM[smid].threads_per_pe[pe_id].size()){
            if ((SM[smid].thread_status_per_pe[pe_id][warpid] != execution_done) && 
                (SM[smid].thread_status_per_pe[pe_id][warpid] != invalid))
            {
              SM[smid].thread_status_per_pe[pe_id][warpid]      = running;
              SM[smid].thread_instoffset_per_pe[pe_id][warpid]  = 0;
              SM[smid].thread_traceoffset_per_pe[pe_id][warpid] = 0;
            }
          }
        }
        SM[smid].warp_status[warpid] = traceReady;
        SM[smid].warp_last_active_pe[warpid] = 0;

        if (SM[smid].smstatus != SMcomplete)
          SM[smid].smstatus = SMalive;
      }
    }

  } //End of for each SM loop.
}

void GPUThreadManager::reset_nextRabbitBB(uint32_t* h_trace){
  //Pause the timing threads, //Unpause the Rabbit threads
  GPUFlowsAlive = 0;

  for(uint32_t smid = 0; smid < numSM; smid++){
    if (SM[smid].totalthreadsinSM > 0){

      if (canTurnOff(smid)) {
        SM[smid].smstatus = SMcomplete;
      } else {
        SM[smid].smstatus = SMalive;
      }

      if (SM[smid].totalthreadsinSM > SM[smid].timingthreadsComplete) {
        GPUFlowsAlive++;
        GPUFlowStatus[smid] = true;
      } else {
        GPUFlowStatus[smid] = false;
      }

      for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
        //Pause the timing
        for (uint64_t warpid = 0; warpid < SM[smid].timing_warps;warpid++) {
          if (warpid < SM[smid].threads_per_pe[pe_id].size()){
            if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)){
              SM[smid].thread_instoffset_per_pe[pe_id][warpid] = 0; 
              SM[smid].thread_traceoffset_per_pe[pe_id][warpid] = 0; 
              SM[smid].thread_status_per_pe[pe_id][warpid] = running;
              h_trace[(SM[smid].threads_per_pe[pe_id][warpid])*traceSize+4] = 1; // pause
            }
          }
        }

        //Unpause the rabbit
        for (uint64_t warpid = SM[smid].timing_warps; warpid < SM[smid].warp_status.size();warpid++) {
          if (warpid < SM[smid].threads_per_pe[pe_id].size()){
            if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)){
              SM[smid].thread_instoffset_per_pe[pe_id][warpid] = 0; //Doesn't really matter
              SM[smid].thread_traceoffset_per_pe[pe_id][warpid] = 0; //Doesn't really matter
              SM[smid].thread_status_per_pe[pe_id][warpid] = running;
              h_trace[(SM[smid].threads_per_pe[pe_id][warpid])*traceSize+4] = 0; // Unpause
              h_trace[(SM[smid].threads_per_pe[pe_id][warpid])*traceSize+2] = 32767 ; // Runahead (this is a large number for a single thread) 
              h_trace[(SM[smid].threads_per_pe[pe_id][warpid])*traceSize+9] = 0 ; // Runahead (this is a large number for a single thread) 
            }
          }
        }
      }
      //SM[smid].warpchange = false;
      //SM[smid].warprollover = false;
    } else {
      SM[smid].smstatus = SMcomplete;
    }
  }
}

void GPUThreadManager::reset_nextkernel(){

  for(uint32_t smid = 0; smid < numSM; smid++){
    if (SM[smid].totalthreadsinSM > 0){

      SM[smid].timingthreadsComplete = 0;
      SM[smid].rabbitthreadsComplete = 0;
      SM[smid].resetCurrentWarp();

      uint64_t warpid = SM[smid].getCurrentWarp();
      ///*
      for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
        for (warpid = 0; warpid<SM[smid].warp_status.size(); warpid++){
          if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)){
            SM[smid].warp_status.at(warpid) = traceReady;
            SM[smid].warp_last_active_pe.at(warpid) = 0;
            SM[smid].warp_complete.at(warpid) = 0;
            SM[smid].warp_blocked.at(warpid) = 0;
            SM[smid].thread_instoffset_per_pe[pe_id][warpid] = 0;
            SM[smid].thread_traceoffset_per_pe[pe_id][warpid] = 0;
            SM[smid].thread_status_per_pe[pe_id][warpid] = running;
          }
        }
      }
      //*/
      //SM[smid].warpchange = false;
      //SM[smid].warprollover = false;
      SM[smid].cycledthru_currentwarpset = false;
      SM[smid].smstatus = SMalive;
      SM[smid].setActive();
    } else {
      SM[smid].cycledthru_currentwarpset = true;
      SM[smid].smstatus = SMcomplete;
    }
  }
}

void GPUThreadManager::decodeSignature(uint32_t* h_trace_qemu){
  ofstream outdata; // outdata is like cin
  uint64_t threadid; // loop index

  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  string filename = kernel->kernel_name; 
  filename.append(".csv");
  outdata.open(filename.c_str()); // opens the file

  if( !outdata ) { // file couldn't be opened
    cerr << "Error: file could not be opened" << endl;
    exit(1);
  }


  for (threadid=0; threadid<numThreads; threadid++)
    outdata << h_trace_qemu[threadid*traceSize+6]<<"," //threadid
      << h_trace_qemu[threadid*traceSize+5]<<"," //blockid
      << h_trace_qemu[threadid*traceSize+7]<<"," //smid
      << h_trace_qemu[threadid*traceSize+8]<<"," //warpid
      << h_trace_qemu[threadid*traceSize+10]<<"," //Branch Count
      << h_trace_qemu[threadid*traceSize+11]<<"," //Taken Branch Count
      << endl;

  outdata.close();

}


void GPUThreadManager::putinfile(uint64_t inst){
  ofstream outdata; // outdata is like cin
  outdata.open("starthere.csv"); // opens the file

  if( !outdata ) { // file couldn't be opened
    cerr << "Error: file could not be opened" << endl;
    exit(1);
  }
  
  outdata <<inst<< endl;

  outdata.close();
  //exit(1);

}

uint32_t GPUThreadManager::getMinNumSM() {
  return maxSamplingThreads/(blockSize*max_blocks_sm);
}
