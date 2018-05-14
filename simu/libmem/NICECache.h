// includes and copyright notice {{{1
// Contributed by Jose Renau
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

#ifndef NICECACHE_H
#define NICECACHE_H

#include "GStats.h"
#include "MemObj.h"
#include "estl.h"
#include "nanassert.h"
/* }}} */

class NICECache : public MemObj {
private:
  // a 100% hit cache, used for debugging or as main memory
  const uint32_t hitDelay;
  const uint32_t bsize;
  const uint32_t bsizeLog2;

  bool coldWarmup;

  HASH_SET<uint32_t> warmup;
  uint32_t           warmupStepStart;
  uint32_t           warmupStep;
  uint32_t           warmupNext;
  uint32_t           warmupSlowEvery;

protected:
  // BEGIN Statistics
  GStatsCntr readHit;
  GStatsCntr pushDownHit;
  GStatsCntr writeHit;

  // The following statistics don't make any sense for a niceCache, but are instantiated
  // for compatibility, and to supress bogus warnings from the PowerModel about missing
  // statistics for the NICECache.

  GStatsCntr readMiss;
  GStatsCntr readHalfMiss;
  GStatsCntr writeMiss;
  GStatsCntr writeHalfMiss;
  GStatsCntr writeExclusive;
  GStatsCntr writeBack;
  GStatsAvg  avgMemLat;

public:
  NICECache(MemorySystem *gms, const char *section, const char *name = NULL);

  // Entry points to schedule that may schedule a do?? if needed
  void req(MemRequest *req) {
    doReq(req);
  };
  void reqAck(MemRequest *req) {
    doReqAck(req);
  };
  void setState(MemRequest *req) {
    doSetState(req);
  };
  void setStateAck(MemRequest *req) {
    doSetStateAck(req);
  };
  void disp(MemRequest *req) {
    doDisp(req);
  }

  // This do the real work
  void doReq(MemRequest *r);
  void doReqAck(MemRequest *req);
  void doSetState(MemRequest *req);
  void doSetStateAck(MemRequest *req);
  void doDisp(MemRequest *req);

  void tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb = 0);

  TimeDelta_t ffread(AddrType addr);
  TimeDelta_t ffwrite(AddrType addr);

  bool isBusy(AddrType addr) const;
};

#endif
