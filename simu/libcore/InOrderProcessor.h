// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and 
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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

  void fetch(FlowID fid);
protected:
  ClusterManager clusterManager;
  // BEGIN VIRTUAL FUNCTIONS of GProcessor

  bool advance_clock(FlowID fid);
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
