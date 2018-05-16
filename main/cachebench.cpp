
#include <sys/time.h>

#include "CacheCore.h"
#include "Report.h"
#include "SescConf.h"

class SampleState : public StateGeneric<long> {
public:
  int32_t id;

  SampleState(int32_t lineSize) {
    id = 0;
  }
  bool operator==(SampleState s) const {
    return id == s.id;
  }
};

typedef CacheGeneric<SampleState, long> MyCacheType;

MyCacheType *cache;

timeval stTime;
timeval endTime;
double  nAccess;
double  nMisses;

void startBench() {
  nAccess = 0;
  nMisses = 0;
  gettimeofday(&stTime, 0);
}

void endBench(const char *str) {
  gettimeofday(&endTime, 0);

  double usecs = (endTime.tv_sec - stTime.tv_sec) * 1000000 + (endTime.tv_usec - stTime.tv_usec);

  fprintf(stderr, "%s: %8.2f Maccesses/s %7.3f%%\n", str, nAccess / usecs, 100 * nMisses / nAccess);
}

#define MSIZE 256

double A[MSIZE][MSIZE];
double B[MSIZE][MSIZE];
double C[MSIZE][MSIZE];

void benchMatrix(const char *str) {
  MSG("Benchmark a code like a matrix multiply: %s", str);

  startBench();

  MyCacheType::CacheLine *line;

  for(int32_t i = 0; i < MSIZE; i++) {
    for(int32_t j = 0; j < MSIZE; j++) {
      // A[i][j]=0;

      // A[i][j]=...
      line = cache->writeLine((long)&A[i][j]);
      nAccess++;
      if(line == 0) {
        cache->fillLine((long)&A[i][j]);
        nAccess++;
        nMisses++;
      }

      for(int32_t k = 0; k < MSIZE; k++) {
        // A[i][j] += B[i][j]*C[j][k];

        // = ... A[i][j]
        line = cache->readLine((long)&A[i][j]);
        nAccess++;
        if(line == 0) {
          cache->fillLine((long)&A[i][j]);
          nAccess++;
          nMisses++;
        }

        // = ... B[i][j]
        line = cache->readLine((long)&B[i][j]);
        nAccess++;
        if(line == 0) {
          cache->fillLine((long)&B[i][j]);
          nAccess++;
          nMisses++;
        }

        // = ... C[i][j]
        line = cache->readLine((long)&C[i][j]);
        nAccess++;
        if(line == 0) {
          cache->fillLine((long)&C[i][j]);
          nAccess++;
          nMisses++;
        }

        // A[i][j]=...;
        line = cache->writeLine((long)&A[i][j]);
        nAccess++;
        if(line == 0) {
          cache->fillLine((long)&A[i][j]);
          nAccess++;
          nMisses++;
        }
      }
    }
  }

  endBench(str);
}

int main(int32_t argc, const char **argv) {
  if(argc != 2) {
    MSG("use: CacheSample <cfg_file>");
    exit(0);
  }

  Report::openFile("report.log");

  setenv("ESESC_tradCORE_DL1", "DL1_core DL1", 1);
  SescConf = new SConfig(argc, argv);
  unsetenv("ESESC_tradCore_DL1");

  cache = MyCacheType::create("DL1_core", "", "tst1");

  int32_t assoc = SescConf->getInt("DL1_core", "assoc");
  for(int32_t i = 0; i < assoc; i++) {
    ulong addr = (i << 8) + 0xfa;

    MyCacheType::CacheLine *line = cache->findLine(addr);
    if(line) {
      fprintf(stderr, "ERROR: Line 0x%lX (0x%lX) found\n", cache->calcAddr4Tag(line->getTag()), addr);
      exit(-1);
    }
    line     = cache->fillLine(addr);
    line->id = i;
  }

  for(int32_t i = 0; i < assoc; i++) {
    ulong addr = (i << 8) + 0xFa;

    MyCacheType::CacheLine *line = cache->findLine(addr);
    if(line == 0) {
      fprintf(stderr, "ERROR: Line (0x%lX) NOT found\n", addr);
      exit(-1);
    }
    if(line->id != i) {
      fprintf(stderr, "ERROR: Line 0x%lX (0x%lX) line->id %d vs id %d (bad LRU policy)\n", cache->calcAddr4Tag(line->getTag()),
              addr, line->id, i);
      exit(-1);
    }
  }

  // cache = MyCacheType::create("PerCore_TLB","","TLB");
  // benchMatrix("PerCore_TLB");

  cache = MyCacheType::create("DL1_core", "", "L1");
  benchMatrix("DL1_core");

#if 0
  cache = MyCacheType::create("BTB","","BTB");
  benchMatrix("BTB");

  cache = MyCacheType::create("DM","","DM");
  benchMatrix("DM");
#endif

  GStats::report("Cache Stats");

  Report::close();

  // cache->destroy();

  return 0;
}
