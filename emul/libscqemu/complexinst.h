#ifndef COMPLEXINST_H_
#define COMPLEXINST_H_
#include <stdint.h>
#include "ProgramBase.h"

#define CINST_CLZ         0x1FFFF   //Count Leading Zeroes
#define CINST_MRC         0x2FFFF   //Move from Co-processor register to ARM register
#define CINST_VABS        0x3FFFF   //Vector Absolute (Saturate)
#define CINST_VABA        0x4FFFF   //Vector Absolute Accumulate 
#define CINST_VABD        0x5FFFF   //Vector Absolute Difference
#define CINST_VCLZ        0x6FFFF   //Vector Count Leading Zeroes
#define CINST_VCLS        0x7FFFF   //Vector Count Leading Sign bits
#define CINST_VCNT        0x8FFFF   //Vector Count set bits
#define CINST_VQSHL       0x9FFFF   //Vector Saturating Left Shift
#define CINST_VQRSHL      0xAFFFF   //Vector Saturating Rounding Left Shift
#define CINST_VQDMULH     0xBFFFF   //Vector Saturating (Rounding) Doubling Multiply Returning High Half
#define CINST_VQDMLAL_SL  0xCFFFF   //Vector Saturating Doubling Multiply Accumulate Long/Subtract Long
#define CINST_VMULPOLY    0xDFFFF   //Vector Multiply Polynomial
#define CINST_VQSHRN      0xEFFFF   //Vector Saturating (Rounding) Shift Right Narrow
#define CINST_VQMOVN      0xFFFFF   //Vector Saturating (Unsigned) Move Narrow
#define CINST_VCVTB_HTOS  0x2FFFFF  //Vector Convert Bottom Half to Single
#define CINST_VCVTT_HTOS  0x3FFFFF  //Vector Convert Top Half to Single
#define CINST_VCVTFXFP    0x4FFFFF  //Vector Convert between (FP and int,sp,dp,fixed)
#define CINST_VCVTB_STOH  0x5FFFFF  //Vector Convert Bottom Single to Half
#define CINST_VCVTT_STOH  0x6FFFFF  //Vector Convert Top Single to Half

#define CVAR_TLS2_OFFST 19

uint32_t do_clz(ProgramBase *prog, uint32_t arg1);
uint32_t do_mrc(ProgramBase *prog, uint32_t arg1);
uint32_t do_vabs(ProgramBase *prog, uint32_t arg1);
uint32_t do_vaba(ProgramBase *prog, uint32_t arg1);
uint32_t do_vabd(ProgramBase *prog, uint32_t arg1);
uint32_t do_vclz(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcls(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcnt(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcvtfxfp(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqshl(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqrshl(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqdmulh(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqdmlal_sl(ProgramBase *prog, uint32_t arg1);
uint32_t do_vmulpoly(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqshrn(ProgramBase *prog, uint32_t arg1);
uint32_t do_vqmovn(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcvtb_htos(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcvtt_htos(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcvtb_stoh(ProgramBase *prog, uint32_t arg1);
uint32_t do_vcvtt_stoh(ProgramBase *prog, uint32_t arg1);

#endif
