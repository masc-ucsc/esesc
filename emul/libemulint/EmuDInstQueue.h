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

#ifndef EMUDINSTQUEUE_H
#define EMUDINSTQUEUE_H

#include <vector>

#include "DInst.h"
#include "nanassert.h"

class EmuDInstQueue {
private:
  uint32_t head;
  uint32_t tail;
  uint32_t ndrop;
  uint32_t insertpoint;
  uint32_t nFreeElems;

  std::vector<DInst *> trace;

protected:
  void adjust_trace();

public:
  EmuDInstQueue();

  void popHead() {
    I(!empty());

    head = (head + 1) & (trace.size() - 1);
  };

  DInst *getHead() {
    I(!empty());

    return trace[head];
  };

  DInst *getTail() {
    I(!empty());

    return trace[tail];
  }

  DInst **getInsertPointRef() {
    return &trace[insertpoint];
  }

  void add() {
    I(nFreeElems);

    insertpoint = (insertpoint + 1) & (trace.size() - 1);
    nFreeElems--;

    if(nFreeElems == 0)
      adjust_trace();
  }

  void add(DInst *dinst) {
    trace[insertpoint] = dinst;
    add();
  }

  bool advanceTail() {
    if(ndrop) {
      ndrop--;
      return true;
    }
    if(insertpoint == tail)
      return false;

    //    I(tail!=head);
    tail = (tail + 1) & (trace.size() - 1);
    nFreeElems++;

    return true;
  }

  void moveHead2Tail() {
    uint32_t tail_copy = tail;
    while(head != tail) {
      trace[tail] = trace[tail]->clone();
      tail        = (tail + 1) & (trace.size() - 1);
      ndrop++;
    }
    head = tail_copy;
    tail = tail_copy;
  }

  uint32_t size() const {
    return (trace.size() - nFreeElems);
  }

  bool empty() const {
    return head == insertpoint;
  }
};

#endif
