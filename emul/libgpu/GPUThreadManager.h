// Contributed by  Alamelu Sankaranarayanan
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

#ifndef GPU_THREADMANAGER_H
#define GPU_THREADMANAGER_H

#include <unistd.h>
#include <stdint.h>
#include <string>
#include <queue>

#include "nanassert.h"
#include "GPUInterface.h"
#include "nanassert.h"
#include "Instruction.h"
#include "CUDAInstruction.h"
#include "EmuSampler.h"
#include "GStats.h"

#include <iostream>
using std::cerr;
using std::endl;
#include <fstream>
using std::ofstream;

extern uint64_t numThreads;
extern uint32_t traceSize;
extern bool istsfifoBlocked;
extern string current_CUDA_kernel;

enum ThreadStatus {
  running = 0,
  paused_on_a_memaccess,
  paused_on_a_barrier,
  block_done,
  execution_done,
  invalid 
};

enum WarpStatus {
  traceReady = 0,
  waiting_TSFIFO,
  waiting_barrier,
  waiting_memory,
  relaunchReady,
  warpComplete
};

enum SMStatus {
  SMalive = 0,
  SMdone,
  SMcomplete,
  SMTiming,
  SMRabbit
};

enum BarrierStatus {
  stop = 0,
  go
};


class EmuSampler;

class ThreadBlockStatus {
  public:
  uint32_t id;
  BarrierStatus status; //Needed?
  uint32_t threads_at_barrier; 
  std::vector < uint32_t > warps;

  ThreadBlockStatus(){
    id                 = 0;
    status             = stop;
    threads_at_barrier = 0;
  }

  void init(uint32_t bid){
    id                 = bid;
    status             = stop;
    threads_at_barrier = 0;
  }

  void incthreadcount(){
    threads_at_barrier++;
  }

  void clearBarrier(){
    status             = go;
    threads_at_barrier = 0;
  }

  BarrierStatus getStatus(){
    return status;
  }

  void addWarp(uint32_t warpid){
    warps.push_back(warpid);
  }
  

  bool checkStatus(uint32_t bsize){
    if (bsize == threads_at_barrier){
      status = go;
      return true;
    } else {
      status = stop;
      return false;
    }

  }

  bool checkThreadCount(uint32_t blocksize, uint32_t complete_threads = 0){
    if (blocksize <= (threads_at_barrier+complete_threads))
      return true;
    return false;
  }

} ;

class esescSM {
  private:

  protected:
    uint64_t global_smid;
    uint64_t currentwarpid;
    uint64_t numSP;
    bool active;

  public:
    std::vector < int64_t >*        threads_per_pe;
    std::vector < int16_t >*        thread_instoffset_per_pe;
    std::vector < int16_t >*        thread_traceoffset_per_pe;
    std::vector < ThreadStatus>*    thread_status_per_pe;
    std::vector < WarpStatus >      warp_status;
    std::vector < uint32_t >        active_sps;
    std::vector < uint32_t >        blockid;
    std::vector < uint32_t >        warp_last_active_pe;
    std::vector < uint32_t >        warp_complete;
    std::vector < uint32_t >        warp_blocked;
    std::vector < uint32_t >        warp_block_map;
    std::vector < uint32_t >        warp_threadcount;

    bool warpchange;
    bool warprollover;
    bool cycledthru_currentwarpset;

    uint64_t totalthreadsinSM;
    uint64_t rabbitthreadsComplete;
    uint64_t timingthreadsComplete;

    SMStatus smstatus;
    uint32_t timing_warps;        // warps whose threads are all timingmode

    // The following have been added to support memory warping.
    
    uint32_t* currently_executing_warps;//An array of warps that will be executed to completion
    uint32_t  concurrent_warp_count;    //Size of currently_executing_warps
    uint32_t  nextwarp2add;             //The next warp that will be added to the pool of currently executing_warps;

    uint32_t  warpsets;
    uint32_t  currentwarpset;

    uint32_t  th_sb_sw;

    esescSM () {
      resetCurrentWarp();
      active                    = false;
      timingthreadsComplete     = 0;
      rabbitthreadsComplete     = 0;
      warpchange                = false;
      warprollover              = true; 
      timing_warps              = 0;
      cycledthru_currentwarpset    = false;
      currently_executing_warps = NULL;
      concurrent_warp_count     = 0;
      nextwarp2add              = 0;
      warpsets                  = 0;  
      th_sb_sw                  = 0;
      resetCurrentWarpSet();
    } 

    ~esescSM (){ 
      if (currently_executing_warps != NULL)
        delete [] currently_executing_warps; //FIXME : Deletes an array
    }

    void addTimingThreads(uint64_t* threads, uint32_t warpsize){
      if (*threads > (concurrent_warp_count*warpsize)){
      //if (*threads >= (th_sb_sw)){
        //*threads = *threads - (th_sb_sw);
        //*threads = *threads - (concurrent_warp_count*warpsize);
        *threads = *threads - (th_sb_sw);
        timing_warps += concurrent_warp_count;
      } else {
        timing_warps += (*threads/warpsize);
        if ((*threads % warpsize) >0){
          timing_warps++;
        }
        *threads = 0;
      }
    }

    uint64_t getCurrentWarp () {
      return ((currentwarpset*concurrent_warp_count) + currentwarpid);
    }

    uint64_t getCurrentLocalWarp () {
      return (currentwarpid);
    }


    void resetCurrentWarp () {
      currentwarpid = 0;
    }

    void resetCurrentWarpSet () {
      currentwarpset = 0;
    }

    uint32_t retNextwarp2add(){
      return nextwarp2add;
    }

    void resetNextwarp2add(){
      nextwarp2add = 0;
    }

    uint64_t getnumSP () {
      return numSP;
    }

    void dump(){
      uint64_t warp_id = 0; 
      while (warp_id < warp_status.size()) {
        fprintf(stderr,"\nWARP (%4d):\t",(int)warp_id);
        for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
          if (warp_id < threads_per_pe[pe_id].size())
            fprintf(stderr,"(%5d)\t",(int)threads_per_pe[pe_id][warp_id]);
        }
        warp_id++;
      }

      fprintf(stderr,"\n\n WILL FIRST EXECUTE ......\n\n");
      warp_id = 0;
      uint32_t warp_count = 0;

      while (warp_count< concurrent_warp_count) {
        warp_id = currently_executing_warps[warp_count];
        fprintf(stderr,"\nWARP (%4d):\t",(int)warp_id);
        for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
          if (warp_id < threads_per_pe[pe_id].size())
            fprintf(stderr,"(%5d)\t",(int)threads_per_pe[pe_id][warp_id]);
        }
        warp_count++;
      }

      fprintf(stderr,"\n\n WILL NEXT ADD ......\n\n");
      fprintf(stderr,"\nWARP (%4d):\t",(int)nextwarp2add);
      for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
        if (nextwarp2add < threads_per_pe[pe_id].size())
          fprintf(stderr,"(%5d)\t",(int)threads_per_pe[pe_id][nextwarp2add]);
      }
    }

    void move2nextwarp () {
      currentwarpid++;
      if (currentwarpid >= (concurrent_warp_count)){
        if (warprollover == true){
          currentwarpid = 0;
          //warprollover = false;
          cycledthru_currentwarpset = false;
        } else {
          cycledthru_currentwarpset = true;
          currentwarpid--; // Stay the same, no change
        }
      }
    }


    bool move2nextwarpset () {
      currentwarpset++;
      if (currentwarpset >= warpsets){
        currentwarpset--;
        return false;
      }
      resetCurrentWarp();
      return true;
    }

    void warpsetdump(){
      uint32_t count   = 0;
      uint32_t warpcount   = 0;
      uint32_t warp_id = 0;

      if (warpsets > 0){
        do {
          count = 0;
          fprintf(stderr,"\n--------WARP SET (%4d) of %d --------", warpcount,warpsets-1);
              
          do {
            warp_id = getCurrentWarp();
            if (warp_id <warp_status.size()){
              fprintf(stderr,"\nWARP ID (%4d) BLOCK ID (%4d) TH (%4d):", 
                  warp_id,
                  warp_block_map[warp_id],
                  warp_threadcount[warp_id]
                  );
              for (uint64_t pe_id = 0; pe_id < numSP; pe_id++){
                if (warp_id < threads_per_pe[pe_id].size())
                  fprintf(stderr,"(%5d)\t",(int)threads_per_pe[pe_id][warp_id]);
              }
            }
            move2nextwarp(); 
            count++;
          } while (count < concurrent_warp_count);
          resetCurrentWarp();
          warpcount++;
        } while (move2nextwarpset());
      }

      resetCurrentWarp();
      resetCurrentWarpSet();
    }

    void setnumPE (uint64_t num) {
      numSP = num;
      threads_per_pe = new std::vector<int64_t>[numSP];
      thread_instoffset_per_pe  = new std::vector < int16_t >[numSP];
      thread_traceoffset_per_pe = new std::vector < int16_t >[numSP];
      thread_status_per_pe      = new std::vector < ThreadStatus>[numSP];


      for(uint64_t i =0; i <numSP; i++){
        threads_per_pe[i].clear();
        thread_instoffset_per_pe[i].clear(); 
        thread_traceoffset_per_pe[i].clear();
        thread_status_per_pe[i].clear();
      }

    }

    void addThreads (uint64_t startfid, uint64_t count, uint32_t bs) {
      uint32_t blockend = bs;
      uint64_t pe_id = 0;
      while (count > 0) {
        threads_per_pe[pe_id].push_back (startfid);
        thread_instoffset_per_pe[pe_id].push_back(0);
        thread_traceoffset_per_pe[pe_id].push_back(0);
        thread_status_per_pe[pe_id].push_back (running);
        startfid++;
        totalthreadsinSM++;
        pe_id++;
        if (blockend == 1){
          while (pe_id < numSP){
            threads_per_pe[pe_id].push_back (-1);
            thread_instoffset_per_pe[pe_id].push_back(0);
            thread_traceoffset_per_pe[pe_id].push_back(0);
            thread_status_per_pe[pe_id].push_back (invalid);
            pe_id++;
          }
          blockend = bs;
        } else {
          blockend--;
        }
        if (pe_id >= numSP){
          pe_id = 0;
        }
        count--;
      }

      while ((pe_id != 0) && (pe_id < numSP)) {
        threads_per_pe[pe_id].push_back (-1);
        thread_instoffset_per_pe[pe_id].push_back(0);
        thread_traceoffset_per_pe[pe_id].push_back(0);
        thread_status_per_pe[pe_id].push_back (invalid);
        pe_id++;
      }
    }

    void clearAll () {
      resetCurrentWarp();
      active = false;

      for(uint64_t i =0; i <numSP; i++){
        threads_per_pe[i].clear();
        thread_instoffset_per_pe[i].clear(); 
        thread_traceoffset_per_pe[i].clear();
        thread_status_per_pe[i].clear();
      }

      warp_status.clear();
      warp_last_active_pe.clear();
      warp_block_map.clear();

      active_sps.clear();
      warp_complete.clear();
      warp_blocked.clear();

      totalthreadsinSM      = 0;
      rabbitthreadsComplete = 0;
      timingthreadsComplete = 0;
      timing_warps          = 0;
      th_sb_sw              = 0;

      resetCurrentWarpSet(); //Needed?
    }

    void updateCurrentWarps(uint32_t conc_warps, uint32_t maxwarpssm, uint32_t thsbsw){
      th_sb_sw = thsbsw;
      if (conc_warps > maxwarpssm){
        //Should not happen.
        I(0);
        concurrent_warp_count = maxwarpssm;
      } else {
        concurrent_warp_count = conc_warps;
      }

      if (concurrent_warp_count > warp_status.size())
        concurrent_warp_count = warp_status.size();

      if (currently_executing_warps != NULL) 
        delete [] currently_executing_warps;
      currently_executing_warps = new uint32_t [concurrent_warp_count];

      for (uint32_t i = 0; i < concurrent_warp_count; i++){
        currently_executing_warps[i] = i;
      }

      nextwarp2add = concurrent_warp_count;
    }


    void setglobal_smid (uint64_t smid) {
      global_smid = smid;
    }

    uint64_t getglobal_smid () {
      return global_smid;
    }

    void setActive () {
      active = true;
    }

    void setInactive () {
      active = false;
      smstatus = SMcomplete;
    }

    bool isActive () {
      return active;
    }

    bool isTimingComplete(){
      if (timingthreadsComplete >= (timing_warps*numSP)){
        return true;
      }
      return false;
    }

};

class GPUThreadManager {
  private:
    //uint64_t numthreads;
    esescSM *SM;
    uint64_t *SM_fid_revmap;
    uint64_t numSM;              // The number of SMs in the system
    uint64_t numSP;              // The number of SPs per SM
    std::vector <uint64_t> kernelsNumSM;

    uint64_t maxSMID;
    uint32_t number_ffinst_perThread;
#if TRACKGPU_MEMADDR
    ofstream memdata;
#endif
  protected:
    static GStatsCntr *totalPTXinst_T;
    static GStatsCntr *totalPTXinst_R;
    static GStatsCntr *totalPTXmemAccess_Shared;
    static GStatsCntr *totalPTXmemAccess_Global;
    static GStatsCntr *totalPTXmemAccess_Rest;

  public:

    //GPUThreadManager (uint64_t numFlows, uint64_t numPEs, uint64_t numthreads, uint64_t blocksize);
    GPUThreadManager (uint64_t numFlows, uint64_t numPEs);
    ~GPUThreadManager ();

    void dump();

    uint64_t mapLocalID (uint64_t lid) {
      return SM[lid].getglobal_smid ();
    } 

    uint64_t mapGlobalID (uint64_t gid) {
      return SM_fid_revmap[gid];
    }

    bool isSMactive (uint64_t smid) {
      return SM[smid].isActive ();
    }

    bool canTurnOff (uint64_t smid) {
      //IS(MSG("timing threads complete = %llu,rabbit threads complete = %llu, totalthreadsin sm = %llu",SM[smid].timingthreadsComplete,SM[smid].rabbitthreadsComplete, SM[smid].totalthreadsinSM));
      if ((SM[smid].timingthreadsComplete+SM[smid].rabbitthreadsComplete) == SM[smid].totalthreadsinSM)
        return true;

      return false;
    }

    bool canTurnOffTiming (uint64_t smid) {
      //IS(MSG("timing threads complete = %llu,rabbit threads complete = %llu, totalthreadsin sm = %llu",SM[smid].timingthreadsComplete,SM[smid].rabbitthreadsComplete, SM[smid].totalthreadsinSM));
      if ((SM[smid].timingthreadsComplete) >= (SM[smid].timing_warps*numSP)){
        //MSG(" [%d] timing threads complete out of [%d]. Turning it off...",(int)(SM[smid].timingthreadsComplete), (int)(SM[smid].timing_warps*numSP));
        return true;
      }
      //  MSG(" [%d] timing threads complete out of [%d]. Cannot turn it off...",(int)(SM[smid].timingthreadsComplete),(int)(SM[smid].timing_warps*numSP));
      return false;
    }

    void setSMactive (uint64_t smid) {
      SM[smid].setActive ();
    }

    void setSMinactive (uint64_t smid) {
      SM[smid].setInactive ();
    }

    uint64_t ret_numSM () {
      return numSM;
    }

    void setNumSM(uint32_t kid) {  numSM = kernelsNumSM[kid]; printf("numSM:%d\n", numSM);};
    void setKernelsNumSM(uint32_t kid, uint64_t nsm) { kernelsNumSM[kid] = nsm; };

    // Empties all the arrays
    void clearAll (); 

    // Redistributes the threads into different warps etc. 
    void reOrganize (); 
    uint32_t calcOccupancy(); 
    
    void pauseAllThreads(uint32_t* h_trace, bool init, bool isRabbit = false);
    void resumeAllThreads(uint32_t* h_trace, bool init,  bool isRabbit = false);

    void pauseThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit = false);
    void resumeThreadsInRange(uint32_t* h_trace, uint64_t startRange, uint64_t endRange, bool init, bool isRabbit = false);

    void resumeSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit = false);
    void pauseSelectivethreads(uint32_t* h_trace, uint32_t smid, uint64_t warpid, bool init, bool isRabbit = false);

    //Makes the association between the global fid and the local smid
    void link (uint64_t glosmid, uint64_t smid);

    //Makes the association between the local smid and the global fid
    void createRevMap ();

    // Initialize h_trace before a kernel launch for a brand new kernel
    void init_trace (uint32_t * h_trace);

    // Initialize h_trace before launching the same kernel but for a different BB
    void init_trace_newBB (uint32_t * h_trace);

    // Decode a h_trace before a kernel launch
    bool decode_trace(EmuSampler * gsampler, uint32_t * h_trace, void *env, uint32_t* qemuid);
    bool Timing(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid);
    bool Rabbit(EmuSampler* gsampler, uint32_t* h_trace_qemu, void* env, uint32_t* qemuid);

    void possiblyturnOFFSM(uint32_t smid);
    bool switch2nextwarp(uint32_t smid,uint32_t* h_trace);
    void reset_nextRabbitBB(uint32_t* h_trace_qemu);
    void reset_continueAllTiming(bool afterrabbit);
    void reset_nextTimingBB(uint32_t* h_trace_qemu);
    void reset_pauseAllRabbit(uint32_t* h_trace_qemu);


    // Reset the status flags of the SMs
    void reset_nextBB(uint32_t* h_trace);
    void reset_nextkernel();
    void reset_finishkernel(uint32_t* h_trace);

    //Signature
    void decodeSignature(uint32_t* h_trace_qemu);
    void putinfile(uint64_t inst);

    uint32_t getNumKernels() { return kernels.size(); };
    uint32_t getMinNumSM();

};

#endif
