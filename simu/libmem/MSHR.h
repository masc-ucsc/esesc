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

#include "MemRequest.h"
#include "callback.h"
#include "nanassert.h"
#include "GStats.h"
#include "pool.h"
#include "DInst.h"

#include "MSHRentry.h"

#include "BloomFilter.h"
/* }}} */

class MSHR {
private:
protected:
  class OverflowField {
  public:
    AddrType      addr;
    CallbackBase *cb;
  };

  const char    *name;
  const uint32_t Log2LineSize;
  const int16_t  nEntries;
  const int16_t  nSubEntries;

  int16_t        nFreeEntries;

  GStatsAvg      avgUse;
  GStatsAvg      avgSubUse;
  GStatsCntr     nOverflows;

  typedef std::deque<OverflowField> Overflow;
  Overflow overflow;

  AddrType calcLineAddr(AddrType addr) const { return addr >> Log2LineSize; }

  static MSHR *create(const char *name, const char *type, int32_t size, int16_t lineSize, int16_t nSubEntries);
public:
  static MSHR *create(const char *name, const char *section, int16_t lineSize);

  MSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);
  virtual ~MSHR() { 
  }
  bool hasFreeEntries() const {
     return (nFreeEntries > 0);
  }

  // All derived classes must implement this interface
  virtual bool canAccept(AddrType paddr) const = 0;
  virtual bool canIssue(AddrType addr) const = 0;
  virtual void addEntry(AddrType addr, CallbackBase *c=0) = 0;
  virtual void retire(AddrType addr) = 0;
  virtual void dump() const = 0;
};

//
// MSHR that just queues the reqs, does NOT enforce address dependencies
//
class BlockingMSHR : public MSHR {
private:  
protected:
  friend class MSHR;
  BlockingMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);
  
public:
  virtual ~BlockingMSHR() { }

  bool canAccept(AddrType paddr) const;
  bool canIssue(AddrType paddr) const; 
  void addEntry(AddrType paddr, CallbackBase *c=0);
  void retire(AddrType paddr);
  void dump() const;
};

class NoMSHR : public MSHR {
private:  
protected:
  friend class MSHR;
  NoMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);
  
public:
  virtual ~NoMSHR() { }

  bool canAccept(AddrType paddr) const;
  bool canIssue(AddrType paddr) const; 
  void addEntry(AddrType paddr, CallbackBase *c=0);
  void retire(AddrType paddr);
  void dump() const;
};

//
// regular full MSHR, address deps enforcement and overflowing capabilities
//

class FullMSHR : public MSHR {
private:
  GStatsCntr nStallConflict;

  const int32_t    MSHRSize;
  const int32_t    MSHRMask;

  // If a non-integer type is defined, the MSHR template should accept
  // a hash function as a parameter

  // Running crafty, I verified that the current hash function
  // performs pretty well (due to the extra space on MSHRSize). It
  // performs only 5% worse than an oversize prime number (noise).
  uint32_t calcEntry(AddrType paddr) const {
    uint64_t p = paddr >> Log2LineSize;
    return (p ^ (p>>11)) & MSHRMask;
  }

  class EntryType {
  public:
    CallbackContainer cc;
    int32_t nUse;
  };

  std::vector<EntryType> entry;

protected:
  friend class MSHR;
  FullMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);

public:
  virtual ~FullMSHR() { 
  }

  bool canAccept(AddrType paddr) const; // block cache requests if necessary
  bool canIssue(AddrType paddr) const;
  void addEntry(AddrType paddr, CallbackBase *c=0);
  void retire(AddrType paddr);
  void dump() const;
};

#if 0
class SingleMSHR : public MSHR {
private:  
  int32_t nOutsReqs;

  bool checkingOverflow;

  int32_t nFullReadEntries;
  int32_t nFullWriteEntries;

  typedef HASH_MAP<AddrType, MSHRentry > MSHRstruct;
  typedef MSHRstruct::iterator       MSHRit;
  typedef MSHRstruct::const_iterator const_MSHRit;

  MSHRstruct ms;
  GStatsAvg  avgOverflowConsumptions;
  GStatsMax  maxOutsReqs;
  GStatsAvg  avgReqsPerLine;
  GStatsCntr nIssuesNewEntry;
  GStatsAvg  avgQueueSize;
  GStatsAvg  avgWritesPerLine;
  GStatsAvg  avgWritesPerLineComb;
  GStatsCntr nOnlyWrites;
  GStatsCntr nRetiredEntries;
  GStatsCntr nRetiredEntriesWritten;

protected:
  friend class MSHR;
  SingleMSHR(const char *name, int32_t size, int16_t lineSize, int16_t nSubEntries);

  void toOverflow(AddrType paddr, CallbackBase *c);

  void checkOverflow();
public:

  virtual ~SingleMSHR() { }

  bool canIssue(AddrType paddr) const; 
  void addEntry(AddrType paddr, CallbackBase *c=0);
  bool retire(AddrType paddr);
};
#endif

#endif // MSHR_H
