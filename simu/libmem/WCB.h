// Contributed by Luis Ceze
//                Karin Strauss
//                Jose Renau
//                David Munday
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

#ifndef WCB_H
#define WCB_H

#include "CacheCore.h"
#include "DInst.h"
#include "GProcessor.h"
#include "GStats.h"
#include "MSHR.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "Port.h"
#include "Snippets.h"
#include "estl.h"

#include "OoOProcessor.h"

enum valid_status_t { INVALID, VALID, PENDING };

class WCBLine {
public:
  void reset(int sz) {
    stID.resize(sz);
    data.resize(sz);
    present.resize(sz);
    storepc.resize(sz);
    storeset_merge_set.resize(sz);
    storeset_merge_pc.resize(sz);
    for(int32_t i = 0; i < sz; i++) {
      stID[i]               = 0;
      data[i]               = 0;
      storepc[i]            = 0;
      storeset_merge_set[i] = -1;
      storeset_merge_pc[i]  = 0;
      present[i]            = false;
    }
    tag   = 0;
    valid = INVALID;
  }
  std::vector<Time_t>   stID;
  std::vector<DataType> data;
  std::vector<AddrType> storepc;            // pc of store that wrote this word
  std::vector<SSID_t>   storeset_merge_set; // if MAX_SSID no merging, else merging on for this word
  std::vector<AddrType> storeset_merge_pc;
  std::vector<bool>     present; // 1 bit per word

  AddrType       tag;
  valid_status_t valid; // cache line available?
  void           copy(WCBLine *l) {
    stID    = l->stID;
    data    = l->data;
    storepc = l->storepc;
    present = l->present;
    tag     = l->tag;
    valid   = l->valid;
  }
};

class WCB {
protected:
  const AddrType log2AddrLs;
  const AddrType wordMask;
  const uint32_t nWords;
  const uint32_t PatchSize;
  const uint32_t PatchMask;

  GStatsCntr WCBreplayXcoreStore;

  std::vector<DataType> patchdata;
  std::vector<AddrType> patchaddr;
  std::vector<AddrType> patchid;
  std::vector<AddrType> patchpc;
  std::vector<bool>     xCoreDataPresent;

  uint32_t patchhash(AddrType addr) {
    return (addr >> 2) & PatchMask;
  }

public:
  WCBLine  displaced;
  AddrType incorrect_pc;

  WCB(int32_t sz, int32_t l, const char *name)
      : log2AddrLs(l)
      , wordMask((1 << (l)) - 1)
      , nWords(1 << l)
      , PatchSize(sz)
      , PatchMask(sz - 1)
      , WCBreplayXcoreStore("%s_WCBreplayXcoreStore", name) {

    displaced.reset(nWords);

    patchdata.resize(PatchSize);
    patchaddr.resize(PatchSize);
    patchid.resize(PatchSize);
    patchpc.resize(PatchSize);
    xCoreDataPresent.resize(PatchSize);
  }

  uint32_t getnWords() const {
    return nWords;
  }
  int32_t calcWord(AddrType addr) const {
    return (addr & wordMask) >> 2;
  }

  bool loadCheck(const DInst *dinst, bool &correct) { // return value indicates WCB hit/miss
    DataTypedata = dinst->getData();

    if(patchaddr[patchhash(dinst->getAddr())] == dinst->getAddr()) {
      DataType data2 = patchdata[patchhash(dinst->getAddr())];
      correct        = (data == data2);
      if(!correct) {
        incorrect_pc = patchpc[patchhash(dinst->getAddr())];
        if(xCoreDataPresent[patchhash(dinst->getAddr())])
          WCBreplayXcoreStore.inc(dinst->getStatsFlag());
      }
      return true;
    }
    return false;
  }

  void clear(std::vector<AddrType> tags) {
    for(uint32_t j = 0; j < PatchSize; j++) {
      tags.push_back(patchaddr[j] >> log2AddrLs);
      patchaddr[j] = 0;
    }
  }

  bool write(DInst *dinst) { // return true if a displacement occurs

    bool disp = false;

#if 0
    // Strange, but it is better to allow OoO updates without checking
    if (patchaddr[patchhash(dinst->getAddr())] == dinst->getAddr() && patchid[patchhash(dinst->getAddr())] > dinst->getID())
      return false; // Nothing to write as it is already newer
#endif

    if((patchaddr[patchhash(dinst->getAddr())] != dinst->getAddr() && patchaddr[patchhash(dinst->getAddr())] != 0) ||
       (patchaddr[patchhash(dinst->getAddr())] == dinst->getAddr() && patchid[patchhash(dinst->getAddr())] < dinst->getID())) {
      disp = true;

      uint32_t w = calcWord(patchdata[patchhash(dinst->getAddr())]);

      for(uint32_t j = 0; j < nWords; j++) {
        displaced.stID[j]    = 0;
        displaced.data[j]    = 0;
        displaced.storepc[j] = 0;
        displaced.present[j] = false;
      }
      displaced.tag        = patchaddr[patchhash(dinst->getAddr())] >> log2AddrLs;
      displaced.stID[w]    = patchid[patchhash(dinst->getAddr())];
      displaced.data[w]    = patchdata[patchhash(dinst->getAddr())];
      displaced.storepc[w] = patchpc[patchhash(dinst->getAddr())];
      displaced.valid      = VALID;
    }

    patchdata[patchhash(dinst->getAddr())]        = dinst->getData();
    patchaddr[patchhash(dinst->getAddr())]        = dinst->getAddr();
    patchid[patchhash(dinst->getAddr())]          = dinst->getID();
    patchpc[patchhash(dinst->getAddr())]          = dinst->getPC();
    xCoreDataPresent[patchhash(dinst->getAddr())] = false;

    return disp;
  }

  bool writeAddress(const DInst *dinst) {
    return false;
  }

  void checkSetXCoreStore(AddrType addr) {
    if(patchaddr[patchhash(addr)] == addr) { // addr exists in WCB
      xCoreDataPresent[patchhash(addr)] = true;
      // patchdata[patchhash(addr)] = 0xDEADBEEF; //force a replay
    }
  }
};

#endif
