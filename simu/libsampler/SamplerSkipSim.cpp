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

#include "SamplerSkipSim.h"
#include "EmulInterface.h"
#include "BootLoader.h"
#include "SescConf.h"
#include <set>

SamplerSkipSim::SamplerSkipSim(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : SamplerBase(iname, section, emu)
  /* SamplerSkipSim constructor {{{1 */
{
  nInstSkip = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkip"));
  nInstMax  = static_cast<uint64_t>(SescConf->getDouble(section,"nInstMax"));
  
  if (fid != 0) // first thread might need a different skip
    nInstSkip   = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkipThreads"));
  
  if (nInstSkip)
    startRabbit(fid);
  else
    startTiming(fid);

}
/* }}} */

SamplerSkipSim::~SamplerSkipSim() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

void SamplerSkipSim::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env)
  /* main qemu/gpu/tracer/... entry point {{{1 */
{
  if (TaskHandler::isTerminated())
    return;

  if(!execute(fid,icount))
    return; // QEMU can still send a few additional instructions (emul should stop soon)
  
  I(mode!=EmuInit);
  I(insn!=0);
  I(icount!=0);
  
  if (nInstMax <= totalnInst || endSimSiged) {
    markDone();
    return;
  }

  if (nInstSkip>=totalnInst) {
    I(mode==EmuRabbit);
    //pthread_mutex_unlock (&mode_lock);
    return;
  }

  static bool first = true;
  if (first) {
    lastTime = getLocalTime();
    first    = false;
  }

  I(nInstSkip<=totalnInst);
  if (mode == EmuRabbit) {
    stop();
    startTiming(fid);
  }

  emul->queueInstruction(insn,pc,addr,data, op&0xc0 /*thumb*/, fid, env, getStatsFlag());

  if (!doPower) 
    return;
 
  pthread_mutex_lock (&mode_lock); //*************** BEGIN ATOMIC

  uint64_t ti = 0;
  bool callpwr = callPowerModel(ti, fid);

  if (callpwr) { 
    I(ti > 0);

    int32_t simt = 0;
    //setMode(EmuTiming, fid);  //Spikes?
    TaskHandler::syncStats();
    simt =  BootLoader::getPowerModelPtr()->calcStats(ti, !(mode == EmuTiming), fid);

    endSimSiged = (simt==90)?1:0;
  }

  pthread_mutex_unlock (&mode_lock); //*************** END ATOMIC
}
/* }}} */


