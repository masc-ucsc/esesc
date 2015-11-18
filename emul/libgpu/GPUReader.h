/*
ESESC: Super ESCalar simulator
Copyright (C) 2009 University California, Santa Cruz.

Contributed by  Alamelu Sankaranarayanan
Jose Renau
Gabriel Southern

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
  //void queueInstruction (uint32_t insn, AddrType pc, AddrType addr, DataType data,char thumb,  FlowID fid, void *env, bool inEmuTiming = false);
  void queueInstruction (uint32_t insn, AddrType pc, AddrType addr, char thumb,  FlowID fid, void *env, bool inEmuTiming = false);
  void syscall (uint32_t num, Time_t time, FlowID fid);

  void start ();

  // Whenever we have a change in statistics (mode), we should drain the queue
  // as much as possible
  void drainFIFO (FlowID fid);
  bool drainedFIFO (FlowID fid);

  uint32_t getKernelId() { return kernelsId[current_CUDA_kernel]; };
  void fillCudaSpecficfields(DInst* dinst, RAWDInst* rinst, FlowID fid);

};

#endif
