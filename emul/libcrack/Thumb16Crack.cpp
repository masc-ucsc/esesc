// Contributed by Jose Renau
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

#include "ThumbCrack.h"
#include "crack_scqemu.h"
#include <iostream>

//Search for ARMv6T2 or ARMv6K or ARMv7 for instructions only in those versions and Higher
//The rest of the instructions are from ARMv6 and Higher

//Cracks Thumb instructions into SCOORE uOps.
//
//

#ifdef DEBUG
#define OOPS(x) do { static int conta=0; if (conta<1) { printf(x); conta++; }; }while(0)
#else
#define OOPS(x) 
#endif 

void ThumbCrack::thumb16expand(RAWDInst *rinst)
{
  rinst->clearInst();

  const uint8_t AL             = 14;
  uint8_t cond = AL;

  if(itblockVector->empty()) { //not in ITBLOCK
    rinst->setInITBlock(false);
  } else {
    rinst->setInITBlock(true);

    cond = itblockVector->back();
    itblockVector->pop_back();
  }

  uint32_t insn                = rinst->getInsn();


  uint8_t b15                  = (insn>>15) & 0x1;
  uint8_t b14                  = (insn>>14) & 0x1;
  uint8_t b13                  = (insn>>13) & 0x1;
  uint8_t b12                  = (insn>>12) & 0x1;
  uint8_t b11                  = (insn>>11) & 0x1;
  uint8_t b10                  = (insn>>10) & 0x1;

  uint8_t b9                   = (insn>>9) & 0x1;
  uint8_t b8                   = (insn>>8) & 0x1;
  uint8_t b7                   = (insn>>7) & 0x1;
  uint8_t b6                   = (insn>>6) & 0x1;
  uint8_t b5                   = (insn>>5) & 0x1;

  //+1 to account for R0 which is a register hardcoded to 0 in SCOORE but is a general register in ARM
  const uint8_t  STACKPTR      = 13 + 1;
  const uint8_t  LINK          = 14 + 1;
  const uint8_t  PC_REG        = 15 + 1;
  const uint64_t PC            = rinst->getPC();

  //IMMEDIATES
  const uint32_t  IMM3         = (insn>>6) & 0x7;
  const uint32_t  IMM5         = (insn>>6) & 0x1F;
  const uint32_t  IMM7         = insn & 0x7F;
  const uint32_t  IMM8         = insn & 0xFF;
  const uint32_t IMM11         = insn & 0x7FF;

  //+1 to account for R0 which is a register hardcoded to 0 in SCOORE but is a general register in ARM
  uint8_t RN                   = ((insn>>3) & 0x7) + 1;
  uint8_t RM                   = ((insn>>6) & 0x7) + 1;
  uint8_t RD                   = (insn & 0x7) + 1;
  uint8_t RDN                  = ((insn>>8) & 0x7) + 1;


  Scopcode cmoveArray[] = { OP_U64_CMOV_E,         OP_U64_CMOV_NE,  OP_U64_CMOV_CS, 
                            OP_U64_CMOV_CC,        OP_U64_CMOV_NEG, OP_U64_CMOV_POS, 
                            OP_U64_CMOV_VS,        OP_U64_CMOV_VC,  OP_U64_CMOV_C_AND_NOTZ, 
                            OP_U64_CMOV_NOTC_OR_Z, OP_U64_CMOV_GE,  OP_U64_CMOV_L,
                            OP_U64_CMOV_G,         OP_U64_CMOV_LE,  OP_U64_CMOV_A
                          };

  Scopcode negCmoveArray[] = { OP_U64_CMOV_NE,          OP_U64_CMOV_E,   OP_U64_CMOV_CC, 
                               OP_U64_CMOV_CS,          OP_U64_CMOV_POS, OP_U64_CMOV_NEG, 
                               OP_U64_CMOV_VC,          OP_U64_CMOV_VS,  OP_U64_CMOV_NOTC_OR_Z, 
                               OP_U64_CMOV_C_AND_NOTZ, OP_U64_CMOV_L,   OP_U64_CMOV_GE,
                               OP_U64_CMOV_LE,          OP_U64_CMOV_G,   OP_U64_CMOV_A
                             };

  Scopcode rbranchArray[] = { OP_U64_RBE,         OP_U64_RBNE,  OP_U64_RBCS, 
                              OP_U64_RBCC,        OP_U64_RBNEG, OP_U64_RBPOS, 
                              OP_U64_RBVS,        OP_U64_RBVC,  OP_U64_RBC_AND_NOTZ,
                              OP_U64_RBNOTC_OR_Z, OP_U64_RBGE,  OP_U64_RBL,
                              OP_U64_RBG,         OP_U64_RBLE
                            };

  Scopcode negrbranchArray[] = { OP_U64_RBNE,        OP_U64_RBE,   OP_U64_RBCC, 
                                 OP_U64_RBCS,        OP_U64_RBPOS, OP_U64_RBNEG, 
                                 OP_U64_RBVC,        OP_U64_RBVS,  OP_U64_RBNOTC_OR_Z,
                                 OP_U64_RBNOTC_OR_Z, OP_U64_RBL,   OP_U64_RBGE,
                                 OP_U64_RBLE,        OP_U64_RBG
                               };

  Scopcode caddArray[] = { OP_U64_CADD_E,          OP_U64_CADD_NE, 
                           OP_U64_CADD_CS,         OP_U64_CADD_CC,
                           OP_U64_CADD_NEG,        OP_U64_CADD_POS, 
                           OP_U64_CADD_VS,         OP_U64_CADD_VC,
                           OP_U64_CADD_C_AND_NOTZ, OP_U64_CADD_NOTC_OR_Z, 
                           OP_U64_CADD_GE,         OP_U64_CADD_L,
                           OP_U64_CADD_G,          OP_U64_CADD_LE, 
                           OP_U64_ADD
                         };

  if(!b15 && !b14) { //shift(imm), add, sub, move, compare //A6-7

      if(cond != AL) { //not always
        if(!b13 && !b12 && !b11) { //SLL (imm) //A8-178
          CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, IMM5, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && !b12 && b11) { //SRL (imm) //A8-182
          CrackInst::setup(rinst, iAALU, OP_S32_SRL, RN, 0, IMM5, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && b12 && !b11) { //SRA (imm) //A8-40
          CrackInst::setup(rinst, iAALU, OP_S32_SRA, RN, 0, IMM5, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && b12 && b11 && !b10 && !b9) { //ADD register //A8-24
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && b12 && b11 && !b10 && b9) { //SUB register //A8-422
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RN, RM, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && b12 && b11 && b10 && !b9) { //ADD 3-bit imm ADD(imm, Thumb) //A8-20
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RN, 0, IMM3, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(!b13 && b12 && b11 && b10 && b9) { //SUB 3-bit imm SUB(imm, Thumb)  //A8-418
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RN, 0, IMM3, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
        }
        else if(b13 && !b12 && !b11) { //Move //A8-194
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, 0, 0, IMM8, LREG_TMP1, 0, 0, 0); //FIXME: RAT/ROB move?
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RDN, 0, RDN, 0, 0, 0);  
        }
        else if(b13 && !b12 && b11) { //Compare //A8-80
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RDN, 0, IMM8, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
          CrackInst::setup(rinst, iAALU, negCmoveArray[cond], LREG_ICC, LREG_TMP1, 0, LREG_ICC, 1, 0, 0);  
        }
        else if(b13 && b12 && !b11) { //ADD 8-bit imm ADD(imm, Thumb) //A8-20
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RDN, 0, IMM8, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RDN, 0, RDN, 0, 0, 0);  
        }
        else if(b13 && b12 && b11) { //SUB 8-bit imm SUB(imm, Thumb) //A8-418
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RDN, 0, IMM8, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
          CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RDN, 0, RDN, 0, 0, 0);  
        }
      } //end not always
      else { //always
        if(!b13 && !b12 && !b11) { //SLL (imm) //A8-178
          if(IMM5 != 0)
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, IMM5, RD, 1, 0, 0);  
          else //MOV //A8-196 //Encoding T2 //NOT PERMITTED IN ITBLOCK
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RN, 0, 0, RD, 1, 0, 0);  
        }
        else if(!b13 && !b12 && b11) //SRL (imm) //A8-182
          CrackInst::setup(rinst, iAALU, OP_S32_SRL, RN, 0, IMM5, RD, 1, 0, 0);  
        else if(!b13 && b12 && !b11) //SRA (imm) //A8-40
          CrackInst::setup(rinst, iAALU, OP_S32_SRA, RN, 0, IMM5, RD, 1, 0, 0);  
        else if(!b13 && b12 && b11 && !b10 && !b9) //ADD register //A8-24
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RN, RM, 0, RD, 1, 0, 0);  
        else if(!b13 && b12 && b11 && !b10 && b9) { //SUB register //A8-422
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RN, RM, 0, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, RD, 1, 0, 0); //invert carry
        }
        else if(!b13 && b12 && b11 && b10 && !b9) //ADD 3-bit imm ADD(imm, Thumb) //A8-20
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RN, 0, IMM3, RD, 1, 0, 0);  
        else if(!b13 && b12 && b11 && b10 && b9) { //SUB 3-bit imm SUB(imm, Thumb)  //A8-418
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RN, 0, IMM3, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, RD, 1, 0, 0); //invert carry
        }
        else if(b13 && !b12 && !b11) //Move //A8-194
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, 0, 0, IMM8, RDN, 1, 0, 0); //FIXME: RAT/ROB move?
        else if(b13 && !b12 && b11) {//Compare //A8-80
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RDN, 0, IMM8, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, 0, 1, 0, 0); //invert carry
        }
        else if(b13 && b12 && !b11) //ADD 8-bit imm ADD(imm, Thumb) //A8-20
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RDN, 0, IMM8, RDN, 1, 0, 0);
        else if(b13 && b12 && b11) { //SUB 8-bit imm SUB(imm, Thumb) //A8-418
          CrackInst::setup(rinst, iAALU, OP_S32_SUB, RDN, 0, IMM8, LREG_TMP1, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, RDN, 1, 0, 0); //invert carry
        }
      } //end always
  } //end shift(imm), add, sub, move, compare
  else if(!b15 && b14 && !b13 && !b12 && !b11 && !b10) { //data-processing //A6-8
    uint8_t opcode = (insn>>6) & 0xF;

      if(cond != AL) { //not always
        switch(opcode) {
          case 0: //AND (register) //A8-36
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 1: //EOR (register) //A8-96
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 2: //SLL (register) //A8-180
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 3: //SRL (register) //A8-184
            CrackInst::setup(rinst, iAALU, OP_S32_SRL, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 4: //SRA (register) //A8-42
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 5: //ADC (register) //A8-16
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RD, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 6: //SBC (register) //A8-304
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RD, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 7: //ROR (register) //A8-280
            CrackInst::setup(rinst, iAALU, OP_S32_ROTR, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 8: //TST (register) //A8-456
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, negCmoveArray[cond], LREG_ICC, LREG_TMP1, 0, LREG_ICC, 1, 0, 0);  
            break;
          case 9: //RSB from 0 (register) NEG //A8-284
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, 0, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 10: //CMP registers (register) //A8-82
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RD, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, negCmoveArray[cond], LREG_ICC, LREG_TMP1, 0, LREG_ICC, 1, 0, 0);  
            break;
          case 11: //CMN Compare negative (register) //A8-76
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RD, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, negCmoveArray[cond], LREG_ICC, LREG_TMP1, 0, LREG_ICC, 1, 0, 0);  
            break;
          case 12: //ORR (register) //A8-230
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 13: //MUL (register) //A8-212
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RN, RD, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 14: //BIC (register) //A8-52
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RD, RN, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
          case 15: //MVN (register) //A8-216
            CrackInst::setup(rinst, iAALU, OP_U64_SUB, 0, 0, 1, LREG_TMP2, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, LREG_TMP2, 0, 32, LREG_TMP2, 0, 0, 0);  

            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RN, LREG_TMP2, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
            break;
        }
      } //end not always
      else { //always
        switch(opcode) {
          case 0: //AND (register) //A8-36
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 1: //EOR (register) //A8-96
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 2: //SLL (register) //A8-180
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 3: //SRL (register) //A8-184
            CrackInst::setup(rinst, iAALU, OP_S32_SRL, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 4: //SRA (register) //A8-42
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 5: //ADC (register) //A8-16
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RD, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, RN, 0, RD, 0, 0, 0);  
            break;
          case 6: //SBC (register) //A8-304
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RD, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, RD, 0, 0, 0);
            break;
          case 7: //ROR (register) //A8-280
            CrackInst::setup(rinst, iAALU, OP_S32_ROTR, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 8: //TST (register) //A8-456
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RD, RN, 0, 0, 1, 0, 0);  
            break;
          case 9: //RSB from 0 (register) NEG //A8-284
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, 0, RN, 0, RD, 0, 0, 0);
            break;
          case 10: //CMP registers (register) //A8-82
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RD, RN, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, 0, 1, 0, 0); //invert carry
            break;
          case 11: //CMN Compare negative (register) //A8-76
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RD, RN, 0, 0, 1, 0, 0);
            break;
          case 12: //ORR (register) //A8-230
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 13: //MUL (register) //A8-212
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RN, RD, 0, RD, 0, 0, 0);
            break;
          case 14: //BIC (register) //A8-52
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RD, RN, 0, RD, 0, 0, 0);  
            break;
          case 15: //MVN (register) //A8-216
            CrackInst::setup(rinst, iAALU, OP_U64_SUB, 0, 0, 1, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, LREG_TMP1, 0, 32, LREG_TMP1, 0, 0, 0);  

            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RN, LREG_TMP1, 0, RD, 0, 0, 0);  
            break;
        }
      } //end always
  } //end data-processing
  else if(!b15 && b14 && !b13 && !b12 && !b11 && b10) { //specal data instructions and branch and exchange //A6-9
    uint8_t opcode = (insn>>6) & 0xF;
    uint8_t RMH  = ((insn>>3) & 0xF) + 1;
    uint8_t RND = ((b7 << 3) | RD);

       if(cond != AL) { //not always
         switch(opcode) {
           case 0: //ADD registers //A8-24 //ARMv6T2 //Encoding T2
           case 1: 
           case 2: 
           case 3:
             //If RM == 13+1: then it is STACKPTR
             //If RND==PC_REG and if RM == PC_REG then UNPREDICTABLE

             if(RMH == PC_REG) {
               if(RND != PC_REG) {
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RND, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);

                 CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
                 CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RND, 0, RND, 0, 0, 0);  
               }
             }
             else {
               if(RND == PC_REG) {
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, RMH, 0, LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iBALU_RBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
               }
               else {
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RMH, RND, 0, LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
                 CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RND, 0, RND, 0, 0, 0);  
               }
             }
             break;
           case 4: //UNPREDICTABLE
             OOPS("WARNING: Instruction defined as UNPREDICTABLE.\n");
             break;
           case 5: //Compare High registers //A8-82
           case 6:
           case 7:
             //If RND < 8+1 or if RMH < 8+1 then UNPREDICTABLE
             //If RND==PC_REG or if RMH == PC_REG then UNPREDICTABLE
             CrackInst::setup(rinst, iAALU, OP_S32_SUB, RND, RMH, 0, LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
             CrackInst::setup(rinst, iAALU, negCmoveArray[cond], LREG_ICC, LREG_TMP1, 0, LREG_ICC, 1, 0, 0);  
             break;
           case 8: //MOV //A8-196
           case 9:
           case 10:
           case 11:
               //If RMH == PC_REG, DEPRECATED so it is unsupported here
             if(RND == PC_REG) {
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RMH, 0, LREG_TMP1, 0, 0, 0);
               if(RMH == LINK) 
                 CrackInst::setup(rinst, iBALU_RET, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
               else
                 CrackInst::setup(rinst, iBALU_RBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
             }
             else {
               if(RMH == PC_REG) {
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
                 CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RND, 0, RND, 0, 0, 0);  
               }
               else {
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RMH, 0, 0, LREG_TMP1, 0, 0, 0); //FIXME: RAT/ROB move? 
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
                 CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RND, 0, RND, 0, 0, 0);  
               }
             }
             break;
           case 12: //Branch and Exchange BX //A8-62
           case 13:
             if(RMH == PC_REG) {
               CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  

               CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 1, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP2, LREG_TTYPE, 0, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP2, 0, ARM, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);  
               CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP2, LREG_TTYPE, 0, LREG_TTYPE, 0, 0, 0);  

               CrackInst::setup(rinst, iBALU_RBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
             }
             else {
               CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 1, LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP1, LREG_TTYPE, 0, LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP1, 0, ARM, LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
               CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, LREG_TTYPE, 0, LREG_TTYPE, 0, 0, 0);  

               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RMH, 0, LREG_TMP1, 0, 0, 0);  
               CrackInst::setup(rinst, iBALU_RBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
             }
             break;
           case 14: //BLX (register) //A8-60
           case 15:
             //If RMH == PC_REG, UNPREDICTABLE
             CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC+2)|1), LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, LINK, 0, LINK, 0, 0, 0);  
             CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RMH, 0, LREG_TMP1, 0, 0, 0);

             CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 1, LREG_TMP2, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP2, LREG_TTYPE, 0, LREG_TMP2, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP2, 0, ARM, LREG_TMP2, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP2, LREG_TTYPE, 0, LREG_TTYPE, 0, 0, 0);  

             CrackInst::setup(rinst, iBALU_RCALL, cmoveArray[cond], LREG_TMP1, 0, 0, 0, 0, 0, 0);
             break;
         }    
       } //end not always
       else { //always
         switch(opcode) {
           case 0: //ADD registers //A8-24 //ARMv6T2 //Encoding T2
           case 1: 
           case 2: 
           case 3:
             //If RMH == 13+1: then it is STACK POINTER
             //If RND==PC_REG and if RMH == PC_REG then UNPREDICTABLE

             if(RMH == PC_REG) {
               if(RND != PC_REG) {
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RND, LREG_TMP1, 0, RND, 0, 0, 0);
               }
               else { //UNPREDICTABLE
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
                 CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
               }
             }
             else { 
               if(RND == PC_REG) {
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4), LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, RMH, 0, LREG_TMP1, 0, 0, 0);
                 CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
                 CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
               }
               else {
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RND, RMH, 0, RND, 0, 0, 0);
               }
             }
             break;
           case 4: //UNPREDICTABLE
             OOPS("WARNING: Instruction defined as UNPREDICTABLE.\n");
             break;
           case 5: //Compare High registers //A8-82
           case 6:
           case 7:
             //If RND < 8+1 or if RMH < 8+1 then UNPREDICTABLE
             //If RND==PC_REG or if RMH == PC then UNPREDICTABLE
             CrackInst::setup(rinst, iAALU, OP_S32_SUB, RND, RMH, 0, LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, 0, 1, 0, 0); //invert carry
             break;
           case 8: //MOV //A8-196
           case 9:
           case 10:
           case 11:
               //If RMH == PC_REG, DEPRECATED so it is unsupported here
             if(RND == PC_REG)
               if(RMH == LINK) {
                 CrackInst::setup(rinst, iBALU_RET, OP_U64_JMP_REG, RMH, 0, 0, 0, 0, 0, 0);
               }
               else {
                 CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, RMH, 0, 0, 0, 0, 0, 0);
               }
             else {
               if(RMH == PC_REG)
                 CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), RND, 0, 0, 0);
               else  
                 CrackInst::setup(rinst, iAALU, OP_S32_ADD, RMH, 0, 0, RND, 0, 0, 0); //FIXME: RAT/ROB move?
             }
             break;
           case 12: //Branch and Exchange BX //A8-62
           case 13:
             if(RMH == PC_REG) {
               CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFE)+4), LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 1, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP2, LREG_TTYPE, 0, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP2, 0, ARM, LREG_TTYPE, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
               CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
             }
             else {
               CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 1, LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 0xFFFFFFFE, LREG_TMP2, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP1, LREG_TTYPE, 0, LREG_TMP1, 0, 0, 0);
               CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP1, 0, ARM, LREG_TTYPE, 0, 0, 0);
               CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP2, 0, 0, 0, 0, 0, 0);
             }
             break;
           case 14: //BLX (register) //A8-60
           case 15:
             //If RMH == PC then UNPREDICTABLE
             CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC+2)|1), LINK, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 1, LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP1, LREG_TTYPE, 0, LREG_TMP1, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP1, 0, ARM, LREG_TTYPE, 0, 0, 0);
             CrackInst::setup(rinst, iAALU, OP_S64_AND, RMH, 0, 0xFFFFFFFE, LREG_TMP2, 0, 0, 0);
             CrackInst::setup(rinst, iBALU_RCALL, OP_U64_JMP_REG, LREG_TMP2, 0, 0, 0, 0, 0, 0);
             break;
         }
       } //end always
  } //end specal data instructions and branch and exchange
  else if(!b15 && b14 && !b13 && !b12 && b11) { //load from literal pool, see LDR (literal) //A8-122


      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
      } //end not always

      CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFC)+4), LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, (IMM8 << 2), LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, RDN, 0, 0, 0);
  } //end load from literal pool, see LDR (literal)
  else if(!b15 && b14 && !b13 && b12) { //load/store single data item //A6-10
    uint8_t opB = (insn>>9) & 0x7;

      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
      }

      switch(opB) {
        case 0: //STR (register) //A8-386
          CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, RD, 0, 0, 0, 0, 0);
          break;
        case 1: //STRH (register) //A8-412
          CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U16_ST_L, LREG_TMP1, RD, 0, 0, 0, 0, 0);
          break;
        case 2: //STRB (register) //A8-392
          CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U08_ST, LREG_TMP1, RD, 0, 0, 0, 0, 0);
          break;
        case 3: //LDRSB (register) //A8-164
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_S08_LD, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          break;
        case 4: //LDR (register) //A8-124
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          break;
        case 5: //LDRH (register) //A8-156
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U16_LD_L, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          break;
        case 6: //LDRB (register) //A8-132
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U08_LD, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          break;
        case 7: //LDRSH (register) //A8-172
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, RM, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_S16_LD_L, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          break;
      }
  } //end load/store single data item
  else if(!b15 && b14 && b13) { //load/store single data item //A6-10
      
      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
      } //end not always

      if(!b12) {
        if(!b11) { //STR (imm, Thumb) //A8-382 //Encoding T1
          uint32_t imm = IMM5<<2;
          if (imm) {
            CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, 0, (IMM5 << 2), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, RD, 0, 0, 0, 0, 0);
          }else{
            CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, RN, RD, 0, 0, 0, 0, 0);
          }
        } //end STR (imm, Thumb)
        else { //LDR (imm, Thumb) //A8-118 //Encoding T1
          uint32_t imm = IMM5<<2;
          if (imm) {
            CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, 0, (IMM5 << 2), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          }else{
            CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, RN, 0, 0, RD, 0, 0, 0);
          }
        } //end LDR (imm, Thumb)
      }
      else {
        if(!b11) { //STRB (imm, Thumb) //A8-288
          if (IMM5) {
            CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, 0, (IMM5), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iSALU_ST, OP_U08_ST, LREG_TMP1, RD, 0, 0, 0, 0, 0);
          }else{
            CrackInst::setup(rinst, iSALU_ST, OP_U08_ST, RN, RD, 0, 0, 0, 0, 0);
          }
        } //end STRB (imm, Thumb)
        else { //LDRB (imm, Thumb) //A8-126
          if (IMM5) {
            CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, 0, (IMM5), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iLALU_LD, OP_U08_LD, LREG_TMP1, 0, 0, RD, 0, 0, 0);
          }else{
            CrackInst::setup(rinst, iLALU_LD, OP_U08_LD, RN, 0, 0, RD, 0, 0, 0);
          }
        } //end LDRB (imm, Thumb)
      }
  } //end load/store single data item
  else if(b15 && !b14 && !b13) { //load/store single data item //A6-10

      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
        CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
      } //end not always

      if(!b12) {
        if(!b11) { //STRH (imm, Thumb) //A8-408
          CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RN, 0, (IMM5 << 1), LREG_TMP1, 0, 0, 0); //eka, 1 shift for IMM5
          CrackInst::setup(rinst, iSALU_ST, OP_U16_ST_L, LREG_TMP1, RD, 0, 0, 0, 0, 0);
        } //end STRH (imm, Thumb)
        else { //LDRH (imm, Thumb) //A8-150
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RN, 0, (IMM5 << 1), LREG_TMP1, 0, 0, 0); //eka, 1 shift for IMM5
          CrackInst::setup(rinst, iLALU_LD, OP_U16_LD_L, LREG_TMP1, 0, 0, RD, 0, 0, 0);
        } //end LDRH (imm, Thumb) //A8-150
      }
      else {
        if(!b11) { //Store Register SP relative (imm, Thumb) //A8-382 //Encoding T2
          CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, STACKPTR, 0, (IMM8 << 2), LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, RDN, 0, 0, 0, 0, 0);
        } //end Store Register SP relative
        else { //Load Register SP relative (imm, Thumb) //A8-118 //Encoding T2
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, STACKPTR, 0, (IMM8 << 2), LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, RDN, 0, 0, 0);
        } //end Load Register SP relative (imm, Thumb)
      }
  } //end load/store single data item
  else if(b15 && !b14 && b13 && !b12 && !b11) { //Generate PC-relative address see ADR //A8-32
    CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>((PC&0xFFFFFFFC)+4), LREG_TMP1, 0, 0, 0);

      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, (IMM8 << 2), LREG_TMP1, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
        CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RDN, 0, RDN, 0, 0, 0);  
      } //end not always
      else { //always
        CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, (IMM8 << 2), RDN, 0, 0, 0);
      } //end always
  } //end Generate PC-relative address
  else if(b15 && !b14 && b13 && !b12 && b11) { //Generate SP-relative address see ADD (SP plus imm) //A8-28 //Encoding T1

      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S32_ADD, STACKPTR, 0, (IMM7 << 2), LREG_TMP1, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
        CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RDN, 0, RDN, 0, 0, 0);  
      } //end not always
      else { //always
        CrackInst::setup(rinst, iAALU, OP_S32_ADD, STACKPTR, 0, (IMM7 << 2), RDN, 0, 0, 0);
      } //end always
  } //end Generate SP-relative address
  else if(b15 && !b14 && b13 && b12) { //Miscellaneous 16-bit instructions //A6-11

        if(cond != AL) { //not always
          if(!b11 && b10 && b9 && !b8 && !b7 && b6 && !b5) { //SETEND //A8-314
            OOPS("WARNING: Set Endianess (SETEND) is NOT IMPLEMENTED.\n");
          }
          else if(!b11 && b10 && b9 && !b8 && !b7 && b6 && b5) { //CPS //B6-3
            OOPS("WARNING: Change Processor State (CPS) is NOT IMPLEMENTED.\n");
          } 
          else if(!b11 && !b10 && !b9 && !b8 && !b7) { //ADD (SP + imm) //A8-28
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, STACKPTR, 0, (IMM7 << 2), STACKPTR, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, STACKPTR, 0, (IMM7 << 2), LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, STACKPTR, 0, STACKPTR, 0, 0, 0);  
          }  //end ADD (SP + imm)
          else if(!b11 && !b10 && !b9 && !b8 && b7) { //SUB (SP - imm) //A8-426
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, STACKPTR, 0, (IMM7 << 2), STACKPTR, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, STACKPTR, 0, (IMM7 << 2), LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XORICC, LREG_TMP1, 0, 0x2, LREG_TMP1, 0, 0, 0); //invert carry
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, STACKPTR, 0, STACKPTR, 0, 0, 0);  
          }  //end SUB (SP - imm)
          else if(!b10 && b8) { //Compare and Branch on Zero CBNZ, CBZ //A8-66 //ARMv6T2
            OOPS("WARNING: Compare and Branch on Zero/Non-Zero (CBNZ, CBZ) can NOT be in ITBLOCK.\n");
          }  //end Compare and Branch on Zero CBNZ, CBZ
          else if(!b11 && !b10 && b9 && !b8 && !b7 && !b6) { //SXTH //A8-444
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, 16, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 16, RD, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 16, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
          }  //end SXTH
          else if(!b11 && !b10 && b9 && !b8 && !b7 && b6) { //SXTB //A8-440
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, 24, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 24, RD, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 24, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
          }  //end SXTB
          else if(!b11 && !b10 && b9 && !b8 && b7 && !b6) { //UXTH //A8-524
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFFFF, RD, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFFFF, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
          }  //end UXTH
          else if(!b11 && !b10 && b9 && !b8 && b7 && b6) { //UXTB //A8-520
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFF, RD, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFF, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);  
          }  //end UXTB
          else if(!b11 && b10 && !b9) { //Push Multiple Registers PUSH //A8-248
            int      count;
            uint16_t reglist  = ((b8 << 14) | (insn & 0xFF));
            uint16_t reglist2 = reglist;
            bool     firstuse = false;

            uint8_t free_reg_list[] = {
              1, 2, 3, 
              4, 5, 6, 7, 
              8, 9, 10, 11, 
              12, 13, STACKPTR, LINK, PC_REG
            };

            for (count = 0; reglist2; count++)
              reglist2 &= reglist2 - 1; // clear the least significant bit set

            if(count < 1) //UNPREDICTABLE if reg_list < 1;
              return;

            if(cond != AL) {
              CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
              CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
            }

            CrackInst::setup(rinst, iAALU, OP_U32_SUB, STACKPTR, 0, (count*4), LREG_TMP1, 0, 0, 0); //start address

            for(int i = 0; i < 15; i++) {
              if(reglist & 1) {
                if(!firstuse) {
                  CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, LREG_TMP1, 0, 0, LREG_TMP1, 0, 0, 0); 
                  firstuse = true;
                }

                CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, free_reg_list[i], 0, 0, 0, 0, 0);
              }

  
              if((reglist != 0) && (reglist & 1))
                CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);

              reglist = reglist >> 1;
            }

            CrackInst::setup(rinst, iAALU, OP_U32_SUB, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address
          }  //end Push Multiple Registers PUSH
          else if(b11 && !b10 && b9 && !b8 && !b7 && !b6) { //REV //A8-272
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_ROTR, LREG_TMP1, 0, 16, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);
          }  //end REV
          else if(b11 && !b10 && b9 && !b8 && !b7 && b6) { //REV16 //A8-274
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, RD, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);
          }  //end REV16
          else if(b11 && !b10 && b9 && !b8 && b7 && b6) { //REVSH //A8-276
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, LREG_TMP1, 0, 16, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 16, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, cmoveArray[cond], LREG_TMP1, RD, 0, RD, 0, 0, 0);
          }  //end REVSH
          else if(b11 && b10 && !b9) { //Pop Multiple Registers POP //A8-246
            int      count;
            uint16_t reglist  = ((b8 << 15) | (insn & 0xFF));
            uint16_t reglist2 = reglist;
            bool first_use = true;

            uint8_t free_reg_list[] = {
              1, 2, 3, 
              4, 5, 6, 7, 
              8, 9, 10, 11, 
              12, 13, STACKPTR, LINK, PC_REG
            };

            for (count = 0; reglist2; count++)
              reglist2 &= reglist2 - 1; // clear the least significant bit set

            if(count < 1) //UNPREDICTABLE if reg_list < 1;
              return;

            if(cond != AL) {
              CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
              CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
            }

            for(int i = 0; i < 15; i++) {
              if(reglist & 1) {
                if(first_use)
                  CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, STACKPTR, 0, 0, free_reg_list[i], 0, 0, 0);
                else
                  CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, free_reg_list[i], 0, 0, 0);
              }


              if((reglist != 0) && (reglist & 1)) {
                if(first_use) {
                  first_use = false;
                  CrackInst::setup(rinst, iAALU, OP_U64_ADD, STACKPTR, 0, 4, LREG_TMP1, 0, 0, 0);
                }
                else
                  CrackInst::setup(rinst, iAALU, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);
              }

              reglist = reglist >> 1;
            }

            if(reglist & 1) { //check bit 15

              if(first_use) {
                CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, STACKPTR, 0, 0, LREG_TMP1, 0, 0, 0);
                first_use = false;
              }else {
                CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, LREG_TMP1, 0, 0, 0);
              }
              CrackInst::setup(rinst, iAALU, OP_U32_ADD, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address

              CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 1, LREG_TMP2, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP2, LREG_TTYPE, 0, LREG_TMP2, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP2, 0, ARM, LREG_TTYPE, 0, 0, 0);

              CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
              CrackInst::setup(rinst, iBALU_RET, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
            }
            else
              CrackInst::setup(rinst, iAALU, OP_U32_ADD, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address
          }  //end Pop Multiple Registers POP
          else if(b11 && b10 && b9 && !b8) { //BKPT //A8-56
            OOPS("WARNING: Breakpoint (BKPT) is NOT IMPLEMENTED.\n");
          } 
          else if(b11 && b10 && b9 && b8) { //If-then, and hints //A6-12
            uint8_t opA = (insn>>4) & 0xF;  
            uint8_t opB = insn & 0xF;  

            if(opB != 0) { //If-Then IT //A8-104 //ARMv6T2
              OOPS("WARNING: If-Then (IT) can NOT be in IT Block.\n");
            }
            else if((opA==0) && (opB==0)) { //NOP //A8-22 //ARMv6T2
              OOPS("WARNING: No Operation hint (NOP) is NOT IMPLEMENTED.\n");
            }
            else if((opA==1) && (opB==0)) { //YIELD //A8-812 //ARMv7
              OOPS("WARNING: Yield hint (YIELD) is NOT IMPLEMENTED.\n");
            }
            else if((opA==2) && (opB==0)) { //Wait for Event WFE //A8-808 //ARMv7
              OOPS("WARNING: Wait for Event hint (WFE) is NOT IMPLEMENTED.\n");
            }
            else if((opA==3) && (opB==0)) { //Wait for Interrupt WFI //A8-810 //ARMv7
              OOPS("WARNING: Wait for Interrupt hint (WFI) is NOT IMPLEMENTED.\n");
            }
            else if((opA==4) && (opB==0)) { //Send Event SEV //A8-316 //ARMv7
              OOPS("WARNING: Send Event hint (SEV) is NOT IMPLEMENTED.\n");
            }
            else {
              OOPS("WARNING: Unallocated hints. Executed as NOPs.\n");
            }
          } 
          else {
            OOPS("WARNING: Instruction is UNDEFINED in Miscellaneous 16-bit space.\n");
          } 
        } //end not always
        else { //always
          if(!b11 && b10 && b9 && !b8 && !b7 && b6 && !b5) { //SETEND //A8-314
            OOPS("WARNING: Set Endianess (SETEND) is NOT IMPLEMENTED.\n");
          }
          else if(!b11 && b10 && b9 && !b8 && !b7 && b6 && b5) { //CPS //B6-3
            OOPS("WARNING: Change Processor State (CPS) is NOT IMPLEMENTED.\n");
          } 
          else if(!b11 && !b10 && !b9 && !b8 && !b7) { //ADD (SP + imm) //A8-28
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, STACKPTR, 0, (IMM7 << 2), STACKPTR, 0, 0, 0);
          }  //end ADD (SP + imm)
          else if(!b11 && !b10 && !b9 && !b8 && b7) { //SUB (SP - imm) //A8-426
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, STACKPTR, 0, (IMM7 << 2), STACKPTR, 0, 0, 0);
          }  //end SUB (SP - imm)
          else if(!b10 && b8) { //Compare and Branch on Zero CBNZ, CBZ //A8-66 //ARMv6T2 //Can NOT be in ITBLOCK
            uint32_t IMM5 = ((b9 << 5) | ((insn>>3) & 0x1F)) << 1;
     
            CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4), LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, IMM5, LREG_TMP2, 0, 0, 0);
    
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RD, 0, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP1, LREG_TMP2, 0, LREG_TMP1, 0, 0, 0);
    
            if(!b11) //Zero
              CrackInst::setup(rinst, iBALU_RBRANCH, OP_U64_RBE, LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
            else //NonZero
              CrackInst::setup(rinst, iBALU_RBRANCH, OP_U64_RBNE, LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
          }  //end Compare and Branch on Zero CBNZ, CBZ
          else if(!b11 && !b10 && b9 && !b8 && !b7 && !b6) { //SXTH //A8-444
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, 16, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 16, RD, 0, 0, 0);  
          }  //end SXTH
          else if(!b11 && !b10 && b9 && !b8 && !b7 && b6) { //SXTB //A8-440
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RN, 0, 24, LREG_TMP1, 0, 0, 0);  
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 24, RD, 0, 0, 0);  
          }  //end SXTB
          else if(!b11 && !b10 && b9 && !b8 && b7 && !b6) { //UXTH //A8-524
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFFFF, RD, 0, 0, 0);  
          }  //end UXTH
          else if(!b11 && !b10 && b9 && !b8 && b7 && b6) { //UXTB //A8-520
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RN, 0, 0xFF, RD, 0, 0, 0);  
          }  //end UXTB
          else if(!b11 && b10 && !b9) { //Push Multiple Registers PUSH //A8-248
            int      count;
            uint16_t reglist  = ((b8 << 14) | (insn & 0xFF));
            uint16_t reglist2 = reglist;
            bool     firstuse = false;
    
            uint8_t free_reg_list[] = {
              1, 2, 3, 
              4, 5, 6, 7, 
              8, 9, 10, 11, 
              12, 13, STACKPTR, LINK, PC_REG
            };
    
            for (count = 0; reglist2; count++)
              reglist2 &= reglist2 - 1; // clear the least significant bit set

            if(count < 1) //UNPREDICTABLE if reg_list < 1;
              return;
    
            CrackInst::setup(rinst, iAALU, OP_U32_SUB, STACKPTR, 0, (count*4), LREG_TMP1, 0, 0, 0); //start address
    
            for(int i = 0; i < 15; i++) {
              if(reglist & 1) {
                if(!firstuse) {
                  CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, LREG_TMP1, 0, 0, LREG_TMP1, 0, 0, 0); 
                  firstuse = true;
                }

                CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, free_reg_list[i], 0, 0, 0, 0, 0);
              }
    
      
              if((reglist != 0) && (reglist & 1))
                CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);

              reglist = reglist >> 1;
            }
    
            CrackInst::setup(rinst, iAALU, OP_U32_SUB, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address
          }  //end Push Multiple Registers PUSH
          else if(b11 && !b10 && b9 && !b8 && !b7 && !b6) { //REV //A8-272
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_ROTR, LREG_TMP1, 0, 16, RD, 0, 0, 0);
          }  //end REV
          else if(b11 && !b10 && b9 && !b8 && !b7 && b6) { //REV16 //A8-274
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, RD, 0, 0, 0);
          }  //end REV16
          else if(b11 && !b10 && b9 && !b8 && b7 && b6) { //REVSH //A8-276
            CrackInst::setup(rinst, iAALU, OP_S16_ROTR, RN, 0, 8, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, LREG_TMP1, 0, 16, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, LREG_TMP1, 0, 16, RD, 0, 0, 0);
          }  //end REVSH
          else if(b11 && b10 && !b9) { //Pop Multiple Registers POP //A8-246
            int      count;
            uint16_t reglist  = ((b8 << 15) | (insn & 0xFF));
            uint16_t reglist2 = reglist;
            bool first_use = true;
    
            uint8_t free_reg_list[] = {
              1, 2, 3, 
              4, 5, 6, 7, 
              8, 9, 10, 11, 
              12, 13, STACKPTR, LINK, PC_REG
            };
    
            for (count = 0; reglist2; count++)
              reglist2 &= reglist2 - 1; // clear the least significant bit set
    
            if(count < 1) //UNPREDICTABLE if reg_list < 1;
              return;

            for(int i = 0; i < 15; i++) {
              if(reglist & 1) {
                if(first_use)
                  CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, STACKPTR, 0, 0, free_reg_list[i], 0, 0, 0);
                else
                  CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, free_reg_list[i], 0, 0, 0);
              }
    
      
              if((reglist != 0) && (reglist & 1)) {
                if(first_use){
                  CrackInst::setup(rinst, iAALU, OP_U64_ADD, STACKPTR, 0, 4, LREG_TMP1, 0, 0, 0);
                  first_use = false;
                }
                else
                  CrackInst::setup(rinst, iAALU, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);
              }

              reglist = reglist >> 1;
            }
    
            if(reglist & 1) { //check bit 15

              if(first_use) {
                CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, STACKPTR, 0, 0, LREG_TMP1, 0, 0, 0);
                first_use = false;
              }else {
                CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, LREG_TMP1, 0, 0, 0);
              }
              CrackInst::setup(rinst, iAALU, OP_U32_ADD, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address

              CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 1, LREG_TMP2, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_TMP2, LREG_TTYPE, 0, LREG_TMP2, 0, 0, 0);
              CrackInst::setup(rinst, iAALU, OP_U64_CMOV_NE, LREG_TMP2, 0, ARM, LREG_TTYPE, 0, 0, 0);
 
              CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
              CrackInst::setup(rinst, iBALU_RET, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
            }
            else
              CrackInst::setup(rinst, iAALU, OP_U32_ADD, STACKPTR, 0, (count*4), STACKPTR, 0, 0, 0); //end address
          }  //end Pop Multiple Registers POP
          else if(b11 && b10 && b9 && !b8) { //BKPT //A8-56
            OOPS("WARNING: Breakpoint (BKPT) is NOT IMPLEMENTED.\n");
          } 
          else if(b11 && b10 && b9 && b8) { //If-then, and hints //A6-12
            uint8_t opA = (insn>>4) & 0xF;  
            uint8_t opB = insn & 0xF;  

            if(opB != 0) { //If-Then IT //A8-104 //ARMv6T2
              uint8_t firstcond = (insn>>4) & 0xF;  
              uint8_t mask      = insn & 0xF;  
    
              if(mask == 8) {
                itblockVector->push_back(firstcond);
              }
              else if((mask == 4) || (mask == 12)) {
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 3) & 1));
                itblockVector->push_back(firstcond);
              }
              else if( mask == 2 || mask == 6 || mask == 10 || mask == 14 ) {
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 2) & 1));
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 3) & 1));
                itblockVector->push_back(firstcond);
              }
              else {
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 1) & 1));
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 2) & 1));
                itblockVector->push_back((firstcond & 0xE) ^ ((mask >> 3) & 1));
                itblockVector->push_back(firstcond);
              }
            }
            else if((opA==0) && (opB==0)) { //NOP //A8-22 //ARMv6T2
              OOPS("WARNING: No Operation hint (NOP) is NOT IMPLEMENTED.\n");
            }
            else if((opA==1) && (opB==0)) { //YIELD //A8-812 //ARMv7
              OOPS("WARNING: Yield hint (YIELD) is NOT IMPLEMENTED.\n");
            }
            else if((opA==2) && (opB==0)) { //Wait for Event WFE //A8-808 //ARMv7
              OOPS("WARNING: Wait for Event hint (WFE) is NOT IMPLEMENTED.\n");
            }
            else if((opA==3) && (opB==0)) { //Wait for Interrupt WFI //A8-810 //ARMv7
              OOPS("WARNING: Wait for Interrupt hint (WFI) is NOT IMPLEMENTED.\n");
            }
            else if((opA==4) && (opB==0)) { //Send Event SEV //A8-316 //ARMv7
              OOPS("WARNING: Send Event hint (SEV) is NOT IMPLEMENTED.\n");
            }
            else {
              OOPS("WARNING: Unallocated hints. Executed as NOPs.\n");
            }
          } 
          else {
            OOPS("WARNING: Instruction is UNDEFINED in Miscellaneous 16-bit space.\n");
          } 
        } //end always
  } //end Miscellaneous 16-bit instructions
  else if(b15 && b14 && !b13 && !b12 && !b11) { //Store multiple registers see STM/STMIA/STMEA //A8-374

      int     count;
      uint8_t reglist  = IMM8;
      uint8_t reglist2 = reglist;
      bool    firstuse = false;

      uint8_t free_reg_list[] = {
        1, 2, 3, 4, 5, 6, 7, 8
      };

      for (count = 0; reglist2; count++)
        reglist2 &= reglist2 - 1; // clear the least significant bit set

      if(reglist < 1) //UNPREDICTABLE if reg_list < 1;
        return;

        if(cond != AL) { //not always
          CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
          CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
        } //end not always

        for(int i = 0; i < 8; i++) {
          if(reglist & 1) {
            if(!firstuse) {
              CrackInst::setup(rinst, iSALU_ADDR, OP_U32_ADD, RDN, 0, 0, LREG_TMP1, 0, 0, 0); 
              firstuse = true;
            }

            CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_L, LREG_TMP1, free_reg_list[i], 0, 0, 0, 0, 0);
          }

          reglist = reglist >> 1;
  
          if((reglist != 0) && (reglist & 1))
            CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);
        }

        CrackInst::setup(rinst, iAALU, OP_U32_ADD, RDN, 0, (count*4), RDN, 0, 0, 0); //writeback
  } //end Store multiple registers
  else if(b15 && b14 && !b13 && !b12 && b11) { //Load multiple registers see LDM/LDMIA/LDMFD //A8-110

      int     count;
      uint8_t reglist  = IMM8;
      uint8_t reglist2 = reglist;
      bool    firstuse = false;

      uint8_t free_reg_list[] = {
        1, 2, 3, 4, 5, 6, 7, 8
      };

      for (count = 0; reglist2; count++)
        reglist2 &= reglist2 - 1; // clear the least significant bit set

      if(reglist  < 1) //UNPREDICTABLE if reg_list < 1;
        return;

        if(cond != AL) { //not always
          CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+2), LREG_TMP3, 0, 0, 0);
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP3, 0, LREG_TMP3, 0, 0, 0);
          CrackInst::setup(rinst, iBALU_RBRANCH, negrbranchArray[cond], LREG_TMP3, LREG_TMP3, 0, 0, 0, 0, 0);
        } //end not always

        for(int i = 0; i < 8; i++) {
          if(reglist & 1) {
            if(!firstuse) {
              CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, RDN, 0, 0, free_reg_list[i], 0, 0, 0);
            }
            else
              CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP1, 0, 0, free_reg_list[i], 0, 0, 0);
          }

          reglist = reglist >> 1;
  
          if((reglist != 0) && (reglist & 1)) {
            if(!firstuse) {
              CrackInst::setup(rinst, iAALU, OP_U64_ADD, RDN, 0, 4, LREG_TMP1, 0, 0, 0);
              firstuse = true;
            }
            else
              CrackInst::setup(rinst, iAALU, OP_U64_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0);
          }
        }

        if(!((IMM8>>(RDN-1)) & 1))
          CrackInst::setup(rinst, iAALU, OP_U32_ADD, RDN, 0, (count*4), RDN, 0, 0, 0); //writeback
  } //end Load multiple registers
  else if(b15 && b14 && !b13 && b12) { //Conditional Branch, and Supervisor Call //A6-13
    uint8_t opcode = (insn>>8) & 0xF;
 
    if((opcode != 14) && (opcode !=15)) { //Condition Branch B //A8-44 //Can NOT be in ITBLOCK //Encoding T1
      uint8_t cond   = (insn>>8) & 0xF;

      int32_t SIMM8 = IMM8<<1;
      SIMM8  = (( SIMM8<< 23) >> 23);

      CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4+SIMM8), LREG_TMP1, 0, 0, 0);
      CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0); 
      CrackInst::setup(rinst, iBALU_RBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
    } //end Condition Branch B
    else if(opcode == 14) { //Permanently UNDEFINED
      OOPS("WARNING: Instruction in this space is PERMANENTLY UNDEFINED.\n");
    }
    else if(opcode == 15) { //Supervisor Call SVC (previously SWI) //A8-430

        if(cond != AL) { //not always
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_SYSMEM, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 1, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 2, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 3, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 4, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 5, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0);
          CrackInst::setup(rinst, iSALU_ADDR, caddArray[cond], LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 6, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U32_STSC, LREG_SYSMEM, 8, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0, 0); 
        } //end not always
        else { //always
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_SYSMEM, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 1, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 2, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 3, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 4, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 5, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_U64_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U64_ST_L, LREG_TMP2, 6, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U32_STSC, LREG_SYSMEM, 8, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_L, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0, 0); 
        } //end always
    } //end Supervisor Call SVC (previously SWI)
  } //end Conditional Branch, and Supervisor Call
  else if(b15 && b14 && b13 && !b12 && !b11) { //Unconditional Branch see B //A8-44 //Encoding T2
    uint32_t SIMM11;

    if(b10)
      SIMM11 = (0xFFFFF << 12) | (IMM11 << 1); //SignExtended IMM8
    else 
      SIMM11 = (IMM11 << 1);

    CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC+4+(SIMM11)), LREG_TMP1, 0, 0, 0);

      if(cond != AL) { //not always
        CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
        CrackInst::setup(rinst, iAALU, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
      } //end not always
      else { //always 
        CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP1, 0, 0xFFFFFFFE, LREG_TMP1, 0, 0, 0); // clear bit 0
        CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0);
      } //end always
  } //end Unconditional Branch

  return;
}
