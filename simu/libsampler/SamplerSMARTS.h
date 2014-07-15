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

#ifndef EMU_SAMPLER_SMARTS_H
#define EMU_SAMPLER_SMARTS_H

#include <vector>

#include "nanassert.h"
#include "SamplerBase.h"


class SamplerSMARTS : public SamplerBase {
private:
protected:

public:
  SamplerSMARTS(const char *name, const char *section, EmulInterface *emul, FlowID fid);
  virtual ~SamplerSMARTS();

  //changed by Hamid R. Khaleghzadeh///////////////
  int queue(uint32_t insn, uint64_t pc, uint64_t addr, uint32_t fid, char op, uint64_t icount, void *env);

  void updateCPI(uint32_t fid);
  void syncStats(){
  };
  void nextMode(bool rotate, FlowID fid, EmuMode mod = EmuRabbit);
  float getSamplingRatio() {return static_cast<float>(nInstTiming)/static_cast<float>(nInstRabbit + nInstWarmup + nInstDetail + nInstTiming);};
};
#endif

