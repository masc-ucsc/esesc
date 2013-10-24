#ifndef SCCORE_H
#define SCCORE_H

#include "nanassert.h"

#include "scstate.h"
#include "scuop.h"

#include "ProgramBase.h"

class Sccore {
private:
protected:

  Scstate state;
  bool flush;
  ProgramBase *prog;
  FILE *syscallTrace;
  bool nop;

public:
  Sccore(FlowID _fid, ProgramBase *prog);

  void reset();

  void execute(const Scuop *uop);

  void dump(const char *str) const;
  void validateRegister(int reg_num, char *exp_reg_val) const;
  void dumpPC() const;

  void sync() {
    state.sync();
  }

  void setCrackInt(CrackBase * crackInt_) { state.setCrackInt(crackInt_); };
  DataType getPC(){
    return state.crackInt->getPC();
  }
  void setPC(DataType addr){
    state.crackInt->setPC(addr);
  }

  TranslationType getTType(){
    return state.getTType();
  }
  void setReg(uint8_t reg, DataType data){
    state.setReg(reg, data);
  }

  void setSyscallTraceFile(FILE *fp) {
    syscallTrace = fp;
  }

  bool getFlushDecode(){ return flush; }
  void resetFlushDecode(){ flush = false; }

  bool getNOPStatus() { return nop; }
  void resetNOPStatus() { nop = false; }

inline  uint8_t calcICC(uint64_t src1, uint64_t src2, uint64_t res){
    uint8_t icc = 0;
    icc = (((int64_t)res < 0) ? (1 << 3) : 0);
    icc = icc | ((res == 0) ? (1 << 2) : 0);
    icc = icc | ((((int64_t)res < (int64_t)src1) || (((int64_t)res < (int64_t)src2))) ? (1 <<1) : 0);
    //icc = icc | ((((int64_t) src1 < (int64_t)src2) ) ? (1 <<1) : 0);
    return icc;
  }
inline  uint8_t calcICC_sub(uint64_t src1, uint64_t src2, uint64_t res){
    uint8_t icc = 0;
    icc = (((int64_t)res < 0) ? (1 << 3) : 0);
    icc = icc | ((res == 0) ? (1 << 2) : 0);
    //icc = icc | ((((int64_t)res < (int64_t)src1) || (((int64_t)res < (int64_t)src2))) ? (1 <<1) : 0);
    icc = icc | (((src1) < (src2))  ? (1 <<1) : 0);
    return icc;
  }
inline  uint8_t calcNZ(uint64_t res){
    uint8_t icc = 0;
    icc = (((int64_t)res < 0) ? (1 << 3) : 0); //neg
    icc = icc | ((res == 0) ? (1 << 2) : 0); //eq
    return icc;
  }

uint8_t calcNZ32(uint32_t res) {
    uint8_t icc = 0;
    uint8_t zero = 0;
    uint8_t negative = 0;

    negative = ((int32_t)res < 0) ? (1 << 3) : 0; //neg
    icc = negative;
    zero = (res == 0) ? (1 << 2) : 0; //eq
    icc = icc | zero;
    return icc;
}

uint8_t calcNZ16(uint16_t res) {
    uint8_t icc = 0;
    uint8_t zero = 0;
    uint8_t negative = 0;

    negative = ((int16_t)res < 0) ? (1 << 3) : 0; //neg
    icc = negative;
    zero = (res == 0) ? (1 << 2) : 0; //eq
    icc = icc | zero;
    return icc;
}

uint8_t calcNZ8(uint8_t res) {
    uint8_t icc = 0;
    uint8_t zero = 0;
    uint8_t negative = 0;

    negative = ((int8_t)res < 0) ? (1 << 3) : 0; //neg
    icc = negative;
    zero = (res == 0) ? (1 << 2) : 0; //eq
    icc = icc | zero;
    return icc;
}

CP15Type * getcp15ptr(){
  return &state.cp15;
}


};



#endif
