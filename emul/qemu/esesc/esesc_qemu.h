#ifndef ESESCQEMU_H
#define ESESCQEMU_H
// Implemented in QEMUInterface.cpp

typedef uint32_t T_CONTEXT_32;
typedef uint32_t T_SHARED_32;
typedef uint64_t T_SHARED_64;
typedef uint64_t T_CONTEXT_64;
//Can be either 32 bit or 64 depending on the architecture.
//But since we are using ARM now and we know ARM is 32 bit,
// we are using a 32 bit pointer only.
typedef uint32_t T_TRACE;
typedef uint32_t T_SHARED_ADDR;


uint64_t QEMUReader_get_time(void);
uint32_t QEMUReader_getFid(uint32_t last_fid); //FIXME: use FlowID instead of unint32_t
void QEMUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, uint32_t op, uint64_t icount, void *env);
int32_t QEMUReader_setnoStats(uint32_t fid);
// icount: # instruction executed
//    -must be 1 during TIMING and DETAIL modeling
//    -must be less than 64 during RABBIT MODE
//    -must be less than 64 AND have a single LD/ST or Branch at the end of the block during WARMUP

// op: quick predecode of the instruction ONLY used during WARMUP mode.
//    0 : other
//    1 : LD
//    2 : ST
//    3 : BRANCH

void QEMUReader_syscall(uint32_t num, uint64_t usecs, uint32_t fid);
//void QEMUReader_suspend(uint32_t fid);
//void QEMUReader_resume(uint32_t fid);
void QEMUReader_finish(uint32_t fid);
void QEMUReader_finish_thread(uint32_t fid);
uint32_t QEMUReader_resumeThread(uint32_t fid, uint32_t last_fid);
void QEMUReader_pauseThread(uint32_t fid);
int QEMUReader_is_sampler_done(uint32_t fid);

void esesc_set_rabbit(uint32_t fid);
void esesc_set_warmup(uint32_t fid);
void esesc_set_timing(uint32_t fid);

uint32_t esesc_iload(uint32_t);

uint64_t esesc_ldu64(uint32_t addr);
uint32_t esesc_ldu32(uint32_t addr);
uint16_t esesc_ldu16(uint32_t addr);
 int16_t esesc_lds16(uint32_t addr);
uint8_t  esesc_ldu08(uint32_t addr);
 int8_t  esesc_lds08(uint32_t addr);

void esesc_st64(uint32_t addr);
void esesc_st32(uint32_t addr);
void esesc_st16(uint32_t addr);
void esesc_st08(uint32_t addr);

// defined in exec-all.h
//extern int esesc_allow_large_tb;
//extern int esesc_single_inst_tb;

#if defined(CONFIG_ESESC_CUDA)
// Implemented in GPUInterface.cpp
void     GPUReader_init_trace(uint32_t* h_trace);
void     GPUReader_declareLaunch(void);

void     GPUReader_queue_inst(uint32_t insn, uint32_t pc, uint32_t addr, uint32_t fid, char op, uint64_t icount, void* env);
uint32_t GPUReader_decode_trace(uint32_t* h_trace, uint32_t* qemuid, void* env); //returns if the execution should be paused or no

uint64_t  GPUReader_getNumThreads(void);
uint64_t  GPUReader_getBlockThreads(void);
uint32_t  GPUReader_setNumThreads(uint64_t binnumthreads, uint64_t binblockthreads);

uint32_t  GPUReader_getTracesize(void);

void GPUReader_mallocAddress(uint32_t dev_addr, uint32_t size, uint32_t* qemuid );
void GPUReader_mapcudaMemcpy(uint32_t dev_addr, uint32_t host_addr, uint32_t size, uint32_t kind, void *env, uint32_t* cpufid);

void GPUReader_setCurrentKernel(const char* local_kernel_name);
void GPUReader_getKernelParameters(uint32_t* DFL_REG32, uint32_t* DFL_REG64, uint32_t* DFL_REGFP, uint32_t* DFL_REGFP64, uint32_t* SM_REG32, uint32_t* SM_REG64, uint32_t* SM_REGFP, uint32_t* SM_REGFP64, uint32_t* SM_ADDR , uint32_t* TRACESIZE);
void GPUReader_CUDA_exec_done(void);

void esesc_set_rabbit_gpu(void);
void esesc_set_warmup_gpu(void);
void esesc_set_timing_gpu(void);

uint32_t GPUReader_reexecute_same_kernel(void);
#if 0
uint32_t GPUReader_getNumKernels();
bool     GPUReader_isLearning();
#endif

#endif



#endif
