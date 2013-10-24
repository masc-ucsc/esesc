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
#if 0
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#ifdef USE_RTL
#include "vpi_user.h"
#include "veriuser.h"
#endif

#include "qemumin.h"
#include "nanassert.h"
extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int qemuesesc_main(int argc, char **argv, char **envp);

QEMUArgs *qargs ;
volatile int done = 0;
static volatile int awake = 0;

uint32_t starting_pc=0xdeaddead;

extern "C" {
  void *qemuesesc_main_bootstrap(void *threadargs) {

    QEMUArgs *qdata = (struct QEMUArgs *) threadargs;

    int    qargc    = qdata->qargc;
    char **qargv    = qdata->qargv;

    MSG("QEMU:Starting with");
    for(int i = 0; i < qargc; i++)
      MSG("  arg[%d] is: %s\n",i,qargv[i]);

    qemuesesc_main(qargc,qargv,0);
    return NULL;
  }
}

void start_qemu(int argc, char **argv) 
{
  qargs = (QEMUArgs *) malloc(sizeof(QEMUArgs));

  qargs->qargc = argc;
  qargs->qargv = argv;

  pthread_attr_t attr;
  pthread_attr_init(&attr);

  size_t stacksize = 1024*1024;
  pthread_attr_setstacksize(&attr, stacksize);

  pthread_t qemu_thread;

  if (pthread_create(&qemu_thread, &attr, qemuesesc_main_bootstrap, (void *)qargs) != 0) {
    MSG("ERROR: pthread create failed\n");
    exit(-2);
  }



  while(done==0){
    usleep(100);
  }
  //qemuesesc_syscall(qargs->env, 0);
  MSG("QEMU:done...\n");
}


extern "C" uint32_t QEMUReader_getFid(FlowID last_fid)
{
  return 0;
}

extern "C" uint64_t QEMUReader_get_time() 
{
  return 0;
}

extern "C" void QEMUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t data, uint32_t fid, char op, uint64_t icount, void *env) 
{
  starting_pc = pc;
  MSG("QEMU:awake reached. Starting pc=0x%x\n", pc);
  awake = 1;
  pthread_exit(0); // Just wanted to initialize qemu, 
  //printf("pc=0x%x addr=0x%x data=0x%x op=%d\n",pc,addr,data,op);
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
  //static uint64_t fid = 0; //FIXME: a hack for now, need fid pool later.
  return last_fid;//++fid;
}

extern "C" void QEMUReader_pauseThread(FlowID fid) {
}

extern "C" void QEMUReader_setFlowCmd(bool* flowStatus)
{

}

#endif
