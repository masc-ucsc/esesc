#ifndef QEMUMIN_H
#define QEMUMIN_H
/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau
Gabriel Southern

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

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

