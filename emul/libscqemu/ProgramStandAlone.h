#ifndef PROGRAMSTANDALONE_H
#define PROGRAMSTANDALONE_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// PACKAGE is needed by bfd
#define PACKAGE "sccore" 
#include "bfd/bfd.h"
#include <dis-asm.h>

#include "ProgramBase.h"
#define MAX_NTHREADS 128

/* rounding macros, assumes ALIGN is a power of two */
#define PAGE_SIZE 4096
#define PAGE_ALIGN_UP(N) (((N) + ((PAGE_SIZE)-1)) & ~((PAGE_SIZE)-1))
#define PAGE_ALIGN_DOWN(N) ((N) & ~((PAGE_SIZE)-1))

class ProgramStandAlone : public ProgramBase {
private:

  const char *filename;
  bfd        *bfdFile;
  asection   *SYSMEM_section; //**

  // native text, data segments
  uint32_t  text_start;
  uint32_t  text_end;
  uint32_t  text_size;

  uint32_t  data_start;
  uint32_t  data_end;
  uint32_t  data_size;

  uint32_t  data_break;

  uint32_t mem_mapped_file_start;
  uint32_t mem_mapped_file_end;

  // simulated text, data segments
  uint8_t *text_buffer;
  uint8_t *data_buffer;
 
  uint64_t     SYSMEM_section_start; //**
  uint64_t     SYSMEM_section_end; //**
  uint8_t     *SYSMEM_section_buffer; //**

  uint8_t     *stack_buffer;
  uint32_t     stack_start;
  uint32_t     stack_end;
  uint32_t     stack_pointer_start;
   
  struct disassemble_info info;
  disassembler_ftype disassemble;

  static void override_print_address(bfd_vma addr, struct disassemble_info *info);
 
protected:
public:
  uint32_t *g2h(uint32_t addr);
  uint8_t *load_stack(uint32_t stack_start_addr, uint32_t stack_buffer_start_addr, const char *stackdump);

  void set_SYSMEM_section(uint64_t *syscall_mem_loc); //**
  uint32_t *get_data_buffer(uint32_t *size);
  void set_data_buffer(uint32_t size, uint8_t *data_ptr);

public:
  ProgramStandAlone(const char *name, uint64_t  *syscall_mem_loc);
  virtual ~ProgramStandAlone() {
  };

  uint32_t get_start_address();

  uint32_t get_code32(uint32_t pc);

  void dump(uint32_t pc);

  uint64_t ldu64(uint32_t addr);
  uint32_t ldu32(uint32_t addr);
  uint16_t ldu16(uint32_t addr);
  uint8_t  ldu08(uint32_t addr);

  int64_t lds64(uint32_t addr);
  int32_t lds32(uint32_t addr);
  int16_t lds16(uint32_t addr);
  int8_t  lds08(uint32_t addr);

  void st64(uint32_t addr, uint64_t data);
  void st32(uint32_t addr, uint32_t data);
  void st16(uint32_t addr, uint16_t data);
  void st08(uint32_t addr, uint8_t  data);
  uint32_t get_reg(uint8_t reg, TranslationType ttype);
  void set_reg(uint8_t reg, uint32_t val);
  uint32_t do_syscall(ProgramBase *prog, int num, uint32_t arg1,
                            uint32_t arg2, uint32_t arg3, uint32_t arg4,
                            uint32_t arg5, uint32_t arg6, FILE *syscallTrace);
  void target_set_brk(uint32_t addr);
  uint32_t getValAtAddr(uint32_t addr);
  void set_mem_mapped_file_endpts(long mapped_addr, uint32_t size);
};

#endif
