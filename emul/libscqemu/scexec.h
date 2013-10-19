/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2010 University California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan K.Ardestani

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


#ifndef SCEXEC_H
#define SCEXEC_H

#include "CrackBase.h"
#include "ARMCrack.h"
#include "ThumbCrack.h"
#include "SPARCCrack.h"
#include "sccore.h"
#include <sys/time.h>
#include <iostream>

#define CRACK_CACHE_SIZE 1024

class Scexec {
private:
  Sccore core;

  ProgramBase *prog;

  CrackBase   *crackInst;
  ARMCrack    *crackInstARM;
  ThumbCrack  *crackInstThumb;
#ifdef SPARC32
  SPARCCrack  *crackInstSPARC;
#endif

  RAWDInst *rinst; // FIXME: do not use pointer. Inline

  FILE *fp; // File pointer to ARM Log file, used to validate ISA
  bool dmb_sy_flag; //data memory barrier instruction flag to keep track of
                    // dmb_sy, ldrex, strex and dmb_sy block of instructions
  bool ldrex_block; // flag to keep track of ldrex/strex block
  bool validate_insn; // flag to validate/not validate an instruction
  uint64_t vInst; // number of validated instructions

  RAWDInst crackCache[CRACK_CACHE_SIZE]; 
  TranslationType crackCacheTType[CRACK_CACHE_SIZE]; 
  uint64_t nInst;
  uint64_t nUops;
  uint64_t nHit;

  uint32_t tls2;
  FlowID fid;

  void fetch_crack();
  void execute();
  void validate();
  bool isDMBsyInsn(RAWInstType insn);
  bool isLDREXinsn(RAWInstType insn);

  bool firstIteration;

  uint32_t hash(uint32_t hval){
    uint32_t tmp = hval ^ (hval >> 5 );
    return tmp ;
  }
public:
  Scexec(FlowID _fid, void *syscall_mem, TranslationType ttype, ProgramBase *prog, FILE *fp, FILE *syscallTrace);

  void exec_loop(); 

  virtual ~Scexec(){
  }
};



#endif
