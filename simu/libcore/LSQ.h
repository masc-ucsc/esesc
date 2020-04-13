/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Luis Ceze

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

#ifndef LDSTQ_H
#define LDSTQ_H

#include <map>
#include <set>
#include <vector>

#include "GStats.h"
#include "estl.h"

#include "DInst.h"

class LSQ {
private:
protected:
  int32_t freeEntries;
  int32_t unresolved;

  LSQ(int32_t size) {
    freeEntries = size;
    unresolved  = 0;
  }

  virtual ~LSQ() {
  }

public:
  virtual bool   insert(DInst *dinst)    = 0;
  virtual DInst *executing(DInst *dinst) = 0;
  virtual void   remove(DInst *dinst)    = 0;

  void incFreeEntries() {
    freeEntries++;
  }
  void decFreeEntries() {
    unresolved++;
    freeEntries--;
  }
  bool hasFreeEntries() const {
    return freeEntries > 0;
  }
  bool hasPendingResolution() const {
    return unresolved > 0;
  }
};

class LSQFull : public LSQ {
private:
  typedef HASH_MULTIMAP<AddrType, DInst *> AddrDInstQMap;

  GStatsCntr    stldForwarding;
  AddrDInstQMap instMap;

  static AddrType calcWord(const DInst *dinst) {
    return (dinst->getAddr()) >> 3;
  }

public:
  LSQFull(const int32_t id, int32_t size);
  ~LSQFull() {
  }

  bool   insert(DInst *dinst);
  DInst *executing(DInst *dinst);
  void   remove(DInst *dinst);
};

class LSQNone : public LSQ {
private:
  DInst *addrTable[128];

  int getEntry(AddrType addr) const {
    return ((addr >> 1) ^ (addr >> 17)) & 127;
  }

public:
  LSQNone(const int32_t id, int32_t size);
  ~LSQNone() {
  }

  bool   insert(DInst *dinst);
  DInst *executing(DInst *dinst);
  void   remove(DInst *dinst);
};

class LSQVPC : public LSQ {
private:
  std::multimap<AddrType, DInst *> instMap;

  GStatsCntr LSQVPC_replays;

  static AddrType calcWord(const DInst *dinst) {
    return (dinst->getAddr()) >> 2;
  }

public:
  LSQVPC(int32_t size);
  ~LSQVPC() {
  }

  bool     insert(DInst *dinst);
  DInst *  executing(DInst *dinst);
  void     remove(DInst *dinst);
  AddrType replayCheck(DInst *dinst);
};
#endif
