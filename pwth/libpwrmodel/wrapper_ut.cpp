// Contributed by Elnaz Ebrahimi
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

// libmcpat
#include "GStats.h"
#include "SescConf.h"
#include "XML_Parse.h"
#include "io.h"
#include "processor.h"
#include "xmlParser.h"

/* }}} */

#define CNTR_COUNT 43
extern bool opt_for_clk;
void        print_usage(const char *argv0);
void        fillStatsVec(std::vector<uint32_t> *statsVector);
void        fillLkgVecDeviceVec(ChipEnergyBundle *energyBundle);

int main(int argc, const char **argv)
/* main {{{1 */
{
  // bool infile_specified     = false;
  // int  plevel               = 2;
  const char *           section      = 0;
  std::vector<uint32_t> *stats_vector = new std::vector<uint32_t>;
  ChipEnergyBundle *     energyBundle = new ChipEnergyBundle;
  uint32_t               coreEIdx;
  uint32_t               ncores;
  uint32_t               nL2;
  uint32_t               nL3;
  std::vector<uint32_t> *coreIndex;
  std::vector<uint32_t> *gpuIndex;

  Wrapper *mcpat = new Wrapper();

  SescConf = new SConfig(argc, argv);
  section  = (char *)SescConf->getCharPtr("", "pwrmodel", 0);

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
  statsVector->at(0)  = 36229;    // icache_read_accesses
  statsVector->at(1)  = 33590;    // icache_read_misses
  statsVector->at(2)  = 263921;   // itlb_total_accesses
  statsVector->at(3)  = 4;        // itlb_total_misses
  statsVector->at(4)  = 1;        // dtlb_total_accesses
  statsVector->at(5)  = 1;        // dtlb_total_misses
  statsVector->at(6)  = 58824;    // dcache_read_accesses
  statsVector->at(7)  = 27276;    // dcache_write_accesses
  statsVector->at(8)  = 1;        // dcache_total_misses
  statsVector->at(9)  = 1632;     // dcache_read_misses
  statsVector->at(10) = 183;      // dcache_write_misses
  statsVector->at(11) = 1;        // BTB_read_accesses
  statsVector->at(12) = 1;        // BTB_write_accesses
  statsVector->at(13) = 263886;   // core_total_instruction
  statsVector->at(14) = 263886;   // core_int_instruction
  statsVector->at(15) = 263886;   // core_fp_instruction
  statsVector->at(16) = 263886;   // core_branch_instructions
  statsVector->at(17) = 263886;   // core_branch_mispredictions
  statsVector->at(18) = 263886;   // core_committed_instructions
  statsVector->at(19) = 263886;   // core_committed_int_instructions
  statsVector->at(20) = 263886;   // core_committed_fp_instructions
  statsVector->at(21) = 263886;   // core_load_instructions
  statsVector->at(22) = 263886;   // core_store_instructions
  statsVector->at(23) = 14771637; // core_total_cycles
  statsVector->at(24) = 263886;   // core_ROB_reads
  statsVector->at(25) = 263886;   // core_ROB_writes
  statsVector->at(26) = 278909;   // core_int_regfile_reads
  statsVector->at(27) = 278909;   // core_int_regfile_writes
  statsVector->at(28) = 260343;   // core_float_regfile_reads
  statsVector->at(29) = 260343;   // core_float_regfile_writes
  statsVector->at(30) = 260343;   // core_function_calls
  statsVector->at(31) = 1;        // bypassbus_accesses`
  statsVector->at(32) = 58824;    // L2_read_accesses
  statsVector->at(33) = 27276;    // L2_write_accesses
  statsVector->at(34) = 1632;     // L2_read_misses
  statsVector->at(35) = 183;      // L2_write_misses
  statsVector->at(36) = 58824;    // L3_read_accesses
  statsVector->at(37) = 27276;    // L3_write_accesses
  statsVector->at(38) = 1632;     // L3_read_misses
  statsVector->at(39) = 183;      // L3_read_misses
  statsVector->at(40) = 1052;     // noc_total_accesses
  statsVector->at(41) = 1052;     // mc_memory_reads
  statsVector->at(42) = 1052;     // mc_memory_writes
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
