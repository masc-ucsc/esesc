// Contributed by Jose Renau
//                Luis Ceze
//                James Tuck
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

#include "MSHR.h"
#include "MemRequest.h"
#include "MemorySystem.h"
#include "SescConf.h"
#include "Snippets.h"

/* }}} */

MSHR::MSHR(const char *n, int32_t size, int16_t lineSize, int16_t nsub)
    /* baseline MSHR constructor {{{1 */
    : name(strdup(n))
    , Log2LineSize(log2i(lineSize))
    , nEntries(size)
    , nSubEntries(nsub)
    , avgUse("%s_MSHR_avgUse", n)
    , avgSubUse("%s_MSHR_avgSubUse", n)
    , nStallConflict("%s_MSHR:nStallConflict", name)
    , MSHRSize(roundUpPower2(size) * 4)
    , MSHRMask(MSHRSize - 1) {
  I(size > 0 && size < 1024 * 32 * 32);

  nFreeEntries = size;

  I(lineSize >= 0 && Log2LineSize < (8 * sizeof(AddrType) - 1));

  entry.resize(MSHRSize);

  for(int32_t i = 0; i < MSHRSize; i++) {
    entry[i].nUse = 0;
    I(entry[i].cc.empty());
  }
}

/* }}} */
bool MSHR::canAccept(AddrType addr) const
/* check if can accept new requests {{{1 */
{
  if(nFreeEntries <= 0)
    return false;

  uint32_t pos = calcEntry(addr);
  if(entry[pos].nUse >= nSubEntries)
    return false;

  return true;
}
/* }}} */
bool MSHR::canIssue(AddrType addr) const
/* check if can issue {{{1 */
{
  uint32_t pos = calcEntry(addr);
  if(entry[pos].nUse)
    return false;

  I(entry[pos].cc.empty());

  return true;
}
/* }}} */
void MSHR::addEntry(AddrType addr, CallbackBase *c, MemRequest *mreq)
/* add entry to wait for an address {{{1 */
{
  I(mreq->isRetrying());
  I(nFreeEntries <= nEntries);
  nFreeEntries--; // it can go negative because invalidate and writeback requests

  avgUse.sample(nEntries - nFreeEntries, mreq->getStatsFlag());

  uint32_t pos = calcEntry(addr);

  I(c);
  entry[pos].cc.add(c);

  I(nFreeEntries >= 0);

  I(entry[pos].nUse);
  entry[pos].nUse++;
  avgSubUse.sample(entry[pos].nUse, mreq->getStatsFlag());

  nStallConflict.inc();

#ifdef DEBUG
  I(!entry[pos].pending_mreq.empty());
  entry[pos].pending_mreq.push_back(mreq);
#endif
}
/* }}} */

void MSHR::blockEntry(AddrType addr, MemRequest *mreq)
/* add entry to wait for an address {{{1 */
{
  I(!mreq->isRetrying());
  I(nFreeEntries <= nEntries);
  nFreeEntries--; // it can go negative because invalidate and writeback requests

  avgUse.sample(nEntries - nFreeEntries, mreq->getStatsFlag());

  uint32_t pos = calcEntry(addr);
  I(nFreeEntries >= 0);

  I(entry[pos].nUse == 0);
  entry[pos].nUse++;
  avgSubUse.sample(entry[pos].nUse, mreq->getStatsFlag());

#ifdef DEBUG
  I(entry[pos].pending_mreq.empty());
  entry[pos].pending_mreq.push_back(mreq);
  entry[pos].block_mreq = mreq;
#endif
}
/* }}} */

bool MSHR::retire(AddrType addr, MemRequest *mreq)
/* retire, and check for deps {{{1 */
{
  uint32_t pos = calcEntry(addr);
  I(entry[pos].nUse);
#ifdef DEBUG
  I(!entry[pos].pending_mreq.empty());
  I(entry[pos].pending_mreq.front() == mreq);
  entry[pos].pending_mreq.pop_front();
  if(!entry[pos].pending_mreq.empty()) {
    MemRequest *mreq2 = entry[pos].pending_mreq.front();
    if(mreq2 != entry[pos].block_mreq)
      I(mreq2->isRetrying());
    else
      I(!mreq2->isRetrying());
  }
  if(entry[pos].block_mreq) {
    I(mreq == entry[pos].block_mreq);
    entry[pos].block_mreq = 0;
  }
#endif

  nFreeEntries++;
  I(nFreeEntries <= nEntries);

  I(entry[pos].nUse);
  entry[pos].nUse--;

  I(nFreeEntries >= 0);

  GI(entry[pos].nUse == 0, entry[pos].cc.empty());

  if(!entry[pos].cc.empty()) {
    entry[pos].cc.callNext();
    return true;
  }

  return false;
}
/* }}} */
void MSHR::dump() const
/* dump blocking state {{{1 */
{
  printf("MSHR[%s]", name);
  for(int i = 0; i < MSHRSize; i++) {
    if(entry[i].nUse)
      printf(" [%d].nUse=%d", i, entry[i].nUse);
    GI(entry[i].nUse == 0, entry[i].cc.empty());
    // GI(entry[i].cc.empty(), entry[i].nUse==0);
  }
  printf("\n");
}
/* }}} */
