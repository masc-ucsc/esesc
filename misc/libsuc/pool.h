/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Code based on Jose Martinez pool code (Thanks)

   Contributed by Jose Renau
                  Milos Prvulovic
                  James Tuck

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

#ifndef _POOL_H
#define _POOL_H

#include <pthread.h>
#include <string.h>
#include <strings.h>

#include "nanassert.h"
// Recycle memory allocated from time to time. This is useful for adapting to
// the phases of the application

#include "Snippets.h"

#ifdef DEBUG
//#define POOL_TIMEOUT 1
#define POOL_SIZE_CHECK 1
//#define CLEAR_ON_INSERT 1
#endif

#ifdef POOL_TIMEOUT
#define POOL_CHECK_CYCLE 12000
#endif

template <class Ttype, class Parameter1, bool noTimeCheck = false> class pool1 {
protected:
  class Holder : public Ttype {
  public:
    Holder(Parameter1 p1)
        : Ttype(p1) {
    }
    Holder *holderNext;
#ifdef DEBUG
    Holder *allNext; // List of Holders when active
#endif
#ifdef POOL_TIMEOUT
    Time_t outCycle; // Only valid if inPool is false
#endif
    ID(bool inPool;)
  };

#ifdef POOL_SIZE_CHECK
  unsigned long psize;
  unsigned long warn_psize;
#endif

  ID(bool deleted;)

#ifdef POOL_TIMEOUT
  Time_t need2cycle;
#endif
#ifdef DEBUG
  Holder *  allFirst; // List of Holders when active
  pthread_t thid;
#endif

  const int32_t Size; // Reproduction size

  Parameter1 p1;

  Holder *first; // List of free nodes

  void reproduce() {
    I(first == 0);

    for(int32_t i = 0; i < Size; i++) {
      Holder *h     = ::new Holder(p1);
      h->holderNext = first;
      IS(h->inPool = true);
#ifdef DEBUG
      h->allNext = allFirst;
      allFirst   = h;
#endif
      first = h;
    }
  }

public:
  pool1(Parameter1 a1, int32_t s = 32)
      : Size(s)
      , p1(a1) {
    I(Size > 0);
    IS(deleted = false);

#ifdef POOL_SIZE_CHECK
    psize      = 0;
    warn_psize = s * 8;
#endif

    first = 0;

#ifdef POOL_TIMEOUT
    need2cycle = globalClock + POOL_CHECK_CYCLE;
#endif
#ifdef DEBUG
    allFirst = 0;
    thid     = 0;
#endif

    if(first == 0)
      reproduce();
  }

  ~pool1() {
    // The last pool whould delete all the crap
    while(first) {
      Holder *h = first;
      first     = first->holderNext;
      ::delete h;
    }
    first = 0;
    IS(deleted = true);
  }

  void doChecks() {
#ifdef POOL_TIMEOUT
    if(noTimeCheck)
      return;
    if(need2cycle < globalClock) {
      Holder *tmp = allFirst;
      while(tmp) {
        GI(!tmp->inPool, (tmp->outCycle + POOL_CHECK_CYCLE) > need2cycle);
        tmp = tmp->allNext;
      }
      need2cycle = globalClock + POOL_CHECK_CYCLE;
    }
#endif // POOL_TIMEOUT
  }

  void in(Ttype *data) {
#ifdef DEBUG
    if(thid == 0)
      thid = pthread_self();
    I(thid == pthread_self());
#endif
    I(!deleted);
    Holder *h = static_cast<Holder *>(data);

    I(!h->inPool);
    IS(h->inPool = true);

    h->holderNext = first;
    first         = h;

#ifdef POOL_SIZE_CHECK
    psize--;
#endif

    doChecks();
  }

  Ttype *out() {
#ifdef DEBUG
    if(thid == 0)
      thid = pthread_self();
    I(thid == pthread_self());
#endif
    I(!deleted);
    I(first);

    I(first->inPool);
    IS(first->inPool = false);
#ifdef POOL_TIMEOUT
    first->outCycle = globalClock;
    doChecks();
#endif

#ifdef POOL_SIZE_CHECK
    psize++;
    if(psize >= warn_psize) {
      I(0);
      MSG("Pool1 class size grew to %lu", psize);
      warn_psize = 4 * psize;
    }
#endif

    Ttype *h = static_cast<Ttype *>(first);
    first    = first->holderNext;
    if(first == 0)
      reproduce();

    return h;
  }
};

//*********************************************

template <class Ttype> class tspool {
protected:
  class Holder : public Ttype {
  public:
    volatile Holder *holderNext;
    ID(bool inPool;)
  };

  volatile Holder *first; // List of free nodes

  const int32_t Size; // Reproduction size
  const char *  Name;

  ID(bool deleted;)

  void insert(Holder *h) {
    // Equivalent to
    //  h->holderNext = first;
    //  first         = h;

    IS(h->inPool = true);
    volatile Holder *old_first;
    volatile Holder *c_first;
    do {
      c_first       = first;
      h->holderNext = c_first;
      old_first     = AtomicCompareSwap(&first, c_first, h);
    } while(old_first != c_first);
  };

  void reproduce() {
    I(first == 0);

    for(int32_t i = 0; i < Size; i++) {
      Holder *h = ::new Holder;
      insert(h);
    }
  };

public:
  tspool(int32_t s = 32, const char *n = "pool name not declared")
      : Size(s)
      , Name(n) {
    I(Size > 0);
    IS(deleted = false);

    first = 0;

    reproduce();
  };

  ~tspool() {
    // The last pool should delete all the crap
    while(first) {
      volatile Holder *h = first;
      first              = first->holderNext;
      ::delete h;
    }
    first = 0;
    IS(deleted = true);
  }

  void in(Ttype *data) {
    I(!deleted);
    Holder *h = static_cast<Holder *>(data);

    I(!h->inPool);

    insert(h);
  }

  Ttype *out() {
    I(!deleted);
    I(first);

    volatile Holder *old_first;
    volatile Holder *c_first;
    do {
      c_first   = first;
      old_first = AtomicCompareSwap(&first, c_first, c_first->holderNext);
    } while(old_first != c_first);

    if(first == 0)
      reproduce();

    Ttype *h = (Ttype *)(c_first);
    I(c_first->inPool);
    IS(c_first->inPool = false);
    return h;
  }
};

template <class Ttype, bool noTimeCheck = false> class pool {
protected:
  class Holder : public Ttype {
  public:
    Holder *holderNext;
#ifdef DEBUG
    Holder *  allNext; // List of Holders when active
    pthread_t thid;
#endif
#ifdef POOL_TIMEOUT
    Time_t outCycle; // Only valid if inPool is false
#endif
    // ID(bool inPool;)
    bool inPool;
  };

#ifdef POOL_SIZE_CHECK
  unsigned long psize;
  unsigned long warn_psize;
#endif

  ID(bool deleted;)

#ifdef POOL_TIMEOUT
  Time_t need2cycle;
#endif
#ifdef DEBUG
  Holder *  allFirst; // List of Holders when active
  pthread_t thid;
#endif

  const int32_t Size; // Reproduction size
  const char *  Name;

  Holder *first; // List of free nodes

  void reproduce() {
    I(first == 0);

    for(int32_t i = 0; i < Size; i++) {
      Holder *h = ::new Holder;
#ifdef CLEAR_ON_INSERT
      bzero(h, sizeof(Holder));
#endif
      h->holderNext = first;
      IS(h->inPool = true);
#ifdef DEBUG
      h->allNext = allFirst;
      allFirst   = h;
#endif
      first = h;
    }
  }

public:
  pool(int32_t s = 32, const char *n = "pool name not declared")
      : Size(s)
      , Name(n) {
    I(Size > 0);
    IS(deleted = false);

#ifdef POOL_SIZE_CHECK
    psize      = 0;
    warn_psize = s * 8;
#endif

    first = 0;

#ifdef POOL_TIMEOUT
    need2cycle = globalClock + POOL_CHECK_CYCLE;
#endif
#ifdef DEBUG
    allFirst = 0;
    thid     = 0;
#endif

    if(first == 0)
      reproduce();
  }

  ~pool() {
    // The last pool whould delete all the crap
#if 0
    while(first) {
      Holder *h = first;
      first = first->holderNext;
      ::delete h;
    }
    first = 0;
    IS(deleted=true);
#endif
  }

  void doChecks() {
#ifdef POOL_TIMEOUT
    if(noTimeCheck)
      return;
    if(need2cycle < globalClock) {
      Holder *tmp = allFirst;
      while(tmp) {
        GI(!tmp->inPool, (tmp->outCycle + POOL_CHECK_CYCLE) > need2cycle);
        tmp = tmp->allNext;
      }
      need2cycle = globalClock + POOL_CHECK_CYCLE;
    }
#endif // POOL_TIMEOUT
  }

#ifdef DEBUG
  Ttype *nextInUse(Ttype *current) {
    Holder *tmp = static_cast<Holder *>(current);
    tmp         = tmp->allNext;
    while(tmp) {
      if(!tmp->inPool)
        return static_cast<Ttype *>(tmp);
      tmp = tmp->allNext;
    }
    return 0;
  }
  Ttype *firstInUse() {
    return nextInUse(static_cast<Ttype *>(allFirst));
  }
#endif

  void in(Ttype *data) {
#ifdef DEBUG
    if(thid == 0)
      thid = pthread_self();
    I(thid == pthread_self());
#endif
    I(!deleted);
    Holder *h = static_cast<Holder *>(data);

    I(!h->inPool);
#ifdef CLEAR_ON_INSERT
    bzero(data, sizeof(Ttype));
#endif
    IS(h->inPool = true);

    h->holderNext = first;
    first         = h;

#ifdef POOL_SIZE_CHECK
    psize--;
#endif

    doChecks();
  }

  Ttype *out() {
#ifdef DEBUG
    if(thid == 0)
      thid = pthread_self();
    I(thid == pthread_self());
#endif
    I(!deleted);
    I(first);

    I(first->inPool);
    IS(first->inPool = false);
#ifdef POOL_TIMEOUT
    first->outCycle = globalClock;
    doChecks();
#endif

#ifdef POOL_SIZE_CHECK
    psize++;
    if(psize >= warn_psize) {
      I(0);
      MSG("%s:pool class size grew to %lu", Name, psize);
      warn_psize = 4 * psize;
    }
#endif

    Ttype *h = static_cast<Ttype *>(first);
    first    = first->holderNext;
    if(first == 0)
      reproduce();

#if defined(CLEAR_ON_INSERT) && defined(DEBUG)
    const char *ptr = (const char *)(h);
    for(size_t i = 0; i < sizeof(Ttype); i++) {
      I(ptr[i] == 0);
    }
#endif

    return h;
  }
};

//*********************************************

template <class Ttype, bool timeCheck = false> class poolplus {
protected:
  class Holder : public Ttype {
  public:
    Holder *holderNext;
#ifdef DEBUG
    Holder *allNext; // List of Holders when active
#endif
#ifdef POOL_TIMEOUT
    Time_t outCycle; // Only valid if inPool is false
#endif
    ID(bool inPool;)
  };

#ifdef POOL_SIZE_CHECK
  unsigned long psize;
  unsigned long warn_psize;
#endif

  ID(bool deleted;)

#ifdef POOL_TIMEOUT
  Time_t need2cycle;
#endif
#ifdef DEBUG
  Holder *allFirst; // List of Holders when active
#endif

  const int32_t Size; // Reproduction size
  const char *  Name;

  Holder *first; // List of free nodes

  void reproduce() {
    I(first == 0);

    for(int32_t i = 0; i < Size; i++) {
      Holder *h     = ::new Holder;
      h->holderNext = first;
      IS(h->inPool = true);
#ifdef DEBUG
      h->allNext = allFirst;
      allFirst   = h;
#endif
      first = h;
    }
  }

public:
  poolplus(int32_t s = 32, const char *n = "poolplus name not declared")
      : Size(s)
      , Name(n) {
    I(Size > 0);
    IS(deleted = false);

#ifdef POOL_SIZE_CHECK
    psize      = 0;
    warn_psize = s * 8;
#endif

    first = 0;

#ifdef POOL_TIMEOUT
    need2cycle = globalClock + POOL_CHECK_CYCLE;
#endif
#ifdef DEBUG
    allFirst = 0;
#endif

    if(first == 0)
      reproduce();
  }

  ~poolplus() {
    // The last pool whould delete all the crap
    while(first) {
      Holder *h = first;
      first     = first->holderNext;
      ::delete h;
    }
    first = 0;
    IS(deleted = true);
  }

  void doChecks() {
#ifdef POOL_TIMEOUT
    if(!timeCheck)
      return;
    if(need2cycle < globalClock) {
      Holder *tmp = allFirst;
      while(tmp) {
        GI(!tmp->inPool, (tmp->outCycle + POOL_CHECK_CYCLE) > need2cycle);
        tmp = tmp->allNext;
      }
      need2cycle = globalClock + POOL_CHECK_CYCLE;
    }
#endif // POOL_TIMEOUT
  }

  void in(Ttype *data) {
    I(!deleted);
    Holder *h = static_cast<Holder *>(data);

    I(!h->inPool);
    IS(h->inPool = true);

    h->holderNext = first;
    first         = h;

#ifdef POOL_SIZE_CHECK
    psize--;
#endif

    doChecks();
  }

  Ttype *out() {
    I(!deleted);
    I(first);

    I(first->inPool);
    IS(first->inPool = false);
#ifdef POOL_TIMEOUT
    first->outCycle = globalClock;
    doChecks();
#endif

#ifdef POOL_SIZE_CHECK
    psize++;
    if(psize >= warn_psize) {
      I(0);
      MSG("%s:pool class size grew to %lu", Name, psize);
      warn_psize = 4 * psize;
    }
#endif

    Ttype *h = static_cast<Ttype *>(first);
    first    = first->holderNext;
    if(first == 0)
      reproduce();

    h->Ttype::prepare();

    return h;
  }
};

//*********************************************

#endif // _POOL_H
