/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by  Sina Hassani
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


#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../emul/libemulint/InstOpcode.h"
#include "../misc/libsuc/LiveCache.h"
extern LiveCache * live_warmup_cache;
extern bool global_in_roi;
typedef uint32_t FlowID; // DInst.h

extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int qemuesesc_main(int argc, char **argv, char **envp);
extern "C" void esesc_set_timing(uint32_t fid);

extern "C" uint32_t QEMUReader_getFid(FlowID last_fid)
{
  return 0;
}

extern "C" void QEMUReader_finish_thread(uint32_t fid)
{
  
}

extern "C" uint64_t QEMUReader_get_time() 
{
  printf("Loaded: QEMUReader_get_time\n");
  return 0;
}

extern "C" uint32_t QEMUReader_setnoStats() 
{
  return 0;
}

extern "C" void QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2, uint16_t dest, void *env) 
{
  if(op == iLALU_LD) {
    live_warmup_cache->read(addr);
  } else if(op == iSALU_ST) {
    live_warmup_cache->write(addr);
  }
}

extern "C" void QEMUReader_finish(uint32_t fid)
{

}

extern "C" void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid)
{
}

extern "C" FlowID QEMUReader_resumeThreadGPU(FlowID uid) 
{
  return 0;
}

extern "C" FlowID QEMUReader_resumeThread(FlowID uid, FlowID last_fid) 
{
  static int conta=0;
  return conta++;
}

extern "C" void QEMUReader_pauseThread(FlowID id) {
}

extern "C" void QEMUReader_setFlowCmd(bool* flowStatus)
{

}

extern "C" void QEMUReader_start_roi(uint32_t fid)
{
  fprintf(stderr,"Rabbit: start_roi\n");
  global_in_roi = true;
}
