/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic
                  Smruti Sarangi

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

/*
 * This launches the ESESC simulator environment with an ideal memory
 */

#include <sys/types.h>
#include <signal.h>

#include "nanassert.h"

#include "BootLoader.h"
#ifdef ENABLE_NBSD
#include "MemRequest.h"
void meminterface_start_snoop_req(uint64_t addr, bool inv, uint16_t coreid, void *_mreq) {
  MemRequest *mreq = (MemRequest *)_mreq;

  //MSG("@%lld snoop %s 0x%lx %d %s",globalClock, mreq->getCurrMem()->getName(), addr, mreq->getAction(), inv?"inv":"wb");
  mreq->convert2SetStateAck();
  mreq->getCurrMem()->doSetStateAck(mreq);
}
#endif

int main(int argc, const char **argv) { 

  BootLoader::plug(argc, argv);
  BootLoader::boot();
  BootLoader::report("done");
  BootLoader::unboot();
  BootLoader::unplug();

  return 0;
}
