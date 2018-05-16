// Contributed by Jose Renau
//                Basilio Fraguela
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

#ifndef PIPELINE_H
#define PIPELINE_H

#include <queue>
#include <set>
#include <vector>
//#include <boost/heap/priority_queue.hpp>

#include "FastQueue.h"
#include "nanassert.h"

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

typedef uint32_t CPU_t;
class IBucket;

class PipeIBucketLess {
public:
  bool operator()(const IBucket *x, const IBucket *y) const;
};

class Pipeline {
private:
  const size_t         PipeLength;
  const size_t         bucketPoolMaxSize;
  const int32_t        MaxIRequests;
  int32_t              nIRequests;
  FastQueue<IBucket *> buffer;

  typedef std::vector<IBucket *> IBucketCont;
  IBucketCont                    bucketPool;

  // typedef boost::heap::priority_queue<IBucket *,boost::heap::compare<PipeIBucketLess> > ReceivedType;
  typedef std::priority_queue<IBucket *, std::vector<IBucket *>, PipeIBucketLess> ReceivedType;
  // std::priority_queue<IBucket *, std::vector<IBucket*>, PipeIBucketLess> received;
  ReceivedType received;

  Time_t maxItemCntr;
  Time_t minItemCntr;

protected:
  void clearItems();

public:
  Pipeline(size_t s, size_t fetch, int32_t maxReqs);
  virtual ~Pipeline();

  void cleanMark();

  IBucket *newItem();
  bool     hasOutstandingItems() const;
  void     readyItem(IBucket *b);
  void     doneItem(IBucket *b);
  IBucket *nextItem();

  size_t size() const {
    return buffer.size();
  }
};

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

  Time_t getPipelineId() const {
    return pipeId;
  }
  void setPipelineId(Time_t i) {
    pipeId = i;
  }

  void markFetched();

  Time_t getClock() const {
    return clock;
  }
  void setClock() {
    clock = globalClock;
  }

public:
  IBucket(size_t size, Pipeline *p, bool clean = false);
  virtual ~IBucket() {
  }

  StaticCallbackMember0<IBucket, &IBucket::markFetched> markFetchedCB;
};

class PipeQueue {
public:
  PipeQueue(CPU_t i);
  ~PipeQueue();

  Pipeline             pipeLine;
  FastQueue<IBucket *> instQueue;
};

#endif // PIPELINE_H
