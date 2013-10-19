/*
ESESC: Super ESCalar simulator
Copyright (C) 2009 University California, Santa Cruz.

Contributed by Jose Renau

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

#ifndef READER_H
#define READER_H

#include "Instruction.h"
#include "ThreadSafeFIFO.h"
#include "DInst.h"
#include "SescConf.h"
#include "EmuDInstQueue.h"
#include "GStats.h"

class Reader {
private:
protected:
  static FlowID nemul;
  static ThreadSafeFIFO<RAWDInst> *tsfifo;
  static EmuDInstQueue            *ruffer;
  static std::vector <GStatsCntr*>       rawInst;
  static std::vector <GStatsCntr*>       LD_global;
  static std::vector <GStatsCntr*>       LD_shared;
public:
  Reader(const char* section);
  virtual ~Reader() {
  };

  virtual DInst *executeHead(FlowID fid)   = 0;
  virtual void   reexecuteTail(FlowID fid) = 0;
  virtual void   syncHeadTail(FlowID fid)  = 0;

  FlowID getnemul() const { return nemul; }
};


#endif
