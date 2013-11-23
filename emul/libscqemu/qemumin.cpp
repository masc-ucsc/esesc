// Contributed by Jose Renau
//                Gabriel Southern
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
