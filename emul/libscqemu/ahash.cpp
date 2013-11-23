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

#include "ahash.h"

AHash::AHash()
{

  init_atomicHash();
  //pthread_mutex_init(&hash_lock, NULL);
  marked = 0;
}

void AHash::init_atomicHash() {
  for (int i = 0; i < HASH_SIZE; i++) {
    atomic_hash[i].valid = false;
    atomic_hash[i].scPassed = false;

    hash_lock[i] = false;
  }
}

void AHash::ahashLock(bool * ptr) {
  while (!AtomicCompareSwap(ptr, false, true)) ;
}

void AHash::ahashUnlock(bool * ptr)
{
  AtomicCompareSwap(ptr, true, false);
}

void AHash::insert(FlowID fid, AddrType addr) {
  //pthread_mutex_lock(&hash_lock);
  ahashLock(addr);
  insert_nolock(fid, addr);
  //pthread_mutex_unlock(&hash_lock);
  ahashUnlock(addr);
}

void AHash::insert_nolock(FlowID fid, AddrType addr) {
  atomic_hash[hash(addr)].valid = true;
  atomic_hash[hash(addr)].fid = fid;
  marked++;
}

bool AHash::setSC(FlowID fid, AddrType addr) {
  //pthread_mutex_lock(&hash_lock);
  ahashLock(addr);
  if (atomic_hash[hash(addr)].valid == true &&
      atomic_hash[hash(addr)].fid == fid) {
    atomic_hash[hash(addr)].scPassed = true;
    //pthread_mutex_unlock(&hash_lock);
    ahashUnlock(addr);
    return true;
  } else {
    //pthread_mutex_unlock(&hash_lock);
    ahashUnlock(addr);
    return false;
  }
}

bool AHash::checkSC(FlowID fid, AddrType addr) {
  //pthread_mutex_lock(&hash_lock);
  ahashLock(addr);
  if (atomic_hash[hash(addr)].valid == true &&
      atomic_hash[hash(addr)].scPassed == true &&
      atomic_hash[hash(addr)].fid == fid) {
    //pthread_mutex_unlock(&hash_lock);
    ahashUnlock(addr);
    return true;
  } else {
    //pthread_mutex_unlock(&hash_lock);
    ahashUnlock(addr);
    return false;
  }
}

void AHash::remove(FlowID fid, AddrType addr) {
  //pthread_mutex_lock(&hash_lock);
  ahashLock(addr);
  atomic_hash[hash(addr)].valid = false;
  atomic_hash[hash(addr)].scPassed = false;
  marked--;
  I(marked >= 0);
  //pthread_mutex_unlock(&hash_lock);
  ahashUnlock(addr);
}

AHash::~AHash() {
}

