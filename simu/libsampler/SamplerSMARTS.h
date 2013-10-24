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

#ifndef EMU_SAMPLER_SMARTS_H
#define EMU_SAMPLER_SMARTS_H

#include <vector>

#include "nanassert.h"
#include "SamplerBase.h"


class SamplerSMARTS : public SamplerBase {
private:
protected:

  uint64_t nInstRabbit;
  uint64_t nInstWarmup;
  uint64_t nInstDetail;
  uint64_t nInstTiming;
  uint64_t nInstForcedDetail;

  uint64_t nextSwitch;

  EmuMode  next2EmuTiming;

  //std::vector<MemObj *> DL1;
  MemObj *DL1;

  FlowID winnerFid;

  bool allDone();
  void markThisDone(FlowID fid);
public:
  SamplerSMARTS(const char *name, const char *section, EmulInterface *emul, FlowID fid);
  virtual ~SamplerSMARTS();

  void queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env);

  void updateCPI(uint32_t fid);
  void syncStats(){
  };
  void nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);
  float getSamplingRatio() {return static_cast<float>(nInstTiming)/static_cast<float>(nInstRabbit + nInstWarmup + nInstDetail + nInstTiming);};
};
#endif

