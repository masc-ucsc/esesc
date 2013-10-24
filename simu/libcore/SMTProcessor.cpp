/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic
                  Luis Ceze

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

#include "SMTProcessor.h"
#include "SescConf.h"


SMTProcessor::Fetch::Fetch(GMemorySystem *gm, CPU_t cpuID, GProcessor *gproc, FetchEngine *fe)
  : IFID(cpuID, gproc,gm, fe)
  ,pipeQ(cpuID)
{
}

SMTProcessor::Fetch::~Fetch()
{
}

SMTProcessor::SMTProcessor(GMemorySystem *gm, CPU_t Id)
  :GProcessor(gm,Id,SescConf->getInt("cpusimu", "smtContexts",Id))
  ,smtContexts(SescConf->getInt("cpusimu", "smtContexts",Id))
  ,smtFetchs4Clk(SescConf->getInt("cpusimu", "smtFetchs4Clk",Id))
  ,smtDecodes4Clk(SescConf->getInt("cpusimu", "smtDecodes4Clk",Id))
  ,smtIssues4Clk(SescConf->getInt("cpusimu", "smtIssues4Clk",Id))
  // FIXME: the first context Ids not valid if there is a mix of Processor and SMTs
  ,firstContext(Id*smtContexts)
  ,fetchDist("Processor(%d)_fetchDist", Id) // noFetch is on GProcessor
{
  SescConf->isInt("cpusimu", "smtContexts",Id);
  SescConf->isGT("cpusimu", "smtContexts", 1,Id);

  SescConf->isInt("cpusimu", "smtFetchs4Clk",Id);
  SescConf->isBetween("cpusimu", "smtFetchs4Clk", 1,smtContexts,Id);

  SescConf->isInt("cpusimu", "smtDecodes4Clk",Id);
  SescConf->isBetween("cpusimu", "smtDecodes4Clk", 1,smtContexts,Id);

  SescConf->isInt("cpusimu", "smtIssues4Clk",Id);
  SescConf->isBetween("cpusimu", "smtIssues4Clk", 1,smtContexts,Id);

  flow.resize(smtContexts);

  Fetch *f = new Fetch(gm, Id, this);
  flow[0] = f;
  gRAT = (DInst ***) malloc(sizeof(DInst ***) * smtContexts);

  for(int32_t i = 1; i < smtContexts; i++) {
    flow[i] = new Fetch(gm, Id, this, &(f->IFID));
  }

  for(int32_t i = 0; i < smtContexts; i++) {
    gRAT[i] = (DInst **) malloc(sizeof(DInst **) * LREG_MAX);
    bzero(gRAT[i],sizeof(DInst*)*LREG_MAX);
  }

  cFetchId =0;
  cDecodeId=0;
  cIssueId =0;

  spaceInInstQueue = InstQueueSize;
}

SMTProcessor::~SMTProcessor() {
  for(FetchContainer::iterator it = flow.begin();
      it != flow.end();
      it++) {
    delete *it;
  }
}

void SMTProcessor::selectFetchFlow() {
  cFetchId = (cFetchId+1) % smtContexts;

  cFetchId = -1;
}

void SMTProcessor::selectDecodeFlow()
{
  // ROUND-ROBIN POLICY
  cDecodeId = (cDecodeId+1) % smtContexts;
}

void SMTProcessor::selectIssueFlow()
{
  // ROUND-ROBIN POLICY
  cIssueId = (cIssueId+1) % smtContexts;
}

void SMTProcessor::fetch(FlowID fid) {

#if 0
  // OLD CODE (not good)
  for(int32_t i = 0; i < smtContexts && nFetched < FetchWidth; i++) {
    selectFetchFlow();
    if (cFetchId >=0) {
      I(BootLoader::isActive(cFetchID+firstContext));

      fetchMax = fetchMax > (FetchWidth - nFetched) 
  ? (FetchWidth - nFetched) : fetchMax;
      
      I(fetchMax > 0);

      IBucket *bucket = flow[cFetchId]->pipeQ.pipeLine.newItem();
      if( bucket ) {
  flow[cFetchId]->IFID.fetch(bucket, fetchMax);
  // readyItem will be called once the bucket is fetched
  nFetched += bucket->size();
  fetchDist.sample(cFetchId, bucket->size()); 
      } else {
  noFetch.inc();
      }
    }else{
      I(!BootLoader::isActive(0+firstContext));
    }
  }
#endif
}

bool SMTProcessor::execute() {
#if 0
  if (!busy)
    return false;

  clockTicks.inc();
  
  // Fetch Stage
  int32_t nFetched = 0;
  int32_t fetchMax = FetchWidth;

  // ID Stage (insert to instQueue)
  for(int32_t i=0;i<smtContexts && spaceInInstQueue >= FetchWidth ;i++) {
    selectDecodeFlow();
    
    IBucket *bucket = flow[cDecodeId]->pipeQ.pipeLine.nextItem();
    if( bucket ) {
      I(!bucket->empty());
      
      spaceInInstQueue -= bucket->size();
      
      flow[cDecodeId]->pipeQ.instQueue.push(bucket);
    }
  }

  // RENAME Stage
  int32_t totalIssuedInsts=0;
  for(int32_t i = 0; i < smtContexts && totalIssuedInsts < IssueWidth; i++) {
    selectIssueFlow();
    
    if( flow[cIssueId]->pipeQ.instQueue.empty() )
      continue;
    
    int32_t issuedInsts = issue(flow[cIssueId]->pipeQ);
    
    totalIssuedInsts += issuedInsts;
  }
  spaceInInstQueue += totalIssuedInsts;
  
  retire();
#endif

  return true;
}

StallCause SMTProcessor::addInst(DInst *dinst)  {
#if 0
  const Instruction *inst = dinst->getInst();

  DInst **RAT = gRAT[dinst->getContextId()-firstContext];

  if( InOrderCore ) {
    if(RAT[inst->getSrc1()] != 0 || RAT[inst->getSrc2()] != 0) {
      return SmallWinStall;
    }
  }

  StallCause sc = sharedAddInst(dinst);
  if (sc != NoStall)
    return sc;

  I(dinst->getResource() != 0); // Resource::schedule must set the resource field

  if(!dinst->isSrc2Ready()) {
    // It already has a src2 dep. It means that it is solved at
    // retirement (Memory consistency. coherence issues)
    if( RAT[inst->getSrc1()] )
      RAT[inst->getSrc1()]->addSrc1(dinst);
  }else{
    if( RAT[inst->getSrc1()] )
      RAT[inst->getSrc1()]->addSrc1(dinst);

    if( RAT[inst->getSrc2()] )
      RAT[inst->getSrc2()]->addSrc2(dinst);
  }

  dinst->setRATEntry(&RAT[inst->getDest()]);

  RAT[inst->getDest()] = dinst;

  I(dinst->getResource());
  dinst->getResource()->getCluster()->addInst(dinst);
#endif

  return NoStall;
}

