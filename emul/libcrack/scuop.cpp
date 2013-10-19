#include "scuop.h"

const char *Scuop::getRegString(unsigned int reg) const 
// User friendly register {{{1 
{

  const char *regString[] = { "ZERO", "R0", "R1", "R2", "R3",
                              "R4", "R5", "R6", "R7",
                              "R8", "R9", "R10", "R11",
                              "R12", "R13/SP", "R14/LINK", "R15/PC",
                              "R16", "R17", "R18", "R19",
                              "R20", "R21", "R22", "R23",
                              "R24", "R25", "R26", "R27",
                              "R28", "R29", "R30", "R31"
                            };
  
#ifdef SPARC32
  const char *regString[] = { "%g0", "%g1", "%g2", "%g3",
                              "%g4", "%g5", "%g6", "%g7",
                              "%o0", "%o1", "%o2", "%o3",
                              "%o4", "%o5", "%sp", "%o7",
                              "%l0", "%l1", "%l2", "%l3",
                              "%l4", "%l5", "%l6", "%l7",
                              "%i0", "%i1", "%i2", "%i3",
                              "%i4", "%i5", "%fp", "%i7"
                            };
#endif
  
  const char *fpRegString[] = { "FP0", "FP1", "FP2", "FP3",
                                "FP4", "FP5", "FP6", "FP7",
                                "FP8", "FP9","FP10", "FP11",
                                "FP12", "FP13", "FP14", "FP15",
                                "FP16", "FP17", "FP18", "FP19",
                                "FP20", "FP21", "FP22", "FP23",
                                "FP24", "FP25", "FP26", "FP27",
                                "FP28", "FP29", "FP30", "FP31",
                                "RND"
                              };
  
  const char *statusRegString[] = {  "ARCH0/PSR",   "ARCH1/ICC",  "ARCH2/CWP",
                                     "ARCH3/Y/GE_FLAG", "ARCH4/Q_FLAG", "ARCH5/WIM",
                                     "ARCH6/FSR", "ARCH7/FCC", "ARCH8/CEXC",
                                     "ARCH9", "ARCH10"
                                   };
  
  const char *tmpRegString[] = { "TMP1", "TMP2", "TMP3",
                                 "TMP4", "TMP5", "TMP6", 
                                 "TMP7", "TMP8", 
                                 "SYSMEM", "TTYPE"
                               };
  
  if(reg >= LREG_R0 && reg < LREG_FP0)
    return regString[reg];
  else if(reg >= LREG_FP0 && reg < LREG_ARCH0)
    return fpRegString[reg-LREG_FP0];
  else if(reg >= LREG_ARCH0 && reg < LREG_TMP1)
    return statusRegString[reg-LREG_ARCH0];
  else if(reg >= LREG_TMP1 && reg < LREG_SCLAST)
    return tmpRegString[reg-LREG_TMP1];
  else
    return "LREG_InvalidOutput";
}
// }}}

const char *Scuop::getOpString(unsigned op) const 
// User friendly optype {{{1 
{
  const char *opString[] = {  
    "OP_nop",

    "OP_S64_ADD",
    "OP_S32_ADD",
    "OP_S16_ADD",
    "OP_S08_ADD",
    "OP_U64_ADD",
    "OP_U32_ADD",
    "OP_U16_ADD",
    "OP_U08_ADD",
    "OP_S64_ADDXX",
    "OP_S32_ADDXX",
    "OP_S64_SUB",
    "OP_S32_SUB",
    "OP_S16_SUB",
    "OP_S08_SUB",
    "OP_U64_SUB",
    "OP_U32_SUB",
    "OP_U16_SUB",
    "OP_U08_SUB",
    "OP_S64_SLL",
    "OP_S32_SLL",
    "OP_S16_SLL",
    "OP_S08_SLL",
    "OP_S64_SRL",
    "OP_S32_SRL",
    "OP_S16_SRL",
    "OP_S08_SRL",
    "OP_S64_SRA",
    "OP_S32_SRA",
    "OP_S16_SRA",
    "OP_S08_SRA",
    "OP_S64_ROTR",
    "OP_S32_ROTR",
    "OP_S16_ROTR",
    "OP_S08_ROTR",
    "OP_S64_ROTRXX",
    "OP_S32_ROTRXX",
    "OP_S64_ANDN",
    "OP_S64_AND",
    "OP_S64_OR",
    "OP_S64_XOR",
    "OP_S64_ORN",
    "OP_S64_XNOR",
    "OP_S64_SADD",
    "OP_S32_SADD",
    "OP_S16_SADD",
    "OP_S08_SADD",
    "OP_U64_SADD",
    "OP_U32_SADD",
    "OP_U16_SSADD",
    "OP_U08_SADD",
    "OP_S64_SSUB",
    "OP_S32_SSUB",
    "OP_S16_SSUB",
    "OP_S08_SSUB",
    "OP_U64_SSUB",
    "OP_U32_SSUB",
    "OP_U16_SSUB",
    "OP_U08_SSUB",
    "OP_S64_COPY",
    "OP_S64_FCC2ICC",
    "OP_S64_JOINPSR",
    //NOTUSED
    "OP_S64_COPYICC",
    "OP_S64_GETICC",
    "OP_S64_PUTICC",
    "OP_S64_NOTICC",
    "OP_S64_ANDICC",
    "OP_S64_ORICC  ",
    "OP_S64_XORICC",
    //NOTUSED
    //NOTUSED
    //NOTUSED
    "OP_S64_COPYPCL",
    "OP_S64_COPYPCH",
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
 
    ///////////////////////////////////////////////////////////////////////
    //CONDITIONAL MOVES
    ////////////////////////////////////////////////////////////////////////
    "OP_U64_CMOV_A", //not passed
    "OP_U64_CMOV_E",
    "OP_U64_CMOV_NE",
    "OP_U64_CMOV_CS",
    "OP_U64_CMOV_CC",
    "OP_U64_CMOV_NEG",
    "OP_U64_CMOV_POS",
    "OP_U64_CMOV_VS",
    "OP_U64_CMOV_VC",
    "OP_U64_CMOV_C_AND_NOTZ",
    "OP_U64_CMOV_NOTC_OR_Z",
    "OP_U64_CMOV_GE",
    "OP_U64_CMOV_L",
    "OP_U64_CMOV_G",
    "OP_U64_CMOV_LE",
    "OP_U64_CMOV_LGU",
    "OP_U64_CMOV_EGU",
    "OP_U64_CMOV_LEG",
    "OP_U64_CMOV_UG",
    "OP_U64_CMOV_EL",
    "OP_U64_CMOV_EG",
    "OP_U64_CMOV_LU",
    "OP_U64_CMOV_LUE",
    "OP_U64_CMOV_LG",
    "OP_U64_CMOV_EU",
    //NOTUSED
    //NOTUSED
    //NOTUSED

    ///////////////////////////////////////////////////////////////////////
    //BRANCH ALU (BALU)
    ////////////////////////////////////////////////////////////////////////",
    "OP_U64_JMP_REG",
    "OP_U64_JMP_IMM",

    ///////////////////////////////////////////////////////////////////////
    //BRANCH ALU (BALU) BRANCH REGISTER 
    ////////////////////////////////////////////////////////////////////////
    //OP_U64_RBA //not passed
    "OP_U64_RBE",
    "OP_U64_RBNE",
    "OP_U64_RBCS",
    "OP_U64_RBCC",
    "OP_U64_RBNEG",
    "OP_U64_RBPOS",
    "OP_U64_RBVS",
    "OP_U64_RBVC",
    "OP_U64_RBC_OR_Z_NOT",
    "OP_U64_RBC_OR_Z",
    "OP_U64_RBC_AND_NOTZ",
    "OP_U64_RBNOTC_OR_Z",
    "OP_U64_RBGE",
    "OP_U64_RBL",
    "OP_U64_RBG",
    "OP_U64_RBLE",
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED

    ///////////////////////////////////////////////////////////////////////
    //BRANCH ALU (BALU) BRANCH IMMEDIATE
    ////////////////////////////////////////////////////////////////////////
    //OP_U64_LBA //not passed
    "OP_U64_LBE",
    "OP_U64_LBNE",
    "OP_U64_LBCS",
    "OP_U64_LBCC",
    "OP_U64_LBNEG",
    "OP_U64_LBPOS",
    "OP_U64_LBVS",
    "OP_U64_LBVC",
    "OP_U64_LBC_OR_Z_NOT",
    "OP_U64_LBC_OR_Z",
    "OP_U64_LBC_AND_NOTZ",
    "OP_U64_LBNOTC_OR_Z",
    "OP_U64_LBGE",
    "OP_U64_LBL",
    "OP_U64_LBG",
    "OP_U64_LBLE",
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    
    ///////////////////////////////////////////////////////////////////////
    //BRANCH ALU (BALU) FLOATING POINT BRANCH BASED ON FCC
    ////////////////////////////////////////////////////////////////////////
    "OP_U64_FBA", //not passed
    "OP_U64_FBUG",
    "OP_U64_FBLU",
    "OP_U64_FBLG",
    "OP_U64_FBLGU",
    "OP_U64_FBEU",
    "OP_U64_FBEG",
    "OP_U64_FBEGU",
    "OP_U64_FBEL",
    "OP_U64_FBLUE",
    "OP_U64_FBLEG",
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    //NOTUSED
    
    ///////////////////////////////////////////////////////////////////////
    //LOAD/STORE ALU (MEMORY ALU)
    ////////////////////////////////////////////////////////////////////////
    "OP_U64_LD_L", //Little Endian
    "OP_S32_LD_L", //Little Endian
    "OP_S16_LD_L", //Little Endian
    "OP_S08_LD",
    "OP_U32_LD_L", //Little Endian
    "OP_U16_LD_L", //Little Endian
    "OP_U08_LD",
 
    "OP_U64_LD_B", //Big Endian
    "OP_S32_LD_B", //Big Endian
    "OP_S16_LD_B", //Big Endian
    "OP_U32_LD_B", //Big Endian
    "OP_U16_LD_B", //Big Endian
    ///////////////////////////////////////////////////////////////////////
    //STORE ALU (SALU)
    ////////////////////////////////////////////////////////////////////////
    "OP_U64_ST_L", //Little Endian
    "OP_U32_ST_L", //Little Endian
    "OP_U16_ST_L", //Little Endian

    "OP_U64_ST_B", //Big Endian
    "OP_U32_ST_B", //Big Endian
    "OP_U16_ST_B", //Big Endian

    "OP_U08_ST",
    "OP_U32_STSC",
    
    ///////////////////////////////////////////////////////////////////////
    //STORE CALCULATE ADDRESS
    ///////////////////////////////////////////////////////////////////////
    "OP_U64_CADD_E",
    "OP_U64_CADD_NE",
    "OP_U64_CADD_CS",
    "OP_U64_CADD_CC",
    "OP_U64_CADD_NEG",
    "OP_U64_CADD_POS",
    "OP_U64_CADD_VS",
    "OP_U64_CADD_VC",
    "OP_U64_CADD_C_OR_Z_NOT",
    "OP_U64_CADD_C_OR_Z",
    "OP_U64_CADD_C_AND_NOTZ",
    "OP_U64_CADD_NOTC_OR_Z",
    "OP_U64_CADD_GE",
    "OP_U64_CADD_L",
    "OP_U64_CADD_G",
    "OP_U64_CADD_LE",
    
    "OP_U64_CSUB_E",
    "OP_U64_CSUB_NE",
    "OP_U64_CSUB_CS",
    "OP_U64_CSUB_CC",
    "OP_U64_CSUB_NEG",
    "OP_U64_CSUB_POS",
    "OP_U64_CSUB_VS",
    "OP_U64_CSUB_VC",
    "OP_U64_CSUB_C_OR_Z_NOT",
    "OP_U64_CSUB_C_OR_Z",
    "OP_U64_CSUB_C_AND_NOTZ",
    "OP_U64_CSUB_NOTC_OR_Z",
    "OP_U64_CSUB_GE",
    "OP_U64_CSUB_L",
    "OP_U64_CSUB_G",
    "OP_U64_CSUB_LE",
    
    ///////////////////////////////////////////////////////////////////////
    //ATOMICITY Instructions
    ////////////////////////////////////////////////////////////////////////
    "OP_U64_MARK_EX",
    "OP_U64_CHECK_EX",
    "OP_U64_ST_CHECK_EX",
    "OP_U64_CLR_ADDR", //NOTE: If addr==0 then clear everything
    "OP_U64_ST_COND",
    "OP_U32_ST_COND",
    "OP_U16_ST_COND",
    "OP_U08_ST_COND",

    //////////////////////////////////////
    //FPU ALU (CALU)
    //////////////////////////////////////
    "OP_C_JOINFSR",
  
    "OP_C_FSTOI",
    "OP_C_FSTOD",
    "OP_C_FMOVS",
    "OP_C_FNEGS",
    "OP_C_FABSS",
    "OP_C_FSQRTS",
    "OP_C_FADDS",
    "OP_C_FSUBS",
    "OP_C_FMULS",
    "OP_C_FDIVS",
    "OP_C_FSMULD",
    "OP_C_FCMPS",
    "OP_C_FCMPES",

    "OP_C_FDTOI",
    "OP_C_FDTOS",
    "OP_C_FMOVD",
    "OP_C_FNEGD",
    "OP_C_FABSD",
  
    "OP_C_FSQRTD",
    "OP_C_FADDD",
    "OP_C_FSUBD",
    "OP_C_FMULD",
    "OP_C_FDIVD",
    "OP_C_FCMPD",
    "OP_C_FCMPED",

    "OP_C_UMUL",
    "OP_C_UDIV",
    "OP_C_UDIVCC",
    "OP_C_UMULCC",

    "OP_C_FITOS",
    "OP_C_FITOD",

    "OP_C_CONCAT",
    "OP_C_MULSCC",

    "OP_C_SMUL",
    "OP_C_SDIV",
    "OP_C_SDIVCC",
    "OP_C_SMULCC",
    "OP_iRALU_move"
  };

  return opString[op];
}
// }}}

void Scuop::dump(const char *str) const
  /* dump uop {{{1  */
{
  printf("%s: %s = %s %s %s useImm=%d imm=%d seticc=%d setrnd=%d getrnd=%d"
      ,str
      ,getRegString(uop.dst) , getRegString(uop.src1), getOpString(uop.op), getRegString(uop.src2)
      ,uop.useImm, uop.imm, uop.seticc, uop.setrnd, uop.getrnd);

}
/* }}} */


