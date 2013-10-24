/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.
   Copyright (C) 2009 University California, Santa Cruz.

   Contributed by Jose Renau
                  Luis Ceze

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

#include "DInst.h"
#include "EmulInterface.h"
/* }}} */

pool<DInst> DInst::dInstPool(1024, "DInst"); //4 * tsfifo size

Time_t DInst::currentID=0;

DInst::DInst()
{
  pend[0].init(this);
  pend[1].init(this);
  pend[2].init(this);
  I(MAX_PENDING_SOURCES==3);
  nDeps = 0;
#ifdef ENABLE_CUDA
  pe_id = 0;
#endif
}

void DInst::dump(const char *str) {
#ifdef ENABLE_CUDA
  printf("%s:PE:%d %p (%d) %lld %c DInst: pc=0x%x, addr=0x%x data=0x%x src1=%d (%d) src2 = %d dest1 =%d dest2 = %d",str, (int)getPE(), this, fid, (long long)ID, keepStats? 't': 'd', (int)pc,(int)addr, (int)data,(int)(inst.getSrc1()), inst.getOpcode(),inst.getSrc2(),inst.getDst1(), inst.getDst2());
#else
  printf("%s:%p (%d) %lld %c DInst: pc=0x%x, addr=0x%x data=0x%x src1=%d (%d) src2 = %d dest1 =%d dest2 = %d",str, this, fid, (long long)ID, keepStats? 't': 'd', (int)pc,(int)addr, (int)data,(int)(inst.getSrc1()), inst.getOpcode(),inst.getSrc2(),inst.getDst1(), inst.getDst2());
#endif

  if (performed) {
    printf(" performed");
  }else if (executed) {
    printf(" executed");
  }else if (issued) {
    printf(" issued");
  }else{
    printf(" non-issued");
  }
  if (replay)
    printf(" REPLAY ");

  if (hasPending())
    printf(" has pending");
  if (!isSrc1Ready())
    printf(" has src1 deps");
  if (!isSrc2Ready())
    printf(" has src2 deps");
  if (!isSrc3Ready())
    printf(" has src3 deps");

  //inst.dump("Inst->");

  printf("\n");
}

void DInst::clearRATEntry() {
  I(RAT1Entry);
  if ( (*RAT1Entry) == this )
    *RAT1Entry = 0;
  if ( (*RAT2Entry) == this )
    *RAT2Entry = 0;
  if (serializeEntry)
    if ( (*serializeEntry) == this )
      *serializeEntry = 0;
}

DInst *DInst::clone() {

  DInst *i = dInstPool.out();

  i->fid           = fid;
  i->inst          = inst;
  i->pc            = pc;
  i->addr          = addr;
  i->data          = data;

#ifdef ENABLE_CUDA
  i->memaccess = memaccess;
  i->pe_id = pe_id;
#endif
  i->keepStats   = keepStats;

  i->setup();

  return i;
}

void DInst::scrap(EmulInterface *eint) {
  I(nDeps == 0);   // No deps src
  I(first == 0);   // no dependent instructions

  I(eint);
  eint->reexecuteTail(fid);

  dInstPool.in(this);
}

void DInst::destroy(EmulInterface *eint) {
  I(nDeps == 0);   // No deps src

  I(!fetch); // if it block the fetch engine. it is unblocked again
  I(issued);
  I(executed);

  I(first == 0);   // no dependent instructions

  scrap(eint);
}

