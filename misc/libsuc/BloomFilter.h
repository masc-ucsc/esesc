/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2004 University of Illinois.

   Contributed by Luis Ceze
                  Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.21

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include "GStats.h"
#include "nanassert.h"

class BloomFilter {
private:
  int32_t * vSize;
  int32_t * vBits;
  unsigned *vMask;
  int32_t * rShift;
  int32_t **countVec;
  int32_t   nVectors;
  int32_t * nonZeroCount;
  char *    desc;
  int32_t   nElements;

  bool BFBuild;

  void    initMasks();
  int32_t getIndex(unsigned val, int32_t chunkPos);

public:
  ~BloomFilter();

  // the chunk parameters are from the least significant to
  // the most significant portion of the address
  BloomFilter(int32_t nv, ...);
  BloomFilter()
      : BFBuild(false) {
  }

  BloomFilter(const BloomFilter &bf);

  BloomFilter &operator=(const BloomFilter &bf);

  void init(bool build, int32_t nv, ...);

  void insert(unsigned e);
  void remove(unsigned e);

  void clear();

  bool mayExist(unsigned e);
  bool mayIntersect(BloomFilter &otherbf);

  void intersectionWith(BloomFilter &otherbf, BloomFilter &inter);

  void mergeWith(BloomFilter &otherbf);
  void subtract(BloomFilter &otherbf);

  bool isSubsetOf(BloomFilter &otherbf);

  int32_t countAlias(unsigned e);

  void        dump(const char *msg);
  const char *getDesc() {
    return desc;
  }

  int32_t size() { //# of elements encoded
    return nElements;
  }

  int32_t getSize(); // size of the vectors in bits
  int32_t getSizeRLE(int32_t base = 0, int32_t runBits = 7);

  FILE *         dumpPtr;
  static int32_t numDumps;
  void           begin_dump_pychart(const char *bname = "bf");
  void           end_dump_pychart();
  void           add_dump_line(unsigned e);
};

class BitSelection {
private:
  int32_t  nBits;
  int32_t  bits[32];
  unsigned mask[32];

public:
  BitSelection() {
    nBits = 0;
    for(int32_t i = 0; i < 32; i++) {
      bits[i] = 0;
    }
  }

  BitSelection(int32_t *bitPos, int32_t n) {
    nBits = 0;
    for(int32_t i = 0; i < n; i++)
      addBit(bitPos[i]);
  }

  ~BitSelection() {
  }

  int32_t getNBits() {
    return nBits;
  }

  void addBit(int32_t b) {
    bits[nBits] = b;
    mask[nBits] = 1 << b;
    nBits++;
  }

  unsigned permute(unsigned val) {
    unsigned res = 0;
    for(int32_t i = 0; i < nBits; i++) {
      unsigned bit = (val & mask[i]) ? 1 : 0;
      res          = res | (bit << i);
    }
    return res;
  }

  void dump(const char *msg) {
    printf("%s:", msg);
    for(int32_t i = 0; i < nBits; i++) {
      printf(" %d", bits[i]);
    }
    printf("\n");
  }
};

#endif // BLOOMFILTER_H
