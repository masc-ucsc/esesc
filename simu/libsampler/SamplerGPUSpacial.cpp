// Contributed by Jose Renau
//                Ehsan K.Ardestani
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

#include "SamplerGPUSpacial.h"
#include "EmulInterface.h"
#include "SescConf.h"
#include "BootLoader.h"
#include "TaskHandler.h"
#include "MemObj.h"
#include "GProcessor.h"
#include "GMemorySystem.h"


extern uint32_t roundUp(uint32_t numToRound, uint32_t multiple);
extern uint32_t roundDown(uint32_t numToRound, uint32_t multiple);


SamplerGPUSpacial::SamplerGPUSpacial(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : SamplerBase(iname, section, emu, fid)
  /* SamplerGPUSpacial constructor  */
{

    max_exec_threads = SescConf->getInt(section, "nMaxThreads", 0);
    if (doPower)
      timeFactor       = SescConf->getInt(section,"timeFactor");

    if (!max_exec_threads) {
      MSG("WARNING : No max_exec_threads specified for the GPU, Assuming RABBIT only"); 
      max_exec_threads = 0;
    }

    max_exec_threads_conf_file = max_exec_threads;

    fullTiming = (max_exec_threads<0);
    if (fullTiming) {
      MSG("Full Timing Simulation Mode. It's going to be long!"); 
    }


    /*
    if (max_exec_threads > 0) {
      int64_t PEtimesSM = emul->getNumPEs() * emul->getNumFlows();
      //CHECKME
      if((max_exec_threads % PEtimesSM) != 0){
        max_exec_threads = (max_exec_threads / PEtimesSM) * PEtimesSM;
        MSG("WARNING : Invalid max_exec_threads specified for the GPU, Assuming default size of %d (PE*SM) threads",(int)max_exec_threads);
      } 
    }
    */

    //If, for a given kernel iteration,  max_exec_threads is more than num_threads, then it will be logged and it will be as good as full 
    //timing for that iteration
}
/*  */

SamplerGPUSpacial::~SamplerGPUSpacial() 
  /* DestructorRabbit  */
{
}
/*  */

void SamplerGPUSpacial::queue(uint32_t insn, uint64_t pc, uint64_t addr, FlowID fid, char op)
  /* main qemu/gpu/tracer/... entry point  */
{

  if(likely(!execute(fid, icount)))
    return; // QEMU can still send a few additional instructions (emul should stop soon)
  I(mode!=EmuInit);

  I(insn);

  if (mode == EmuRabbit || mode == EmuWarmup){
    if (doPower && noActiveCPU()) {
      if (lastMode == EmuTiming) { 
#if 1 // For now GPU only, CPU side must be all rabbit!
        doPWTH();
#endif
      }
    }
    lastMode = mode;
    return;
  }

  if (mode == EmuDetail || mode == EmuTiming) {
    emul->queueInstruction(insn,pc,addr, (op&0xc0) /* thumb */ ,fid, getStatsFlag());
    lastMode = mode;
    return;
  }


  // not happening for GPU, the CPU will decide!

  // We did enough
  //if (totalnInst >= nInstMax/* || endSimSiged*/) {
  if (totalnInst >= nInstMax) {
    markDone();
    return;
  }


}
/*  */



void SamplerGPUSpacial::updateIPC(FlowID fid){
  //extract ipc of last sample interval 

}

bool SamplerGPUSpacial::noActiveCPU() {
  for(size_t i = 0; i< TaskHandler::FlowIDEmulMapping.size(); i++) {
    if (TaskHandler::FlowIDEmulMapping[i] == 0) { // CPUs
      if (isActive(i))
        return false;
    }
  }
  return true;
}


void SamplerGPUSpacial::doPWTH() {
  uint64_t mytime = 0.0;
  int simt = 0 ; 

    mytime          = getGPUTime();
    I(mytime > lastTime);

    uint64_t ti = static_cast<uint64_t> (mytime - lastTime);

    
    BootLoader::getPowerModelPtr()->setKernelId(emul->getKernelId());

    //std::cout<<" Timeinterval "<<ti<<" mytime "<<mytime<<" last time "<<lastTime<<"\n";  

    BootLoader::getPowerModelPtr()->calcStats(ti, false, sFid); 
    lastTime = mytime;

    //endSimSiged = (simt==90)?1:0; // 90 is a signal, not the actual time.
}

uint64_t SamplerGPUSpacial::getGPUTime() {
  // the time factor is here because GPU kernels are short.
  uint64_t gpuEstimatedTime = timeFactor*(gpuEstimatedCycles/freq)*1e9;
  return gpuEstimatedTime;
}

#endif
