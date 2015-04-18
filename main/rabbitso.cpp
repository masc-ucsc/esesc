/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau
Gabriel Southern

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
  // call (*live_queue_inst)(insn,pc,addr,fid,op,icount,env)
  //
  // if live_queue_inst == 0 ; do an active loop waiting (this is done by 
  // while(live_queue_inst==0)
  //   usleep(10);
  //
  // If live_queue_inst is set, make sure that we do not run more than the checkpoint distance. Otherwise, it will overlap execution between checkpoints. It is OK just ot kill iself afternotifying node that it is done
  /*if ((pc > 0xeb90 && pc < 0xeb9f) || addr == 0x407fdb1c ) {
    printf("%d pc=0x%x insn=0x%x op=%d icount=%d addr=0x%lx data=0x%lx\n",fid,pc,insn, op,(uint32_t)icount, addr, data);
  } */
}

extern "C" void QEMUReader_finish(uint32_t fid)
{

   // call (*live_finish)(fid)
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
