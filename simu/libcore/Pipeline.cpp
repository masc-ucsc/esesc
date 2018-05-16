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

#include "Pipeline.h"
#include "SescConf.h"

IBucket::IBucket(size_t size, Pipeline *p, bool clean)
    : FastQueue<DInst *>(size)
    , cleanItem(clean)
    , pipeLine(p)
    , markFetchedCB(this) {
}

void IBucket::markFetched() {
  I(fetched == false);
  IS(fetched = true); // Only called once

  if(!empty()) {
    //    if (top()->getFlowId())
    //      MSG("@%lld: markFetched Bucket[%p]",(long long int)globalClock, this);
  }

  pipeLine->readyItem(this);
}

bool PipeIBucketLess::operator()(const IBucket *x, const IBucket *y) const {
  return x->getPipelineId() > y->getPipelineId();
}

Pipeline::Pipeline(size_t s, size_t fetch, int32_t maxReqs)
    : PipeLength(s)
    , bucketPoolMaxSize(s + 1 + maxReqs)
    , MaxIRequests(maxReqs)
    , nIRequests(maxReqs)
    , buffer(s + 1 + maxReqs)

{
  maxItemCntr = 0;
  minItemCntr = 0;

  bucketPool.reserve(bucketPoolMaxSize);
  I(bucketPool.empty());

  for(size_t i = 0; i < bucketPoolMaxSize; i++) {
    IBucket *ib = new IBucket(fetch + 1, this); // +1 instructions
    bucketPool.push_back(ib);
  }

  I(bucketPool.size() == bucketPoolMaxSize);
}

Pipeline::~Pipeline() {
  while(!bucketPool.empty()) {
    delete bucketPool.back();
    bucketPool.pop_back();
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

  nIRequests++;
  if(b->getPipelineId() != minItemCntr) {
    received.push(b);
    return;
  }

  // If the message is received in-order. Do not use the sorting
  // receive structure (remember that a cache can respond
  // out-of-order the memory requests)
  minItemCntr++;

  if(b->empty())
    doneItem(b);
  else
    buffer.push(b);

  clearItems(); // Try to insert on minItem reveiced (OoO) buckets
}

void Pipeline::clearItems() {
  while(!received.empty()) {
    IBucket *b = received.top();

    if(b->getPipelineId() != minItemCntr) {
      break;
    }

    received.pop();

    minItemCntr++;

    if(b->empty())
      doneItem(b);
    else
      buffer.push(b);
  }
}

void Pipeline::doneItem(IBucket *b) {
  I(b->getPipelineId() < minItemCntr);
  I(b->empty());
  b->clock = 0;

  bucketPool.push_back(b);
}

IBucket *Pipeline::nextItem() {
  while(1) {
    if(buffer.empty()) {
#ifdef DEBUG
      // It should not be possible to propagate more buckets
      clearItems();
      I(buffer.empty());
#endif
      return 0;
    }

    if(((buffer.top())->getClock() + PipeLength) > globalClock) {
#if 0
      fprintf(stderr,"1 @%lld Buffer[%p] .top.ID (%d) ->getClock(@%lld) to be issued after %d cycles\n"
          ,(long long int) globalClock
          ,buffer.top()
          ,(int) ((buffer.top())->top())->getID()
          ,(long long int)((buffer.top())->getClock())
          ,PipeLength
          );
#endif
      return 0;
    } else {
#if 0
      fprintf(stderr,"2 @%lld Buffer[%p] .top.ID (%d) ->getClock(@%lld) to be issued after %d cycles\n"
          ,(long long int) globalClock
          ,buffer.top()
          ,(int) ((buffer.top())->top())->getID()
          ,(long long int)((buffer.top())->getClock())
          ,PipeLength
          );
#endif
    }
    IBucket *b = buffer.top();
    buffer.pop();
    // fprintf(stderr,"@%lld: Popping Bucket[%p]\n",(long long int)globalClock ,b);
    I(!b->empty());
    I(!b->cleanItem);

    I(!b->empty());
    I(b->top() != 0);

    return b;
  }

  I(0);
}

PipeQueue::PipeQueue(CPU_t i)
    : pipeLine(SescConf->getInt("cpusimu", "decodeDelay", i) + SescConf->getInt("cpusimu", "renameDelay", i),
               SescConf->getInt("cpusimu", "fetchWidth", i), SescConf->getInt("cpusimu", "maxIRequests", i))
    , instQueue(SescConf->getInt("cpusimu", "instQueueSize", i)) {
  SescConf->isInt("cpusimu", "decodeDelay", i);
  SescConf->isBetween("cpusimu", "decodeDelay", 1, 64, i);

  SescConf->isInt("cpusimu", "renameDelay", i);
  SescConf->isBetween("cpusimu", "renameDelay", 1, 64, i);

  SescConf->isInt("cpusimu", "maxIRequests", i);
  SescConf->isBetween("cpusimu", "maxIRequests", 0, 32000, i);

  SescConf->isInt("cpusimu", "instQueueSize", i);
  SescConf->isBetween("cpusimu", "instQueueSize", SescConf->getInt("cpusimu", "fetchWidth", i), 32768, i);
}

PipeQueue::~PipeQueue() {
  // do nothing
}

IBucket *Pipeline::newItem() {

  if(nIRequests == 0 || bucketPool.empty())
    return 0;

  nIRequests--;

  IBucket *b = bucketPool.back();
  bucketPool.pop_back();

  b->setPipelineId(maxItemCntr);
  maxItemCntr++;

  IS(b->fetched = false);

  I(b->empty());
  return b;
}

bool Pipeline::hasOutstandingItems() const {
  // bucketPool.size() has lineal time O(n)
#if 0
  if (!buffer.empty()){
    MSG("Pipeline !buffer.empty()");
  }

  if (!received.empty()){
    MSG("Pipeline !received.empty()");
  }

  if (nIRequests < MaxIRequests){
    MSG("Pipeline nIRequests(%d) < MaxIRequests(%d)",nIRequests, MaxIRequests);
  }
#endif
  return !buffer.empty() || !received.empty() || nIRequests < MaxIRequests;
}
