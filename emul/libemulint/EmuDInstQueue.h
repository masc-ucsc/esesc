/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
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

    head = (head + 1) & (trace.size()-1);
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

    insertpoint = (insertpoint + 1) & (trace.size()-1);
    nFreeElems--;

    if (nFreeElems == 0)
      adjust_trace();
  }

  void add(DInst *dinst) {
    trace[insertpoint] = dinst;
    add();
  }

  bool advanceTail() {
    if (ndrop) {
      ndrop--;
      return true;
    }
    if (insertpoint == tail)
      return false;

//    I(tail!=head);
    tail = (tail + 1) & (trace.size()-1);
    nFreeElems++;

    return true;
  }

  void moveHead2Tail() {
    uint32_t tail_copy = tail;
    while( head != tail ) {
      trace[tail] = trace[tail]->clone();
      tail        = (tail + 1) & (trace.size()-1);
      ndrop++;
    }
    head = tail_copy;
    tail = tail_copy;
  }

  uint32_t size() const {
    return (trace.size()-nFreeElems);
  }

  bool empty() const {
    return head == insertpoint;
  }
};

#endif

