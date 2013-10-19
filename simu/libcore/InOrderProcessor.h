/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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


#ifndef INORDERPROCESSOR_H_
#define INORDERPROCESSOR_H_

#include "nanassert.h"

#include "GProcessor.h"
#include "Pipeline.h"
#include "FetchEngine.h"
#include "LSQ.h"

class InOrderProcessor : public GProcessor {
private:
  FetchEngine IFID;
  PipeQueue   pipeQ;
  int32_t     spaceInInstQueue;

  LSQNone     lsq;
  bool        busy;

  DInst *RAT[LREG_MAX];

  FastQueue<DInst *> rROB; // ready/retiring/executed ROB

protected:
  ClusterManager clusterManager;
  // BEGIN VIRTUAL FUNCTIONS of GProcessor

  void fetch(FlowID fid);
  bool execute();
  void retire();

  StallCause addInst(DInst *dinst);
  // END VIRTUAL FUNCTIONS of GProcessor

public:
  InOrderProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~InOrderProcessor();

  LSQ *getLSQ() { return &lsq; }
  void replay(DInst *dinst);
  bool isFlushing() {
    I(0);
    return false;
  }
  bool isReplayRecovering() {
    I(0);
    return false;
  }
    Time_t getReplayID()
  {
    I(0);
    return false;
  }

#ifdef SCOORE_CORE  
  void set_StoreValueTable(AddrType addr, DataType value){ };
  void set_StoreAddrTable(AddrType addr){ };
  DataType get_StoreValueTable(AddrType addr){I(0); return 0; };
  AddrType get_StoreAddrTable(AddrType addr){I(0); return 0; };
#endif

};


#endif /* INORDERPROCESSOR_H_ */
