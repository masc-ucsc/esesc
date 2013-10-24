/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University California, Santa Cruz.

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

#ifndef SAMPLER_GPU_SIM_H
#define SAMPLER_GPU_SIM_H

#include "SamplerBase.h"
uint32_t roundUp(uint32_t numToRound, uint32_t multiple) 
{ 
  if(multiple == 0) 
    return numToRound; 

  uint32_t remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;
  return numToRound + multiple - remainder;
} 

uint32_t roundDown(uint32_t numToRound, uint32_t multiple) 
{ 
  if(multiple == 0) 
    return numToRound; 

  uint32_t remainder = numToRound % multiple;
  return numToRound - remainder;
} 


class SamplerGPUSim : public SamplerBase {
private:
protected:

public:
  SamplerGPUSim(const char *name, const char *section, EmulInterface *emul);
  virtual ~SamplerGPUSim();

  void queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env);

  uint64_t getTime() const;

};
#endif

