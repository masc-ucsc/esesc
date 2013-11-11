/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau, Edward Kuang

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

#include "MemStruct.h"
#include <string.h>
#include <stdio.h>
/* }}} */


CacheDebugAccess* CacheDebugAccess::getInstance() {
  static CacheDebugAccess* c = new CacheDebugAccess(); 
  return c;
}

void CacheDebugAccess::setCacheAccess(char* cacheLevel) {
  string s = string(cacheLevel);
  if (s.compare("DL10") == 0 || s.compare("niceCache0") == 0 ||
      s.compare("L3")   == 0 || s.compare("L20")        == 0) 
    debugMap[s] = true;
}

//Careful! Returns false for non-existing entry in map.
bool CacheDebugAccess::readCacheAccess(string cacheLevel) {
  return debugMap[cacheLevel];
}

void CacheDebugAccess::setAddrsAccessed(int a) {
  cacheAccesses = a;
}

int CacheDebugAccess::readAddrsAccessed(void) {
  return cacheAccesses;
}

int CacheDebugAccess::cacheAccessSum(void) {
  int sum = 0;
  map<string, bool>::iterator it;
  for (it = debugMap.begin(); it != debugMap.end(); it++) {
    if (it->second)
      sum++;
  }
  return sum;
}

void CacheDebugAccess::mapReset(void) {
  debugMap.clear();
}

