/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela

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
#ifndef PIPELINE_H
#define PIPELINE_H

#include <vector>
#include <set>
#include <queue>

#include "nanassert.h"
#include "FastQueue.h"

#include "DInst.h"

/*
 * This class simulates a pipeline. Besides this simple operation, it
 * also makes possible to receive the IData out-of-order, and it
 * orders so that the execution is correct. This is very useful for
 * the I-cache because it can answer out-of-order.
 *
 * The process follows:
 *  getItem();
 *  readyItem(); // when the fetch process has finished
 *  doneItem();  // when all the instructions are executed
 */

class Pipeline;

typedef uint32_t CPU_t;

class IBucket : public FastQueue<DInst *> {
private:
protected:
  const bool cleanItem;

  Time_t pipeId;
  Time_t clock;

  friend class Pipeline;
  friend class PipeIBucketLess;

  Pipeline *const pipeLine;
  ID(bool fetched;)

  Time_t getPipelineId() const { return pipeId; }
  void setPipelineId(Time_t i) {
    pipeId=i;
  }

  void markFetched();

  Time_t getClock() const { return clock; }
  void setClock() {
    clock = globalClock;
  }
 
public:
  IBucket(size_t size, Pipeline *p, bool clean=false);
  virtual ~IBucket() { }

  StaticCallbackMember0<IBucket, &IBucket::markFetched> markFetchedCB;
};

class PipeIBucketLess {
 public:
  bool operator()(const IBucket *x, const IBucket *y) const;
};

class Pipeline {
private:
  const size_t PipeLength;
  const size_t bucketPoolMaxSize;
  const int32_t MaxIRequests;
  int32_t nIRequests;
  FastQueue<IBucket *> buffer;

  typedef std::vector<IBucket *> IBucketCont;
  IBucketCont bucketPool;
  IBucketCont cleanBucketPool;

  std::priority_queue<IBucket *, std::vector<IBucket*>, PipeIBucketLess> received;

  Time_t maxItemCntr;
  Time_t minItemCntr;
  
protected:
  void clearItems();
public:
  Pipeline(size_t s, size_t fetch, int32_t maxReqs);
  virtual ~Pipeline();
 
  void cleanMark();

  IBucket *newItem() {
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

  bool hasOutstandingItems() const {
    // bucketPool.size() has lineal time O(n)
    return !buffer.empty() || !received.empty() || nIRequests < MaxIRequests;
  } 
  void readyItem(IBucket *b);
  void doneItem(IBucket *b);
  IBucket *nextItem();

  size_t size() const { return buffer.size(); }
};

class PipeQueue {
public:
  PipeQueue(CPU_t i);
  ~PipeQueue();

  Pipeline pipeLine;
  FastQueue<IBucket *> instQueue;

};


#endif // PIPELINE_H
