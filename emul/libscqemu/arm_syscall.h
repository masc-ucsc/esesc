
uint32_t do_syscall_arm(ProgramBase *prog, uint32_t num, uint32_t arg1,uint32_t arg2, uint32_t arg3,uint32_t arg4,uint32_t arg5, uint32_t arg6, FILE *syscallTrace);

void target_set_brk_arm(uint32_t addr);
