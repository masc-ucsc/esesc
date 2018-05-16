#ifndef THREADSAFEFIFO_H
#define THREADSAFEFIFO_H

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "Snippets.h"

template <class Type> class ThreadSafeFIFO {
private:
  typedef uint16_t   IndexType;
  volatile IndexType tail;
  volatile IndexType head;
  Type               array[32768];

public:
  uint16_t size() const {
    return 32768 / 2 - 2048;
  }
#if 0
    uint16_t realsize() const{

      if (head == tail)
        return 0;

      if (head > tail){
        return (4000-(head-tail-1));
        //return ((head-tail-1));
      }
      return (tail-head-1);
    }
#endif

  ThreadSafeFIFO()
      : tail(0)
      , head(0) {
  }
  virtual ~ThreadSafeFIFO() {
  }

  Type *getTailRef() {
    return &array[tail];
  }

  void push() {
    // AtomicAdd(&tail,static_cast<IndexType>(1));
    tail = (tail + 1) & 32767;
  };
  void push(const Type *item_) {
    array[tail] = *item_;
    push();
  };

  bool full() const {
    if(((tail + 2) & 32767) == head)
      return true;
    IndexType nextTail = ((tail + 1) & 32767); // Give some space
    return (nextTail == head);
  }

  bool halfFull() const {
    uint32_t n;
    if(head > tail) {
      n = 32768 - head + tail;
    } else {
      n = tail - head;
    }

    return n > size();
  }

  bool empty() {
    return (tail == head);
  }

  void pop() {
    // AtomicAdd(&head,static_cast<IndexType>(1));
    head = (head + 1) & 32767;
  };
  Type *getHeadRef() {
    return &array[head];
  }
  Type *getNextHeadRef() {
    return &array[static_cast<IndexType>((head + 1) & 32767)];
  }
  void pop(Type *obj) {
    *obj = array[head];
    // AtomicAdd(&head,static_cast<IndexType>(1));
    head = (head + 1) & 32767;
  };
};

#if 0
#include <pthread.h>

ThreadSafeFIFO<uint32_t> cf;

extern "C" void *qemuesesc_main_bootstrap(void *threadargs) {
  // Producer

  for(uint32_t i=0;i<10000000;i++) {
    while(cf.full()) {
      ;
      //printf("f");
    }
    cf.push(&i);
  }
}


main() {

  pthread_t qemu_thread;

  pthread_create(&qemu_thread,0,&qemuesesc_main_bootstrap,(void *) 0);

  // Consumer

  for(uint32_t i=0;i<10000000;i++) {
    while(cf.empty()) {
      ;
      // printf("e");
    }
    uint32_t j;
    cf.pop(&j);
    if (j!=i) {
      printf("ERROR %d vs %d\n",i,j);
      exit(-3);
    }
  }

  printf("Job done\n");
  pthread_kill(qemu_thread,SIGKILL);

}

#endif

#endif
