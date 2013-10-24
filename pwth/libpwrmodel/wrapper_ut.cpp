/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Elnaz Ebrahimi
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

//libmcpat
#include "io.h"
#include "xmlParser.h"
#include "XML_Parse.h"
#include "processor.h"
#include "SescConf.h"
#include "GStats.h"

/* }}} */

#define CNTR_COUNT 43
extern bool opt_for_clk;
void print_usage(const char * argv0);
void fillStatsVec(std::vector<uint32_t> *statsVector);
void fillLkgVecDeviceVec(ChipEnergyBundle *energyBundle);

int main(int argc, const char **argv)
  /* main {{{1 */
{
  //bool infile_specified     = false;
  //int  plevel               = 2;
  const char* section = 0;
  std::vector<uint32_t> *stats_vector = new std::vector<uint32_t>;
	ChipEnergyBundle *energyBundle = new ChipEnergyBundle;
	uint32_t coreEIdx;
	uint32_t ncores;
	uint32_t nL2;
	uint32_t nL3;
	std::vector<uint32_t> *coreIndex;
	std::vector<uint32_t> *gpuIndex;

  Wrapper *mcpat = new Wrapper();

  SescConf = new SConfig(argc, argv);
  section = (char*) SescConf->getCharPtr("", "pwrmodel", 0);

  LOG("Power Section is %s", section);
  stats_vector->resize(CNTR_COUNT);
  fillStatsVec(stats_vector);
  fillLkgVecDeviceVec(energyBundle);
  mcpat->plug(section, stats_vector, energyBundle, &coreEIdx, &ncores, &nL2, &nL3, coreIndex, gpuIndex, false);

  fill(stats_vector->begin(), stats_vector->end(), 1);

  return 0;
}
/* }}} */

void fillStatsVec(std::vector<uint32_t> *statsVector)
  /* fillStatsVec {{{1 */
{
  statsVector->at(0) = 36229;   //icache_read_accesses
  statsVector->at(1) = 33590;   //icache_read_misses
  statsVector->at(2) = 263921;  //itlb_total_accesses
  statsVector->at(3) = 4;       //itlb_total_misses
  statsVector->at(4) = 1;        //dtlb_total_accesses
  statsVector->at(5) = 1;        //dtlb_total_misses
  statsVector->at(6) = 58824;    //dcache_read_accesses
  statsVector->at(7) = 27276;    //dcache_write_accesses
  statsVector->at(8) = 1;        //dcache_total_misses
  statsVector->at(9) = 1632;     //dcache_read_misses
  statsVector->at(10) = 183;     //dcache_write_misses
  statsVector->at(11) = 1;       //BTB_read_accesses
  statsVector->at(12) = 1;       //BTB_write_accesses
  statsVector->at(13) = 263886;  //core_total_instruction
  statsVector->at(14) = 263886;  //core_int_instruction
  statsVector->at(15) = 263886;  //core_fp_instruction
  statsVector->at(16) = 263886;  //core_branch_instructions
  statsVector->at(17) = 263886;  //core_branch_mispredictions
  statsVector->at(18) = 263886;  //core_committed_instructions
  statsVector->at(19) = 263886;  //core_committed_int_instructions
  statsVector->at(20) = 263886;  //core_committed_fp_instructions
  statsVector->at(21) = 263886;  //core_load_instructions
  statsVector->at(22) = 263886;  //core_store_instructions
  statsVector->at(23) = 14771637;//core_total_cycles
  statsVector->at(24) = 263886;  //core_ROB_reads
  statsVector->at(25) = 263886;  //core_ROB_writes
  statsVector->at(26) = 278909;  //core_int_regfile_reads
  statsVector->at(27) = 278909;  //core_int_regfile_writes
  statsVector->at(28) = 260343;  //core_float_regfile_reads
  statsVector->at(29) = 260343;  //core_float_regfile_writes
  statsVector->at(30) = 260343;  //core_function_calls
  statsVector->at(31) = 1;       //bypassbus_accesses`
  statsVector->at(32) = 58824;   //L2_read_accesses
  statsVector->at(33) = 27276;   //L2_write_accesses
  statsVector->at(34) = 1632;    //L2_read_misses
  statsVector->at(35) = 183;     //L2_write_misses
  statsVector->at(36) = 58824;   //L3_read_accesses 
  statsVector->at(37) = 27276;   //L3_write_accesses
  statsVector->at(38) = 1632;    //L3_read_misses
  statsVector->at(39) = 183;     //L3_read_misses
  statsVector->at(40) = 1052;    //noc_total_accesses
  statsVector->at(41) = 1052;    //mc_memory_reads
  statsVector->at(42) = 1052;    //mc_memory_writes
}
/* }}} */

void fillLkgVecDeviceVec(ChipEnergyBundle *energyBundle)
  /* fillLkgVecDeviceVec {{{1 */
{
  // # of blocks in mcpat 
  uint32_t total_blocks = 28;
  for(uint32_t i = 0; i < total_blocks; i++) {
    energyBundle->cntrs[i].setDevType(1);
    energyBundle->cntrs[i].setLkg(0.0);
  }
}
/* }}} */

