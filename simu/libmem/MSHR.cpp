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

#include "Snippets.h"
#include "SescConf.h"
#include "MemorySystem.h"
#include "MSHR.h"

/* }}} */

MSHR *MSHR::create(const char *name, const char *type, int32_t size, int16_t lineSize, int16_t nSubEntries)
  /* internal create method {{{1 */
{
  MSHR *mshr=0;

  if(strcasecmp(type, "blocking") == 0) {
    mshr = new BlockingMSHR(name, size, lineSize, nSubEntries);
  } else if(strcasecmp(type, "full") == 0) {
    mshr = new FullMSHR(name, size, lineSize, nSubEntries);
  } else if(strcasecmp(type, "none") == 0) {
    mshr = new NoMSHR(name, size, lineSize, nSubEntries);
//  } else if(strcmp(type, "single") == 0) {
//    mshr = new SingleMSHR(name, size, lineSize, , nSubEntrie);
  } else {
    MSG("WARNING:MSHR: type \"%s\" unknown, defaulting to \"full\"", type);
    mshr = new FullMSHR(name, size, lineSize, nSubEntries);
  }
  I(mshr);

  return mshr;
}
/* }}} */

MSHR *MSHR::create(const char *name, const char *section, int16_t lineSize)
  /* main entry point to create MSHRs {{{1 */
{
  MSHR *mshr;

  const char *type    = SescConf->getCharPtr(section, "type");
  int32_t size        = SescConf->getInt(section, "size");
  int16_t nSubEntries = SescConf->getInt(section, "nSubEntries");

  mshr = MSHR::create(name, type, size, lineSize, nSubEntries);

  return mshr;
}
/* }}} */

MSHR::MSHR(const char *n, int32_t size, int16_t lineSize, int16_t nsub)
  /* baseline MSHR constructor {{{1 */
  :name(strdup(n))
  ,Log2LineSize(log2i(lineSize))
  ,nEntries(size)
  ,nSubEntries(nsub)
  ,avgUse("%s_MSHR_avgUse", n)
  ,avgSubUse("%s_MSHR_avgSubUse", n)
  ,nOverflows("%s_MSHR:nOverflows", n)
{
  I(size>0 && size<1024*32);

  nFreeEntries = size;
}
/* }}} */

// BlockingMSHR class (all the deps in the overflow) == blocking cache

BlockingMSHR::BlockingMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nsub)
  /* constructor {{{1 */
  : MSHR(name, size, lineSize, nsub)
{
}
/* }}} */
bool BlockingMSHR::canAccept(AddrType addr) const
/* check if can accept new requests {{{1 */
{
  // sine it is blocking, it can not accept anything until the last one finishes
  return overflow.empty();
}
/* }}} */
bool BlockingMSHR::canIssue(AddrType addr) const
/* check if can issue {{{1 */
{
  return overflow.empty();
}
/* }}} */
void BlockingMSHR::addEntry(AddrType addr, CallbackBase *c)
  /* add entry to wait for an address {{{1 */
{
  OverflowField f;
  f.addr    = addr;
  f.cb      = c;
  overflow.push_back(f);

  //MSG("Added %llx at position %d",addr,overflow.size());
  avgUse.sample(overflow.size()); // Only one allowed, but blocking has tons of overflow (stats to track)
}
/* }}} */
void BlockingMSHR::retire(AddrType addr)
  /* retire, and check for deps {{{1 */
{
  //MSG("Retiring = %llx",addr);
  
  uint32_t i = 0;
  while(!overflow.empty()) {
    OverflowField f = overflow.front();
    overflow.pop_front();
    i++;
    if (f.cb) { // cb is optional
      f.cb->call();
      //MSG("Return Retired %llx from position %d ",addr,i);
      //dump();
      return;
    }
  }
  //MSG("Retired %llx from position %d ",addr,i);
  //dump();
}
/* }}} */
void BlockingMSHR::dump() const
  /* dump blocking state {{{1 */
{
  fprintf(stderr, "blockingMSHR[%s]", name);
  if (overflow.empty())
    fprintf(stderr, " nothing pending\n");
  else
    fprintf(stderr, " overflowing 0x%lx %p\n", overflow.front().addr, overflow.front().cb);
}
/* }}} */

// BlockingMSHR class (all the deps in the overflow) == blocking cache

NoMSHR::NoMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nsub)
  /* constructor {{{1 */
  : MSHR(name, size, lineSize, nsub)
{
}
/* }}} */
bool NoMSHR::canAccept(AddrType addr) const
/* check if can accept new requests {{{1 */
{
  return true;
}
/* }}} */
bool NoMSHR::canIssue(AddrType addr) const
/* check if can issue {{{1 */
{
  return true;
}
/* }}} */
void NoMSHR::addEntry(AddrType addr, CallbackBase *c)
  /* add entry to wait for an address {{{1 */
{
  OverflowField f;
  f.addr    = addr;
  f.cb      = c;
  overflow.push_back(f);

  avgUse.sample(overflow.size());
}
/* }}} */
void NoMSHR::retire(AddrType addr)
  /* retire, and check for deps {{{1 */
{
  while(!overflow.empty()) {
    OverflowField f = overflow.front();
    overflow.pop_front();

    if (f.cb) { // cb is optional
      f.cb->call();
      return;
    }
  }
}
/* }}} */
void NoMSHR::dump() const
  /* dump blocking state {{{1 */
{
  printf("NoMSHR[%s]", name);
  if (overflow.empty())
    printf(" nothing pending\n");
  else
    printf(" overflowing 0x%lx %p\n", overflow.front().addr, overflow.front().cb);
}
/* }}} */

// FullMSHR class

FullMSHR::FullMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nsub)
  /* constructor {{{1 */
  : MSHR(name, size, lineSize,nsub)
  ,nStallConflict("%s_MSHR:nStallConflict", name)
  ,MSHRSize(roundUpPower2(size)*4)
  ,MSHRMask(MSHRSize-1)
{
  nFreeEntries = size;
  I(lineSize>=0 && Log2LineSize<(8*sizeof(AddrType)-1));

  entry.resize(MSHRSize);

  for(int32_t i=0;i<MSHRSize;i++) {
    entry[i].nUse = 0;
    I(entry[i].cc.empty());
  }
}
/* }}} */
bool FullMSHR::canAccept(AddrType addr) const
/* check if can accept new requests {{{1 */
{
  if (nFreeEntries <= 0)
    return false;

  uint32_t pos = calcEntry(addr);
  if (entry[pos].nUse >= nSubEntries )
    return false;

  return true;
}
/* }}} */
bool FullMSHR::canIssue(AddrType addr) const
/* check if can issue {{{1 */
{
  uint32_t pos = calcEntry(addr);
  if (entry[pos].nUse)
    return false;

  I(entry[pos].cc.empty());

  return true;
}
/* }}} */
void FullMSHR::addEntry(AddrType addr, CallbackBase *c)
  /* add entry to wait for an address {{{1 */
{
  I(nFreeEntries <= nEntries);
  nFreeEntries--; // it can go negative because invalidate and writeback requests

  avgUse.sample(nEntries-nFreeEntries);

  uint32_t pos = calcEntry(addr);

  if(c)
    entry[pos].cc.add(c);

  I(nFreeEntries>-1000);
  if (entry[pos].nUse) {
    nStallConflict.inc();
  }
  entry[pos].nUse++;
  avgSubUse.sample(entry[pos].nUse);
}
/* }}} */
void FullMSHR::retire(AddrType addr)
  /* retire, and check for deps {{{1 */
{
  uint32_t pos = calcEntry(addr);
  if(entry[pos].nUse==0)
    return;

  nFreeEntries++;
  I(nFreeEntries <= nEntries);

  I(entry[pos].nUse);
  entry[pos].nUse--;

  I(nFreeEntries>-1000);

  if (!entry[pos].cc.empty()) {
    entry[pos].cc.callNext();
    GI(entry[pos].nUse==0, entry[pos].cc.empty());
    if (entry[pos].cc.empty()) {
      nFreeEntries   += entry[pos].nUse;
      entry[pos].nUse = 0;
    }
  }else{
    nFreeEntries   += entry[pos].nUse;
    entry[pos].nUse = 0;
  }

  GI(entry[pos].nUse==0, entry[pos].cc.empty());
  GI(entry[pos].cc.empty(), entry[pos].nUse==0);
}
/* }}} */
void FullMSHR::dump() const
  /* dump blocking state {{{1 */
{
  printf("FullMSHR[%s]",name);
  for(int i=0;i<MSHRSize;i++) {
    if (entry[i].nUse)
      printf(" [%d].nUse=%d",i, entry[i].nUse);
    GI(entry[i].nUse==0   , entry[i].cc.empty());
    //GI(entry[i].cc.empty(), entry[i].nUse==0);
  }
  printf("\n");
}
/* }}} */

#if 0

SingleMSHR<Cache_t>::SingleMSHR(const char *name, int32_t size,
                                        int32_t lineSize, int32_t nrd, int32_t nwr)
  : MSHR(name, size, lineSize),
    nReads(nrd),
    nWrites(nwr),
    avgOverflowConsumptions("%s_MSHR_avgOverflowConsumptions", name),
    maxOutsReqs("%s_MSHR_maxOutsReqs", name),
    avgReqsPerLine("%s_MSHR_avgReqsPerLine", name),
    nIssuesNewEntry("%s_MSHR:nIssuesNewEntry", name),
    avgQueueSize("%s_MSHR_avgQueueSize", name),
    avgWritesPerLine("%s_MSHR_avgWritesPerLine", name),
    avgWritesPerLineComb("%s_MSHR_avgWritesPerLineComb", name),
    nOnlyWrites("%s_MSHR:nOnlyWrites", name),
    nRetiredEntries("%s_MSHR:nRetiredEntries", name),
    nRetiredEntriesWritten("%s_MSHR:nRetiredEntriesWritten", name)
{
  nFreeEntries = nEntries;
  nFullReadEntries = 0;
  nFullWriteEntries = 0;
  checkingOverflow = false;
  nOutsReqs = 0;
}


bool SingleMSHR<Cache_t>::issue(AddrType addr)
{
  MSHRit it = ms.find(calcLineAddr(addr));

  nUse.inc();

  if(!overflow.empty())
    return false;

  I(nFreeEntries >= 0 && nFreeEntries <=nEntries);

  if(it == ms.end()) {
    if(nFreeEntries > 0) {
      ms[calcLineAddr(addr)].setup(nSubEntries);
      nFreeEntries--;

      nOutsReqs++;

      nIssuesNewEntry.inc();
      avgQueueSize.sample(0);

      return true;
    }
  }

  return false;
}


void SingleMSHR< Cache_t>::toOverflow(AddrType addr, CallbackBase *c, CallbackBase *ovflwc)
{
  OverflowField f;

  f.addr   = addr;
  f.cb      = c;
  f.ovflwcb = ovflwc;

  overflow.push_back(f);
  nOverflows.inc();
}


void SingleMSHR::checkOverflow()
{
  if(overflow.empty()) //nothing to do
    return;

  if(checkingOverflow) // i am already checking the overflow
    return;

  I(!overflow.empty());
  I(!checkingOverflow);

  checkingOverflow = true;

  int32_t nConsumed = 0;

  do {
    OverflowField f = overflow.front();
    MSHRit it = ms.find(calcLineAddr(f.addr));

    if(it == ms.end()) {
      if(nFreeEntries > 0) {
        ms[calcLineAddr(f.addr)].setup(nSubEntries);
        nFreeEntries--;

        nConsumed++;
        nOutsReqs++;

        f.ovflwcb->call();
        f.cb->destroy();
        overflow.pop_front();
        nIssuesNewEntry.inc();
        avgQueueSize.sample(0);
      } else {
        break;
      }
    } else { // just try to add the entry
      if((*it).second.addRequest(f.addr, f.cb, f.mo)) {
        // succesfully accepted entry, but no need to call the callback
        // since there was already an entry pending for the same line
        avgQueueSize.sample((*it).second.getPendingReqs() - 1);
        f.ovflwcb->destroy();
        overflow.pop_front();
        nOutsReqs++;

      } else {
        break;
      }
    }
  } while(!overflow.empty());

  if(nConsumed)
    avgOverflowConsumptions.sample(nConsumed);

  checkingOverflow = false;
}


void SingleMSHR::addEntry(AddrType addr, CallbackBase *c, CallbackBase *ovflwc)
{
  I(ovflwc); // for single MSHR, overflow handler REQUIRED!

  if(!overflow.empty()) {
    toOverflow(addr, c, ovflwc);
    return;
  }

  MSHRit it = ms.find(calcLineAddr(addr));
  if(it == ms.end())  {// we must be overflowing because the issue did not happen
    toOverflow(addr, c, ovflwc);
    return;
  }

  I(it != ms.end());

  if((*it).second.addRequest(addr, c, mo)) {
    // ok, the addrequest succeeded, the request was added
    avgQueueSize.sample((*it).second.getPendingReqs() - 1);
    nOutsReqs++;

    // there was no overflow, so the callback needs to be destroyed
    ovflwc->destroy();
    return;
  } else {
    // too many oustanding requests to the same line already. send to overflow
    toOverflow(addr, c, ovflwc);
    return;
  }
}


bool SingleMSHR::retire(AddrType addr)
{
  bool rmEntry = false;

  MSHRit it = ms.find(calcLineAddr(addr));
  I(it != ms.end());
  I(calcLineAddr(addr) == (*it).second.getLineAddr());

  maxOutsReqs.sample(nOutsReqs);
  nOutsReqs--;

  //MSG("[%llu] nFullSubE=%d a=%lu",globalClock,nFullReadEntries,calcLineAddr(addr));

  rmEntry = (*it).second.retire();
  if(rmEntry) {
    // the last pending request for the MSHRentry was completed
    // recycle the entry
    nRetiredEntries.inc();
    avgReqsPerLine.sample((*it).second.getUsedReads() + (*it).second.getUsedWrites());
    maxUsedEntries.sample(nEntries - nFreeEntries);
    avgWritesPerLine.sample((*it).second.getUsedWrites());
    avgWritesPerLineComb.sample((*it).second.getNWrittenWords());

    if((*it).second.getUsedWrites() > 0)
      nRetiredEntriesWritten.inc();

    if( ! (*it).second.hasFreeReads() ) {
      nFullReadEntries--;
      I(nFullReadEntries>=0);
    }

    if( ! (*it).second.hasFreeWrites() ) {
      nFullWriteEntries--;
      I(nFullWriteEntries>=0);
    }

    nFreeEntries++;

    ms.erase(it);
  }

  checkOverflow();

  return rmEntry;
}

#endif
