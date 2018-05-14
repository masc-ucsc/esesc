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

#ifndef TQUEUEMODULE_H
#define TQUEUEMODULE_H

#include <algorithm>
#include <vector>

#include <string.h>
#include <strings.h>

#include "nanassert.h"
#include "pool.h"

/*
 * The maximum time difference between tasks in the queue should be less than
 * MaxTimeDiff. Otherways, an additional slowdown would be suffered because
 * heaps would be used.
 *
 */

template <class Data, class Time> class TQueue {
public:
  class User {
  private:
    Time time; // when the instruction finishes
    Data next;
    enum QueueType { InNoQueue, InFastQueue, InTooFarQueue } qType;

  public:
    User() {
      qType = InNoQueue;
    };

    void removeFromQueue() {
      qType = InNoQueue;
    };
    bool isInQueue() const {
      return qType != InNoQueue;
    };

    void setInFastQueue() {
      qType = InFastQueue;
    };
    bool isInFastQueue() const {
      return qType == InFastQueue;
    };

    void setInTooFarQueue() {
      qType = InTooFarQueue;
    };
    bool isInTooFarQueue() const {
      return qType == InTooFarQueue;
    };

    void setTQTime(Time t) {
      time = t;
    };

    Time getTQTime() const {
      return time;
    };

    void setTQNext(Data n) {
      next = n;
    };
    Data getTQNext() const {
      return next;
    };
  };

private:
  Time    minTime;
  int32_t minPos;

  int32_t nNodes;

  const uint32_t AccessSize;
  const uint32_t AccessMask;

  Data *access;
  Data *accessTail;

  class DLess {
  public:
    bool operator()(const Data x, const Data y) const {
      return x->getTQTime() > y->getTQTime();
    };
  } dLess;

  std::vector<Data> tooFar;

  Time minTooFar;

  void adjustTooFar() {

    I(!tooFar.empty());

    while(((uint32_t)(tooFar.front()->getTQTime() - minTime)) < AccessSize) {

      addNode(tooFar.front(), tooFar.front()->getTQTime());

      std::pop_heap(tooFar.begin(), tooFar.end(), dLess);
      tooFar.pop_back();

      if(tooFar.empty()) {
        minTooFar = MaxTime;
        return;
      }
    }

    minTooFar = tooFar.front()->getTQTime();
  };

  void addNode(Data node, Time time) {
    I(time >= minTime);
    I((unsigned)abs((int)(time - minTime)) < AccessSize);

    uint32_t pos = ((uint32_t)(minPos + time - minTime)) & AccessMask;

    if(access[pos] == 0) {
      access[pos] = node;
    } else {
      accessTail[pos]->setTQNext(node);
    }
    accessTail[pos] = node;
    node->setInFastQueue();
    node->setTQNext(0);
    nNodes++;
  };

protected:
public:
  TQueue(uint32_t MaxTimeDiff);
  ~TQueue();

  void reset();

  void insert(Data data, Time time) {
    I(!data->isInQueue());
    I(time >= minTime);

    data->setTQTime(time);

    if((uint32_t)(time - minTime) < AccessSize) {
      addNode(data, time);
    } else {
      // Some nodes already exists, and this is too distant in
      // time.
      data->setInTooFarQueue();

      tooFar.push_back(data);
      std::push_heap(tooFar.begin(), tooFar.end(), dLess);

      if(minTooFar > time)
        minTooFar = time;
    }
  };

  Data nextJob(Time cTime) {
    if(likely(access[minPos] && minTime == cTime)) {
      /* Common case. Only for speed up reasons */
      Data node = access[minPos];
      nNodes--;
      access[minPos] = node->getTQNext();
      node->removeFromQueue();
      return node;
    }

    if(minTooFar <= cTime)
      adjustTooFar();

    if(nNodes == 0) {
      minTime = cTime;
      minPos  = 0;
      return 0;
    }

    Data node = access[minPos];

    while(node == 0 && minTime < cTime) {
      minPos = (minPos + 1) & AccessMask;
      minTime++;
      node = access[minPos];
    }

    if(node == 0)
      return 0;

    I(minTime <= cTime);

    I(nNodes);
    nNodes--;
    access[minPos] = node->getTQNext();
    node->removeFromQueue();

    return node;
  };

  void remove(Data node) {
    if(node->isInTooFarQueue()) {
      if(tooFar.front() == node) {
        std::pop_heap(tooFar.begin(), tooFar.end(), dLess);
        tooFar.pop_back();

        if(tooFar.empty())
          minTooFar = MaxTime;
        else
          minTooFar = tooFar.front()->getTQTime();

      } else {
        typedef typename std::vector<Data>::iterator DataIter;
        DataIter                                     it = std::find(tooFar.begin(), tooFar.end(), node);

        I(it != tooFar.end());
        tooFar.erase(it);
        I(std::find(tooFar.begin(), tooFar.end(), node) == tooFar.end());
        std::make_heap(tooFar.begin(), tooFar.end(), dLess);
      }
    } else if(node->isInFastQueue()) {
      Time time = node->getTQTime();

      if(access[minPos] == node) {
        // First in FastQueue
        nNodes--;
        access[minPos] = access[minPos]->getTQNext();
        node->removeFromQueue();
      } else {
        uint32_t pos = ((uint32_t)(minPos + time - minTime)) & AccessMask;

        Data prev = 0;
        Data curr = access[pos];

        while(curr != node) {
          prev = curr;
          curr = curr->getTQNext();
        }
        I(curr == node);

        if(prev == 0) {
          access[pos] = access[pos]->getTQNext();
        } else {
          prev->setTQNext(curr->getTQNext());
        }

        if(accessTail[pos] == node) {
          prev = access[pos];
          while(prev) {
            prev = prev->getTQNext();
          }
          accessTail[pos] = prev;
        }

        nNodes--;
      }
    } else {
      I(!node->isInQueue());
    }
  };

  void reschedule(Data node, Time rTime) {
    remove(node);

    I(!node->isInQueue());

    insert(node, rTime);
  };

  size_t size() const {
    return nNodes + tooFar.size();
  };
  bool empty() const {
    return nNodes == 0 && tooFar.empty();
  };

  void dump();
};

#define exportTemplate /* export not impl */
#ifndef TQUEUE_CPP
#include "TQueue.cpp"
#endif

#endif /* TQUEUEMODULE_H */
