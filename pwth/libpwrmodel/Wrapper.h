#ifndef WRAPPER_H
#define WRAPPER_H

/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Elnaz Ebrahimi
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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <map>
#include <stdlib.h>

#include "parameter.h" // eka, to be able to refer to RunTimeParameters
#include "XML_Parse.h"
#include "processor.h"

/* }}} */

class ChipEnergyBundle;


class Wrapper {
private:
  char* xmlConfig;
  ParseXML *p;
  Processor *proc;
  uint32_t totalPowerSamples;
  const char* pwrlogFilename;
  const char* pwrsection;
  FILE *logpwr;
  bool dumppwth;

public: 
  Wrapper();
  ~Wrapper();

  FILE *logprf;
  int cntr_count;
  uint32_t nPowerCall;

  std::map<std::string, int> mcpat_map;
  typedef std::map<std::string, int>::iterator MapIter;
  std::pair<std::map<std::string, int>::iterator, bool> ret;

  std::map<int,int> stats_map;

  void plug(const char* section, 
      std::vector<uint32_t> *statsVector,  
			ChipEnergyBundle *energyBundle,
      uint32_t *coreEIdx, uint32_t *ncores,
      uint32_t *nL2, uint32_t *nL3,
      std::vector<uint32_t> *coreIndex,
      std::vector<uint32_t> *gpuIndex,
			bool display);
  //deallocate structures, empty for now     
  void unplug() { }

  void calcPower(vector<uint32_t> *statsVector, ChipEnergyBundle *energyBundle, std::vector<uint64_t> *clockInterval);
};

#endif

