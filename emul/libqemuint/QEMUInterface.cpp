/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2005 University California, Santa Cruz.

   Contributed by Gabriel Southern
                  Jose Renau
                  Sushant Kondguli

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

#include <pthread.h>

#include "InstOpcode.h"
#include "Instruction.h"
#include "Snippets.h"

#include "QEMUInterface.h"
#include "QEMUReader.h"
#include "EmuSampler.h"

EmuSampler *qsamplerlist[128];
//EmuSampler *qsampler = 0;
bool* globalFlowStatus = 0;

extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int qemuesesc_main(int argc, char **argv, char **envp);

extern "C" void *qemuesesc_main_bootstrap(void *threadargs) {

  static bool qemuStarted = false;
  if (!qemuStarted) {
    qemuStarted = true;
    QEMUArgs *qdata = (struct QEMUArgs *) threadargs;

    int    qargc = qdata->qargc;
    char **qargv = qdata->qargv;

    MSG("Starting qemu with");
    for(int i = 0; i < qargc; i++)
      MSG("arg[%d] is: %s",i,qargv[i]);

    qemuesesc_main(qargc,qargv,NULL);

    MSG("qemu done");

    QEMUReader_finish(0);

    pthread_exit(0);
  }else{
    MSG("QEMU already started! Ignoring the new start.");
  }

  return 0;
}

extern "C" uint32_t QEMUReader_getFid(FlowID last_fid)
{
  return qsamplerlist[last_fid]->getFid(last_fid);
}

extern "C" uint64_t QEMUReader_get_time() 
{
  return qsamplerlist[0]->getTime();
}

extern "C" uint64_t QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2, uint16_t dest, void *dummy) {
  I(fid<128); // qsampler statically sized to 128 at most

  //MSG("pc=%llx addr=%llx op=%d",pc,addr,op);
  return qsamplerlist[fid]->queue(pc,addr,fid,op,src1, src2, dest, LREG_InvalidOutput, dummy);
}

extern "C" void QEMUReader_finish(uint32_t fid)
{
  MSG("QEMUReader_finish(%d)",fid);
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
  qsamplerlist[fid]->terminate();
}

extern "C" void QEMUReader_finish_thread(uint32_t fid)
{
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
}

extern "C" void QEMUReader_start_roi(uint32_t fid)
{
  qsamplerlist[fid]->start_roi();
}

extern "C" void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid)
{
  qsamplerlist[fid]->syscall(num, usecs, fid);
}

#if 1
extern "C" FlowID QEMUReader_resumeThreadGPU(FlowID uid) {
  return(qsamplerlist[uid]->resumeThread(uid));
}

extern "C" FlowID QEMUReader_resumeThread(FlowID uid, FlowID last_fid) { 

  uint32_t fid = qsamplerlist[0]->getFid(last_fid); 
  MSG("resume %d -> %d",last_fid,fid);
  return(qsamplerlist[fid]->resumeThread(uid, fid));
}
extern "C" void QEMUReader_pauseThread(FlowID fid) {
  qsamplerlist[fid]->pauseThread(fid);
}

extern "C" void QEMUReader_setFlowCmd(bool* flowStatus) {

}
#endif

extern "C" int32_t QEMUReader_setnoStats(FlowID fid){
  return 0;
}


