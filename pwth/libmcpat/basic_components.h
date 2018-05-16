/*****************************************************************************
 *                                McPAT
 *                      SOFTWARE LICENSE AGREEMENT
 *            Copyright 2009 Hewlett-Packard Development Company, L.P.
 *                          All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Any User of the software ("User"), by accessing and using it, agrees to the
 * terms and conditions set forth herein, and hereby grants back to Hewlett-
 * Packard Development Company, L.P. and its affiliated companies ("HP") a
 * non-exclusive, unrestricted, royalty-free right and license to copy,
 * modify, distribute copies, create derivate works and publicly display and
 * use, any changes, modifications, enhancements or extensions made to the
 * software by User, including but not limited to those affording
 * compatibility with other hardware or software, but excluding pre-existing
 * software applications that may incorporate the software.  User further
 * agrees to use its best efforts to inform HP of any such changes,
 * modifications, enhancements or extensions.
 *
 * Correspondence should be provided to HP at:
 *
 * Director of Intellectual Property Licensing
 * Office of Strategy and Technology
 * Hewlett-Packard Company
 * 1501 Page Mill Road
 * Palo Alto, California  94304
 *
 * The software may be further distributed by User (but not offered for
 * sale or transferred for compensation) to third parties, under the
 * condition that such third parties agree to abide by the terms and
 * conditions of this license.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITH ANY AND ALL ERRORS AND DEFECTS
 * AND USER ACKNOWLEDGES THAT THE SOFTWARE MAY CONTAIN ERRORS AND DEFECTS.
 * HP DISCLAIMS ALL WARRANTIES WITH REGARD TO THE SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL
 * HP BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 *
 ***************************************************************************/

#ifndef BASIC_COMPONENTS_H_
#define BASIC_COMPONENTS_H_

#include "XML_Parse.h"
#include "array.h"
#include "parameter.h"
#include <vector>

typedef struct {
  // params
  ArrayST * caches;
  ArrayST * missb;
  ArrayST * ifb;
  ArrayST * prefetchb;
  ArrayST * wbb;
  uca_org_t result_caches, result_missb, result_ifb, result_prefetchb, result_wbb;
  // stats
  int      total_accesses;
  int      read_accesses;
  int      write_accesses;
  int      total_hits;
  int      total_misses;
  int      read_hits;
  int      write_hits;
  int      read_misses;
  int      write_misses;
  int      replacements;
  int      write_backs;
  int      miss_buffer_access;
  int      fill_buffer_accesses;
  int      prefetch_buffer_accesses;
  int      prefetch_buffer_writes;
  int      prefetch_buffer_reads;
  int      prefetch_buffer_hits;
  int      wbb_writes;
  int      wbb_reads;
  powerDef tot_power, max_power;
  double   area, maxPower, runtimeDynamicPower;
} cache_processor;

enum FU_type { FPU, ALU };

enum Core_type { OOO, Inorder };

enum Renaming_type { RAMbased, CAMbased };

enum Scheduler_type { PhysicalRegFile, ReservationStation };

enum cache_level { L2, L3, L1Directory, L2Directory, STLB, L2G };

enum Dir_type {
  ST,
  DC,
  NonDir

};
class CoreDynParam {
public:
  CoreDynParam(){};
  CoreDynParam(ParseXML *XML_interface, int ithCore_);
  //    :XML(XML_interface),
  //     ithCore(ithCore_)
  //     core_ty(inorder),
  //     rm_ty(CAMbased),
  //     scheu_ty(PhysicalRegFile),
  //     clockRate(1e9),//1GHz
  //     arch_ireg_width(32),
  //     arch_freg_width(32),
  //     phy_ireg_width(128),
  //     phy_freg_width(128),
  //     perThreadState(8),
  //     globalCheckpoint(32),
  //     instructionLength(32){};
  // ParseXML * XML;
  enum Core_type      core_ty;
  enum Renaming_type  rm_ty;
  enum Scheduler_type scheu_ty;
  double              clockRate, executionTime;
  int                 arch_ireg_width, arch_freg_width, phy_ireg_width, phy_freg_width;
  int                 num_IRF_entry, num_FRF_entry, num_ifreelist_entries, num_ffreelist_entries;
  int                 fetchW, decodeW, issueW, commitW, predictionW, fp_issueW, fp_decodeW;
  int                 perThreadState, globalCheckpoint, instruction_length, pc_width, opcode_length;
  int                 num_hthreads, pipeline_stages;
  int                 int_data_width, fp_data_width, v_address_width, p_address_width;
  bool                regWindowing, multithreaded;
  int                 numSMs, numLanes;
  bool                homoSM, homoLanes;
  ~CoreDynParam(){};
};

class CacheDynParam {
public:
  CacheDynParam(){};
  CacheDynParam(ParseXML *XML_interface, int ithCache_);
  string        name;
  enum Dir_type dir_ty;
  double        clockRate, executionTime;
  int           capacity, blockW, assoc, nbanks;
  double        throughput, latency;
  int           missb_size, fu_size, prefetchb_size, wbb_size;
  ~CacheDynParam(){};
};

class MCParam {
public:
  MCParam(){};
  MCParam(ParseXML *XML_interface, int ithCache_);
  string name;
  double clockRate;
  //  double mcTEPowerperGhz;
  //	double mcPHYperGbit;
  //	double area;
  int    llcBlockSize, dataBusWidth, addressBusWidth;
  int    opcodeW;
  int    peakDataTransferRate, memAccesses, llcBlocksize; // e.g 800M for DDR2-800M
  int    memRank, num_channels;
  double executionTime, reads, writes;

  ~MCParam(){};
};

class NoCParam {
public:
  NoCParam(){};
  NoCParam(ParseXML *XML_interface, int ithCache_);
  string name;
  double clockRate;
  int    flit_size, input_ports, output_ports, virtual_channel_per_port, input_buffer_entries_per_vc;
  int    horizontal_nodes, vertical_nodes, total_nodes;
  double executionTime, total_access, link_throughput, link_latency;
  bool   has_global_link;

  ~NoCParam(){};
};

class ProcParam {
public:
  ProcParam(){};
  ProcParam(ParseXML *XML_interface, int ithCache_);
  string name;
  int    numCore, numGPU, numSM, numLane, numL2, numSTLB, numL3, numNOC, numL1Dir, numL2Dir;
  bool   homoCore, homoL2, homoSTLB, homoL3, homoNOC, homoL1Dir, homoL2Dir;
  float  freq;

  ~ProcParam(){};
};

#endif /* BASIC_COMPONENTS_H_ */
