/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan K.Ardestani


This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

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
  /* SamplerGPUSpacial constructor {{{1 */
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
/* }}} */

SamplerGPUSpacial::~SamplerGPUSpacial() 
  /* DestructorRabbit {{{1 */
{
}
/* }}} */

void SamplerGPUSpacial::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, FlowID fid, char op, uint64_t icount, void *env)
  /* main qemu/gpu/tracer/... entry point {{{1 */
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
    emul->queueInstruction(insn,pc,addr,data, (op&0xc0) /* thumb */ ,fid, env, getStatsFlag());
    lastMode = mode;
    return;
  }


  // not happening for GPU, the CPU will decide!

  // We did enough
  if (totalnInst >= nInstMax || endSimSiged) {
  //if (totalnSamples >= nSampleMax || endSimSiged) { //FIXME
    markDone();
    return;
  }


}
/* }}} */



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

    TaskHandler::syncStats();

    
    BootLoader::getPowerModelPtr()->setKernelId(emul->getKernelId());

    std::cout<<" Timeinterval "<<ti<<" mytime "<<mytime<<" last time "<<lastTime<<"\n";  
    simt = BootLoader::getPowerModelPtr()->calcStats(ti, false, sFid); 
    lastTime = mytime;

    endSimSiged = (simt==90)?1:0; // 90 is a signal, not the actual time.
}

uint64_t SamplerGPUSpacial::getGPUTime() {
  // the time factor is here because GPU kernels are short.
  uint64_t gpuEstimatedTime = timeFactor*(gpuEstimatedCycles/freq)*1e9;
  return gpuEstimatedTime;
}

#endif
