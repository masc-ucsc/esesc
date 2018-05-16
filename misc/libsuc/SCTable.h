/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Luis Ceze
      Milos Prvulovic

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef SCTABLE_H
#define SCTABLE_H

#include "Snippets.h"
#include "nanassert.h"

class SCTable {
private:
  const uint64_t sizeMask;
  const uint8_t  Saturate;
  const uint8_t  MaxValue;

  uint8_t *table;

protected:
public:
  SCTable(const char *str, size_t size, uint8_t bits = 2);
  ~SCTable(void);
  void clear(uint32_t cid); // Bias to not-taken
  void reset(uint32_t cid, bool taken);
  bool predict(uint32_t cid, bool taken); // predict and update
  void update(uint32_t cid, bool taken);
  void inc(uint32_t cid, int d);
  void dec(uint32_t cid, int d);

  bool predict(uint32_t cid) const {
    return table[cid & sizeMask] >= Saturate;
  }
  bool isLowest(uint32_t cid) const {
    return table[cid & sizeMask] == 0;
  }
  bool isHighest(uint32_t cid) const {
    return table[cid & sizeMask] == MaxValue;
  }
  uint8_t getValue(uint32_t cid) const {
    return table[cid & sizeMask];
  }
};

#endif
