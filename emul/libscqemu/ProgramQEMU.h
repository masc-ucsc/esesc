#ifndef PROGRAMQEMU_H
#define PROGRAMQEMU_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ProgramBase.h"

#include "qemumin.h"

extern "C" uint64_t qemuesesc_getReg(void * env, uint8_t reg);
extern "C" uint32_t  qemuesesc_getCP15tls2(void * env);

class ProgramQEMU : public ProgramBase {
private:
  void *env;

protected:
public:
  ProgramQEMU(void *env);
  virtual ~ProgramQEMU() {
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

  uint32_t do_syscall(ProgramBase *prog, uint32_t num, uint32_t arg1,
                      uint32_t arg2, uint32_t arg3, uint32_t arg4,
                      uint32_t arg5, uint32_t arg6);

  //uint32_t do_syscall();
  uint32_t get_reg(uint8_t reg, TranslationType dontcare = ARM);
  void set_reg(uint8_t reg, uint32_t val);
  uint32_t *g2h(uint32_t addr);
};

#endif
