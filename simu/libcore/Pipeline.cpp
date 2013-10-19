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

#include "SescConf.h"
#include "Pipeline.h"

#ifdef ESESC_FUZE
#include <iostream>
#endif

IBucket::IBucket(size_t size, Pipeline *p, bool clean)
  : FastQueue<DInst *>(size)
  ,cleanItem(clean)
  ,pipeLine(p)
  ,markFetchedCB(this) {
}

void IBucket::markFetched() {
  I(fetched == false);
  IS(fetched = true); // Only called once

  if (!empty()) {
//    if (top()->getFlowId())
//      MSG("markFetched %p",this);
  }

  pipeLine->readyItem(this);
}

bool PipeIBucketLess::operator()(const IBucket *x, const IBucket *y) const {
  return x->getPipelineId() > y->getPipelineId();
}

Pipeline::Pipeline(size_t s, size_t fetch, int32_t maxReqs)
  : PipeLength(s)
  ,bucketPoolMaxSize(s+1+maxReqs)
  ,MaxIRequests(maxReqs)
  ,nIRequests(maxReqs)
  ,buffer(s+1+maxReqs)
#ifdef ESESC_FUZE
  ,mixInstr       ("IanLee1521_x_mixInstr")
  ,numImmed       ("IanLee1521_x_numImmed")

  ,numUniqueSrcs  ("IanLee1521_x_numUniqueSrcs")
  ,numUniqueDsts  ("IanLee1521_x_numUniqueDsts")
  ,numUniqueInstr ("IanLee1521_x_numUniqueInstr")
  
  // B
  ,B_numSrcPost   ("IanLee1521_B_numSrcPost")
  ,B_numDstPost   ("IanLee1521_B_numDstPost")
  ,B_numInstPost  ("IanLee1521_B_numInstPost")
  ,B_numMegaOp    ("IanLee1521_B_numMegaOp")
    
  // N
  ,N_numSrcPost   ("IanLee1521_N_numSrcPost")
  ,N_numDstPost   ("IanLee1521_N_numDstPost")
  ,N_numInstPost  ("IanLee1521_N_numInstPost")
  ,N_numMegaOp    ("IanLee1521_N_numMegaOp")
  
  // Q
  ,Q_numSrcPost   ("IanLee1521_Q_numSrcPost")
  ,Q_numDstPost   ("IanLee1521_Q_numDstPost")
  ,Q_numInstPost  ("IanLee1521_Q_numInstPost")
  ,Q_numMegaOp    ("IanLee1521_Q_numMegaOp")
  

#endif
  {
  maxItemCntr = 0;
  minItemCntr = 0;

  bucketPool.reserve(bucketPoolMaxSize);
  I(bucketPool.empty());
  
  for(size_t i=0;i<bucketPoolMaxSize;i++) {
    IBucket *ib = new IBucket(fetch+1, this); // +1 instructions
    bucketPool.push_back(ib);

    ib = new IBucket(4, this, true);
    cleanBucketPool.push_back(ib);
  }

  I(bucketPool.size() == bucketPoolMaxSize);
}

Pipeline::~Pipeline() {
  while(!bucketPool.empty()) {
    delete bucketPool.back();
    bucketPool.pop_back();
  }
  while(!cleanBucketPool.empty()){
    delete cleanBucketPool.back();
    cleanBucketPool.pop_back();
  }
  while(!buffer.empty()) {
    delete buffer.top();
    buffer.pop();
  }
  while(!received.empty()) {
    delete received.top();
    received.pop();
  }
}

void Pipeline::readyItem(IBucket *b) {
  b->setClock();

#ifdef ESESC_FUZE
  if(b->size() > 0) {
    std::set<RegType> unique_srcs;
    std::set<RegType> unique_dsts;

    uint32_t numOp_ALU = 0;
    uint32_t numOp_Other = 0;


    for(unsigned int i = 0; i < b->size(); i++) {
      // FIXME IanLee1521 -- getData only works with FastQueue :: FASTQUEUE_USE_QUEUE = 0
      DInst* bucketInst = b->getData(b->getIdFromTop(i));
      MegaOp currMegaOp(*bucketInst);
      
      mixInstr.sample(currMegaOp.opcode);
      numImmed.sample(currMegaOp.numImm);

      mCluster.pendingMegaOps.push_back(currMegaOp);

      unique_srcs.insert (currMegaOp.srcs.begin(), currMegaOp.srcs.end());
      unique_dsts.insert (currMegaOp.dsts.begin(), currMegaOp.dsts.end());

      if (currMegaOp.opcode == iAALU) {
        ++numOp_ALU;
      } else {
        ++numOp_Other;
      }
    }

    for (std::set<RegType>::iterator iter = unique_srcs.begin(); iter != unique_srcs.end(); ++iter) {
      numUniqueSrcs.sample (*iter);
    }
    for (std::set<RegType>::iterator iter = unique_dsts.begin(); iter != unique_dsts.end(); ++iter) {
      numUniqueDsts.sample (*iter);
    }
    
    numUniqueInstr.sample (numOp_ALU ? numOp_Other + 1 : numOp_Other);

    mCluster.fuzion('B');
    {
      //mCluster.visualize("Fuze_B::");
      std::list<MegaOp> opList = mCluster.readyMegaOps;
      B_numInstPost.sample(opList.size());
      for(std::list<MegaOp>::iterator iter = opList.begin(); iter != opList.end(); iter++) {
        B_numSrcPost.sample(iter->countSrcs());
        B_numDstPost.sample(iter->countDsts());
        B_numMegaOp.sample(iter->countInstrs());
      }
    }

    mCluster.fuzion('N');
    {
      //mCluster.visualize("Fuze_N::");
      std::list<MegaOp> opList = mCluster.readyMegaOps;
      N_numInstPost.sample(opList.size());
      for(std::list<MegaOp>::iterator iter = opList.begin(); iter != opList.end(); iter++) {
        N_numSrcPost.sample(iter->countSrcs());
        N_numDstPost.sample(iter->countDsts());
        N_numMegaOp.sample(iter->countInstrs());
      }
    }

    mCluster.fuzion('Q');
    {
      //mCluster.visualize("Fuze_Q::");
      std::list<MegaOp> opList = mCluster.readyMegaOps;
      Q_numInstPost.sample(opList.size());
      for(std::list<MegaOp>::iterator iter = opList.begin(); iter != opList.end(); iter++) {
        Q_numSrcPost.sample(iter->countSrcs());
        Q_numDstPost.sample(iter->countDsts());
        Q_numMegaOp.sample(iter->countInstrs());
      }
    }
    
    //mCluster.fuzion('D');
    //mCluster.visualize("Fuze_D::");
    //std::list<MegaOp> opList = mCluster.readyMegaOps;
    //numInstPost_B.sample(opList.size());
    //for(std::list<MegaOp>::iterator iter = opList.begin(); iter != opList.end(); iter++) {
    //  numSrcPost_B.sample(iter->countSrcs());
    //  numDstPost_B.sample(iter->countDsts());
    //  numMegaOp_B.sample(iter->countInstrs());
    //}
    


/*
    while(!b->empty()) {
      b->pop();
    }

    for(std::list<MegaOp>::iterator iter = opList.begin(); iter != opList.end(); iter++) {
      DInst* newInst = 
    }      
*/
    mCluster.reset();
  }
#endif

  nIRequests++;
  if( b->getPipelineId() != minItemCntr ) {
    received.push(b);
    return;
  }

  // If the message is received in-order. Do not use the sorting
  // receive structure (remember that a cache can respond
  // out-of-order the memory requests)
  minItemCntr++;
  
  if( b->empty() )
    doneItem(b);
  else
    buffer.push(b);

  clearItems(); // Try to insert on minItem reveiced (OoO) buckets
}

void Pipeline::clearItems() {
  while( !received.empty() ) {
      IBucket *b = received.top(); 

    if(b->getPipelineId() != minItemCntr){
      break;
    }
     
    received.pop();

    minItemCntr++;
    
    if( b->empty() )
      doneItem(b);
    else
      buffer.push(b);
  }
}

void Pipeline::doneItem(IBucket *b) {
  I(b->getPipelineId() < minItemCntr);
  I(b->empty());
    
  bucketPool.push_back(b);
}
  


IBucket *Pipeline::nextItem() {
  while(1) {
    if (buffer.empty()) {
#ifdef DEBUG
      // It should not be possible to propagate more buckets
      clearItems();
      I(buffer.empty());
#endif
      return 0;
    }

    if( ((buffer.top())->getClock() + PipeLength) > globalClock )
      return 0;

    IBucket *b = buffer.top();
    buffer.pop();
    I(!b->empty());
    if (!b->cleanItem) {
      I(!b->empty());
      I(b->top() != 0);


      return b;
    }

    I(b->cleanItem);
    I(!b->empty());
    I(b->top() == 0);
    b->pop();
    I(b->empty());
    cleanBucketPool.push_back(b);
  }

  I(0);
}

PipeQueue::PipeQueue(CPU_t i)
  :pipeLine(
            SescConf->getInt("cpusimu", "decodeDelay",i)
            +SescConf->getInt("cpusimu", "renameDelay",i)
            ,SescConf->getInt("cpusimu", "fetchWidth",i)
            ,SescConf->getInt("cpusimu", "maxIRequests",i))
  ,instQueue(SescConf->getInt("cpusimu", "instQueueSize",i))
{
  SescConf->isInt("cpusimu", "decodeDelay", i);
  SescConf->isBetween("cpusimu", "decodeDelay", 1, 64,i);

  SescConf->isInt("cpusimu", "renameDelay", i);
  SescConf->isBetween("cpusimu", "renameDelay", 1, 64, i);

  SescConf->isInt("cpusimu", "maxIRequests",i);
  SescConf->isBetween("cpusimu", "maxIRequests", 0, 32000,i);
    
  SescConf->isInt("cpusimu", "instQueueSize",i);
  SescConf->isBetween("cpusimu", "instQueueSize"
                      ,SescConf->getInt("cpusimu","fetchWidth",i)
                      ,32768,i);

}

PipeQueue::~PipeQueue()
{
  // do nothing
}
