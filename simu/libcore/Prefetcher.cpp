// Contributed by Jose Renau
//                Basilio Fraguela
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

#include <math.h>
#include <iostream>
#include <stdlib.h>

#include "FetchEngine.h"
#include "MemObj.h"
#include "Prefetcher.h"
#include "SescConf.h"

/* }}} */

//#define PREFETCH_HIST 1

Prefetcher::Prefetcher(MemObj *_l1, int cpuID)
    /* constructor {{{1 */
    : DL1(_l1)
    , avgPrefetchNum("P(%d)_pref_avgPrefetchNum", cpuID)
    , avgPrefetchConf("P(%d)_pref__avgPrefetchConf", cpuID)
    , histPrefetchDelta("P(%d)_pref__histPrefetchDelta", cpuID)
    , nextPrefetchCB(this) {
  const char *section = SescConf->getCharPtr("cpusimu", "prefetcher", cpuID);

  maxPrefetch = SescConf->getInt(section, "maxPrefetch");
  minDistance = SescConf->getInt(section, "minDistance");
  pfStride    = SescConf->getInt(section, "pfStride");

  SescConf->isBetween(section, "minDistance", 0, maxPrefetch);
  SescConf->isBetween(section, "pfStride", 1, 32); // Really?? More than 32??

  pending_prefetch    = false;
  pending_chain_fetch = 0;
  pending_preq_conf   = 0;
  curPrefetch         = 0;

  const char *dl1_section = DL1->getSection();
  int         bsize       = SescConf->getInt(dl1_section, "bsize");
  lineSizeBits            = log2i(bsize);

  const char *type = SescConf->getCharPtr(section, "type");

  if(strcasecmp(type, "stride") == 0) {
    pref_sign = PSIGN_STRIDE;

    apred = new StrideAddressPredictor();
  } else if(strcasecmp(type, "indirect") == 0) {
    pref_sign = PSIGN_STRIDE; // STRIDE, Indirect are generated inside the try_chain

    apred = new IndirectAddressPredictor();
  } else if(strcasecmp(type, "tage") == 0) {
    pref_sign = PSIGN_TAGE;

    uint64_t bimodalSize = SescConf->getInt(section, "bimodalSize");
    SescConf->isInt(section, "bimodalSize");
    SescConf->isGT(section, "bimodalSize", 1);
    SescConf->isPower2(section, "bimodalSize");

    uint64_t FetchWidth = SescConf->getInt("cpusimu", "fetchWidth");
    SescConf->isInt("cpusimu", "fetchWidth");
    SescConf->isGT("cpusimu", "fetchWidth", 1);
    SescConf->isPower2("cpusimu", "fetchWidth");

    uint64_t Log2FetchWidth = log2(FetchWidth);

    uint64_t bwidth = SescConf->getInt(section, "bimodalWidth");
    SescConf->isGT(section, "bimodalWidth", 1);

    uint64_t nhist = SescConf->getInt(section, "nhist");
    SescConf->isGT(section, "nhist", 1);

    apred = new vtage(bimodalSize, Log2FetchWidth, bwidth, nhist);
  } else if(strcasecmp(type, "void") == 0) {
    apred = 0;
  } else {
    SescConf->notCorrect(); // Unknown prefetcher
  }

  MSG("****PREFETCHING****%s \n", section);
}
/* }}} */

void Prefetcher::exe(DInst *dinst)
/* forward bus read {{{1 */
{
  if(apred == 0)
    return;

  uint16_t conf = apred->exe_update(dinst->getPC(), dinst->getAddr(), dinst->getData());

  GI(!pending_prefetch, pending_preq_conf == 0);

  if(pending_preq_pc == dinst->getPC() && pending_preq_conf > (conf / 2))
    return; // Do not kill itself

  static uint8_t rnd_xtra_conf = 0;
  rnd_xtra_conf                = 0; // (rnd_xtra_conf>>1) ^ (dinst->getPC()>>5) ^ (dinst->getAddr()>>2) ^ (dinst->getPC()>>7);
  if(conf <= (pending_preq_conf + (rnd_xtra_conf & 0x7)) || conf <= 4)
    return; // too low of a chance

  avgPrefetchConf.sample(conf, dinst->getStatsFlag());
  if(pending_prefetch && (curPrefetch - minDistance) > 0)
    avgPrefetchNum.sample(curPrefetch - minDistance, pending_statsFlag);

  pending_preq_pc   = dinst->getPC();
  pending_preq_conf = conf;
  pending_statsFlag = dinst->getStatsFlag();
  if(dinst->getChained()) {
    I(dinst->getFetchEngine());
    curPrefetch         = dinst->getChained();
    pending_chain_fetch = dinst->getFetchEngine();
  } else {
    I(!dinst->getFetchEngine());
    curPrefetch         = minDistance;
    pending_chain_fetch = 0;
  }
  pending_preq_addr = dinst->getAddr();

  if(!pending_prefetch) {
    pending_prefetch = true;
    nextPrefetchCB.schedule(1);
  }
}
/* }}} */

void Prefetcher::ret(DInst *dinst)
// {{{1 update prefetcher state at retirement
{
  if(apred == 0)
    return;

  int ret = apred->ret_update(dinst->getPC(), dinst->getAddr(), dinst->getData());
  if(ret) {
    dinst->markPrefetch();
  }
}
// 1}}}

void Prefetcher::nextPrefetch()
// {{{1 Method called to trigger a prefetch
{
  I(apred);

  if(!pending_prefetch)
    return;

  if(pending_preq_conf > 0)
    pending_preq_conf--;

  curPrefetch++;

  if(curPrefetch >= maxPrefetch || pending_preq_conf <= 1) {
    pending_prefetch    = false;
    pending_chain_fetch = 0;
    pending_preq_conf   = 0;
    avgPrefetchNum.sample(curPrefetch - minDistance - 1, pending_statsFlag);
    return;
  }

  AddrType paddr;
  if(pending_chain_fetch) {
    paddr = apred->predict(pending_preq_pc, curPrefetch + 4, false);

  } else {
    paddr = apred->predict(pending_preq_pc, curPrefetch + (curPrefetch - minDistance) * (pfStride - 1), true);
  }
  if((paddr >> 12) == 0) {
    bool chain = apred->try_chain_predict(DL1, pending_preq_pc, curPrefetch + (curPrefetch - minDistance) * (pfStride - 1));
    if(!chain) {
      if((curPrefetch - minDistance - 1) > 0)
        avgPrefetchNum.sample(curPrefetch - minDistance - 1, pending_statsFlag);
      pending_prefetch    = false;
      pending_chain_fetch = 0;
      pending_preq_conf   = 0;
      return;
    }
  } else {
#ifdef PREFETCH_HIST
    histPrefetchDelta.sample(pending_statsFlag, (paddr - pending_preq_addr), 1);
#endif
    pending_preq_addr = paddr;
    CallbackBase *cb  = 0;
    if(pending_chain_fetch) {
      cb = FetchEngine::chainPrefDoneCB::create(pending_chain_fetch, pending_preq_pc, curPrefetch + 4, paddr);
    }
    DL1->tryPrefetch(paddr, pending_statsFlag, curPrefetch, pref_sign, pending_preq_pc, cb);
  }

  if(paddr == pending_preq_addr) { // Offset 0
    pending_prefetch    = false;
    pending_chain_fetch = 0;
    pending_preq_conf   = 0;
    return;
  }

  I(pending_prefetch);
  nextPrefetchCB.schedule(1);
}
// 1}}}
