/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau


This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef EMU_SAMPLER_GPU_SPACIAL_H
#define EMU_SAMPLER_GPU_SPACIAL_H

#include <vector>

#include "nanassert.h"
#include "SamplerBase.h"


class SamplerGPUSpacial : public SamplerBase {
private:
protected:
  int64_t max_exec_threads;
  int64_t max_exec_threads_conf_file;
  int timeFactor;
  bool fullTiming;
  EmuMode lastMode;

  void allDone();
public:
  SamplerGPUSpacial(const char *name, const char *section, EmulInterface *emul, FlowID fid);
  virtual ~SamplerGPUSpacial();

  void queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env);

  float getSamplingRatio() {return 0;} //FIXME

  void updateIPC(uint32_t fid);
  int64_t getThreads2Simulate(){
    return max_exec_threads;
  };

  int64_t getOrigThreads2Simulate(){
    return max_exec_threads_conf_file;
  };

  void syncStats(){
  };
  bool noActiveCPU();
  uint64_t getGPUTime();
  void doPWTH();
};
#endif

