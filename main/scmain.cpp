// LINK: pp.c /usr/lib/libbfd-*multiarch*.so /usr/lib/libopcodes-*multiarch*.so
//
//
// gdb ./loader
// >b _init
// >r mytest arg1 arg2
// >c
// >info registers
//

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ProgramStandAlone.h"
#include "scexec.h"

//#define MAX_NTHREADS 128 
uint64_t syscall_mem[20*MAX_NTHREADS];
bool thread_done[MAX_NTHREADS];

int main (int argc, char **argv)
{
  if (argc < 5) {
    fprintf(stderr,"Usage:\n\t%s stack_start stack_buffer_start stackdump.bin binary <arm_log  syscall_trace>\n",argv[0]);
#ifdef SPARC32
    //fprintf(stderr,"Usage:\n\t%s stack_start stack_buffer_start stackdump.bin binary <a/s> <arm log>\n",argv[0]);
#endif
    // to get a stack dump
    // info registers
    // dump binary memory foo.txt 0xbeffff30 0xbeffffff
    exit(-3);
  }

  FILE *fp = NULL;
  FILE *syscallTrace_fp = NULL;
  if (argc == 7) {
    // Open the optional <arm log> file and <syscall_trace_file> for validating ISA
    fp = fopen(argv[5], "r");
    syscallTrace_fp = fopen(argv[6], "r");
  } 
#ifdef SPARC32
  //if (argc == 7) {
  //  // Open the optional <arm log> file for validating ISA
  //  fp = fopen(argv[6], "r");
  //} 
#endif

  uint32_t stack_start = strtol(argv[1],0,0);
  uint32_t stack_buffer_start = strtol(argv[2],0,0);

  ProgramStandAlone *prog = new ProgramStandAlone(argv[4],syscall_mem);

  uint32_t pc = prog->get_start_address();
  uint8_t *sp = prog->load_stack(stack_start, stack_buffer_start, argv[3]);

  TranslationType ttype;
#ifdef SPARC32
  ttype = (argv[5][0] == 'a') ? ARM : SPARC32;
#endif

  printf("starting at 0x%x : 0x%x  with stack = 0x%x\n", pc, prog->get_code32(pc), sp);
  printf("\n\n");

  if (prog->get_start_address()&1)
    ttype = THUMB32;
  else
    ttype = ARM;

  Scexec cpu(0, (void *) (&syscall_mem[0]), ttype , prog, fp, syscallTrace_fp);

  cpu.exec_loop();
  return 0;
}
