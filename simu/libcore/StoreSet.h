// Contributed by David Munday
//                Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef STSET_H
#define STSET_H

#include "estl.h"
#include <vector>

#include "DInst.h"
#include "GStats.h"
#include "callback.h"

/* }}} */
#define STORESET_MERGING 1
#define STORESET_CLEARING 1
#define CLR_INTRVL 90000000

class StoreSet {
private:
  typedef std::vector<SSID_t>  SSIT_t;
  typedef std::vector<DInst *> LFST_t;

  SSIT_t SSIT;
  LFST_t LFST;

  SSID_t StoreSetSize;

#ifdef STORESET_CLEARING
  Time_t                                                          clear_interval;
  void                                                            clearStoreSetsTimer();
  StaticCallbackMember0<StoreSet, &StoreSet::clearStoreSetsTimer> clearStoreSetsTimerCB;
#endif

  AddrType hashPC(AddrType PC) const {
    //    return ((PC>>2) % 8191);
    return ((((PC >> 2) ^ (PC >> 11)) + PC) >> 2) & (StoreSetSize - 1);
  }

  bool isValidSSID(SSID_t SSID) const {
    return SSID != -1;
  }

  // SSIT Functions
  SSID_t get_SSID(AddrType PC) const {
    return SSIT[hashPC(PC)];
  };
  void clear_SSIT();

  // LFST Functions
  DInst *get_LFS(SSID_t SSID) const {
    I(SSID <= (int32_t)LFST.size());
    return LFST[SSID];
  };
  void set_LFS(SSID_t SSID, DInst *dinst) {
    LFST[SSID] = dinst;
  }
  void clear_LFST(void);

  SSID_t create_id();
  void   set_SSID(AddrType PC, SSID_t SSID) {
    SSIT[hashPC(PC)] = SSID;
  };
  SSID_t create_set(AddrType);

#if 1
  // TO - delete
  void stldViolation_withmerge(DInst *ld_dinst, DInst *st_dinst);
  void VPC_misspredict(DInst *ld_dinst, AddrType store_pc);
  void assign_SSID(DInst *dinst, SSID_t SSID);
#endif
public:
  StoreSet(const int32_t cpu_id);
  ~StoreSet() {
  }

  bool insert(DInst *dinst);
  void remove(DInst *dinst);
  void stldViolation(DInst *ld_dinst, AddrType st_pc);
  void stldViolation(DInst *ld_dinst, DInst *st_dinst);

  SSID_t mergeset(SSID_t id1, SSID_t id2);

#ifdef STORESET_MERGING
  // move violating load to qdinst load's store set, stores will migrate as violations occur.
  void merge_sets(DInst *m_dinst, DInst *d_dinst);
#else
  void merge_sets(DInst *m_dinst, DInst *d_dinst){};
#endif
};

#endif
