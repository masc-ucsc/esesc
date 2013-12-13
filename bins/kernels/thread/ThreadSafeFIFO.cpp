
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


#define SCOORE

#ifdef SCOORE
#define SIZE 100000
#else
#define SIZE 10000000
#endif

template<class Type>
class ThreadSafeFIFO {
  private:
    typedef uint16_t IndexType;
    volatile IndexType tail;
    volatile IndexType head;
    Type array[65536];

  public:

    ThreadSafeFIFO() : tail(0), head(0) {
    }
    virtual ~ThreadSafeFIFO() {}

    void push(Type item_) {
      IndexType nextTail = tail + 1;
      array[tail] = item_;
      tail        = nextTail;
    };
    bool full() {
      IndexType nextTail = tail + 1;
      return (nextTail == head);
    }
    bool empty() {
      return (tail == head);
    }

    void pop(Type &obj) {

      obj = array[head];
      head = head + 1;
    };
};

#if 1
#include <pthread.h>

ThreadSafeFIFO<uint32_t> cf;

extern "C" void *qemuesesc_main_bootstrap(void *threadargs) {
  // Producer

  for(uint32_t i=0;i<SIZE;i++) {
    while(cf.full()) {
      ;
      // printf("f");
    }
    cf.push(i);
  }
}


main() {

  pthread_t qemu_thread;

  pthread_create(&qemu_thread,0,&qemuesesc_main_bootstrap,(void *) 0);

  // Consumer
  
  for(uint32_t i=0;i<SIZE;i++) {
    while(cf.empty()) {
      ;
      //printf("e");
    }
    uint32_t j;
    cf.pop(j);
    if (j!=i) {
      printf("ERROR %d vs %d\n",i,j);
      exit(-3);
    }
  }

  printf("Job done\n");
  pthread_kill(qemu_thread,SIGKILL);

}

#endif
