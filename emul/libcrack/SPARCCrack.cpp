/*
   SESC: Super ESCalar simulator
   Copyright (C) 2010 University California, Santa Cruz.

   Contributed by Jose Renau

This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "SPARCCrack.h"
#include "crack_scqemu.h"
#include <iostream>

#define SLIDING_WINDOW_ADDR 0xEFFFFF00U //dont want to sign-extend the address

#define OOPS(x) do { static int conta=0; if (conta<1) { printf(x); conta++; }; }while(0)

//Cracks SPARCv8 Instructions into SCOORE uOps.
void SPARCCrack::expand(RAWDInst *rinst)
{
  settransType(SPARC32);

#if defined(DEBUG)  && defined (ENABLE_SCQEMU)
    printf("SPARC inst: 0x%x\n", rinst->getInsn());
#endif

  uint32_t insn = rinst->getInsn();

  Scopcode rbranchArray[] = { OP_nop,               OP_U64_RBE,      OP_U64_RBLE,  
                              OP_U64_RBL,           OP_U64_RBC_OR_Z, OP_U64_RBCS,  
                              OP_U64_RBNEG,         OP_U64_RBVS,     OP_nop, 
                              OP_U64_RBNE,          OP_U64_RBG,      OP_U64_RBGE, 
                              OP_U64_RBC_OR_Z_NOT,  OP_U64_RBCC,     OP_U64_RBPOS,   
                              OP_U64_RBVC
                           };

  Scopcode fbranchArray[] = { OP_nop,       OP_U64_FBLGU,  OP_U64_FBLG, 
                              OP_U64_FBLU,  OP_U64_RBNEG,  OP_U64_FBUG, 
                              OP_U64_RBCS,  OP_U64_RBVS,   OP_U64_FBA,
                              OP_U64_RBE,   OP_U64_FBEU,   OP_U64_FBEG,
                              OP_U64_FBEGU, OP_U64_FBEL,   OP_U64_FBLUE,
                              OP_U64_FBLEG
                            };

  rinst->clearInst();

  uint8_t op                   = (insn >> 30) & 0x3;

  //CALL
  uint32_t DISP30              = (insn & 0x3FFFFFFF) << 2;
  
  //SETHI & BRANCHES
  int32_t  SIMM22              = (((insn & 0x3FFFFF) << 2) << 8) >> 8; //sign-extended
  uint32_t IMM22               = insn & 0x3FFFFF;
  uint8_t  op2                 = (insn>>22) & 0x7;
  uint8_t  cond                = (insn>>25) & 0xF;
  
  //Remaining Instructions
  uint8_t  ibit                = (insn>>13) & 0x1;
  uint8_t  op3                 = (insn>>19) & 0x3F;
  uint32_t SIMM13              = (insn & 0x1000) ? ((0xFFFFE << 12) | (insn & 0x1FFF)) : (insn & 0x1FFF);
  uint16_t opf                 = (insn>>5) & 0x1FF;
  uint8_t  RS2                 = insn & 0xF;
  
  uint8_t RS1                  = (insn>>14) & 0x1F;
  uint8_t RD                   = (insn>>25) & 0x1F;
  uint8_t RD1                  = RD + 1;

  const uint64_t PC             = rinst->getPC();

  switch(op) {
    case 0: //SETHI & BRANCHES
      switch(op2) {
        case 2: //Bicc
          if(cond == 0)  //branch never: NOP
            ;
          else if(cond == 8) //branch always: Crack handled. No uOP. Common for loops.
            ;
          else {
            CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC + SIMM22), LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iBALU_LBRANCH, rbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0);
          }
        break;
        case 4: //SETHI
          if(IMM22 == 0 || RD==0)
            ;  //NOP
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, 0, 0, static_cast<uint32_t>(IMM22<<10), RD, 0, 0, 0);
        break;
        case 6: //FBfcc
          if(cond == 0) //FBN
            ;
          else if(cond == 8)
            ;
          else {
            CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, static_cast<uint32_t>(PC + SIMM22), LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_FCC, LREG_TMP1, 0, LREG_TMP1, 0, 0, 0);
            CrackInst::setup(rinst, iBALU_LBRANCH, fbranchArray[cond], LREG_TMP1, LREG_TMP1, 0, 0, 0, 0, 0); 
          }
        break;
        default: //Not implemented
          break;
          //OOPS("ERROR: Branch Instructions with op2=%d is NOT IMPLEMENTED.\n",op2); 
      }
    break;
    case 1: //CALL
      CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, PC, 15, 0, 0, 0); 
      CrackInst::setup(rinst, iBALU_LCALL, OP_U64_JMP_IMM, 0, 0, static_cast<uint32_t>(PC+DISP30), 0, 0, 0, 0); 
    break;
    case 2: //Arithmetic, logical, shift, and remaining instructions
      switch(op3) {
        case 0:  //ADD
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 1: //AND
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 2: //OR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 3: //XOR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 4: //SUB
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 5: //ANDN
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 6: //ORN
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_ORN, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_ORN, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 7: //XNOR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XNOR, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XNOR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 8: //ADDX
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RS1, 0, LREG_TMP1, 0, 0, 0); 
          
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADDXX, LREG_TMP1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADDXX, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 10: //UMUL
          if(!ibit) {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_UMUL, RS1, RS2, 0, RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
          else {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_UMUL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
        break;
        case 11: //SMUL
          if(!ibit) {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RS1, RS2, 0, RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
          else {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
        break;
        case 12: //SUBX
          if(!ibit) {
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, RS2, 0, LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, RD, 0, 0, 0);
          }
          else {
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, RD, 0, 0, 0);
          }
        break;
        case 14: //UDIV
          CrackInst::setup(rinst, iAALU, OP_S64_SLL, LREG_Y, 0, 32, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_OR, LREG_TMP1, RS1, 0, LREG_TMP1, 0, 0, 0); 

          if(!ibit)
            CrackInst::setup(rinst, iCALU_DIV, OP_C_UDIV, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iCALU_DIV, OP_C_UDIV, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 15: //SDIV
          CrackInst::setup(rinst, iAALU, OP_S64_SLL, LREG_Y, 0, 32, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_OR, LREG_TMP1, RS1, 0, LREG_TMP1, 0, 0, 0); 

          if(!ibit)
            CrackInst::setup(rinst, iCALU_DIV, OP_C_SDIV, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iCALU_DIV, OP_C_SDIV, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 16: //ADDcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 17: //ANDcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_AND, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 18: //ORcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_OR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 19: //XORcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 20: //SUBcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 21: //ANDNcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_ANDN, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 22: //ORNcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_ORN, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_ORN, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 23: //XNORcc
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XNOR, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XNOR, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 24: //ADDXcc
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, LREG_ICC, RS1, 0, LREG_TMP1, 0, 0, 0); 
          
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADDXX, LREG_TMP1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADDXX, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
        break;
        case 26: //UMULcc
          if(!ibit) {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_UMUL, RS1, RS2, 0, RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
          else {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_UMUL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
          
          //FIXME: Is this right?
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, RD, 0, 0, LREG_ICC, 1, 0, 0); 
        break;
        case 27: //SMULcc
          if(!ibit) {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RS1, RS2, 0, RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
          else {
            CrackInst::setup(rinst, iCALU_MULT, OP_C_SMUL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_SRL, RD, 0, 32, LREG_Y, 0, 0, 0); 
          }
            
          //FIXME: Is this right?
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, RD, 0, 0, LREG_ICC, 1, 0, 0); 
        break;
        case 28: //SUBXcc
          if(!ibit) {
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, RS2, 0, LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, RD, 1, 0, 0);
          }
          else {
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
            CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_TMP2, 0, 2, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S08_SRL, LREG_TMP2, 0, 1, LREG_TMP2, 0, 0, 0);
            CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_TMP1, LREG_TMP2, 0, RD, 1, 0, 0);
          }
        break;
        case 30: //UDIVcc
          CrackInst::setup(rinst, iAALU, OP_S64_SLL, LREG_Y, 0, 32, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_OR, LREG_TMP1, RS1, 0, LREG_TMP1, 0, 0, 0); 
          
          if(!ibit)
            CrackInst::setup(rinst, iCALU_DIV, OP_C_UDIV, LREG_TMP1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iCALU_DIV, OP_C_UDIV, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            
          //FIXME: Is this right?
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, RD, 0, 0, LREG_ICC, 1, 0, 0); 
        break;
        case 31: //SDIVcc
          CrackInst::setup(rinst, iAALU, OP_S64_SLL, LREG_Y, 0, 32, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_OR, LREG_TMP1, RS1, 0, LREG_TMP1, 0, 0, 0); 
          
          if(!ibit)
            CrackInst::setup(rinst, iCALU_DIV, OP_C_SDIV, LREG_TMP1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iCALU_DIV, OP_C_SDIV, LREG_TMP1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
            
          //FIXME: Is this right?
          CrackInst::setup(rinst, iAALU, OP_S64_COPYICC, RD, 0, 0, LREG_ICC, 1, 0, 0); 
        break;
        case 32: //TADDcc //TADD SCOORE instruction does not exist anymore
          OOPS("WARNING: TADDcc is NOT IMPLEMENTED.\n");
/*
          if(!ibit)
            CrackInst::setup(rinst, iAALU, tadd, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, tadd, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
*/
        break; 
        case 33: //TSUBcc //TSUB SCOORE instruction does not exist anymore
          OOPS("WARNING: TSUBcc is NOT IMPLEMENTED.\n");
/*
          if(!ibit)
            CrackInst::setup(rinst, iAALU, tsub, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, tsub, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
*/
        break;
        case 34: //TADDccTV //TSUB SCOORE instruction does not exist anymore
          OOPS("WARNING: TADDccTV is NOT IMPLEMENTED.\n");
/*
          if(!ibit)
            CrackInst::setup(rinst, iAALU, taddtv, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, taddtv, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
*/
        break;
        case 35: //TSUBccTV //TSUBtv SCOORE instruction does not exist anymore
          OOPS("WARNING: TSUBccTV is NOT IMPLEMENTED.\n");
/*
          if(!ibit)
            CrackInst::setup(rinst, iAALU, tsubtv, RS1, RS2, 0, RD, 1, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, tsubtv, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 1, 0, 0); 
*/
        break;
        case 37: //SLL
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 38: //SRL
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_SRL, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_SRL, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 39: //SRA
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, RS1, RS2, 0, RD, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_SRA, RS1, 0, static_cast<uint32_t>(SIMM13), RD, 0, 0, 0); 
        break;
        case 40: //RDASR(rs1!=0), RDY(rs1=0), STBAR(rs1=15,rd=0)
          if(RS1 == 0) //RDY
            CrackInst::setup(rinst, iRALU, OP_iRALU_move, LREG_Y, 0, 0, RD, 0, 0, 0); 
          else if((RS1 == 15) && (RD == 0)) //STBAR - NOP
            ;
          else //RDASR
            OOPS("WARNING: RDASR is NOT IMPLEMENTED.\n");
        break;
        case 41: //RDPSR
          //CrackInst::setup(rinst, iAALU, OP_S64_JOINLREG_PSR, LREG_PSR, LREG_ICC, 0, RD, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_GETICC, LREG_ICC, 0, 0, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_SLL, LREG_TMP1, 0, 20, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_OR, LREG_CWP, LREG_TMP1, 0, RD, 0, 0, 0); 
        break;
        case 42: //RDWIM
          CrackInst::setup(rinst, iRALU, OP_iRALU_move, LREG_WIM, 0, 0, RD, 0, 0, 0); 
        break;
        case 43: //RDTBR
          CrackInst::setup(rinst, iRALU, OP_iRALU_move, LREG_TBR, 0, 0, RD, 0, 0, 0); 
        break;
        case 48: //WRASR(rd!=0), WRY(rd=0)
          if(RD == 0) { //WRY
            if(!ibit)
              CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
            else
              CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
            
            CrackInst::setup(rinst, iAALU, OP_S32_SLL, LREG_TMP1, 0, 32, LREG_Y, 0, 0, 0); 
          }
          else //WRASR
            OOPS("WARNING: WRASR is NOT IMPLEMENTED.\n");
        break;
        case 49: //WRLREG_PSR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, LREG_PSR, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_PSR, 0, 0, 0); 
            
          //get ICC from LREG_PSR and write it to ICC register
          CrackInst::setup(rinst, iAALU, OP_S32_SRL, LREG_PSR, 0, 20, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_AND, 0, 0, 0xF, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_PUTICC, LREG_TMP1, 0, 0, LREG_ICC, 1, 0, 0); 
        break;
        case 50: //WRWIM
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, LREG_WIM, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_WIM, 0, 0, 0); 
        break;
        case 51: //WRTBR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, RS2, 0, LREG_TBR, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S64_XOR, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TBR, 0, 0, 0); 
        break;
        case 52: //FPop1
          switch(opf) {
            case 1:  //FMOVs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FMOVS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 5:  //FNEGs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FNEGS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 9:  //FABSs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FABSS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 41:  //FSQRTs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSQRTS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 42:  //FSQRTd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSQRTD, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 43:  //FSQRTq
            break;
            case 65:  //FADDs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FADDS, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 66:  //FADDd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FADDD, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 67:  //FADDq
              OOPS("WARNING: FADDq is NOT IMPLEMENTED.\n");
            break;
            case 69:  //FSUBs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSUBS, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 70:  //FSUBd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSUBD, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 71:  //FSUBq
              OOPS("WARNING: FSUBq is NOT IMPLEMENTED.\n");
            break;
            case 73:  //FMULs
              CrackInst::setup(rinst, iCALU_FPMULT, OP_C_FMULS, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 74:  //FMULd
              CrackInst::setup(rinst, iCALU_FPMULT, OP_C_FMULD, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 75:  //FMULq
              OOPS("WARNING: FMULq is NOT IMPLEMENTED.\n");
            break;
            case 77:  //FDIVs
              CrackInst::setup(rinst, iCALU_FPDIV, OP_C_FDIVS, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 78:  //FDIVd
              CrackInst::setup(rinst, iCALU_FPDIV, OP_C_FDIVD, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 79:  //FDIVq
              OOPS("WARNING: FDIVq is NOT IMPLEMENTED.\n");
            break;
            case 105:  //FsMULd
              CrackInst::setup(rinst, iCALU_FPMULT, OP_C_FSMULD, RS1, RS2, 0, RD, 0, 0, 1); 
            break;
            case 110:  //FdMULq
              OOPS("WARNING: FdMULq is NOT IMPLEMENTED.\n");
            break;
            case 196:  //FiTOs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FITOS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 198:  //FdTOs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FDTOS, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 199:  //FqTOs
              OOPS("WARNING: FqTOs is NOT IMPLEMENTED.\n");
            break;
            case 200:  //FiTOd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FITOD, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 201:  //FsTOd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSTOD, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 203:  //FqTOd
              OOPS("WARNING: FqTOd is NOT IMPLEMENTED.\n");
            break;
            case 204:  //FiTOq
              OOPS("WARNING: FiTOq is NOT IMPLEMENTED.\n");
            break;
            case 205:  //FdTOq
              OOPS("WARNING: FdTOq is NOT IMPLEMENTED.\n");
            break;
            case 209:  //FsTOi
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSTOI, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 210:  //FdTOi
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FDTOI, RS2, 0, 0, RD, 0, 0, 1); 
            break;
            case 211:  //FqTOi
              OOPS("WARNING: FqTOi is NOT IMPLEMENTED.\n");
            break;
            default:
            break;
              //OOPS("ERROR: FPop1 with opf=%d is UNDEFINED.\n",opf);
          }
        break;
        case 53: //FPop2
          switch(opf) {
            case 81:  //FCMPs
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSUBS, RS1, RS2, 0, 0, 1, 0, 1); 
            break;
            case 82:  //FCMPd
              CrackInst::setup(rinst, iCALU_FPALU, OP_C_FSUBD, RS1, RS2, 0, 0, 1, 0, 1); 
            break;
            case 83:  //FCMPq
              OOPS("WARNING: FCMPq is NOT IMPLEMENTED.\n");
            break;
            case 85:  //FCMPEs
              OOPS("WARNING: FCMPEs is NOT IMPLEMENTED.\n");
            break;
            case 86:  //FCMPEd
              OOPS("WARNING: FCMPEd is NOT IMPLEMENTED.\n");
            break;
            case 87:  //FCMPEq
              OOPS("WARNING: FCMPEq is NOT IMPLEMENTED.\n");
            break;
            default:
            break;
              //OOPS("ERROR: FPop2 with opf=%d is UNDEFINED.\n",opf);
          }
        break;
        case 56: //JMPL
          if(RD != 0)
            CrackInst::setup(rinst, iAALU, OP_S64_COPYPCL, 0, 0, PC, RD, 0, 0, 0); 

          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
        
          if((RS1 == 31 || RS1 == 15) && RS2 == 0 && RD == 0)
            CrackInst::setup(rinst, iBALU_RET, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0); 
          else if(RD == 15)
            CrackInst::setup(rinst, iBALU_RCALL, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iBALU_RJUMP, OP_U64_JMP_REG, LREG_TMP1, 0, 0, 0, 0, 0, 0); 
        break;
        case 57: //RETT ?????
          OOPS("WARNING: RETT is NOT IMPLEMENTED.\n");
        break;
        case 58: //Ticc
          OOPS("WARNING: Ticc is NOT IMPLEMENTED.\n");
        break;
        case 59: //FLUSH
          ; //NOP
        break;
        case 60: //SAVE //FIXME
          OOPS("WARNING: SAVE is NOT IMPLEMENTED CORRECTLY.\n");

          if(RD >= 8 && RD <= 15) {
            RD = RD + 16;
          }
          else if(RD >= 24 && RD <= 31) {
            RD = RD - 16;
          }

          CrackInst::setup(rinst, iAALU, OP_S32_SUB, LREG_CWP, 0, 1, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, 0, 0, 1, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_SLL, LREG_TMP2, LREG_TMP1, 0, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_WIM, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, 0, 0, SLIDING_WINDOW_ADDR, LREG_TMP2, 0, 0, 0); 

          for(int i = 16; i < 24; i++) { //ST locals (R16-R23)
            CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP2, i, 0, 0, 0, 0, 0); 

            if(i != 23)
              CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          }

          for(int i = 8; i < 15; i++) //Move outs -> ins (R8-R15)
            CrackInst::setup(rinst, iRALU, OP_iRALU_move, i, 0, 0, i+16, 0, 0, 0); 
           
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, RD, 0, 0, 0); 
        break;
        case 61: //RESTORE //FIXME
          OOPS("WARNING: RESTORE is NOT IMPLEMENTED CORRECTLY.\n");

          if(RD >= 8 && RD <= 15) {
            RD = RD + 16;
          }
          else if(RD >= 24 && RD <= 31) {
            RD = RD - 16;
          }

          CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_CWP, 0, 1, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, 0, 0, 1, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_SLL, LREG_TMP2, LREG_TMP1, 0, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S64_AND, LREG_WIM, LREG_TMP2, 0, LREG_TMP2, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, 0, 0, SLIDING_WINDOW_ADDR, LREG_TMP2, 0, 0, 0); 

          for(int i = 16; i < 24; i++) { //LD locals (R16-R23)
            CrackInst::setup(rinst, iLALU_LD, OP_U32_LD_B, LREG_TMP2, 0, 0, i, 0, 0, 0); 

            if(i != 23)
              CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP2, 0, 4, LREG_TMP2, 0, 0, 0); 
          }

          for(int i = 8; i < 15; i++) //Move ins -> outs (R8-R15)
            CrackInst::setup(rinst, iRALU, OP_iRALU_move, i+16, 0, 0, i, 0, 0, 0); 
           
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, RD, 0, 0, 0); 
        break;
        default: //shouldn't happen or not implemented
        break;
          //OOPS("ERROR: Arithmetic Instruction with op3=%d is UNDEFINED.\n", op3);
      }
    break;
    case 3: //Memory Instructions
      switch(op3) {
        case 0: //LD
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 1: //LDUB
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_U08_LD, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 2: //LDUH
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_U16_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 3: //LDD
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD1, 0, 0, 0); 
        break;
        case 4: //ST
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP1, RD, 0, 0, 0, 0, 0); 
        break;
        case 5: //STB
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 

          CrackInst::setup(rinst, iSALU_ST, OP_U08_ST, LREG_TMP1, RD, 0, 0, 0, 0, 0); 
        break;
        case 6: //STH
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iSALU_ST, OP_U16_ST_B, LREG_TMP1, RD, 0, 0, 0, 0, 0); 
        break;
        case 7: //STD
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP1, RD, 0, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP1, RD1, 0, 0, 0, 0, 0); 
        break;
        case 9: //LDSB
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_S08_LD, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 10: //LDSH
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
            
          CrackInst::setup(rinst, iLALU_LD, OP_S16_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 13: //LDSTUB
          OOPS("WARNING: LDSTUB is NOT IMPLEMENTED.\n");
        break;
        case 15: //SWAP
          OOPS("WARNING: SWAP is NOT IMPLEMENTED.\n");
        break;
        case 32: //LDF
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
            
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
        break;
        case 33:  //LDFSR
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, LREG_FSR, 0, 0, 0);
        break;
        case 35: //LDDF
          if(!ibit)
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iAALU, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD, 0, 0, 0); 
          CrackInst::setup(rinst, iAALU, OP_S32_ADD, LREG_TMP1, 0, 4, LREG_TMP1, 0, 0, 0); 
          CrackInst::setup(rinst, iLALU_LD, OP_S32_LD_B, LREG_TMP1, 0, 0, RD1, 0, 0, 0); 
        break;
        case 36: //STF
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP1, RD, 0, 0, 0, 0, 0); 
        break;
        case 37: //STFSR
          if(!ibit)
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, RS2, 0, LREG_TMP1, 0, 0, 0); 
          else
            CrackInst::setup(rinst, iSALU_ADDR, OP_S32_ADD, RS1, 0, static_cast<uint32_t>(SIMM13), LREG_TMP1, 0, 0, 0); 
          
          //FIXME: CrackInst::setup(rinst, iCALU_FPALU, OP_S32_ADD, LREG_CEXC, FCC, 0, LREG_TMP2, LREG_FCC, 0, 0, 0, 0); 
          CrackInst::setup(rinst, iCALU_FPALU, OP_C_JOINFSR, LREG_FSR, LREG_TMP2, 0, LREG_FSR, 0, 1, 1); //FIXME
          CrackInst::setup(rinst, iSALU_ST, OP_U32_ST_B, LREG_TMP1, LREG_FSR, 0, 0, 0, 0, 0); 
        break;
        default: //not implemented or shouldn't happen
        break;
          //OOPS("ERROR: Memory Instruction with op3=%d is UNDEFINED.\n", op3); 
      }
    break;
    default: //shouldn't happen
      OOPS("ERROR: op not being decoded correctly\n"); 
  }
}
