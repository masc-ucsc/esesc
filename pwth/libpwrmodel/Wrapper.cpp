/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Elnaz Ebrahimi
                  Ehsan K. Ardestani
                  Gabriel Southern
                  Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

********************************************************************************
Description: Wrapper class structure creates the interface between McPAT area,
power, and timing model and the eSESC performance simulator.
********************************************************************************/

#include "Wrapper.h"
#include "SescConf.h"
#include "io.h"
#include "processor.h"
#include "XML_Parse.h"
#include "xmlParser.h"

/* }}} */

RuntimeParameter mcpat_call; //eka, to keep track of #of calls to mcpat

Wrapper::Wrapper()
  /* constructor {{{1 */
{
  xmlConfig = 0;
  p = 0;
  proc = 0;
  nPowerCall = 0;
}
/* }}} */


void Wrapper::plug(const char *section, 
                   std::vector<uint32_t> *statsVector,
									 ChipEnergyBundle *energyBundle,
                   uint32_t *coreEIdx, uint32_t *ncores,
                   uint32_t *nL2, uint32_t *nL3,
                   std::vector<uint32_t> *coreIndex,
                   std::vector<uint32_t> *gpuIndex,
                   bool display)
  /* plug {{{1 */
{
  string stats_str = "";
  int str_end_pos = 0;
  
  //format of a 'section' of the config file 
  //[SECTION_NAME]
  //stats[#] = mcpat_counter_name +/-gstats1 +/-gstats2
  //...
  cntr_count = SescConf->getRecordSize(section, "stats");
  for (int i = 0; i < cntr_count; i++) {
    const char *stats_string = SescConf->getCharPtr(section, "stats", i);
    stats_str = stats_string + stats_str;
    
    size_t space = stats_str.find_first_of(" ");
    size_t plus  = stats_str.find_first_of("+");
    size_t minus = stats_str.find_first_of("-");
    if (space > 0)
      str_end_pos = (int) space;
    else if (plus > 0)
      str_end_pos = (int) plus;
    else if (minus > 0)
      str_end_pos = (int) minus;
    
    stats_str.erase(str_end_pos, stats_str.length() - str_end_pos);
    mcpat_map.insert(pair<std::string, int>(stats_str, i));
    stats_str.clear();
  }

  //Initialize mcpat strctures
  p = new ParseXML();
  xmlConfig = const_cast<char*>(SescConf->getCharPtr(section, "xmlconfig"));
  p->initialize(statsVector, mcpat_map, coreIndex, gpuIndex);
  //p->parse(xmlConfig);
  p->parseEsescConf(section);

  proc = new Processor(p); 
  *ncores = proc->procdynp.numCore;
  if (proc->procdynp.numGPU)
    (*ncores) ++;
  *nL2    = proc->procdynp.numL2;
  *nL3    = proc->procdynp.numL3;
  proc->dumpStatics(energyBundle);
  if (display)
    proc->displayEnergy(5, 5, true);

}
/* }}} */
void Wrapper::calcPower(vector<uint32_t> *statsVector, ChipEnergyBundle *energyBundle, std::vector<uint64_t> *clockInterval){
   p->updateCntrValues(statsVector, mcpat_map);
   proc->load(clockInterval);
   proc->Processor2(p);
}
/* }}} */

Wrapper::~Wrapper()
  /* destructor {{{1 */
{ }
/* }}} */

