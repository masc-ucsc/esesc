// copyright and includes {{{1
// Contributed by Sina Hassani
//                Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2015, Regents of the University of California and
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

#include "LiveCache.h"
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//#define MTRACE(a...)   do{ fprintf(stderr,"@%lld %s %d 0x%x:",(long long int)globalClock,getName(), (int)mreq->getID(), (unsigned
// int)mreq->getAddr()); fprintf(stderr,##a); fprintf(stderr,"\n"); }while(0)
#define MTRACE(a...)

LiveCache::LiveCache() {
  cacheBank    = CacheType::create("LiveCache", "", "LL");
  lineSize     = cacheBank->getLineSize();
  lineSizeBits = log2i(lineSize);

  I(getLineSize() < 4096); // To avoid bank selection conflict (insane LiveCache line)

  lineCount = (uint64_t)cacheBank->getNumLines();
  maxOrder  = 0;
}

LiveCache::~LiveCache() {
  cacheBank->destroy();
}

void LiveCache::read(uint64_t addr) {
  uint64_t tmp;
  if(cacheBank->findLine(addr))
    tmp = cacheBank->findLine(addr)->getTag();
  else
    tmp = 0;
  Line *l = cacheBank->fillLine(addr);
  if(l->getTag() != tmp) {
    l->st = false;
  }
  l->order = maxOrder;
  maxOrder++;
}

void LiveCache::write(uint64_t addr) {
  Line *l  = cacheBank->fillLine(addr);
  l->st    = true;
  l->order = maxOrder;
  maxOrder++;
}

uint64_t LiveCache::traverse(uint64_t *addrs, bool *st) {
  // Creating an array of cache lines
  Line *   arr[lineCount];
  uint64_t cnt = 0;
  for(uint64_t i = 0; i < lineCount; i++) {
    if(cacheBank->getPLine(i) && cacheBank->getPLine(i)->order) {
      arr[cnt] = cacheBank->getPLine(i);
      cnt++;
    }
  }
  mergeSort(arr, cnt);

  // creating loads and stores arrays
  uint64_t in = 0;
  for(uint64_t i = 0; i < cnt; i++) {
    if(arr[i]->getTag() > 0) {
      addrs[in] = (uint64_t)cacheBank->calcAddr4Tag(arr[i]->getTag());
      if(arr[i]->st)
        st[in] = true;
      else
        st[in] = false;
      in++;
    }
  }

  return in;
}

void LiveCache::mergeSort(Line **arr, uint64_t len) {
  // in case we had one element
  if(len < 2)
    return;

  // in case we had two elements
  if(len == 2) {
    if(arr[0]->order > arr[1]->order) {
      // swap
      Line *t = arr[0];
      arr[0]  = arr[1];
      arr[1]  = t;
    }
    return;
  }

  // divide and conquer
  uint64_t mid = (uint64_t)(len / 2);
  Line *   arr1[mid];
  Line *   arr2[len - mid];
  for(int i = 0; i < mid; i++)
    arr1[i] = arr[i];
  for(int i = 0; i < len - mid; i++)
    arr2[i] = arr[mid + i];
  mergeSort(arr1, mid);
  mergeSort(arr2, len - mid);

  // merging
  uint64_t m = 0;
  uint64_t n = 0;
  for(uint64_t i = 0; i < len; i++) {
    if(n >= (len - mid) || (m < mid && arr1[m]->order <= arr2[n]->order)) {
      arr[i] = arr1[m];
      m++;
    } else {
      arr[i] = arr2[n];
      n++;
    }
  }
}
