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

#include "BloomFilter.h"
#include <stdarg.h>
#include <string.h>

int32_t BloomFilter::numDumps = 0;

BloomFilter::BloomFilter(int32_t nv, ...) {
  va_list argL;
  va_start(argL, nv);

  nVectors     = nv;
  vSize        = new int[nVectors];
  vBits        = new int[nVectors];
  vMask        = new unsigned[nVectors];
  rShift       = new int[nVectors];
  countVec     = new int *[nVectors];
  nonZeroCount = new int[nVectors];

  for(int32_t i = 0; i < nVectors; i++) {
    vBits[i] = va_arg(argL, int); // # of bits from the address
    vSize[i] = va_arg(argL, int); // # of entries in the corresponding vector

    countVec[i] = new int[vSize[i]];
    for(int32_t j = 0; j < vSize[i]; j++) {
      countVec[i][j] = 0;
    }

    nonZeroCount[i] = 0;
  }

  va_end(argL);

  desc    = new char[128];
  desc[0] = 0;
  for(int32_t i = 0; i < nVectors; i++)
    sprintf(desc + strlen(desc), "[%d, %d]", vBits[i], vSize[i]);

  initMasks();

  nElements = 0;

  BFBuild = true;
}

BloomFilter::~BloomFilter() {
  if(!BFBuild)
    return;

  delete[] vSize;
  delete[] vBits;
  delete[] vMask;
  delete[] rShift;
  delete[] nonZeroCount;

  for(int32_t i = 0; i < nVectors; i++) {
    delete[] countVec[i];
  }

  delete[] countVec;
  delete[] desc;
}

BloomFilter::BloomFilter(const BloomFilter &bf) {

  if(!bf.BFBuild) {
    BFBuild = false;
    return;
  }

  BFBuild = true;

  nVectors = bf.nVectors;

  vSize        = new int[nVectors];
  vBits        = new int[nVectors];
  vMask        = new unsigned[nVectors];
  rShift       = new int[nVectors];
  countVec     = new int *[nVectors];
  nonZeroCount = new int[nVectors];
  desc         = strdup(bf.desc);
  nElements    = bf.nElements;

  for(int32_t i = 0; i < nVectors; i++) {
    vSize[i]        = bf.vSize[i];
    vBits[i]        = bf.vBits[i];
    vMask[i]        = bf.vMask[i];
    rShift[i]       = bf.rShift[i];
    nonZeroCount[i] = bf.nonZeroCount[i];

    countVec[i] = new int[vSize[i]];

    for(int32_t j = 0; j < vSize[i]; j++) {
      countVec[i][j] = bf.countVec[i][j];
    }
  }
}

BloomFilter &BloomFilter::operator=(const BloomFilter &bf) {

  if(this == &bf)
    return *this;

  if(!bf.BFBuild) {
    if(BFBuild)
      clear();
    return *this;
  }

  BFBuild = true;

  I(nVectors == bf.nVectors);
#ifdef DEBUG
  for(int32_t i = 0; i < nVectors; i++) {
    I(vSize[i] == bf.vSize[i]);
  }
#endif

  nVectors = bf.nVectors;
  for(int32_t i = 0; i < nVectors; i++) {
    vSize[i]        = bf.vSize[i];
    vBits[i]        = bf.vBits[i];
    vMask[i]        = bf.vMask[i];
    rShift[i]       = bf.rShift[i];
    nonZeroCount[i] = bf.nonZeroCount[i];
    for(int32_t j = 0; j < vSize[i]; j++) {
      countVec[i][j] = bf.countVec[i][j];
    }
  }

  nElements = bf.nElements;

  return *this;
}

void BloomFilter::init(bool build, int32_t nv, ...) {
  if(BFBuild) {
    return;
  }

  I(!BFBuild);

  va_list argL;
  va_start(argL, nv);

  BFBuild = true;

  nVectors = nv;
  vSize    = new int[nVectors];
  vBits    = new int[nVectors];

  for(int32_t i = 0; i < nVectors; i++) {
    vBits[i] = va_arg(argL, int); // # of bits from the address
    vSize[i] = va_arg(argL, int); // # of entries in the corresponding vector
  }

  va_end(argL);

  vMask        = new unsigned[nVectors];
  rShift       = new int[nVectors];
  countVec     = new int *[nVectors];
  nonZeroCount = new int[nVectors];

  for(int32_t i = 0; i < nVectors; i++) {

    countVec[i] = new int[vSize[i]];
    for(int32_t j = 0; j < vSize[i]; j++) {
      countVec[i][j] = 0;
    }

    nonZeroCount[i] = 0;
  }

  nElements = 0;

  initMasks();

  desc    = new char[128];
  desc[0] = 0;
  for(int32_t i = 0; i < nVectors; i++)
    sprintf(desc + strlen(desc), "[%d, %d]", vBits[i], vSize[i]);
}

void BloomFilter::initMasks() {
  // now preparing masks, bit shifts etc...
  int32_t totShift = 0;
  for(int32_t i = 0; i < nVectors; i++) {
    unsigned mask = 0;
    for(int32_t m = 0; m < vBits[i]; m++)
      mask = mask | 1 << m;

    mask      = mask << totShift;
    vMask[i]  = mask;
    rShift[i] = totShift;

    totShift += vBits[i];
  }
}

int32_t BloomFilter::getIndex(unsigned val, int32_t chunkPos) {
  unsigned uidx;
  int32_t  ret;

  // val = val ^ SWAP_WORD(val);

  I(BFBuild);

  uidx = val & vMask[chunkPos];
  uidx = uidx >> rShift[chunkPos];

  uidx = (uidx % vSize[chunkPos]);

  ret = (int)uidx;

  I(ret > -1 && ret < vSize[chunkPos]);

  return ret;
}

void BloomFilter::insert(unsigned e) {
  if(!BFBuild)
    return;

  for(int32_t i = 0; i < nVectors; i++) {
    int32_t idx = getIndex(e, i);

    if(countVec[i][idx] == 0) { // it won't be zero anymore
      nonZeroCount[i]++;
    }

    countVec[i][idx]++;
  }
  nElements++;
}

void BloomFilter::remove(unsigned e) {

  if(!BFBuild)
    return;

  for(int32_t i = 0; i < nVectors; i++) {
    int32_t idx = getIndex(e, i);

    countVec[i][idx]--;
    I(countVec[i][idx] >= 0);

    if(countVec[i][idx] == 0) {
      nonZeroCount[i]--;
    }
  }

  I(nElements > 0);
  nElements--;
}

void BloomFilter::clear() {

  if(!BFBuild)
    return;

  for(int32_t i = 0; i < nVectors; i++) {
    for(int32_t j = 0; j < vSize[i]; j++) {
      countVec[i][j] = 0;
    }
    nonZeroCount[i] = 0;
  }

  nElements = 0;
}

bool BloomFilter::mayExist(unsigned e) {

  if(!BFBuild)
    return true;

  for(int32_t i = 0; i < nVectors; i++) {
    int32_t idx = getIndex(e, i);
    if(countVec[i][idx] == 0)
      return false;
  }
  return true;
}

bool BloomFilter::mayIntersect(BloomFilter &otherbf) {

  if(!BFBuild || !otherbf.BFBuild)
    return true;

  I(nVectors == otherbf.nVectors);
#ifdef DEBUG
  for(int32_t i = 0; i < nVectors; i++) {
    I(vSize[i] == otherbf.vSize[i]);
  }
#endif

  for(int32_t v = 0; v < nVectors; v++) {
    bool vectorInt = false;
    for(int32_t e = 0; e < vSize[v]; e++) {
      if((countVec[v][e] > 0) && (otherbf.countVec[v][e] > 0)) {
        vectorInt = true;
        break;
      }
    }
    if(!vectorInt) {
      // at least one vector does not intersect, so intersection is empty
      return false;
    }
  }

  return true;
}

void BloomFilter::mergeWith(BloomFilter &otherbf) {

  if(!BFBuild || !otherbf.BFBuild)
    return;

  I(nVectors == otherbf.nVectors);
#ifdef DEBUG
  for(int32_t i = 0; i < nVectors; i++) {
    I(vSize[i] == otherbf.vSize[i]);
  }
#endif

  for(int32_t v = 0; v < nVectors; v++) {
    nonZeroCount[v] = 0;
    for(int32_t e = 0; e < vSize[v]; e++) {
      countVec[v][e] += otherbf.countVec[v][e];
      if(countVec[v][e] != 0)
        nonZeroCount[v]++;
    }
  }
}

void BloomFilter::subtract(BloomFilter &otherbf) {

  if(!BFBuild || !otherbf.BFBuild)
    return;

  I(nVectors == otherbf.nVectors);
#ifdef DEBUG
  for(int32_t i = 0; i < nVectors; i++) {
    I(vSize[i] == otherbf.vSize[i]);
  }
#endif

  for(int32_t v = 0; v < nVectors; v++) {
    nonZeroCount[v] = 0;
    for(int32_t e = 0; e < vSize[v]; e++) {
      countVec[v][e] -= otherbf.countVec[v][e];
      I(countVec[v][e] >= 0);
      if(countVec[v][e] != 0)
        nonZeroCount[v]++;
    }
  }
}

void BloomFilter::dump(const char *msg) {

  printf("%s:", msg);

  if(!BFBuild) {
    printf("BF never built!\n");
    return;
  }

  for(int32_t i = 0; i < nVectors; i++) {
    printf("\t[%d, %d]", nonZeroCount[i], vSize[i]);
  }
  printf("\t%d \t%d\n", getSize(), getSizeRLE(0, 7));
}

int32_t BloomFilter::getSize() {

  if(!BFBuild)
    return 0;

  int32_t size = 0;

  for(int32_t i = 0; i < nVectors; i++)
    size += vSize[i];

  return size;
}

int32_t BloomFilter::getSizeRLE(int32_t base, int32_t runBits) {

  if(!BFBuild)
    return 0;

  int32_t rleSize = 0;
  int32_t runSize = 0;
  int32_t maxRun  = (1 << runBits) - 1;

  for(int32_t i = 0; i < nVectors; i++) {
    for(int32_t j = 0; j < vSize[i]; j++) {
      if(countVec[i][j] != base) {
        if(runSize != 0) {
          int32_t nRuns = (runSize / maxRun) + ((runSize % maxRun) > 0 ? 1 : 0);
          rleSize += (1 + runBits) * nRuns;
        }
        rleSize++;
        runSize = 0;
      } else {
        runSize++;
      }
    }
  }
  return rleSize;
}

bool BloomFilter::isSubsetOf(BloomFilter &otherbf) {

  if(!BFBuild || !otherbf.BFBuild)
    return true;

  I(nVectors == otherbf.nVectors);
#ifdef DEBUG
  for(int32_t i = 0; i < nVectors; i++) {
    I(vSize[i] == otherbf.vSize[i]);
  }
#endif

  for(int32_t v = 0; v < nVectors; v++) {
    for(int32_t e = 0; e < vSize[v]; e++) {
      if(countVec[v][e] > otherbf.countVec[v][e]) {
        return false;
      }
    }
  }

  return true;
}

void BloomFilter::begin_dump_pychart(const char *bname) {

  char str[512];

  sprintf(str, "%s.%d.py", bname, numDumps);

  numDumps++;

  dumpPtr = fopen(str, "w");

  fprintf(dumpPtr, "from pychart import *\n"
                   "import sys           \n\n"
                   "theme.get_options()  \n"
                   "can = canvas.default_canvas()\n"
                   "size = (300, 200)\n"
                   "ar = area.T(size = size, legend=None, y_range = (0, 2048),\n"
                   "            x_axis = axis.X(format=\"%%d\", label=\"Bank\"),\n"
                   "            y_axis = axis.Y(format=\"%%d\", label=\"Index\"))\n");
}

void BloomFilter::end_dump_pychart() {

  fprintf(dumpPtr, "ar.draw()\n");

  fclose(dumpPtr);
  dumpPtr = NULL;

  if(numDumps > 200) {
    exit(-1);
  }
}

void BloomFilter::add_dump_line(unsigned e) {

  fprintf(dumpPtr, "ar.add_plot(line_plot.T(data=[ ");

  for(int32_t i = 0; i < nVectors; i++) {
    int32_t idx = getIndex(e, i);

    if(i == 2)
      idx *= 32;

    if(i != nVectors - 1) {
      fprintf(dumpPtr, "[%d, %d],", i, idx);
    } else {
      fprintf(dumpPtr, "[%d, %d]", i, idx);
    }
  }

  fprintf(dumpPtr, " ]))\n\n");
}

int32_t BloomFilter::countAlias(unsigned e) {

  int32_t a = 0;

  for(int32_t i = 0; i < nVectors; i++) {
    int32_t idx = getIndex(e, i);
    if(countVec[i][idx] != 0)
      a++;
  }

  return a;
}

void BloomFilter::intersectionWith(BloomFilter &otherbf, BloomFilter &inter) {

  for(int32_t v = 0; v < nVectors; v++) {
    for(int32_t e = 0; e < vSize[v]; e++) {
      if(countVec[v][e] != 0 && otherbf.countVec[v][e] != 0) {
        inter.countVec[v][e] = 1;
      }
    }
  }
}
