#include "cpu.h"
#include "dyngen-exec.h"
#include "helper.h"

#if !defined(CONFIG_USER_ONLY)
static void do_unaligned_access(target_ulong addr, int is_write, int is_user,
                                void *retaddr);

#define MMUSUFFIX _mmu
#define ALIGNED_ONLY

#define SHIFT 0
#include "softmmu_template.h"

#define SHIFT 1
#include "softmmu_template.h"

#define SHIFT 2
#include "softmmu_template.h"

#define SHIFT 3
#include "softmmu_template.h"

/* XXX: make it generic ? */
static void cpu_restore_state2(void *retaddr)
{
    TranslationBlock *tb;
    unsigned long pc;

    if (retaddr) {
        /* now we have a real cpu fault */
        pc = (unsigned long)retaddr;
        tb = tb_find_pc(pc);
        if (tb) {
            /* the PC is inside the translated code. It means that we have
               a virtual CPU fault */
            cpu_restore_state(tb, env, pc);
        }
    }
}

static void do_unaligned_access(target_ulong addr, int is_write, int is_user,
                                void *retaddr)
{
#ifdef DEBUG_UNALIGNED
    printf("Unaligned access to 0x" TARGET_FMT_lx " from 0x" TARGET_FMT_lx
           "\n", addr, env->pc);
#endif
    cpu_restore_state2(retaddr);
    helper_raise_exception(env, TT_UNALIGNED);
}

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
/* XXX: fix it to restore all registers */
void tlb_fill(CPUState *env1, target_ulong addr, int is_write, int mmu_idx,
              void *retaddr)
{
    int ret;
    CPUState *saved_env;

    saved_env = env;
    env = env1;

    ret = cpu_sparc_handle_mmu_fault(env, addr, is_write, mmu_idx);
    if (ret) {
        cpu_restore_state2(retaddr);
        cpu_loop_exit(env);
    }
    env = saved_env;
}

#endif /* !CONFIG_USER_ONLY */
#ifdef CONFIG_ESESC_CUDA
uint64_t  helper_ld_leswap(target_ulong addr, int size, int sign) 
{
  uint64_t ret = 0;
  uint64_t flip = 0;
  helper_check_align(addr,size-1); //Needed?
  //address_mask(env, &addr); // Needed?
  
  if(env->current_tb && addr) 
  {
    if ((addr >= env->le_memory_start_addr) && 
        (addr < env->le_memory_start_addr+env->le_memory_size)){
         //printf("--------------------------------->%u <  %u < %u \n",env->le_memory_start_addr,addr, env->le_memory_start_addr+env->le_memory_size);
    //if (addr > )){
      flip = 1; 
    } else {
      flip = 0;
    }
  } else {
      flip = 0;
  }

  switch(size) {
    case 1:
      ret = ldub_raw(addr);
      break;
    case 2:
      ret = lduw_raw(addr);
      break;
    case 4:
      ret = ldl_raw(addr);
      break;
    default:
    case 8:
      ret = ldq_raw(addr);
      break;
  }
  
  if (flip) {
    switch(size) {
      case 8:
          ret = bswap64(ret);
          //printf("8 bytes\n");
          break;
      case 2:
          ret = bswap16(ret);
          //printf("2 bytes\n");
          break;
      case 4:
          //if (flip) printf("------- 0x%x\n",(uint32_t)ret);
          ret = bswap32(ret);
          //if (flip) printf("------- 0x%x\n",(uint32_t)ret);
          //printf("4 bytes\n");
          break;
      default:
          break;
      }
  }
 
  if (sign) {
    switch(size) {
      case 8:
          ret = (int8_t) ret;
          break;
      case 2:
          ret = (int16_t) ret;
          break;
      case 4:
          ret = (int32_t) ret;
          break;
      default:
          break;
      }
  }
  return ret;
}

void helper_st_leswap(target_ulong addr, target_ulong val, int size){
  uint64_t flip = 0;
  
  if(env->current_tb && addr) 
  {
    if ((addr >= env->le_memory_start_addr) && 
        (addr <= env->le_memory_start_addr+env->le_memory_size)){
      flip = 1; 
    } else {
      flip = 0;
    }
  } else {
      flip = 0;
  }
  //flip = 0; 
  if (flip) {
    switch(size) {
      case 8:
          val = bswap64(val);
          break;
      case 2:
          val = bswap16(val);
          break;
      case 4:
          //if (flip) printf("------- 0x%x\n",val);
          val = bswap32(val);
          break;
      default:
          break;
      }
  } else {
    helper_check_align(addr,size-1); //Needed?
  }

  switch(size) {
    case 1:
        //if (flip) printf("------- 0x%x\n",val);
        stb_raw(addr, val);
        break;
    case 2:
        //if (flip) printf("------- 0x%x\n",val);
        stw_raw(addr, val);
        break;
    case 4:
        //if (flip) printf("------- 0x%x\n",val);
        stl_raw(addr, val);
        break;
    case 8:
    default:
        //if (flip) printf("------- 0x%x\n",val);
        stq_raw(addr, val);
        break;
    }
}

void helper_stf_leswap(target_ulong addr , int size, int rd)
{
    target_ulong val = 0;

    helper_check_align(addr, 3);
    
    switch(size) {
    default:
    case 4:
        val = *((uint32_t *)&env->fpr[rd]);
        break;
    case 8:
        val = *((int64_t *)&DT0);
        break;
    case 16:
        // XXX
        break;
    }
    helper_st_leswap(addr, val, size);
}

void helper_ldf_leswap(target_ulong addr , int size, int rd)
{
    target_ulong val = 0;

    helper_check_align(addr, 3);
    val = helper_ld_leswap(addr, size, 0);
    
    //printf("***sizeof(float) = %d--- %f\n",(int)sizeof(float),(float)val); 

    switch(size) {
    default:
    case 4:
        *((uint32_t *)&env->fpr[rd]) = val;
        break;
    case 8:
        *((int64_t *)&DT0) = val;
        break;
    case 16:
        // XXX
        break;
    }
}
#endif //#ifdef CONFIG_ESESC_CUDA
