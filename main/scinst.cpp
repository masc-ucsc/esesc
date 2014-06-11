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

#include "ARMCrack.h"
#include "ThumbCrack.h"

int main(int argc, char **argv)
{
  if (argc == 1) {
    printf("Usage:\n\t%s insn\n",argv[0]);
    exit(0);
  }

  CrackBase *crackARM   = new ARMCrack;
  ThumbCrack *crackThumb = new ThumbCrack;

  RAWDInst *rinst  = new RAWDInst;
  RAWInstType insn = strtoll(argv[1],0,16);

#ifdef SCOORE
  rinst->set(insn, 0xdeaddead);
#else
  rinst->set(insn, 0xdeaddead,0 , 0);
#endif
  crackARM->expand(rinst);

  printf("ARM   %x to %d\n", insn, rinst->getNumInst());
  for(size_t j=0;j<rinst->getNumInst();j++) {
    const Scuop *inst = rinst->getInstRef(j);
    inst->dump("");
    printf("\n");
  }

#ifdef SCOORE
  rinst->set(insn, 0xdeaddead);
#else
  rinst->set(insn, 0xdeaddead,0 , 0);
#endif
  crackThumb->thumb16expand(rinst);

  printf("THUMB16 %x to %d\n", insn, rinst->getNumInst());
  for(size_t j=0;j<rinst->getNumInst();j++) {
    const Scuop *inst = rinst->getInstRef(j);
    inst->dump("");
    printf("\n");
  }

#ifdef SCOORE
  rinst->set(insn, 0xdeaddead);
#else
  rinst->set(insn, 0xdeaddead,0 , 0);
#endif
  crackThumb->thumb32expand(rinst);

  printf("THUMB32 %x to %d\n", insn, rinst->getNumInst());
  for(size_t j=0;j<rinst->getNumInst();j++) {
    const Scuop *inst = rinst->getInstRef(j);
    inst->dump("");
    printf("\n");
  }

  return 0;
  
}

