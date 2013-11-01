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
#ifndef memstruct_h
#define memstruct_h

#include <map>
#include <string>

using namespace std;
/* }}} */

/* This file is used to pass back the data values containing the cache levels
 * accessed. Used in membench.cpp to verify cache coherence.
 */


//Implementing singleton design. Only one instance of CacheDebugAccess exists
class CacheDebugAccess {
  public:
    static CacheDebugAccess* getInstance();

    void setCacheAccess(char*);
    bool readCacheAccess(string);

    void setAddrsAccessed(int);
    int readAddrsAccessed(void);
   
    int cacheAccessSum(void);
    void mapReset(void);

  private:
    map<string, bool> debugMap;
    int cacheAccesses;                          //Addr for where mem read starts

    CacheDebugAccess() {};                      //private constructor
    CacheDebugAccess(CacheDebugAccess const&);  //no accidental creation from copy constructor
    void operator=(CacheDebugAccess const&);
    
};
#endif
