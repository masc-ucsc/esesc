/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
                  James Tuck
                  Smruti Sarangi
                  Luis Ceze
                  Karin Strauss

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define LIVECACHECORE_CPP
#define alamelu_dbg1 0

#include <stdarg.h>
#include <string.h>
#include <strings.h>

#include "LiveCacheCore.h"

#define k_RANDOM "RANDOM"
#define k_LRU "LRU"
#define k_LRUp "LRUp"
#define k_SHIP "SHIP"

//
// Class CacheGeneric, the combinational logic of Cache
//
template <class State, class Addr_t>
CacheGeneric<State, Addr_t> *CacheGeneric<State, Addr_t>::create(int32_t size, int32_t assoc, int32_t bsize, int32_t addrUnit,
                                                                 const char *pStr, bool skew, bool xr, uint32_t shct_size) {
  CacheGeneric *cache;

  if(skew) {
    I(assoc == 1); // Skew cache should be direct map
    cache = new CacheDMSkew<State, Addr_t>(size, bsize, addrUnit, pStr);
  } else if(assoc == 1) {
    // Direct Map cache
    cache = new CacheDM<State, Addr_t>(size, bsize, addrUnit, pStr, xr);
  } else if(size == (assoc * bsize)) {
    if(strcasecmp(pStr, k_SHIP) != 0) {
      // TODO: Fully assoc can use STL container for speed
      cache = new CacheAssoc<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    } else {
      // SHIP Cache
      cache = new CacheSHIP<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, shct_size);
    }
  } else {
    if(strcasecmp(pStr, k_SHIP) != 0) {
      // Associative Cache
      cache = new CacheAssoc<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    } else {
      // SHIP Cache
      cache = new CacheSHIP<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, shct_size);
    }
  }

  I(cache);
  return cache;
}

template <class State, class Addr_t> void CacheGeneric<State, Addr_t>::createStats(const char *section, const char *name) {
#if 0
  int32_t procId = 0;
  if ( name[0] == 'P' && name[1] == '(' ) {
    // This structure is assigned to an specific processor
    const char *number = &name[2];
    procId = atoi(number);
  }
#endif
}

template <class State, class Addr_t>
CacheGeneric<State, Addr_t> *CacheGeneric<State, Addr_t>::create(const char *section, const char *append, const char *format, ...) {
  CacheGeneric *cache = 0;
  char          size[STR_BUF_SIZE];
  char          bsize[STR_BUF_SIZE];
  char          addrUnit[STR_BUF_SIZE];
  char          assoc[STR_BUF_SIZE];
  char          repl[STR_BUF_SIZE];
  char          skew[STR_BUF_SIZE];

  snprintf(size, STR_BUF_SIZE, "%sSize", append);
  snprintf(bsize, STR_BUF_SIZE, "%sBsize", append);
  snprintf(addrUnit, STR_BUF_SIZE, "%sAddrUnit", append);
  snprintf(assoc, STR_BUF_SIZE, "%sAssoc", append);
  snprintf(repl, STR_BUF_SIZE, "%sReplPolicy", append);
  snprintf(skew, STR_BUF_SIZE, "%sSkew", append);

  int32_t     s    = 16 * 1024 * 1024;
  int32_t     a    = 32;
  int32_t     b    = 64;
  bool        xr   = false;
  bool        sk   = false;
  int32_t     u    = 1;
  const char *pStr = "LRU";

  // SHIP
  uint32_t shct_size = 0;
  if(strcasecmp(pStr, k_SHIP) == 0) {
    shct_size = 0;
  }

  cache = create(s, a, b, u, pStr, sk, xr, shct_size);

  I(cache);

  char cacheName[STR_BUF_SIZE];
  {
    va_list ap;

    va_start(ap, format);
    vsprintf(cacheName, format, ap);
    va_end(ap);
  }
  cache->createStats(section, cacheName);

  return cache;
}

/*********************************************************
 *  CacheAssoc
 *********************************************************/

template <class State, class Addr_t>
CacheAssoc<State, Addr_t>::CacheAssoc(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr, bool xr)
    : CacheGeneric<State, Addr_t>(size, assoc, blksize, addrUnit, xr) {
  I(numLines > 0);

  if(strcasecmp(pStr, k_RANDOM) == 0)
    policy = RANDOM;
  else if(strcasecmp(pStr, k_LRU) == 0)
    policy = LRU;
  else if(strcasecmp(pStr, k_LRUp) == 0)
    policy = LRUp;
  else {
    MSG("Invalid cache policy [%s]", pStr);
    exit(0);
  }

  mem = (Line *)malloc(sizeof(Line) * (numLines + 1));
  ////read
  for(uint32_t i = 0; i < numLines; i++) {
    new(&mem[i]) Line(blksize);
  }
  content = new Line *[numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }

  irand = 0;
}

template <class State, class Addr_t>
typename CacheAssoc<State, Addr_t>::Line *CacheAssoc<State, Addr_t>::findLinePrivate(Addr_t addr, bool updateSHIP,
                                                                                     Addr_t SHIP_signature) {
  Addr_t tag = this->calcTag(addr);

  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // I((*theSet)->isValid());   // TODO: why is this assertion failing
    return *theSet;
  }

  Line **lineHit = 0;
  Line **setEnd  = theSet + assoc;

  // For sure that position 0 is not (short-cut)
  {
    Line **l = theSet + 1;
    while(l < setEnd) {
      if((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      l++;
    }
  }

  if(lineHit == 0)
    return 0;

  // I((*lineHit)->isValid()); //TODO: see why this assertion is failing

  // No matter what is the policy, move lineHit to the *theSet. This
  // increases locality
  Line *tmp = *lineHit;
  {
    Line **l = lineHit;
    while(l > theSet) {
      Line **prev = l - 1;
      *l          = *prev;
      ;
      l = prev;
    }
    *theSet = tmp;
  }

  return tmp;
}

template <class State, class Addr_t>
typename CacheAssoc<State, Addr_t>::Line *CacheAssoc<State, Addr_t>::findLine2Replace(Addr_t addr, bool updateSHIP,
                                                                                      Addr_t SHIP_signature) {
  Addr_t tag = this->calcTag(addr);
  I(tag);
  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // GI(tag,(*theSet)->isValid()); //TODO: why is this assertion failing
    return *theSet;
  }

  Line **lineHit  = 0;
  Line **lineFree = 0; // Order of preference, invalid
  Line **setEnd   = theSet + assoc;

  {
    Line **l = setEnd - 1;
    while(l >= theSet) {
      if((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      if(!(*l)->isValid())
        lineFree = l;
      else if(lineFree == 0)
        lineFree = l;

      l--;
    }
  }

  Line * tmp;
  Line **tmp_pos;
  if(!lineHit) {

    I(lineFree);

    if(policy == RANDOM) {
      lineFree = &theSet[irand];
      irand    = (irand + 1) & maskAssoc;
      if(irand == 0)
        irand = (irand + 1) & maskAssoc; // Not MRU
    } else {
      I(policy == LRU || policy == LRUp);
      // Get the oldest line possible
      lineFree = setEnd - 1;
    }

    I(lineFree);

    if(lineFree == theSet)
      return *lineFree; // Hit in the first possition

    tmp     = *lineFree;
    tmp_pos = lineFree;
  } else {
    tmp     = *lineHit;
    tmp_pos = lineHit;
  }

  if(policy == LRUp) {
#if 0
    Line **l = tmp_pos;
    while(l > &theSet[13]) {
      Line **prev = l - 1;
      *l = *prev;;
      l = prev;
    }
    theSet[13] = tmp;
#endif
  } else {
    Line **l = tmp_pos;
    while(l > theSet) {
      Line **prev = l - 1;
      *l          = *prev;
      ;
      l = prev;
    }
    *theSet = tmp;
  }

  return tmp;
}

/*********************************************************
 *  CacheDM
 *********************************************************/

template <class State, class Addr_t>
CacheDM<State, Addr_t>::CacheDM(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr, bool xr)
    : CacheGeneric<State, Addr_t>(size, 1, blksize, addrUnit, xr) {
  I(numLines > 0);

  mem = (Line *)malloc(sizeof(Line) * (numLines + 1));
  for(uint32_t i = 0; i < numLines; i++) {
    new(&mem[i]) Line(blksize);
  }
  content = new Line *[numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }
}

template <class State, class Addr_t>
typename CacheDM<State, Addr_t>::Line *CacheDM<State, Addr_t>::findLinePrivate(Addr_t addr, bool updateSHIP,
                                                                               Addr_t SHIP_signature) {
  Addr_t tag = this->calcTag(addr);
  I(tag);

  Line *line = content[this->calcIndex4Tag(tag)];

  if(line->getTag() == tag) {
    I(line->isValid());
    return line;
  }

  return 0;
}

template <class State, class Addr_t>
typename CacheDM<State, Addr_t>::Line *CacheDM<State, Addr_t>::findLine2Replace(Addr_t addr, bool updateSHIP,
                                                                                Addr_t SHIP_signature) {
  Addr_t tag  = this->calcTag(addr);
  Line * line = content[this->calcIndex4Tag(tag)];

  return line;
}

/*********************************************************
 *  CacheDMSkew
 *********************************************************/

template <class State, class Addr_t>
CacheDMSkew<State, Addr_t>::CacheDMSkew(int32_t size, int32_t blksize, int32_t addrUnit, const char *pStr)
    : CacheGeneric<State, Addr_t>(size, 1, blksize, addrUnit, false) {
  I(numLines > 0);

  mem = (Line *)malloc(sizeof(Line) * (numLines + 1));
  for(uint32_t i = 0; i < numLines; i++) {
    new(&mem[i]) Line(blksize);
  }
  content = new Line *[numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }
}

template <class State, class Addr_t>
typename CacheDMSkew<State, Addr_t>::Line *CacheDMSkew<State, Addr_t>::findLinePrivate(Addr_t addr, bool updateSHIP,
                                                                                       Addr_t SHIP_signature) {
  Addr_t tag1 = this->calcTag(addr);
  I(tag1);

  Line *line = content[this->calcIndex4Tag(tag1)];

  if(line->getTag() == tag1) {
    I(line->isValid());
    line->recent = true;
    return line;
  }
  Line *line0 = line;

  // BEGIN Skew cache
  // Addr_t tag2 = (tag1 ^ (tag1>>1));
  Addr_t addrh = (addr >> 5) ^ (addr >> 11);
  Addr_t tag2  = this->calcTag(addrh);
  I(tag2);
  line = content[this->calcIndex4Tag(tag2)];

  if(line->getTag() == tag1) { // FIRST TAG, tag2 is JUST used for indexing the table
    I(line->isValid());
    line->recent = true;
    return line;
  }
  Line *line1 = line;

#if 1
  // Addr_t tag3 = (tag1 ^ ((tag1>>1) + ((tag1 & 0xFFFF))));
  addrh       = addrh + (addr & 0xFF);
  Addr_t tag3 = this->calcTag(addrh);
  I(tag3);
  line = content[this->calcIndex4Tag(tag3)];

  if(line->getTag() == tag1) { // FIRST TAG, tag2 is JUST used for indexing the table
    I(line->isValid());
    line->recent = true;
    return line;
  }
  Line *line3 = line;

  line3->recent = false;
#endif
  line1->recent = false;
  line0->recent = false;

  return 0;
}

template <class State, class Addr_t>
typename CacheDMSkew<State, Addr_t>::Line *CacheDMSkew<State, Addr_t>::findLine2Replace(Addr_t addr, bool updateSHIP,
                                                                                        Addr_t SHIP_signature) {
  Addr_t tag1  = this->calcTag(addr);
  Line * line1 = content[this->calcIndex4Tag(tag1)];

  if(line1->getTag() == tag1) {
    GI(tag1, line1->isValid());
    return line1;
  }

  // BEGIN Skew cache
  // Addr_t tag2 = (tag1 ^ (tag1>>1));
  Addr_t addrh = (addr >> 5) ^ (addr >> 11);
  Addr_t tag2  = this->calcTag(addrh);
  Line * line2 = content[this->calcIndex4Tag(tag2)];

  if(line2->getTag() == tag1) { // FIRST TAG, tag2 is JUST used for indexing the table
    I(line2->isValid());
    return line2;
  }

  static int32_t rand_number = 0;
  rand_number++;

#if 1
  // Addr_t tag3 = (tag1 ^ ((tag1>>1) + ((tag1 & 0xFFFF))));
  addrh        = addrh + (addr & 0xFF);
  Addr_t tag3  = this->calcTag(addrh);
  Line * line3 = content[this->calcIndex4Tag(tag3)];

  if(line3->getTag() == tag1) { // FIRST TAG, tag2 is JUST used for indexing the table
    I(line3->isValid());
    return line3;
  }

  while(1) {
    if(rand_number > 2)
      rand_number = 0;

    if(rand_number == 0) {
      if(line1->recent)
        line1->recent = false;
      else {
        line1->recent = true;
        line2->recent = false;
        line3->recent = false;
        return line1;
      }
    }
    if(rand_number == 1) {
      if(line2->recent)
        line2->recent = false;
      else {
        line1->recent = false;
        line2->recent = true;
        line3->recent = false;
        return line2;
      }
    } else {
      if(line3->recent)
        line3->recent = false;
      else {
        line1->recent = false;
        line2->recent = false;
        line3->recent = true;
        return line3;
      }
    }
  }
#else
  if((rand_number & 1) == 0)
    return line1;
  return line2;
#endif
  // END Skew cache
}

/*********************************************************
 *  CacheSHIP
 *********************************************************/

template <class State, class Addr_t>
CacheSHIP<State, Addr_t>::CacheSHIP(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr,
                                    uint32_t shct_size)
    : CacheGeneric<State, Addr_t>(size, assoc, blksize, addrUnit, false) {
  I(numLines > 0);
  log2shct = shct_size;
  I(shct_size < 31);
  SHCT = new uint8_t[(2 << log2shct)];

  if(strcasecmp(pStr, k_SHIP) != 0) {
    MSG("Invalid cache policy [%s]", pStr);
    exit(0);
  } else {
    policy = SHIP;
  }

  mem = (Line *)malloc(sizeof(Line) * (numLines + 1));
  for(uint32_t i = 0; i < numLines; i++) {
    new(&mem[i]) Line(blksize);
  }
  content = new Line *[numLines + 1];

  for(uint32_t i = 0; i < numLines; i++) {
    mem[i].initialize(this);
    mem[i].invalidate();
    content[i] = &mem[i];
  }

  for(uint32_t j = 0; j < (2 ^ log2shct); j++) {
    SHCT[j] = 0;
  }

  irand = 0;
}

template <class State, class Addr_t>
typename CacheSHIP<State, Addr_t>::Line *CacheSHIP<State, Addr_t>::findLinePrivate(Addr_t addr, bool updateSHIP,
                                                                                   Addr_t SHIP_signature) {
  Addr_t tag     = this->calcTag(addr);
  Line **theSet  = &content[this->calcIndex4Tag(tag)];
  Line **setEnd  = theSet + assoc;
  Line **lineHit = 0;

  {
    Line **l = theSet;
    while(l < setEnd) {
      if((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      l++;
    }
  }

  if(lineHit == 0) {
    // Cache MISS
    return 0;
  }

  I((*lineHit)->isValid());
  // Cache HIT
  if(updateSHIP) {
    // SHIP_signature is the PC or the memory address, not the usable signature
    Addr_t signature = (SHIP_signature ^ ((SHIP_signature >> 1) + ((SHIP_signature & 0xFFFFFFFF))));
    signature &= ((2 << log2shct) - 1);

    (*lineHit)->setOutcome(true);

    if(SHCT[signature] < 7) // 3 bit counter. But why? Whatabout 2^log2Assoc - 1
      SHCT[signature] += 1;
  }

  (*lineHit)->setRRPV(0);
  Line *tmp = *lineHit;

  /*
     // No matter what is the policy, move lineHit to the *theSet. This
     // increases locality
     {
      Line **l = lineHit;
      while(l > theSet) {
        Line **prev = l - 1;
        *l = *prev;;
        l = prev;
      }
      *theSet = tmp;
     }
  */
  return tmp;
}

template <class State, class Addr_t>
typename CacheSHIP<State, Addr_t>::Line *CacheSHIP<State, Addr_t>::findLine2Replace(Addr_t addr, bool updateSHIP,
                                                                                    Addr_t SHIP_signature) {
  if(updateSHIP == false) {
    // MSG("Calling a CacheSHIP function with update disabled");
  }

  Addr_t tag = this->calcTag(addr);
  I(tag);

  Line **theSet = &content[this->calcIndex4Tag(tag)];
  Line **setEnd = theSet + assoc;

  Line **lineFree = 0; // Order of preference, invalid, rrpv = 3
  Line **lineHit  = 0; // Exact tag match

  {
    Line **l = theSet;
    do {
      l = theSet;

      while(l < setEnd) {
        // Tag matches
        if((*l)->getTag() == tag) {
          lineHit = l;
          if(alamelu_dbg1)
            MSG("Replacing Existing line");
          break;
        }

        // Line is invalid
        if(!(*l)->isValid()) {
          lineFree = l;
          if(alamelu_dbg1)
            MSG("Replacing invalid line");
          break;
        }

        // Line is distant re-referenced
        if((*l)->getRRPV() >= RRIP_M - 1) {
          lineFree = l;
          if(alamelu_dbg1)
            MSG("Replacing new line");
          break;
        }

        l++;
      }

      if((lineHit == 0) && (lineFree == 0)) {
        l = theSet;
        while(l < setEnd) {
          (*l)->incRRPV();
          l++;
        }
      }
    } while((lineHit == 0) && (lineFree == 0));
  }

  Line *tmp;
  if(!lineHit) {
    tmp = *lineFree;
  } else {
    tmp = *lineHit;
  }

  // updateSHIP = false;
  if(updateSHIP) {
    // SHIP_signature is the PC or the memory address, not the usable signature
    Addr_t signature = (SHIP_signature ^ ((SHIP_signature >> 1) + ((SHIP_signature & 0xFFFFFFFF))));
    signature &= ((2 << log2shct) - 1);

    if((tmp->isValid())) {
      Addr_t signature_m = (tmp)->getSignature();
      if((tmp)->getOutcome() == false) {
        if(SHCT[signature_m] > 0) {
          SHCT[signature_m] -= 1;
        }
        // MSG("Old Signature %llu (%d): New Signature %llu (%d)", signature_m, SHCT[signature_m], signature, SHCT[signature]);
      }
    }

    (tmp)->setOutcome(false);
    (tmp)->setSignature(signature);
    if(SHCT[signature] == 0) {
      (tmp)->setRRPV(RRIP_M - 1);
    } else {
      (tmp)->setRRPV(RRIP_M / 2);
    }
  } else {
    (tmp)->setRRPV(RRIP_M / 2);
  }

  /*
    {
      Line **l = tmp_pos;
      while(l > theSet) {
        Line **prev = l - 1;
        *l = *prev;;
        l = prev;
      }
      *theSet = tmp;
    }
  */
  return tmp;
}
