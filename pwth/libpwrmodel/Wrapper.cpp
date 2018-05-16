// Contributed by Elnaz Ebrahimi
//                Ehsan K. Ardestani
//                Gabriel Southern
//                Jose Renau
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

/*******************************************************************************
Description: Wrapper class structure creates the interface between McPAT area,
power, and timing model and the eSESC performance simulator.
********************************************************************************/

#include "Wrapper.h"
#include "SescConf.h"
#include "XML_Parse.h"
#include "io.h"
#include "processor.h"
#include "xmlParser.h"

/* }}} */

RuntimeParameter mcpat_call; // eka, to keep track of #of calls to mcpat

Wrapper::Wrapper()
/* constructor {{{1 */
{
  p          = 0;
  proc       = 0;
  nPowerCall = 0;
}
/* }}} */

void Wrapper::plug(const char *section, std::vector<uint32_t> *statsVector, ChipEnergyBundle *energyBundle, uint32_t *coreEIdx,
                   uint32_t *ncores, uint32_t *nL2, uint32_t *nL3, std::vector<uint32_t> *coreIndex,
                   std::vector<uint32_t> *gpuIndex, bool display)
/* plug {{{1 */
{
  string stats_str   = "";
  int    str_end_pos = 0;

  // format of a 'section' of the config file
  //[SECTION_NAME]
  // stats[#] = mcpat_counter_name +/-gstats1 +/-gstats2
  //...
  cntr_count = SescConf->getRecordSize(section, "stats");
  for(int i = 0; i < cntr_count; i++) {
    const char *stats_string = SescConf->getCharPtr(section, "stats", i);
    stats_str                = stats_string + stats_str;

    size_t space = stats_str.find_first_of(" ");
    size_t plus  = stats_str.find_first_of("+");
    size_t minus = stats_str.find_first_of("-");
    if(space > 0)
      str_end_pos = (int)space;
    else if(plus > 0)
      str_end_pos = (int)plus;
    else if(minus > 0)
      str_end_pos = (int)minus;

    stats_str.erase(str_end_pos, stats_str.length() - str_end_pos);
    mcpat_map.insert(pair<std::string, int>(stats_str, i));
    stats_str.clear();
  }

  // Initialize mcpat strctures
  p = new ParseXML();
  p->initialize(statsVector, mcpat_map, coreIndex, gpuIndex);
  p->parseEsescConf(section);

  proc    = new Processor(p);
  *ncores = proc->procdynp.numCore;
  if(proc->procdynp.numGPU)
    (*ncores)++;
  *nL2 = proc->procdynp.numL2;
  *nL3 = proc->procdynp.numL3;
  proc->dumpStatics(energyBundle);
  if(display)
    proc->displayEnergy(5, 5, true);
}
/* }}} */
void Wrapper::calcPower(vector<uint32_t> *statsVector, ChipEnergyBundle *energyBundle, std::vector<uint64_t> *clockInterval) {
  p->updateCntrValues(statsVector, mcpat_map);
  proc->load(clockInterval);
  proc->Processor2(p);
}
/* }}} */

Wrapper::~Wrapper()
/* destructor {{{1 */
{
}
/* }}} */
