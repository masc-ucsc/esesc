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

#include "scexec.h"
#include "qemumin.h"
#include <sys/time.h>
#include "ahash.h"

#include "ProgramQEMU.h"

extern "C" void scfork(uint64_t fid, void * syscall_mem, TranslationType ttype, void *);

int main(int argc, char **argv)
{
  if (argc == 1) {
    printf("Usage:\n\t%s arm_binary args...\n",argv[0]);
    exit(0);
  }
  
  start_qemu(argc, argv);

  return 0;
}


extern "C" {
  void scfork(uint64_t fid, void * syscall_mem, TranslationType ttype, void * env){
    ProgramQEMU *prog = new ProgramQEMU(env);

    Scexec cpu(fid, syscall_mem, ttype, prog);

    cpu.exec_loop();
  }
}
