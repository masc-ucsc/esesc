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

#ifndef XML_PARSE_H_
#define XML_PARSE_H_
//#define CONFIG_STANDALONE 1

//#ifdef WIN32
//#define _CRT_SECURE_NO_DEPRECATE
//#endif

#include "xmlParser.h"
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef CONFIG_STANDALONE
#include <map>
#include <utility>
#include <vector>
#endif

using namespace std;

/*
void myfree(char *t); // {free(t);}
ToXMLStringTool tx,tx2;
*/

typedef uint32_t FlowID;

#define MAX_PROC_NUM 512

// all subnodes at the level of system.core(0-n)
typedef struct {
  int    prediction_width;
  char   prediction_scheme[20];
  int    predictor_size;
  int    predictor_entries;
  int    local_predictor_size;
  int    local_predictor_entries;
  int    global_predictor_entries;
  int    global_predictor_bits;
  int    chooser_predictor_entries;
  int    chooser_predictor_bits;
  double predictor_accesses;
} predictor_systemcore;
typedef struct {
  int    number_entries;
  double read_accesses;
  double write_accesses;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} lsq_systemcore;
typedef struct {
  int    number_entries;
  double total_hits;
  double total_accesses;
  double total_misses;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} itlb_systemcore;
typedef struct {
  // params
  int icache_config[20];
  int buffer_sizes[20];
  // stats
  double total_accesses;
  double read_accesses;
  double read_misses;
  double replacements;
  double read_hits;
  double total_hits;
  double total_misses;
  double miss_buffer_access;
  double fill_buffer_accesses;
  double prefetch_buffer_accesses;
  double prefetch_buffer_writes;
  double prefetch_buffer_reads;
  double prefetch_buffer_hits;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} icache_systemcore;
typedef struct {
  // params
  int number_entries;
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double write_hits;
  double read_hits;
  double read_misses;
  double write_misses;
  double total_hits;
  double total_misses;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} dtlb_systemcore;

typedef struct {
  // params
  int number_entries;
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double write_hits;
  double read_hits;
  double read_misses;
  double write_misses;
  double total_hits;
  double total_misses;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} scratchpad_systemcore;

typedef struct {
  // params
  int dcache_config[20];
  int buffer_sizes[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double total_hits;
  double total_misses;
  double read_hits;
  double write_hits;
  double read_misses;
  double write_misses;
  double replacements;
  double write_backs;
  double miss_buffer_access;
  double fill_buffer_accesses;
  double prefetch_buffer_accesses;
  double prefetch_buffer_writes;
  double prefetch_buffer_reads;
  double prefetch_buffer_hits;
  double wbb_writes;
  double wbb_reads;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} dcache_systemcore;
typedef struct {
  // params
  int BTB_config[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double total_hits;
  double total_misses;
  double read_hits;
  double write_hits;
  double read_misses;
  double write_misses;
  double replacements;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} BTB_systemcore;
typedef struct {
  // all params at the level of system.core(0-n)
  float clock_rate;
  int   machine_bits;
  int   virtual_address_width;
  int   physical_address_width;
  int   opcode_width;
  int   instruction_length;
  int   machine_type;
  int   scoore;
  int   internal_datapath_width;
  int   number_hardware_threads;
  int   fetch_width;
  int   number_instruction_fetch_ports;
  int   decode_width;
  int   issue_width;
  int   commit_width;
  int   pipelines_per_core[20];
  int   pipeline_depth[20];
  char  FPU[20];
  char  divider_multiplier[20];
  int   ALU_per_core;
  int   FPU_per_core;
  int   instruction_buffer_size;
  int   decoded_stream_buffer_size;
  int   instruction_window_scheme;
  int   instruction_window_size;
  int   fp_instruction_window_size;
  int   ROB_size;
  int   archi_Regs_IRF_size;
  int   archi_Regs_FRF_size;
  int   phy_Regs_IRF_size;
  int   phy_Regs_FRF_size;
  int   rename_scheme;
  int   register_windows_size;
  char  LSU_order[20];
  int   store_buffer_size;
  int   load_buffer_size;
  int   memory_ports;
  int   LSQ_ports;
  char  Dcache_dual_pump[20];
  int   RAS_size;
  int   fp_issue_width;
  int   prediction_width;
  int   ssit_size;
  int   lfst_size;
  int   lfst_width;
  int   scoore_store_retire_buffer_size;
  int   scoore_load_retire_buffer_size;

  // all stats at the level of system.core(0-n)
  double total_instructions;
  double int_instructions;
  double fp_instructions;
  double branch_instructions;
  double branch_mispredictions;
  double committed_instructions;
  double committed_int_instructions;
  double committed_fp_instructions;
  double load_instructions;
  double store_instructions;
  double total_cycles;
  double idle_cycles;
  double busy_cycles;
  double instruction_buffer_reads;
  double instruction_buffer_write;
  double ROB_reads;
  double ROB_writes;
  double rename_accesses;
  double fp_rename_accesses;
  double inst_window_reads;
  double inst_window_writes;
  double inst_window_wakeup_accesses;
  double inst_window_selections;
  double fp_inst_window_reads;
  double fp_inst_window_writes;
  double fp_inst_window_wakeup_accesses;
  double fp_inst_window_selections;
  double archi_int_regfile_reads;
  double archi_float_regfile_reads;
  double phy_int_regfile_reads;
  double phy_float_regfile_reads;
  double phy_int_regfile_writes;
  double phy_float_regfile_writes;
  double archi_int_regfile_writes;
  double archi_float_regfile_writes;
  double int_regfile_reads;
  double float_regfile_reads;
  double int_regfile_writes;
  double float_regfile_writes;
  double windowed_reg_accesses;
  double windowed_reg_transports;
  double function_calls;
  double context_switches;
  double ialu_access;
  double fpu_access;
  double bypassbus_access;
  double load_buffer_reads;
  double load_buffer_writes;
  double load_buffer_cams;
  double store_buffer_reads;
  double store_buffer_writes;
  double store_buffer_cams;
  double store_buffer_forwards;
  double main_memory_access;
  double main_memory_read;
  double main_memory_write;
  // all subnodes at the level of system.core(0-n)
  predictor_systemcore predictor;
  itlb_systemcore      itlb;
  lsq_systemcore       LSQ;
  lsq_systemcore       LoadQ;
  lsq_systemcore       ssit;
  lsq_systemcore       lfst;
  lsq_systemcore       strb;
  lsq_systemcore       ldrb;
  icache_systemcore    icache;
  dtlb_systemcore      dtlb;
  dcache_systemcore    dcache;
  dcache_systemcore    VPCfilter;
  dcache_systemcore    vpc_buffer;
  BTB_systemcore       BTB;

} system_core;

typedef struct {
  float             clock_rate;
  int               machine_bits;
  int               virtual_address_width;
  int               physical_address_width;
  int               opcode_width;
  int               instruction_length;
  int               machine_type;
  int               internal_datapath_width;
  int               number_hardware_threads;
  int               fetch_width;
  int               number_instruction_fetch_ports;
  int               decode_width;
  int               issue_width;
  int               commit_width;
  int               pipelines_per_core[20];
  int               pipeline_depth[20];
  char              FPU[20];
  char              divider_multiplier[20];
  int               ALU_per_core;
  int               FPU_per_core;
  int               instruction_buffer_size;
  int               decoded_stream_buffer_size;
  int               archi_Regs_IRF_size;
  int               archi_Regs_FRF_size;
  int               phy_Regs_IRF_size;
  int               phy_Regs_FRF_size;
  char              LSU_order[20];
  int               store_buffer_size;
  int               load_buffer_size;
  int               memory_ports;
  int               LSQ_ports;
  char              Dcache_dual_pump[20];
  int               RAS_size;
  int               fp_issue_width;
  int               prediction_width;
  lsq_systemcore    LSQ;
  dcache_systemcore tinycache_data;
  dcache_systemcore tinycache_inst;
} system_cfg_Lane;

typedef struct {
  double total_instructions;
  double int_instructions;
  double fp_instructions;
  double branch_instructions;
  double branch_mispredictions;
  double committed_instructions;
  double committed_int_instructions;
  double committed_fp_instructions;
  double load_instructions;
  double store_instructions;
  double total_cycles;
  double idle_cycles;
  double busy_cycles;
  double instruction_buffer_reads;
  double instruction_buffer_write;
  double archi_int_regfile_reads;
  double phy_int_regfile_reads;
  double phy_int_regfile_writes;
  double archi_int_regfile_writes;
  double int_regfile_reads;
  double int_regfile_writes;
  double function_calls;
  double context_switches;
  double ialu_access;
  double fpu_access;
  double bypassbus_access;
  double load_buffer_reads;
  double load_buffer_writes;
  double load_buffer_cams;
  double store_buffer_reads;
  double store_buffer_writes;
  double store_buffer_cams;
  double store_buffer_forwards;
  double main_memory_access;
  double main_memory_read;
  double main_memory_write;
  // all subnodes at the level of system.core(0-n)
  lsq_systemcore    LSQ;
  dcache_systemcore tinycache_data;
  dcache_systemcore tinycache_inst;
} system_Lane;

typedef struct {
  int                   number_of_lanes;
  system_cfg_Lane       homolane;
  itlb_systemcore       itlb;
  icache_systemcore     icache;
  dtlb_systemcore       dtlb;
  dcache_systemcore     dcache;
  scratchpad_systemcore scratchpad;
} system_cfg_SM;

typedef struct {
  system_Lane           lanes[MAX_PROC_NUM];
  itlb_systemcore       itlb;
  icache_systemcore     icache;
  dtlb_systemcore       dtlb;
  dcache_systemcore     dcache;
  scratchpad_systemcore scratchpad;
} system_SM;

typedef struct {
  // params
  int   Directory_type;
  int   Dir_config[20];
  int   buffer_sizes[20];
  float clockrate;
  int   ports[20];
  int   device_type;
  char  threeD_stack[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double read_misses;
  double write_misses;
  double conflicts;
} system_L1Directory;
typedef struct {
  // params
  int   Directory_type;
  int   Dir_config[20];
  int   buffer_sizes[20];
  float clockrate;
  int   ports[20];
  int   device_type;
  char  threeD_stack[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double read_misses;
  double write_misses;
  double conflicts;
} system_L2Directory;
typedef struct {
  // params
  int   STLB_config[20];
  float clockrate;
  int   ports[20];
  int   device_type;
  char  threeD_stack[20];
  int   buffer_sizes[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double total_hits;
  double total_misses;
  double read_hits;
  double write_hits;
  double read_misses;
  double write_misses;
  double replacements;
  double write_backs;
  double miss_buffer_accesses;
  double fill_buffer_accesses;
  double prefetch_buffer_accesses;
  double prefetch_buffer_writes;
  double prefetch_buffer_reads;
  double prefetch_buffer_hits;
  double wbb_writes;
  double wbb_reads;
  double conflicts;
} system_STLB;
typedef struct {
  // params
  int   L2_config[20];
  float clockrate;
  int   ports[20];
  int   device_type;
  char  threeD_stack[20];
  int   buffer_sizes[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double total_hits;
  double total_misses;
  double read_hits;
  double write_hits;
  double read_misses;
  double write_misses;
  double replacements;
  double write_backs;
  double miss_buffer_accesses;
  double fill_buffer_accesses;
  double prefetch_buffer_accesses;
  double prefetch_buffer_writes;
  double prefetch_buffer_reads;
  double prefetch_buffer_hits;
  double wbb_writes;
  double wbb_reads;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} system_L2;
typedef struct {
  // params
  int   L3_config[20];
  float clockrate;
  int   ports[20];
  int   device_type;
  char  threeD_stack[20];
  int   buffer_sizes[20];
  // stats
  double total_accesses;
  double read_accesses;
  double write_accesses;
  double total_hits;
  double total_misses;
  double read_hits;
  double write_hits;
  double read_misses;
  double write_misses;
  double replacements;
  double write_backs;
  double miss_buffer_accesses;
  double fill_buffer_accesses;
  double prefetch_buffer_accesses;
  double prefetch_buffer_writes;
  double prefetch_buffer_reads;
  double prefetch_buffer_hits;
  double wbb_writes;
  double wbb_reads;
  double conflicts;
  double force_lkg_w;
  double force_rddyn_w;
  double force_wrdyn_w;
  bool   force_lkg;
  bool   force_rddyn;
  bool   force_wrdyn;
} system_L3;
typedef struct {
  // params
  int number_of_inputs_of_crossbars;
  int number_of_outputs_of_crossbars;
  int flit_bits;
  int input_buffer_entries_per_port;
  int ports_of_input_buffer[20];
  // stats
  double crossbar_accesses;
} xbar0_systemNoC;
typedef struct {
  // params
  float           clockrate;
  char            topology[20];
  int             horizontal_nodes;
  int             vertical_nodes;
  int             has_global_link;
  int             link_throughput;
  int             link_latency;
  int             input_ports;
  int             output_ports;
  int             virtual_channel_per_port;
  int             flit_bits;
  int             input_buffer_entries_per_vc;
  int             ports_of_input_buffer[20];
  int             dual_pump;
  int             number_of_crossbars;
  char            crossbar_type[20];
  char            crosspoint_type[20];
  xbar0_systemNoC xbar0;
  int             arbiter_type;
  // stats
  double total_accesses;
} system_NoC;
typedef struct {
  // params
  int mem_tech_node;
  int device_clock;
  int peak_transfer_rate;
  int internal_prefetch_of_DRAM_chip;
  int capacity_per_channel;
  int number_ranks;
  int num_banks_of_DRAM_chip;
  int Block_width_of_DRAM_chip;
  int output_width_of_DRAM_chip;
  int page_size_of_DRAM_chip;
  int burstlength_of_DRAM_chip;
  // stats
  double memory_accesses;
  double memory_reads;
  double memory_writes;
} system_mem;
typedef struct {
  // params
  int mc_clock;
  int llc_line_length;
  int number_mcs;
  int memory_channels_per_mc;
  int req_window_size_per_channel;
  int IO_buffer_size_per_channel;
  int databus_width;
  int addressbus_width;
  // stats
  double memory_accesses;
  double memory_reads;
  double memory_writes;
} system_mc;

typedef struct {
  int           number_of_SMs;
  system_cfg_SM homoSM;
  system_SM     SMS[MAX_PROC_NUM];
  system_L2     L2;
} system_GPU;

typedef struct root_system_typ {
  // All number_of_* at the level of 'system' Ying 03/21/2009
  uint32_t number_of_cores;
  uint32_t number_of_GPU;
  uint32_t number_of_L1Directories;
  uint32_t number_of_L2Directories;
  uint32_t number_of_L2s;
  uint32_t number_of_STLBs;
  uint32_t number_of_L3s;
  uint32_t number_of_NoCs;
  uint32_t number_of_dir_levels;
  uint32_t domain_size;
  uint32_t first_level_dir;
  // All params at the level of 'system'
  int    homogeneous_cores;
  int    homogeneous_L1Directories;
  int    homogeneous_L2Directories;
  double core_tech_node;
  float  target_core_clockrate;
  int    target_chip_area;
  int    temperature;
  int    number_cache_levels;
  int    L1_property;
  int    L2_property;
  int    homogeneous_L2s;
  int    homogeneous_STLBs;
  int    L3_property;
  int    homogeneous_L3s;
  int    homogeneous_NoCs;
  int    homogeneous_ccs;
  int    Max_area_deviation;
  int    Max_power_deviation;
  int    device_type;
  int    opt_dynamic_power;
  int    opt_lakage_power;
  float  opt_clockrate;
  int    opt_area;
  int    interconnect_projection_type;
  int    machine_bits;
  int    virtual_address_width;
  int    physical_address_width;
  int    virtual_memory_page_size;
  double total_cycles;
  double executionTime;
  // system.core(0-n):3rd level
  system_core        core[MAX_PROC_NUM];
  system_GPU         gpu;
  system_L1Directory L1Directory[MAX_PROC_NUM];
  system_L2Directory L2Directory[MAX_PROC_NUM];
  system_L2          L2[MAX_PROC_NUM];
  system_STLB        STLB[MAX_PROC_NUM];
  system_L3          L3[MAX_PROC_NUM];
  system_NoC         NoC[MAX_PROC_NUM];
  system_mem         mem;
  system_mc          mc;
  float              scale_dyn;
  float              scale_lkg;
} root_system;

class ParseXML {
public:
  void parse(char *filepath);
  void parseEsescConf(const char *section);
  void initialize(vector<uint32_t> *stats_vector, map<string, int> mcpat_map, vector<uint32_t> *cidx, vector<uint32_t> *gidx);

  // public:
  root_system sys;

  vector<FlowID> *           stats_vec;
  map<string, int>           str2pos_map;
  map<string, int>::iterator it1;
  vector<FlowID> *           coreIndex;
  vector<FlowID> *           gpuIndex;

  typedef pair<int, int> int2int;
  int                    cntr_pos_value(string cntr_name);
  bool                   check_cntr(string cntr_name);
  void                   updateCntrValues(vector<FlowID> *stats_vector, map<string, int> mcpat_map);

  void getGeneralParams();
  void getCoreParams();
  void getGPUParams();
  void getmimdGPUParams();
  void getMemParams();
  void getMemoryObj(const char *block, const char *field, FlowID Id);
  void getConfMemObj(std::vector<char *> vPars, FlowID Id);
};

#endif /* XML_PARSE_H_ */
