/* License & includes {{{1 */
/* 
  ESESC: Super ESCalar simulator
  Copyright (C) 2010 University of California, Santa Cruz.

  Contributed by Jose Renau
                 Luis Ceze

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
