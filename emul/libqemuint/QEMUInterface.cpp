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

#include "EmuSampler.h"
#include "QEMUInterface.h"
#include "QEMUReader.h"

//#define DEBUG_QEMU_TRACE 1

EmuSampler *qsamplerlist[128];
// EmuSampler *qsampler = 0;
bool *globalFlowStatus = 0;

extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int  qemuesesc_main(int argc, char **argv, char **envp);
extern "C" uint64_t esesc_mem_read(uint64_t addr);

extern "C" void *qemuesesc_main_bootstrap(void *threadargs) {

  static bool qemuStarted = false;
  if(!qemuStarted) {
    qemuStarted     = true;
    QEMUArgs *qdata = (struct QEMUArgs *)threadargs;

    int    qargc = qdata->qargc;
    char **qargv = qdata->qargv;

    MSG("Starting qemu with");
    for(int i = 0; i < qargc; i++)
      MSG("arg[%d] is: %s", i, qargv[i]);

    qemuesesc_main(qargc, qargv, NULL);

    MSG("qemu done");

    QEMUReader_finish(0);

    pthread_exit(0);
  } else {
    MSG("QEMU already started! Ignoring the new start.");
  }

  return 0;
}

extern "C" uint32_t QEMUReader_getFid(FlowID last_fid) {
  return qsamplerlist[last_fid]->getFid(last_fid);
}

extern "C" uint64_t QEMUReader_get_time() {
  return qsamplerlist[0]->getTime();
}

#ifdef DEBUG_QEMU_TRACE
uint64_t last_addr=0;
#endif

extern "C" uint64_t QEMUReader_queue_load(uint64_t pc, uint64_t addr, uint64_t data, uint16_t fid, uint16_t src1, uint16_t dest) {
  I(fid < 128); // qsampler statically sized to 128 at most

  // I(qsamplerlist[fid]->isActive(fid) || EmuSampler::isTerminated());

#ifdef DEBUG_QEMU_TRACE
  //uint64_t raw = esesc_mem_read(pc);
  MSG("load pc=%llx addr=%llx data=%u op=%d cpu=%d src:%d dst:%d",pc,addr,data,iLALU_LD,fid,src1,dest);

  I(pc == (last_addr+2) || pc == (last_addr+4) || last_addr==0);
  last_addr = pc;
#endif

  uint64_t res = qsamplerlist[fid]->queue(pc, addr, data, fid, iLALU_LD, src1, 0, dest, LREG_InvalidOutput);
  return res;
}

extern "C" uint64_t QEMUReader_queue_store(uint64_t pc, uint64_t addr, uint64_t data_new, uint64_t data_old, uint16_t fid, uint16_t src1, uint16_t src2, uint16_t dest) {
  I(fid < 128); // qsampler statically sized to 128 at most

  // I(qsamplerlist[fid]->isActive(fid) || EmuSampler::isTerminated());

#ifdef DEBUG_QEMU_TRACE
  //uint64_t raw = esesc_mem_read(pc);
  MSG("store pc=%llx addr=%llx data_new=%u data_old=%u op=%d cpu=%d src1:%d src2:%d dst:%d",pc,addr,data_new,data_old,iSALU_ST,fid,src1,src2,dest);

  I(pc == (last_addr+2) || pc == (last_addr+4) || last_addr==0);
  last_addr = pc;
#endif

  uint64_t res = qsamplerlist[fid]->queue(pc, addr, data_new, fid, iSALU_ST, src1, src2, dest, LREG_InvalidOutput, data_old);
  return res;
}
extern "C" uint64_t QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2,
                                          uint16_t dest) {
  I(fid < 128); // qsampler statically sized to 128 at most

  // I(qsamplerlist[fid]->isActive(fid) || EmuSampler::isTerminated());
#ifdef DEBUG_QEMU_TRACE
  MSG("pc=%llx addr=%llx op=%d cpu=%d",pc,addr,op,fid);

  I(pc == (last_addr+2) || pc == (last_addr+4) || last_addr==0);
  last_addr = pc;
  if (addr && op >= iBALU_LBRANCH && op <= iBALU_RET)
   last_addr = addr - 4; // fake -4 so that next check works
#endif
  uint64_t res = qsamplerlist[fid]->queue(pc, addr, 0, fid, op, src1, src2, dest, LREG_InvalidOutput);
  return res;
}
extern "C" uint64_t QEMUReader_queue_ctrl_data(uint64_t pc, uint64_t addr, uint64_t data1, uint64_t data2, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2,
                                          uint16_t dest) {
  I(fid < 128); // qsampler statically sized to 128 at most

  // I(qsamplerlist[fid]->isActive(fid) || EmuSampler::isTerminated());
#ifdef DEBUG_QEMU_TRACE
  uint64_t raw = esesc_mem_read(pc);
  MSG("ctrl pc=%llx addr=%llx data1=%llx data2=%llx op=%d cpu=%d raw=%llx",pc,addr,data1,data2,op,fid, raw);

  I(pc == (last_addr+2) || pc == (last_addr+4) || last_addr==0);
  last_addr = pc;
  if (addr && op >= iBALU_LBRANCH && op <= iBALU_RET)
   last_addr = addr - 4; // fake -4 so that next check works
#endif
  uint64_t res = qsamplerlist[fid]->queue(pc, addr, data1, fid, op, src1, src2, dest, LREG_InvalidOutput, data2);
  return res;
}

extern "C" void QEMUReader_finish(uint32_t fid) {
  MSG("QEMUReader_finish(%d)", fid);
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
  qsamplerlist[fid]->terminate();
}

extern "C" void QEMUReader_finish_thread(uint32_t fid) {
  MSG("QEMUReader_finish_thread(%d)", fid);
  qsamplerlist[fid]->stop();
  qsamplerlist[fid]->pauseThread(fid);
  qsamplerlist[0]->freeFid(fid);
}

extern "C" int QEMUReader_toggle_roi(uint32_t fid) {
  return qsamplerlist[fid]->toggle_roi()?1:0;
}

extern "C" void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid) {
  qsamplerlist[fid]->syscall(num, usecs, fid);
}

extern "C" FlowID QEMUReader_resumeThreadGPU(FlowID uid) {
  return (qsamplerlist[uid]->resumeThread(uid));
}

extern "C" FlowID QEMUReader_cpu_start(uint32_t cpuid) {
#if 1
  static bool initialized = false;
  MSG("QEMUReader_cpu_start(%d)",cpuid);
  if (!initialized) {
    I(cpuid==0);
    initialized = true;
    return 0;
  }
  qsamplerlist[0]->setFid(cpuid);
  return (qsamplerlist[cpuid]->resumeThread(cpuid, cpuid));
#endif
  return 0;
}

extern "C" FlowID QEMUReader_cpu_stop(uint32_t cpuid) {
#if 1
  // MSG("cpu_stop %d",cpuid);
  qsamplerlist[cpuid]->pauseThread(cpuid);
  return cpuid;
#endif
  return 0; 
}

extern "C" FlowID QEMUReader_resumeThread(FlowID uid, FlowID last_fid) {
  uint32_t fid = qsamplerlist[0]->getFid(last_fid);
  MSG("resume %d -> %d", last_fid, fid);
  return (qsamplerlist[fid]->resumeThread(uid, fid));
}
extern "C" void QEMUReader_pauseThread(FlowID fid) {
  qsamplerlist[fid]->pauseThread(fid);
  qsamplerlist[0]->freeFid(fid);
}

extern "C" void QEMUReader_setFlowCmd(bool *flowStatus) {
}

extern "C" int32_t QEMUReader_setnoStats(FlowID fid) {
  return 0;
}
