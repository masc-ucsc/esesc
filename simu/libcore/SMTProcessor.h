// Contributed by Jose Renau
//                Basilio Fraguela
//                Milos Prvulovic
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

#ifndef SMTPROCESSOR_H
#define SMTPROCESSOR_H

#include <vector>
#include <queue>

#include "nanassert.h"

#include "GProcessor.h"
#include "Pipeline.h"
#include "FetchEngine.h"

class SMTProcessor:public GProcessor {
private:
  int32_t cFetchId;
  int32_t cDecodeId;
  int32_t cIssueId;

  const int32_t smtContexts;
  const int32_t smtFetchs4Clk;
  const int32_t smtDecodes4Clk;
  const int32_t smtIssues4Clk;
  const int32_t firstContext;

  DInst ***gRAT;

  class Fetch {
  public:
    Fetch(GMemorySystem *gm, CPU_t cpuID, GProcessor *gproc, FetchEngine *fe=0);
    ~Fetch();

    FetchEngine IFID;
    PipeQueue   pipeQ;
  };

  int32_t spaceInInstQueue;

  typedef std::vector<Fetch *> FetchContainer;
  FetchContainer flow;

  void selectFetchFlow();
  void selectDecodeFlow();
  void selectIssueFlow();

protected:
  // BEGIN VIRTUAL FUNCTIONS of GProcessor
  LSQ *getLSQ() { 
    I(0);
    return 0; 
  }

  void advanceClock();

  StallCause addInst(DInst *dinst);

  void fetch(FlowID fid);
  bool execute();
  void retire(){

  }

  // END VIRTUAL FUNCTIONS of GProcessor

public:
  SMTProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~SMTProcessor();

  bool isFlushing() {
    I(0);
    return false;
  }

  bool isReplayRecovering() {
    I(0);
    return false;
  }

  void replay(DInst *dinst) {
    I(0);
  }
  
  Time_t getReplayID()
  {
    I(0);
    return false;
  }

  
};

#endif   // SMTPROCESSOR_H
