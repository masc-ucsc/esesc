#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <elf.h>
#include <endian.h>
#include <errno.h>
#include <unistd.h> 
#include <fcntl.h>
#include <time.h>
#include <limits.h> 
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/swap.h>
#include <signal.h> 
#include <sched.h>
#include <iostream>
#include <regex.h>
#ifdef __ia64__
int __clone2(int (*fn)(void *), void *child_stack_base,
                 size_t stack_size, int flags, void *arg, ...);
#endif
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/times.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/statfs.h>
#include <utime.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/wireless.h>
#include "ProgramStandAlone.h"
#include "arm_syscall_nr.h"
#include "ProgramBase.h"
#include "sccore.h"
#include "InstOpcode.h"
#include "qemumin.h"
#include "ahash.h"
#include "libsoftfpu/littleendian/softfloat.h"
#include "complexinst.h"
#ifdef TARGET_GPROF
#include <sys/gmon.h>
#endif
#ifdef CONFIG_EVENTFD
#include <sys/eventfd.h>
#endif
#ifdef CONFIG_EPOLL
#include <sys/epoll.h>
#endif
#ifdef CONFIG_ATTR
#include "qemu-xattr.h"
#endif

#define termios host_termios
#define winsize host_winsize
#define termio host_termio
#define sgttyb host_sgttyb /* same as target */
#define tchars host_tchars /* same as target */
#define ltchars host_ltchars /* same as target */
#include <linux/termios.h>
#include <linux/unistd.h>
#include <linux/utsname.h>
#include <linux/cdrom.h>
#include <linux/hdreg.h>
#include <linux/soundcard.h>
#include <linux/kd.h>
#include <linux/mtio.h>
#include <linux/fs.h>
#if defined(CONFIG_FIEMAP)
#include <linux/fiemap.h>
#endif
#include <linux/fb.h>
#include <linux/vt.h>
#ifdef CONFIG_ESESC_user
#include "esesc_qemu.h"
static struct timespec start_time={0,0};
#endif
#define VERIFY_READ 0
#ifdef CONFIG_SCQEMU
extern uint64_t syscall_mem[];
extern bool thread_done[];
//typedef enum TranslationType { ARM = 0, THUMB, THUMB32, SPARC32};
extern void scfork(uint64_t, void *, uint8_t, void *);
#endif
#endif
static bitmask_transtbl fcntl_flags_tbl[] = {
  {TARGET_O_ACCMODE,   TARGET_O_WRONLY,    O_ACCMODE,   O_WRONLY,    },
  { TARGET_O_ACCMODE,   TARGET_O_RDWR,      O_ACCMODE,   O_RDWR,      },
  { TARGET_O_CREAT,     TARGET_O_CREAT,     O_CREAT,     O_CREAT,     },
  { TARGET_O_EXCL,      TARGET_O_EXCL,      O_EXCL,      O_EXCL,      },
  { TARGET_O_NOCTTY,    TARGET_O_NOCTTY,    O_NOCTTY,    O_NOCTTY,    },
  { TARGET_O_TRUNC,     TARGET_O_TRUNC,     O_TRUNC,     O_TRUNC,     },
  { TARGET_O_APPEND,    TARGET_O_APPEND,    O_APPEND,    O_APPEND,    },
  { TARGET_O_NONBLOCK,  TARGET_O_NONBLOCK,  O_NONBLOCK,  O_NONBLOCK,  },
  { TARGET_O_SYNC,      TARGET_O_SYNC,      O_SYNC,      O_SYNC,      },
  { TARGET_FASYNC,      TARGET_FASYNC,      FASYNC,      FASYNC,      },
  { TARGET_O_DIRECTORY, TARGET_O_DIRECTORY, O_DIRECTORY, O_DIRECTORY, },
  { TARGET_O_NOFOLLOW,  TARGET_O_NOFOLLOW,  O_NOFOLLOW,  O_NOFOLLOW,  },
  { TARGET_O_LARGEFILE, TARGET_O_LARGEFILE, O_LARGEFILE, O_LARGEFILE, },
#if defined(O_DIRECT)
  { TARGET_O_DIRECT,    TARGET_O_DIRECT,    O_DIRECT,    O_DIRECT,    },
#endif
  { 0, 0, 0, 0 } 
};
static bitmask_transtbl mmap_flags_tbl[] = {
  { TARGET_MAP_SHARED, TARGET_MAP_SHARED, MAP_SHARED, MAP_SHARED },
  { TARGET_MAP_PRIVATE, TARGET_MAP_PRIVATE, MAP_PRIVATE, MAP_PRIVATE },
  { TARGET_MAP_FIXED, TARGET_MAP_FIXED, MAP_FIXED, MAP_FIXED },
  { TARGET_MAP_ANONYMOUS, TARGET_MAP_ANONYMOUS, MAP_ANONYMOUS, MAP_ANONYMOUS },
  { TARGET_MAP_GROWSDOWN, TARGET_MAP_GROWSDOWN, MAP_GROWSDOWN, MAP_GROWSDOWN },
  { TARGET_MAP_DENYWRITE, TARGET_MAP_DENYWRITE, MAP_DENYWRITE, MAP_DENYWRITE },
  { TARGET_MAP_EXECUTABLE, TARGET_MAP_EXECUTABLE, MAP_EXECUTABLE, MAP_EXECUTABLE },
  { TARGET_MAP_LOCKED, TARGET_MAP_LOCKED, MAP_LOCKED, MAP_LOCKED },
  { 0, 0, 0, 0 }
};
#define ERRNO_TABLE_SIZE 1200
#define TARGET_EFAULT          14
#define PAGE_READ      0x0001
#define PAGE_WRITE     0x0002

static uint32_t target_brk;
static uint32_t target_original_brk;
static uint32_t brk_page;

uint32_t *lock_user(ProgramBase *prog, uint32_t guest_addr, uint32_t length)
{

  return (prog->g2h(guest_addr));       

}

uint32_t target_strlen(ProgramBase *prog, uint32_t guest_addr)
{
  void * p = lock_user(prog, guest_addr, 0/* Length!!! */); 
  if (p)  // FIXME: assuming 0 or NULL is error. update based on the error handling in g2h 
    return (strlen((const char *) p));
  else
    return 0; // FIXME: define ERROR macros (e.g. #define TARGET_ERROR 0)

}

/* Like lock_user but for null terminated strings.  */
uint32_t *lock_user_string(ProgramBase *prog, uint32_t guest_addr)
{
  uint32_t len;

  len = target_strlen(prog, guest_addr);
  if (len < 0)
    return NULL;

  return lock_user(prog, guest_addr,(len + 1));
}


uint32_t target_to_host_bitmask(uint32_t x86_mask, bitmask_transtbl *trans_tbl)
{
  bitmask_transtbl *btp;
  uint32_t  alpha_mask = 0;

  for(btp = trans_tbl; btp->x86_mask && btp->alpha_mask; btp++) {
    if((x86_mask & btp->x86_mask) == btp->x86_bits) {
      alpha_mask |= btp->alpha_bits;
    }
  }
  return(alpha_mask);
}

static uint8_t target_to_host_signal_table[_NSIG];
uint32_t target_to_host_signal(uint32_t sig)
{
  if (sig >= _NSIG)
    return sig;
  return target_to_host_signal_table[sig];
}
//*****************************************************************************************************************
// This part is related to IOCTL syscall and is still incomplete.
/*#define TARGET_ENOSYS          38      
#define THUNK_TARGET 0
#define THUNK_HOST   1
#define MAX_STRUCT_SIZE 4096
#define HOST_LONG_SIZE (HOST_LONG_BITS / 8)
#define MAX_STRUCTS 128
typedef struct {
  // standard struct handling 
  const argtype *field_types;
  int nb_fields;
  int *field_offsets[2];
  // special handling 
  void (*convert[2])(void *dst, const void *src);
  int size[2];
  int align[2];
  const char *name;
} StructEntry;
extern StructEntry struct_entries[];
StructEntry struct_entries[MAX_STRUCTS];
typedef enum argtype {
  TYPE_NULL,
  TYPE_CHAR,
  TYPE_SHORT,
  TYPE_INT,
  TYPE_LONG,
  TYPE_ULONG,
  TYPE_PTRVOID, //
  TYPE_LONGLONG,
  TYPE_ULONGLONG,
  TYPE_PTR,
  TYPE_ARRAY,
  TYPE_STRUCT,
} argtype;
static inline uint32_t is_error(uint32_t ret)
{
        return ret = (-4096);
}
static inline const argtype *thunk_type_next(const argtype *type_ptr);
static inline int thunk_type_size(const argtype *type_ptr, int is_host);
static const argtype *thunk_type_next_ptr(const argtype *type_ptr)
{
    return thunk_type_next(type_ptr);
}
static inline const argtype *thunk_type_next(const argtype *type_ptr)
{
  int type;

  type = *type_ptr++;
  switch(type) {
    case TYPE_CHAR:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_LONGLONG:
    case TYPE_ULONGLONG:
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
      return type_ptr;
    case TYPE_PTR:
      return thunk_type_next_ptr(type_ptr);
    case TYPE_ARRAY:
      return thunk_type_next_ptr(type_ptr + 1);
    case TYPE_STRUCT:
      return type_ptr + 1;
    default:
      return NULL;
  }
}


// now we can define the main conversion functions 
const argtype *thunk_convert(void *dst, const void *src,
    const argtype *type_ptr, int to_host)
{
  int type;

  type = *type_ptr++;
  switch(type) {
    case TYPE_CHAR:
      *(uint8_t *)dst = *(uint8_t *)src;
      break;
    case TYPE_SHORT:
      *(uint16_t *)dst = tswap16(*(uint16_t *)src);
      break;
    case TYPE_INT:
      *(uint32_t *)dst = tswap32(*(uint32_t *)src);
      break;
    case TYPE_LONGLONG:
    case TYPE_ULONGLONG:
      *(uint64_t *)dst = tswap64(*(uint64_t *)src);
      break;
#if HOST_LONG_BITS == 32 && TARGET_ABI_BITS == 32
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
      *(uint32_t *)dst = tswap32(*(uint32_t *)src);
      break;
#elif HOST_LONG_BITS == 64 && TARGET_ABI_BITS == 32
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
      if (to_host) {
        if (type == TYPE_LONG) {
          // sign extension 
          *(uint64_t *)dst = (int32_t)tswap32(*(uint32_t *)src);
        } else {
          *(uint64_t *)dst = tswap32(*(uint32_t *)src);
        }
      } else {
        *(uint32_t *)dst = tswap32(*(uint64_t *)src & 0xffffffff);
      }
      break;
#elif HOST_LONG_BITS == 64 && TARGET_ABI_BITS == 64
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
      *(uint64_t *)dst = tswap64(*(uint64_t *)src);
      break;
#elif HOST_LONG_BITS == 32 && TARGET_ABI_BITS == 64
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
      if (to_host) {
        *(uint32_t *)dst = tswap64(*(uint64_t *)src);
      } else {
        if (type == TYPE_LONG) {
          // sign extension 
          *(uint64_t *)dst = tswap64(*(int32_t *)src);
        } else {
          *(uint64_t *)dst = tswap64(*(uint32_t *)src);
        }
      }
      break;

#endif
    case TYPE_ARRAY:
      {
        int array_length, i, dst_size, src_size;
        const uint8_t *s;
        uint8_t  *d;

        array_length = *type_ptr++;
        dst_size     = thunk_type_size(type_ptr, to_host);
        src_size     = thunk_type_size(type_ptr, 1 - to_host);
        d            = dst;
        s            = src;
        for(i = 0;i < array_length; i++) {
          thunk_convert(d, s, type_ptr, to_host);
          d += dst_size;
          s += src_size;
        }
        type_ptr = thunk_type_next(type_ptr);
      }
      break;
    case TYPE_STRUCT:
      {
        {
          int i;
          const StructEntry *se;
          const uint8_t *s;
          uint8_t  *d;
          const argtype *field_types;
          const int *dst_offsets, *src_offsets;

          se = struct_entries + *type_ptr++;
          if (se->convert[0] != NULL) {
            // specific conversion is needed 
            (*se->convert[to_host])(dst, src);
          } else {
            // standard struct conversion 
            field_types = se->field_types;
            dst_offsets = se->field_offsets[to_host];
            src_offsets = se->field_offsets[1 - to_host];
            d           = dst;
            s           = src;
            for(i = 0;i < se->nb_fields; i++) {
              field_types = thunk_convert(d + dst_offsets[i],
                  s + src_offsets[i],
                  field_types, to_host);
            }
          }
        }
        break;
        default:
        fprintf(stderr, "Invalid type 0x%x\n", type);
        break;
      }
      return type_ptr;
  }
  typedef struct IOCTLEntry IOCTLEntry;
#define STRUCT(name, ...) static const argtype struct_ ## name ## _def[] = {  __VA_ARGS__, TYPE_NULL };
  typedef uint32_t do_ioctl_fn(const IOCTLEntry *ie, uint8_t *buf_temp,
      int fd, uint32_t cmd, uint32_t arg);

  struct IOCTLEntry {
    unsigned int target_cmd;
    unsigned int host_cmd;
    const char *name;
    int access;
    do_ioctl_fn *do_ioctl;
    const argtype arg_type[5];
};
#define IOC_R 0x0001
#define IOC_W 0x0002
#define IOC_RW (IOC_R | IOC_W)
static IOCTLEntry ioctl_entries[] = {
#define IOCTL(cmd, access, ...) \
      { TARGET_ ## cmd, cmd, #cmd, access, 0, {  __VA_ARGS__ } },
#define IOCTL_SPECIAL(cmd, access, dofn, ...)                      \
      { TARGET_ ## cmd, cmd, #cmd, access, dofn, {  __VA_ARGS__ } },
//#include "ioctls.h"
    { 0, 0, },
    };
static inline int thunk_type_size(const argtype *type_ptr, int is_host)
{
  int type, size;
  const StructEntry *se;

  type = *type_ptr;
  switch(type) {
    case TYPE_CHAR:
      return 1;
    case TYPE_SHORT:
      return 2;
    case TYPE_INT:
      return 4;
    case TYPE_LONGLONG:
    case TYPE_ULONGLONG:
      return 8;
    case TYPE_LONG:
    case TYPE_ULONG:
    case TYPE_PTRVOID:
    case TYPE_PTR:
      if (is_host) {
        return HOST_LONG_SIZE;
      } else {
        return TARGET_ABI_BITS / 8;
      }
      break;
    case TYPE_ARRAY:
      size = type_ptr[1];
      return size * thunk_type_size_array(type_ptr + 2, is_host);
    case TYPE_STRUCT:
      se = struct_entries + type_ptr[1];
      return se->size[is_host];
    default:
      return -1;
  }
}
// ??? Implement proper locking for ioctls.  
// do_ioctl() Must return target values and target errnos. 
static uint32_t do_ioctl(ProgramBase *prog, uint32_t fd, uint32_t cmd, uint32_t arg)
{
    const IOCTLEntry *ie;
    const argtype *arg_type;
    uint32_t ret;
  uint8_t buf_temp[MAX_STRUCT_SIZE];
  uint32_t target_size;
  void *argptr;

  ie = ioctl_entries;
  for(;;) {
    if (ie->target_cmd == 0) {
//      gemu_log("Unsupported ioctl: cmd=0x%04lx\n", (long)cmd);
      return -TARGET_ENOSYS;
    }
    if (ie->target_cmd == cmd)
      break;
    ie++;
  }
  arg_type = ie->arg_type;
#if defined(DEBUG)
 // gemu_log("ioctl: cmd=0x%04lx (%s)\n", (long)cmd, ie->name);
#endif
  if (ie->do_ioctl) {
    return ie->do_ioctl(ie, buf_temp, fd, cmd, arg);
  }

  switch(arg_type[0]) {
    case TYPE_NULL:
      // no argument 
      ioctl(fd, ie->host_cmd);
      break;
    case TYPE_PTRVOID:
    case TYPE_INT:
      // int argment 
      ioctl(fd, ie->host_cmd, arg);
      break;
    case TYPE_PTR:
      arg_type++;
      target_size = thunk_type_size(arg_type, 0);
      switch(ie->access) {
       case IOC_R:
          ioctl(fd, ie->host_cmd, buf_temp);
        if (!is_error(ret)) {
          argptr = lock_user(prog, arg, target_size);
          if (!argptr)
            return -TARGET_EFAULT;
          thunk_convert(argptr, buf_temp, arg_type, THUNK_TARGET);

        }
        break;
        case IOC_W:
        argptr = lock_user(prog, arg, target_size);
        if (!argptr)
          return -TARGET_EFAULT;
        thunk_convert(buf_temp, argptr, arg_type, THUNK_HOST);

        ioctl(fd, ie->host_cmd, buf_temp);
        break;
        default:
        case IOC_RW:
        argptr = lock_user(prog, arg, target_size);
        if (!argptr)
          return -TARGET_EFAULT;
        thunk_convert(buf_temp, argptr, arg_type, THUNK_HOST);
        
        ioctl(fd, ie->host_cmd, buf_temp);
        if (!is_error(ret)) {
          argptr = lock_user(prog, arg, target_size);
          if (!argptr)
            return -TARGET_EFAULT;
          thunk_convert(argptr, buf_temp, arg_type, THUNK_TARGET);
        }
        break;
      }
      break;
    default:
     // gemu_log("Unsupported ioctl type: cmd=0x%04lx type=%d\n",
       //   (long)cmd, arg_type[0]);
      ret = -TARGET_ENOSYS;
      break;
  }
  return ret;
}*/

//*****************************************************************************************************

void target_set_brk_arm(uint32_t addr) {
  target_original_brk = target_brk = addr;
  brk_page = PAGE_ALIGN_UP(target_brk);
  printf("original brk 0x%08x brk_page 0x%08x \n", addr, brk_page);
}

uint32_t do_syscall_arm(ProgramBase *prog, uint32_t num, uint32_t arg1,uint32_t
    arg2, uint32_t arg3,uint32_t arg4,uint32_t arg5, uint32_t arg6, FILE *syscallTrace)

{ 

#ifdef DEBUG2
  printf ("syscall %d \n", num);
#endif

  uint32_t ret;  
  void  *p;
#if defined (CONFIG_ESESC_system) || defined (CONFIG_ESESC_user)
  struct timespec startTime;
  clock_gettime(CLOCK_REALTIME,&startTime);
#endif

  char tmp_str[128];
  char pc[32];
  char systrace_syscall_num[32];
  char emul_syscall_num[32];
  int i, j, ret1; 
  regex_t re1;
  regmatch_t match[3];

  if (syscallTrace != NULL) {

    // Example trace file section
    //
    //----------
    //pc: 0x8174
    //syscall: 122
    //Linux
    //masca1
    //3.0.0-1205-omap4
    //#10-Ubuntu SMP PREEMPT Thu Sep 29 03:57:24 UTC 2011
    //armv7l
    //----------
    //pc: 0x8230
    //syscall: 4
    //----------

    //printf("Advance to the section \n");
    fgets(tmp_str, sizeof tmp_str, syscallTrace);
    tmp_str[strlen (tmp_str) - 1] = '\0';
    memset(tmp_str, '\0', 128);

    // read pc
    fgets(pc, sizeof pc, syscallTrace);
    pc[strlen (pc) - 1] = '\0';

    // read syscall number
    fgets(tmp_str, sizeof tmp_str, syscallTrace);
    tmp_str[strlen (tmp_str) - 1] = '\0';
    //printf("pc %s, syscall %s \n", pc, tmp_str);

    ret1 = regcomp(&re1, "syscall: ([0-9]+)", REG_EXTENDED);
    ret1 = regexec(&re1, tmp_str, 2, match, 0);
    if(!ret1) {
      j = 0;
      for(i = match[1].rm_so; i < match[1].rm_eo; i++) {
        systrace_syscall_num[j] = tmp_str[i];
        j++;
      }
      systrace_syscall_num[j] = '\0';
    }else{
      I(0);
    }

    sprintf(emul_syscall_num, "%d", num);

    // Sanity check for syscall number
    if (strcmp(systrace_syscall_num, emul_syscall_num) != 0) {
      printf("Emulator syscall trace and expected syscall trace are out of sync \n");
      printf("  syscall num - Expected: %s, Emulator: %s\n", systrace_syscall_num, emul_syscall_num);
      I(0);
      // exit(0);
    }
  }

  switch(num) {
    case TARGET_NR_exit:
      exit(arg1);
      return 0;
      break;
    case TARGET_NR_exit_group:
      exit(arg1);
      return 0;
      break;
    case TARGET_NR_write: 
      ret = write(arg1, prog->g2h(arg2), arg3);
      return ret;
      break;
    case TARGET_NR_read: 
      if (arg3 == 0) {
        ret = 0;
      }else{
        ret = read(arg1, prog->g2h(arg2), arg3);
      }
      return ret;
      break;
      /* case TARGET_NR_ioctl:
         do_ioctl(prog, arg1, arg2, arg3);
         return 0;
         break;*/
    case TARGET_NR_lseek:
      ret = lseek(arg1, arg2, arg3);
      return ret;
      break;
    case TARGET_NR_gettimeofday:
      {          
        // FIXME: Not correct
        struct timeval tv; 
        gettimeofday(&tv, NULL);
      }  
      return 0;
      break;
    case TARGET_NR_open:
      // FIXME: how to check if this is a open(a,b,c) or open(a,b)
      ret = open((const char *)prog->g2h(arg1), arg2);
      return ret;
      break;        
    case TARGET_NR_close:
      ret = close(arg1);
      return ret;
      break;
    case TARGET_NR_mkdir:
      p = lock_user_string(prog, arg1);
      ret = mkdir((const char *)p, arg2);
      return ret; 
      break;
    case TARGET_NR_rmdir:
      p = lock_user_string(prog, arg1);
      ret = rmdir((const char *)p);
      return ret;                
      break;
    case TARGET_NR_rename:
      {
        void *p2;
        p  = lock_user_string(prog, arg1);
        p2 = lock_user_string(prog, arg2);
        if (!p || !p2)
          ret = -TARGET_EFAULT;
        else
          rename((const char *)p, (const char *)p2);
      }
      return 0;
      break;

    case TARGET_NR_uname:
      // copy 390 bytes from syscalltrace to buf
      //Linux
      //masca1
      //3.0.0-1205-omap4
      //#10-Ubuntu SMP PREEMPT Thu Sep 29 03:57:24 UTC 2011
      {
        struct utsname *uts_buf = (struct utsname *)prog->g2h(arg1);

        if (syscallTrace != NULL) {

          for (i = 0; i < 5; i++) {
            memset(tmp_str, '\0', 65);
            fgets(tmp_str, sizeof tmp_str, syscallTrace);
            tmp_str[strlen (tmp_str) - 1] = '\0';
            if(i == 0)
              strcpy(uts_buf->sysname, tmp_str);
            else if(i == 1)
              strcpy(uts_buf->nodename, tmp_str);
            else if(i == 2)
              strcpy(uts_buf->release, tmp_str);
            else if(i == 3)
              strcpy(uts_buf->version, tmp_str);
            else if(i == 4)
              strcpy(uts_buf->machine, tmp_str);
          }
        } else {
          if (uname(uts_buf) < 0)
            return (-1);
        }
      }
      return 0;
      break;

    case TARGET_NR_brk:
      {

        if (!arg1) {
          return target_brk;
        } else if(arg1 < target_original_brk) { 
          return target_brk; 
        } else {

          if(arg1 < brk_page) {
            if(arg1 > target_brk) {
              // initialize as it may contain garbage due to a previous heap usage
              // (grown then shrunken)
              memset(prog->g2h(target_brk), 0, arg1 - target_brk);
            }
            // deallocate is handled automatically here.
            target_brk = arg1;
            return target_brk;
          } 

          // Need to allocate memory.
          // We can't call local host brk() system call which tends to change the 
          // simulator program's brk_addr.
          // What we need to adjust here is the simulated virtual memory data segment.
          // so simulate brk here by calling realloc to reallocate simulator data segment.

          uint32_t size_incr = PAGE_ALIGN_UP(arg1 - brk_page); 

          uint32_t old_size;
          uint32_t *data_ptr = NULL;
          data_ptr = prog->get_data_buffer(&old_size);

          uint8_t *new_data_ptr = (uint8_t *)realloc(data_ptr, (old_size + size_incr));

          if (new_data_ptr != NULL) {

            prog->set_data_buffer((old_size + size_incr), new_data_ptr);
            target_brk = arg1;
            brk_page = PAGE_ALIGN_UP(target_brk);

            return target_brk;
          } else {
            return (-1);
          }
        }
      }
      return target_brk;;
      break;

    case TARGET_NR_fstat64:
      {
        if (syscallTrace == NULL) {
          ret = fstat(arg1, (struct stat *)prog->g2h(arg2));
          return ret;
        } else {
          // copy 144 bytes (size of stat structure) from systrace file
          // number of lines in the file: total 144 bytes/8 bytes per line = 18 lines
          for (i=0; i < 18; i++) {
            memset(tmp_str, '\0', 128);
            fgets(tmp_str, 128, syscallTrace);
            long long int val = strtoull(tmp_str, NULL, 16);
            memcpy( (prog->g2h(arg2 + (i * 8) )), &val, 8);
          }
        }
      }
      return 0;
      break;

    case TARGET_NR_mmap2:
      {
        if (arg1 != 0x00000000) {
          // FIXME: This is the non-zero address case.
          // The address gives a hint to the kernel about where to map the file.
          // We might still do a malloc but need to remember a mapping of the native
          // address (i.e arg1) to pointer returned by malloc.
          printf("TARGET_NR_mmap2 map address not zero!!! This case is not handled \n");
          exit(-1);
        }
        uint8_t *p = (uint8_t *)malloc(arg2);
        if (p == MAP_FAILED)
          return -1;
        else {
          prog->set_mem_mapped_file_endpts((long)p, arg2);
          return (long)p;
        }
      }
      return 0;
      printf("return address 0x%08x \n", p);
      break;

    case TARGET_NR_getpid:
      getpid();
      return 0;
      break;
    case TARGET_NR_kill:
      kill(arg1, target_to_host_signal(arg2));
      return 0;                 
      break;
    case TARGET_NR_pause:
      pause();
      return 0;
      break;
#ifdef TARGET_NR_shutdown
    case TARGET_NR_shutdown:
      shutdown(arg1, arg2);
      return 0;
      break;
#endif
#ifdef TARGET_NR_semget
    case TARGET_NR_semget:
      semget(arg1, arg2, arg3);
      return 0;       
      break; 
#endif
    case TARGET_NR_setdomainname:
      p = lock_user_string(prog, arg1);
      setdomainname((const char *)p, arg2);
      return 0;      
      break;
    case TARGET_NR_getsid:
      getsid(arg1);
      return 0;
      break;
#if defined(TARGET_NR_fdatasync)
      /* Not on alpha (osf_datasync ?) */
    case TARGET_NR_fdatasync:
      fdatasync(arg1);
      return 0;
      break;
#endif
#ifdef TARGET_NR_getuid32
    case TARGET_NR_getuid32:
      getuid();
      return 0;
      break;
#endif 
    case TARGET_NR_ioctl:
      printf("ioctl syscall not implemented\n");
      return 0;
      break;
    default:
      printf("syscall %d is not implemented\n", num);
      exit(-1);
  }    

}
