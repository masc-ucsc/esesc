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

#include "GProcessor.h"
#include "BootLoader.h"
#include "MemObj.h"
#include "MemRequest.h"
#include "callback.h"
#include "MemorySystem.h"
#include "SescConf.h"
#include "DInst.h"
#include "nanassert.h"
#include "RAWDInst.h"
#include "Instruction.h"
#include "CrackBase.h"
#include "ARMCrack.h"
#include "ThumbCrack.h"

static int rd_pending = 0;
static int wr_pending = 0;

MemObj *cache;
DInst *ld;
DInst *st;
DInst *add;
MemRequest *mreq;
ARMCrack crackInstARM;

RAWDInst rinst;
void rdDone()
{
	rd_pending--;
 	mreq->rawdump_calledge(2,10);
}

void wrDone()
{
	wr_pending--;
 	mreq->rawdump_calledge(2,10);
}

typedef CallbackFunction0<&rdDone> rdDoneCB;
typedef CallbackFunction0<&wrDone> wrDoneCB;


uint64_t conta =0;
//RAWDInst *rinst1;

 
void doread(AddrType addr)
{
  printf("Read Addr: %x ", addr);
	conta++;
        while(!cache->canAcceptRead(ld))
		EventScheduler::advanceClock();
	//MemRequest *createRead(Memobj *m, DInst *dinst, AddrType addr, CallbackBase *cb=0)
	mreq = MemRequest::createRead(cache, ld, addr, rdDoneCB::create());
	cache->read(mreq);
	rd_pending++;
}

void dowrite(AddrType addr)
{
  printf("Write %x ", rinst.getAddr() );
  conta++;
	while(!cache->canAcceptWrite(st))
		EventScheduler::advanceClock();
	mreq = MemRequest::createWrite(cache, st, wrDoneCB::create());
	cache->write(mreq);
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
  cache = cache1;
  doread(1203);
//  cache = cache2;
  //doread(1203);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  doread(1203);
  //cache = cache2;
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
   doread(1400);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  doread(1407);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  doread(1408);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  dowrite(0x200);
  printf("Fuck1\n");

/*	for(int j=0;j<8;j++) {
		printf("memcpy test\n");
		for(int i=0;i<256*1024;i++) {

      if((i&3) == 1) {
        cache = cache1;
      }else{
        cache = cache2;
      }

			doread(32768+i);
			dowrite(1024*1024*1024+32768+i);

			EventScheduler::advanceClock();
			if ((i&32767)== 1) {
				printf(".");
				fflush(stdout);
			}
		}
		printf("\n");
		printf("random test\n");
		for(int i=0;i<65*1024*1024;i++) {

      if(i&1) {
        cache = cache1;
      }else{
        cache = cache2;
      }

			int rnd = random();

			if ((rnd & 7) == 3)
				doread(rnd+1024*i*4);
			if ((rnd & 7) == 6)
				dowrite(rnd+1024*i*4);

			EventScheduler::advanceClock();
			if ((i&(16*32767))== 1) {
				printf(".");
				fflush(stdout);
			}
		}
		printf("\n");
	}
*/
	while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
    printf("Fuck2\n");
}
#if 0

void multi() {

  FlowID nsimu = SescConf->getRecordSize("","cpusimu");
  if(nsimu<1) {
    printf("No multicore test, configuration has a single CPU\n");
    return;
  }

  MemObj *caches[nsimu];
  for(size_t i=0;i<nsimu;i++) {
    GProcessor *gproc = TaskHandler::getSimu(i); 
    I(gproc);
    caches[i] = gproc->getMemorySystem()->getDL1();
  }

	for(int j=0;j<8;j++) {
		printf("produce/consumers\n");
		for(int k=0;k<1024*1024;k++) {

      size_t pid = random() % nsimu;

      cache = caches[pid];
      dowrite(random()%1024+((random()&4)*1024*1024)+32);
      for(size_t l=0;l<nsimu;l++) {
        if (l==pid)
          continue;
        if ((random() & 7) == 3)
          continue;

        cache = caches[l];
        doread(random()%1024+((random()&4)*1024*1024)+32);
      }

			EventScheduler::advanceClock();
			if ((k&32767)== 1) {
				printf(".");
				fflush(stdout);
			}
		}
		printf("\n");
	}

	while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
}
#endif

void multi()
{

	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

  printf("Multicore trivial test\n");
  cache = P0DL1;
  doread(1203);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  cache = P1DL1;
  doread(1203);
  
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
   doread(1400);
  while(rd_pending || wr_pending)
    EventScheduler::advanceClock();
  doread(1407);



}
//Test 1 - Test that the cache line size affects load/st hits on subsequent addresses
void test1()
{
	
	GProcessor *gproc = TaskHandler::getSimu(0); 
	I(gproc);

	MemObj *cache1 = gproc->getMemorySystem()->getDL1();
	I(cache1);
	MemObj *cache2 = gproc->getMemorySystem()->getIL1();
	I(cache2);

  printf("Test 1\n");
  cache = cache1;
  AddrType addr1 = 0;
  for(int i =0; i< 40; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
}


//Test 2 - Test that when Core 0 loads an address and then Core 1 accesses the same address, the data will come from L2 cache and not from Memory
// Test that when there are 4 cores, if core 0 loads an address, and then core 2 loads the same address it will come from cache L3 and not from memory.
void test2()
{
	
	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

	GProcessor *gproc2 = TaskHandler::getSimu(2); 
	I(gproc2);

	MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
	I(P2DL1);
	MemObj *P2IL1 = gproc2->getMemorySystem()->getIL1();
	I(P2DL1);

  printf("Test 2\n");
	printf("Core 0\n");
  cache = P0DL1;

  AddrType addr1 = 0;
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);//goes to line 1024
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 1\n");
 	cache = P1DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);//goes to line 1024
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 2\n");
 	cache = P2DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);//goes to line 1024
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}

}

//Test 3 - Core 0 writes to address 0x400, then core 1 writes to the same address, core 0's L1 data at addr 0x400 should be invalidated.
void test3()
{
	
	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

	GProcessor *gproc2 = TaskHandler::getSimu(2); 
	I(gproc2);

	MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
	I(P2DL1);
	MemObj *P2IL1 = gproc2->getMemorySystem()->getIL1();
	I(P2DL1);

	GProcessor *gproc3 = TaskHandler::getSimu(3); 
	I(gproc3);

	MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
	I(P3DL1);
	MemObj *P3IL1 = gproc3->getMemorySystem()->getIL1();
	I(P3DL1);
  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe5832000,0x400,0x400,130,1,1,true);
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
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 2\n");
 	cache = P2DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	dowrite(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 0\n");
  cache = P0DL1;

  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 1\n");
 	cache = P1DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 2\n");
 	cache = P2DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	dowrite(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 0\n");
  cache = P0DL1;

  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 1\n");
 	cache = P1DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}

}

	
//Test 4 - Test that if colisions happen in L1 (changed it to be direct mapped), the data is still available in cache L2 and L3 and that the data  is replaced in L1
//Note: It seems like the cache does not take in account the associativity or colisions 
void test4()
{
	
	GProcessor *gproc = TaskHandler::getSimu(0); 
	I(gproc);

	MemObj *cache1 = gproc->getMemorySystem()->getDL1();
	I(cache1);
	MemObj *cache2 = gproc->getMemorySystem()->getIL1();
	I(cache2);

  printf("Test 4\n");
  cache = cache1;
  for(int i =0; i< 64*1024; i+=32)
	{
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
  	doread(0x400);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();


}

//alternating writes between cores
void test5()
{
	
	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

	GProcessor *gproc2 = TaskHandler::getSimu(2); 
	I(gproc2);

	MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
	I(P2DL1);
	MemObj *P2IL1 = gproc2->getMemorySystem()->getIL1();
	I(P2DL1);


	GProcessor *gproc3 = TaskHandler::getSimu(3); 
	I(gproc3);

	MemObj *P3DL1 = gproc3->getMemorySystem()->getDL1();
	I(P3DL1);
	MemObj *P3IL1 = gproc3->getMemorySystem()->getIL1();
	I(P3DL1);

  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe5832000,0x400,0x400,130,1,1,true);
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
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
  }
}

void test6()
{
	
	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

	GProcessor *gproc2 = TaskHandler::getSimu(2); 
	I(gproc2);

	MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
	I(P2DL1);
	MemObj *P2IL1 = gproc2->getMemorySystem()->getIL1();
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
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
  }
}




void test7()
{
	
	GProcessor *gproc0 = TaskHandler::getSimu(0); 
	I(gproc0);

	MemObj *P0DL1 = gproc0->getMemorySystem()->getDL1();
	I(P0DL1);
	MemObj *P0IL1 = gproc0->getMemorySystem()->getIL1();
	I(P0DL1);

	GProcessor *gproc1 = TaskHandler::getSimu(1); 
	I(gproc1);

	MemObj *P1DL1 = gproc1->getMemorySystem()->getDL1();
	I(P1DL1);
	MemObj *P1IL1 = gproc1->getMemorySystem()->getIL1();
	I(P1DL1);

	GProcessor *gproc2 = TaskHandler::getSimu(2); 
	I(gproc2);

	MemObj *P2DL1 = gproc2->getMemorySystem()->getDL1();
	I(P2DL1);
	MemObj *P2IL1 = gproc2->getMemorySystem()->getIL1();
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
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 2\n");
 	cache = P2DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	dowrite(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 0\n");
  cache = P0DL1;

  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}
	printf("Core 1\n");
 	cache = P1DL1;	
  for(int i =0; i< 2; i++)
	{
    addr1 += 1;
  	doread(0x400+i);
 		 while(rd_pending || wr_pending)
   		 EventScheduler::advanceClock();
	}

}

int main(int argc, const char **argv) { 

  BootLoader::plug(argc, argv);
  printf("SINGLE CORE TESTS\n");


  // rinst->set(insn,pc,addr,data,L1clkRatio,L3clkRatio,keepStats);
  rinst.set(0xe5d33000,0xfeeffeef,1203,130,1,1,true);
  crackInstARM.expand(&rinst);
 // for(size_t j=0;j<rinst->getNumInst();j++) {
  //  const Instruction *inst = rinst->getInstRef(j);
  // }

  ld = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  rinst.set(0xe5832000,0x400,0x400,130,1,1,true);
  crackInstARM.expand(&rinst);
  st = DInst::create(rinst.getInstRef(0), &rinst, rinst.getPC(), 0);

  //Instruction *ld.set(iLALU_LD, 0, 1, 0, 1, 0);
  //Instruction *st    = Instruction::create(iSALU_ST, 0, 0, 0, 0, false );
  //Instruction *stadd = Instruction::create(iSALU_ADDR, 0, 0, 0, 0, false );
//
//DInst *dld1 = DInst::create(ld, rinst, 0, 0);

	printf("TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
  //single();
  test1();
// 		test2();
// 	test3();
//		test4();
//		test5();
//		test6();
	//	test7();
#if 0 
  printf("MULTI CORE TESTS\n");
  multi();
#endif
  printf("test Done!/n");
//  TaskHandler::terminate(); // put CPUs to sleep while running fast-forward
 


//DM

  BootLoader::report("done");
  BootLoader::unplug();

	printf("It performed a total of %lld operations\n", (long long) conta);

  return 0;
}
