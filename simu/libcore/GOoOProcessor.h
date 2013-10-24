/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _GOOOPROCESSOR_H_
#define _GOOOPROCESSOR_H_

#include "nanassert.h"

#include "GProcessor.h"

class GOoOProcessor : public GProcessor {
private:

protected:
  // BEGIN VIRTUAL FUNCTIONS of GProcessor

  //  virtual void fetch(EmulInterface *eint, FlowID fid);
  //  virtual bool execute();
  //  virtual StallCause addInst(DInst *dinst);
  //  virtual void retire();

#ifdef SCOORE_CORE  
  virtual void set_StoreValueTable(AddrType addr, DataType value){ };
  virtual void set_StoreAddrTable(AddrType addr){ };
  virtual DataType get_StoreValueTable(AddrType addr){I(0); return 0; };
  virtual AddrType get_StoreAddrTable(AddrType addr){I(0); return 0; };
#endif
  
  // END VIRTUAL FUNCTIONS of GProcessor
public:
  GOoOProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~GOoOProcessor() { }
  //  virtual LSQ *getLSQ();

};

#endif   // SPROCESSOR_H
