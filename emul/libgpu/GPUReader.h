// Contributed by  Alamelu Sankaranarayanan
//                 Jose Renau
//                 Gabriel Southern
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

#ifndef GPU_READER_H
#define GPU_READER_H

#include <queue>
#include <unistd.h>

#include "nanassert.h"

#include "Reader.h"
#include "GStats.h"
#include "EmuDInstQueue.h"
#include "DInst.h"
#include "FastQueue.h"

#include "GPUInterface.h"
#include "ThreadSafeFIFO.h"
#include "ARMCrack.h"
#include "CUDAInstruction.h"

class DInst;

class GPUReader:public Reader {

private:

#ifdef ESESC_QEMU_ISA_ARMEL
  ARMCrack  crackInst;
#endif

  FlowID numFlows;
  bool started;
  EmulInterface *eint;
  volatile bool *active;  // Modified by GPU thread, read by simu thread

public:
  //GPUReader(QEMUArgs *qargs, const char *section);
    GPUReader (const char *section, EmulInterface * eint);
    virtual ~ GPUReader ();

  DInst *executeHead (FlowID fid);
  void reexecuteTail (FlowID fid);
  void syncHeadTail (FlowID fid);

  // Only method called by remote thread
  void queueInstruction (uint32_t insn, AddrType pc, AddrType addr, DataType data,char thumb,  FlowID fid, void *env, bool inEmuTiming = false);
  void syscall (uint32_t num, Time_t time, FlowID fid);

  void start ();

  // Whenever we have a change in statistics (mode), we should drain the queue
  // as much as possible
  void drainFIFO (FlowID fid);
  bool drainedFIFO (FlowID fid);

  uint32_t getKernelId() { return kernelsId[current_CUDA_kernel]; };

};

#endif
