#ifndef SCUOP_H
#define SCUOP_H

#include "nanassert.h"

#include "InstOpcode.h"

class Scuop {
private:
protected:

public:
  struct { 
    // SCOORE ISA expanded after the crack stage
    bool useImm; //:1;
    bool seticc; //:1;
    bool setrnd; //:1;
    bool getrnd; //:1;
    uint8_t op;     //:7; // Sub operation type
    uint8_t src1;   //:7; // Logic Register
    uint8_t src2;   //:7;
    uint8_t dst;    //:7;

    int32_t imm; // Signed
  } uop;

  Scuop() {
    uop.op = 0; // Invalid op
  };

  void set(Scopcode op, unsigned int src1, unsigned int src2, bool useImm, int32_t imm, unsigned int dst, bool seticc, bool setrnd, bool getrnd) {
    uop.op     = op;
    uop.src1   = src1;
    uop.src2   = src2;
    uop.useImm = useImm;
    uop.imm    = imm;
    uop.dst    = dst;
    uop.seticc = seticc;
    uop.setrnd = setrnd;
    uop.getrnd = getrnd;
  }

  Scuop(Scopcode op, unsigned int src1, unsigned int src2, bool useImm, int32_t imm, unsigned int dst, bool seticc, bool setrnd, bool getrnd) {
    uop.op     = op;
    uop.src1   = src1;
    uop.src2   = src2;
    uop.useImm = useImm;
    uop.imm    = imm;
    uop.dst    = dst;
    uop.seticc = seticc;
    uop.setrnd = setrnd;
    uop.getrnd = getrnd;
  }

  const char *getRegString(unsigned int reg) const;
  const char *getOpString(unsigned int op) const;
  //const char *getRegString(unsigned int reg) const {
  //  return getRegString(static_cast<RegType>(reg));
  //};

  void dump(const char *str) const;

};

#endif
