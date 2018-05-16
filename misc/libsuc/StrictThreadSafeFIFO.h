#ifndef StrictThreadSafeFIFO_H
#define StrictThreadSafeFIFO_H

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "Snippets.h"
#include "nanassert.h"
#define MAXSIZE 0xFFFFFFFF
enum QueueState { MostlyFull = 0, SomewhatFull, SomewhatEmpty, MostlyEmpty };

template <class Type> class StrictThreadSafeFIFO {
private:
  typedef uint8_t IndexType32;

  volatile IndexType32 tail;
  volatile IndexType32 head;
  IndexType32          max_size;
  volatile IndexType32 valid_size;
  QueueState           state;
  Type *               array;

public:
  uint32_t size() const {
    return max_size;
  }

  StrictThreadSafeFIFO()
      : tail(0)
      , head(0)
      , valid_size(0) {
    tail       = 0;
    head       = 0;
    valid_size = 0;
    array      = NULL;
  }

  void allocate(uint32_t s) {
    max_size = ((s > 0 && s < MAXSIZE) ? s : 65535); // Assuming upperbound of 65535 entries
    // array = new Type[max_size];
    // array = new Type[4096];
    array = new Type[4096];
    state = MostlyEmpty;
  }

  ~StrictThreadSafeFIFO() {
    delete[] array;
  }

  Type *getTailRef() {
    return &array[tail];
  }

  void push() {
    // Without object !!?? (Trick if the getTailRef was correctly used to save a memcpy

    AtomicAdd(&tail, 1);
    AtomicAdd(&valid_size, 1);
    fprintf(stderr, "%u ", tail);
    I(tail != head);
  };

  void push(const Type *item_) {
    IndexType32 otail = AtomicAdd(&tail, 1);
    AtomicAdd(&valid_size, 1);
    I(tail != head);
    array[otail] = *item_;
  };

  bool full() {
    IndexType32 nextTail = tail + 1;
    return (nextTail == head);
  }

  bool empty() {
    return (tail == head);
  }

  bool isMostlyFull() {
    if(valid_size > max_size / 4 * 3)
      state = MostlyFull;
    return state == MostlyFull;
  }

  bool isSomewhatFull() {
    if((valid_size <= max_size / 4 * 3) && (valid_size > max_size / 2))
      state = SomewhatFull;
    return state == SomewhatFull;
  }

  bool isSomewhatEmpty() {
    if((valid_size <= max_size / 2) && (valid_size > max_size / 4))
      state = SomewhatEmpty;
    return state == SomewhatEmpty;
  }

  bool isMostlyEmpty() {
    if(valid_size <= max_size / 4)
      state = MostlyEmpty;
    return state == MostlyEmpty;
  }

  void pop(Type *obj) {
    I(head != tail);
    *obj = array[head];
    AtomicAdd(&head, 1); // head = head + 1;
    AtomicSub(&valid_size, 1);
  };
};

#endif
