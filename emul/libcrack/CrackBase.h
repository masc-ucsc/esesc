/*
   SESC: Super ESCalar simulator
   Copyright (C) 2006 University California, Santa Cruz.

   Contributed by   
   Ehsan K.Ardestani
   Jose Renau

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

#ifndef CRACKBASE_H
#define CRACKBASE_H

#include <string.h>

#include "RAWDInst.h"
#include "Instruction.h"
#include "InstOpcode.h"


class CrackBase  {
private:
protected:
  uint64_t pc;
  TranslationType transType;
public:

  virtual void expand(RAWDInst *rinst) = 0;
  virtual void advPC() = 0;

  void advPC(AddrType new_pc) { 
    pc = new_pc; 
  };

  void settransType (TranslationType tt) { transType=tt; };
  TranslationType gettransType () const { return transType; };

  void setPC(uint64_t _pc) { pc = _pc; }
  uint64_t getPC() const { return pc; }

  void dumpPC(uint32_t fid) const ;


};

#endif
