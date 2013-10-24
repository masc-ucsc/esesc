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

  GStatsHist fetchDist;

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
