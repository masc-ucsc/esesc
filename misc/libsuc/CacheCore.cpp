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

#define CACHECORE_CPP
#define alamelu_dbg1 0

#include <stdarg.h>
#include <string.h>
#include <strings.h>

#include "CacheCore.h"
#include "SescConf.h"

#define k_RANDOM "RANDOM"
#define k_LRU "LRU"
#define k_LRUp "LRUp"
#define k_SHIP "SHIP"
#define k_HAWKEYE "HAWKEYE"
#define k_PAR "PAR"
#define k_UAR "UAR"

// UAR: Usage Aware Replacement

// Class CacheGeneric, the combinational logic of Cache
template <class State, class Addr_t>
CacheGeneric<State, Addr_t> *CacheGeneric<State, Addr_t>::create(int32_t size, int32_t assoc, int32_t bsize, int32_t addrUnit,
                                                                 const char *pStr, bool skew, bool xr, uint32_t shct_size) {
  CacheGeneric *cache;

  if(size / bsize < assoc) {
    MSG("ERROR:Invalid cache configuration size %d, line %d, assoc %d (increase size, or decrease line)", size, bsize, assoc);
    SescConf->notCorrect();
    return 0;
  }

  if(skew && shct_size) {
    MSG("ERROR:Invalid cache configuration size %d, line %d, assoc %d has SHIP enabled ", size, bsize, assoc);
    SescConf->notCorrect();
    return 0;
  }

  if(skew) {
    I(assoc == 1); // Skew cache should be direct map
    cache = new CacheDMSkew<State, Addr_t>(size, bsize, addrUnit, pStr);
  } else if(assoc == 1) {
    // Direct Map cache
    cache = new CacheDM<State, Addr_t>(size, bsize, addrUnit, pStr, xr);
  } else if(size == (assoc * bsize)) {
    // FA
    if(strcasecmp(pStr, k_SHIP) == 0) {
      cache = new CacheSHIP<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, shct_size);
    } else if(strcasecmp(pStr, k_HAWKEYE) == 0) {
      cache = new HawkCache<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    } else {
      cache = new CacheAssoc<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    }
  } else {
    if(strcasecmp(pStr, k_SHIP) == 0) {
      cache = new CacheSHIP<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, shct_size);
    } else if(strcasecmp(pStr, k_HAWKEYE) == 0) {
      cache = new HawkCache<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    } else {
      cache = new CacheAssoc<State, Addr_t>(size, assoc, bsize, addrUnit, pStr, xr);
    }
  }

  I(cache);
  return cache;
}

template <class State, class Addr_t> void CacheGeneric<State, Addr_t>::createStats(const char *section, const char *name) {

  for(int i = 0; i < 16; i++) {
    trackstats[i] = new GStatsCntr("%s_tracker%d", name, i);
  }
  trackerZero = new GStatsCntr("%s_trackerZero", name);
  trackerOne  = new GStatsCntr("%s_trackerOne", name);
  trackerTwo  = new GStatsCntr("%s_trackerTwo", name);
  trackerMore = new GStatsCntr("%s_trackerMore", name);

  trackerUp1    = new GStatsCntr("%s_trackerUp1", name);
  trackerUp1n   = new GStatsCntr("%s_trackerUp1n", name);
  trackerDown1  = new GStatsCntr("%s_trackerDown1", name);
  trackerDown2  = new GStatsCntr("%s_trackerDown2", name);
  trackerDown3  = new GStatsCntr("%s_trackerDown3", name);
  trackerDown4  = new GStatsCntr("%s_trackerDown4", name);
  trackerDown1n = new GStatsCntr("%s_trackerDown1n", name);
  trackerDown2n = new GStatsCntr("%s_trackerDown2n", name);
  trackerDown3n = new GStatsCntr("%s_trackerDown3n", name);
  trackerDown4n = new GStatsCntr("%s_trackerDown4n", name);
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

  int32_t s  = SescConf->getInt(section, size);
  int32_t a  = SescConf->getInt(section, assoc);
  int32_t b  = SescConf->getInt(section, bsize);
  bool    xr = false;
  if(SescConf->checkBool(section, "xorIndex")) {
    xr = SescConf->getBool(section, "xorIndex");
  }
  // printf("Created %s cache, with size:%d, assoc %d, bsize %d\n",section,s,a,b);
  bool sk = false;
  if(SescConf->checkBool(section, skew))
    sk = SescConf->getBool(section, skew);
  // For now, tolerate caches that don't have this defined.
  int32_t u;
  if(SescConf->checkInt(section, addrUnit)) {
    if(SescConf->isBetween(section, addrUnit, 0, b) && SescConf->isPower2(section, addrUnit))
      u = SescConf->getInt(section, addrUnit);
    else
      u = 1;
  } else {
    u = 1;
  }

  const char *pStr = SescConf->getCharPtr(section, repl);

  // SHIP
  uint32_t shct_size = 0;
  if(strcasecmp(pStr, k_SHIP) == 0) {
    shct_size = SescConf->getInt(section, "ship_signature_bits");
  }

  if(SescConf->isGT(section, size, 0) && SescConf->isGT(section, bsize, 0) && SescConf->isGT(section, assoc, 0) &&
     SescConf->isPower2(section, size) && SescConf->isPower2(section, bsize) && SescConf->isPower2(section, assoc) &&
     SescConf->isInList(section, repl, k_RANDOM, k_LRU, k_SHIP, k_LRUp, k_HAWKEYE, k_PAR, k_UAR)) {
    cache = create(s, a, b, u, pStr, sk, xr, shct_size);
  } else {
    // this is just to keep the configuration going,
    // sesc will abort before it begins
    cache = new CacheAssoc<State, Addr_t>(2, 1, 1, 1, pStr, xr);
  }

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
  else if(strcasecmp(pStr, k_PAR) == 0)
    policy = PAR;
  else if(strcasecmp(pStr, k_UAR) == 0)
    policy = UAR;
  else if(strcasecmp(pStr, k_HAWKEYE) == 0) {
    MSG("Invalid cache policy. HawkCache should be used [%s]", pStr);
    exit(0);
  } else {
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
    mem[i].rrip = 0;
    content[i]  = &mem[i];
  }

  irand = 0;
}

template <class State, class Addr_t>
typename CacheAssoc<State, Addr_t>::Line *CacheAssoc<State, Addr_t>::findLineNoEffectPrivate(Addr_t addr) {
  Addr_t tag = this->calcTag(addr);

  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // JustDirectory can break this I((*theSet)->isValid());
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

  return *lineHit;
}

template <class State, class Addr_t>
typename CacheAssoc<State, Addr_t>::Line *CacheAssoc<State, Addr_t>::findLinePrivate(Addr_t addr, Addr_t pc) {
  Addr_t tag = this->calcTag(addr);

  Line **theSet = &content[this->calcIndex4Tag(tag)];
  Line **setEnd = theSet + assoc;

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // JustDirectory can break this I((*theSet)->isValid());

    if(policy == PAR || policy == UAR) {
      // MSG("1inc pc:%llx",(*theSet)->getPC());
      uint16_t next_rrip = (*theSet)->rrip;
      if(tag) {
        next_rrip = RRIP_MAX;
      } else if((*theSet)->rrip < RRIP_PREF_MAX) {
        next_rrip = RRIP_PREF_MAX;
      }
      if(policy == UAR) {
        (*theSet)->incnDemand();
        if(next_rrip > 0 && pc2tracker[(*theSet)->getPC()].conf > 8 &&
           (1 + pc2tracker[(*theSet)->getPC()].demand_trend) < (*theSet)->getnDemand()) {
          trackerDown3->inc();
          next_rrip /= 2;
        } else {
          trackerDown3n->inc();
        }
      }

      adjustRRIP(theSet, setEnd, *theSet, next_rrip);
    }

    return *theSet;
  }

  Line **lineHit = 0;

  {
    Line **l = theSet + 1; // +1 because 0 is already checked
    while(l < setEnd) {
      if((*l)->getTag() == tag) {
        lineHit = l;
        break;
      }
      l++;
    }
  }

  if(lineHit == 0) {
    return 0;
  }

  I((*lineHit)->isValid());

  // No matter what is the policy, move lineHit to the *theSet. This
  // increases locality
  Line *tmp = *lineHit;
  {
    Line **l = lineHit;
    while(l > theSet) {
      Line **prev = l - 1;
      *l          = *prev;
      l           = prev;
    }
    *theSet = tmp;
  }

  uint16_t next_rrip = tmp->rrip;
  if(tag) {
    next_rrip = RRIP_MAX;
  } else if((tmp)->rrip < RRIP_PREF_MAX) {
    next_rrip = RRIP_PREF_MAX;
  }
  if(policy == UAR) {
    tmp->incnDemand();
    // MSG("2inc pc:%llx",(tmp)->getPC());
    if(pc2tracker[tmp->getPC()].conf > 8 && (1 + pc2tracker[tmp->getPC()].demand_trend) < tmp->getnDemand() && next_rrip > 0) {
      trackerDown4->inc();
      next_rrip /= 2;
    } else {
      trackerDown4n->inc();
    }
  }
  adjustRRIP(theSet, setEnd, tmp, next_rrip);

#if 0
  if (policy == UAR) {
    printf("read %d ", this->calcIndex4Tag(tag));
    Line **l = setEnd -1;
    int conta = 0;
    while(l >= theSet) {
      printf(" %d:%d", conta,(*l)->rrip);
      l--;
      conta++;
    }
    printf("\n");
  }
#endif

  return tmp;
}

template <class State, class Addr_t>
typename CacheAssoc<State, Addr_t>::Line *CacheAssoc<State, Addr_t>::findLine2Replace(Addr_t addr, Addr_t pc, bool prefetch) {
  Addr_t tag = this->calcTag(addr);
  I(tag);
  Line **theSet = &content[this->calcIndex4Tag(tag)];
  Line **setEnd = theSet + assoc;

#if 0
  // OK for cache, not BTB
  assert((*theSet)->getTag() != tag);
#else
  if((*theSet)->getTag() == tag) {
    GI(tag, (*theSet)->isValid());

    if(policy == PAR || policy == UAR) {
      if(prefetch)
        I(tag == 0);

      uint16_t next_rrip = (*theSet)->rrip;
      if(tag) {
        next_rrip = RRIP_MAX;
      } else if((*theSet)->rrip < RRIP_PREF_MAX) {
        if(prefetch) {
          next_rrip = RRIP_PREF_MAX;
        } else
          next_rrip = RRIP_MAX;
      }
      adjustRRIP(theSet, setEnd, *theSet, next_rrip);
    }

    return *theSet;
  }
#endif

  Line **lineHit  = 0;
  Line **lineFree = 0; // Order of preference, invalid

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
      else if((*l)->rrip < (*lineFree)->rrip) // == too to add a bit of LRU order between same RIPs
        lineFree = l;
      else if(policy == UAR && ((*l)->rrip == (*lineFree)->rrip)) {
        if(pc2tracker[(*l)->getPC()].demand_trend < pc2tracker[(*lineFree)->getPC()].demand_trend)
          lineFree = l;
      }

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
      I(policy == LRU || policy == LRUp || policy == PAR || policy == UAR);
      // Get the oldest line possible
      lineFree = setEnd - 1;
    }

    I(lineFree);

    if(lineFree == theSet && policy != PAR && policy != UAR)
      return *lineFree; // Hit in the first possition

    tmp     = *lineFree;
    tmp_pos = lineFree;
  } else {
    tmp     = *lineHit;
    tmp_pos = lineHit;
  }

  if(tmp->isValid() && policy == UAR) {
    pc2tracker[tmp->getPC()].done(tmp->getnDemand());
    if(tmp->getnDemand() == 0) {
      trackerZero->inc();
    } else if(tmp->getnDemand() == 1) {
      trackerOne->inc();
    } else if(tmp->getnDemand() == 2) {
      trackerTwo->inc();
    } else {
      trackerMore->inc();
    }

    trackstats[pc2tracker[tmp->getPC()].conf]->inc();
  }
  tmp->setPC(pc);

  if(prefetch) {
    if(policy == PAR) {
      adjustRRIP(theSet, setEnd, tmp, 0);
    } else if(policy == UAR) {
      uint16_t default_rrip_prefetch = 0;
      if(pc2tracker[pc].conf > 0 && pc2tracker[pc].demand_trend > 0) {
        default_rrip_prefetch = RRIP_MAX / 2;
        trackerUp1->inc();
      } else {
        trackerUp1n->inc();
      }
      adjustRRIP(theSet, setEnd, tmp, default_rrip_prefetch);
    }
    return tmp;
  }

  if(policy == LRUp) {
  } else if(policy == PAR || policy == UAR) {

    uint16_t default_rrip = RRIP_MAX;
    if(policy == UAR) {
      if(pc2tracker[pc].conf > 8 && pc2tracker[pc].demand_trend == 0) {
        default_rrip = 0;
        trackerDown1->inc();
      } else if(pc2tracker[pc].conf > 8 && pc2tracker[pc].demand_trend == 1) {
        default_rrip /= 2;
        trackerDown2->inc();
      } else {
        trackerDown2n->inc();
      }
    }
    adjustRRIP(theSet, setEnd, tmp, default_rrip);

    Line **l = tmp_pos;
    while(l > theSet) {
      Line **prev = l - 1;
      *l          = *prev;
      ;
      l = prev;
    }
    *theSet = tmp;
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

  // tmp->rrip = RRIP_MAX;

#if 0
  if (policy == UAR) {
    printf("repl %d ", this->calcIndex4Tag(tag));
    Line **l = setEnd -1;
    int conta = 0;
    while(l >= theSet) {
      printf(" %d:%d", conta,(*l)->rrip);
      l--;
      conta++;
    }
    printf("\n");
  }
#endif

  return tmp;
}
/*********************************************************
 *  HawkCache
 *********************************************************/

template <class State, class Addr_t>
HawkCache<State, Addr_t>::HawkCache(int32_t size, int32_t assoc, int32_t blksize, int32_t addrUnit, const char *pStr, bool xr)
    : CacheGeneric<State, Addr_t>(size, assoc, blksize, addrUnit, xr) {
  I(numLines > 0);

  if(strcasecmp(pStr, k_HAWKEYE) != 0) {
    MSG("Invalid cache policy. Cache should be used [%s]", pStr);
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

  // hawkeye
  prediction.resize(512);
  predictionMask = prediction.size() - 1;

  usageInterval.resize(8192);
  usageIntervalMask = usageInterval.size() - 1;

  trackedAddresses.resize(8192);
  trackedAddresses_ptr = 0;

  occupancyVector.resize(8192);
  occVectIterator = 0;
}

template <class State, class Addr_t>
typename HawkCache<State, Addr_t>::Line *HawkCache<State, Addr_t>::findLineNoEffectPrivate(Addr_t addr) {
  Addr_t tag = this->calcTag(addr);

  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // JustDirectory can break this I((*theSet)->isValid());
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

  return *lineHit;
}

template <class State, class Addr_t>
typename HawkCache<State, Addr_t>::Line *HawkCache<State, Addr_t>::findLinePrivate(Addr_t addr, Addr_t pc) {
  Addr_t tag = this->calcTag(addr);

  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    // JustDirectory can break this I((*theSet)->isValid());
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

  // hawkeye
  uint8_t OPToutcome  = 0;
  uint8_t missFlag    = 0;
  int     occVectSize = 8192;
  int     trAddSize   = 8192;
  int     curAddr     = 0;

  int hashAddr = getUsageIntervalHash(addr);

  int predHPC = getPredictionHash(pc);

  // OPTgen -----------------
  occVectIterator++;
  if(occVectIterator >= occVectSize) {
    occVectIterator = 0;
    for(int i = 0; i < occVectSize; i++) {
      occupancyVector[i] = 0;
    }
  }
  for(int i = 0; i < trAddSize; i++) { // Update usage interval for all tracked addresses
    if(trackedAddresses[i] == 0)
      continue;
    if(trackedAddresses[i] == tag) {
      curAddr = i;
    }

    if(usageInterval[getUsageIntervalHash(trackedAddresses[i])] < 255)
      usageInterval[getUsageIntervalHash(trackedAddresses[i])]++;
  }

  if(usageInterval[hashAddr] > 0) { // Address has been loaded before
    int limit = occVectIterator - usageInterval[hashAddr];
    if(limit < 0) {
      limit = 0;
    } else if(limit >= occVectSize) {
      limit = occVectSize - 1;
    }
    for(int i = limit; i < occVectIterator; i++) {
      if(occupancyVector[i] >= numLines) {
        missFlag = 1;
        break; // If cache is full during usage interval, then it will miss
      }
    }

    // If it doesn't miss, increase occupancy vector during liveness interval
    if(missFlag != 1) {
      for(int i = limit; i < occVectIterator; i++) {
        if(occupancyVector[i] < 255)
          occupancyVector[i]++;
      }
      usageInterval[hashAddr]   = 0;
      trackedAddresses[curAddr] = 0;
    }
  } else { // Current address hasn't been loaded yet
    usageInterval[hashAddr] = 1;

    // Start tracking current address;
    bool found = false;
    for(int i = 0; i < trAddSize; i++) {
      if(trackedAddresses[i] == tag || trackedAddresses[i] == 0) {
        found               = true;
        trackedAddresses[i] = tag;
        break;
      }
    }
    if(!found) {
      trackedAddresses[trackedAddresses_ptr++] = tag;
      if(trackedAddresses_ptr >= trackedAddresses.size())
        trackedAddresses_ptr = 0;
    }
  }

  // 0 means it predicts a miss, 1 means it predicts a hit
  if(missFlag == 1) {
    OPToutcome = 0;
  } else {
    OPToutcome = 1;
  }

  // Hawkeye prediction ---------------
  if(OPToutcome == 1) {
    if(prediction[predHPC] < 7) {
      prediction[predHPC]++;
    }
  } else {
    if(prediction[predHPC] > 0) {
      prediction[predHPC]--;
    }
  }

  // A prediction of 0 is cache-averse, 1 is friendly
  uint8_t hawkPrediction = prediction[predHPC] >> 2;

  if(lineHit == 0) {
    if(hawkPrediction == 1) { // if predict friendly and cache miss, age all lines;
      Line **l = theSet + 1;
      while(l < setEnd) {
        if((*l)->isValid() && (*l)->rrip < 6) {
          (*l)->rrip++;
        }
        l++;
      }
    }
    return 0;
  }

  if((*lineHit)->isValid()) {
    // Setting RRIPs
    if(hawkPrediction == 0) {
      (*lineHit)->rrip = 7;
    } else {
      (*lineHit)->rrip = 0;
    }
  }

  I((*lineHit)->isValid());

  return *lineHit;
}

template <class State, class Addr_t>
typename HawkCache<State, Addr_t>::Line *HawkCache<State, Addr_t>::findLine2Replace(Addr_t addr, Addr_t pc, bool prefetch) {
  Addr_t tag = this->calcTag(addr);
  I(tag);
  Line **theSet = &content[this->calcIndex4Tag(tag)];

  // Check most typical case
  if((*theSet)->getTag() == tag) {
    GI(tag, (*theSet)->isValid());
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

  if(lineHit) {
    return *lineHit;
  }

  I(lineFree);

  Line **l   = setEnd - 1;
  Line **tmp = lineFree;
  // iterate through each cacheline and free the first one with rrip of 7
  while(l >= theSet) {
    if((*l)->isValid() && (*l)->rrip == 7) {
      lineFree = l;
      break;
    }
    l--;
  }

  // if no lines have rrip == 7, revert to LRU and detrain hawk prediction
  if(lineFree == tmp && (*lineFree)->rrip != 7) {
    lineFree = setEnd - 1;
    if(prediction[getPredictionHash(pc)] > 0)
      prediction[getPredictionHash(pc)]--;
  }

  I(lineFree);

  return *lineFree;
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
typename CacheDM<State, Addr_t>::Line *CacheDM<State, Addr_t>::findLineNoEffectPrivate(Addr_t addr) {
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
typename CacheDM<State, Addr_t>::Line *CacheDM<State, Addr_t>::findLinePrivate(Addr_t addr, Addr_t pc) {
  return findLineNoEffectPrivate(addr);
}

template <class State, class Addr_t>
typename CacheDM<State, Addr_t>::Line *CacheDM<State, Addr_t>::findLine2Replace(Addr_t addr, Addr_t pc, bool prefetch) {
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
typename CacheDMSkew<State, Addr_t>::Line *CacheDMSkew<State, Addr_t>::findLineNoEffectPrivate(Addr_t addr) {
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
typename CacheDMSkew<State, Addr_t>::Line *CacheDMSkew<State, Addr_t>::findLinePrivate(Addr_t addr, Addr_t pc) {
  return findLineNoEffectPrivate(addr);
}

template <class State, class Addr_t>
typename CacheDMSkew<State, Addr_t>::Line *CacheDMSkew<State, Addr_t>::findLine2Replace(Addr_t addr, Addr_t pc, bool prefetch) {
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

  if(strcasecmp(pStr, k_SHIP) == 0) {
    policy = SHIP;
  } else {
    MSG("Invalid cache policy [%s]", pStr);
    exit(0);
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
typename CacheSHIP<State, Addr_t>::Line *CacheSHIP<State, Addr_t>::findLineNoEffectPrivate(Addr_t addr) {
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

  return *lineHit;
}

template <class State, class Addr_t>
typename CacheSHIP<State, Addr_t>::Line *CacheSHIP<State, Addr_t>::findLinePrivate(Addr_t addr, Addr_t pc) {
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
  if(true) {
    // pc is the PC or the memory address, not the usable signature
    Addr_t signature = (pc ^ ((pc >> 1) + ((pc & 0xFFFFFFFF))));
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
typename CacheSHIP<State, Addr_t>::Line *CacheSHIP<State, Addr_t>::findLine2Replace(Addr_t addr, Addr_t pc, bool prefetch) {

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
  if(true) {
    // pc is the PC or the memory address, not the usable signature
    Addr_t signature = (pc ^ ((pc >> 1) + ((pc & 0xFFFFFFFF))));
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
