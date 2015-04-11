/* License & includes {{{1 */
/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2005 University California, Santa Cruz.

  Contributed by Gabriel Southern
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

#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "EmuSampler.h"
#include "SescConf.h"
#include "QEMUReader.h"
//#include "SPARCInstruction.h"
#include "Snippets.h"
#include "callback.h"
#include "DInst.h"
#include "QEMUInterface.h"

/* }}} */

//live stuff
extern pthread_mutex_t mutex_live;
extern bool live_qemu_active;
extern pthread_cond_t cond_live;

#if 0
void *QEMUReader::getSharedMemory(size_t size) 
/* Allocate a shared memory region {{{1 */
{
  int shmid= shmget(0,size,IPC_CREAT|0666);
  if (shmid<0) {
    MSG("ERROR: Could not allocate shared memory for fifo (run ipcs and iprm if necessary)");
    exit(-3);
  }

  void *shared_mem_ptr = shmat(shmid,NULL,0);

  shmctl(shmid, IPC_RMID, 0);

  return shared_mem_ptr;
}
/* }}} */
#endif

bool QEMUReader::started = false;

QEMUReader::QEMUReader(QEMUArgs *qargs, const char *section, EmulInterface *eint_)
  /* constructor {{{1 */
  : Reader(section),
    qemuargs(qargs) {


  numFlows = 0;
  numAllFlows = 0;
  FlowID nemul = SescConf->getRecordSize("","cpuemul");
  for(FlowID i=0;i<nemul;i++) {
    const char *section = SescConf->getCharPtr("","cpuemul",i);
    const char *type    = SescConf->getCharPtr(section,"type");
    if(strcasecmp(type,"QEMU") == 0 ) {
      numFlows++;
    }
    numAllFlows++;
  }

  qemu_thread = -1;
  //started = false;
}
/* }}} */

void QEMUReader::start() 
/* Start QEMU Thread (wait until sampler is ready {{{1 */
{

  if (started)
    return;

  started = true;

#if 1
  MSG ("STARTING QEMU ......");
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  size_t stacksize = 1024*1024;
  pthread_attr_setstacksize(&attr, stacksize);

#if 0
  sigset_t mysigset;

  sigemptyset (&mysigset);
  sigaddset(&mysigset, SIGALRM);
  sigaddset(&mysigset, SIGHUP);
  sigaddset(&mysigset, SIGINT);
  sigaddset(&mysigset, SIGPIPE);

  pthread_sigmask (SIG_UNBLOCK, &mysigset, NULL);
#endif

  if (pthread_create(&qemu_thread, &attr, qemuesesc_main_bootstrap, (void *)qemuargs) != 0) {
    MSG("ERROR: pthread create failed");
    exit(-2);
  }
#else
  // Useful for debugging qemu (no threads)
  qemuesesc_main_bootstrap((void *)qemuargs);
#endif
  MSG("QEMUReader: Initializing qemu...");
}
/* }}} */

QEMUReader::~QEMUReader() {
  /* destructor {{{1 */
#if 0
  MSG("Killing group %d",getpgid(0));
  kill(-getpgid(0), SIGKILL);
#endif
}
/* }}} */

void QEMUReader::queueInstruction(AddrType pc, AddrType addr, FlowID fid, int op, int src1, int src2, int dest, int dest2, void * env, bool keepStats)
/* queue instruction (called by QEMU) {{{1 */
{
  uint64_t conta=0;

  if (tsfifo[fid].full()) {
    tsfifo_mutex_blocked[fid] = 1;
    pthread_mutex_lock(&tsfifo_mutex[fid]);
  }

  RAWDInst *rinst = tsfifo[fid].getTailRef();

  I(rinst);

  rinst->set(pc,addr,static_cast<InstOpcode>(op), static_cast<RegType>(src1),static_cast<RegType>(src2),static_cast<RegType>(dest), static_cast<RegType>(dest2), keepStats);
  
  tsfifo[fid].push();
 }
/* }}} */

void QEMUReader::syscall(uint32_t num, Time_t time, FlowID fid)
/* Create an syscall instruction and inject in the pipeline {{{1 */
{
  RAWDInst *rinst = tsfifo[fid].getTailRef();

  rinst->set(0xdeaddead,0,iRALU,LREG_R0,LREG_R0,LREG_InvalidOutput, LREG_InvalidOutput,true);

  tsfifo[fid].push();
}
// }}}

uint32_t QEMUReader::wait_until_FIFO_full(FlowID fid)
  // active wait for fifo full before read {{{1
{
  MSG("checking %d is %s",fid,tsfifo[fid].full()?"FULL":"Not FULL");
  while(!tsfifo[fid].full()) {
    pthread_yield();
    if (qsamplerlist[fid]->isActive(fid) == false)
      return 0;
  }
  MSG("done %d is %s",fid,tsfifo[fid].full()?"FULL":"Not FULL");
  pthread_mutex_unlock(&tsfifo_mutex[fid]);
  return 1;
}
// }}}

DInst *QEMUReader::executeHead(FlowID fid)
/* speculative advance of execution {{{1 */
{
  if (!ruffer[fid].empty()) {
    DInst *dinst = ruffer[fid].getHead();
    ruffer[fid].popHead();
    I(dinst); // We just added, there should be one or more
    return dinst;
  }

  int conta = 0;
  while(!tsfifo[fid].full()) {
    
    // FIXME: thead cond between push and pop (queue an executeHead)
    if (tsfifo_mutex_blocked[fid]) {
      tsfifo_mutex_blocked[fid] = 0;
      pthread_mutex_unlock(&tsfifo_mutex[fid]);
    }

    //live stuff
    pthread_mutex_lock(&mutex_live);
    if (!live_qemu_active) {
      //printf("------------- wait signal received\n");
      pthread_cond_wait(&cond_live, &mutex_live);
      //printf("------------- resume signal received\n");
    }
    pthread_mutex_unlock(&mutex_live);
    //printf("@%d",fid); fflush(stdout);

    if (qsamplerlist[fid]->isActive(fid) == false)
      return 0;

    if (conta++>100)
      return 0;
  }

  for(int i=32;i<tsfifo[fid].size();i++) {
    RAWDInst  *rinst = tsfifo[fid].getHeadRef();
    DInst **dinsth = ruffer[fid].getInsertPointRef();

    *dinsth = DInst::create(rinst->getInst(), rinst->getPC(), rinst->getAddr(), fid, rinst->getStatsFlag());

    ruffer[fid].add();
    tsfifo[fid].pop();
  }

  if (tsfifo_mutex_blocked[fid]) {
    tsfifo_mutex_blocked[fid] = 0;
    pthread_mutex_unlock(&tsfifo_mutex[fid]);
  }

  if (ruffer[fid].empty())
    return 0;

  DInst *dinst = ruffer[fid].getHead();
  ruffer[fid].popHead();
  I(dinst); // We just added, there should be one or more
  return dinst;
}
/* }}} */

void QEMUReader::reexecuteTail(FlowID fid) {
  /* safe/retire advance of execution {{{1 */

  ruffer[fid].advanceTail();
}
/* }}} */

void QEMUReader::syncHeadTail(FlowID fid){
  /* replay triggers a syncHeadTail {{{1 */

  ruffer[fid].moveHead2Tail();
  //ruffer[fid].advanceTail(); // Make sure that the same inst is not re-exec again
 
}
/* }}} */

void QEMUReader::drainFIFO(FlowID fid)
/* Drain the tsfifo as much as possible due to a mode change {{{1 */
{
  I(0); // not needed for the moment
  if (pthread_equal(qemu_thread,pthread_self()))
    return;

  while(!tsfifo[fid].empty() && !ruffer[fid].empty()) {
    pthread_yield();
  }
}
/* }}} */

