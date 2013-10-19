/*
   SESC: Super ESCalar simulator
   Copyright (C) 2006 University California, Santa Cruz.

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

#ifndef CRACKINST_H
#define CRACKINST_H

#include "RAWDInst.h"
#include "Instruction.h"
#include "InstOpcode.h"

class CrackInst {
private:
protected:
public:

  static void setup(RAWDInst *rinst, InstOpcode iop, Scopcode sop, uint8_t srcA,   
                       uint8_t srcB, uint32_t immA, uint8_t dstA,  
                       unsigned seticc, unsigned  setRND, unsigned getRND);

};

#endif
