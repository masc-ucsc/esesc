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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t FlowID; // DInst.h

extern "C" void QEMUReader_goto_sleep(void *env);
extern "C" void QEMUReader_wakeup_from_sleep(void *env);
extern "C" int  qemuesesc_main(int argc, char **argv, char **envp);
extern "C" void esesc_set_timing(uint32_t fid);

void start_qemu(int argc, char **argv) {
  char **qargv = (char **)malloc(argc * sizeof(char **));

  qargv[0] = (char *)"qemu";
  for(int j = 1; j < argc; j++) {
    qargv[j] = strdup(argv[j]);
  }

  qemuesesc_main(argc, qargv, 0);
}

extern "C" uint32_t QEMUReader_getFid(FlowID last_fid) {
  return 0;
}

extern "C" void QEMUReader_finish_thread(uint32_t fid) {
  printf("QEMUReader_finishThread(%d)\n",fid);
}

extern "C" uint64_t QEMUReader_get_time() {
  return 0;
}

extern "C" uint32_t QEMUReader_setnoStats() {
  return 0;
}

extern "C" void QEMUReader_toggle_roi(uint32_t fid) {
}

extern "C" uint64_t QEMUReader_queue_load(uint64_t pc, uint64_t addr, uint64_t data, uint16_t fid, uint16_t src1, uint16_t dest) {
  return 0;
}
extern "C" void QEMUReader_queue_inst(uint64_t pc, uint64_t addr, int fid, int op, int src1, int src2, int dest, void *env) {
  // printf("%d pc=0x%llx addr=0x%llx op=%d src1=%d src2=%d dest=%d\n",fid,(long long)pc,(long long)addr, op, src1, src2, dest);
}

extern "C" void QEMUReader_finish(uint32_t fid) {
  printf("QEMUReader_finish(%d)\n",fid);
  exit(0);
}

extern "C" void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid) {
}

extern "C" FlowID QEMUReader_resumeThread(FlowID uid, FlowID last_fid) {
  static int conta = 0;
  printf("QEMUReader_resumeThread(%d,%d)\n",uid,last_fid);
  return conta++;
}

extern "C" void QEMUReader_pauseThread(FlowID fid) {
  printf("QEMUReader_pauseThread(%d)\n",fid);
}

extern "C" void QEMUReader_setFlowCmd(bool *flowStatus) {
}

extern "C" uint32_t QEMUReader_cpu_start(uint32_t cpuid) {
  static int cpu_counter=0;
  printf("QEMUReader_cpu_start(%d)\n",cpuid);
  return cpu_counter++;
}

extern "C" void QEMUReader_cpu_stop(uint32_t cpuid) {
  printf("QEMUReader_cpu_stop(%d)\n",cpuid);
}

int main(int argc, char **argv) {

  //esesc_set_timing(0);

  start_qemu(argc, argv);
}
