/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic
                  Smruti Sarangi

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * This launches the ESESC simulator environment with an ideal memory
 */

#include "nanassert.h"

#include "ARMCrack.h"
#include "BootLoader.h"
#include "CrackBase.h"
#include "DInst.h"
#include "GProcessor.h"
#include "Instruction.h"
#include "MemObj.h"
#include "MemRequest.h"
#include "MemStruct.h"
#include "MemorySystem.h"
#include "RAWDInst.h"
#include "SescConf.h"
#include "ThumbCrack.h"
#include "callback.h"
#include "nanassert.h"

static int rd_pending = 0;
static int wr_pending = 0;

DInst *  ld;
DInst *  st;
ARMCrack crackInstARM;

RAWDInst rinst;
void     rdDone(DInst *dinst) {

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

long long num_operations = 0;

static void doread(MemObj *cache, AddrType addr) {
  num_operations++;
  DInst *ldClone = ld->clone();
  ldClone->setAddr(addr);

  while(cache->isBusy(addr))
    EventScheduler::advanceClock();

  rdDoneCB *cb = rdDoneCB::create(ldClone);
  printf("rd %x @%lld\n", (unsigned int)addr, (long long)globalClock);

  ExtraParameters param;
  param.configure(ldClone);
  MemRequest::sendReqRead(cache, ldClone->getStatsFlag(), addr, cb, &param);
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

  ExtraParameters param;
  param.configure(stClone);

  MemRequest::sendReqWrite(cache, stClone->getStatsFlag(), addr, cb, &param);
  wr_pending++;
}

void single() {
  GProcessor *gproc = TaskHandler::getSimu(0);
  I(gproc);

  MemObj *cache1 = gproc->getMemorySystem()->getDL1();
  I(cache1);
  MemObj *cache2 = gproc->getMemorySystem()->getIL1();
  I(cache2);

  printf("trivial test\n");

  doread(cache1, 0x101);
  waitAllMemOpsDone();
  printf("************** Finished request 1 ****************** \n");

  printf("************** Start request 2 ****************** \n");
  doread(cache1, 0x102);
  waitAllMemOpsDone();
  printf("************** Finished request 2 ****************** \n");

  printf("************** Start request 3 ****************** \n");
  doread(cache1, 0x103);
  waitAllMemOpsDone();
  printf("************** Finished request 3 ****************** \n");

  printf("************** Start request 4 ****************** \n");
  doread(cache1, 0x104);
  waitAllMemOpsDone();
  printf("************** Finished request 4 ****************** \n");
}

void isca_demo() {
  GProcessor *gproc = TaskHandler::getSimu(0);
  I(gproc);

  MemObj *cache1 = gproc->getMemorySystem()->getDL1();
  I(cache1);

  printf("access even address 0x102\n");
  doread(cache1, 0x102);
  waitAllMemOpsDone();

  printf("access odd address 0x201\n");
  doread(cache1, 0x201);
  waitAllMemOpsDone();
}

void multi() {
  if(TaskHandler::getNumCPUS() < 2) {
    MSG("The esesc.conf should have 2 or more CPUs to run this test\n");
    exit(-2);
  }

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  printf("Multicore trivial test\n");
  doread(P0DL1, 1203); // miss

  waitAllMemOpsDone();

  doread(P1DL1, 1203); // miss L1, hit L2

  waitAllMemOpsDone();

  doread(P0DL1, 1203); // hit L1

  waitAllMemOpsDone();

  dowrite(P1DL1, 1203); // hit L1, trigger invalidate in P0L1

  waitAllMemOpsDone();

  doread(P0DL1, 1203); // miss L1

  waitAllMemOpsDone();
}

void memcpy() {
  if(TaskHandler::getNumCPUS() < 2) {
    MSG("The esesc.conf should have 2 or more CPUs to run this test\n");
    exit(-2);
  }

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  printf("Multicore trivial test\n");
  int rd_addr = 0x00100000;
  int wr_addr = 0xFF000000;
  int cp_size = 1024;

  Time_t start = globalClock;
#if 0
  printf("starting memcpy waiting for each iteration...\n");
  for(int i=0;i<cp_size;i+=4) {
    doread(P0DL1, rd_addr);
    dowrite(P0DL1, wr_addr);
    rd_addr += 4;
    wr_addr += 4;
    waitAllMemOpsDone();
  }
  printf("Waiting memcpy took %lld cycles (%fB/s)\n",(long long)(globalClock-start),((double)cp_size)/(globalClock-start));
#endif
  start = globalClock;
  printf("starting memcpy saturating...\n");
  for(int i = 0; i < cp_size; i += 4) {
    doread(P0DL1, rd_addr);
    dowrite(P0DL1, wr_addr);
    rd_addr += 4;
    wr_addr += 4;
    EventScheduler::advanceClock();
  }
  waitAllMemOpsDone();
  printf("Saturating memcpy took %lld cycles (%fB/s)\n", (long long)(globalClock - start),
         ((double)cp_size) / (globalClock - start));
}

// Test 2 - Test that when Core 0 loads an address and then Core 1 accesses the same address,
// the data will come from L2 cache and not from Memory
// Test that when there are 4 cores, if core 0 loads an address, and then core 2 loads the
// same address it will come from cache L3 and not from memory.
void test2() {

  // fix scenario read read write read

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  GProcessor *gproc2 = TaskHandler::getSimu(2);
  I(gproc2);

  MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
  I(P2DL1);

  /*
  printf("Test 2\n");
  printf("Core 0\n");
  cache = P0DL1;

  AddrType addr1 = 0;
  for(int i = 0; i < 132; i += 4)
  {
    addr1 += 1;
    doread(0x400+i);//goes to line 1024
    waitAllMemOpsDone();
  }
  printf("Core 1\n");
  cache = P1DL1;
  for(int i = 0; i < 132; i += 4)
  {
    addr1 += 1;
    doread(0x400+i);//goes to line 1024
    waitAllMemOpsDone();
  }
  printf("Core 2\n");
  cache = P2DL1;
  for(int i =0; i< 2; i += 4)
  {
    addr1 += 1;
    doread(0x400+i);//goes to line 1024
    waitAllMemOpsDone();
  }
  */
}

// Test 3 - Core 0 writes to address 0x400, then core 1 writes to the same address, core 0's L1 data at addr 0x400 should be
// invalidated.
void test3() {

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  GProcessor *gproc2 = TaskHandler::getSimu(2);
  I(gproc2);

  MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
  I(P2DL1);

  GProcessor *gproc3 = TaskHandler::getSimu(3);
  I(gproc3);

  MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
  I(P3DL1);

  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe5832000, 0x400, 0x400, true);
  crackInstARM.expand(&rinst);
  st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  printf("Test 3\n");
  printf("Core 0\n");

  AddrType addr1 = 0;
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P0DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 2\n");
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    dowrite(P2DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 0\n");

  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P0DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 1\n");
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P1DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P1DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 2\n");
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    dowrite(P2DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 0\n");

  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P0DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
  printf("Core 1\n");
  for(int i = 0; i < 2; i++) {
    addr1 += 1;
    doread(P1DL1, 0x400 + i);
    waitAllMemOpsDone();
  }
}

// Test 4 - Test that if colisions happen in L1 (changed it to be direct mapped), the data is still available in cache L2 and L3 and
// that the data  is replaced in L1 Note: It seems like the cache does not take in account the associativity or colisions
void test4() {

  GProcessor *gproc = TaskHandler::getSimu(0);
  I(gproc);

  MemObj *cache1 = gproc->getMemorySystem()->getDL1();
  I(cache1);
  MemObj *cache2 = gproc->getMemorySystem()->getIL1();
  I(cache2);

  printf("Test 4\n");
  for(int i = 0; i < 64 * 1024; i += 32) {
    doread(cache1, 0x400 + i);
    waitAllMemOpsDone();
  }
  doread(cache1, 0x400);
  waitAllMemOpsDone();
}

#if 0
//alternating writes between cores
void test5()
{

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  GProcessor *gproc2 = TaskHandler::getSimu(2);
  I(gproc2);

  MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
  I(P2DL1);


  GProcessor *gproc3 = TaskHandler::getSimu(3);
  I(gproc3);

  MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
  I(P3DL1);
  MemObj *P3IL1 = gproc3->getMemorySystem()->getIL1();
  I(P3DL1);

  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe5832000,0x400,0x400,1,1,true);
  crackInstARM.expand(&rinst);
  st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  printf("Test 5\n");
  for(int c = 0; c<10;c++)
  {
    cache = P0DL1;
    for(int i =0; i< 4; i++)
    {
      if(i == 0)
      {
        cache = P0DL1;
        printf("Core 0\n");
      }
      else if(i == 1)
      {
        cache = P1DL1;
        printf("Core 1\n");
      }
      else if(i == 2)
      {
        cache = P2DL1;
        printf("Core 2\n");
      }
      else
      {
        cache = P3DL1;
        printf("Core 3\n");
      }
      doread(0x400+0x400*c);//goes to line 1024
      waitAllMemOpsDone();

    }
  }
}

void test6()
{

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  GProcessor *gproc2 = TaskHandler::getSimu(2);
  I(gproc2);

  MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
  I(P2DL1);


  GProcessor *gproc3 = TaskHandler::getSimu(3);
  I(gproc3);

  MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
  I(P3DL1);
  MemObj *P3IL1 = gproc3->getMemorySystem()->getIL1();
  I(P3DL1);

  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);

  printf("Test 6\n");
  for(int c = 0; c<100;c++)
  {
    cache = P0DL1;
    for(int i =0; i< 4; i++)
    {
      if(i == 0)
      {
        cache = P0DL1;
        printf("Core 0\n");
      }
      else if(i == 1)
      {
        cache = P1DL1;
        printf("Core 1\n");
      }
      else if(i == 2)
      {
        cache = P2DL1;
        printf("Core 2\n");
      }
      else
      {
        cache = P3DL1;
        printf("Core 3\n");
      }
      rinst.set(0xe5832000,0x400+c,0x400+0x400*c,130,1,1,true);
      crackInstARM.expand(&rinst);
      st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);
      dowrite(0x400+0x400*c);//goes to line 1024
      waitAllMemOpsDone();

    }
  }
}




void test7()
{

  GProcessor *gproc0 = TaskHandler::getSimu(0);
  I(gproc0);

  MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
  I(P0DL1);

  GProcessor *gproc1 = TaskHandler::getSimu(1);
  I(gproc1);

  MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
  I(P1DL1);

  GProcessor *gproc2 = TaskHandler::getSimu(2);
  I(gproc2);

  MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
  I(P2DL1);

  GProcessor *gproc3 = TaskHandler::getSimu(3);
  I(gproc3);

  MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
  I(P3DL1);
  MemObj *P3IL1 = gproc3->getMemorySystem()->getIL1();
  I(P3DL1);
  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe0803003,0x400,0x400,130,1,1,true);
  crackInstARM.expand(&rinst);
  st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  printf("Test 3\n");
  printf("Core 0\n");
  cache = P0DL1;

  AddrType addr1 = 0;
  for(int i =0; i< 2; i++)
  {
    addr1 += 1;
    doread(0x400+i);
    waitAllMemOpsDone();

  }
  printf("Core 2\n");
  cache = P2DL1;
  for(int i =0; i< 2; i++)
  {
    addr1 += 1;
    dowrite(0x400+i);
    waitAllMemOpsDone();

  }
  printf("Core 0\n");
  cache = P0DL1;

  for(int i =0; i< 2; i++)
  {
    addr1 += 1;
    doread(0x400+i);
    waitAllMemOpsDone();

  }
  printf("Core 1\n");
  cache = P1DL1;
  for(int i =0; i< 2; i++)
  {
    addr1 += 1;
    doread(0x400+i);
    waitAllMemOpsDone();

  }

}

#endif

int main(int argc, const char **argv) {

  BootLoader::plug(argc, argv);

  // Create a LD (e5d33000) with PC = 0xfeeffeef and address 1203
  rinst.set(0xe5d33000, 0xfeeffeef, 1203, true);
  crackInstARM.expand(&rinst);
  ld = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  // Create a ST (e5832000) with PC = 0x410 and address 0x400
  rinst.set(0xe5832000, 0x410, 0x400, true);
  crackInstARM.expand(&rinst);
  st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  isca_demo();

  // printf("SINGLE CORE TEST\n");
  // single();

  // multi();
  // memcpy();

  printf("test Done!\n");
  // TaskHandler::terminate(); // put CPUs to sleep while running fast-forward

  BootLoader::report("done");
  BootLoader::unplug();

  printf("It performed a total of %lld operations\n", num_operations);

  return 0;
}
