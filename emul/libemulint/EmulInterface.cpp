// Contributed by Jose Renau
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
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

#include <stdio.h>
#include <string.h>

#include "EmuSampler.h"
#include "EmulInterface.h"
#include "SescConf.h"

/* }}} */

EmulInterface::EmulInterface(const char *sect)
    : section(strdup(sect))
/* Base class EmulInterface constructor  */
{
  sampler              = 0;
  numFlows             = 0;
  cputype              = CPU;
  const char *emultype = SescConf->getCharPtr(section, "type");

  FlowID nemul = SescConf->getRecordSize("", "cpuemul");
  for(FlowID i = 0; i < nemul; i++) {
    const char *section = SescConf->getCharPtr("", "cpuemul", i);
    const char *type    = SescConf->getCharPtr(section, "type");
    if(strcasecmp(type, emultype) == 0) {
      numFlows++;
    }
  }
}
/*  */

EmulInterface::~EmulInterface()
/* EmulInterface destructor  */
{
}
/*  */

void EmulInterface::setSampler(EmuSampler *a_sampler, FlowID fid) {
  // I(sampler==0);
  sampler = a_sampler;
}
