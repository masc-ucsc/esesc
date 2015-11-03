/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by  Jose Renau
Alamelu Sankaranarayanan

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

#include <string.h>

#include "GPUEmulInterface.h"
#include "GPUInterface.h"
#include "SescConf.h"

extern   bool     unifiedCPUGPUmem;
extern   bool     MIMDmode;
extern   div_type branch_div_mech;

  GPUEmulInterface::GPUEmulInterface(const char *section)
:EmulInterface(section) 
{/*{{{*/

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

  if (SescConf->checkBool(section, "enableMIMDmode")) {
    MIMDmode = SescConf->getBool(section, "enableMIMDmode");
  } else {
    IS(fprintf(stderr,"\nMIMD mode (enableMIMDmode =) not specified  in esesc.conf... Assuming conventional GPU Setup"));
    MIMDmode = false;
  }


  if (SescConf->checkCharPtr(section, "divergence_mech")) {
    const char* branch_div = SescConf->getCharPtr(section, "divergence_mech");
    if (branch_div != NULL) {
      if (strcasecmp(branch_div, "serial") == 0) {
        branch_div_mech = serial;
      } else if (strcasecmp(branch_div, "post_dom") == 0) {
        branch_div_mech = post_dom;
      } else if (strcasecmp(branch_div, "sbi") == 0) {
        branch_div_mech = sbi;
      } else if (strcasecmp(branch_div, "sbi_swi") == 0) {
        branch_div_mech = sbi_swi;
      } else if (strcasecmp(branch_div, "original") == 0) {
        branch_div_mech = original;
      } else {
        IS(fprintf(stderr,"\n Unable to parse %s? Branch divergence mechanism set to post_dom",branch_div));
        //branch_div_mech = post_dom;
        branch_div_mech = serial ;
      }
    } else {
      IS(fprintf(stderr,"\n Branch divergence mechanism not specified, set to post_dom"));      
      //branch_div_mech = post_dom;
      branch_div_mech = serial ;
    }
  } else {
    IS(fprintf(stderr,"\nBranch divergence mechanism (divergence_mech =) not specified  in esesc.conf... Assuming Serial Diveregnce"));
    //branch_div_mech = post_dom;
    branch_div_mech = serial ;
  }

  for (FlowID i = 0; i < nEmuls; i++) {
    const char *section = SescConf->getCharPtr("", "cpuemul", i);
    const char *type = SescConf->getCharPtr(section, "type");
    if (strcasecmp(type, "GPU") == 0) {
      nFlows++;
    }
  }
#ifdef DEBUG
  MSG("\nGPUEmulInterface.cpp::nFlows = %d, MIMD mode is%senabled", nFlows,(MIMDmode==true) ? " ":" not ");
#endif

  // number of pes per SM
  numPEs = SescConf->getInt(section, "pes_per_sm", 0);
  if (!numPEs) {
    MSG("WARNING : Invalid PE_per_SM specified for the GPU, Assuming default size of 32");
    numPEs = 32;
  }

  if (MIMDmode){

    //Error checking for MIMD mode.

    // 1. We have to make sure that we have enough flows to be at least a multiple of the 
    // number of PEs, else we will quit. 
    if ((nFlows % numPEs) != 0 ){
      MSG("\n ERROR : We have only %d GPU flows to emulate SMs with %d PEs each. The number of flows should be a perfect multiple", nFlows, numPEs);
      exit (-1);
    } else {
      MSG("\n We will simulate %d SMs with %d PEs each in MIMD mode", (nFlows/numPEs), numPEs);
    }

  }

  //warpsize is the number of threads per warp
  warpsize = SescConf->getInt(section, "warpsize", 0);
  if (!warpsize) {
    warpsize = numPEs;
    MSG("WARNING : Invalid warpsize specified for the GPU, Assuming default size of %d", warpsize);
  }

  // half warpsize is the number of the threads in the warp that can be executed on the SM at a given time.
  halfwarpsize = SescConf->getInt(section, "halfwarpsize", 0);
  if (!halfwarpsize) {
    halfwarpsize = warpsize;
    MSG("WARNING : Invalid halfwarpsize specified for the GPU, Assuming default size of %d",halfwarpsize);
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
    MSG("WARNING : Invalid shared memory granularity (shmem_granularity) specified for the GPU, Assuming default size of 128");
    shmem_granularity = 128;
  }

  // Maximum amount of shmem available per SM 
  max_shmem_sm = SescConf->getInt(section, "max_shmem_sm", 0);
  if (!max_shmem_sm) {
    max_shmem_sm = 48 * 1024;
    MSG("WARNING : Invalid max_shmem_sm specified for the GPU, Assuming default size of %d bytes",max_shmem_sm);
  }

  // Maximum number of allowable resident warps on the sm
  max_warps_sm = SescConf->getInt(section, "max_warps_sm", 0);
  if (!max_warps_sm) {
    max_warps_sm = 48;
    MSG("WARNING : Invalid max_warps_sm specified for the GPU, Assuming default size of %d", max_warps_sm);
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

  uint64_t gfid, lfid = 0;
  for (uint32_t i = 0; ((i < nFlows) && (lfid < nFlows)); i++) {
    gfid = mapLid(lfid);
    gpuTM->link(gfid, i);
    
    if(MIMDmode){
      lfid = lfid + (i+1)*numPEs;
    } else {
      lfid = (i+1);
    }

  }

  gpuTM->createRevMap();
  reader = new GPUReader(section, this);

}/*}}}*/

GPUEmulInterface::~GPUEmulInterface() 
{/*{{{*/
  delete reader;
  reader = 0;
}/*}}}*/

FlowID GPUEmulInterface::getFid(FlowID fid)
  /* For GPU, Get a new fid, tries to use the same last_fid if possible/applicable */
  /* This function is used only by the GPU */
{/*{{{*/
  return mapLid(fid);
}/*}}}*/

FlowID GPUEmulInterface::mapLid(FlowID lid)
{/*{{{*/
  I(lid >= 0);
  I(lid < fidFreePool.size());
  return fidFreePool.at(lid);
}/*}}}*/

void GPUEmulInterface::freeFid(FlowID fid)
{/*{{{*/
  //pthread_mutex_lock (&mutex);

  fidFreePool.push_back(fid);

  //pthread_mutex_unlock(&mutex);
}/*}}}*/

DInst *GPUEmulInterface::executeHead(FlowID fid) 
{/*{{{*/
  I(nFlows);
  return reader->executeHead(fid);
}/*}}}*/

void GPUEmulInterface::reexecuteTail(FlowID fid) 
{/*{{{*/
  I(nFlows);
  reader->reexecuteTail(fid);
}/*}}}*/

void GPUEmulInterface::syncHeadTail(FlowID fid) 
{/*{{{*/
  I(nFlows);
  reader->syncHeadTail(fid);
}/*}}}*/

FlowID GPUEmulInterface::getNumFlows(void) const 
{/*{{{*/
  return nFlows;
}/*}}}*/

FlowID GPUEmulInterface::getNumEmuls(void) const 
{/*{{{*/
  return nEmuls; //FIXME: Is this okay?
}/*}}}*/

void GPUEmulInterface::startRabbit(FlowID fid) 
{/*{{{*/
}/*}}}*/

void GPUEmulInterface::startWarmup(FlowID fid) 
{/*{{{*/
}/*}}}*/

void GPUEmulInterface::startDetail(FlowID fid) 
{/*{{{*/
}/*}}}*/

void GPUEmulInterface::startTiming(FlowID fid) 
{/*{{{*/
}/*}}}*/

void GPUEmulInterface::setSampler(EmuSampler * a_sampler, FlowID fid) 
{/*{{{*/
  EmulInterface::setSampler(a_sampler);
  //I(gsampler == 0); //True only for the initial sampler, will fail because the sampler is shared. 
  gsampler = a_sampler;
}/*}}}*/


#if 0
void GPUEmulInterface::drainFIFO()/*{{{*/
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
/*}}}*/
#else 

void GPUEmulInterface::drainFIFO()
{/*{{{*/
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
}/*}}}*/
#endif
