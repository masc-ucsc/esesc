// cache test
//
// TODO:
//
// -no double snoop detect
//
// -Create represure in the L1 cache with requests.
//
// -3 cores doing a write simultaneously, how to handle snoops

#include <exception>
#include <stdio.h>
#include <strings.h>

#include "CCache.h"
#include "DInst.h"
#include "GProcessor.h"
#include "Instruction.h"
#include "MemObj.h"
#include "MemRequest.h"
#include "MemStruct.h"
#include "MemorySystem.h"
#include "RAWDInst.h"
#include "SescConf.h"
#include "callback.h"
#include "nanassert.h"
#include "gtest/gtest.h"

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestCase;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

#ifdef DEBUG_CALLPATH
extern bool forcemsgdump;
#endif

static int rd_pending     = 0;
static int wr_pending     = 0;
static int num_operations = 0;

#ifdef ENABLE_NBSD
#include "MemRequest.h"
void meminterface_start_snoop_req(uint64_t addr, bool inv, uint16_t coreid, bool dcache, void *_mreq) {
  I(0);
}
void meminterface_req_done(void *param, int mesi) {
  I(0);
}
#endif

enum currState {
  Modified,  // 0
  Exclusive, // 1
  Shared,    // 2
  Invalid    // 3
};

currState getState(CCache *cache, AddrType addr) {
  currState state;

  if(cache->Modified(addr))
    state = Modified;

  if(cache->Exclusive(addr))
    state = Exclusive;

  if(cache->Shared(addr))
    state = Shared;

  if(cache->Invalid(addr))
    state = Invalid;

  return state;
}

DInst *  ld;
DInst *  st;
RAWDInst rinst;

void rdDone(DInst *dinst) {

  printf("rddone @%lld\n", (long long)globalClock);

  rd_pending--;
  dinst->recycle();
}

void wrDone(DInst *dinst) {

  printf("wrdone @%lld\n", (long long)globalClock);

  wr_pending--;
  dinst->recycle();
}

static void waitAllMemOpsDone() {
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
}

typedef CallbackFunction1<DInst *, &rdDone> rdDoneCB;
typedef CallbackFunction1<DInst *, &wrDone> wrDoneCB;

static void doread(MemObj *cache, AddrType addr) {
  num_operations++;
  DInst *ldClone = ld->clone();
  ldClone->setAddr(addr);

  while(cache->isBusy(addr))
    EventScheduler::advanceClock();

  rdDoneCB *cb = rdDoneCB::create(ldClone);
  printf("rd %x @%lld\n", (unsigned int)addr, (long long)globalClock);

  MemRequest::sendReqRead(cache, ldClone->getStatsFlag(), addr, cb);
  rd_pending++;
}

static void doprefetch(MemObj *cache, AddrType addr) {
  num_operations++;
  DInst *ldClone = ld->clone();
  ldClone->setAddr(addr);

  while(cache->isBusy(addr))
    EventScheduler::advanceClock();

  rdDoneCB *cb = rdDoneCB::create(ldClone);
  printf("rd %x @%lld\n", (unsigned int)addr, (long long)globalClock);

  MemRequest::sendReqReadPrefetch(cache, ldClone->getStatsFlag(), addr, cb);
  rd_pending++;
}

static void dowrite(MemObj *cache, AddrType addr) {
  num_operations++;
  DInst *stClone = st->clone();
  stClone->setAddr(addr);

  while(cache->isBusy(addr))
    EventScheduler::advanceClock();

  wrDoneCB *cb = wrDoneCB::create(stClone);
  printf("wr %x @%lld\n", (unsigned int)addr, (long long)globalClock);

  MemRequest::sendReqWrite(cache, stClone->getStatsFlag(), addr, cb);
  wr_pending++;
}

bool           pluggedin = false;
GMemorySystem *gms_p0    = 0;
GMemorySystem *gms_p1    = 0;
void           initialize() {

  if(!pluggedin) {
    int         arg1   = 1;
    const char *arg2[] = {"cachetest", 0};
    // setenv("ESESC_tradCORE_DL1","DL1_core DL1",1);

    SescConf = new SConfig(arg1, arg2);
    // unsetenv("ESESC_tradCore_DL1");
    gms_p0 = new MemorySystem(0);
    gms_p0->buildMemorySystem();
    gms_p1 = new MemorySystem(1);
    gms_p1->buildMemorySystem();
    pluggedin = true;
#ifdef DEBUG_CALLPATH
    forcemsgdump = true;
#endif
  }

  // Create a LD (e5d33000) with PC = 0xfeeffeef and address 1203
  Instruction *ld_inst = new Instruction();
  ld_inst->set(iLALU_LD, LREG_R1, LREG_R2, LREG_R3, LREG_R4);
  ld = DInst::create(ld_inst, 0xfeeffeef, 1203, 0, true);

  Instruction *st_inst = new Instruction();
  st_inst->set(iSALU_ST, LREG_R1, LREG_R2, LREG_R3, LREG_R4);
  st = DInst::create(st_inst, 0x410, 0x400, 0, true);
}

CCache *getDL1(GMemorySystem *gms) {
  MemObj *dl1 = gms->getDL1();
  if(strcasecmp(dl1->getDeviceType(), "cache") != 0) {
    printf("going down from %s\n", dl1->getDeviceType());
    MRouter *router = dl1->getRouter();
    dl1             = router->getDownNode();
    if(strcasecmp(dl1->getDeviceType(), "cache") != 0) {
      printf("ERROR: Neither first or second level is a cache %s\n", dl1->getDeviceType());
      exit(-1);
    }
  }

  CCache *cdl1 = static_cast<CCache *>(dl1);
  return cdl1;
}

CCache *getL3(MemObj *L2) {
  MRouter *router2 = L2->getRouter();
  MemObj * L3      = router2->getDownNode();
  if(strncasecmp(L3->getName(), "L3", 2) != 0)
    return 0;
  if(strcasecmp(L3->getDeviceType(), "cache") != 0)
    return 0;

  CCache *l3c = static_cast<CCache *>(L3);
  return l3c;
}

CCache *getL2(MemObj *P0DL1) {
  MRouter *router = P0DL1->getRouter();
  MemObj * L2     = router->getDownNode();
  CCache * l2c    = static_cast<CCache *>(L2);
  // l2c->setNeedsCoherence();
  return l2c;
}

class CacheTest : public testing::Test {
protected:
  CCache *     p0dl1;
  CCache *     p0l2;
  CCache *     p1l2;
  CCache *     l3;
  CCache *     p1dl1;
  virtual void SetUp() {
    initialize();
    p0dl1 = getDL1(gms_p0);

    p0l2 = getL2(p0dl1);
    p0l2->setCoreDL1(0); // to allow fake L2 direct accesses

    p1dl1 = getDL1(gms_p1);
    p1l2  = getL2(p1dl1);

    l3 = getL3(p0l2);
  }
  virtual void TearDown() {
  }
};

// the first test checks that if no cache has data and then one
// cache reads it from memory, then that cache gets set to Exclusive

TEST_F(CacheTest, l1miss_l2miss) {
  doread(p0dl1, 0x100); // L1 miss, L2 miss
  waitAllMemOpsDone();
  EXPECT_EQ(Exclusive, getState(p0dl1, 0x100));
  EXPECT_EQ(Invalid, getState(p1dl1, 0x100));

  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0x100));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0x100));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x100));
}

TEST_F(CacheTest, L1miss_l2hit) {
  doread(p0l2, 0x180); // L2 miss
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p0dl1, 0x180));
  EXPECT_EQ(Invalid, getState(p1dl1, 0x180));
  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0x180));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0x180));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x180));

  dowrite(p0dl1, 0x180); // L1 miss, L2 hit
  waitAllMemOpsDone();
  EXPECT_EQ(Modified, getState(p0dl1, 0x180));
  EXPECT_EQ(Invalid, getState(p1dl1, 0x180));
  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0x180));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0x180));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x180));
}

TEST_F(CacheTest, multiple_reqs) {
  // Setup, L2 line (empty in all the others)
  doread(p0l2, 0xF000); // L2 miss
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p0dl1, 0xF000));
  EXPECT_EQ(Invalid, getState(p1dl1, 0xF000));

  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0xF000));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0xF000));

  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0xF000));

  doread(p0dl1, 0xF000); // L1 miss, L2 hit
  waitAllMemOpsDone();
  EXPECT_EQ(Exclusive, getState(p0dl1, 0xF000));
  doread(p1dl1, 0xF000); // L1 miss, L2 hit
  waitAllMemOpsDone();
  EXPECT_EQ(Shared, getState(p0dl1, 0xF000));
  EXPECT_EQ(Shared, getState(p1dl1, 0xF000));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0xF000));
    else
      EXPECT_EQ(Exclusive, getState(p0l2, 0xF000));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0xF000));
    else
      EXPECT_EQ(Shared, getState(p0l2, 0xF000));
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0xF000));
    else
      EXPECT_EQ(Shared, getState(p1l2, 0xF000));
  }
  if(l3) {
    EXPECT_EQ(Exclusive, getState(l3, 0xF000));
  }
}

// the second test checks that if one cache reads a line and then
// a second cache reads that line,  both become shared
TEST_F(CacheTest, Shared_second_test) {
  doread(p0dl1, 0x200);
  waitAllMemOpsDone();
  doread(p1dl1, 0x200);
  waitAllMemOpsDone();

  EXPECT_EQ(Shared, getState(p0dl1, 0x200));
  EXPECT_EQ(Shared, getState(p1dl1, 0x200));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x200));
    else
      EXPECT_EQ(Exclusive, getState(p0l2, 0x200));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x200));
    else
      EXPECT_EQ(Shared, getState(p0l2, 0x200));

    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x200));
    else
      EXPECT_EQ(Shared, getState(p1l2, 0x200));
  }
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x200));
}

// in the third test, a cache has a write miss that no one else has, and we check if
// the state of that cache goes to modified

TEST_F(CacheTest, Modified_third_test) {
  dowrite(p0dl1, 0x300);
  waitAllMemOpsDone();
  EXPECT_EQ(Modified, getState(p0dl1, 0x300));

  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0x300));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0x300));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x300));
}

// in the fourth test, a cache reads from memory and becomes exclusive,
// then has a write hit and should go to modified
//
// currently this test fails, and it stays at exclusive
//
TEST_F(CacheTest, Modified_fourth_test) {
  doread(p0dl1, 0x400);
  waitAllMemOpsDone();
  dowrite(p0dl1, 0x400);
  waitAllMemOpsDone();
  EXPECT_EQ(Modified, getState(p0dl1, 0x400));
  if(p0l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p0l2, 0x400));
  else
    EXPECT_EQ(Exclusive, getState(p0l2, 0x400));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x400));
}

// in the fifth test, a cache has a write miss and goes to modified, then
// another cache has a read miss, so the first one becomes owner and the
// second becomes shared
//
TEST_F(CacheTest, Owner_fifth_test) {
  dowrite(p0dl1, 0x500);
  waitAllMemOpsDone();
  doread(p1dl1, 0x500);
  waitAllMemOpsDone();

  EXPECT_EQ(Shared, getState(p0dl1, 0x500));
  EXPECT_EQ(Shared, getState(p1dl1, 0x500));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x500));
    else
      EXPECT_EQ(Modified, getState(p0l2, 0x500));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x500));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x500));
    else
      EXPECT_EQ(Shared, getState(p0l2, 0x500));
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Shared, getState(p1l2, 0x500));
    else
      EXPECT_EQ(Shared, getState(p1l2, 0x500));
    if(l3)
      EXPECT_EQ(Modified, getState(l3, 0x500));
  }
}

// in the sixth test, a cache has a read miss, then the other
// cache has a write miss, invalidating the first cache and becoming modified
// itself
//
// The test currently passes, but it should be noted that the cache line does
// not actually enter an invalid state, it is just assigned a null pointer
TEST_F(CacheTest, Invalid_sixth_test) {
  doread(p0dl1, 0x600);
  waitAllMemOpsDone();
  dowrite(p1dl1, 0x600);
  waitAllMemOpsDone();

  EXPECT_EQ(Invalid, getState(p0dl1, 0x600));
  EXPECT_EQ(Modified, getState(p1dl1, 0x600));
  if(p1l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p1l2, 0x600));
  else
    EXPECT_EQ(Exclusive, getState(p1l2, 0x600));
  if(l3)
    EXPECT_EQ(Exclusive, getState(l3, 0x600));

  dowrite(p0dl1, 0x600);
  waitAllMemOpsDone();
  EXPECT_EQ(Modified, getState(p0dl1, 0x600));
  EXPECT_EQ(Invalid, getState(p1dl1, 0x600));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x600));
    else
      EXPECT_EQ(Modified, getState(p0l2, 0x600));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x600));
  } else {
    EXPECT_EQ(Invalid, getState(p1l2, 0x600));
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x600));
    else
      EXPECT_EQ(Exclusive, getState(p0l2, 0x600));
    if(l3)
      EXPECT_EQ(Modified, getState(l3, 0x600));
  }
}

// in the seventh test, a cache has a read miss and then the other cache has
// a read miss, so then they are both shared. Then the second cache does a
// write and becomes modified, while the first cache is invalidated.

TEST_F(CacheTest, Invalid_seventh_test) {
  doread(p0dl1, 0x700);
  waitAllMemOpsDone();
  EXPECT_EQ(Exclusive, getState(p0dl1, 0x700));
  doread(p1dl1, 0x700);
  waitAllMemOpsDone();
  EXPECT_EQ(Shared, getState(p0dl1, 0x700));
  EXPECT_EQ(Shared, getState(p1dl1, 0x700));
  if(l3) {
    if(p0l2 == p1l2) { // Shared L2 conf
      if(p1l2->isJustDirectory())
        EXPECT_EQ(Invalid, getState(p1l2, 0x700));
      else
        EXPECT_EQ(Exclusive, getState(p1l2, 0x700));
      EXPECT_EQ(Exclusive, getState(l3, 0x700));
    } else {
      if(p0l2->isJustDirectory())
        EXPECT_EQ(Invalid, getState(p0l2, 0x700));
      else
        EXPECT_EQ(Shared, getState(p0l2, 0x700));
      if(p1l2->isJustDirectory())
        EXPECT_EQ(Invalid, getState(p1l2, 0x700));
      else
        EXPECT_EQ(Shared, getState(p1l2, 0x700));
      EXPECT_EQ(Exclusive, getState(l3, 0x700));
    }
  }
  dowrite(p1dl1, 0x700);
  waitAllMemOpsDone();

  EXPECT_EQ(Invalid, getState(p0dl1, 0x700));
  EXPECT_EQ(Modified, getState(p1dl1, 0x700));
  if(p1l2->isJustDirectory())
    EXPECT_EQ(Invalid, getState(p1l2, 0x700));
  else
    EXPECT_EQ(Exclusive, getState(p1l2, 0x700));
  if(l3) {
    EXPECT_EQ(Exclusive, getState(l3, 0x700));
  }
}

// in the eighth test, a cache has a write miss, becoming modified. Then
// another cache has a read miss, so that the first one becomes owner
// and the second becomes shared. Then the second one is written to becoming
// Modified, invalidating the owner
//
// This test fails. The first cache does transition from Owner to Invalid,
// but the second cache fails to go from shared to modified.
TEST_F(CacheTest, Invalid_eighth_test) {
  dowrite(p1dl1, 0x800);
  waitAllMemOpsDone();
  doread(p0dl1, 0x800);
  waitAllMemOpsDone();
  EXPECT_EQ(Shared, getState(p1dl1, 0x800));
  EXPECT_EQ(Shared, getState(p0dl1, 0x800));
  dowrite(p0dl1, 0x800);
  waitAllMemOpsDone();

  EXPECT_EQ(Invalid, getState(p1dl1, 0x800));
  EXPECT_EQ(Modified, getState(p0dl1, 0x800));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x800));
    else
      EXPECT_EQ(Modified, getState(p0l2, 0x800));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x800));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x800));
    else
      EXPECT_EQ(Shared, getState(p0l2, 0x800));
    if(l3)
      EXPECT_EQ(Modified, getState(l3, 0x800));
  }
}

// in the ninth test, a cache does a read to become exclusive. Then another
// cache does a write which makes it modified and the first cache invalid
//
TEST_F(CacheTest, Invalid_ninth_test) {
  doread(p0dl1, 0x900);
  waitAllMemOpsDone();
  dowrite(p1dl1, 0x900);
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p0dl1, 0x900));
  EXPECT_EQ(Modified, getState(p1dl1, 0x900));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Exclusive, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  } else {
    EXPECT_EQ(Invalid, getState(p0l2, 0x900));
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Exclusive, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  }

  doread(p1dl1, 0x900);
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p0dl1, 0x900));
  EXPECT_EQ(Modified, getState(p1dl1, 0x900));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Exclusive, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  } else {
    EXPECT_EQ(Invalid, getState(p0l2, 0x900));
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Exclusive, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  }

  doread(p0dl1, 0x900);
  waitAllMemOpsDone();
  EXPECT_EQ(Shared, getState(p0dl1, 0x900));
  EXPECT_EQ(Shared, getState(p1dl1, 0x900));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Modified, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x900));
    else
      EXPECT_EQ(Shared, getState(p0l2, 0x900));
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Shared, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Modified, getState(l3, 0x900));
  }

  dowrite(p0dl1, 0x900);
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p1dl1, 0x900));
  EXPECT_EQ(Modified, getState(p0dl1, 0x900));
  if(p0l2 == p1l2) { // Shared L2 conf
    if(p1l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    else
      EXPECT_EQ(Modified, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Exclusive, getState(l3, 0x900));
  } else {
    if(p0l2->isJustDirectory())
      EXPECT_EQ(Invalid, getState(p0l2, 0x900)); // It could be E
    else
      EXPECT_EQ(Shared, getState(p0l2, 0x900)); // It could be E
    EXPECT_EQ(Invalid, getState(p1l2, 0x900));
    if(l3)
      EXPECT_EQ(Modified, getState(l3, 0x900));
  }
}

TEST_F(CacheTest, justDirectory) {
  printf("BEGIN L2 directory test\n");
  doread(p0dl1, 0x900);
  waitAllMemOpsDone();
  dowrite(p1dl1, 0x900);
  waitAllMemOpsDone();
  EXPECT_EQ(Invalid, getState(p0dl1, 0x900));
  EXPECT_EQ(Modified, getState(p1dl1, 0x900));
}

#if 0
TEST_F(CacheTest,prefetch1){
  printf("BEGIN prefetch is not in the way\n");
  waitAllMemOpsDone();

  Time_t start;
  Time_t pref1;
  Time_t pref2;

  // Calibration
  start = globalClock;
  doread(p0dl1,0xa00);
  waitAllMemOpsDone();
  Time_t nopref = globalClock - start;

  // Prefetch ahead 2 cycles
  start = globalClock;
  doprefetch(p0dl1,0xb00);
  EventScheduler::advanceClock();
  EventScheduler::advanceClock();
  doread(p0dl1,0xb00);
  waitAllMemOpsDone();
  pref2 = globalClock - start;
  EXPECT_EQ(nopref,pref2); // prefetch started ahead 2 cycles

  // Prefetch ahead 1 cycles
  start = globalClock;
  doprefetch(p0dl1,0xc00);
  EventScheduler::advanceClock();
  doread(p0dl1,0xc00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  // Prefetch ahead 2 cycles
  start = globalClock;
  doread(p0dl1,0xd00);
  EventScheduler::advanceClock();
  EventScheduler::advanceClock();
  doprefetch(p0dl1,0xd00);
  waitAllMemOpsDone();
  pref2 = globalClock - start;
  EXPECT_EQ(nopref,pref2); // prefetch started ahead 2 cycles

  // Prefetch ahead 1 cycles
  start = globalClock;
  doread(p0dl1,0xe00);
  EventScheduler::advanceClock();
  doprefetch(p0dl1,0xe00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  // Prefetch ahead 0 cycles
  start = globalClock;
  doprefetch(p0dl1,0xf00);
  doread(p0dl1,0xf00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  printf("nopred=%d pref2=%d\n",nopref, pref2);
}

TEST_F(CacheTest,prefetch2) {
  printf("BEGIN prefetch L1/L2 mix\n");
  waitAllMemOpsDone();

  Time_t start;
  Time_t pref1;
  Time_t pref2;

  // Calibration
  start = globalClock;
  doread(p0dl1,0x100a00);
  waitAllMemOpsDone();
  Time_t nopref = globalClock - start;

  // Prefetch ahead 2 cycles
  start = globalClock;
  doprefetch(p0l2,0x100b00);
  EventScheduler::advanceClock();
  EventScheduler::advanceClock();
  doread(p0dl1,0x100b00);
  waitAllMemOpsDone();
  pref2 = globalClock - start;
  EXPECT_EQ(nopref,pref2); // prefetch started ahead 2 cycles

  // Prefetch ahead 1 cycles
  start = globalClock;
  doprefetch(p0l2,0x100c00);
  EventScheduler::advanceClock();
  doread(p0dl1,0x100c00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  // Prefetch ahead 2 cycles
  start = globalClock;
  doread(p0dl1,0x100d00);
  EventScheduler::advanceClock();
  EventScheduler::advanceClock();
  doprefetch(p0l2,0x100d00);
  waitAllMemOpsDone();
  pref2 = globalClock - start;
  EXPECT_EQ(nopref,pref2); // prefetch started ahead 2 cycles

  // Prefetch ahead 1 cycles
  start = globalClock;
  doread(p0dl1,0x100e00);
  EventScheduler::advanceClock();
  doprefetch(p0l2,0x100e00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  // Prefetch ahead 0 cycles
  start = globalClock;
  doprefetch(p0l2,0x100f00);
  doread(p0dl1,0x100f00);
  waitAllMemOpsDone();
  pref1 = globalClock - start;
  EXPECT_EQ(nopref,pref1); // prefetch started ahead 2 cycles

  printf("nopred=%d pref2=%d\n",nopref, pref2);
}
#endif

TEST_F(CacheTest, l2_bw) {

  printf("BEGIN L2 BW test\n");

  Time_t start;
  int    conta;
  double t;

  start = globalClock;
  conta = 0;
  for(int i = 0; i < 32768; i += 128) {
    doread(p0l2, 0x1000 + i); // Fill L2
    conta++;
  }
  waitAllMemOpsDone();
  Time_t l2filltime = globalClock - start;
  t                 = ((double)l2filltime) / conta;
  printf("L2 BW miss test %lld (%g cycles/l2_miss)\n", l2filltime, t);

  start = globalClock;
  conta = 0;
  for(int i = 0; i < 32768 / 2; i += 128) {
    doread(p0dl1, 0x1000 + i);
    conta++;
  }
  waitAllMemOpsDone();
  Time_t l1_rdtime = globalClock - start;
  t                = ((double)l1_rdtime) / conta;
  printf("L2 BW rd test %lld (%g cycles/l1_rd)\n", l1_rdtime, t);

  start = globalClock;
  conta = 0;
  for(int i = 32768 / 2; i < 32768; i += 128) {
    dowrite(p0dl1, 0x1000 + i);
    conta++;
  }
  waitAllMemOpsDone();
  Time_t l1_wrtime = globalClock - start;
  t                = ((double)l1_wrtime) / conta;
  printf("L2 BW wr test %lld (%g cycles/l1_wr)\n", l1_wrtime, t);

  printf("END L2 BW test\n");
}
