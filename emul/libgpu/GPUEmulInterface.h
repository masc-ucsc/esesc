/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

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
  
  void queueInstruction (uint32_t insn, AddrType pc, AddrType addr, char thumb, FlowID fid, void *env, bool inEmuTiming = false) {
    reader->queueInstruction (insn, pc, addr, thumb, fid, env, inEmuTiming);
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
