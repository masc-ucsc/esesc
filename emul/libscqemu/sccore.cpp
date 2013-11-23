//  Contributed by Jose Renau
//                 Ehsan K.Ardestani
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and 
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "sccore.h"
#include "qemumin.h"
#include "ahash.h"
#include "libsoftfpu/littleendian/softfloat.h"
#include "complexinst.h"

#define INT8_MAX 0x7F
#define INT8_MIN (-INT8_MAX - 1)
#define INT16_MAX 0x7FFF
#define INT16_MIN (-INT16_MAX - 1)
#define INT32_MAX 0x7FFFFFFFL
#define INT32_MIN (-INT32_MAX - 1L)
#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL
#define INT64_MIN (-INT64_MAX - 1LL)

#define UNSIGNEDMAX64 0xFFFFFFFFFFFFFFFFULL
#define UNSIGNEDMAX32 0xFFFFFFFFU
#define UNSIGNEDMAX16 0xFFFFU
#define UNSIGNEDMAX8 0xFFU

static AHash atomHash;

void Sccore::reset() 
  /* Reset the core to a known zero state {{{1 */
{
  state.reset();
}
/* }}} */

Sccore::Sccore(FlowID _fid, ProgramBase *_prog)
  /* Constructor {{{1 */
  :state(_fid)
  ,prog(_prog)
{
  reset();
  flush = false;
}
/* }}} */

void Sccore::execute(const Scuop *inst)
  /* Execute a single scuop instruction, and update state {{{1 */
{
  DataType res;
  uint8_t icc = 0;
  uint8_t icc2 = 0;

  uint8_t carry = 0;
  uint8_t overflow = 0;

  int64_t temp1, temp2;

  int32_t bottomS32, topS32;
  int32_t tempAS32, tempBS32;

  int16_t bottomS16, secondS16, thirdS16, lastS16;
  int16_t tempAS16, tempBS16;

  int8_t bottomS8, secondS8, thirdS8, fourthS8,
         fifthS8, sixthS8, seventhS8, lastS8;
  int8_t tempAS8, tempBS8;

  uint64_t tempAU64, tempBU64;

  uint32_t bottomU32, topU32;
  uint32_t tempAU32, tempBU32;

  uint16_t bottomU16, secondU16, thirdU16, lastU16;
  uint16_t tempAU16, tempBU16;

  uint8_t bottomU8, secondU8, thirdU8, fourthU8,
          fifthU8, sixthU8, seventhU8, lastU8;
  uint8_t tempAU8, tempBU8;

  uint8_t carry_bit;
  uint32_t arg1, arg2, arg3, arg4, arg5, arg6;

  float tempFSA, tempFSB, resFS;
  double tempFDA, tempFDB, resFD;

  switch(inst->uop.op) {
    case OP_iRALU_move: 
      temp1 = state.getReg(inst->uop.src1);
      res = temp1;
      state.setDst(inst, res, 0);
      break;
    case OP_S64_ADD :
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm) ;
      else
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 + temp2;
 
      icc = calcICC(temp1, temp2, res);
      overflow = ((temp1 < 0) && (temp2 < 0) && (res > 0)) || ((temp1 > 0) && (temp2 > 0) && (res < 0));
      icc = icc | overflow;

      state.setDst(inst, res, icc);
      break;
    case OP_S32_ADD:
      // Begin SSE
      tempAS32  = (state.getReg(inst->uop.src1) & 0xFFFFFFFF);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = (state.getReg(inst->uop.src2) & 0xFFFFFFFF);
      bottomS32 = tempAS32 + tempBS32; 

      icc = calcICC(uint64_t (tempAS32), uint64_t (tempBS32), uint64_t (bottomS32));
      overflow = ((tempAS32 < 0) && (tempBS32 < 0) && (bottomS32 > 0)) || ((tempAS32 > 0) && (tempBS32 > 0) && (bottomS32 < 0));
      icc = icc | overflow;

      tempAS32  = (state.getReg(inst->uop.src1) >> 32);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = ((uint64_t)state.getReg(inst->uop.src2) >> 32);
      topS32 = tempAS32 + tempBS32;
      res = ((((uint64_t)topS32) << 32) | bottomS32);
      // End SSE


      state.setDst(inst, res, icc);
      break;
    case OP_S16_ADD:
      tempAS16  = (state.getReg(inst->uop.src1) & 0xFFFF);
      if (inst->uop.useImm)
        tempBS16  = (((inst->uop.imm) ) & 0xFFFF);
      else
        tempBS16  = (state.getReg(inst->uop.src2) & 0xFFFF);

      bottomS16 = tempAS16 + tempBS16; 

      icc = calcICC(uint64_t (tempAS16), uint64_t (tempBS16), uint64_t (bottomS16));
      overflow = ((tempAS16 < 0) && (tempBS16 < 0) && (bottomS16 > 0)) || ((tempAS16 > 0) && (tempBS16 > 0) && (bottomS16 < 0));
      icc = icc | overflow;

      tempAS16  = (state.getReg(inst->uop.src1) >> 16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16  = inst->uop.imm & 0xFFFF;
      else
        tempBS16  = (state.getReg(inst->uop.src2) >> 16) & 0xFFFF;

      secondS16 = tempAS16 + tempBS16; 

      tempAS16  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFFFF;
      else
      tempBS16  = (state.getReg(inst->uop.src2) >> 32) & 0xFFFF;

      thirdS16 = tempAS16 + tempBS16; 

      tempAS16  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFFFF;
      else
      tempBS16  = (state.getReg(inst->uop.src2) >> 48) & 0xFFFF;

      lastS16 = tempAS16 + tempBS16; 

      res = (((int64_t) lastS16) << 48) | (((int64_t) thirdS16) << 32) | (((int64_t) secondS16) << 16) | bottomS16;

      state.setDst(inst, res, icc);
      break;
    case OP_S08_ADD:
      tempAS8  = (state.getReg(inst->uop.src1) & 0xFF);
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = (state.getReg(inst->uop.src2) & 0xFF);

      bottomS8 = tempAS8 + tempBS8; 

      icc = calcICC(uint64_t (tempAS8), uint64_t (tempBS8), uint64_t (bottomS8));
      overflow = ((tempAS8 < 0) && (tempBS8 < 0) && (bottomS8 > 0)) || ((tempAS8 > 0) && (tempBS8 > 0) && (bottomS8 < 0));
      icc = icc | overflow;

      tempAS8  = (state.getReg(inst->uop.src1) >> 8) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 8) & 0xFF;

      secondS8 = tempAS8 + tempBS8; 

      tempAS8  = (state.getReg(inst->uop.src1) >> 16) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 16) & 0xFF;

      thirdS8 = tempAS8 + tempBS8; 

      tempAS8  = (state.getReg(inst->uop.src1) >> 24) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 24) & 0xFF;

      fourthS8 = tempAS8 + tempBS8; 

      tempAS8  = (state.getReg(inst->uop.src1) >> 32) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFF;

      fifthS8 = tempAS8 + tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 40) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 40) & 0xFF;

      sixthS8 = tempAS8 + tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF;
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFF;

      seventhS8 = tempAS8 + tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 56) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = ((inst->uop.imm) & 0xFF);
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 56) & 0xFF;

      lastS8 = tempAS8 + tempBS8; 

      res = (((int64_t) lastS8) << 56)   | (((int64_t) seventhS8) << 48) |
            (((int64_t) sixthS8) << 40)  | (((int64_t) fifthS8) << 32)   |
            (((int64_t) fourthS8) << 32) | (((int64_t) thirdS8) << 32)   |
            (((int64_t) secondS8) << 32) | bottomS8;

      state.setDst(inst, res, icc);
      break;
    case OP_U64_ADD:
      tempAU64 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        tempBU64  = inst->uop.imm;
      else
        tempBU64 = state.getReg(inst->uop.src2);
      res = tempAU64 + tempBU64;
 
      icc = calcICC(uint64_t (tempAU64), uint64_t (tempBU64), uint64_t (res));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U32_ADD:
      // Begin SSE
      tempAU32  = (state.getReg(inst->uop.src1) & 0xFFFFFFFF);
      if (inst->uop.useImm)
        tempBU32  = inst->uop.imm & 0xFFFFFFFF;
      else
        tempBU32  = (state.getReg(inst->uop.src2) & 0xFFFFFFFF);
      bottomU32 = tempAU32 + tempBU32; 

      icc = calcICC(uint64_t (tempAU32), uint64_t (tempBU32), uint64_t (bottomU32));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU32  = ((uint64_t)state.getReg(inst->uop.src1) >> 32);
      if (inst->uop.useImm)
        tempBU32  = inst->uop.imm;
      else
        tempBU32  = ((uint64_t)state.getReg(inst->uop.src2) >> 32);
      topU32 = tempAU32 + tempBU32;
      res = ((((uint64_t)topU32) << 32) | bottomU32);
      
      // End SSE
      state.setDst(inst, res, icc);

      break;
    case OP_U16_ADD:
      tempAU16  = (state.getReg(inst->uop.src1) & 0xFFFF);
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF;
      else
        tempBU16  = (state.getReg(inst->uop.src2) & 0xFFFF);

      bottomU16 = tempAU16 + tempBU16; 

      icc = calcICC(uint64_t (tempAU16), uint64_t (tempBU16), uint64_t (bottomU16));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU16  = (state.getReg(inst->uop.src1) >> 16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF;
      else
        tempBU16  = (state.getReg(inst->uop.src2) >> 16) & 0xFFFF;

      secondU16 = tempAU16 + tempBU16; 

      tempAU16  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF;
      else
        tempBU16  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFFFF;

      thirdU16 = tempAU16 + tempBU16; 


      tempAU16  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF;
      else
        tempBU16  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFFFF;

      lastU16 = tempAU16 + tempBU16; 


      res = (((uint64_t) lastU16) << 48) | (((uint64_t) thirdU16) << 32) | (((uint64_t) secondU16) << 16) | bottomU16;

      state.setDst(inst, res, icc);
      break;
    case OP_U08_ADD:
      tempAU8  = (state.getReg(inst->uop.src1) & 0xFF);
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = (state.getReg(inst->uop.src2) & 0xFF);

      bottomU8 = tempAU8 + tempBU8; 

      icc = calcICC(uint64_t (tempAU8), uint64_t (tempBU8), uint64_t (bottomU8));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU8  = (state.getReg(inst->uop.src1) >> 8) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 8) & 0xFF;

      secondU8 = tempAU8 + tempBU8; 

      tempAU8  = (state.getReg(inst->uop.src1) >> 16) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 16) & 0xFF;

      thirdU8 = tempAU8 + tempBU8; 


      tempAU8  = (state.getReg(inst->uop.src1) >> 24) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 24) & 0xFF;

      fourthU8 = tempAU8 + tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFF;

      fifthU8 = tempAU8 + tempBU8; 


      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 40) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 40) & 0xFF;

      sixthU8 = tempAU8 + tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFF;

      seventhU8 = tempAU8 + tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 56) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF;
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 56) & 0xFF;

      lastU8 = tempAU8 + tempBU8; 


      res = (((uint64_t) lastU8) << 56)   | (((uint64_t) seventhU8) << 48) |
            (((uint64_t) sixthU8) << 40)  | (((uint64_t) fifthU8) << 32)   |
            (((uint64_t) fourthU8) << 32) | (((uint64_t) thirdU8) << 32)   |
            (((uint64_t) secondU8) << 32) | bottomU8;

      state.setDst(inst, res, icc);
      break;
    case OP_S64_ADDXX:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else
        temp2 = state.getReg(inst->uop.src2);
      carry_bit = (state.getICC(inst->uop.src1) >> 5) & 1;

      res = temp1 + temp2 + carry_bit;
 
      icc = calcICC(uint64_t (temp1), uint64_t (temp2), uint64_t (res));
      overflow = ((temp1 < 0) && (temp2 < 0) && (res > 0)) || ((temp1 > 0) && (temp2 > 0) && (res < 0));
      icc = icc | overflow;

      state.setDst(inst, res, icc);
      break;
    case OP_S32_ADDXX:
      tempAS32  = (state.getReg(inst->uop.src1) & 0xFFFFFFFF);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = (state.getReg(inst->uop.src2) & 0xFFFFFFFF);
      carry_bit = (state.getICC(inst->uop.src1) >> 2) & 1;

      bottomS32 = tempAS32 + tempBS32 + carry_bit; 

      icc = calcICC(uint64_t (tempAS32), uint64_t (tempBS32), uint64_t (bottomS32));
      overflow = ((tempAS32 < 0) && (tempBS32 < 0) && (bottomS32 > 0)) || ((tempAS32 > 0) && (tempBS32 > 0) && (bottomS32 < 0));
      icc = icc | overflow;

      tempAS32  = ((uint64_t)state.getReg(inst->uop.src1) >> 32);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = ((uint64_t)state.getReg(inst->uop.src2) >> 32);
      carry_bit = (state.getICC(inst->uop.src1) >> 5) & 1;

      topS32 = tempAS32 + tempBS32 + carry_bit;

      res = ((((uint64_t)topS32) << 32) | bottomS32);
      
      state.setDst(inst, res, icc);
      break;
    case OP_S64_SUB:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = inst->uop.imm; 
      else
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 - temp2;

      icc = calcICC(uint64_t (temp1), uint64_t (temp2), uint64_t (res));
      overflow = ((temp1 >= 0) && (temp2 < 0) && (res < 0)) || ((temp1 < 0) && (temp2 >= 0) && (res >= 0));
      icc = icc | overflow;

      state.setDst(inst, res, icc);
      break;
    case OP_S32_SUB:
      tempAS32  = (state.getReg(inst->uop.src1) & 0xFFFFFFFF);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = (state.getReg(inst->uop.src2) & 0xFFFFFFFF);
      bottomS32 = tempAS32 - tempBS32; 

      icc = calcICC_sub(uint64_t (tempAS32), uint64_t (tempBS32), uint64_t (bottomS32));
      overflow = ((tempAS32 >= 0) && (tempBS32 < 0) && (bottomS32 < 0)) || ((tempAS32 < 0) && (tempBS32 >= 0) && (bottomS32 >= 0));
      icc = icc | overflow;

      tempAS32  = ((uint64_t)state.getReg(inst->uop.src1) >> 32);
      if (inst->uop.useImm)
        tempBS32  = inst->uop.imm;
      else
        tempBS32  = ((uint64_t)state.getReg(inst->uop.src2) >> 32);
      topS32 = tempAS32 - tempBS32;

      res = ((((uint64_t)topS32) << 32) | bottomS32);

      state.setDst(inst, res, icc);
      break;
    case OP_S16_SUB:
      tempAS16  = (state.getReg(inst->uop.src1) & 0xFFFF);
      if (inst->uop.useImm)
        tempBS16  = inst->uop.imm & 0xFFFF;
      else
        tempBS16  = (state.getReg(inst->uop.src2) & 0xFFFF);

      bottomS16 = tempAS16 - tempBS16; 

      icc = calcICC_sub(uint64_t (tempAS16), uint64_t (tempAS16), uint64_t (bottomS16));
      overflow = ((tempAS16 >= 0) && (tempBS16 < 0) && (bottomS16 < 0)) || ((tempAS16 < 0) && (tempBS16 >= 0) && (bottomS16 >= 0));
      icc = icc | overflow;

      tempAS16  = (state.getReg(inst->uop.src1) >> 16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16  = inst->uop.imm & 0xFFFF;
      else
        tempBS16  = (state.getReg(inst->uop.src2) >> 16) & 0xFFFF;

      secondS16 = tempAS16 - tempBS16; 


      tempAS16  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16  = inst->uop.imm & 0xFFFF;
      else
        tempBS16  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFFFF;

      thirdS16 = tempAS16 - tempBS16; 

      tempAS16  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16  = inst->uop.imm & 0xFFFF;
      else
        tempBS16  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFFFF;

      lastS16 = tempAS16 - tempBS16; 


      res = (((int64_t) lastS16) << 48) | (((int64_t) thirdS16) << 32) | (((int64_t) secondS16) << 16) | bottomS16;

      state.setDst(inst, res, icc);
      break;
    case OP_S08_SUB:
      tempAS8  = (state.getReg(inst->uop.src1) & 0xFF);
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = (state.getReg(inst->uop.src2) & 0xFF);

      bottomS8 = tempAS8 - tempBS8; 

      icc = calcICC_sub(uint64_t (tempAS8), uint64_t (tempBS8), uint64_t (bottomS8));
      overflow = ((tempAS8 >= 0) && (tempBS8 < 0) && (bottomS8 < 0)) || ((tempAS8 < 0) && (tempBS8 >= 0) && (bottomS8 >= 0));
      icc = icc | overflow;

      tempAS8  = (state.getReg(inst->uop.src1) >> 8) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 8) & 0xFF;

      secondS8 = tempAS8 - tempBS8; 

      tempAS8  = (state.getReg(inst->uop.src1) >> 16) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 16) & 0xFF;

      thirdS8 = tempAS8 - tempBS8; 

      tempAS8  = (state.getReg(inst->uop.src1) >> 24) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = (state.getReg(inst->uop.src2) >> 24) & 0xFF;

      fourthS8 = tempAS8 - tempBS8; 


      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFF;

      fifthS8 = tempAS8 - tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 40) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 40) & 0xFF;

      sixthS8 = tempAS8 - tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFF;

      seventhS8 = tempAS8 - tempBS8; 

      tempAS8  = ((uint64_t)state.getReg(inst->uop.src1) >> 56) & 0xFF;
      if (inst->uop.useImm)
        tempBS8  = inst->uop.imm & 0xFF; 
      else
        tempBS8  = ((uint64_t)state.getReg(inst->uop.src2) >> 56) & 0xFF;

      lastS8 = tempAS8 - tempBS8; 

      res = (((int64_t) lastS8) << 56)   | (((int64_t) seventhS8) << 48) |
        (((int64_t) sixthS8) << 40)  | (((int64_t) fifthS8) << 32)   |
        (((int64_t) fourthS8) << 32) | (((int64_t) thirdS8) << 32)   |
        (((int64_t) secondS8) << 32) | bottomS8;

      state.setDst(inst, res);
      break;
    case OP_U64_SUB:
      tempAU64 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        tempBU64  = inst->uop.imm; 
      else
        tempBU64 = state.getReg(inst->uop.src2);
      res = tempAU64 - tempBU64;

      icc = calcICC(uint64_t (tempAU64), uint64_t (tempBU64), uint64_t (res));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U32_SUB:
      tempAU32  = (state.getReg(inst->uop.src1) & 0xFFFFFFFF);
      if (inst->uop.useImm)
        tempBU32  = inst->uop.imm;
      else
        tempBU32  = (state.getReg(inst->uop.src2) & 0xFFFFFFFF);
      bottomU32 = tempAU32 - tempBU32; 

      icc = calcICC(uint64_t (tempAU32), uint64_t (tempBU32), uint64_t (bottomU32));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU32  = ((uint64_t)state.getReg(inst->uop.src1) >> 32);
      if (inst->uop.useImm)
        tempBU32  = inst->uop.imm;
      else
        tempBU32  = ((uint64_t)state.getReg(inst->uop.src2) >> 32);
      topU32 = tempAU32 - tempBU32;

      res = ((((uint64_t)topU32) << 32) | bottomU32);

      state.setDst(inst, res, icc);
      break;
    case OP_U16_SUB:
      tempAU16  = (state.getReg(inst->uop.src1) & 0xFFFF);
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF; 
      else
        tempBU16  = (state.getReg(inst->uop.src2) & 0xFFFF);

      bottomU16 = tempAU16 - tempBU16; 

      icc = calcICC(uint64_t (tempAU16), uint64_t (tempBU16), uint64_t (bottomU16));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU16  = (state.getReg(inst->uop.src1) >> 16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF; 
      else
        tempBU16  = (state.getReg(inst->uop.src2) >> 16) & 0xFFFF;

      secondU16 = tempAU16 - tempBU16; 


      tempAU16  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF; 
      else
        tempBU16  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFFFF;

      thirdU16 = tempAU16 - tempBU16; 

      tempAU16  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16  = inst->uop.imm & 0xFFFF; 
      else
        tempBU16  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFFFF;

      lastU16 = tempAU16 - tempBU16; 

      res = (((uint64_t) lastU16) << 48) | (((uint64_t) thirdU16) << 32) | (((uint64_t) secondU16) << 16) | bottomU16;

      state.setDst(inst, res, icc);
      break;
    case OP_U08_SUB:
      tempAU8  = (state.getReg(inst->uop.src1) & 0xFF);
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = (state.getReg(inst->uop.src2) & 0xFF);

      bottomU8 = tempAU8 - tempBU8; 

      icc = calcICC(uint64_t (tempAU8), uint64_t (tempBU8), uint64_t (bottomU8));
      icc = icc | (state.getICC(LREG_ICC) & 1);

      tempAU8  = (state.getReg(inst->uop.src1) >> 8) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 8) & 0xFF;

      secondU8 = tempAU8 - tempBU8; 

      tempAU8  = (state.getReg(inst->uop.src1) >> 16) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = (((inst->uop.imm) >> 16) & 0xFF); 
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 16) & 0xFF;

      thirdU8 = tempAU8 - tempBU8; 

      tempAU8  = (state.getReg(inst->uop.src1) >> 24) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = (state.getReg(inst->uop.src2) >> 24) & 0xFF;

      fourthU8 = tempAU8 - tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 32) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 32) & 0xFF;

      fifthU8 = tempAU8 - tempBU8; 


      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 40) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 40) & 0xFF;

      sixthU8 = tempAU8 - tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 48) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 48) & 0xFF;

      seventhU8 = tempAU8 - tempBU8; 

      tempAU8  = ((uint64_t)state.getReg(inst->uop.src1) >> 56) & 0xFF;
      if (inst->uop.useImm)
        tempBU8  = inst->uop.imm & 0xFF; 
      else
        tempBU8  = ((uint64_t)state.getReg(inst->uop.src2) >> 56) & 0xFF;

      lastU8 = tempAU8 - tempBU8; 

      res = (((uint64_t) lastU8) << 56)   | (((uint64_t) seventhU8) << 48) |
        (((uint64_t) sixthU8) << 40)  | (((uint64_t) fifthU8) << 32)   |
        (((uint64_t) fourthU8) << 32) | (((uint64_t) thirdU8) << 32)   |
        (((uint64_t) secondU8) << 32) | bottomU8;

      state.setDst(inst, res, icc);
      break;
    case OP_S64_SLL:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
   
      if(temp2 == 0)
        res = temp1;
      else
       res =  temp1 << temp2;

      icc = calcNZ(uint64_t(res));
      if(temp2 != 0)
      carry = (temp1 >> (64 - temp2)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_SLL:
      tempAS32 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = state.getReg(inst->uop.src2) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        bottomS32 = tempAS32;
      else
        bottomS32 = tempAS32 << tempBS32;

      icc = calcNZ(uint64_t(bottomS32));
      if(tempBS32 != 0)
        carry = (tempAS32 >> (32 - tempBS32)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS32 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        topS32 = tempAS32;
      else
        topS32 = tempAS32 << tempBS32;


      res = (((uint64_t)topS32 << 32) | bottomS32);
      state.setDst(inst, res, icc);
      break;
    case OP_S16_SLL:
      tempAS16 = state.getReg(inst->uop.src1) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = state.getReg(inst->uop.src2) & 0xFFFF;

      if(tempBS16 == 0)
        bottomS16 = tempAS16;
      else
        bottomS16 = tempAS16 << tempBS16;

      icc = calcNZ(uint64_t(bottomS16));
      if(tempBS16 != 0)
        carry = (tempAS16 >> (16 - tempBS16)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS16 = (state.getReg(inst->uop.src1)>>16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = (state.getReg(inst->uop.src2)>>16) & 0xFFFF;

      if(tempBS16 == 0)
        secondS16 = tempAS16;
      else
        secondS16 = tempAS16 << tempBS16;

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFF;

      if(tempBS16 == 0)
        thirdS16 = tempAS16;
      else
        thirdS16 = tempAS16 << tempBS16;

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFFFF;

      if(tempBS16 == 0)
        lastS16 = tempAS16;
      else
        lastS16 = tempAS16 << tempBS16;


      res = (((uint64_t)lastS16 << 48)   | ((uint64_t)thirdS16 << 32) |
             ((uint64_t)secondS16 << 16) | bottomS16);
      state.setDst(inst, res, icc);
      break;
    case OP_S08_SLL:
      tempAS8 = state.getReg(inst->uop.src1) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = state.getReg(inst->uop.src2) & 0xFF;

      if(tempBS8 == 0)
        bottomS8 = tempAS8;
      else
        bottomS8 = tempAS8 << tempBS8;

      icc = calcNZ(uint64_t(bottomS8));
      if(tempBS8 != 0)
        carry = (tempAS8 >> (8 - tempBS8)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS8 = (state.getReg(inst->uop.src1)>>8) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>8) & 0xFF;

      if(tempBS8 == 0)
        secondS8 = tempAS8;
      else
        secondS8 = tempAS8 << tempBS8;


      tempAS8 = (state.getReg(inst->uop.src1)>>16) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>16) & 0xFF;

      if(tempBS8 == 0)
        thirdS8 = tempAS8;
      else
        thirdS8 = tempAS8 << tempBS8;

      tempAS8 = (state.getReg(inst->uop.src1)>>24) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>24) & 0xFF;

      if(tempBS8 == 0)
        fourthS8 = tempAS8;
      else
        fourthS8 = tempAS8 << tempBS8;


      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFF;

      if(tempBS8 == 0)
        fifthS8 = tempAS8;
      else
        fifthS8 = tempAS8 << tempBS8;


      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>40) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>40) & 0xFF;

      if(tempBS8 == 0)
        sixthS8 = tempAS8;
      else
        sixthS8 = tempAS8 << tempBS8;


      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFF;

      if(tempBS8 == 0)
        seventhS8 = tempAS8;
      else
        seventhS8 = tempAS8 << tempBS8;


      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>56) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>56) & 0xFFFF;

      if(tempBS8 == 0)
        lastS8 = tempAS8;
      else
        lastS8 = tempAS8 << tempBS8;


      res = (((uint64_t)lastS8 << 56)   | ((uint64_t)seventhS8 << 48) |
             ((uint64_t)sixthS8 << 40)  | ((uint64_t)fifthS8 << 32) |
             ((uint64_t)fourthS8 << 24) | ((uint64_t)thirdS8 << 16) |
             ((uint64_t)secondS8 << 8)  | bottomS8);
      state.setDst(inst, res, icc);
      break;
    case OP_S64_SRL: //FIXME: I believe >> is arithmetic right shift in C++
      tempAU64 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        tempBU64 = inst->uop.imm;
      else 
        tempBU64 = state.getReg(inst->uop.src2);
   
      if(tempAU64 == 0)
        res = tempAU64;
      else
       res =  tempAU64 >> tempBU64;

      icc = calcNZ(uint64_t(res));
      if(tempBU64 != 0)
        carry = (tempAU64 >> (64-tempBU64)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_SRL:
      tempAU32 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBU32 = inst->uop.imm;
      else 
        tempBU32 = state.getReg(inst->uop.src2) & 0xFFFFFFFF;

      if(tempBU32 == 0)
        bottomU32 = tempAU32;
      else
        bottomU32 = tempAU32 >> tempBU32;

      icc = calcNZ(uint64_t(bottomU32));
      if(tempBU32 != 0)
        carry = (tempAU32 >> (32 - tempBU32)) & 1;
      icc = icc | (carry<<1);

      tempAU32 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBU32 = inst->uop.imm;
      else 
        tempBU32 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFFFFFF;

      if(tempBU32 == 0)
        topU32 = tempAU32;
      else
        topU32 = tempAU32 >> tempBU32;

      res = (((uint64_t)topU32 << 32) | bottomU32);
      state.setDst(inst, res, icc);
      break;
    case OP_S16_SRL:
      tempAU16 = state.getReg(inst->uop.src1) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16 = inst->uop.imm & 0xFFFF;
      else 
        tempBU16 = state.getReg(inst->uop.src2) & 0xFFFF;

      if(tempBU16 == 0)
        bottomS16 = tempAU16;
      else
        bottomS16 = tempAU16 >> tempBU16;

      icc = calcNZ(uint64_t(bottomS16));
      if(tempBU16 != 0)
        carry = (tempAU16 >> (16 - tempBU16)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAU16 = (state.getReg(inst->uop.src1)>>16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16 = inst->uop.imm & 0xFFFF;
      else 
        tempBU16 = (state.getReg(inst->uop.src2)>>16) & 0xFFFF;

      if(tempBU16 == 0)
        secondS16 = tempAU16;
      else
        secondS16 = tempAU16 >> tempBU16;

      tempAU16 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16 = inst->uop.imm & 0xFFFF;
      else 
        tempBU16 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFF;

      if(tempBU16 == 0)
        thirdS16 = tempAU16;
      else
        thirdS16 = tempAU16 >> tempBU16;

      tempAU16 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU16 = inst->uop.imm & 0xFFFF;
      else 
        tempBU16 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFFFF;

      if(tempBU16 == 0)
        lastS16 = tempAU16;
      else
        lastS16 = tempAU16 >> tempBU16;

      res = (((uint64_t)lastS16 << 48)   | ((uint64_t)thirdS16 << 32) |
             ((uint64_t)secondS16 << 16) | bottomS16);
      state.setDst(inst, res, icc);
      break;
    case OP_S08_SRL:
      tempAU8 = state.getReg(inst->uop.src1) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = state.getReg(inst->uop.src2) & 0xFF;

      if(tempBU8 == 0)
        bottomS8 = tempAU8;
      else
        bottomS8 = tempAU8 >> tempBU8;

      icc = calcNZ(uint64_t(bottomS8));
      if(tempBU8 != 0)
        carry = (tempAU8 >> (8 - tempBU8)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAU8 = (state.getReg(inst->uop.src1)>>8) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = (state.getReg(inst->uop.src2)>>8) & 0xFF;

      if(tempBU8 == 0)
        secondS8 = tempAU8;
      else
        secondS8 = tempAU8 >> tempBU8;

      tempAU8 = (state.getReg(inst->uop.src1)>>16) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = (state.getReg(inst->uop.src2)>>16) & 0xFF;

      if(tempBU8 == 0)
        thirdS8 = tempAU8;
      else
        thirdS8 = tempAU8 >> tempBU8;

      tempAU8 = (state.getReg(inst->uop.src1)>>24) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = (state.getReg(inst->uop.src2)>>24) & 0xFF;

      if(tempBU8 == 0)
        fourthS8 = tempAU8;
      else
        fourthS8 = tempAU8 >> tempBU8;

      tempAU8 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFF;

      if(tempBU8 == 0)
        fifthS8 = tempAU8;
      else
        fifthS8 = tempAU8 >> tempBU8;

      tempAU8 = ((uint64_t)state.getReg(inst->uop.src1)>>40) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = ((uint64_t)state.getReg(inst->uop.src2)>>40) & 0xFF;

      if(tempBU8 == 0)
        sixthS8 = tempAU8;
      else
        sixthS8 = tempAU8 >> tempBU8;

      tempAU8 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFF;

      if(tempBU8 == 0)
        seventhS8 = tempAU8;
      else
        seventhS8 = tempAU8 >> tempBU8;

      tempAU8 = ((uint64_t)state.getReg(inst->uop.src1)>>56) & 0xFFFF;
      if (inst->uop.useImm)
        tempBU8 = inst->uop.imm & 0xFF;
      else 
        tempBU8 = ((uint64_t)state.getReg(inst->uop.src2)>>56) & 0xFFFF;

      if(tempBU8 == 0)
        lastS8 = tempAU8;
      else
        lastS8 = tempAU8 >> tempBU8;


      res = (((uint64_t)lastS8 << 56)   | ((uint64_t)seventhS8 << 48) |
             ((uint64_t)sixthS8 << 40)  | ((uint64_t)fifthS8 << 32) |
             ((uint64_t)fourthS8 << 24) | ((uint64_t)thirdS8 << 16) |
             ((uint64_t)secondS8 << 8)  | bottomS8);
      state.setDst(inst, res, icc);
      break;
    case OP_S64_SRA:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
   
      if(temp2 == 0)
        res = temp1;
      else
       res =  temp1 >> temp2;

      icc = calcNZ(uint64_t(res));
      if(temp2 != 0)
        carry = (temp1 >> (64-temp2)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_SRA:
      tempAS32 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = state.getReg(inst->uop.src2) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        bottomS32 = tempAS32;
      else
        bottomS32 = tempAS32 >> tempBS32;

      icc = calcNZ(uint64_t(bottomS32));
      if(tempBS32 != 0)
        carry = (tempAS32 >> (tempBS32-1)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS32 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        topS32 = tempAS32;
      else
        topS32 = tempAS32 >> tempBS32;

      res = (((uint64_t)topS32 << 32) | bottomS32);
      state.setDst(inst, res, icc);
      break;
    case OP_S16_SRA:
      tempAS16 = state.getReg(inst->uop.src1) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = state.getReg(inst->uop.src2) & 0xFFFF;

      if(tempBS16 == 0)
        bottomS16 = tempAS16;
      else
        bottomS16 = tempAS16 >> tempBS16;

      icc = calcNZ(uint64_t(bottomS16));
      if(tempBS16 != 0)
        carry = (tempAS16 >> (16 - tempBS16)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS16 = (state.getReg(inst->uop.src1)>>16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = (state.getReg(inst->uop.src2)>>16) & 0xFFFF;

      if(tempBS16 == 0)
        secondS16 = tempAS16;
      else
        secondS16 = tempAS16 >> tempBS16;

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFF;

      if(tempBS16 == 0)
        thirdS16 = tempAS16;
      else
        thirdS16 = tempAS16 >> tempBS16;

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFFFF;

      if(tempBS16 == 0)
        lastS16 = tempAS16;
      else
        lastS16 = tempAS16 >> tempBS16;

      res = (((uint64_t)lastS16 << 48)   | ((uint64_t)thirdS16 << 32) |
             ((uint64_t)secondS16 << 16) | bottomS16);
      state.setDst(inst, res, icc);
      break;
    case OP_S08_SRA:
      tempAS8 = state.getReg(inst->uop.src1) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = state.getReg(inst->uop.src2) & 0xFF;

      if(tempBS8 == 0)
        bottomS8 = tempAS8;
      else
        bottomS8 = tempAS8 >> tempBS8;

      icc = calcNZ(uint64_t(bottomS8));
      if(tempBS8 != 0)
        carry = (tempAS8 >> (8 - tempBS8)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS8 = (state.getReg(inst->uop.src1)>>8) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>8) & 0xFF;

      if(tempBS8 == 0)
        secondS8 = tempAS8;
      else
        secondS8 = tempAS8 >> tempBS8;

      tempAS8 = (state.getReg(inst->uop.src1)>>16) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>16) & 0xFF;

      if(tempBS8 == 0)
        thirdS8 = tempAS8;
      else
        thirdS8 = tempAS8 >> tempBS8;


      tempAS8 = (state.getReg(inst->uop.src1)>>24) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>24) & 0xFF;

      if(tempBS8 == 0)
        fourthS8 = tempAS8;
      else
        fourthS8 = tempAS8 >> tempBS8;

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFF;

      if(tempBS8 == 0)
        fifthS8 = tempAS8;
      else
        fifthS8 = tempAS8 >> tempBS8;

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>40) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>40) & 0xFF;

      if(tempBS8 == 0)
        sixthS8 = tempAS8;
      else
        sixthS8 = tempAS8 >> tempBS8;

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFF;

      if(tempBS8 == 0)
        seventhS8 = tempAS8;
      else
        seventhS8 = tempAS8 >> tempBS8;

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>56) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>56) & 0xFFFF;

      if(tempBS8 == 0)
        lastS8 = tempAS8;
      else
        lastS8 = tempAS8 >> tempBS8;

      res = (((uint64_t)lastS8 << 56)   | ((uint64_t)seventhS8 << 48) |
             ((uint64_t)sixthS8 << 40)  | ((uint64_t)fifthS8 << 32) |
             ((uint64_t)fourthS8 << 24) | ((uint64_t)thirdS8 << 16) |
             ((uint64_t)secondS8 << 8)  | bottomS8);
      state.setDst(inst, res, icc);
      break;
    case OP_S64_ROTR:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
   
      if(temp2 == 0)
        res = temp1;
      else
       res =  (temp1 >> temp2) | (temp1 << (64-temp2));

      icc = calcNZ(uint64_t(res));
      if(temp2 != 0)
        carry = (temp1 >> 63) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_ROTR:
      tempAS32 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = state.getReg(inst->uop.src2) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        bottomS32 = tempAS32;
      else
        bottomS32 = (tempAS32 >> tempBS32) | (tempAS32 << (32-tempBS32));

      icc = calcNZ(uint64_t(bottomS32));
      if(tempBS32 != 0)
        carry = (tempAS32 >> 31) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS32 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFFFFFF;
      if (inst->uop.useImm)
        tempBS32 = inst->uop.imm;
      else 
        tempBS32 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFFFFFF;

      if(tempBS32 == 0)
        topS32 = tempAS32;
      else
        topS32 = (tempAS32 >> tempBS32) | (tempAS32 << (32-tempBS32));

      res = (((uint64_t)topS32 << 32) | bottomS32);
      state.setDst(inst, res, icc);
      break;
    case OP_S16_ROTR:
      tempAS16 = state.getReg(inst->uop.src1) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = state.getReg(inst->uop.src2) & 0xFFFF;

      if(tempBS16 == 0)
        bottomS16 = tempAS16;
      else
        bottomS16 = (tempAS16 >> tempBS16) | (tempAS16 << (16 - tempBS16));

      icc = calcNZ(uint64_t(bottomS16));
      if(tempBS16 != 0)
        carry = (tempAS16 >> (16 - tempBS16)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS16 = (state.getReg(inst->uop.src1)>>16) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = (state.getReg(inst->uop.src2)>>16) & 0xFFFF;

      if(tempBS16 == 0)
        secondS16 = tempAS16;
      else
        secondS16 = (tempAS16 >> tempBS16) | (tempAS16 << (16 - tempBS16));

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFFFF;

      if(tempBS16 == 0)
        thirdS16 = tempAS16;
      else
        thirdS16 = (tempAS16 >> tempBS16) | (tempAS16 << (16 - tempBS16));

      tempAS16 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS16 = inst->uop.imm & 0xFFFF;
      else 
        tempBS16 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFFFF;

      if(tempBS16 == 0)
        lastS16 = tempAS16;
      else
        lastS16 = (tempAS16 >> tempBS16) | (tempAS16 << (16 - tempBS16));

      res = (((uint64_t)lastS16 << 48)   | ((uint64_t)thirdS16 << 32) |
             ((uint64_t)secondS16 << 16) | bottomS16);
      state.setDst(inst, res, icc);
      break;
    case OP_S08_ROTR:
      tempAS8 = state.getReg(inst->uop.src1) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = state.getReg(inst->uop.src2) & 0xFF;

      if(tempBS8 == 0)
        bottomS8 = tempAS8;
      else
        bottomS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      icc = calcNZ(uint64_t(bottomS8));
      if(tempBS8 != 0)
        carry = (tempAS8 >> (8 - tempBS8)) & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS8 = (state.getReg(inst->uop.src1)>>8) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>8) & 0xFF;

      if(tempBS8 == 0)
        secondS8 = tempAS8;
      else
        secondS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = (state.getReg(inst->uop.src1)>>16) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>16) & 0xFF;

      if(tempBS8 == 0)
        thirdS8 = tempAS8;
      else
        thirdS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = (state.getReg(inst->uop.src1)>>24) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = (state.getReg(inst->uop.src2)>>24) & 0xFF;

      if(tempBS8 == 0)
        fourthS8 = tempAS8;
      else
        fourthS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>32) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>32) & 0xFF;

      if(tempBS8 == 0)
        fifthS8 = tempAS8;
      else
        fifthS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>40) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
      tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>40) & 0xFF;

      if(tempBS8 == 0)
        sixthS8 = tempAS8;
      else
        sixthS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>48) & 0xFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>48) & 0xFF;

      if(tempBS8 == 0)
        seventhS8 = tempAS8;
      else
        seventhS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      tempAS8 = ((uint64_t)state.getReg(inst->uop.src1)>>56) & 0xFFFF;
      if (inst->uop.useImm)
        tempBS8 = inst->uop.imm & 0xFF;
      else 
        tempBS8 = ((uint64_t)state.getReg(inst->uop.src2)>>56) & 0xFFFF;

      if(tempBS8 == 0)
        lastS8 = tempAS8;
      else
        lastS8 = (tempAS8 >> tempBS8) | (tempAS8 << (8 - tempBS8));

      res = (((uint64_t)lastS8 << 56)   | ((uint64_t)seventhS8 << 48) |
             ((uint64_t)sixthS8 << 40)  | ((uint64_t)fifthS8 << 32) |
             ((uint64_t)fourthS8 << 24) | ((uint64_t)thirdS8 << 16) |
             ((uint64_t)secondS8 << 8)  | bottomS8);
      state.setDst(inst, res, icc);
      break;
    case OP_S64_ROTRXX:
      temp1 = state.getReg(inst->uop.src1);
      carry_bit = (state.getICC(inst->uop.src1) >> 2) & 1;
   
      res =  (temp1 >> 1) | ((uint64_t)carry_bit << 63);

      icc = calcNZ(uint64_t(res));
      carry = temp1 & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_ROTRXX:
      tempAS32 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      carry_bit = (state.getICC(inst->uop.src1) >> 2) & 1;

      bottomS32 = (tempAS32 >> 1) | (carry_bit << 31);

      icc = calcNZ(uint64_t(bottomS32));
      carry = tempAS32 & 1;
      icc = icc | (carry<<1) | (state.getICC(LREG_ICC) & 1);

      tempAS32 = (state.getReg(inst->uop.src1)>>32) & 0xFFFFFFFF;
      carry_bit = (state.getICC(inst->uop.src1) >> 5) & 1;

      topS32 = (tempAS32 >> 1) | (carry_bit << 31);

      res = (((uint64_t)topS32 << 32) | bottomS32);
      state.setDst(inst, res, icc);
      break;
    case OP_S64_ANDN:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 & (~temp2);
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_AND:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 & temp2;
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_OR:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 | temp2;
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_XOR:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 ^ temp2;
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_ORN:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 | (~temp2);
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_XNOR:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2 = inst->uop.imm;
      else 
        temp2 = state.getReg(inst->uop.src2);
      res = ~(temp1 ^ temp2);
 
      icc = calcNZ(uint64_t(res));
      icc = icc | (state.getICC(LREG_ICC) & 3);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_SADD:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
      carry = 0;

      res = temp1 + temp2;

      if(((int64_t)temp1 >= 0) && ((int64_t)temp2 >= 0) && ((int64_t)res < 0)) {
        res = INT64_MAX;
        carry = 1;
      }
      else if(((int64_t)temp1 < 0) && ((int64_t)temp2 < 0) && ((int64_t)res >= 0)) {
        res = INT64_MIN;
        carry = 1;
      }

      icc = calcNZ(res);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S32_SADD:
      carry = 0;
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAS32 = temp1 & 0xFFFFFFFF;
      tempBS32 = temp2 & 0xFFFFFFFF;

      bottomS32 = tempAS32 + tempBS32;

      if(((int32_t)tempAS32 >= 0) && ((int32_t)tempBS32 >= 0) && ((int32_t)bottomS32 < 0)) {
        bottomS32 = INT32_MAX;
        carry = 1;
      }
      else if(((int32_t)tempAS32 < 0) && ((int32_t)tempBS32 < 0) && ((int32_t)bottomS32 >= 0)) {
        bottomS32 = INT32_MIN;
        carry = 1;
      }

      icc = calcNZ32(bottomS32);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);
      
      tempAS32 = (temp1>>32) & 0xFFFFFFFF;

      if(!inst->uop.useImm)
        tempBS32 = (temp2>>32) & 0xFFFFFFFF;
      else
        tempBS32 = temp2 & 0xFFFFFFFF;

      topS32 = tempAS32 + tempBS32;

      if(((int32_t)tempAS32 >= 0) && ((int32_t)tempBS32 >= 0) && ((int32_t)topS32 < 0))
        topS32 = INT32_MAX;
      else if(((int32_t)tempAS32 < 0) && ((int32_t)tempBS32 < 0) && ((int32_t)topS32 >= 0))
        topS32 = INT32_MIN;
      
      res = ((uint64_t)(topS32)<<32) | bottomS32; 

      state.setDst(inst, res, icc);
      break;
    case OP_S16_SADD:
      carry = 0;
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAS16 = (temp1>>48) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>48) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;

      lastS16 = tempAS16 + tempBS16;

      if(((int16_t)tempAS16 >= 0) && ((int16_t)tempBS16 >= 0) && ((int16_t)lastS16 < 0))
        lastS16 = INT16_MAX;
      else if(((int16_t)tempAS16 <0) && ((int16_t)tempBS16 < 0) && ((int16_t)lastS16 >= 0))
        lastS16 = INT16_MIN;
      
      tempAS16 = (temp1>>32) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>32) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;

      thirdS16 = tempAS16 + tempBS16;

      if(((int16_t)tempAS16 >= 0) && ((int16_t)tempBS16 >= 0) && ((int16_t)thirdS16 < 0))
        thirdS16 = INT16_MAX;
      else if(((int16_t)tempAS16 <0) && ((int16_t)tempBS16 < 0) && ((int16_t)thirdS16 >= 0))
        thirdS16 = INT16_MIN;
      
      tempAS16 = (temp1>>16) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>16) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;

      secondS16 = tempAS16 + tempBS16;

      if(((int16_t)tempAS16 >= 0) && ((int16_t)tempBS16 >= 0) && ((int16_t)secondS16 < 0))
        secondS16 = INT16_MAX;
      else if(((int16_t)tempAS16 <0) && ((int16_t)tempBS16 < 0) && ((int16_t)secondS16 >= 0))
        secondS16 = INT16_MIN;
      
      tempAS16 = temp1 & 0xFFFF;
      tempBS16 = temp2 & 0xFFFF;

      bottomS16 = tempAS16 + tempBS16;

      if(((int16_t)tempAS16 >= 0) && ((int16_t)tempBS16 >= 0) && ((int16_t)bottomS16 < 0)) {
        bottomS16 = INT16_MAX;
        carry = 1;
      }
      else if(((int16_t)tempAS16 <0) && ((int16_t)tempBS16 < 0) && ((int16_t)bottomS16 >= 0)) {
        bottomS16 = INT16_MIN;
        carry = 1;
      }
      
      icc = calcNZ16(bottomS16);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      res = ((uint64_t)(lastS16)<<48) | ((uint64_t)(thirdS16)<<32) |
            ((uint64_t)(secondS16)<<16) | bottomS16; 

      state.setDst(inst, res, icc);
      break;
    case OP_S08_SADD:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
      carry = 0;

      tempAS8 = (temp1>>56) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>56) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      lastS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)lastS8 < 0))
        lastS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)lastS8 >= 0))
        lastS8 = INT8_MIN;
      
      tempAS8 = (temp1>>48) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>48) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      seventhS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)seventhS8 < 0))
        seventhS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)seventhS8 >= 0))
        seventhS8 = INT8_MIN;
      
      tempAS8 = (temp1>>40) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>40) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      sixthS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)sixthS8 < 0))
        sixthS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)sixthS8 >= 0))
        sixthS8 = INT8_MIN;
      
      tempAS8 = (temp1>>32) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>32) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      fifthS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)fifthS8 < 0))
        fifthS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)fifthS8 >= 0))
        fifthS8 = INT8_MIN;
      
      tempAS8 = (temp1>>24) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>24) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      fourthS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)fourthS8 < 0))
        fourthS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)fourthS8 >= 0))
        fourthS8 = INT8_MIN;
      
      tempAS8 = (temp1>>16) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>16) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      thirdS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)thirdS8 < 0))
        thirdS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)thirdS8 >= 0))
        thirdS8 = INT8_MIN;
      
      tempAS8 = (temp1>>8) & 0xFF;
      tempBS8 = temp2 & 0xFF;

      secondS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)secondS8 < 0))
        secondS8 = INT8_MAX;
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)secondS8 >= 0))
        secondS8 = INT8_MIN;
      
      tempAS8 = temp1 & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = temp2 & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomS8 = tempAS8 + tempBS8;

      if(((int8_t)tempAS8 >= 0) && ((int8_t)tempBS8 >= 0) && ((int8_t)bottomS8 < 0)) {
        bottomS8 = INT8_MAX;
        carry = 1;
      }
      else if(((int8_t)tempAS8 <0) && ((int8_t)tempBS8 < 0) && ((int8_t)bottomS8 >= 0)) {
        bottomS8 = INT8_MIN;
        carry = 1;
      }
      
      icc = calcNZ8(bottomS8);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      res = ((uint64_t)(lastS8)<<56)   | ((uint64_t)(seventhS8)<<48) | 
            ((uint64_t)(sixthS8)<<40)  | ((uint64_t)(fifthS8)<<32) |
            ((uint64_t)(fourthS8)<<24) | ((uint64_t)(thirdS8)<<16) |
            ((uint64_t)(secondS8)<<8)  | bottomS8; 

      state.setDst(inst, res, icc);
      break;
    case OP_U64_SADD:
      tempAU64 = state.getReg(inst->uop.src1);
      tempBU64 = state.getReg(inst->uop.src2);

      res = UNSIGNEDMAX64 - tempBU64;

      if(tempAU64 > res) {
        res = UNSIGNEDMAX64;
        carry = 1;
      }
      else {
        res = tempAU64 + tempBU64;
        carry = 0;
      }

      icc = calcNZ(res);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U32_SADD:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU32 = (temp1>>32) & 0xFFFFFFFF;

      if(!inst->uop.useImm)
        tempBU32 = (temp2>>32) & 0xFFFFFFFF;
      else
        tempBU32 = temp2 & 0xFFFFFFFF;

      topU32 = UNSIGNEDMAX32 - tempBU32;

      if(tempAU32 > topU32)
        topU32 = UNSIGNEDMAX32;
      else
        topU32 = tempAU32 + tempBU32;
      
      tempAU32 = temp1 & 0xFFFFFFFF;
      tempBU32 = temp2 & 0xFFFFFFFF;

      bottomU32 = UNSIGNEDMAX32 - tempBU32;

      if(tempAU32 > bottomU32) {
        bottomU32 = UNSIGNEDMAX32;
        carry = 1;
      }
      else { 
        bottomU32 = tempAU32 + tempBU32;
        carry = 0;
      }
      
      res = ((uint64_t)(topU32)<<32) | bottomU32; 

      icc = calcNZ32(bottomU32);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U16_SADD:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU16 = (temp1>>48) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>48) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      lastU16 = UNSIGNEDMAX16 - tempBU16;
      if(tempAU16 > UNSIGNEDMAX16)
        lastU16 = UNSIGNEDMAX16;
      else
        lastU16 = tempAU16 + tempBU16;
      
      tempAU16 = (temp1>>32) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>32) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      thirdU16 = UNSIGNEDMAX16 - tempBU16;
      if(tempAU16 > thirdU16)
        thirdU16 = UNSIGNEDMAX16;
      else
        thirdU16 = tempAU16 + tempBU16;
      
      tempAU16 = (temp1>>16) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>16) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      secondU16 = UNSIGNEDMAX16 - tempBU16;
      if(tempAU16 > secondU16)
        secondU16 = UNSIGNEDMAX16;
      else
        secondU16 = tempAU16 + tempBU16;

      tempAU16 = temp1 & 0xFFFF;
      tempBU16 = temp2 & 0xFFFF;

      bottomU16 = UNSIGNEDMAX16 - tempBU16;

      if(tempAU16 > bottomU16) { 
        bottomU16 = UNSIGNEDMAX16;
        carry = 1;
      }
      else {
        bottomU16 = tempAU16 + tempBU16;
        carry = 0;
      }
      
      res = ((uint64_t)(lastU16)<<48) | ((uint64_t)(thirdU16)<<32) |
            ((uint64_t)(secondU16)<<16) | bottomU16; 

      icc = calcNZ16(bottomU16);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U08_SADD:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU8 = (temp1>>56) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>56) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      lastU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > lastU8)
        lastU8 = UNSIGNEDMAX8;
      else
        lastU8 = tempAU8 + tempBU8;

      tempAU8 = (temp1>>48) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>48) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      seventhU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > seventhU8)
        seventhU8 = UNSIGNEDMAX8;
      else
        seventhU8 = tempAU8 + tempBU8;

      tempAU8 = (temp1>>40) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>40) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      sixthU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > sixthU8)
        sixthU8 = UNSIGNEDMAX8;
      else
        sixthU8 = tempAU8 + tempBU8;
      
      tempAU8 = (temp1>>32) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>32) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      fifthU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > fifthU8)
        fifthU8 = UNSIGNEDMAX8;
      else
        fifthU8 = tempAU8 + tempBU8;
      
      tempAU8 = (temp1>>24) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>24) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      fourthU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > fourthU8)
        fourthU8 = UNSIGNEDMAX8;
      else
        fourthU8 = tempAU8 + tempBU8;
      
      tempAU8 = (temp1>>16) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>16) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      thirdU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > thirdU8)
        thirdU8 = UNSIGNEDMAX8;
      else
        thirdU8 = tempAU8 + tempBU8;
      
      tempAU8 = (temp1>>8) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>8) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      secondU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > secondU8)
        secondU8 = UNSIGNEDMAX8;
      else
        secondU8 = tempAU8 + tempBU8;

      tempAU8 = temp1 & 0xFF;
      tempBU8 = temp2 & 0xFF;

      bottomU8 = UNSIGNEDMAX8 - tempBU8;
      if(tempAU8 > bottomU8) {
        bottomU8 = UNSIGNEDMAX8;
        carry = 1;
      }
      else {
        bottomU8 = tempAU8 + tempBU8;
        carry = 0;
      }
      res = ((uint64_t)(lastU8)<<56)   | ((uint64_t)(seventhU8)<<48) | 
            ((uint64_t)(sixthU8)<<40)  | ((uint64_t)(fifthU8)<<32) |
            ((uint64_t)(fourthU8)<<24) | ((uint64_t)(thirdU8)<<16) |
            ((uint64_t)(secondU8)<<8)  | bottomU8; 

      icc = calcNZ8(bottomU8);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S64_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = INT64_MIN + temp2;

      if(temp1 > (int64_t)res) { 
        res = INT64_MAX;
        carry = 1;
      }
      else {
        res = temp1 - temp2;
        carry = 0;
      }

      icc = calcNZ(res);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);
      state.setDst(inst, res, icc);
      break;
    case OP_S32_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAS32 = (temp1>>32) & 0xFFFFFFFF;

      if(!inst->uop.useImm)
        tempBS32 = (temp2>>32) & 0xFFFFFFFF;
      else
        tempBS32 = temp2 & 0xFFFFFFFF;

      topS32 = INT32_MIN + tempBS32;

      if(tempAS32 > topS32)
        topS32 = INT32_MAX;
      else
        topS32 = tempAS32 - tempBS32;
      
      tempAS32 = temp1 & 0xFFFFFFFF;
      tempBS32 = temp2 & 0xFFFFFFFF;

      bottomS32 = INT32_MIN + tempBS32;

      if(tempAS32 > bottomS32) {
        bottomS32 = INT32_MAX;
        carry = 1;
      }
      else {
        bottomS32 = tempAS32 - tempBS32;
        carry = 0;
      }
      
      res = ((uint64_t)(topS32)<<32) | bottomS32; 

      icc = calcNZ32(bottomS32);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S16_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAS16 = (temp1>>48) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>48) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;

      bottomU32 = (uint32_t)(int16_t)tempAS16 - (uint32_t)(int16_t)tempBS16;

      if(bottomU32 != (int16_t)bottomU32) {
        if((int16_t)tempBS16 < 0)
          bottomU32 = (1 << (sizeof(int16_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int16_t) * 8 - 1);
      }

      lastS16 = bottomU32;

      tempAS16 = (temp1>>32) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>32) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;
      
      bottomU32 = (uint32_t)(int16_t)tempAS16 - (uint32_t)(int16_t)tempBS16;

      if(bottomU32 != (int16_t)bottomU32) {
        if((int16_t)tempBS16 < 0)
          bottomU32 = (1 << (sizeof(int16_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int16_t) * 8 - 1);
      }

      thirdS16 = bottomU32;

      tempAS16 = (temp1>>16) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBS16 = (temp2>>16) & 0xFFFF;
      else
        tempBS16 = temp2 & 0xFFFF;
      
      bottomU32 = (uint32_t)(int16_t)tempAS16 - (uint32_t)(int16_t)tempBS16;

      if(bottomU32 != (int16_t)bottomU32) {
        if((int16_t)tempBS16 < 0)
          bottomU32 = (1 << (sizeof(int16_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int16_t) * 8 - 1);
      }

      secondS16 = bottomU32;

      tempAS16 = temp1 & 0xFFFF;
      tempBS16 = temp2 & 0xFFFF;

      bottomU32 = (uint32_t)(int16_t)tempAS16 - (uint32_t)(int16_t)tempBS16;

      if(bottomU32 != (int16_t)bottomU32) {
        carry = 1;
        if((int16_t)tempBS16 < 0)
          bottomU32 = (1 << (sizeof(int16_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int16_t) * 8 - 1);
      }
      else
        carry = 0;

      bottomS16 = bottomU32;

      res = ((uint64_t)(lastS16)<<48) | ((uint64_t)(thirdS16)<<32) |
            ((uint64_t)(secondS16)<<16) | bottomS16; 

      icc = calcNZ16(bottomS16);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_S08_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAS8 = (temp1>>56) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>56) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      lastS8 = bottomU32;
      
      tempAS8 = (temp1>>48) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>48) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      seventhS8 = bottomU32;
      
      tempAS8 = (temp1>>40) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>40) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      sixthS8 = bottomU32;
      
      tempAS8 = (temp1>>32) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>32) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      fifthS8 = bottomU32;
      
      tempAS8 = (temp1>>24) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>24) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      fourthS8 = bottomU32;
      
      tempAS8 = (temp1>>16) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>16) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      thirdS8 = bottomU32;
      
      tempAS8 = (temp1>>8) & 0xFF;

      if(!inst->uop.useImm)
        tempBS8 = (temp2>>8) & 0xFF;
      else
        tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }

      secondS8 = bottomU32;
      
      tempAS8 = temp1 & 0xFF;
      tempBS8 = temp2 & 0xFF;

      bottomU32 = (uint32_t)(int8_t)tempAS8 - (uint32_t)(int8_t)tempBS8;

      if(bottomU32 != (int8_t)bottomU32) {
        carry = 1;
        if((int8_t)tempBS8 < 0)
          bottomU32 = (1 << (sizeof(int8_t) * 8 - 1)) - 1;
        else
          bottomU32 = 1 << (sizeof(int8_t) * 8 - 1);
      }
      else
        carry = 0;

      bottomS8 = bottomU32;
      
      res = ((uint64_t)(lastS8)<<56) | ((uint64_t)(seventhS8)<<48) | 
            ((uint64_t)(sixthS8)<<40) | ((uint64_t)(fifthS8)<<32) |
            ((uint64_t)(fourthS8)<<24) | ((uint64_t)(thirdS8)<<16) |
            ((uint64_t)(secondS8)<<8)  | bottomS8; 

      icc = calcNZ8(bottomS8);
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U64_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      if(temp1 < temp2) { 
        res = 0;
        carry = 1;
      }
      else {
        res = temp1 - temp2;
        carry = 0;
      }

      icc = calcNZ(res);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U32_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU32 = (temp1>>32) & 0xFFFFFFFF;

      if(!inst->uop.useImm)
        tempBU32 = (temp2>>32) & 0xFFFFFFFF;
      else
        tempBU32 = temp2 & 0xFFFFFFFF;

      if(tempAU32 < tempBU32)
        topU32 = 0;
      else
        topU32 = tempAU32 - tempBU32;
      
      tempAU32 = temp1 & 0xFFFFFFFF;
      tempBU32 = temp2 & 0xFFFFFFFF;

      if(tempAU32 < tempBU32) {
        bottomU32 = 0;
        carry = 1;
      }
      else {
        bottomU32 = tempAU32 - tempBU32;
        carry = 0;
      }
      
      res = ((uint64_t)(topU32)<<32) | bottomU32; 

      icc = calcNZ32(bottomU32);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U16_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU16 = (temp1>>48) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>48) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      if(tempAU16 < tempBU16)
        lastU16 = 0;
      else
        lastU16 = tempAU16 - tempBU16;

      tempAU16 = (temp1>>32) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>32) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      if(tempAU16 < tempBU16)
        thirdU16 = 0;
      else
        thirdU16 = tempAU16 - tempBU16;

      tempAU16 = (temp1>>16) & 0xFFFF;

      if(!inst->uop.useImm)
        tempBU16 = (temp2>>16) & 0xFFFF;
      else
        tempBU16 = temp2 & 0xFFFF;

      if(tempAU16 < tempBU16)
        secondU16 = 0;
      else
        secondU16 = tempAU16 - tempBU16;

      tempAU16 = temp1 & 0xFFFF;
      tempBU16 = temp2 & 0xFFFF;

      if(tempAU16 < tempBU16) {
        bottomU16 = 0;
        carry = 1;
      }
      else {
        bottomU16 = tempAU16 - tempBU16;
        carry = 0;
      }
      
      res = ((uint64_t)(lastU16)<<48) | ((uint64_t)(thirdU16)<<32) |
            ((uint64_t)(secondU16)<<16) | bottomU16; 

      icc = calcNZ16(bottomU16);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
    case OP_U08_SSUB:
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      tempAU8 = (temp1>>56) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>56) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        lastU8 = 0;
      else
        lastU8 = tempAU8 - tempBU8;

      tempAU8 = (temp1>>48) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>48) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        seventhU8 = 0;
      else
        seventhU8 = tempAU8 - tempBU8;

      tempAU8 = (temp1>>40) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>40) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        sixthU8 = 0;
      else
        sixthU8 = tempAU8 - tempBU8;

      tempAU8 = (temp1>>32) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>32) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        fifthU8 = 0;
      else
        fifthU8 = tempAU8 - tempBU8;

      tempAU8 = (temp1>>24) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>24) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        fourthU8 = 0;
      else
        fourthU8 = tempAU8 - tempBU8;

      tempAU8 = (temp1>>16) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>16) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        thirdU8 = 0;
      else
        thirdU8 = tempAU8 - tempBU8;
      
      tempAU8 = (temp1>>8) & 0xFF;

      if(!inst->uop.useImm)
        tempBU8 = (temp2>>8) & 0xFF;
      else
        tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8)
        secondU8 = 0;
      else
        secondU8 = tempAU8 - tempBU8;

      tempAU8 = temp1 & 0xFF;
      tempBU8 = temp2 & 0xFF;

      if(tempAU8 < tempBU8) {
        bottomU8 = 0;
        carry = 1;
      }
      else {
        bottomU8 = tempAU8 - tempBU8;
        carry = 0;
      }
      
      res = ((uint64_t)(lastU8)<<56) | ((uint64_t)(seventhU8)<<48) | 
            ((uint64_t)(sixthU8)<<40) | ((uint64_t)(fifthU8)<<32) |
            ((uint64_t)(fourthU8)<<24) | ((uint64_t)(thirdU8)<<16) |
            ((uint64_t)(secondU8)<<8)  | bottomU8; 

      icc = calcNZ8(bottomU8);
      icc = icc & 0x7;
      icc = icc | (carry << 1) | (state.getICC(LREG_ICC) & 1);

      state.setDst(inst, res, icc);
      break;
#if 0
//NOT IMPLEMENTED FOR NOW
    case OP_S64_COPY:
     break;
    case OP_S64_FCC2ICC:
      break;
    case OP_S64_JOINPSR:
      break;
#endif
    case OP_U64_JMP_REG:
      state.crackInt->setPC(state.getReg(inst->uop.src1)); 
      flush = true;
      break;
    case OP_U64_JMP_IMM:
      state.crackInt->setPC(inst->uop.imm); 
      flush = true;
      break;
    case OP_U64_RBE:
      icc = state.getICC(inst->uop.src1);
      if((icc>>2) & 1){ //Z==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBLE:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1))){ //(Z OR (N XOR V))==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBL:
      icc = state.getICC(inst->uop.src1);
      if((((icc>>3) & 1) ^ (icc & 1))){ //(N XOR V)==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBCS:
      icc = state.getICC(inst->uop.src1);
      if((icc>>1) & 1) {//C==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBNEG:
      icc = state.getICC(inst->uop.src1);
      if((icc>>3) & 1){ //N==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBVS:
      icc = state.getICC(inst->uop.src1);
      if(icc & 1){ //V==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
      //case OP_U64_BA:
      //  break;
    case OP_U64_RBNE:
      icc = state.getICC(inst->uop.src1);
      if(!((icc>>2) & 1)){ //Z==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBG:
      icc = state.getICC(inst->uop.src1);
      if(!(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1)))) {//(Z OR (N XOR V))==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBGE:
      icc = state.getICC(inst->uop.src1);
      if(!(((icc>>3) & 1) ^ (icc & 1))) {//(N XOR V)==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBC_AND_NOTZ: //ARM HI
      icc = state.getICC(inst->uop.src1);
      if(((icc>>1) & 1) && !((icc>>2) & 1)) { //C==1 && Z==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBNOTC_OR_Z: //ARM LO
      icc = state.getICC(inst->uop.src1);
      if(!((icc>>1) & 1) || ((icc>>2) & 1)){ //C==0 || Z==1
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBC_OR_Z_NOT: //SPARC BGU
      icc = state.getICC(inst->uop.src1);
      if(!(((icc>>1) & 1) || ((icc>>2) & 1))) { //!(C || Z)
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBC_OR_Z: //SPARC BLEU
      icc = state.getICC(inst->uop.src1);
      if(((icc>>1) & 1) || ((icc>>2) & 1)) { //(C || Z)
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBCC:
      icc = state.getICC(inst->uop.src1);
      if(!((icc>>1) & 1)) {//C==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBPOS:
      icc = state.getICC(inst->uop.src1);
      if(!((icc>>3) & 1)) {//N==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_RBVC:
      icc = state.getICC(inst->uop.src1);
      if(!(icc & 1)) {//V==0
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_LBE:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if((icc>>2) & 1){ //Z==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBLE:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1))){ //(Z OR (N XOR V))==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBL:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if((((icc>>3) & 1) ^ (icc & 1))){ //(N XOR V)==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBCS:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if((icc>>1) & 1) {//C==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBNEG:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if((icc>>3) & 1){ //N==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBVS:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(icc & 1){ //V==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
      //case OP_U64_BA:
      //  break;
    case OP_U64_LBNE:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!((icc>>2) & 1)){ //Z==0
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBG:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1)))) {//(Z OR (N XOR V))==0
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBGE:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!(((icc>>3) & 1) ^ (icc & 1))) {//(N XOR V)==0
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBC_AND_NOTZ: //ARM HI
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(((icc>>1) & 1) && !((icc>>2) & 1)){ //C==1 && Z==0
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBNOTC_OR_Z: //ARM LO
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!((icc>>1) & 1) || ((icc>>2) & 1)){ //C==0 || Z==1
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBC_OR_Z_NOT: //SPARC BGU
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!(((icc>>1) & 1) || ((icc>>2) & 1))) { //!(C || Z)
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBC_OR_Z: //SPARC BLEU
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(((icc>>1) & 1) || ((icc>>2) & 1)) { //(C || Z)
        state.crackInt->setPC(temp2); 

        flush = true;
      }
      break;
    case OP_U64_LBCC:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!((icc>>1) & 1)) {//C==0
        state.crackInt->setPC(temp2); 

        flush = true;
      } else {
        nop = true;
      }
      break;
    case OP_U64_LBPOS:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!((icc>>3) & 1)) {//N==0
        state.crackInt->setPC(temp2); 

        flush = true;
      } else {
        // nop = true;
      }
      break;
    case OP_U64_LBVC:
      icc = state.getICC(inst->uop.src1);
      temp2 = inst->uop.imm;
      if(!(icc & 1)) {//V==0
        state.crackInt->setPC(temp2); 

        flush = true;
      } else {
        // nop = true;
      }
      break;
    //case OP_U64_FBA:
    //  break;
    case OP_U64_FBUG:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>2) & 1) || (icc & 1)){ //C or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBLU:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || (icc & 1)){ //N or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBLG:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || ((icc>>2) & 1)){ //N or C
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBLGU:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || ((icc>>2) & 1) || (icc & 1)){ //N or C or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBEU:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>2) & 1) || (icc & 1)){ //Z or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBEG:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>2) & 1) || ((icc>>1) & 1)){ //Z or C
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBEGU:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>2) & 1) || ((icc>>1) & 1) || (icc & 1)){ //Z or C or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBEL:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || ((icc>>2) & 1)){ //N or Z
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBLUE:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || ((icc>>2) & 1) || (icc & 1)){ //N or Z or V
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_U64_FBLEG:
      icc = state.getICC(inst->uop.src1);
      if(((icc>>3) & 1) || ((icc>>2) & 1) || ((icc>>1) & 1)){ //N or Z or C
        state.crackInt->setPC(state.getReg(inst->uop.src1)); 

        flush = true;
      }
      break;
    case OP_S64_COPYICC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src2);
      state.setDst(inst, temp1, icc);
      break;
    case OP_S64_GETICC:
      icc = state.getICC(inst->uop.src1);
      state.setDst(inst, icc);
      break;
    case OP_S64_PUTICC:
      icc = state.getReg(inst->uop.src1);
      state.setDst(inst, 0, icc);
      break;
    case OP_S64_NOTICC:
      icc = state.getICC(inst->uop.src1);
      icc = icc ^ 0xF;
      temp1 = state.getReg(inst->uop.src1);
      state.setDst(inst, temp1, icc);
      break;
    case OP_S64_ANDICC:
      icc = state.getICC(inst->uop.src1);
      icc = icc & (inst->uop.imm & 0xF);
      temp1 = state.getReg(inst->uop.src1);
      state.setDst(inst, temp1, icc);
      break;
    case OP_S64_ORICC:
      icc = state.getICC(inst->uop.src1);
      icc = icc | (inst->uop.imm & 0xF);
      temp1 = state.getReg(inst->uop.src1);
      state.setDst(inst, temp1, icc);
      break;
    case OP_S64_XORICC:
      icc = state.getICC(inst->uop.src1);
      icc = icc ^ (inst->uop.imm & 0xF);
      temp1 = state.getReg(inst->uop.src1);
      state.setDst(inst, temp1, icc);
      break;
    case OP_S64_COPYPCL:
      I(inst->uop.src1==0 && inst->uop.src2==0);
      temp1 = inst->uop.imm;
      temp1 = temp1 & 0xFFFFFFFF;
      state.setDst(inst, temp1);
      break;
    case OP_S64_COPYPCH:
      I(inst->uop.src1==0 && inst->uop.src2==0);
      temp1 = inst->uop.imm;
      temp1 = temp1 << 32;
      temp2 = state.getReg(inst->uop.dst);
      temp1 = temp1 | temp2;
      state.setDst(inst, temp1);
      break;
    case OP_U64_CMOV_LGU:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (((icc>>1) & 1)) || (icc & 1)) //N or C or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_EGU:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>2) & 1) || (((icc>>1) & 1)) || (icc & 1)) //Z or C or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_LEG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (((icc>>2) & 1)) || ((icc>>1) & 1)) //N or Z or C
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_UG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if((((icc>>1) & 1)) || (icc & 1)) //C or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_EL:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (((icc>>2) & 1))) //N or Z
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_EG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>2) & 1) || (((icc>>1) & 1))) //Z or C
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_LU:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (icc & 1)) //N or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_LUE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (((icc>>2) & 1)) || (icc & 1)) //N or Z or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_LG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>3) & 1) || (((icc>>1) & 1))) //N or C
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_CMOV_E: 
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if((icc>>2) & 1) //Z==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_NE:
      icc = state.getICC(inst->uop.src1);
      icc2 = state.getICC(inst->uop.src2);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!((icc>>2) & 1)) //Z==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_CS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if((icc>>1) & 1) //C==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_CC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!((icc>>1) & 1)) //C==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_NEG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if((icc>>3) & 1) //N==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_POS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!((icc>>3) & 1)) //N==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_VS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(icc & 1) //V==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_VC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!(icc & 1)) //V==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_C_AND_NOTZ: //ARM HI
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>1) & 1) && !((icc>>2) & 1)) //C==1 && Z==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_NOTC_OR_Z:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!((icc>>1) & 1) || ((icc>>2) & 1)) //C==0 || Z==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_GE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!(((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_L:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if((((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_G:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(!(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1)))) //(Z OR (N XOR V))==0
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_LE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1))) //(Z OR (N XOR V))==1
        state.setDst(inst, temp1, icc);
      else {
        state.setDst(inst, temp2, icc2);
        nop = true;
      }
      break;
    case OP_U64_CMOV_A:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);

      state.setDst(inst, temp1, icc);
      break;
    case OP_U64_CMOV_EU:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else{
        temp2 = state.getReg(inst->uop.src2);
        icc2 = state.getICC(inst->uop.src2);
      }

      if(((icc>>2) & 1) || (icc & 1)) //Z or V
        state.setDst(inst, temp1, icc);
      else
        state.setDst(inst, temp2, icc2);
      break;
    case OP_U64_LD_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFe;
      res = prog->ldu64(static_cast<uint32_t>(temp1));
      state.setDst(inst, res);
      break;
    case OP_S16_LD_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAS16 = prog->lds16(static_cast<uint32_t>(temp1)) & 0xFFFF;
      temp1 = static_cast<int64_t>(tempAS16);
      res       = static_cast<DataType>(temp1);
      state.setDst(inst, res);
      break;
    case OP_S08_LD:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAS8 = prog->lds08(static_cast<uint32_t>(temp1)) & 0xFF;
      temp1 = static_cast<int64_t>(tempAS8);
      res       = static_cast<DataType>(temp1);
      state.setDst(inst, res);
      break;
    case OP_S32_LD_L:
      I(0);
    case OP_U32_LD_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFe;
      tempAU32 = prog->ldu32(static_cast<uint32_t>(temp1)) & 0xFFFFFFFF;
      res = static_cast<DataType>(tempAU32);
      state.setDst(inst, res);
      break;
    case OP_U16_LD_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAU16 = prog->ldu16(static_cast<uint32_t>(temp1)) & 0xFFFF;
      res       = static_cast<DataType>(tempAU16);
      state.setDst(inst, res);
      break;
    case OP_U08_LD:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAU8 = prog->ldu08(static_cast<uint32_t>(temp1)) & 0xFF;
      res       = static_cast<DataType>(tempAU8);
      state.setDst(inst, res);
      break;
    case OP_U64_LD_B:
      break;
    case OP_U32_LD_B:
      break;
    case OP_U16_LD_B:
      break;
    case OP_S32_LD_B:
      break;
    case OP_S16_LD_B:
      break;
    case OP_U64_ST_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFe;
      tempAU64 = state.getReg(inst->uop.src2);
      if (inst->uop.useImm)
        tempAU64 = (inst->uop.imm);
      atomHash.ahashLock(temp1);
      if (atomHash.isMarked())
        atomHash.insert_nolock(state.getFid(), temp1);
      prog->st64(static_cast<uint32_t>(temp1), tempAU64);
      atomHash.ahashUnlock(temp1);
      break;
    case OP_U32_ST_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFe;
      if (temp1 != 0xFFFFFFFF){
        tempAU32 = state.getReg(inst->uop.src2) & 0xFFFFFFFF;
        if (inst->uop.useImm)
          tempAU32 = (inst->uop.imm);
        atomHash.ahashLock(temp1);
        if (atomHash.isMarked())
          atomHash.insert_nolock(state.getFid(), temp1);
        prog->st32(static_cast<uint32_t>(temp1), tempAU32);
        atomHash.ahashUnlock(temp1);
      }
      break;
    case OP_U16_ST_L:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAU16 = state.getReg(inst->uop.src2) & 0xFFFF;
      if (inst->uop.useImm)
        tempAU16 = (inst->uop.imm);
      atomHash.ahashLock(temp1);
      if (atomHash.isMarked())
        atomHash.insert_nolock(state.getFid(), temp1);
      prog->st16(static_cast<uint32_t>(temp1), tempAU16);
      atomHash.ahashUnlock(temp1);
      break;
    case OP_U08_ST:
      temp1 = state.getReg(inst->uop.src1) & 0xFFFFFFFF;
      tempAU8 = state.getReg(inst->uop.src2) & 0xFF;
      if (inst->uop.useImm)
        tempAU8 = (inst->uop.imm);
      atomHash.ahashLock(temp1);
      if (atomHash.isMarked())
        atomHash.insert_nolock(state.getFid(), temp1);
      prog->st08(static_cast<uint32_t>(temp1), tempAU8);
      atomHash.ahashUnlock(temp1);
     break;
    case OP_U64_ST_B:
      break;
    case OP_U32_ST_B:
      break;
    case OP_U16_ST_B:
      break;
   case OP_U32_STSC: //syscall trigger
      
 
      /* Only for now that we do not support system call in stand alone version
       * We exit at syscall. The only syscall should be NR_EXIT
       */
     



      tempAU32 = state.getReg(inst->uop.src2);
      arg1 = state.getReg(1);
      arg2 = state.getReg(2);
      arg3 = state.getReg(3);
      arg4 = state.getReg(4);
      arg5 = state.getReg(5);
      arg6 = state.getReg(6);
      //TODO: if QEMU does not protect for parallel access, do it here
      

      if (tempAU32 == 0xf0005){ //QEMU does not support this syscall, we need to add it
        prog->st32(static_cast<uint32_t>(state.getReg(LREG_SYSMEM) +
              CVAR_TLS2_OFFST*sizeof(uint64_t)), state.getReg(1)); //FIXME: store the R0 in memory? 
        state.setReg(1,0,0);
      }else if (tempAU32 == 0x1FFFF){
        do_clz(prog, arg1);
      } else if (tempAU32 == 0x2FFFF){
        do_mrc(prog, arg1);
      } else if (tempAU32 == 0x3FFFF){
        do_vabs(prog, arg1);
      } else if (tempAU32 == 0x4FFFF){
        do_vaba(prog, arg1);
      } else if (tempAU32 == 0x5FFFF){
        do_vabd(prog, arg1);
      } else if(tempAU32 == 0x6FFFF) {
        do_vclz(prog, arg1);
      } else if(tempAU32 == 0x7FFFF) {
        do_vcls(prog, arg1);
      } else if(tempAU32 == 0x8FFFF) {
        do_vcnt(prog, arg1);
      } else if(tempAU32 == 0x9FFFF) {
        do_vqshl(prog, arg1);
      } else if(tempAU32 == 0xAFFFF) {
        do_vqrshl(prog, arg1);
      } else if(tempAU32 == 0xBFFFF) {
        do_vqdmulh(prog, arg1);
      } else if(tempAU32 == 0xCFFFF) {
        do_vqdmlal_sl(prog, arg1);
      } else if(tempAU32 == 0xDFFFF) {
        do_vmulpoly(prog, arg1);
      } else if(tempAU32 == 0xEFFFF) {
        do_vqshrn(prog, arg1);
      } else if(tempAU32 == 0xFFFFF) {
        do_vqmovn(prog, arg1);
      } else if(tempAU32 == 0x2FFFFF) {
        do_vcvtb_htos(prog, arg1);
      } else if(tempAU32 == 0x3FFFFF) {
        do_vcvtt_htos(prog, arg1);
      } else if(tempAU32 == 0x4FFFFF) {
        do_vcvtfxfp(prog, arg1);
      } else if(tempAU32 == 0x5FFFFF) {
        do_vcvtb_stoh(prog, arg1);
      } else if(tempAU32 == 0x6FFFFF) {
        do_vcvtt_stoh(prog, arg1);
      } else{
        
#if 1
        for(int i=0;i<16;i++){ // Transfer Arch state to QEMU
          if (i == 15) //pc
            prog->set_reg(i, state.crackInt->getPC()+4);
          else
            prog->set_reg(i, state.getReg(i+1));
        }
#if 0
        printf("num:0x%x\n", tempAU32);
        printf("arg1:0x%x\n", arg1);
        printf("arg2:0x%x\n", arg2);
        printf("arg3:0x%x\n", arg3);
        printf("arg4:0x%x\n", arg4);
        printf("arg5:0x%x\n", arg5);
        printf("arg6:0x%x\n", arg6); 
#endif
        arg1= prog->do_syscall(prog, tempAU32, arg1, arg2, arg3, arg4, arg5, arg6, syscallTrace);
        state.setReg(1,arg1,0); //FIXME: should be read in the LD after STSC
#else
        printf("FIXME:ERROR!!! syscall disabled to have a stand alone sccore\n");
#endif
      }
      break;
    case OP_U64_CADD_E: 
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 + temp2;
      if((icc>>2) & 1) //Z==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_NE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 + temp2;
      if(!((icc>>2) & 1)) //Z==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_CS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 + temp2;
      if((icc>>1) & 1) //C==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_CC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 + temp2;

      if(!((icc>>1) & 1)) //C==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_NEG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if((icc>>3) & 1) //N==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_POS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!((icc>>3) & 1)) //N==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_VS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(icc & 1) //V==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_VC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!(icc & 1)) //V==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_C_OR_Z_NOT:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!(((icc>>1) & 1) || ((icc>>2) & 1))) //!(C || Z)
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_C_OR_Z:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(((icc>>1) & 1) || ((icc>>2) & 1)) //(C || Z)
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_C_AND_NOTZ:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(((icc>>1) & 1) && !((icc>>2) & 1)) //C==1 && Z==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_NOTC_OR_Z:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!((icc>>1) & 1) || ((icc>>2) & 1)) //C==0 || Z==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_GE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!(((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_L:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if((((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_G:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(!(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1)))) //(Z OR (N XOR V))==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CADD_LE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 + temp2;
      if(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1))) //(Z OR (N XOR V))==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_E: 
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 - temp2;
      if((icc>>2) & 1) //Z==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_NE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 - temp2;
      if(!((icc>>2) & 1)) //Z==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_CS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 - temp2;
      if((icc>>1) & 1) //C==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_CC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);

      res = temp1 - temp2;
      if(!((icc>>1) & 1)) //C==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_NEG:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if((icc>>3) & 1) //N==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_POS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!((icc>>3) & 1)) //N==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_VS:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(icc & 1) //V==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_VC:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!(icc & 1)) //V==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_C_OR_Z_NOT:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!(((icc>>1) & 1) || ((icc>>2) & 1))) //!(C || Z)
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_C_OR_Z:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(((icc>>1) & 1) || ((icc>>2) & 1)) //(C || Z)
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_C_AND_NOTZ:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(((icc>>1) & 1) || !((icc>>2) & 1)) //C==1 && Z==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_NOTC_OR_Z:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!((icc>>1) & 1) && ((icc>>2) & 1)) //C==0 || Z==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_GE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!(((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_L:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if((((icc>>3) & 1) ^ (icc & 1))) //(N XOR V)==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_G:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(!(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1)))) //(Z OR (N XOR V))==0
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_U64_CSUB_LE:
      icc = state.getICC(inst->uop.src1);
      temp1 = state.getReg(inst->uop.src1);
      temp2 = state.getReg(inst->uop.src2);
 
      res = temp1 - temp2;
      if(((icc>>2) & 1)  || (((icc>>3) & 1) ^ (icc & 1))) //(Z OR (N XOR V))==1
        state.setDst(inst, res);
      else
        state.setDst(inst, -1);
      break;
    case OP_C_SMUL:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 * temp2;
 
      icc = calcNZ(uint64_t(res));

      state.setDst(inst, res, icc);
      break;
    case OP_C_UMUL:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else
        temp2 = state.getReg(inst->uop.src2);
      res = (uint64_t)temp1 * (uint64_t)temp2;
 
      icc = calcNZ(uint64_t(res));

      state.setDst(inst, res, icc);
      break;
    case OP_C_UDIV:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else
        temp2 = state.getReg(inst->uop.src2);
      res = (uint64_t)temp1 / (uint64_t)temp2;
 
      icc = calcNZ(uint64_t(res));

      state.setDst(inst, res, icc);
      break;
    case OP_C_SDIV:
      temp1 = state.getReg(inst->uop.src1);
      if (inst->uop.useImm)
        temp2  = (inst->uop.imm);
      else
        temp2 = state.getReg(inst->uop.src2);
      res = temp1 / temp2;
 
      icc = calcNZ(uint64_t(res));

      state.setDst(inst, res, icc);
      break;
    case OP_U64_MARK_EX:
      temp1 = state.getReg(inst->uop.src1);
      atomHash.insert(state.getFid(), temp1);
      break;

    case OP_U64_ST_COND:
      temp1 = state.getReg(inst->uop.src1);
      if (atomHash.setSC(state.getFid(), temp1)){
        tempAU64 = state.getReg(inst->uop.src2);
        prog->st64(static_cast<uint32_t>(temp1), tempAU64);
      }
      break;
    case OP_U32_ST_COND:
      temp1 = state.getReg(inst->uop.src1);
      if (atomHash.setSC(state.getFid(), temp1)){
        tempAU32 = state.getReg(inst->uop.src2);
        prog->st32(static_cast<uint32_t>(temp1), tempAU32);
      }
      break;
    case OP_U16_ST_COND:
      temp1 = state.getReg(inst->uop.src1);
      if (atomHash.setSC(state.getFid(), temp1)){
        tempAU16 = state.getReg(inst->uop.src2);
        prog->st16(static_cast<uint32_t>(temp1), tempAU16);
      }
      break;
    case OP_U08_ST_COND:
      temp1 = state.getReg(inst->uop.src1);
      if (atomHash.setSC(state.getFid(), temp1)){
        tempAU8 = state.getReg(inst->uop.src2);
        prog->st08(static_cast<uint32_t>(temp1), tempAU8);
      }
      break;
    case OP_U64_ST_CHECK_EX:
      temp1 = state.getReg(inst->uop.src1);
      if (atomHash.checkSC(state.getFid(), temp1)){
        icc = (1 << 2); //EQ on SUCCESS
        state.setReg(LREG_ICC, 0, icc);
      }else{
        icc = (0 << 2); //NEQ on FAILURE
        state.setReg(LREG_ICC, 0, icc);
      }
      break;
    case OP_U64_CLR_ADDR:
      break;
      ///////////////////////////////// Floating Point

    case OP_C_JOINFSR:
    case OP_C_FSTOI:
      tempFSA = state.getReg(inst->uop.src1);
      res = float32_to_int32 (tempFSA);
      state.setDst(inst, res, 0); 
      break;
   case OP_C_FSTOD:
      tempFSA = state.getReg(inst->uop.src1);
      resFS = float32_to_float64 (tempFSA);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FMOVS:
      tempFSA = state.getReg(inst->uop.src1);
      resFS = tempFSA;
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FNEGS:
      tempFSA = state.getReg(inst->uop.src1);
      resFS = float32_mul (tempFSA, -1.0);
      state.setDst(inst, resFS, 0); 
      break;
   case OP_C_FABSS:
      tempFSA = state.getReg(inst->uop.src1);
      if (tempFSA < 0)
        resFS = float32_mul (tempFSA, -1.0);
      else
        resFS = tempFSA;
      state.setDst(inst, resFS, 0); 
      break;
   case OP_C_FSQRTS:
       tempFSA = state.getReg(inst->uop.src1);
      resFS = float32_sqrt (tempFSA);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FADDS:
       tempFSA = state.getReg(inst->uop.src1);
       tempFSB = state.getReg(inst->uop.src2);
      resFS = float32_add (tempFSA, tempFSB);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FSUBS:
       tempFSA = state.getReg(inst->uop.src1);
       tempFSB = state.getReg(inst->uop.src2);
      resFS = float32_sub (tempFSA, tempFSB);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FMULS:
       tempFSA = state.getReg(inst->uop.src1);
       tempFSB = state.getReg(inst->uop.src2);
      resFS = float32_mul (tempFSA, tempFSB);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FDIVS:
       tempFSA = state.getReg(inst->uop.src1);
       tempFSB = state.getReg(inst->uop.src2);
      resFS = float32_div (tempFSA, tempFSB);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FSMULD:
      tempFSA = state.getReg(inst->uop.src1);
      tempFDA = float32_to_float64(tempFSA);
      tempFDB = state.getReg(inst->uop.src2);
      resFD = float64_mul (tempFDA, tempFDB);
      state.setDst(inst, resFD, 0); 
      break;
   case OP_C_FCMPS:
    case OP_C_FCMPES:

    case OP_C_FDTOI:
      tempFDA = state.getReg(inst->uop.src1);
      res = float64_to_int32 (tempFDA);
      state.setDst(inst, res, 0); 
      break;
    case OP_C_FDTOS:
      tempFDA = state.getReg(inst->uop.src1);
      resFS = float64_to_float32 (tempFDA);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FMOVD:
      tempFSA = state.getReg(inst->uop.src1);
      resFS = tempFSA;
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FNEGD:
      tempFDA = state.getReg(inst->uop.src1);
      resFD = float64_mul (tempFDA, -1.0);
      state.setDst(inst, resFD, 0); 
      break;
   case OP_C_FABSD:
      tempFDA = state.getReg(inst->uop.src1);
      if (tempFDA < 0)
        resFD = float32_mul (tempFDA, -1.0);
      else
        resFD = tempFDA;
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FSQRTD:
        tempFSA = state.getReg(inst->uop.src1);
        tempFDA = float32_to_float64(tempFSA);
        resFD = float64_sqrt (tempFDA);
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FADDD:
       tempFDA = state.getReg(inst->uop.src1);
       tempFDB = state.getReg(inst->uop.src2);
      resFD = float64_add (tempFDA, tempFDB);
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FSUBD:
       tempFDA = state.getReg(inst->uop.src1);
       tempFDB = state.getReg(inst->uop.src2);
      resFD = float64_sub (tempFDA, tempFDB);
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FMULD:
       tempFDA = state.getReg(inst->uop.src1);
       tempFDB = state.getReg(inst->uop.src2);
      resFD = float64_mul (tempFDA, tempFDB);
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FDIVD:
       tempFDA = state.getReg(inst->uop.src1);
       tempFDB = state.getReg(inst->uop.src2);
      resFD = float64_div (tempFDA, tempFDB);
      state.setDst(inst, resFD, 0); 
      break;
    case OP_C_FCMPD:
    case OP_C_FCMPED:


    case OP_C_FITOS:
      tempFDA = state.getReg(inst->uop.src1);
      resFS = int32_to_float32 (tempFDA);
      state.setDst(inst, resFS, 0); 
      break;
    case OP_C_FITOD:
      tempFDA = state.getReg(inst->uop.src1);
      resFD = int32_to_float64 (tempFDA);
      state.setDst(inst, resFD, 0); 
      break;

    case OP_C_CONCAT:
    case OP_C_MULSCC:



    default:
      MSG("ERROR: %s op is not implemented", inst->getOpString(inst->uop.op));
  }
}
/* }}} */

void Sccore::dump(const char *str) const
/* dump core stats {{{1 */
{
  state.dump(str);
}
/* }}} */

void Sccore::validateRegister(int reg_num, char *exp_reg_val) const
{
  state.validateRegister(reg_num, exp_reg_val);
}
