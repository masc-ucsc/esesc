/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University California, Santa Cruz.

   Contributed by Jose Renau


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

void SamplerGPUSim::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env)
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

  emul->queueInstruction(insn,pc,addr,data, (op&0x80) /* thumb */, fid, env);
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
