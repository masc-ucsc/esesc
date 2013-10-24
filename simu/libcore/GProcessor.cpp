/*
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Jose Renau
  Luis Ceze
  Karin Strauss
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

#include <sys/time.h>
#include <unistd.h>
#include "GProcessor.h"
#include "Report.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"

GStatsCntr *GProcessor::wallClock=0;
Time_t GProcessor::lastWallClock=0;

GProcessor::GProcessor(GMemorySystem *gm, CPU_t i, size_t numFlows)
  :cpu_id(i)
  ,MaxFlows(numFlows)
  ,FetchWidth(SescConf->getInt("cpusimu", "fetchWidth",i))
  ,IssueWidth(SescConf->getInt("cpusimu", "issueWidth",i))
  ,RetireWidth(SescConf->getInt("cpusimu", "retireWidth",i))
  ,RealisticWidth(RetireWidth < IssueWidth ? RetireWidth : IssueWidth)
  ,InstQueueSize(SescConf->getInt("cpusimu", "instQueueSize",i))
  ,MaxROBSize(SescConf->getInt("cpusimu", "robSize",i))
  ,memorySystem(gm)
  ,storeset(i)
  ,rROB(SescConf->getInt("cpusimu", "robSize", i))
  ,ROB(MaxROBSize)
  ,rrobUsed("P(%d)_rrobUsed", i) // avg
  ,robUsed("P(%d)_robUsed", i) // avg
  ,nReplayInst("P(%d)_nReplayInst", i)
  ,nReplayDist("P(%d)_nReplayDist", i)
  ,nCommitted("P(%d):nCommitted", i) // Should be the same as robUsed - replayed
  ,noFetch("P(%d):noFetch", i)
  ,noFetch2("P(%d):noFetch2", i)
  ,nFreeze("P(%d):nFreeze",i)
  ,clockTicks("P(%d):clockTicks",i)
{
  active = true;

  lastReplay = 0;
  if (wallClock ==0)
    wallClock = new GStatsCntr("OS:wallClock");

  if( SescConf->checkInt("cpusimu", "throtting" , i)) {
    throtting = SescConf->getInt("cpusimu", "throtting" , i);
  }else{
    throtting = 0;
  }
  throtting_cntr = 0;
  bool scooremem = false;
  if (SescConf->checkBool("cpusimu"    , "scooreMemory" , gm->getId()))
    scooremem = SescConf->getBool("cpusimu", "scooreMemory",gm->getId());

  if(scooremem) {
    if((!SescConf->checkCharPtr("cpusimu", "SL0", i))&&(!SescConf->checkCharPtr("cpusimu", "VPC", i))){
      printf("ERROR: scooreMemory requested but no SL0 or VPC specified\n");
      fflush(stdout);
      exit(15);
    }
    if((SescConf->checkCharPtr("cpusimu", "SL0", i))&&(SescConf->checkCharPtr("cpusimu", "VPC", i))){
      printf("ERROR: scooreMemory requested, cannot have BOTH SL0 and VPC specified\n");
      fflush(stdout);
      exit(15);
    }
  }


  SescConf->isInt("cpusimu"    , "issueWidth" , i);
  SescConf->isLT("cpusimu"     , "issueWidth" , 1025, i);

  SescConf->isInt("cpusimu"    , "retireWidth", i);
  SescConf->isBetween("cpusimu", "retireWidth", 0   , 32700 , i);

  SescConf->isInt("cpusimu"    , "robSize"    , i);
  SescConf->isBetween("cpusimu", "robSize"    , 2   , 262144, i);

  nStall[0] = 0 ; // crash if used
  nStall[SmallWinStall]      = new GStatsCntr("P(%d)_ExeEngine:nSmallWinStall",i);
  nStall[SmallROBStall]      = new GStatsCntr("P(%d)_ExeEngine:nSmallROBStall",i);
  nStall[SmallREGStall]      = new GStatsCntr("P(%d)_ExeEngine:nSmallREGStall",i);
  nStall[OutsLoadsStall]    = new GStatsCntr("P(%d)_ExeEngine:nOutsLoadsStall",i);
  nStall[OutsStoresStall]   = new GStatsCntr("P(%d)_ExeEngine:nOutsStoresStall",i);
  nStall[OutsBranchesStall] = new GStatsCntr("P(%d)_ExeEngine:nOutsBranchesStall",i);
  nStall[ReplaysStall]      = new GStatsCntr("P(%d)_ExeEngine:nReplaysStall",i);
  nStall[SyscallStall]      = new GStatsCntr("P(%d)_ExeEngine:nSyscallStall",i);

  I(ROB.size() == 0);

  eint = 0;

  buildInstStats(nInst, "ExeEngine");
}

GProcessor::~GProcessor() {
}


void GProcessor::buildInstStats(GStatsCntr *i[iMAX], const char *txt) {
  bzero(i, sizeof(GStatsCntr *) * iMAX);

  for(int32_t t = 0; t < iMAX; t++) {
    i[t] = new GStatsCntr("P(%d)_%s_%s:n", cpu_id, txt, Instruction::opcode2Name(static_cast<InstOpcode>(t)));
  }

  IN(forall((int32_t a=1;a<(int)iMAX;a++), i[a] != 0));
}

int32_t GProcessor::issue(PipeQueue &pipeQ) {
  int32_t i=0; // Instructions executed counter

  I(!pipeQ.instQueue.empty());

  do{
    IBucket *bucket = pipeQ.instQueue.top();
    do{
      I(!bucket->empty());
      if( i >= IssueWidth ) {
  return i;
      }

      I(!bucket->empty());

      DInst *dinst = bucket->top();
      //  GMSG(getId()==1,"push to pipe %p", bucket);

      StallCause c = addInst(dinst);
      if (c != NoStall) {
  if (i < RealisticWidth)
    nStall[c]->add(RealisticWidth - i, dinst->getStatsFlag());
  return i;
      }
      i++;

      bucket->pop();

    }while(!bucket->empty());

    pipeQ.pipeLine.doneItem(bucket);
    pipeQ.instQueue.pop();
  }while(!pipeQ.instQueue.empty());

  return i;
}


void GProcessor::retire(){

}
