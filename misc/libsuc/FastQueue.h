/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

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

#ifndef FASTQUEUE_H
#define FASTQUEUE_H

#include "nanassert.h"

#include "Snippets.h"
#include <deque>

//#define FASTQUEUE_USE_QUEUE 1

#ifdef FASTQUEUE_USE_QUEUE
template <class Data> class FastQueue {
  std::deque<Data> dq;

public:
  FastQueue(int32_t size){};

  void push(Data d) {
    dq.push_back(d);
  };
  void pop() {
    dq.pop_front();
  };

  Data top() {
    return dq.front();
  };

  size_t size() const {
    return dq.size();
  }
  bool empty() const {
    return dq.empty();
  }
};
#else
template <class Data> class FastQueue {
private:
  Data *pipe;

  uint32_t pipeMask;
  uint32_t start;
  uint32_t end;
  uint32_t nElems;

protected:
public:
  FastQueue(size_t size) {
    // Find the closest power of two
    I(size);
    pipeMask = size;
    I(size < (256 * 1024)); // define FASTQUEUE_USE_QUEUE for those cases

    while(pipeMask & (pipeMask - 1))
      pipeMask++;

    pipe = (Data *)malloc(sizeof(Data) * pipeMask);

    pipeMask--;
    start  = 0;
    end    = 0;
    nElems = 0;
  }

  ~FastQueue() {
    free(pipe);
  }

  void push(Data d) {
    I(nElems <= pipeMask);

    //    pipe[(start+nElems) & pipeMask]=d;
    pipe[end] = d;
    I(end == ((start + nElems) & pipeMask));
    end = (end + 1) & pipeMask;
    nElems++;
  }

  Data top() const {
    // I(nElems);
    return pipe[start];
  }

  void pop() {
    nElems--;
    start = (start + 1) & pipeMask;
  }

  uint32_t getIDFromTop(uint32_t i) const {
    I(nElems > i);
    return (start + i) & pipeMask;
  }

  uint32_t getNextId(uint32_t id) const {
    return (id + 1) & pipeMask;
  }
  bool isEnd(uint32_t id) const {
    return id == end;
  }

  Data getData(uint32_t id) const {
    I(id <= pipeMask);
    I(id != end);
    return pipe[id];
  }

  Data topNext() const {
    return getData(getIDFromTop(1));
  }

  size_t size() const {
    return nElems;
  }
  bool empty() const {
    return nElems == 0;
  }
};
#endif // FASTQUEUE_USE_QUEUE

#endif // FASTQUEUE_H
