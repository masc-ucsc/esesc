// Contributed by Jose Renau
//                Luis Ceze
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

#ifndef MSHR_H
#define MSHR_H

#include "estl.h"
#include <queue>

#include "DInst.h"
#include "GStats.h"
#include "callback.h"
#include "nanassert.h"
#include "pool.h"

#include "MSHRentry.h"

/* }}} */

class MemRequest;

class MSHR {
private:
protected:
  const char *   name;
  const uint32_t Log2LineSize;
  const int32_t  nEntries;
  const int32_t  nSubEntries;

  int32_t nFreeEntries;

  GStatsAvg avgUse;
  GStatsAvg avgSubUse;

  AddrType calcLineAddr(AddrType addr) const {
    return addr >> Log2LineSize;
  }

  GStatsCntr nStallConflict;

  const int32_t MSHRSize;
  const int32_t MSHRMask;

  // If a non-integer type is defined, the MSHR template should accept
  // a hash function as a parameter

  // Running crafty, I verified that the current hash function
  // performs pretty well (due to the extra space on MSHRSize). It
  // performs only 5% worse than an oversize prime number (noise).
  uint32_t calcEntry(AddrType paddr) const {
    uint64_t p = paddr >> Log2LineSize;
    return (p ^ (p >> 11)) & MSHRMask;
  }

  class EntryType {
  public:
    CallbackContainer cc;
    int32_t           nUse;
#ifdef DEBUG
    std::deque<MemRequest *> pending_mreq;
    MemRequest *             block_mreq;
#endif
  };

  std::vector<EntryType> entry;

public:
  MSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);
  virtual ~MSHR() {
  }
  bool hasFreeEntries() const {
    return (nFreeEntries > 0);
  }

  bool canAccept(AddrType paddr) const;
  bool canIssue(AddrType addr) const;
  void addEntry(AddrType addr, CallbackBase *c, MemRequest *mreq);
  void blockEntry(AddrType addr, MemRequest *mreq);
  bool retire(AddrType addr, MemRequest *mreq);
  void dump() const;
};

#endif // MSHR_H
