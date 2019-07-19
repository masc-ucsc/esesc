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

#include <math.h>
#include "DInst.h"
#include "EmulInterface.h"
/* }}} */

pool<DInst> DInst::dInstPool(32768, "DInst"); // 4 * tsfifo size

Time_t DInst::currentID = 0;

DInst::DInst() {
  pend[0].init(this);
  pend[1].init(this);
  pend[2].init(this);
  I(MAX_PENDING_SOURCES == 3);
  nDeps = 0;
}

void DInst::dump(const char *str) {
  fprintf(stderr, "%s:%p (%d) %lld %c DInst: pc=0x%llx, addr=0x%llx src1=%d (%s) src2 = %d dest1 =%d dest2 = %d", str, this, fid,
          (long long)ID, keepStats ? 't' : 'd', (long long)pc, (long long)addr, (int)(inst.getSrc1()), inst.getOpcodeName(),
          inst.getSrc2(), inst.getDst1(), inst.getDst2());

  Time_t t;

  t = getRenamedTime() - getFetchTime();
  if(getRenamedTime())
    fprintf(stderr, " %5d", (int)t);
  else
    fprintf(stderr, "    na");

  t = getIssuedTime() - getRenamedTime();
  if(getIssuedTime())
    fprintf(stderr, " %5d", (int)t);
  else
    fprintf(stderr, "    na");

  t = getExecutedTime() - getIssuedTime();
  if(getExecutedTime())
    fprintf(stderr, " %5d", (int)t);
  else
    fprintf(stderr, "    na");

  t = globalClock - getExecutedTime();
  if(getExecutedTime())
    fprintf(stderr, " %5d", (int)t);
  else
    fprintf(stderr, "    na");

  if(performed) {
    fprintf(stderr, " performed");
  } else if(executing) {
    fprintf(stderr, " executing");
  } else if(executed) {
    fprintf(stderr, " executed");
  } else if(issued) {
    fprintf(stderr, " issued");
  } else {
    fprintf(stderr, " non-issued");
  }
  if(replay)
    fprintf(stderr, " REPLAY ");

  if(hasPending())
    fprintf(stderr, " has pending");
  if(!isSrc1Ready())
    fprintf(stderr, " has src1 deps");
  if(!isSrc2Ready())
    fprintf(stderr, " has src2 deps");
  if(!isSrc3Ready())
    fprintf(stderr, " has src3 deps");

  // inst.dump("Inst->");

  fprintf(stderr, "\n");
}

void DInst::clearRATEntry() {
  I(RAT1Entry);
  if((*RAT1Entry) == this)
    *RAT1Entry = 0;
  if((*RAT2Entry) == this)
    *RAT2Entry = 0;
  if(serializeEntry)
    if((*serializeEntry) == this)
      *serializeEntry = 0;
}

#ifdef ESESC_TRACE_DATA
DataSign DInst::calcDataSign(int64_t _data) {

  DataSign data_sign;
  int64_t  hash, code;

  if(_data == 0)
    data_sign = DS_V0;
  else if(_data == 1)
    data_sign = DS_P1;
  else if(_data == 2)
    data_sign = DS_P2;
  else if(_data == 3)
    data_sign = DS_P3;
  else if(_data == 4)
    data_sign = DS_P4;
  else if(_data == 5)
    data_sign = DS_P5;
  else if(_data == 6)
    data_sign = DS_P6;
  else if(_data == 7)
    data_sign = DS_P7;
  else if(_data == 8)
    data_sign = DS_P8;
  else if(_data == 9)
    data_sign = DS_P9;
  else if(_data == 10)
    data_sign = DS_P10;
  else if(_data == 11)
    data_sign = DS_P11;
  else if(_data == 12)
    data_sign = DS_P12;
  else if(_data == 13)
    data_sign = DS_P13;
  else if(_data == 14)
    data_sign = DS_P14;
  else if(_data == 15)
    data_sign = DS_P15;
  else if(_data == 16)
    data_sign = DS_P16;
  else if(_data == 32)
    data_sign = DS_P32;
  else if(_data == -1)
    data_sign = DS_N1;
  else if(_data == -2)
    data_sign = DS_N2;
  else if(_data % 5 == 0)
    data_sign = DS_FIVE;
  else if(std::ceil(log2(_data)) == std::floor(log2(_data))) //if data is power of 2
    data_sign = DS_POW;
  else if(_data > 1024 * 1024 || _data < -1024 * 1024)
    data_sign = DS_PTR;
  else {
    int v     = static_cast<int>(DS_OPos) + (_data % 255);
    data_sign = static_cast<DataSign>(v);
  }
  return data_sign;
}

void DInst::setDataSign(int64_t _data, AddrType _ldpc) {
  ///data = _data;
  ldpc = _ldpc;

  data_sign = calcDataSign(_data);
  //br_ld_chain_predictable = true; //FIXME - LDBP does prediction only if this flag is set(when load is predictable)
}
void DInst::addDataSign(int ds, int64_t _data, AddrType _ldpc) {
  ldpc = (ldpc << 4) ^ _ldpc;

  if(ds == 0) {
    /*if (_data == data)
      data_sign = DS_EQ;
    else if (_data >= data)
      data_sign = DS_GEC;
    else if (_data < data)
      data_sign = DS_LTC;
    else if (_data != data)
      data_sign = DS_NE;*/  // FIXME: add DS_LT, DS_LE, DS...

    if(_data == data) // beq; rs(data) == rt(_data)
      data_sign = DS_EQ;
    else if(data < _data) // bltc; rs < rt (bgtc alias for bltc)
      data_sign = DS_LTC;
    else if(data > _data) // bgec; rs >= rt (blec is alias for bgec)
      data_sign = DS_GEC;
    else {
      I(_data != data); // bne; rs ! = rt
      data_sign = DS_NE;
    }
  } else if(ds == 1) {
    // DataType mix = data ^ (_data<<3) + (data>>1);
    DataType mix = data ^ (_data << 3);
    data         = mix;
    int v        = static_cast<int>(DS_OPos) + (data % 255);
    data_sign    = static_cast<DataSign>(v);
  } else {
    // Do not mix
  }
}
#endif

DInst *DInst::clone() {

  DInst *i = dInstPool.out();

  i->fid  = fid;
  i->inst = inst;
  i->pc   = pc;
  i->addr = addr;
#ifdef ESESC_TRACE_DATA
  i->ldpc      = ldpc;
  i->data      = data;
  i->data_sign = data_sign;
  i->chained   = 0;
#endif
  i->keepStats = keepStats;

  i->setup();

  return i;
}

void DInst::recycle() {
  I(nDeps == 0); // No deps src
  I(first == 0); // no dependent instructions

  dInstPool.in(this);
}

void DInst::scrap(EmulInterface *eint) {
  I(nDeps == 0); // No deps src
  I(first == 0); // no dependent instructions

  I(eint);
  eint->reexecuteTail(fid);

  dInstPool.in(this);
}

void DInst::destroy(EmulInterface *eint) {
  I(nDeps == 0); // No deps src

  I(issued);
  I(executed);

  I(first == 0); // no dependent instructions

  I(eint);
  eint->reexecuteTail(fid);

  dInstPool.in(this);
}
