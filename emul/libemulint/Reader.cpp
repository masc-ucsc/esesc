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

#include "Reader.h"
#include "Instruction.h"
#include "ThreadSafeFIFO.h"
#include "DInst.h"
#include "SescConf.h"


ThreadSafeFIFO<RAWDInst> *Reader::tsfifo = NULL;
EmuDInstQueue            *Reader::ruffer = NULL;
std::vector<GStatsCntr*>  Reader::rawInst;
std::vector<GStatsCntr*>  Reader::LD_global;
std::vector<GStatsCntr*>  Reader::LD_shared;

FlowID Reader::nemul = 0;

Reader::Reader(const char* section)
{
  if (tsfifo == NULL) {
    // Shared through all the objects, but sized with the # cores
    
    nemul =  SescConf->getRecordSize("","cpuemul");
    tsfifo = new ThreadSafeFIFO<RAWDInst>[nemul];
    ruffer = new EmuDInstQueue[nemul];
    rawInst.resize(nemul);

    for (size_t i=0;i<nemul;i++){
      rawInst[i] = new GStatsCntr("Reader(%d):rawInst",i);
    }

    LD_shared.resize(nemul);
    LD_global.resize(nemul);
    for (size_t i=0;i<nemul;i++){
      LD_shared[i] = new GStatsCntr("Reader(%d):LD_shared",i);
      LD_global[i] = new GStatsCntr("Reader(%d):LD_global",i);
    }
  }

}
