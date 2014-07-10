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
  if (!qemuStarted)
  {
    qemuStarted = true;
    QEMUArgs *qdata = (struct QEMUArgs *) threadargs;

    int    qargc = qdata->qargc;
    char **qargv = qdata->qargv;

    MSG("Starting qemu with");
    for(int i = 0; i < qargc; i++)
      MSG("arg[%d] is: %s",i,qargv[i]);

    qemuesesc_main(qargc,qargv,NULL);

    MSG("qemu done");

    exit(0);
  }else{
    MSG("QEMU already started! Ignoring the new start.");
  }

  return 0;
}

extern "C" uint32_t QEMUReader_getFid(FlowID last_fid)
{
  return qsamplerlist[last_fid]->getFid(last_fid);
}

//Added by Hamid R. Khaleghzadeh///////////
extern "C" short int QEMUReader_fIDStatus(FlowID fid)
{
	return qsamplerlist[0]->fIDStatus(fid);	
}

extern "C" void QEMUReader_drainFIFO(FlowID fid)
{
	qsamplerlist[fid]->drainFIFO(fid);	
}
//end/////////////////////////////////////

extern "C" uint64_t QEMUReader_get_time() 
{
  //FIXME: fid?
  return qsamplerlist[0]->getTime();
}

//changed by Hamid R. Khaleghzadeh///////////////
extern "C" int QEMUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, char op, uint64_t icount, void *env) 
{
  return(qsamplerlist[fid]->queue(insn,pc,addr,fid,op,icount,env));
}

extern "C" void QEMUReader_finish(uint32_t fid)
{
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
  qsamplerlist[fid]->terminate();
//  qsamplerlist[fid]->freeFid(fid);
}

extern "C" void QEMUReader_finish_thread(uint32_t fid)
{
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
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
  //changed by Hamid R. Khaleghzadeh///////////
  MSG("*** resume (TID = %5d) , FIDs: %2d -> %2d",uid,last_fid,fid);
  return(qsamplerlist[fid]->resumeThread(uid, fid));
}

//Added by Hamid R. Khaleghzadeh///////////
extern "C" FlowID QEMUReader_resumeThreadNew(FlowID uid, FlowID last_fid, FlowID new_fid)
{
	I(QEMUReader_fIDStatus(new_fid) == 1);
	uint32_t fid = qsamplerlist[0]->getFid(new_fid); 
	MSG("*** resume (TID = %5d) , FIDs: %2d -> %2d",uid,last_fid,fid);
	return qsamplerlist[fid]->resumeThreadNew(uid, last_fid, fid);	
}

extern "C" size_t QEMUReader_getSCpuMaxFlows(size_t cpuid)
{
	return qsamplerlist[0]->getSCpuMaxFlows(cpuid);
} 

extern "C" size_t QEMUReader_getNumCpus()
{
	return qsamplerlist[0]->getNumCpus();
}
//end/////////////////////////////////////

extern "C" void QEMUReader_pauseThread(FlowID fid) {
  qsamplerlist[fid]->pauseThread(fid);
}

extern "C" void QEMUReader_setFlowCmd(bool* flowStatus) {

}
#endif

extern "C" int32_t QEMUReader_setnoStats(FlowID fid){
  return 0;
}


