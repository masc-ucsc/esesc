// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Milos Prvulovic
//                Smruti Sarangi
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

#ifndef LIVECACHECORE_H
#define LIVECACHECORE_H

#include "Snippets.h"
#include "nanassert.h"

//-------------------------------------------------------------
#define RRIP_M 4 // max value = 2^M   | 4 | 8   | 16   |
//-------------------------------------------------------------
#define DISTANT_REF 3  // 2^M - 1           | 3 | 7   | 15  |
#define IMM_REF 1      // nearimm<imm<dist  | 1 | 1-6 | 1-14|
#define NEAR_IMM_REF 0 // 0                 | 0 | 0   | 0   |
#define LONG_REF 1     // 2^M - 2           | 1 | 6   | 14  |
//-------------------------------------------------------------

enum ReplacementPolicy { LRU, LRUp, RANDOM, SHIP }; // SHIP is RRIP with SHIP (ISCA 2010)

template <class State, class Addr_t> class CacheGeneric {
private:
  static const int32_t STR_BUF_SIZE = 1024;

protected:
  const uint32_t size;
  const uint32_t lineSize;
  const uint32_t addrUnit; // Addressable unit: for most caches = 1 byte
  const uint32_t assoc;
  const uint32_t log2Assoc;
  const uint64_t log2AddrLs;
  const uint64_t maskAssoc;
  const uint32_t sets;
  const uint32_t maskSets;
  const uint32_t log2Sets;
  const uint32_t numLines;

  const bool xorIndex;

  bool goodInterface;

public:
  class CacheLine : public State {
  public:
    bool recent; // used by skew cache
    CacheLine(int32_t lineSize)
        : State(lineSize) {
    }
    // Pure virtual class defines interface
    //
    // Tag included in state. Accessed through:
    //
    // Addr_t getTag() const;
    // void setTag(Addr_t a);
    // void clearTag();
    //
    //
    // bool isValid() const;
    // void invalidate();
  };

  // findLine returns a cache line that has tag == addr, NULL otherwise
  virtual CacheLine *findLinePrivate(Addr_t addr, bool updateSHIP, Addr_t SHIP_signature) = 0;

protected:
  CacheGeneric(uint32_t s, uint32_t a, uint32_t b, uint32_t u, bool xr)
      : size(s)
      , lineSize(b)
      , addrUnit(u)
      , assoc(a)
      , log2Assoc(log2i(a))
      , log2AddrLs(log2i(b / u))
      , maskAssoc(a - 1)
      , sets((s / b) / a)
      , maskSets(sets - 1)
      , log2Sets(log2i(sets))
      , numLines(s / b)
      , xorIndex(xr) {
    // TODO : assoc and sets must be a power of 2
  }

  virtual ~CacheGeneric() {
  }

  void createStats(const char *section, const char *name);

public:
  // Do not use this interface, use other create
  static CacheGeneric<State, Addr_t> *create(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr,
                                             bool skew, bool xr,
                                             uint32_t shct_size = 13); // 13 is the optimal size specified in the paper
  static CacheGeneric<State, Addr_t> *create(const char *section, const char *append, const char *format, ...);
  void                                destroy() {
    delete this;
  }

  virtual CacheLine *findLine2Replace(Addr_t addr, bool updateSHIP, Addr_t SHIP_signature) = 0;

  // TO DELETE if flush from Cache.cpp is cleared.  At least it should have a
  // cleaner interface so that Cache.cpp does not touch the internals.
  //
  // Access the line directly without checking TAG
  virtual CacheLine *getPLine(uint32_t l) = 0;

  // ALL USERS OF THIS CLASS PLEASE READ:
  //
  // readLine and writeLine MUST have the same functionality as findLine. The only
  // difference is that readLine and writeLine update power consumption
  // statistics. So, only use these functions when you want to model a physical
  // read or write operation.

  // Use this is for debug checks. Otherwise, a bad interface can be detected

  CacheLine *findLineDebug(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {
    IS(goodInterface = true);
    CacheLine *line = findLine(addr); // SHIP stats will not be updated
    IS(goodInterface = false);
    return line;
  }

  CacheLine *findLineNoEffect(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {
    IS(goodInterface = true);
    CacheLine *line = findLine(addr); // SHIP stats will not be updated
    IS(goodInterface = false);
    return line;
  }

  CacheLine *findLine(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {
    return findLinePrivate(addr, updateSHIP, SHIP_signature);
  }

  CacheLine *readLine(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {

    IS(goodInterface = true);
    CacheLine *line = findLine(addr, updateSHIP, SHIP_signature);
    IS(goodInterface = false);

    return line;
  }

  CacheLine *writeLine(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {

    IS(goodInterface = true);
    CacheLine *line = findLine(addr, updateSHIP, SHIP_signature);
    IS(goodInterface = false);

    return line;
  }

  CacheLine *fillLine(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {
    CacheLine *l = findLine2Replace(addr, updateSHIP, SHIP_signature);
    I(l);
    l->setTag(calcTag(addr));

    return l;
  }

  CacheLine *fillLine(Addr_t addr, Addr_t &rplcAddr, bool updateSHIP = false, Addr_t SHIP_signature = 0) {
    CacheLine *l = findLine2Replace(addr, updateSHIP, SHIP_signature);
    I(l);
    rplcAddr = 0;

    Addr_t newTag = calcTag(addr);
    if(l->isValid()) {
      Addr_t curTag = l->getTag();
      if(curTag != newTag) {
        rplcAddr = calcAddr4Tag(curTag);
      }
    }

    l->setTag(newTag);

    return l;
  }

  uint32_t getLineSize() const {
    return lineSize;
  }
  uint32_t getAssoc() const {
    return assoc;
  }
  uint32_t getLog2AddrLs() const {
    return log2AddrLs;
  }
  uint32_t getLog2Assoc() const {
    return log2Assoc;
  }
  uint32_t getMaskSets() const {
    return maskSets;
  }
  uint32_t getNumLines() const {
    return numLines;
  }
  uint32_t getNumSets() const {
    return sets;
  }

  Addr_t calcTag(Addr_t addr) const {
    return (addr >> log2AddrLs);
  }

  // Addr_t calcSet4Tag(Addr_t tag)     const { return (tag & maskSets);                  }
  // Addr_t calcSet4Addr(Addr_t addr)   const { return calcSet4Tag(calcTag(addr));        }

  // Addr_t calcIndex4Set(Addr_t set) const { return (set << log2Assoc);                }
  // Addr_t calcIndex4Tag(Addr_t tag) const { return calcIndex4Set(calcSet4Tag(tag));   }
  // uint32_t calcIndex4Addr(Addr_t addr) const { return calcIndex4Set(calcSet4Addr(addr)); }
  Addr_t calcIndex4Tag(Addr_t tag) const {
    Addr_t set;
    if(xorIndex) {
      tag = tag ^ (tag >> log2Sets);
      // Addr_t odd = (tag&1) | ((tag>>2) & 1) | ((tag>>4)&1) | ((tag>>6)&1) | ((tag>>8)&1) | ((tag>>10)&1) | ((tag>>12)&1) |
      // ((tag>>14)&1) | ((tag>>16)&1) | ((tag>>18)&1) | ((tag>>20)&1); // over 20 bit index???
      set = tag & maskSets;
    } else {
      set = tag & maskSets;
    }
    Addr_t index = set << log2Assoc;
    return index;
  }

  Addr_t calcAddr4Tag(Addr_t tag) const {
    return (tag << log2AddrLs);
  }
};

template <class State, class Addr_t> class CacheAssoc : public CacheGeneric<State, Addr_t> {
  using CacheGeneric<State, Addr_t>::numLines;
  using CacheGeneric<State, Addr_t>::assoc;
  using CacheGeneric<State, Addr_t>::maskAssoc;
  using CacheGeneric<State, Addr_t>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t>::CacheLine Line;

protected:
  Line *            mem;
  Line **           content;
  uint16_t          irand;
  ReplacementPolicy policy;

  friend class CacheGeneric<State, Addr_t>;
  CacheAssoc(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr, bool xr);

  Line *findLinePrivate(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);

public:
  virtual ~CacheAssoc() {
    delete[] content;
    delete[] mem;
  }

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l < numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);
};

template <class State, class Addr_t> class CacheDM : public CacheGeneric<State, Addr_t> {
  using CacheGeneric<State, Addr_t>::numLines;
  using CacheGeneric<State, Addr_t>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t>::CacheLine Line;

protected:
  Line * mem;
  Line **content;

  friend class CacheGeneric<State, Addr_t>;
  CacheDM(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr, bool xr);

  Line *findLinePrivate(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);

public:
  virtual ~CacheDM() {
    delete[] content;
    delete[] mem;
  };

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l < numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);
};

template <class State, class Addr_t> class CacheDMSkew : public CacheGeneric<State, Addr_t> {
  using CacheGeneric<State, Addr_t>::numLines;
  using CacheGeneric<State, Addr_t>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t>::CacheLine Line;

protected:
  Line * mem;
  Line **content;

  friend class CacheGeneric<State, Addr_t>;
  CacheDMSkew(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr);

  Line *findLinePrivate(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);

public:
  virtual ~CacheDMSkew() {
    delete[] content;
    delete[] mem;
  };

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l < numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);
};

template <class State, class Addr_t> class CacheSHIP : public CacheGeneric<State, Addr_t> {
  using CacheGeneric<State, Addr_t>::numLines;
  using CacheGeneric<State, Addr_t>::assoc;
  using CacheGeneric<State, Addr_t>::maskAssoc;
  using CacheGeneric<State, Addr_t>::goodInterface;

private:
public:
  typedef typename CacheGeneric<State, Addr_t>::CacheLine Line;

protected:
  Line *            mem;
  Line **           content;
  uint16_t          irand;
  ReplacementPolicy policy;

  /***** SHIP ******/
  uint8_t *SHCT; // (2^log2shct) entries
  uint32_t log2shct;
  /*****************/

  friend class CacheGeneric<State, Addr_t>;
  CacheSHIP(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr,
            uint32_t shct_size = 13); // 13 was the optimal size in the paper

  Line *findLinePrivate(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);

public:
  virtual ~CacheSHIP() {
    delete[] content;
    delete[] mem;
    delete[] SHCT;
  }

  // TODO: do an iterator. not this junk!!
  Line *getPLine(uint32_t l) {
    // Lines [l..l+assoc] belong to the same set
    I(l < numLines);
    return content[l];
  }

  Line *findLine2Replace(Addr_t addr, bool updateSHIP = false, Addr_t SHIP_signature = 0);
};

template <class Addr_t> class StateGenericShip {
private:
  Addr_t tag;
  /* SHIP */
  uint8_t rrpv;      // One per cache line
  Addr_t  signature; // One per cache line
  bool    outcome;   // One per cache line
  /* **** */

public:
  virtual ~StateGenericShip() {
    tag = 0;
  }

  Addr_t getTag() const {
    return tag;
  }
  void setTag(Addr_t a) {
    I(a);
    tag = a;
  }
  void clearTag() {
    tag = 0;
    initSHIP();
  }
  void initialize(void *c) {
    clearTag();
  }

  void initSHIP() {
    rrpv      = RRIP_M - 1;
    signature = 0;
    outcome   = false;
  }

  Addr_t getSignature() const {
    return signature;
  }
  void setSignature(Addr_t a) {
    signature = a;
  }
  bool getOutcome() const {
    return outcome;
  }
  void setOutcome(bool a) {
    outcome = a;
  }
  uint8_t getRRPV() const {
    return rrpv;
  }

  void setRRPV(uint8_t a) {
    rrpv = a;
    if(rrpv > (RRIP_M - 1))
      rrpv = RRIP_M - 1;
    if(rrpv < 0)
      rrpv = 0;
  }

  void incRRPV() {
    if(rrpv < (RRIP_M - 1))
      rrpv++;
  }

  virtual bool isValid() const {
    return tag;
  }

  virtual void invalidate() {
    clearTag();
  }

  virtual void dump(const char *str) {
  }
};

template <class Addr_t> class StateGeneric {
private:
  Addr_t tag;

public:
  virtual ~StateGeneric() {
    tag = 0;
  }

  Addr_t getTag() const {
    return tag;
  }
  void setTag(Addr_t a) {
    I(a);
    tag = a;
  }
  void clearTag() {
    tag = 0;
  }
  void initialize(void *c) {
    clearTag();
  }

  virtual bool isValid() const {
    return tag;
  }

  virtual void invalidate() {
    clearTag();
  }

  virtual void dump(const char *str) {
  }

  Addr_t getSignature() const {
    return 0;
  }
  void setSignature(Addr_t a) {
    I(0); // Incorrect state used for SHIP
  }
  bool getOutcome() const {
    return 0;
  }
  void setOutcome(bool a) {
    I(0); // Incorrect state used for SHIP
  }
  uint8_t getRRPV() const {
    return 0;
  }

  void setRRPV(uint8_t a) {
    I(0); // Incorrect state used for SHIP
  }

  void incRRPV() {
    I(0); // Incorrect state used for SHIP
  }
};

#ifndef LIVECACHECORE_CPP
#include "LiveCacheCore.cpp"
#endif

#endif // LIVECACHECORE_H
