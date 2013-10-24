#include "crackinst.h"

void CrackInst::setup(RAWDInst *rinst, InstOpcode iop, Scopcode sop, uint8_t srcA,   
                         uint8_t srcB, uint32_t immA, uint8_t dstA,  
                         unsigned seticc, unsigned  setRND, unsigned getRND)
{
  I(iop != iOpInvalid);
  
  UOPPredecType *inst = rinst->getNewInst();
  bool useImm = false;
  
  if(immA != 0)
    useImm = true;
#ifdef SCOORE

  inst->set(sop, srcA, srcB, useImm, immA, dstA, seticc, setRND, getRND);
#ifdef DEBUG
  if (sop==OP_iRALU_move) {
    I(srcB==0);
    I(useImm==false);
  }
  if (sop== OP_U32_ADD || sop== OP_U64_ADD || sop==OP_U16_ADD){
    if ((srcB == 0 || srcA==0) && immA ==0) {
      //pretty useless add. Try to optimize the code
      I(0);
    }
  }
#endif
#else
  RegType dstB = LREG_NoDependence;
  if (seticc)
    dstB = LREG_ICC;
  else if (setRND)
    dstB = LREG_RND;

  inst->set(iop, static_cast<RegType>(srcA), static_cast<RegType>(srcB), static_cast<RegType>(dstA), dstB, useImm);
  I(rinst->getNumInst() < 150); // really?! an instruction cracked to 150 uops?
#endif
} 

