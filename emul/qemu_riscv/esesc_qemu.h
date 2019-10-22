#ifndef ESESCQEMUBM_H
#define ESESCQEMUBM_H

uint64_t QEMUReader_get_time(void);
uint32_t QEMUReader_cpu_start(uint32_t cpuid);
uint32_t QEMUReader_cpu_stop(uint32_t cpuid);
uint32_t QEMUReader_getFid(uint32_t last_fid); //FIXME: use FlowID instead of unint32_t
int32_t QEMUReader_setnoStats(uint32_t fid);

uint64_t QEMUReader_queue_store(uint64_t pc, uint64_t addr, uint64_t data_new, uint64_t data_old, uint16_t fid, uint16_t src1, uint16_t src2, uint16_t dest);
uint64_t QEMUReader_queue_load(uint64_t pc, uint64_t addr, uint64_t data, uint16_t fid, uint16_t src1, uint16_t dest);
uint64_t QEMUReader_queue_inst(uint64_t pc, uint64_t addr, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2, uint16_t dest);
uint64_t QEMUReader_queue_ctrl_data(uint64_t pc, uint64_t addr, uint64_t data1, uint64_t data2, uint16_t fid, uint16_t op, uint16_t src1, uint16_t src2, uint16_t dest);

void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid);
void QEMUReader_finish(uint32_t fid);
void QEMUReader_finish_thread(uint32_t fid);
uint32_t QEMUReader_resumeThread(uint32_t fid, uint32_t last_fid);
void QEMUReader_pauseThread(uint32_t fid);
int QEMUReader_toggle_roi(uint32_t fid);

void esesc_set_rabbit(uint32_t fid);
void esesc_set_warmup(uint32_t fid);
void esesc_set_timing(uint32_t fid);

#endif
