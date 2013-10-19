/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by David Munday
                  Jose Renau

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

#ifndef STSET_H
#define STSET_H

#include <vector>
#include "estl.h"

#include "GStats.h"
#include "callback.h"
#include "DInst.h"

/* }}} */
#define STORESET_MERGING 1
#define STORESET_CLEARING 1
#define CLR_INTRVL 100000000

class StoreSet {
private:

  typedef std::vector<SSID_t>  SSIT_t;
  typedef std::vector<DInst *> LFST_t;

  SSIT_t SSIT;
  LFST_t LFST;

  SSID_t StoreSetSize;

#ifdef STORESET_CLEARING
  Time_t  clear_interval;
  void clearStoreSetsTimer();
  StaticCallbackMember0<StoreSet,&StoreSet::clearStoreSetsTimer> clearStoreSetsTimerCB;
#endif

  AddrType hashPC(AddrType PC) const {
//    return ((PC>>2) % 8191);
    return ((((PC>>2) ^ (PC>>11)) + PC)>>2) & (StoreSetSize-1);
  }

  bool isValidSSID(SSID_t SSID) const {
    return SSID != -1;
  }

  // SSIT Functions
  SSID_t get_SSID(AddrType PC) const        { return SSIT[hashPC(PC)]; };
  void   clear_SSIT();

  // LFST Functions
  DInst *get_LFS(SSID_t SSID) const  {
    I(SSID<=(int32_t)LFST.size());
    return LFST[SSID];
  };
  void set_LFS(SSID_t SSID, DInst *dinst) { LFST[SSID] = dinst; }
  void clear_LFST(void);

  SSID_t create_id();
  void set_SSID(AddrType PC, SSID_t SSID) { SSIT[hashPC(PC)] = SSID; };
  SSID_t create_set(AddrType);

#if 1
  // TO - delete
  void stldViolation_withmerge(DInst *ld_dinst, DInst *st_dinst);
  void VPC_misspredict(DInst *ld_dinst, AddrType store_pc);
  void assign_SSID(DInst *dinst, SSID_t SSID);
#endif
public:
  StoreSet(const int32_t cpu_id);
  ~StoreSet() { }

  bool insert(DInst *dinst);
  void remove(DInst *dinst);
  void stldViolation(DInst *ld_dinst, AddrType st_pc);
  void stldViolation(DInst *ld_dinst, DInst *st_dinst);


  SSID_t mergeset(SSID_t id1, SSID_t id2);



#ifdef STORESET_MERGING
  // move violating load to qdinst load's store set, stores will migrate as violations occur.
  void merge_sets(DInst *m_dinst, DInst *d_dinst);
#else
  void merge_sets(DInst *m_dinst, DInst *d_dinst) { };
#endif

};

#endif
