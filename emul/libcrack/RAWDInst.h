/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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
  DataType    data;
#endif
#ifdef ENABLE_CUDA
  CUDAMemType memaccess;
#endif

  bool keepStats;
  bool inITBlock;
  float L1clkRatio;
  float L3clkRatio;

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
    data = p.data;
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
  void set(RAWInstType _insn, AddrType _pc, AddrType _addr, DataType _data, float _L1clkRatio = 1.0, float _L3clkRatio = 1.0, bool _keepStats = false) {
    clearInst();
    L3clkRatio = _L3clkRatio;
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
    data = _data;
    keepStats = _keepStats;
    L1clkRatio = _L1clkRatio;
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
    data = 0;
#else
    isClear = true;
#endif
  }

  AddrType    getPC()   const { return pc;   };
  RAWInstType getInsn() const { return insn; };
#ifdef SCOORE
  AddrType    getAddr() const { I(0); return 0; };
  DataType    getData() const { I(0); return 0; };
#else
  AddrType    getAddr() const { return addr; };
  DataType    getData() const { return data; };
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
  float getL1clkRatio() { return L1clkRatio; };
  float getL3clkRatio() { return L3clkRatio; };
};

#endif

