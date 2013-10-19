#ifndef CRACK_SCQEMU_H_
#define CRACK_SCQEMU_H_

#define CINST_CLZ         0x1FFFF   //Count Leading Zeroes
#define CINST_MRC         0x2FFFF   //Move from Co-processor register to ARM register
#define CINST_VABS        0x3FFFF   //Vector Absolute
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

#endif
