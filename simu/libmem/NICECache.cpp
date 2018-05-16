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

#include "MemRequest.h"
#include "MemorySystem.h"
#include "SescConf.h"

#include "NICECache.h"
/* }}} */
NICECache::NICECache(MemorySystem *gms, const char *section, const char *sName)
    /* dummy constructor {{{1 */
    : MemObj(section, sName)
    , hitDelay(SescConf->getInt(section, "hitDelay"))
    , bsize(SescConf->getInt(section, "bsize"))
    , bsizeLog2(log2i(SescConf->getInt(section, "bsize")))
    , coldWarmup(SescConf->getBool(section, "coldWarmup"))
    , readHit("%s:readHit", sName)
    , pushDownHit("%s:pushDownHit", sName)
    , writeHit("%s:writeHit", sName)
    , readMiss("%s:readMiss", sName)
    , readHalfMiss("%s:readHalfMiss", sName)
    , writeMiss("%s:writeMiss", sName)
    , writeHalfMiss("%s:writeHalfMiss", sName)
    , writeExclusive("%s:writeExclusive", sName)
    , writeBack("%s:writeBack", sName)
    , avgMemLat("%s_avgMemLat", sName) {

  // FIXME: the hitdelay should be converted to dyn_hitDelay to support DVFS

  warmupStepStart = 256 / 4;
  warmupStep      = warmupStepStart;
  warmupNext      = 16;
  warmupSlowEvery = 16;
}
/* }}} */

void NICECache::doReq(MemRequest *mreq)
/* read (down) {{{1 */
{
  TimeDelta_t hdelay = hitDelay;

  if(mreq->isWarmup())
    hdelay = 1;

  readHit.inc(mreq->getStatsFlag());

  if(mreq->isHomeNode()) {
    if(coldWarmup && warmup.find(mreq->getAddr() >> bsizeLog2) == warmup.end()) {

      TimeDelta_t lat;
      warmupNext--;
      if(warmupNext <= 0) {
        warmupNext = warmupSlowEvery;
        warmupStep--;
        if(warmupStep <= 0) {
          warmupSlowEvery = warmupSlowEvery >> 1;
          if(warmupSlowEvery <= 0)
            coldWarmup = false;
          warmupStepStart = warmupStepStart << 1;
          warmupStep      = warmupStepStart;
        }
      } else {
        hdelay = 1;
      }
      warmup.insert(mreq->getAddr() >> bsizeLog2);
    }
    mreq->ack(hdelay);
    return;
  }
  if(mreq->getAction() == ma_setValid || mreq->getAction() == ma_setExclusive) {
    mreq->convert2ReqAck(ma_setExclusive);
    // MSG("rdnice %x",mreq->getAddr());
  } else {
    mreq->convert2ReqAck(ma_setDirty);
    // MSG("wrnice %x",mreq->getAddr());
  }

  if(coldWarmup && warmup.find(mreq->getAddr() >> bsizeLog2) == warmup.end()) {
    warmup.insert(mreq->getAddr() >> bsizeLog2);
    TimeDelta_t lat;
    warmupNext--;
    if(warmupNext <= 0) {
      warmupNext = warmupSlowEvery;
      warmupStep--;
      if(warmupStep <= 0) {
        warmupSlowEvery = warmupSlowEvery >> 1;
        if(warmupSlowEvery <= 0)
          coldWarmup = false;
        warmupStepStart = warmupStepStart << 1;
        warmupStep      = warmupStepStart;
      }
    } else {
      hdelay = 1;
    }
  }
  avgMemLat.sample(hdelay, mreq->getStatsFlag());
  readHit.inc(mreq->getStatsFlag());
  router->scheduleReqAck(mreq, hdelay);
}
/* }}} */

void NICECache::doReqAck(MemRequest *req)
/* req ack {{{1 */
{
  I(0);
}
// 1}}}

void NICECache::doSetState(MemRequest *req)
/* change state request  (up) {{{1 */
{
  I(0);
}
/* }}} */

void NICECache::doSetStateAck(MemRequest *req)
/* push (down) {{{1 */
{
  I(0);
}
/* }}} */

void NICECache::doDisp(MemRequest *mreq)
/* push (up) {{{1 */
{
  writeHit.inc(mreq->getStatsFlag());
  mreq->ack(hitDelay);
}
/* }}} */

bool NICECache::isBusy(AddrType addr) const
/* can accept reads? {{{1 */
{
  return false;
}
/* }}} */

void NICECache::tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* drop prefetch {{{1 */
{
  if(cb)
    cb->destroy();
}
/* }}} */

TimeDelta_t NICECache::ffread(AddrType addr)
/* warmup fast forward read {{{1 */
{
  return 1;
}
/* }}} */

TimeDelta_t NICECache::ffwrite(AddrType addr)
/* warmup fast forward writed {{{1 */
{
  return 1;
}
/* }}} */
