
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "Snippets.h"
#include "nanassert.h"

#include <vector>

#include "ThreadSafeFIFO.h"
#include "pool.h"

class DummyObjTest {
  int32_t c;
  void *  a;
  char    x;

public:
  char get() const {
    return c + x;
  };

  DummyObjTest()
      : c(0)
      , a(NULL)
      , x(0) {
  }

  void put(int32_t c_, char x_) {
    c = c_;
    x = c_;
  };
};

class DummyObjTest2 {
  int32_t c;
  void *  a;
  char    x;

public:
  int32_t get() const {
    return c + x;
  };

  DummyObjTest2()
      : c(0)
      , a(NULL)
      , x(0) {
  }

  void put(int32_t c_, char x_) {
    c = c_;
    x = x_;
  };
};

timeval stTime;

void start() {
  gettimeofday(&stTime, 0);
}

void finish(const char *str, int niters) {

  timeval endTime;
  gettimeofday(&endTime, 0);

  double msecs = (endTime.tv_sec - stTime.tv_sec) * 1000 + (endTime.tv_usec - stTime.tv_usec) / 1000;

  time_t t;
  time(&t);
  fprintf(stderr, "poolBench: %s %8.2f MPools/s :%s", str, (double)niters / (1000 * msecs), ctime(&t));
}

void pool_test() {
  pool<DummyObjTest> pool1(16);

  std::vector<DummyObjTest *> p(64);
  p.clear();

  long long total  = 0;
  long long pooled = 0;

  start();

  for(int32_t i = 0; i < 612333; i++) {
    for(char j = 0; j < 12; j++) {
      DummyObjTest *o = pool1.out();
      pooled++;
      o->put(j, j);
      p.push_back(o);
    }

    for(char j = 0; j < 12; j++) {
      DummyObjTest *o = p.back();
      total += o->get();
      p.pop_back();
      pool1.in(o);
    }
  }

  for(int32_t i = 0; i < 752333; i++) {
    for(char j = 0; j < 20; j++) {
      DummyObjTest *o = pool1.out();
      pooled++;
      o->put(j - 7, j);
      p.push_back(o);
    }

    for(char j = 0; j < 20; j++) {
      DummyObjTest *o = p.back();
      total += o->get();
      p.pop_back();
      pool1.in(o);
    }
  }

  for(int32_t i = 0; i < 20552333; i++) {
    DummyObjTest *o = pool1.out();
    pooled++;
    o->put(i - 1952333, 2);
    total += o->get();
    pool1.in(o);
  }

  finish("Standard", pooled);

  fprintf(stderr, "Total = %lld (135510418?)\n", total);
}
void tspool_test() {
  tspool<DummyObjTest> pool1(16);

  std::vector<DummyObjTest *> p(64);
  p.clear();

  long long total  = 0;
  long long pooled = 0;

  start();

  for(int32_t i = 0; i < 612333; i++) {
    for(char j = 0; j < 12; j++) {
      DummyObjTest *o = pool1.out();
      pooled++;
      o->put(j, j);
      p.push_back(o);
    }

    for(char j = 0; j < 12; j++) {
      DummyObjTest *o = p.back();
      total += o->get();
      p.pop_back();
      pool1.in(o);
    }
  }

  for(int32_t i = 0; i < 752333; i++) {
    for(char j = 0; j < 20; j++) {
      DummyObjTest *o = pool1.out();
      pooled++;
      o->put(j - 7, j);
      p.push_back(o);
    }

    for(char j = 0; j < 20; j++) {
      DummyObjTest *o = p.back();
      total += o->get();
      p.pop_back();
      pool1.in(o);
    }
  }

  for(int32_t i = 0; i < 20552333; i++) {
    DummyObjTest *o = pool1.out();
    pooled++;
    o->put(i - 1952333, 2);
    total += o->get();
    pool1.in(o);
  }

  finish("Thread Safe", pooled);

  fprintf(stderr, "Total = %lld (135510418?)\n", total);
}

ThreadSafeFIFO<DummyObjTest2> tsfifo;

extern "C" void *bootstrap(void *threadargs) {
  // Producer

  for(int32_t i = 0; i < 7000000; i++) {
    DummyObjTest2 obj;
    obj.put(i, 0);
    while(tsfifo.full()) {
      ;
      // printf("f");
    }
    // printf("put(%d) %p\n",i,obj);
    tsfifo.push(&obj);
  }

  return 0;
}

void test_tspool_threaded() {

  pthread_t qemu_thread;

  pthread_create(&qemu_thread, 0, &bootstrap, (void *)0);

  // Consumer
  start();

  for(int32_t i = 0; i < 7000000; i++) {
    while(tsfifo.empty()) {
      ;
      // printf("e");
    }
#if 1
    DummyObjTest2 obj;
    tsfifo.pop(&obj);
    if(obj.get() != i) {
      printf("ERROR %d vs %d\n", obj.get(), i);
      pthread_kill(qemu_thread, SIGKILL);
      exit(-3);
    }
#else
    uint32_t j = tsfifo.pop();
    if(j != i) {
      printf("ERROR %d vs %d\n", j, i);
      pthread_kill(qemu_thread, SIGKILL);
      exit(-3);
    }
#endif
  }

  finish("Multithreaded", 7000000);
  printf("Multithreaded job done\n");
  pthread_kill(qemu_thread, SIGKILL);
}

int main() {

  tspool_test();
  pool_test();
  test_tspool_threaded();

  return 0;
}
