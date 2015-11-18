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

#include "GPUThreadManager.h"
#include <inttypes.h>

#define TRACKGPU_MEMADDR 0
bool     printnow = false;
extern   uint64_t numThreads;
extern   int64_t  maxSamplingThreads;

extern   uint64_t GPUFlowsAlive;
extern   bool*    GPUFlowStatus;

extern   uint64_t oldqemuid;
extern   uint64_t newqemuid;

extern   uint32_t traceSize;
extern   bool     unifiedCPUGPUmem;
extern   bool*    istsfifoBlocked;
extern   bool     rabbit_threads_executing;
extern   ThreadBlockStatus *block;
extern   div_type branch_div_mech;

GStatsCntr*  GPUThreadManager::totalPTXinst_T;
GStatsCntr*  GPUThreadManager::totalPTXinst_R;
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Shared;
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Global;
GStatsCntr*  GPUThreadManager::totalPTXmemAccess_Rest;


extern uint64_t GPUReader_translate_d2h(AddrType addr_ptr);
extern uint64_t GPUReader_translate_shared(AddrType addr_ptr, uint32_t blockid, uint32_t warpid);
extern uint32_t roundUp(uint32_t numToRound, uint32_t multiple);
extern uint32_t roundDown(uint32_t numToRound, uint32_t multiple);
extern uint64_t loopcounter;
extern uint64_t relaunchcounter;
extern uint64_t memref_interval;
extern bool     MIMDmode;

bool sort_by_refcount(const sort_map& a ,const sort_map& b)
{/*{{{*/
  return (a.line->refcount < b.line->refcount) ;
}
/*}}}*/

GPUThreadManager::GPUThreadManager(uint64_t numFlows, uint64_t numPEs)
{/*{{{*/
  loopcounter = 0;
  totalPTXinst_T = new GStatsCntr("PTXStats:totalPTXinst_T");
  totalPTXinst_R = new GStatsCntr("PTXStats:totalPTXinst_R");
  totalPTXmemAccess_Shared = new GStatsCntr("PTXStats:totalmemAccess_Shared");
  totalPTXmemAccess_Global = new GStatsCntr("PTXStats:totalmemAccess_Global");
  totalPTXmemAccess_Rest = new GStatsCntr("PTXStats:totalmemAccess_Rest");

  maxSMID = 0;
  if (MIMDmode) {
    numSM = (numFlows / numPEs) ;
  } else {
    numSM = numFlows;
  }

  numSP = numPEs;
  SM = new esescSM[numSM];
  GPUFlowStatus = new bool[numSM];
  SM_fid_revmap = NULL;

  for (uint32_t smid = 0; smid < numSM; smid++) {
    SM[smid].setnumPE(numSP);
  }


#if TRACKGPU_MEMADDR
  memdata.open("memaccesses_pe.csv"); // opens the file

  if ( !memdata ) { // file couldn't be opened
    cerr << "Error: file could not be opened" << endl;
    exit(1);
  }
#endif

  L1cache.setEntries(192);
  L1cache.setlinesize(32);
}/*}}}*/

GPUThreadManager::~GPUThreadManager()
{/*{{{*/
#if TRACKGPU_MEMADDR
  memdata.close();
#endif
  delete []SM;
  delete SM_fid_revmap;
  delete istsfifoBlocked;
}/*}}}*/

inline uint32_t GPUThreadManager::calcOccupancy()
{/*{{{*/
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
}/*}}}*/

inline void GPUThreadManager::pauseAllThreads(uint32_t* h_trace, bool init, bool isRabbit)
{/*{{{*/
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;


  if (init) {
    if (isRabbit) {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //SkipInst to a big number
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
    else {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
  }
  else {
    if (isRabbit) {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+2]=32760; //SkipInst to a big number
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
    else {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+4]=1;     //Pause = 1 (Pause)
      }
    }
  }
}/*}}}*/

inline void GPUThreadManager::resumeAllThreads(uint32_t* h_trace, bool init, bool isRabbit)
{/*{{{*/

  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;


  if (init) {
    if (isRabbit) {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume);
      }
    }
    else {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume);
      }
    }
  }
  else {
    if (isRabbit) {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
    else {
      for (active_thread = 0; active_thread < (int64_t)numThreads; active_thread++) {
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  }
}/*}}}*/

inline void GPUThreadManager::pauseThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit)
{/*{{{*/
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;

  if (init) {
    if (isRabbit) {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+0]=1; //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
    else {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+0]=1; //First BB to 1
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
  }
  else {
    if (isRabbit) {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
    else {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+4]=1; //Pause = 1 (Pause)
      }
    }
  }
}/*}}}*/

inline void GPUThreadManager::resumeThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit)
{/*{{{*/
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  int64_t active_thread = 0;
  if (init) {
    if (isRabbit) {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    } else  {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+0]=1;     //First BB to 1
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  }
  else {
    if (isRabbit) {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+2]=32760; //Skip Inst to a large number
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    } else  {
      for (active_thread = startRange; active_thread < (int64_t)endRange; active_thread++) {
        h_trace[active_thread*traceSize+4]=0;     //Pause = 0 (Resume)
      }
    }
  }
}/*}}}*/

inline void GPUThreadManager::resumeSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit)
{/*{{{*/
  int64_t active_thread = 0;
  uint64_t maxcount = numSP;

  if (maxcount > SM[smid].totalthreadsinSM) {
    maxcount = SM[smid].totalthreadsinSM;
  }


  if (init) {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+0]=1;
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+0]=1;
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    }
  } else {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+4]=0; //Resume
        }
      }
    }
  }

}/*}}}*/

inline void GPUThreadManager::pauseSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit)
{/*{{{*/
  int64_t active_thread = 0;
  uint64_t maxcount = numSP;

  if (maxcount > SM[smid].totalthreadsinSM) {
    maxcount = SM[smid].totalthreadsinSM;
  }

  if (init) {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+0]=1;
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=1; //Pause
        }
      }
    } else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+0]=1;
          h_trace[active_thread*traceSize+4]=1; //Pause
        }
      }
    }
  } else {
    if (isRabbit) {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        if (likely(active_thread >= 0)) {
          h_trace[active_thread*traceSize+2]=32760;
          h_trace[active_thread*traceSize+4]=1; //Pause
        }
      }
    }
    else {
      for (uint64_t pe_id = 0; pe_id < maxcount; pe_id++) {
        active_thread = SM[smid].threads_per_pe[pe_id][warpid];
        h_trace[active_thread*traceSize+4] = 1; //Pause
      }
    }
  }

}/*}}}*/

void GPUThreadManager::dump()
{/*{{{*/
#if 0
  for (uint32_t smid = 0; smid < numSM; smid++) {
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
  }
  else {
    uint32_t numblks = numThreads/blockSize;
    if (numThreads%blockSize > 0)
      numblks++;
  }

  for (uint32_t i =0; i < numblks; i++) {
    MSG(" Block ID [%d], Status = [%d], threads_at_barrier = [%d]", (block[i].id), block[i].status,block[i].threads_at_barrier);
  }
#endif
#endif
}/*}}}*/

inline void GPUThreadManager::clearAll()
{/*{{{*/

  for (uint32_t localsmid =0; localsmid < numSM; localsmid++) {
    SM[localsmid].clearAll();
  }

}/*}}}*/

void GPUThreadManager::reOrganize()
{/*{{{*/
  clearAll();

  static bool first = true;
  if (first) {
    for (uint32_t i=0; i<getNumKernels(); i++)
      kernelsNumSM.push_back(numSM);
  }
  first = false;
  setNumSM(kernelsId[current_CUDA_kernel]);

  for (uint32_t smid = 0; smid < numSM; smid++) {
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

  for (uint64_t h =0; h <numSM; h++) {
    for (uint64_t i =0; i <numSP; i++) {
      SM[h].threads_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_instoffset_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_traceoffset_per_pe[i].reserve(numThreads/numSP/numSM);
      SM[h].thread_status_per_pe[i].reserve(numThreads/numSP/numSM);
    }
  }

  // Add threads to each SM
  uint64_t tmpnum = 0;
  while (tmpnum < numThreads) {
    uint64_t blockid = tmpnum/(blocks_per_sm*blockSize);
    uint32_t losmid    = blockid % numSM;
    if ((blocks_per_sm*blockSize) > (numThreads-tmpnum))
      SM[losmid].addThreads(tmpnum,(numThreads - tmpnum),blockSize);
    else {
      SM[losmid].addThreads(tmpnum,(blocks_per_sm*blockSize),blockSize);
    }
    tmpnum += blocks_per_sm*blockSize;
  }

  for (uint32_t smid = 0; smid < numSM; smid++) {

    if (SM[smid].totalthreadsinSM > 0) {
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

      warp_div_data div_data;
      uint32_t warpcount = 0;
      do {

        SM[smid].warp_status.push_back(traceReady);
        SM[smid].warp_last_active_pe.push_back(0);
        SM[smid].warp_complete.push_back(0);
        SM[smid].warp_blocked.push_back(0);
        SM[smid].warp_block_map.push_back((SM[smid].threads_per_pe[0][nWarps - num_warps])/blockSize);
        SM[smid].active_sps.push_back(0);
        SM[smid].warp_divergence.push_back(div_data);
        SM[smid].divergence_data_valid.push_back(false);
        SM[smid].unmasked_bb.push_back(0);


        if (blockSize < numSP) {
          SM[smid].warp_threadcount.push_back(blockSize);
        }
        else {
          if (mybs > numSP) {
            SM[smid].warp_threadcount.push_back(numSP);
            mybs = mybs - numSP;
          }
          else {
            SM[smid].warp_threadcount.push_back(mybs);
            mybs = blockSize;
          }
        }

        num_warps--;
        warpcount++;
      } while (num_warps > 0);

      MSG("\nSM[%d] has %d warps",smid, warpcount);

      num_warps = nWarps;
      do {
        SM[smid].warp_last_active_pe[num_warps-1] = 0;
        num_warps--;
      }
      while (num_warps > 0);

      SM[smid].cycledthru_currentwarpset =  false;

    }
    else if (SM[smid].totalthreadsinSM == 0) {
      SM[smid].setInactive();
      SM[smid].cycledthru_currentwarpset = true;
    }

    SM[smid].resetCurrentWarp();
    SM[smid].resetCurrentWarpSet();
  }// For each SM loop

  //Update the concurrent warps per SM. Depending on how many threads are there, this number might be different.
  IS(cout << "concurrent warps per sm = " <<conc_warps<<endl);
  for (uint32_t smid = 0; smid < numSM; smid++) {
    SM[smid].updateCurrentWarps(conc_warps,max_warps_sm,th_sb_sw);
  }


  /* Set maxSamplingThreads */
  maxSamplingThreads = gsampler->getThreads2Simulate();

  if (maxSamplingThreads > 0) {

    IS(cout <<"maxSamplingThreads defined as "<< maxSamplingThreads<<endl);
    maxSamplingThreads = roundUp(maxSamplingThreads, th_sb_sw);
    IS(cout <<"maxSamplingThreads rounded to a multiple of "<<conc_warps*warpsize<< " and is now  " <<  maxSamplingThreads<<endl);

    if ((int64_t) numThreads < maxSamplingThreads) {
      maxSamplingThreads = -1;
    }

  }

  IS(cout<<endl<<endl<<"maxSamplingThreads = "<< maxSamplingThreads<<endl);

  if (maxSamplingThreads != 0) {
    uint64_t total_timing_threads  = maxSamplingThreads;
    if ((int64_t) total_timing_threads < 0 ) {
      IS(cout <<"maxSamplingThreads defined as "<< maxSamplingThreads <<", sampling all the threads" <<endl);
      total_timing_threads = numThreads;
    } else if ((int64_t) total_timing_threads > numThreads) {
      IS(cout <<"maxSamplingThreads defined as "<< maxSamplingThreads <<",which is more than the number of threads in the application ( " << numThreads <<" ), sampling all the threads... " <<endl);
      total_timing_threads = numThreads;
    }
    uint32_t smid = 0;

    do {
      SM[smid].addTimingThreads(&total_timing_threads, warpsize);
      smid++;
      if (smid == numSM) {
        smid = 0;
      }
    }
    while (total_timing_threads > 0);
  }

  for (uint32_t smid = 0; smid < numSM; smid++) {
    SM[smid].warpsets = SM[smid].timing_warps/conc_warps;
    if (SM[smid].timing_warps%conc_warps != 0) {
      SM[smid].warpsets++;
    }
  }


  int64_t active_sp_in_warp      = 0;
  for (uint32_t smid = 0; smid < numSM; smid++) {
    uint64_t warpid = 0;
    while (warpid < SM[smid].warp_status.size()) {
      active_sp_in_warp      = 0;
      for (uint32_t peid = 0; peid < numSP; peid++) {
        if (warpid < SM[smid].threads_per_pe[peid].size() && (SM[smid].thread_status_per_pe[peid][warpid] != invalid)) {
          active_sp_in_warp++;
        }
      }
      //IS(MSG("There are %d active SPs in this warp", (int) active_sp_in_warp));
      SM[smid].active_sps[warpid] = active_sp_in_warp;
      warpid++;
    }
  }

  // Initialize the barrier status data structure
  if (block != NULL) {
    delete [] block;
  }

  uint32_t numblks = 0;
  if (maxSamplingThreads >= 0) {
    numblks = maxSamplingThreads/blockSize;
    if (maxSamplingThreads%blockSize!= 0)
      numblks++;
  }
  else {
    numblks = numThreads/blockSize;
    if (numThreads%blockSize != 0)
      numblks++;
  }

  block = new ThreadBlockStatus[numblks];

  for (uint32_t i = 0; i < numblks ; i++) {
    block[i].init(i);
  }

  for (uint64_t sm = 0; sm < numSM; sm++) {
    uint64_t wid = 0;
    uint64_t blk = 0;
    while (wid < SM[sm].timing_warps) {
      blk = SM[sm].warp_block_map[wid];
      block[blk].addWarp(wid);
      wid++;
    }
  }

  newlaunch = true;
  IS(dump());
  //exit(-1);
  ////////////////
}/*}}}*/

void GPUThreadManager::link(uint64_t glosmid, uint64_t smid)
{/*{{{*/
  I(smid < numSM);
  SM[smid].setglobal_smid(glosmid);

  //MSG("SM[%lld] configured as GlobalFlow[%lld]",smid, glosmid);

  if (MIMDmode) {
    maxSMID = glosmid + SM[smid].getnumSP();
    //MSG("maxSMID configured as [%lld]", maxSMID);
  } else {
    if (glosmid > maxSMID){
      maxSMID = glosmid;
      //MSG("maxSMID configured as [%lld]", maxSMID);
    } else {
      I(0);
    }
  }

}/*}}}*/

void GPUThreadManager::createRevMap()
{/*{{{*/
  //How do we check if a gloid actually is a valid GPU flow? :-)
  I(SM_fid_revmap == NULL);

  if (MIMDmode) {
    SM_fid_revmap = new uint64_t[maxSMID+1];
    for (uint64_t i = 0; i < numSM; i++) {
      for (uint64_t j = 0; j < SM[i].getnumSP(); j++) {
        SM_fid_revmap[SM[i].getglobal_smid(j)] = i;
        //MSG("Global Flow [%lld] Maps to SM [%lld] PE[%lld]", SM[i].getglobal_smid(j), i,j);
      }
    }
  } else {
    SM_fid_revmap = new uint64_t[maxSMID+1];
    for (uint64_t i = 0; i < numSM; i++) {
      SM_fid_revmap[SM[i].getglobal_smid()] = i;
    }
  }

  istsfifoBlocked = new bool[maxSMID+1];
  for (uint64_t k = 0; k < maxSMID+1; k++) {
    istsfifoBlocked[k] = false;
  }


}/*}}}*/

void GPUThreadManager::init_trace(uint32_t* h_trace)
{/*{{{*/
  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  bool init   = true;
  bool rabbit = true;

  //Clear the trace arrays
  memset((void *) h_trace,0,numThreads*traceSize*4);

  if (maxSamplingThreads != 0) {

    //FEW OR ALL TIMING
    pauseAllThreads(h_trace,init,(!rabbit));

    for (uint32_t smid = 0; smid < numSM; smid++) {
      if (SM[smid].totalthreadsinSM > 0) {
        IS(fprintf(stderr, "\n\nSM [%u] has %u warps in timing and %u warps fastforwarded",smid, SM[smid].timing_warps, SM[smid].warp_status.size()-SM[smid].timing_warps));
        uint64_t warpid = 0;
        //SM[smid].smstatus = SMTiming;
        SM[smid].smstatus = SMalive;

        if (SM[smid].timing_warps > 0) {
          SM[smid].resetCurrentWarpSet();
          warpid = SM[smid].getCurrentWarp();
          I(warpid<SM[smid].timing_warps);
          resumeSelectivethreads(h_trace,smid,warpid,init,(!rabbit)); //Unpause the timingthreads (one warp only)
        } else {
          //FIXME: Anything to be done if the SM has no timing threads???
        }

        for (warpid = SM[smid].timing_warps; warpid<SM[smid].warp_status.size() ; warpid++) {
          pauseSelectivethreads(h_trace,smid,warpid,init,rabbit);
        }
      } else {
        fprintf(stderr, "\nSM[%d] : No Threads\n\n",smid);
        SM[smid].smstatus = SMcomplete;
      }
    }
    rabbit_threads_executing = false;
  }
  else {
    //ALL RABBIT
    resumeAllThreads(h_trace,init,rabbit);
    rabbit_threads_executing = true;
    for (uint32_t smid = 0; smid < numSM; smid++) {
      SM[smid].smstatus = SMalive;
    }
  }

  IS(fprintf(stderr, "\n\n\n"));
  //exit(-1);
  ////////////////
}/*}}}*/

bool GPUThreadManager::Rabbit(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid)
{/*{{{*/
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


  for (uint32_t smid = 0; smid < numSM; smid++) {
    if (SM[smid].totalthreadsinSM > 0) {

      if (SM[smid].smstatus == SMalive) {

        for (uint64_t warpid = SM[smid].timing_warps; warpid<SM[smid].warp_status.size() ; warpid++) {

          for (uint64_t pe_id = 0; pe_id < numSP; pe_id++) {

            // data            = pe_id;
            if (SM[smid].thread_status_per_pe[pe_id][warpid] == running) {
              active_thread     = SM[smid].threads_per_pe[pe_id][warpid];
              I(active_thread >= 0);
              trace_nextbbid    = h_trace_qemu[(active_thread*traceSize)+0];
              trace_currentbbid = h_trace_qemu[(active_thread*traceSize)+1];
              trace_ffwd_insts  = h_trace_qemu[(active_thread*traceSize)+9];

              if (trace_currentbbid != 0) {

                //MSG("TID: %llu Current BBID = %d, Next BBID = %d",active_thread,trace_currentbbid,trace_nextbbid);
                gloicount += trace_ffwd_insts;

                if (trace_nextbbid == 0) {
                  SM[smid].thread_status_per_pe[pe_id][warpid] = execution_done; // OK
                  h_trace_qemu[active_thread*traceSize+4]      = 1;              // Pause
                  h_trace_qemu[(active_thread*traceSize)+1]    = 0;              // Mark the currentbbid to 0 so that u don't keep inserting the instruction
                  SM[smid].rabbitthreadsComplete++;
                  SM[smid].warp_complete[warpid]++;
                }
              }
            }
            else {
              //IS(MSG("Why is this happening"));
            }
          }
        }
      }
    }

    if (SM[smid].totalthreadsinSM > 0) {
      //if (maxSamplingThreads == 0){ // all Rabbit
      if (canTurnOff(smid) && GPUFlowStatus[smid]) {
        SM[smid].setInactive();
        if (GPUFlowStatus[smid]) {
          GPUFlowStatus[smid] = false;
          GPUFlowsAlive--;
        }
      }
      //}
    }//End of if (SM[smid].totalthreadsinSM > 0)
  }

  //gsampler->queue(insn,pc,addr,data,glofid,op,gloicount,env); //None of the arguments except icount, env really matter.
  gsampler->queue(insn,pc,addr,glofid,op,gloicount,env);


  totalPTXinst_R->add(gloicount);
  //IS(cerr <<"Adding " <<gloicount<< " instructions starting from Basic Block "<<h_trace_qemu[1024*46+1]<< " going to basicBlock " <<h_trace_qemu[1024*46+0]<<endl);

  //gsampler->stop(1); // We force weight 1 to avoid loss of any stats remaining from timing.
  gsampler->stop(); // We force weight 1 to avoid loss of any stats remaining from timing.

  if (GPUFlowsAlive == 0)
    return true;
  else
    return false;
}/*}}}*/

bool GPUThreadManager::Timing(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid)
{/*{{{*/
  bool status = false;
  memref_interval = 0;
  if (MIMDmode){
    newlaunch = true;
    status      =  Timing_MIMD(gsampler, h_trace_qemu, env, qemuid);
  } else {
    if (branch_div_mech == serial){
      status      =  Timing_serial_branchdiv(gsampler, h_trace_qemu, env, qemuid);
    }
    /*
       else if (branch_div_mech == post_dom){
       status      =  Timing_postdom_branchdiv(gsampler, h_trace_qemu, env, qemuid);
       } else if (branch_div_mech == sbi ){
       status      =  Timing_sbi_branchdiv(gsampler, h_trace_qemu, env, qemuid);
       } else if (branch_div_mech == sbi_swi){
       status      =  Timing_sbi_swi_branchdiv(gsampler, h_trace_qemu, env, qemuid);
       }
       */
    else if (branch_div_mech == original){
      status      =  Timing_Original(gsampler, h_trace_qemu, env, qemuid);
    } else {
      I(0);
      //status      =  Timing_serial_branchdiv(gsampler, h_trace_qemu, env, qemuid);
    }
  }
  return status;
}/*}}}*/

void GPUThreadManager::possiblyturnOFFSM(uint32_t smid)
{/*{{{*/
  //MSG("Checking to see if I can turn off SM [%d]", (int) smid);
  if (canTurnOffTiming(smid) && GPUFlowStatus[smid]) {
    IS(MSG ("SM %d Timing DONE, Pausing global flow %d ",(int) smid, (int)SM[smid].getglobal_smid()));
    //gsampler->pauseThread(glofid);
    GPUFlowStatus[smid] = false;
    //MSG("Yes I can turn it off" );
    GPUFlowsAlive--;
  }
}/*}}}*/

bool GPUThreadManager::switch2nextwarp(uint32_t smid, uint32_t* h_trace)
{/*{{{*/
  int64_t active_thread;
  uint32_t warpid = SM[smid].getCurrentWarp();

  // MSG("Warp %u : Pausing threads in the old warp",warpid);
  // Pause all the threads in the current warp
  for (uint32_t pe_id = 0; pe_id < numSP; pe_id++) {
    if (warpid < SM[smid].threads_per_pe[pe_id].size()) {
      active_thread                      = SM[smid].threads_per_pe[pe_id][warpid];
      if (likely(active_thread >= 0)) {
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

    do {
      if (localwarpcount > SM[smid].concurrent_warp_count) {
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
    }
    while ((SM[smid].warp_status[newwarpid] == warpComplete)
        || (SM[smid].warp_status[newwarpid] == waiting_barrier));

    if ((newwarpid == warpid) && (SM[smid].warp_status[newwarpid] != warpComplete)) {
      //if (printnow)
      //IS (MSG("New warp switched to is the same as the old one"));
      warpfound = true;
    }
    else if (localwarpcount <= SM[smid].concurrent_warp_count) {
      //if (printnow)
      //IS (MSG("1: SM[%d] : Switching from warp %d to warp %d ",(int)smid, warpid, newwarpid));
      warpfound = true;
    }
    else {
      //if (printnow)
      //IS (MSG("No warp to switch to, try to switch to a warp stuck on a barrier!"));
      warpfound = false;
    }

    if (warpfound == false) {
      /********** CHECKME ***********/
      localwarpcount = 1;
      newwarpid      = SM[smid].getCurrentWarp();

      do {
        if (localwarpcount > SM[smid].concurrent_warp_count) {
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

      }
      while (SM[smid].warp_status[newwarpid] == warpComplete);

      if ((newwarpid == warpid) && (SM[smid].warp_status[newwarpid] != warpComplete)) {
        //IS (MSG("New warp switched to is the same as the old one"));
        warpfound = true;
      }
      else if (localwarpcount <= SM[smid].concurrent_warp_count) {
        //IS (MSG("2: SM[%d] : Switching from warp %d to warp %d ",(int)smid, warpid, newwarpid));
        warpfound = true;
      }
      else {
        //IS (MSG("No warp to switch to !"));
        warpfound = false;
      }

      /********** CHECKME ***********/

      if (warpfound == false) {
        //All warps in the current warpset are complete. So move to the next warpSet
        nomoreWarpSets = (SM[smid].move2nextwarpset() == false);
        //printnow = true;
        if (nomoreWarpSets) {
          // If you cannot move to the next warpset, then that
          // means that you were called  from the last warp in
          // the last warpset
          return false;
        }
        else {
          newwarpid = SM[smid].getCurrentWarp();
          if (SM[smid].warp_status[newwarpid] != warpComplete) { //Has to be true.
            warpfound = true;
          }
          else {
            I(0);
            return false;
          }
        }
      }
    }
  }
  while (!warpfound);

  // Resume all the threads in the new warp
  newwarpid = SM[smid].getCurrentWarp();
  //MSG("Warp %d : SM[%d]: Unpausing threads in new Warp %d", (int) warpid, (int)smid, (int)newwarpid);
  for (uint32_t pe_id = 0; pe_id < numSP; pe_id++) {
    if (newwarpid < SM[smid].threads_per_pe[pe_id].size()) {
      active_thread = SM[smid].threads_per_pe[pe_id][newwarpid];
      if (likely(active_thread >= 0)) {
        if (SM[smid].thread_status_per_pe[pe_id][newwarpid] != paused_on_a_barrier){
          h_trace[active_thread*traceSize+4]=0; //Resume
        } else {
          //MSG("New Warp %u : thread %lld is a barrier thread, not unpausing it",newwarpid, active_thread );
        }
      }
    }
  }

  return true;
}/*}}}*/

bool GPUThreadManager::decode_trace(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid)
{/*{{{*/
  static int conta = 0;
  bool continue_samestate;
  if (rabbit_threads_executing == false) {
    //TIMING
    if (conta > 100) {
      IS(fprintf(stderr,"T"));
      conta = 0;
    }
    else {
      conta++;
    }
    continue_samestate = Timing(gsampler, h_trace_qemu, env,qemuid);
    if (!continue_samestate) {
      reset_nextRabbitBB(h_trace_qemu);
      rabbit_threads_executing = true;
    }
    else {
      reset_nextTimingBB(h_trace_qemu);
    }

    if (maxSamplingThreads < 0) { // all Timing
      if (GPUFlowsAlive > 0) {
        return true;
      }
      else {
        return false; //Kernel execution complete
      }
    }
    return true;
  }
  else {
    //RABBIT
    if (conta < 100) {
      IS(fprintf(stderr,"R"));
      conta++;
    }
    else {
      conta = 0;
    }

    bool rabbitdone = Rabbit(gsampler,h_trace_qemu,env,qemuid);
    if (rabbitdone == true) {
      //MSG("*************************************************** \n KERNEL DONE \n ****************************************************");
      return false; //Kernel execution complete
    }
    else {
      return true;
    }
  }

}/*}}}*/

void GPUThreadManager::reset_nextTimingBB(uint32_t* h_trace)
{/*{{{*/
  for (uint32_t smid = 0; smid < numSM; smid++) {
    if (canTurnOffTiming(smid)) {
      SM[smid].smstatus = SMcomplete;
    }
    else {
      if (SM[smid].totalthreadsinSM > 0) {
        uint32_t warpid = SM[smid].getCurrentWarp();
        for (uint32_t pe_id = 0; pe_id < numSP; pe_id++) {
          if (warpid < SM[smid].threads_per_pe[pe_id].size()) {
            if ((SM[smid].thread_status_per_pe[pe_id][warpid] != execution_done) &&
                (SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)) {
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
}/*}}}*/

void GPUThreadManager::reset_nextRabbitBB(uint32_t* h_trace)
{/*{{{*/
  //Pause the timing threads, //Unpause the Rabbit threads
  GPUFlowsAlive = 0;

  for (uint32_t smid = 0; smid < numSM; smid++) {
    if (SM[smid].totalthreadsinSM > 0) {

      if (canTurnOff(smid)) {
        SM[smid].smstatus = SMcomplete;
      }
      else {
        SM[smid].smstatus = SMalive;
      }

      if (SM[smid].totalthreadsinSM > SM[smid].timingthreadsComplete) {
        GPUFlowsAlive++;
        GPUFlowStatus[smid] = true;
      }
      else {
        GPUFlowStatus[smid] = false;
      }

      for (uint64_t pe_id = 0; pe_id < numSP; pe_id++) {
        //Pause the timing
        for (uint64_t warpid = 0; warpid < SM[smid].timing_warps; warpid++) {
          if (warpid < SM[smid].threads_per_pe[pe_id].size()) {
            if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)) {
              SM[smid].thread_instoffset_per_pe[pe_id][warpid] = 0;
              SM[smid].thread_traceoffset_per_pe[pe_id][warpid] = 0;
              SM[smid].thread_status_per_pe[pe_id][warpid] = running;
              h_trace[(SM[smid].threads_per_pe[pe_id][warpid])*traceSize+4] = 1; // pause
            }
          }
        }

        //Unpause the rabbit
        for (uint64_t warpid = SM[smid].timing_warps; warpid < SM[smid].warp_status.size(); warpid++) {
          if (warpid < SM[smid].threads_per_pe[pe_id].size()) {
            if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)) {
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
    }
    else {
      SM[smid].smstatus = SMcomplete;
    }
  }
}/*}}}*/

void GPUThreadManager::reset_nextkernel()
{/*{{{*/

  for (uint32_t smid = 0; smid < numSM; smid++) {
    if (SM[smid].totalthreadsinSM > 0) {

      SM[smid].timingthreadsComplete = 0;
      SM[smid].rabbitthreadsComplete = 0;
      SM[smid].resetCurrentWarp();

      uint64_t warpid = SM[smid].getCurrentWarp();
      ///*
      for (uint64_t pe_id = 0; pe_id < numSP; pe_id++) {
        for (warpid = 0; warpid<SM[smid].warp_status.size(); warpid++) {
          if (likely(SM[smid].thread_status_per_pe[pe_id][warpid] != invalid)) {
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
    }
    else {
      SM[smid].cycledthru_currentwarpset = true;
      SM[smid].smstatus = SMcomplete;
    }
  }
}/*}}}*/

void GPUThreadManager::decodeSignature(uint32_t* h_trace_qemu)
{/*{{{*/
  ofstream outdata; // outdata is like cin
  uint64_t threadid; // loop index

  CUDAKernel* kernel = kernels[current_CUDA_kernel];
  traceSize = kernel->tracesize;
  string filename = kernel->kernel_name;
  filename.append(".csv");
  outdata.open(filename.c_str()); // opens the file

  if ( !outdata ) { // file couldn't be opened
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

}/*}}}*/

void GPUThreadManager::putinfile(uint64_t inst)
{/*{{{*/
  ofstream outdata; // outdata is like cin
  outdata.open("starthere.csv"); // opens the file

  if ( !outdata ) { // file couldn't be opened
    cerr << "Error: file could not be opened" << endl;
    exit(1);
  }

  outdata <<inst<< endl;

  outdata.close();
  //exit(1);

}
/*}}}*/

uint32_t GPUThreadManager::getMinNumSM() {
  return maxSamplingThreads/(blockSize*max_blocks_sm);/*{{{*/
}/*}}}*/
