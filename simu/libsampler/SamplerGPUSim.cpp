// Contributed by Jose Renau
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

#if 0
#include "SamplerGPUSim.h"
#include "EmulInterface.h"
#include "BootLoader.h"
#include "SescConf.h"
#include <set>

SamplerGPUSim::SamplerGPUSim(const char *iname, const char *section, EmulInterface *emu)
  : SamplerBase(iname, section, emu)
  /* SamplerGPUSim constructor {{{1 */
{
  nInstSkip = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkip"));
  nInstMax  = static_cast<uint64_t>(SescConf->getDouble(section,"nInstMax"));


  if (nInstSkip)
    //startRabbit(emul->getFirstFlow());
    startRabbit(emul->mapLid(0));
  else
    //startTiming(emul->getFirstFlow());
    startRabbit(emul->mapLid(0)); //FIXME

}
/* }}} */

SamplerGPUSim::~SamplerGPUSim() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

void SamplerGPUSim::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint32_t fid, char op)
  /* main qemu/gpu/tracer/... entry point {{{1 */
{
  if(!execute(fid,icount))
    return; // QEMU can still send a few additional instructions (emul should stop soon)
  
  I(mode!=EmuInit);

  I(insn!=0);
  I(icount!=0);

  if (doPower){

    uint64_t ti = 0;
    bool callpwr = callPowerModel(ti, fid);

    if (callpwr){ 

      I(ti > 0);
      //printf("totalnInst:%ld, nPassedInst:%ld, interval:%ld\n", totalnInst, nPassedInst, interval);


      bool dummy = false;

      //std::cout<<"mode "<<mode<<" Timeinterval "<<ti<<" last time "<<lastTime<<"\n";  

      int simt = 0;
      if (ti > 0){
        setMode(EmuTiming, fid);
        simt =  BootLoader::pwrmodel.calcStats(ti, 
            !(mode == EmuTiming), static_cast<float>(freq), dummy, dummy, dummy, dummy); 

        endSimSiged = (simt==90)?1:0;
        BootLoader::pwrmodel.sescThermWrapper->sesctherm.updateMetrics();  
      }
    }
  }// doPower
  
  if (nInstMax < totalnInst || endSimSiged) {
    markDone();
    return;
  }
  if (nInstSkip>totalnInst) {
    I(mode==EmuRabbit);
    return;
  }
  I(nInstSkip<=totalnInst);
  if (mode == EmuRabbit) {
    stop();
    startTiming(fid);
  }

#if 0
  static std::set<AddrType> seenPC;
  static Time_t seenPC_last   = 0;
  static Time_t seenPC_active = 0;
  static Time_t seenPC_total  = 0;
  seenPC_total++;
  if (seenPC.find(pc^insn) == seenPC.end()) {
    seenPC.insert(pc^insn);
    seenPC_last = seenPC_total;
  }
  if ((seenPC_last+1000) > seenPC_total)
    seenPC_active++;
/*
  if ((seenPC_total & 1048575) == 1)
    MSG("%5.3f",(100.0*seenPC_active)/(1.0+seenPC_total)); */
#endif

  emul->queueInstruction(insn,pc,addr, (op&0x80) /* thumb */, fid);
}
/* }}} */

uint64_t SamplerGPUSim::getTime() const
/* time in ns since the beginning {{{1 */
{

  //lastIPC = getLastIPC();
  // WARNING: It is OK to read globalClock because Detail time needs to be used
    uint64_t nsticks = static_cast<uint64_t>(((double)globalClock/freq)*1.0e9) ;
    nsticks += static_cast<uint64_t>(((double)phasenInst/lastIPC*freq)*1.0e9);

  return nsticks;
}
/* }}} */


#endif
