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
