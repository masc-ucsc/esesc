/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by David Munday
                  Jose Renau                  

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

#ifndef GXBAR_H
#define GXBAR_H

#include "GStats.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"
/* }}} */

class GXBar: public MemObj {
protected:

  uint32_t addrHash(AddrType addr, uint32_t LineSize, uint32_t Modfactor, uint32_t numLowerBanks) const{
    uint32_t numLineBits = log2i(LineSize);

    addr = addr >> numLineBits;
#if 0
    addr = addr % Modfactor;   
#else
    uint32_t addr1 = addr % 7;   
    uint32_t addr2 = addr % 1023;   
    addr = addr1 ^ addr2;
#endif

    return(addr&(numLowerBanks-1));
  }

  static uint32_t Xbar_unXbar_balance;

 public:
  GXBar(const char *device_descr_section, const char *device_name = NULL);
  ~GXBar() {}

};
#endif
