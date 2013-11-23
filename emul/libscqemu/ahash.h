//  Contributed by Jose Renau
//                 Ehsan K.Ardestani
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
