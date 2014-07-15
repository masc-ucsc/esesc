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

#include "SamplerSMARTS.h"
#include "EmulInterface.h"
#include "SescConf.h"
#include "BootLoader.h"
#include "TaskHandler.h"
#include "MemObj.h"
#include "GProcessor.h"
#include "GMemorySystem.h"

#include <iostream>

SamplerSMARTS::SamplerSMARTS(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : SamplerBase(iname, section, emu, fid)
  /* SamplerSMARTS constructor {{{1 */
{
  finished[fid] = true; // will be set to false in resumeThread
  finished[0] = false;

  nInstForcedDetail = nInstDetail==0? nInstTiming/2:nInstDetail;

  setNextSwitch(nInstSkip);
  if (nInstSkip)
    startRabbit(fid);

  std::cout << "Sampler: TBS, R:" << nInstRabbit
            << ", W:"             << nInstWarmup
            << ", D:"             << nInstDetail
            << ", T:"             << nInstTiming
            << std::endl;

  std::cout << "Sampler: TBS, nInstMax:" << nInstMax
            << ", nInstSkip:"            << nInstSkip
            << ", maxnsTime:"           << maxnsTime
            << std::endl;
}
/* }}} */

SamplerSMARTS::~SamplerSMARTS() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

//changed by Hamid R. Khaleghzadeh///////////////
int SamplerSMARTS::queue(uint32_t insn, uint64_t pc, uint64_t addr, FlowID fid, char op, uint64_t icount, void *env)
  /* main qemu/gpu/tracer/... entry point {{{1 */
{

  I(fid < emul->getNumEmuls());
  if(likely(!execute(fid, icount)))
    return 0; // QEMU can still send a few additional instructions (emul should stop soon)
  I(mode!=EmuInit);

  I(insn);

  // process the current sample mode
  if (getNextSwitch()>totalnInst) {

    if (mode == EmuRabbit || mode == EmuInit)
      return 0;

    if (mode == EmuDetail || mode == EmuTiming) {
      return(emul->queueInstruction(insn,pc,addr, (op&0xc0) /* thumb */ ,fid, env, getStatsFlag()));
      //return;
    }

    I(mode == EmuWarmup);
		doWarmupOpAddr(op, addr);
    return 0;
  }


  // Look for the new mode
  I(getNextSwitch() <= totalnInst);

 // I(mode != next_mode);
  pthread_mutex_lock (&mode_lock);
  //

  if (getNextSwitch() > totalnInst){//another thread just changed the mode
    pthread_mutex_unlock (&mode_lock);
    return 0;
  }

  lastMode = mode;
  nextMode(ROTATE, fid);
  if (lastMode == EmuTiming) { // timing is going to be over
    if (getTime()>=maxnsTime || totalnInst>=nInstMax) {
      markDone();
      pthread_mutex_unlock (&mode_lock);
      return 0;
    }
    if (doPower) {
      uint64_t mytime = getTime();
      int64_t ti = mytime - lastTime;
      I(ti > 0);
      ti = (static_cast<int64_t>(freq)*ti)/1e9;

      BootLoader::getPowerModelPtr()->setSamplingRatio(getSamplingRatio()); 
      BootLoader::getPowerModelPtr()->calcStats(ti, !(lastMode == EmuTiming), fid); 
      lastTime = mytime;
      updateCPI(fid); 
      if (doTherm) {
        BootLoader::getPowerModelPtr()->updateSescTherm(ti);  
      }
    }
  }
  pthread_mutex_unlock (&mode_lock);
  
  return 0;

}
/* }}} */



void SamplerSMARTS::updateCPI(FlowID fid){
  //extract cpi of last sample interval 
 
  estCPI = getMeaCPI();
  return; 

}


void SamplerSMARTS::nextMode(bool rotate, FlowID fid, EmuMode mod){

  if (rotate){

    fetchNextMode();
    I(next_mode != EmuInit);

    setMode(next_mode, fid);
    I(mode == next_mode);
    if (next_mode == EmuRabbit){
      setModeNativeRabbit();
    }
    setNextSwitch(getNextSwitch() + sequence_size[sequence_pos]);
  }else{
    I(0);
  }
}

