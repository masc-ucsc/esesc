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

