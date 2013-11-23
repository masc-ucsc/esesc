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

#ifndef GPUEMULINTERFACE_H
#define GPUEMULINTERFACE_H

#include "nanassert.h"
#include "EmulInterface.h"
#include "GPUInterface.h"
#include "GPUThreadManager.h"
#include "GPUReader.h"

class GPUEmulInterface:public EmulInterface {
private:
  FlowID nFlows;
  FlowID nEmuls;
  GPUReader *reader;
  FlowID numPEs;

protected:
  std::vector<FlowID> fidFreePool; 
public:
    GPUEmulInterface (const char *section);
   ~GPUEmulInterface ();

  DInst *executeHead (FlowID fid);
  void reexecuteTail (FlowID fid);
  void syncHeadTail (FlowID fid);

  FlowID getNumFlows (void) const;
  FlowID getNumEmuls (void) const;

  void start () {
    reader->start ();
  } 
  
  void queueInstruction (uint32_t insn, AddrType pc, AddrType addr, DataType data,char thumb, FlowID fid, void *env, bool inEmuTiming = false) {
    reader->queueInstruction (insn, pc, addr, data,thumb,  fid, env, inEmuTiming);
  }

  uint32_t getKernelId() { return reader->getKernelId(); };

  void syscall (uint32_t num, Time_t time, FlowID fid) {
  }

  void startRabbit (FlowID fid);
  void startWarmup (FlowID fid);
  void startDetail (FlowID fid);
  void startTiming (FlowID fid);

  FlowID getFid(FlowID last_fid);
  void   freeFid(FlowID);
  FlowID mapGlobalID (FlowID gid) const {
    return gpuTM->mapGlobalID (gid);
  }
  FlowID mapLid(FlowID lid);

  void setSampler (EmuSampler * a_sampler, FlowID fid = 0);
  void drainFIFO();
  FlowID getNumPEs() const
  {
    return numPEs;
  }

};

#endif // GPUEMULINTERFACE_H
