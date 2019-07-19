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

#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include "EmuSampler.h"
#include "QEMUReader.h"
#include "SescConf.h"
//#include "SPARCInstruction.h"
#include "DInst.h"
#include "QEMUInterface.h"
#include "Snippets.h"
#include "callback.h"

/* }}} */

pthread_mutex_t mutex_ctrl;

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
    : Reader(section)
    , qemuargs(qargs) {

  numFlows     = 0;
  numAllFlows  = 0;
  FlowID nemul = SescConf->getRecordSize("", "cpuemul");
  for(FlowID i = 0; i < nemul; i++) {
    const char *section = SescConf->getCharPtr("", "cpuemul", i);
    const char *type    = SescConf->getCharPtr(section, "type");
    if(strcasecmp(type, "QEMU") == 0) {
      numFlows++;
    }
    numAllFlows++;
  }

  // qemu_thread = -1;
  // started = false;
}
/* }}} */

void QEMUReader::start()
/* Start QEMU Thread (wait until sampler is ready {{{1 */
{
  if(started)
    return;

  pthread_mutex_init(&mutex_ctrl, 0);

  started = true;

#if 1
  MSG("STARTING QEMU ......");
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  size_t stacksize = 1024 * 1024;
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

  if(pthread_create(&qemu_thread, &attr, qemuesesc_main_bootstrap, (void *)qemuargs) != 0) {
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

void QEMUReader::queueInstruction(AddrType pc, AddrType addr, DataType data, FlowID fid, int op, int src1, int src2, int dest,
                                  int dest2, bool keepStats, DataType data2)
/* queue instruction (called by QEMU) {{{1 */
{
  uint64_t conta = 0;

  I(src1 < LREG_MAX);
  I(src2 < LREG_MAX);
  I(dest < LREG_MAX);
  I(dest2 < LREG_MAX);

  while(tsfifo[fid].full()) {
    if(qsamplerlist[fid]->isActive(fid) == false) {
      qsamplerlist[fid]->resumeThread(fid, fid);
    }
    pthread_mutex_lock(&mutex_ctrl); // BEGIN

#if 0
    if (tsfifo_rcv_mutex_blocked) {
      tsfifo_rcv_mutex_blocked = 0;
      pthread_mutex_unlock(&tsfifo_rcv_mutex);
      //MSG("1.alarmto rcv%d",fid);
    }
#endif

    bool doblock = false;
    if(tsfifo_snd_mutex_blocked[fid] == 0) {
      tsfifo_snd_mutex_blocked[fid] = 1;
      doblock                       = true;
    }
    pthread_mutex_unlock(&mutex_ctrl); // END

    // MSG("2.sleep  snd%d",fid);
    if(doblock) {
      pthread_mutex_lock(&tsfifo_snd_mutex[fid]);
    } else {
      I(0);
    }
    // MSG("2.wakeup snd%d",fid);
  }

  RAWDInst *rinst = tsfifo[fid].getTailRef();

  I(rinst);

  rinst->set(pc, addr, static_cast<InstOpcode>(op), static_cast<RegType>(src1), static_cast<RegType>(src2),
             static_cast<RegType>(dest), static_cast<RegType>(dest2), keepStats);
#ifdef ESESC_TRACE_DATA
  rinst->setData(data);
  rinst->setData2(data2);
#endif

  tsfifo[fid].push();
}
/* }}} */

void QEMUReader::syscall(uint32_t num, Time_t time, FlowID fid)
/* Create an syscall instruction and inject in the pipeline {{{1 */
{
  RAWDInst *rinst = tsfifo[fid].getTailRef();

  rinst->set(0xdeaddead, 0, iRALU, LREG_R0, LREG_R0, LREG_InvalidOutput, LREG_InvalidOutput, true);

  tsfifo[fid].push();
}
// }}}

uint32_t QEMUReader::wait_until_FIFO_full(FlowID fid)
// active wait for fifo full before read {{{1
{
  MSG("OOPS");
  I(0);
  return 1;
}
// }}}

bool QEMUReader::populate(FlowID fid) {
  int conta = 0;
  if(ruffer[fid].size() > 1024) // No need to overpopulate the queues
    return true;

  if(!tsfifo[fid].halfFull()) {
    pthread_mutex_lock(&mutex_ctrl); // BEGIN

    bool unblocked = false;

    if(tsfifo_snd_mutex_blocked[fid]) {
      tsfifo_snd_mutex_blocked[fid] = 0;
      pthread_mutex_unlock(&tsfifo_snd_mutex[fid]);
      // MSG("2.alarmt snd%d",fid);
      unblocked = true;
    }
#if 0
    if (tsfifo_rcv_mutex_blocked == 0) {
      tsfifo_rcv_mutex_blocked = 1;
      pthread_mutex_unlock(&mutex_ctrl); // END

      //MSG("1.sleep  rcv%d",fid);
      pthread_mutex_lock(&tsfifo_rcv_mutex);
      //MSG("1.wakeup rcv%d",fid);
    }else{
      pthread_mutex_unlock(&mutex_ctrl); // END
    }
#else
    pthread_mutex_unlock(&mutex_ctrl); // END
#endif

    if(unblocked)
      return true;

    if(qsamplerlist[fid]->isActive(fid) == false) {
      // MSG("DOWN");
      return true;
    }

    for(int i = 0; i < numFlows; i++) {
      if(tsfifo_snd_mutex_blocked[i] == 0)
        continue;

      if(qsamplerlist[i]->isActive(i) == false) {
#ifdef DEBUG
        MSG("Deadlock avoidance: sleeping thread was not active");
#endif
        qsamplerlist[i]->resumeThread(i, i);
      } else {
#ifdef DEBUG2
        MSG("Slowlock: sleeping thread was active");
#endif
      }
    }

    return false;
  }

  I(tsfifo[fid].halfFull());

  for(int i = 32; i < tsfifo[fid].size(); i++) {
    RAWDInst *rinst  = tsfifo[fid].getHeadRef();
    DInst **  dinsth = ruffer[fid].getInsertPointRef();

    *dinsth = DInst::create(rinst->getInst(), rinst->getPC(), rinst->getAddr(), fid, rinst->getStatsFlag());
#ifdef ESESC_TRACE_DATA
    (*dinsth)->setData(rinst->getData());
    (*dinsth)->setData2(rinst->getData2());
#if 0
    if (rinst->getPC()==0x142cc) {
      MSG("pc=142cc ID=%d d1=%d d2=%d d1=%d d2=%d\n",(int)(*dinsth)->getID(), (int)rinst->getData(), (int)rinst->getData2(), (int)(*dinsth)->getData(), (int)(*dinsth)->getData2());
    }
#endif
#endif

    ruffer[fid].add();
    tsfifo[fid].pop();
  }

  pthread_mutex_lock(&mutex_ctrl); // BEGIN
  if(tsfifo_snd_mutex_blocked[fid]) {
    tsfifo_snd_mutex_blocked[fid] = 0;
    pthread_mutex_unlock(&tsfifo_snd_mutex[fid]);
  }
  pthread_mutex_unlock(&mutex_ctrl); // END

  return true;
}

DInst *QEMUReader::peekHead(FlowID fid) {

  do {

    if(!ruffer[fid].empty()) {
      DInst *dinst = ruffer[fid].getHead();
      I(dinst); // We just added, there should be one or more
#ifdef FETCH_TRACE
      if(trace.is_trace_entry(dinst)) {
        // get trace, get next 256 or end trace (may need to call populate),
        // check if matches trace.  If trace matches. Push OPTIMIZED trace.
        // If missmatch, push optimized which should trigger an abort, and
        // original after.
      }
#endif
      return dinst;
    }

    bool p = populate(fid);
    if(!p)
      return 0;

  } while(!ruffer[fid].empty());

  return 0;
}

DInst *QEMUReader::executeHead(FlowID fid)
/* speculative advance of execution {{{1 */
{
  do {

    if(!ruffer[fid].empty()) {
      DInst *dinst = ruffer[fid].getHead();
      ruffer[fid].popHead();
      I(dinst); // We just added, there should be one or more
      return dinst;
    }

    bool p = populate(fid);
    if(!p)
      return 0;

  } while(!ruffer[fid].empty());

  return 0;
}
/* }}} */

void QEMUReader::reexecuteTail(FlowID fid) {
  /* safe/retire advance of execution {{{1 */

  ruffer[fid].advanceTail();
}
/* }}} */

void QEMUReader::syncHeadTail(FlowID fid) {
  /* replay triggers a syncHeadTail {{{1 */

  ruffer[fid].moveHead2Tail();
  // ruffer[fid].advanceTail(); // Make sure that the same inst is not re-exec again
}
/* }}} */

void QEMUReader::drainFIFO(FlowID fid)
/* Drain the tsfifo as much as possible due to a mode change {{{1 */
{
  I(0); // not implemented
}
/* }}} */
