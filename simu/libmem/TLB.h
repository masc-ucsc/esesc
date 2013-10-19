//Work in Progress
//#if 0


/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
*/


#ifndef TLB_H
#define TLB_H

#include "GStats.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"
#include "CacheCore.h"
#include "MSHR.h"

#include <queue>
using namespace std;


/*****************************************************************/
// Same as the linux-kernel (http://kernel.org/doc/gorman/html/understand/understand006.html)
#define PAGE_SHIFT      12
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))
#define PTRS_PER_PTE    512

/*
 * * the pte page can be thought of an array like this: pte_t[PTRS_PER_PTE]
 * *
 * * this function returns the index of the entry in the pte page which would
 * * control the given virtual address
 * */
static inline unsigned long pte_index(unsigned long address)
{
    return (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
}

#define PMD_SHIFT     21
#define PMD_PAGE_SIZE (1UL << PMD_SHIFT)
#define PMD_PAGE_MASK (~(PMD_PAGE_SIZE-1))
#define PTRS_PER_PMD  512

/*
 * * the pmd page can be thought of an array like this: pmd_t[PTRS_PER_PMD]
 * * this macro returns the index of the entry in the pmd page which would
 * * control the given virtual address
 * 
*/
static inline unsigned long pmd_index(unsigned long address)
{
  return (address >> PMD_SHIFT) & (PTRS_PER_PMD - 1);
}

#define PGDIR_SHIFT 30
#define PGDIR_SIZE  (1UL << PGDIR_SHIFT)
#define PGDIR_MASK  (~(PGDIR_SIZE - 1))
#define PTRS_PER_PGD  4

/*
 * * the pgd page can be thought of an array like this: pgd_t[PTRS_PER_PGD]
 * *
 * * this macro returns the index of the entry in the pgd page which would
 * * control the given virtual address
 * */
static inline unsigned long pgd_index(unsigned long address)
{
  return (address >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1);
}


// Alamelu : Pick a base so that we do not collide with the user address space
#define pgd_base  2147483648UL   //0x 8000 0000
#define pmd_base  4194304      //0x 0040 0000
#define pte_base  49152        //0x 0000 C000

/*****************************************************************/

class TLB: public MemObj {

  class TState : public StateGeneric<AddrType> {
    //No need for a Process ID, because the applications are launched 
    // as pthreads, so we are guaranteed to not have aliasing
    public:
      TState(int32_t lineSize) {
      }
  };

  typedef CacheGeneric<TState,AddrType> CacheType;
  typedef CacheGeneric<TState,AddrType>::CacheLine Line;


protected:
  TimeDelta_t delay;
  TimeDelta_t lowerTLBdelay;
  GStatsHist  avgFree;

  GStatsCntr  nWrite;
  GStatsCntr  nWriteAddress;
  GStatsCntr  nRead;
  GStatsCntr  nBusReadRead;
  GStatsCntr  nBusReadWrite;

  GStatsCntr  tlbReadHit;
  GStatsCntr  tlbReadMiss;
  GStatsCntr  tlbMSHRcant_issue;
  GStatsCntr  tlblowerReadHit;
  GStatsCntr  tlblowerReadMiss;
  GStatsCntr  tlbInvalidate;

  GStatsAvg   mmuavgMissLat;
  GStatsAvg   mmuavgMemLat;

  GStatsAvg   avgMissLat;
  GStatsAvg   avgMemLat;

  PortGeneric *cmdPort;

  bool localcall;
  CacheType   *tlbBank;
  MSHR        *bmshr;
  MemObj      *lowerTLB;   //Points to the next TLB lower in the heirarchy, May be NULL
  MemObj      *lowerCache; //Points to the cache right below the TLB. (Used only for processor direct requests) 

public:
  TLB(MemorySystem* current, const char *device_descr_section, const char *device_name = NULL);
  ~TLB() {}

  Time_t nextReadSlot(       const MemRequest *mreq);
  Time_t nextWriteSlot(      const MemRequest *mreq);
  Time_t nextBusReadSlot(    const MemRequest *mreq);
  Time_t nextPushDownSlot(   const MemRequest *mreq);
  Time_t nextPushUpSlot(     const MemRequest *mreq);
  Time_t nextInvalidateSlot( const MemRequest *mreq);

  // processor direct requests
  void read(MemRequest  *req);
  void write(MemRequest *req);
  void writeAddress(MemRequest *req);

  // DOWN
  void busRead(MemRequest *req);
  void pushDown(MemRequest *req);
  void doRead(MemRequest *req, bool retrying);

  // UP
  void pushUp(MemRequest *req);
  void invalidate(MemRequest *req);

  // Status/state
  uint16_t getLineSize() const;

  bool canAcceptRead(DInst *dinst) const;
  bool canAcceptWrite(DInst *dinst) const;

  TimeDelta_t ffread(AddrType addr, DataType data);
  TimeDelta_t ffwrite(AddrType addr, DataType data);
  void        ffinvalidate(AddrType addr, int32_t lineSize);

  //TLB specific
  void readPage1(MemRequest *mreq);
  void readPage2(MemRequest *mreq);
  void readPage3(MemRequest *mreq);


  AddrType calcPage1Addr(AddrType addr){
    return (pgd_base + pgd_index(addr));
  }

  AddrType calcPage2Addr(AddrType addr){
    //returns pmd_base()+ pmd_pffset();
    return (pmd_base + pmd_index(addr));
  }

  AddrType calcPage3Addr(AddrType addr){
    //returns pte_base()+ pte_pffset();
    return (pte_base + pte_index(addr));
  }

  typedef CallbackMember1<TLB, MemRequest*, &TLB::readPage1> readPage1CB;
  typedef CallbackMember1<TLB, MemRequest*, &TLB::readPage2> readPage2CB;
  typedef CallbackMember1<TLB, MemRequest*, &TLB::readPage3> readPage3CB;

  typedef CallbackMember1<TLB, MemRequest *, &TLB::busRead>  busReadCB;
  typedef CallbackMember2<TLB, MemRequest *,bool, &TLB::doRead>   doReadCB;

  bool checkL2TLBHit(MemRequest *mreq);

};
#endif
//#endif
