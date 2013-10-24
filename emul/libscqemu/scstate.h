#ifndef SCSTATE_H
#define SCSTATE_H

#include "nanassert.h"

#include "scuop.h"
#include "CrackBase.h"
#include "RAWDInst.h"

typedef uint8_t  CCType;
    /* System control coprocessor (cp15) */
typedef    struct {
  uint32_t c0_cpuid;
  uint32_t c0_cachetype;
  uint32_t c0_ccsid[16]; /* Cache size.  */
  uint32_t c0_clid; /* Cache level.  */
  uint32_t c0_cssel; /* Cache size selection.  */
  uint32_t c0_c1[8]; /* Feature registers.  */
  uint32_t c0_c2[8]; /* Instruction set registers.  */
  uint32_t c1_sys; /* System control register.  */
  uint32_t c1_coproc; /* Coprocessor access register.  */
  uint32_t c1_xscaleauxcr; /* XScale auxiliary control register.  */
  uint32_t c1_secfg; /* Secure configuration register. */
  uint32_t c1_sedbg; /* Secure debug enable register. */
  uint32_t c1_nseac; /* Non-secure access control register. */
  uint32_t c2_base0; /* MMU translation table base 0.  */
  uint32_t c2_base1; /* MMU translation table base 1.  */
  uint32_t c2_control; /* MMU translation table base control.  */
  uint32_t c2_mask; /* MMU translation table base selection mask.  */
  uint32_t c2_base_mask; /* MMU translation table base 0 mask. */
  uint32_t c2_data; /* MPU data cachable bits.  */
  uint32_t c2_insn; /* MPU instruction cachable bits.  */
  uint32_t c3; /* MMU domain access control register
                  MPU write buffer control.  */
  uint32_t c5_insn; /* Fault status registers.  */
  uint32_t c5_data;
  uint32_t c6_region[8]; /* MPU base/size registers.  */
  uint32_t c6_insn; /* Fault address registers.  */
  uint32_t c6_data;
  uint32_t c9_insn; /* Cache lockdown registers.  */
  uint32_t c9_data;
  uint32_t c9_pmcr_data; /* Performance Monitor Control Register */
  uint32_t c9_useren; /* user enable register */
  uint32_t c9_inten; /* interrupt enable set/clear register */
  uint32_t c12_vbar; /* secure/nonsecure vector base address register. */
  uint32_t c12_mvbar; /* monitor vector base address register. */
  uint32_t c13_fcse; /* FCSE PID.  */
  uint32_t c13_context; /* Context ID.  */
  uint32_t c13_tls1; /* User RW Thread register.  */
  uint32_t c13_tls2; /* User RO Thread register.  */
  uint32_t c13_tls3; /* Privileged Thread register.  */
  uint32_t c15_cpar; /* XScale Coprocessor Access Register */
  uint32_t c15_ticonfig; /* TI925T configuration byte.  */
  uint32_t c15_i_max; /* Maximum D-cache dirty line index.  */
  uint32_t c15_i_min; /* Minimum D-cache dirty line index.  */
  uint32_t c15_threadid; /* TI debugger thread-ID.  */
} CP15Type;

class Scstate {
private:
protected:

#ifdef DEBUG
  bool     mod[LREG_SCLAST];
  bool     accum_mod[LREG_SCLAST];
#endif

  DataType rf[LREG_SCLAST];
  CCType   iccrf[LREG_SCLAST];

  FlowID fid;
  //uint64_t pc;

public:

  CP15Type cp15;

  CrackBase * crackInt; // to be removed
  void reset() {
    for(int i=0;i<LREG_SCLAST;i++) {
      rf[i]  = 0;
      IS(mod[i] = true);
      iccrf[i] = 0;
     }
  }

  TranslationType getTType(){
    DataType tt = getReg(LREG_TTYPE);
    return TranslationType(tt);
  }

  inline DataType getReg(uint8_t reg) const {
    I(reg<LREG_SCLAST);
    GI(reg == LREG_PC, rf[reg] == crackInt-> getPC());

    return rf[reg];
  }

  inline CCType getICC(uint8_t reg) const {
    I(reg<LREG_SCLAST);

    return iccrf[reg]; // Could be FCC or ICC
  }

  inline void setReg(uint8_t reg, DataType data, CCType cc=0) {
    I(reg<LREG_SCLAST);
    if (reg == 0)
      return;
    GI(reg == LREG_PC, crackInt->getPC() == data);

    rf[reg]    = data;
    IS(mod[reg]= true);
    iccrf[reg] = cc; // Could be FCC or ICC
  }

  inline void setDst(const Scuop *op, DataType data, CCType cc=0) {
    if (op->uop.seticc){
      iccrf[LREG_ICC] = cc;
      IS(mod[LREG_ICC] = true);
    }
    if (op->uop.dst==0)
      return;

    I(op->uop.dst<LREG_SCLAST);
    if (op->uop.dst < 32) // Program visible registers
      rf[op->uop.dst]  = data & 0xFFFFFFFF;
    else
      rf[op->uop.dst]  = data; 
    IS(mod[op->uop.dst] = true); // to be removed
    iccrf[op->uop.dst] = cc; // Could be FCC or ICC
  }

  Scstate(FlowID _fid) {
    reset();
    fid = _fid;
  }

  FlowID getFid(){ return fid; }

  void sync();

  void setCrackInt(CrackBase * crackInt_) { crackInt = crackInt_; };

  void dump(const char *str) const;
  //void dumpPC() const;
  void validateRegister(int reg_num, char *exp_reg_val) const;

};

#endif
