
// LINK: pp.c /usr/lib/libbfd-*multiarch*.so /usr/lib/libopcodes-*multiarch*.so

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ProgramQEMU.h"

extern "C" uint64_t qemuesesc_setReg(void * env, uint8_t reg, uint64_t data);
extern "C" uint32_t do_syscall(ProgramBase *prog, int num, uint32_t arg1,
                    uint32_t arg2, uint32_t arg3, uint32_t arg4,
                   uint32_t arg5, uint32_t arg6);

ProgramQEMU::ProgramQEMU(void *_env) 
{
  env = _env;
}


uint32_t ProgramQEMU::get_start_address() 
{
  return get_reg(15, ARM); // ARM 15 == PC
}

uint32_t ProgramQEMU::get_code32(uint32_t pc) 
{
  return esesc_ldu32(pc);
}

uint64_t ProgramQEMU::ldu64(uint32_t addr) 
{
  return esesc_ldu64(addr);
}
uint32_t ProgramQEMU::ldu32(uint32_t addr)
{
  return esesc_ldu32(addr);
}
uint16_t ProgramQEMU::ldu16(uint32_t addr)
{
  return esesc_ldu16(addr);
}
uint8_t  ProgramQEMU::ldu08(uint32_t addr)
{
  return esesc_ldu08(addr);
}

int64_t ProgramQEMU::lds64(uint32_t addr)
{
  return esesc_ldu64(addr);
}
int32_t ProgramQEMU::lds32(uint32_t addr)
{
  return esesc_ldu32(addr);
}
int16_t ProgramQEMU::lds16(uint32_t addr)
{
  return esesc_lds16(addr);
}
int8_t  ProgramQEMU::lds08(uint32_t addr)
{
  return esesc_lds08(addr);
}

void ProgramQEMU::st64(uint32_t addr, uint64_t data) 
{
  esesc_st64(addr, data);
}
void ProgramQEMU::st32(uint32_t addr, uint32_t data)
{
  esesc_st32(addr, data);
}
void ProgramQEMU::st16(uint32_t addr, uint16_t data)
{
  esesc_st16(addr, data);
}
void ProgramQEMU::st08(uint32_t addr, uint8_t  data)
{
  esesc_st08(addr, data);
}

uint32_t ProgramQEMU::do_syscall(ProgramBase *prog, uint32_t num, uint32_t arg1,
    uint32_t arg2, uint32_t arg3, uint32_t arg4,
    uint32_t arg5, uint32_t arg6)
{
  return ::do_syscall(prog, num, arg1, arg2, arg3, arg4, arg5, arg6);
}
uint32_t ProgramQEMU::get_reg(uint8_t reg, TranslationType dontcare)
{
  return qemuesesc_getReg(env, reg);
}
void ProgramQEMU::set_reg(uint8_t reg, uint32_t val)
{
  qemuesesc_setReg(env, reg, val);
}
//uint32_t *ProgramQEMU::g2h(uint32_t addr)
//{
//  return esesc_g2h(addr);
//}  

void ProgramQEMU::dump(uint32_t pc) 
{
  printf("0x%x : 0x%x\n", pc ,get_code32(pc));
}
