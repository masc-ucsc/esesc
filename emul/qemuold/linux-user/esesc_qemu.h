#ifndef ESESCQEMU_H
#define ESESCQEMU_H

uint64_t QEMUReader_get_time(void);
uint32_t QEMUReader_getFid(uint32_t last_fid); //FIXME: use FlowID instead of unint32_t
void QEMUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, uint32_t op, uint64_t icount, void *env);
int32_t QEMUReader_setnoStats(uint32_t fid);

void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid);
void QEMUReader_finish(uint32_t fid);
void QEMUReader_finish_thread(uint32_t fid);
uint32_t QEMUReader_resumeThread(uint32_t fid, uint32_t last_fid);
void QEMUReader_pauseThread(uint32_t fid);
void QEMUReader_start_roi(uint32_t fid);

void esesc_set_rabbit(uint32_t fid);
void esesc_set_warmup(uint32_t fid);
void esesc_set_timing(uint32_t fid);

#endif
