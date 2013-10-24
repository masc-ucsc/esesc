
// LINK: pp.c /usr/lib/libbfd-*multiarch*.so /usr/lib/libopcodes-*multiarch*.so

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ProgramStandAlone.h"
#include "InstOpcode.h"
#include "arm_syscall.h"

#if 0
#define start_thread(regs,pc,sp)                                \
 ({                                                             \
  unsigned long *stack = (unsigned long *)sp;                   \
  set_fs(USER_DS);                                              \
  memset(regs->uregs, 0, sizeof(regs->uregs));                  \
  if (current->personality & ADDR_LIMIT_32BIT)                  \
   regs->ARM_cpsr = USR_MODE;                                   \
   regs->ARM_cpsr = USR26_MODE;                                 \
  if (elf_hwcap & HWCAP_THUMB && pc & 1)                        \
   regs->ARM_cpsr |= PSR_T_BIT;                                 \
  regs->ARM_cpsr |= PSR_ENDSTATE;                               \
  regs->ARM_pc = pc & ~1;  /* pc */                             \
  regs->ARM_sp = sp;       /* sp */                             \
  regs->ARM_r2 = stack[2]; /* r2 (envp) */                      \
  regs->ARM_r1 = stack[1]; /* r1 (argv) */                      \
  regs->ARM_r0 = stack[0]; /* r0 (argc) */                      \
  nommu_start_thread(regs);                                     \
 })

#endif

void ProgramStandAlone::override_print_address(bfd_vma addr, struct disassemble_info *info) 
  // print address in dump dissasembled code {{{1
{
  printf("0x%x", (uint32_t)addr );
}
// 1}}}

uint8_t *ProgramStandAlone::load_stack(uint32_t stack_start_addr, uint32_t stack_buffer_start_addr, const char *stackdump) 
// load stack from a binary dump and set same addresses {{{1
{
  stack_buffer = (uint8_t *)malloc(8192*1024); // 8M
  if (stack_buffer == 0) {
    fprintf(stderr,"ERROR: could not allocate stack\n");
    exit(-3);
  }
  stack_pointer_start = stack_start_addr;
  stack_start = stack_buffer_start_addr;
  stack_end   = stack_start + 8192*1024;
  printf("Stack 0X%x:0X%x\n", stack_start, stack_end);

  int fd = open(stackdump, O_RDONLY);
  if (fd<0) {
    fprintf(stderr,"ERROR: could not open stack dump [%s]\n",stackdump);
    exit(-4);
  }

  int pos = 0;
  while(read(fd, &stack_buffer[pos],1) == 1){
    //printf("stack(0X%x]=0X%x\n", stack_start+pos, stack_buffer[pos]);
    pos++;
  } 

  printf("allocated stack %s from 0x%x with %d bytes\n",stackdump, stack_start, pos);
  close(fd);

  return stack_buffer;
}
// 1}}}

//***************
uint32_t *ProgramStandAlone::g2h(uint32_t addr)
{
  if (addr >= stack_start && addr < stack_end)
    return (uint32_t *)(stack_buffer + (addr - stack_start));
  if (addr >= text_start && addr < text_end)
    return (uint32_t *)(text_buffer + (addr - text_start));
  if (addr >= data_start && addr < data_end)
    return (uint32_t *)(data_buffer + (addr - data_start));
  if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) {
    return (uint32_t *)addr;
  }
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
    return (uint32_t *)(SYSMEM_section_buffer + (addr - SYSMEM_section_start));
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (g2h).\n", addr);
    return 0;

  }
}
//***************

//**
void ProgramStandAlone::set_SYSMEM_section(uint64_t *syscall_mem_loc)
{
  if (!syscall_mem_loc)
  {
    printf("SYSMEM buffer is not allocated correctly!\n");
    exit(-1);
  }
  SYSMEM_section_buffer = (uint8_t *) syscall_mem_loc;
  SYSMEM_section_start  = (uint64_t) syscall_mem_loc;
 // printf("addr:%lu start:%lu\n", syscall_mem_loc, SYSMEM_section_start);
  SYSMEM_section_end    = SYSMEM_section_start + 20*MAX_NTHREADS * sizeof(uint64_t); 
} 
//**

//  1}}}
ProgramStandAlone::ProgramStandAlone(const char *name, uint64_t *syscall_mem_loc) 
  // Constructor {{{1
{

  asection *sect;

  // initialize to maximum
  uint32_t first_text_section_start = 0xFFFFFFFF;
  uint32_t first_data_section_start = 0xFFFFFFFF; 
  // initialize to minimum
  uint32_t last_text_section_end = 0x00000000;
  uint32_t last_data_section_end = 0x00000000;

  uint32_t offset;

  filename = strdup(name);

  bfd_init();

  bfdFile = bfd_openr(filename, 0);

  if (bfdFile == NULL) {
    printf ("Error [%x]: could not open %s binary: %s\n", bfd_get_error(), filename, bfd_errmsg (bfd_get_error ()));
    exit(-1);
  }
  if (!bfd_check_format(bfdFile, bfd_object)) {
    printf ("Error [%x]: could not process %s binary: %s\n", bfd_get_error(), filename, bfd_errmsg (bfd_get_error ()));
    exit(-1);
  }

  // Assumptions (FIXME: Improve assumptions):
  // 1. First page of the application binary ELF format contains text section
  // 2. First data section is ".tdata" or ".bss"
  // 3. Last data section (which is also part of heap) is ".bss" or "--libc_freeres_ptrs"
  // Note: These assumptions work well for binaries compiled with static libraries and may need 
  // a few changes to work for binaries compiled with dynamic libraries.

  text_start               = PAGE_ALIGN_DOWN((bfd_get_section_by_name(bfdFile, ".text"))->vma);

  for (sect=bfdFile->sections; sect; sect=sect->next) {
    printf ("Section name %s \n", bfd_get_section_name(bfdFile, sect));
    if (!strcmp(bfd_get_section_name(bfdFile, sect),".tdata") || !strcmp(bfd_get_section_name(bfdFile, sect),".bss")) {
      if (first_data_section_start > sect->vma) {
        first_data_section_start = sect->vma;
      }
    }
    if (!strcmp(bfd_get_section_name(bfdFile, sect),".bss") || !strcmp(bfd_get_section_name(bfdFile, sect),"__libc_freeres_ptrs")) {
      if (last_data_section_end < (sect->vma + sect->size)) {
        last_data_section_end = (sect->vma + sect->size);
      }
    }
  }
  data_start               = PAGE_ALIGN_DOWN(first_data_section_start);
  data_end                 = PAGE_ALIGN_UP(last_data_section_end);

  // 2. Section to segment mapping.
  for (sect=bfdFile->sections; sect; sect=sect->next) {
    if ((bfd_get_section_flags(bfdFile, sect) & SEC_ALLOC)
        && (bfd_get_section_flags(bfdFile, sect) & SEC_LOAD)
        && bfd_section_vma(bfdFile, sect)
        && bfd_section_size(bfdFile, sect)) {
      
      if (sect->vma > text_start && (sect->vma + sect->size) < data_start) {
        // Belongs to text section
        if (last_text_section_end < (sect->vma + sect->size)) {
          last_text_section_end = sect->vma + sect->size;
        }   
        if (first_text_section_start > sect->vma) {
          first_text_section_start = sect->vma;
        }
      } else if (sect->vma > data_start && (sect->vma + sect->size) < data_end) {
        // Belongs to data section
      } else {
        printf ("section %s does not map to any segment\n", bfd_get_section_name(bfdFile, sect));
      }
    }
  }

  text_end = PAGE_ALIGN_UP(last_text_section_end);

  text_size = text_end - text_start;
  data_size = data_end - data_start;

#ifdef DEBUG2
  printf("first_text_section_start 0x%08x, last_text_section_end 0x%08x \n",
          first_text_section_start, last_text_section_end);
  printf("first_data_section_start 0x%08x, last_data_section_end 0x%08x \n",
          first_data_section_start, last_data_section_end);
  printf ("text segment: text_start 0x%08x, text_end 0x%08x \n", text_start, text_end);
  printf ("data segment: data_start 0x%08x, data_end 0x%08x \n", data_start, data_end);
#endif

  // Allocate buffer for text, data segments in simulated virtual memory
  text_buffer = (uint8_t *)malloc(text_size*sizeof(uint8_t));
  memset(text_buffer, '\0', text_size);
  data_buffer = (uint8_t *)malloc(data_size*sizeof(uint8_t));
  memset(data_buffer, '\0', data_size);

  for (sect=bfdFile->sections; sect; sect=sect->next) {
    if ((bfd_get_section_flags(bfdFile, sect) & SEC_ALLOC)
        && (bfd_get_section_flags(bfdFile, sect) & SEC_LOAD)
        && bfd_section_vma(bfdFile, sect)
        && bfd_section_size(bfdFile, sect)) {

      if (sect->vma > text_start && (sect->vma + sect->size) < data_start) {
        // Belongs to text segment
        offset = sect->vma - text_start;
        printf("offset 0x%08x \n", offset);
        if (!bfd_get_section_contents(bfdFile, sect, (text_buffer + offset), 0, sect->size)) {
          printf("Unable to read section %s contents \n",
                  bfd_section_name(bfdFile, sect));
        }
      } else if (sect->vma > data_start && (sect->vma + sect->size) < data_end) {
        // Belongs to data segment
        offset = sect->vma - data_start;
        printf("offset 0x%08x \n", offset);
        if (!bfd_get_section_contents(bfdFile, sect, (data_buffer + offset), 0, sect->size)) {
          printf("Unable to read section %s contents \n",
                  bfd_section_name(bfdFile, sect));
        }
      } else {
        printf ("ERROR!!! section %s does not belong to any segment \n", bfd_section_name(bfdFile, sect) );
      }
    }
  }

  // Now, pad the segments..

  // padding 1:
  // The first text page contains ELF header, the program header table, and other information.
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    printf ("Unable to open ELF file %s \n", filename);
    exit(-5);
  }
  uint8_t *hdr_buffer = (uint8_t *)malloc((first_text_section_start - text_start)* sizeof(uint8_t));
  fread(hdr_buffer, 1, (first_text_section_start - text_start), f);
  memcpy(text_buffer, hdr_buffer, (first_text_section_start - text_start));
  free(hdr_buffer);
  fclose(f);

  // padding 2:
  // The last text page holds a copy of the beginning of data.
  uint32_t dst_offset = last_text_section_end - text_start;
  uint32_t src_offset = first_data_section_start - data_start;
  memcpy((text_buffer + dst_offset), (data_buffer + src_offset), 
                              (text_end - last_text_section_end)); 
  // padding 3:
  // The first data page has a copy of the end of text.
  uint32_t data_padding_size = first_data_section_start - data_start;
  src_offset                 = last_text_section_end - data_padding_size;
  memcpy(data_buffer, (text_buffer + src_offset), data_padding_size);

  //**

  SYSMEM_section = bfd_get_section_by_name(bfdFile, ".SYSMEM");
  if (SYSMEM_section == NULL) {
    printf ("Error accessing .SYSMEM section (binaries should start with the .SYSMEM section)\n");
    printf ("Moving on as .SYSMEM is not mandatory.\n");
  }
  set_SYSMEM_section(syscall_mem_loc);
  
  //**

  // Set the brk_addr
  target_set_brk(data_end);
}

//

uint32_t ProgramStandAlone::get_start_address()
// get address were the binary should start execution {{{1
{
  return bfd_get_start_address(bfdFile);
}
// 1}}}
  
uint32_t ProgramStandAlone::get_code32(uint32_t pc) 
  // get 32bit code for a give pc address {{{1
{
  uint32_t insn = 0;
#ifdef DEBUG2
  printf("text_start 0x%08x text_end 0x%08x pc 0x%08x \n", text_start, text_end, pc);
#endif
  if (pc >= text_start && pc < text_end) {
     insn |=  *(text_buffer + (pc - text_start));
     insn |=  (* (text_buffer + (pc - text_start) + 1) << 8);
     insn |=  (* (text_buffer + (pc - text_start) + 2) << 16);
     insn |=  (* (text_buffer + (pc - text_start) + 3) << 24);
    return insn;
  }
  printf("must load new section \n");
  I(0); // must load new section
  exit (-1);

  return 0;
}
// 1}}}

void ProgramStandAlone::dump(uint32_t pc) 
  // provide details (dissasemble) from a given address {{{1
{
  disassemble(pc, &info);
}
// 1}}}


uint64_t ProgramStandAlone::ldu64(uint32_t addr)
{
#ifdef DEBUG2
  printf("  addr(ldu64) 0x%08x \n", addr);
#endif
  uint64_t val = 0;
  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<8;i++)
      val |= (stack_buffer[addr - stack_start + i] << i*8);
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<8;i++)
      val |= (text_buffer[addr - text_start + i] << i*8);
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<8;i++)
      val |= (data_buffer[addr - data_start + i] << i*8);
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) {
    for(int i = 0; i < 8; i++)
      val |= *(uint8_t *)(addr + i) << i*8;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<8;i++)
      val |= (SYSMEM_section_buffer[addr - SYSMEM_section_start + i] << i*8);
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (ldu64).\n", addr);
    exit(-5);
  }
  return val;
}
uint32_t ProgramStandAlone::ldu32(uint32_t addr)
{
#ifdef DEBUG2
  printf("  addr(ldu32) 0x%08x \n", addr);
#endif
  uint32_t val = 0;
  if (addr >= stack_start && addr < stack_end) {
#ifdef DEBUG2
    printf("falls in stack section \n");
#endif
    for(int i=0;i<4;i++) {
#ifdef DEBUG2
      printf("val 0x%08x \n stack_start 0x%08x val 0x%x \n", val, stack_start, stack_buffer[addr - stack_start + i]);
#endif
      val |= (stack_buffer[addr - stack_start + i] << i*8);
    }
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<4;i++)
      val |= (text_buffer[addr - text_start + i] << i*8);
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<4;i++)
      val |= (data_buffer[addr - data_start + i] << i*8);
  }
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<4;i++)
      val |= (SYSMEM_section_buffer[addr - SYSMEM_section_start + i] << i*8);
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) {
    printf ("mem mapped file 0x%x\n",addr);
    for(int i = 0; i < 4; i++)
      val |= *(uint8_t *)(addr + i) << i*8;
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (ldu32).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }

#ifdef DEBUG2
  printf("ldu32 [0x%x] -> 0x%08x\n",addr, val);
#endif

  return val;
}
uint16_t ProgramStandAlone::ldu16(uint32_t addr)
{
#ifdef DEBUG2
  printf("  addr(ldu16) 0x%08x \n", addr);
#endif
  uint16_t val = 0;
  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<2;i++)
      val |= (stack_buffer[addr - stack_start + i] << i*8);
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<2;i++)
      val |= (text_buffer[addr - text_start + i] << i*8);
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<2;i++)
      val |= (data_buffer[addr - data_start + i] << i*8);
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) {
    for(int i = 0; i < 2; i++)
      val |= *(uint8_t *)(addr + i) << i*8;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<2;i++)
      val |= (SYSMEM_section_buffer[addr - SYSMEM_section_start + i] << i*8);
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (ldu16).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
  return val;
}
uint8_t  ProgramStandAlone::ldu08(uint32_t addr)
{
#ifdef DEBUG2
  printf("  addr(ldu08) 0x%08x \n", addr);
#endif
  uint8_t val = 0;
  if (addr >= stack_start && addr < stack_end)
  {
      val = stack_buffer[addr - stack_start];
  }
  else if (addr >= text_start && addr < text_end)
  {
      val = text_buffer[addr - text_start];
  }
  else if (addr >= data_start && addr < data_end)
  {
      val = data_buffer[addr - data_start];
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) {
      val = *(uint8_t *)addr;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
      val = SYSMEM_section_buffer[addr - SYSMEM_section_start];
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (ldu08).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
  return val;
}

int64_t ProgramStandAlone::lds64(uint32_t addr)
{
  return (static_cast<int64_t>(ldu64(addr)));
}
int32_t ProgramStandAlone::lds32(uint32_t addr)
{
  return (static_cast<int32_t>(ldu32(addr)));
}
int16_t ProgramStandAlone::lds16(uint32_t addr)
{
  return (static_cast<int16_t>(ldu16(addr)));
}
int8_t  ProgramStandAlone::lds08(uint32_t addr)
{
  return (static_cast<int8_t>(ldu08(addr)));
}

uint32_t ProgramStandAlone::getValAtAddr(uint32_t addr) {
  uint32_t val;
  printf("  addr(getValAtAddr) 0x%08x \n", addr);
  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<4;i++)
      val |= (stack_buffer[addr - stack_start + i] << i*8);
  }   
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<4;i++)
      val |= (text_buffer[addr - text_start + i] << i*8);
  }   
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<4;i++)
      val |= (data_buffer[addr - data_start + i] << i*8);
  }   
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<4;i++)
      val |= (data_buffer[addr - data_start + i] << i*8);
  }   
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end)
  {
    printf("getValAtAddr mem_mapped_file \n");
    for(int i=0;i<4;i++)
      val |= *(uint8_t *)(addr + i ) << i*8;
  }
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (getVal).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
  return val;
}

void ProgramStandAlone::st64(uint32_t addr, uint64_t data)
{
#ifdef DEBUG2
  printf("  addr(st64) 0x%08x data 0x%016x \n", addr, data);
#endif
  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<8;i++)
      stack_buffer[addr - stack_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<8;i++)
      text_buffer[addr - text_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<8;i++)
      data_buffer[addr - data_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) 
  {
    for(int i = 0; i < 8; i++)
      *(uint8_t *)(addr + i) = (data >> i*8) & 0xFF;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<8;i++)
      SYSMEM_section_buffer[addr - SYSMEM_section_start + i]  = (data >> i*8) & 0xFF;
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (st64).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
}
void ProgramStandAlone::st32(uint32_t addr, uint32_t data)
{
#ifdef DEBUG2
  printf("  addr(st32) 0x%08x, 0x%08x, stack_start 0x%08x \n", addr, data, stack_start);
#endif

  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<4;i++) {
      stack_buffer[addr - stack_start + i]  = (data >> i*8) & 0xFF;
    }
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<4;i++)
      text_buffer[addr - text_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<4;i++)
      data_buffer[addr - data_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) 
  {
    for(int i = 0; i < 4; i++)
      *(uint8_t *)(addr + i) = (data >> i*8) & 0xFF;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<4;i++)
      SYSMEM_section_buffer[addr - SYSMEM_section_start + i]  = (data >> i*8) & 0xFF;
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (st32).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    printf("data mapped 0x%x-0x%x\n",data_start,data_end);
    printf("system mapped 0x%x-0x%x\n",SYSMEM_section_start,SYSMEM_section_end);
    exit(-5);
  }
}
void ProgramStandAlone::st16(uint32_t addr, uint16_t data)
{
#ifdef DEBUG2
  printf("  addr(st16) 0x%08x \n", addr);
#endif
  if (addr >= stack_start && addr < stack_end)
  {
    for(int i=0;i<2;i++)
      stack_buffer[addr - stack_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= text_start && addr < text_end)
  {
    for(int i=0;i<2;i++)
      text_buffer[addr - text_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= data_start && addr < data_end)
  {
    for(int i=0;i<2;i++)
      data_buffer[addr - data_start + i]  = (data >> i*8) & 0xFF;
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) 
  {
    for(int i = 0; i < 2; i++)
      *(uint8_t *)(addr + i) = (data >> i*8) & 0xFF;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
    for(int i=0;i<2;i++)
      SYSMEM_section_buffer[addr - SYSMEM_section_start + i]  = (data >> i*8) & 0xFF;
  }
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (st16).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
}
void ProgramStandAlone::st08(uint32_t addr, uint8_t  data)
{
#ifdef DEBUG2
  printf("  addr(st08) 0x%08x data 0x%08x \n", addr, data);
#endif
  if (addr >= stack_start && addr < stack_end)
  {
      stack_buffer[addr - stack_start ]  = data;
  }
  else if (addr >= text_start && addr < text_end)
  {
      text_buffer[addr - text_start ]  = data;
  }
  else if (addr >= data_start && addr < data_end)
  {
      data_buffer[addr - data_start ]  = data;
  }
  else if (addr >= mem_mapped_file_start && addr < mem_mapped_file_end) 
  {
    *(uint8_t *)addr = data;
  }
  //**
  else if (addr >= SYSMEM_section_start && addr < SYSMEM_section_end)
  {
      SYSMEM_section_buffer[addr - SYSMEM_section_start]  = data;
  } 
  //**
  else
  {
    printf("Cannot map the addr:0X%x to any section in the object file (st08).\n", addr);
    printf("mem mapped 0x%x-0x%x\n",mem_mapped_file_start,mem_mapped_file_end);
    exit(-5);
  }
}


uint32_t ProgramStandAlone::get_reg(uint8_t reg, TranslationType ttype)
{
  if(ttype == ARM || ttype == THUMB32 || ttype == THUMB) {
    if (reg == LREG_SP)
      return stack_pointer_start; 
    else if (reg == LREG_PC) {
      uint32_t pc;
      if (ttype == THUMB)
        pc = get_start_address()|0x1; // THUMB
      else
        pc = get_start_address()& (~0x1UL);   // ARM/SPARC

      return pc;
    }else if (reg == LREG_IP)
      return 0;
  }
  else if(ttype == SPARC32) {
    if(reg == LREG_R14)
      return stack_pointer_start; 
  }
  if (reg == LREG_PC) {
    return get_start_address();
  }

  return 0;
}
void ProgramStandAlone::set_reg(uint8_t reg, uint32_t val)
{
}
uint32_t ProgramStandAlone::do_syscall(ProgramBase *prog, int num, uint32_t arg1,
        uint32_t arg2, uint32_t arg3, uint32_t arg4,
        uint32_t arg5, uint32_t arg6, FILE *syscallTrace)
{
  return do_syscall_arm(prog, num, arg1, arg2, arg3, arg4, arg5, arg6, syscallTrace);
}
void ProgramStandAlone::target_set_brk(uint32_t brk_addr)
{
    target_set_brk_arm(brk_addr);
}
uint32_t *ProgramStandAlone::get_data_buffer(uint32_t *size)
{
  *size    = data_size;
  return (uint32_t *)data_buffer;
}
void ProgramStandAlone::set_data_buffer(uint32_t size, uint8_t *data_ptr)
{
  data_size   = size;
  data_buffer = data_ptr;

  data_end = data_start + size;
}
void ProgramStandAlone::set_mem_mapped_file_endpts(long mapped_addr, uint32_t size) 
{
  mem_mapped_file_start = mapped_addr;
  mem_mapped_file_end   = mapped_addr + size;
}
