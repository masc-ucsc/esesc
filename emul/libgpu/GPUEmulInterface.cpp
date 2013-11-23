// Contributed by Jose Renau
//                Alamelu Sankaranarayanan
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

#include <string.h>

#include "GPUEmulInterface.h"
#include "GPUInterface.h"
#include "SescConf.h"

extern   bool     unifiedCPUGPUmem;

GPUEmulInterface::GPUEmulInterface(const char *section)
  :EmulInterface(section) {

    cputype = GPU;

    nFlows = 0;
    nEmuls = SescConf->getRecordSize("", "cpuemul");
    const char* emultype = SescConf->getCharPtr(section,"type");

    for(FlowID i=1;i<nEmuls;i++) { //fid zero is already taken by qemu
      const char *section = SescConf->getCharPtr("","cpuemul",i);
      const char *type    = SescConf->getCharPtr(section,"type");
      if(strcasecmp(type,emultype) == 0 ) {
        fidFreePool.push_back(i);
      }
    }
    bool dorun = SescConf->getBool(section, "dorun");
    if (!dorun) {
      nFlows = 0;
      return;
    }
    if (SescConf->checkBool(section, "unifiedCPUGPUmem")) {
      unifiedCPUGPUmem = SescConf->getBool(section, "unifiedCPUGPUmem");
    } else {
      IS(fprintf(stderr,"\nFlag unifiedCPUGPUmem not found in esesc.conf..."));
      unifiedCPUGPUmem = false;
    }
    if (unifiedCPUGPUmem){
      IS(fprintf(stderr,"Assuming no memcpy needed between CPU & GPU\n"));
    }else{
      IS(fprintf(stderr,"Assuming memcpy needed between CPU & GPU\n"));
    }

    for (FlowID i = 0; i < nEmuls; i++) {
      const char *section = SescConf->getCharPtr("", "cpuemul", i);
      const char *type = SescConf->getCharPtr(section, "type");
      if (strcasecmp(type, "GPU") == 0) {
        nFlows++;
      }
    }
#ifdef DEBUG
    MSG("GPUEmulInterface.cpp::nFlows = %d", nFlows);
#endif

    // number of pes per SM
    numPEs = SescConf->getInt(section, "pes_per_sm", 0);
    if (!numPEs) {
      MSG("WARNING : Invalid PE_per_SM specified for the GPU, Assuming default size of 32");
      numPEs = 32;
    }

    //warpsize is the number of threads per warp
    warpsize = SescConf->getInt(section, "warpsize", 0);
    if (!warpsize) {
      MSG("WARNING : Invalid warpsize specified for the GPU, Assuming default size of 32");
      warpsize = 32;
    }

    // half warpsize is the number of the threads in the warp that can be executed on the SM at a given time.
    halfwarpsize = SescConf->getInt(section, "halfwarpsize", 0);
    if (!halfwarpsize) {
      MSG("WARNING : Invalid halfwarpsize specified for the GPU, Assuming default size of %d",warpsize);
      halfwarpsize = warpsize;
    }

    // A multiple of "thread allocation granularity" is the number of registers allocated per thread.
    thread_granularity = SescConf->getInt(section, "thread_granularity", 0);
    if (!thread_granularity || ((thread_granularity != 256) && (thread_granularity != 512) && (thread_granularity != 64)) ) {
      MSG("WARNING : Invalid thread_granularity specified for the GPU, Assuming default size of 64");
      thread_granularity = 64;
    }

    // A multiple of "shared mem granularity" is the number of sh mem registers allocated per block of threads.
    shmem_granularity = SescConf->getInt(section, "shmem_granularity", 0);
    if (!shmem_granularity || ((shmem_granularity != 512) && (shmem_granularity != 128)) ) {
      MSG("WARNING : Invalid halfwarpsize specified for the GPU, Assuming default size of 128");
      shmem_granularity = 128;
    }

    // Maximum amount of shmem available per SM 
    max_shmem_sm = SescConf->getInt(section, "max_shmem_sm", 0);
    if (!max_shmem_sm) {
      MSG("WARNING : Invalid max_shmem_sm specified for the GPU, Assuming default size of 48K bytes");
      max_shmem_sm = 48 * 1024;
    }

    // Maximum number of allowable resident warps on the sm
    max_warps_sm = SescConf->getInt(section, "max_warps_sm", 0);
    if (!max_warps_sm) {
      MSG("WARNING : Invalid max_warps_sm specified for the GPU, Assuming default size of 48");
      max_warps_sm = 48;
    }

    // Maximum number of allowable blocks on the sm
    max_blocks_sm = SescConf->getInt(section, "max_blocks_sm", 0);
    if (!max_blocks_sm) {
      MSG("WARNING : Invalid max_blocks_sm specified for the GPU, Assuming default size of 8");
      max_blocks_sm = 8;
    }

    // Maximum number of registers on the sm
    max_regs_sm = SescConf->getInt(section, "max_regs_sm", 0);
    if (!max_regs_sm) {
      MSG("WARNING : Invalid max_regs_sm specified for the GPU, Assuming default size of 32K registers");
      max_regs_sm = 32*1024;
    }

    //gpuTM = new GPUThreadManager(nFlows, numPEs, numThreads, blockSize);
    gpuTM = new GPUThreadManager(nFlows, numPEs);
    for (uint32_t i = 0; i < nFlows; i++) {
      gpuTM->link(mapLid(i), i);
    }
    gpuTM->createRevMap();
    reader = new GPUReader(section, this);
  }

GPUEmulInterface::~GPUEmulInterface() {
  delete reader;
  reader = 0;
}

FlowID GPUEmulInterface::getFid(FlowID fid)
/* For GPU, Get a new fid, tries to use the same last_fid if possible/applicable */
/* This function is used only by the GPU */
{
  return mapLid(fid);
}

FlowID GPUEmulInterface::mapLid(FlowID lid)
{
  I(lid >= 0);
  I(lid < fidFreePool.size());
  return fidFreePool.at(lid);
}

void GPUEmulInterface::freeFid(FlowID fid)
{
  //pthread_mutex_lock (&mutex);

  fidFreePool.push_back(fid);

  //pthread_mutex_unlock(&mutex);
}

DInst *GPUEmulInterface::executeHead(FlowID fid) {
  I(nFlows);
  return reader->executeHead(fid);
}

void GPUEmulInterface::reexecuteTail(FlowID fid) {
  I(nFlows);
  reader->reexecuteTail(fid);
}

void GPUEmulInterface::syncHeadTail(FlowID fid) {
  I(nFlows);
  reader->syncHeadTail(fid);
}

FlowID GPUEmulInterface::getNumFlows(void) const {
  return nFlows;
}

FlowID GPUEmulInterface::getNumEmuls(void) const {
  return nEmuls; //FIXME: Is this okay?
}

void GPUEmulInterface::startRabbit(FlowID fid) {
}

void GPUEmulInterface::startWarmup(FlowID fid) {
}

void GPUEmulInterface::startDetail(FlowID fid) {
}

void GPUEmulInterface::startTiming(FlowID fid) {
}

void GPUEmulInterface::setSampler(EmuSampler * a_sampler, FlowID fid) {
  EmulInterface::setSampler(a_sampler);
  //I(gsampler == 0); //True only for the initial sampler, will fail because the sampler is shared. 
  gsampler = a_sampler;
}
#if 0
void GPUEmulInterface::drainFIFO()
{
  uint32_t drained = nFlows;
  uint32_t cnt = 0;
  do{
    for(size_t i=0; i<nFlows; i++) {
      if (reader->drainedFIFO(mapLid(i))) {
        drained --;
      } else {
        //IS(fprintf(stderr,"-%d-",(int)mapLid(i)));
      }
    }

    if (drained){
      pthread_yield();
      drained = nFlows;
      if (cnt++ > 100000)
      {
        IS(fprintf(stderr,"."));
        cnt = 0;
      }
    }
  }while(drained);
}
#endif
#if 1
void GPUEmulInterface::drainFIFO()
{
  bool drained = true;
  bool pausethread = false;
  uint32_t cnt = 0;
  uint64_t global_fid;

  for(size_t i=0; i<nFlows; i++) {
    global_fid   = mapLid(i);
    drained = reader->drainedFIFO(global_fid);
    pausethread = false;

    while(!drained){
      if (!pausethread){
        gsampler->resumeThread(global_fid);
      }
      pausethread = true;
      struct timespec ts = {0,1000000};
      nanosleep(&ts, 0);
      pthread_yield();
      if (cnt++ > 100000)
      {
        IS(fprintf(stderr,"."));
        cnt = 0;
      }
      drained = reader->drainedFIFO(global_fid);
    } //End of while

    if (pausethread){
      gsampler->pauseThread(global_fid);
    }

  }
}

#endif
