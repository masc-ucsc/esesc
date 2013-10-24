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

#include "ThumbCrack.h"
#include <iostream>

//Search for ARMv6T2 or ARMv6K or ARMv7 for instructions only in those versions and Higher
//The rest of the instructions are from ARMv6 and Higher

//Cracks Thumb instructions into SCOORE uOps.
void ThumbCrack::expand(RAWDInst *rinst)
{
  rinst->clearInst();

  uint32_t insn                = rinst->getInsn();
  uint8_t sixteenthirtytwo     = (insn>>11) & 0x1F;

  switch(sixteenthirtytwo) {
    case(29):
    case(30):
    case(31):
      thumb32expand(rinst); 
      settransType(THUMB32);
#if defined(DEBUG)  && defined (ENABLE_SCQEMU)
//    printf("Thumb32 inst: 0x%x\n", rinst->getInsn());
#endif
      break;
    default:
      thumb16expand(rinst);
      settransType(THUMB);
#if defined(DEBUG)  && defined (ENABLE_SCQEMU)
//    printf("Thumb16 inst: 0x%x\n", rinst->getInsn());
#endif
  }

  return;
} 

void ThumbCrack::advPC(){
  if (gettransType() == THUMB)
    pc += 2;
  else
    pc += 4;
} 
