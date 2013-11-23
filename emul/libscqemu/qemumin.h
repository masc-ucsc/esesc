#ifndef QEMUMIN_H
#define QEMUMIN_H
// Contributed by Jose Renau
//                Gabriel Southern
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

#include <stdint.h>
#include "scstate.h"

extern "C" {
  typedef struct QEMUArgs {
    int qargc;
    char **qargv;
    void * env; //handle to CPUState to be used in syscalls
  } QEMUArgs;
};
extern QEMUArgs *qargs;

typedef int32_t abi_long;

void start_qemu(int argc, char **argv);
void syscall_wrapper();

extern "C" {
  uint32_t esesc_iload(uint32_t addr);

  uint64_t esesc_ldu64(uint32_t addr);

  uint32_t esesc_ldu32(uint32_t addr);
  uint16_t esesc_ldu16(uint32_t addr);
   int16_t esesc_lds16(uint32_t addr);
  uint8_t  esesc_ldu08(uint32_t addr);
   int8_t  esesc_lds08(uint32_t addr);

   void esesc_st64(uint32_t addr, uint64_t data);
   void esesc_st32(uint32_t addr, uint32_t data);
   void esesc_st16(uint32_t addr, uint16_t data);
   void esesc_st08(uint32_t addr, uint8_t data);

}

extern uint32_t starting_pc;

#endif

