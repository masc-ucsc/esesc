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

#ifndef ARMCRACK_H
#define ARMCRACK_H

#include <string.h>

#include "RAWDInst.h"
#include "Instruction.h"
#include "crackinst.h"
#include "InstOpcode.h"
#include "CrackBase.h"


class ARMCrack : public CrackBase {
private:
protected:
public:
  void expand(RAWDInst *rinst);
  void advPC();
};

#endif
