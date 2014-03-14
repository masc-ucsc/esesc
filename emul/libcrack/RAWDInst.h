// Contributed by Jose Renau
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

#ifndef RAWDINST_H
#define RAWDINST_H

#include <vector>

#ifdef SCOORE
#include "scuop.h"
typedef Scuop UOPPredecType;

#else
#include "Instruction.h"

typedef Instruction UOPPredecType;
#endif

enum CUDAMemType{
  RegisterMem = 0,
  GlobalMem,
  ParamMem,
  LocalMem,
  SharedMem,
  ConstantMem,
  TextureMem
};

typedef uint64_t DataType;
typedef uint64_t AddrType;
typedef uint32_t FlowID; 

#include "nanassert.h"

typedef uint32_t RAWInstType;

class RAWDInst {
private:
  RAWInstType insn;
  AddrType    pc;
#ifdef SCOORE
  bool isClear;
#else
  AddrType    addr;
#endif
#ifdef ENABLE_CUDA
  CUDAMemType memaccess;
#endif

  bool keepStats;
  bool inITBlock;

protected:
  uint32_t ninst;
  std::vector<UOPPredecType> predec;
public:
#ifdef ENABLE_CUDA
  void setMemaccesstype(CUDAMemType type){
    memaccess = type;
  }

  CUDAMemType getMemaccesstype(void){
    return memaccess;
  }
#endif


  RAWDInst(const RAWDInst &p) {
    insn = p.insn;
    pc   = p.pc;
#ifdef SCOORE
    isClear = p.isClear;
#else
    addr = p.addr;
#endif
    ninst = p.ninst;
    predec = p.predec;
    keepStats = false;
  }

  RAWDInst() {
    IS(clear());
  }

#ifdef SCOORE
  void set(RAWInstType _insn, AddrType _pc, bool _keepStats = false) {
    clearInst();
    insn = _insn;
    pc   = _pc;
    isClear = false;
    keepStats = _keepStats;
  }
#else
  void set(RAWInstType _insn, AddrType _pc, AddrType _addr, bool _keepStats = false) {
    clearInst();
 #ifdef ENABLE_CUDA
    if ((addr >> 61) == 6)
    {
      memaccess   = SharedMem;
    } else {
      memaccess   = GlobalMem;
    }
#endif
    insn = _insn;
    pc   = _pc;
    addr = _addr;
    keepStats = _keepStats;
  }
#endif


  void clearInst() {
    ninst = 0;
    // predec.reserve(16);
#ifdef ENABLE_CUDA
    memaccess   = GlobalMem;
#endif


  }

  UOPPredecType *getNewInst() {
    ninst++;
    if (ninst >= predec.size())
      predec.resize(2*ninst);
    return &(predec[ninst-1]);
  }

#ifdef SCOORE
  bool isEqual(uint32_t _pc) { 
    if ((isClear == false) && (pc == _pc))
      return true;

    return false; 
  }
  bool isITBlock() { 
    return (((insn & 0x0000BF00) == 0x0000BF00)? true : false);
  }
#endif

  size_t getNumInst() const { return ninst; }
  const UOPPredecType *getInstRef(size_t id) const { return &predec[id]; }

  void clear() {
    insn = 0;
    pc   = 0;
#ifndef SCOORE
    addr = 0;
#else
    isClear = true;
#endif
  }

  AddrType    getPC()   const { return pc;   };
  RAWInstType getInsn() const { return insn; };
#ifdef SCOORE
  AddrType    getAddr() const { I(0); return 0; };
#else
  AddrType    getAddr() const { return addr; };
#endif
  bool getStatsFlag(){
    return keepStats;
  };
  bool isInITBlock() {
    return inITBlock;
  };
  void setInITBlock(bool _flag) {
    inITBlock = _flag;
  };
};

#endif

