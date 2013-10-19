/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2010 University California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan K.Ardestani

This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#ifndef AHASH_H
#define AHASH_H

#include "scstate.h" // for FlowID
#include <pthread.h>

#define AtomicCompareSwap(ptr, tst_ptr, new_ptr) __sync_bool_compare_and_swap(ptr, tst_ptr, new_ptr) 

#define   HASH_SIZE 128 

typedef struct{
  bool valid;
  FlowID fid;
  bool scPassed; 
} atomicHash;

class AHash {

  private:

    atomicHash atomic_hash[HASH_SIZE]; 
    int32_t marked;

    //pthread_mutex_t hash_lock;
    bool hash_lock[HASH_SIZE];

    void init_atomicHash();
    uint32_t hash(uint32_t hval){
      uint32_t tmp = hval ^ (hval >>5 );
      return tmp & (HASH_SIZE-1) ;
    }
    void ahashLock(bool *ptr);
    void ahashUnlock(bool *ptr);
  public:
    void insert(FlowID fid, AddrType addr);
    void insert_nolock(FlowID fid, AddrType addr);
    bool setSC(FlowID fid, AddrType addr);
    bool checkSC(FlowID fid, AddrType addr);
    int32_t isMarked(){ return marked; };
    void remove(FlowID fid, AddrType addr);
    void ahashLock(AddrType addr){ ahashLock(&hash_lock[hash(addr)]); };
    void ahashUnlock(AddrType addr){ ahashUnlock(&hash_lock[hash(addr)]); };


    AHash();
    ~AHash();

};

#endif
