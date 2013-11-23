// Contributed by Jose Renau
//                Luis Ceze
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

void DInst::recycle() {
  I(nDeps == 0);    // No deps src
  I(first == 0);    // no dependent instructions

  dInstPool.in(this);
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

