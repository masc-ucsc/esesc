
#include <sys/time.h>

#include "ReportGen.h"
#include "CacheCore.h"

class SampleState : public StateGeneric<long> {
public:
  int32_t id;

  SampleState() {
    id = 0;
  }
  bool operator==(SampleState s) const {
    return id == s.id;
  }
};

typedef CacheGeneric<SampleState, long, false> MyCacheType;

MyCacheType *cache;


timeval stTime;
timeval endTime;
double nAccess;

void startBench()
{
  nAccess = 0;
  gettimeofday(&stTime, 0);
}


void endBench(const char *str)
{
  gettimeofday(&endTime, 0);

  double usecs = (endTime.tv_sec - stTime.tv_sec) * 1000000
    + (endTime.tv_usec - stTime.tv_usec);
  
  fprintf(stderr,"%s: %8.2f Maccesses/s\n"
	  ,str,nAccess/usecs);
}

#define MSIZE 256

double A[MSIZE][MSIZE];
double B[MSIZE][MSIZE];
double C[MSIZE][MSIZE];

void benchMatrix(const char *str)
{
  MSG("Benchmark a code like a matrix multiply: %s", str);

  startBench();

  MyCacheType::CacheLine *line;

  for(int32_t i=0;i<MSIZE;i++) {
    for(int32_t j=0;j<MSIZE;j++) {
      // A[i][j]=0;

      // A[i][j]=...
      line = cache->writeLine((long)&A[i][j]);
      nAccess++;
      if (line==0) {
	cache->fillLine((long)&A[i][j]);
	nAccess++;
      }

      for(int32_t k=0;k<MSIZE;k++) {
	// A[i][j] += B[i][j]*C[j][k];

	// = ... A[i][j]
	line = cache->readLine((long)&A[i][j]);
	nAccess++;
	if (line==0) {
	  cache->fillLine((long)&A[i][j]);
	  nAccess++;
	}

	// = ... B[i][j]
	line = cache->readLine((long)&B[i][j]);
	nAccess++;
	if (line==0) {
	  cache->fillLine((long)&B[i][j]);
	  nAccess++;
	}

	// = ... C[i][j]
	line = cache->readLine((long)&C[i][j]);
	nAccess++;
	if (line==0) {
	  cache->fillLine((long)&C[i][j]);
	  nAccess++;
	}

	// A[i][j]=...;
	line = cache->writeLine((long)&A[i][j]);
	nAccess++;
	if (line==0) {
	  cache->fillLine((long)&A[i][j]);
	  nAccess++;
	}
      }
    }
  }

  endBench(str);
}

int32_t main(int32_t argc, char **argv)
{
  if( argc != 2 ){
    MSG("use: CacheSample <cfg_file>");
    exit(0);
  }

  Report::openFile("report.log");

  SescConf = new SConfig(argv[1]);
  EnergyMgr::init();

  cache = MyCacheType::create("tst1","","tst1");

  int32_t assoc = SescConf->getInt("tst1","assoc");
  for(int32_t i=0;i<assoc;i++) {
    ulong addr = (i<<8)+0xfa;

    MyCacheType::CacheLine *line = cache->findLine(addr);
    if(line) {
      fprintf(stderr,"ERROR: Line 0x%x (0x%x) found\n"
	      ,cache->calcAddr4Tag(line->getTag())
	      ,addr);
      exit(-1);
    }
    line = cache->fillLine(addr);
    line->id = i;
  }

  for(int32_t i=0;i<assoc;i++) {
    ulong addr = (i<<8)+0xFa;

    MyCacheType::CacheLine *line = cache->findLine(addr);
    if(line == 0) {
      fprintf(stderr,"ERROR: Line (0x%x) NOT found\n"
	      ,addr);
      exit(-1);
    }
    if (line->id != i) {
      fprintf(stderr,"ERROR: Line 0x%x (0x%x) line->id %d vs id %d (bad LRU policy)\n"
	      ,cache->calcAddr4Tag(line->getTag())
	      ,addr
	      ,line->id, i
	      );
      exit(-1);
    }
  }


  cache = MyCacheType::create("TLB","","TLB");
  benchMatrix("TLB");

  cache = MyCacheType::create("L1Cache","","L1");
  benchMatrix("DL1");

  cache = MyCacheType::create("BTB","","BTB");
  benchMatrix("BTB");

  cache = MyCacheType::create("DM","","DM");
  benchMatrix("DM");

  GStats::report("Cache Stats");

  Report::close();

  cache->destroy();

  return 0;
}
