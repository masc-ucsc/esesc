#ifndef PROGRAMBASE_H
#define PROGRAMBASE_H

#include "nanassert.h"
#include "InstOpcode.h"

class ProgramBase {
private:
protected:
public:
  ProgramBase() {
  };

  virtual ~ProgramBase() {
  };
  virtual uint32_t *g2h(uint32_t addr)=0;
  virtual uint32_t get_start_address() = 0;
  virtual uint32_t get_code32(uint32_t pc) = 0;
  virtual uint32_t *get_data_buffer(uint32_t *size) = 0;
  virtual void set_data_buffer(uint32_t size, uint8_t *data_ptr) = 0;
  virtual uint32_t getValAtAddr(uint32_t addr) = 0;

  virtual void dump(uint32_t pc) = 0;

  virtual uint64_t ldu64(uint32_t addr) = 0;
  virtual uint32_t ldu32(uint32_t addr) = 0;
  virtual uint16_t ldu16(uint32_t addr) = 0;
  virtual uint8_t  ldu08(uint32_t addr) = 0;

  virtual int64_t lds64(uint32_t addr) = 0;
  virtual int32_t lds32(uint32_t addr) = 0;
  virtual int16_t lds16(uint32_t addr) = 0;
  virtual int8_t  lds08(uint32_t addr) = 0;

  virtual void st64(uint32_t addr, uint64_t data) = 0;
  virtual void st32(uint32_t addr, uint32_t data) = 0;
  virtual void st16(uint32_t addr, uint16_t data) = 0;
  virtual void st08(uint32_t addr, uint8_t  data) = 0;
  virtual uint32_t get_reg(uint8_t reg, TranslationType ttype = ARM) = 0;
  virtual void set_reg(uint8_t reg, uint32_t val) = 0;
  virtual uint32_t do_syscall(ProgramBase *prog, int num, uint32_t arg1,
                                 uint32_t arg2, uint32_t arg3, uint32_t arg4,
                                 uint32_t arg5, uint32_t arg6, FILE *syscallTrace) = 0; 
  virtual void set_mem_mapped_file_endpts(long mapped_addr, uint32_t size) = 0;
};

#endif

