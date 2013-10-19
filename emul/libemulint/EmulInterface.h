/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

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
#ifndef EMULINTERFACE_H
#define EMULINTERFACE_H

#include <sys/time.h>
#include <stdint.h>
#include <vector>

#include "nanassert.h"
#include "DInst.h"

class EmuSampler;

typedef enum pType{
  CPU,
  GPU
} ProcType;

class EmulInterface {
 protected:
  const char *section;
  EmuSampler *sampler;


  pthread_mutex_t mutex;
  FlowID numFlows;
 public:
  ProcType cputype;

  EmulInterface(const char *section);
  virtual ~EmulInterface();

  EmuSampler *getSampler() const { return sampler; }
  virtual void setSampler(EmuSampler *sampler, FlowID fid = 0);

  const char *getSection() const { return section; }

  // addr meaning
  //  - LD/STs : memory address load/store
  //  - Br/jmp : !=0 if the branch is taken (taken PC)
  //  - Other  : addr == 0

  // Advance the PC of the head
  virtual DInst *executeHead(FlowID fid) = 0;
  // Advance the PC of the tail
  virtual void reexecuteTail(FlowID fid) = 0;

  // Synchronize the head and tail. The head went through a wrong
  // path. The head should point to the current Tail. Sort of a
  // recovery from checkpoint where the checkpoint is the Tail.
  virtual void syncHeadTail(FlowID fid) = 0;

  virtual FlowID getNumFlows() const = 0;
  virtual FlowID getNumEmuls() const = 0;
  virtual FlowID getNumPEs() const {return 0;}; //For GPU
  virtual FlowID mapGlobalID(FlowID gid) const = 0;

  // Called from qemu/gpu thread
  virtual void queueInstruction(uint32_t insn, AddrType pc, AddrType addr, DataType data, char thumb, FlowID fid, void *env, bool keepStats = false) = 0;
#ifdef ENABLE_CUDA
  virtual uint32_t getKernelId() = 0;
#endif
  virtual void syscall(uint32_t num, Time_t time, FlowID fid) = 0;

  virtual void start()=0;

  // Virtual methods used to notify when to start a new simulation phase. They
  // can be used to optimize/speedup the trace generation
  virtual void startRabbit(FlowID fid)=0;
  virtual void startWarmup(FlowID fid)=0;
  virtual void startDetail(FlowID fid)=0;
  virtual void startTiming(FlowID fid)=0;

  virtual FlowID getFid(FlowID last_fid)=0;
  virtual void   freeFid(FlowID)=0;
  virtual FlowID mapLid(FlowID) = 0; // Map local FlowID
  virtual void drainFIFO()=0;
 
};

#endif   // EMULINTERFACE_H
