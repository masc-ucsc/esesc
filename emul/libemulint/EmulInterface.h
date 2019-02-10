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

#ifndef EMULINTERFACE_H
#define EMULINTERFACE_H

#include <stdint.h>
#include <sys/time.h>
#include <vector>

#include "DInst.h"
#include "nanassert.h"

class EmuSampler;

typedef enum pType { CPU, GPU } ProcType;

class EmulInterface {
protected:
  const char *section;
  EmuSampler *sampler;

  pthread_mutex_t mutex;
  FlowID          numFlows;

public:
  ProcType cputype;

  EmulInterface(const char *section);
  virtual ~EmulInterface();

  EmuSampler *getSampler() const {
    return sampler;
  }
  virtual void setSampler(EmuSampler *sampler, FlowID fid = 0);

  const char *getSection() const {
    return section;
  }

  // addr meaning
  //  - LD/STs : memory address load/store
  //  - Br/jmp : !=0 if the branch is taken (taken PC)
  //  - Other  : addr == 0

  // Advance the PC of the head
  virtual bool   populate(FlowID fid)    = 0;
  virtual DInst *peekHead(FlowID fid)    = 0;
  virtual void   executeHead(FlowID fid) = 0;
  // Advance the PC of the tail
  virtual void reexecuteTail(FlowID fid) = 0;

#ifdef FETCH_TRACE
  virtual void markTrace(..) = 0;
#endif

  // Synchronize the head and tail. The head went through a wrong
  // path. The head should point to the current Tail. Sort of a
  // recovery from checkpoint where the checkpoint is the Tail.
  virtual void syncHeadTail(FlowID fid) = 0;

  virtual FlowID getNumFlows() const = 0;
  virtual FlowID getNumEmuls() const = 0;
  virtual FlowID getNumPEs() const {
    return 0;
  }; // For GPU
  virtual FlowID mapGlobalID(FlowID gid) const = 0;

  // Called from qemu/gpu thread
  virtual void queueInstruction(AddrType pc, AddrType addr, DataType data, FlowID fid, int op, int src1, int src2, int dest,
                                int dest2, bool keepStats, DataType data2 = 0)    = 0;
  virtual void syscall(uint32_t num, Time_t time, FlowID fid) = 0;

  virtual void start() = 0;

  // Virtual methods used to notify when to start a new simulation phase. They
  // can be used to optimize/speedup the trace generation
  virtual void startRabbit(FlowID fid) = 0;
  virtual void startWarmup(FlowID fid) = 0;
  virtual void startDetail(FlowID fid) = 0;
  virtual void startTiming(FlowID fid) = 0;

  virtual void   setFid(FlowID cpuid)    = 0;
  virtual FlowID getFid(FlowID last_fid) = 0;
  virtual void   freeFid(FlowID)         = 0;
  virtual FlowID mapLid(FlowID)          = 0; // Map local FlowID
  virtual void   drainFIFO()             = 0;
};

#endif // EMULINTERFACE_H
