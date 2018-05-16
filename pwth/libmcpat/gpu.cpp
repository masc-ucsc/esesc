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
#include "gpu.h"
#include "XML_Parse.h"
#include "basic_circuit.h"
#include "const.h"
#include "io.h"
#include "parameter.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>

bool gpu_tcd_present;
bool gpu_tci_present;

InstFetchUG::InstFetchUG(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_,
                         const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , IB(0) { /*{{{*/

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  // Instruction buffer
  int data = XML->sys.gpu.homoSM.homolane
                 .instruction_length; // icache.caches.l_ip.line_sz; //multiple threads timing sharing the instruction buffer.
  interface_ip.is_cache = false;
  interface_ip.pure_ram = true;
  interface_ip.pure_cam = false;
  interface_ip.line_sz  = int(ceil(data / 8.0));
  interface_ip.cache_sz = XML->sys.gpu.homoSM.homolane.number_hardware_threads *
                                      XML->sys.gpu.homoSM.homolane.instruction_buffer_size * interface_ip.line_sz >
                                  64
                              ? XML->sys.gpu.homoSM.homolane.number_hardware_threads *
                                    XML->sys.gpu.homoSM.homolane.instruction_buffer_size * interface_ip.line_sz
                              : 64;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;

  // NOTE: Assuming IB is time slice shared among threads, every fetch op will at least fetch "fetch width" instructions.
  interface_ip.num_rw_ports    = 0;
  interface_ip.num_rd_ports    = 1;
  interface_ip.num_wr_ports    = 1;
  interface_ip.num_se_rd_ports = 0;
  IB                           = new ArrayST(&interface_ip, "Lane InstBuffer");
  IB->area.set_area(IB->area.get_area() + IB->local_result.area);
  area.set_area(area.get_area() + IB->local_result.area);
  // output_data_csv(IB.IB.local_result);

  //	  inst_decoder.opcode_length = XML->sys.gpu.homoSM.homolane.opcode_width;
  //	  inst_decoder.init_decoder(is_default, &interface_ip);
  //	  inst_decoder.full_decoder_power();

} /*}}}*/

LoadStoreUG::LoadStoreUG(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_,
                         const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , LSQ(0) { /*{{{*/

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  /*
   * LSU--in-order processors do not have separate load queue: unified lsq
   * partitioned among threads
   * it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
   */

  int tag = XML->sys.gpu.homoSM.homolane.opcode_width + XML->sys.virtual_address_width +
            int(ceil(log2(XML->sys.gpu.homoSM.homolane.number_hardware_threads))) + EXTRA_TAG_BITS;
  int data                  = XML->sys.machine_bits; // 64 is the data width
  interface_ip.is_cache     = true;
  interface_ip.pure_ram     = false;
  interface_ip.pure_cam     = false;
  interface_ip.line_sz      = int(ceil(data / 32.0)) * 4;
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = tag;
  interface_ip.cache_sz =
      XML->sys.gpu.homoSM.homolane.store_buffer_size * interface_ip.line_sz * XML->sys.gpu.homoSM.homolane.number_hardware_threads;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 4;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 1;
  interface_ip.num_rd_ports        = 0; // XML->sys.gpu.homoSM.homolane.LSQ_ports;
  interface_ip.num_wr_ports        = 0; // XML->sys.gpu.homoSM.homolane.LSQ_ports;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 1; // XML->sys.gpu.homoSM.homolane.LSQ_ports;
  LSQ                              = new ArrayST(&interface_ip, "Lane Load(Store)Queue");
  LSQ->area.set_area(LSQ->area.get_area() + LSQ->local_result.area);
  area.set_area(area.get_area() + LSQ->local_result.area);
  // output_data_csv(LSQ.LSQ.local_result);
  lsq_height = LSQ->local_result.cache_ht /*XML->sys.gpu.homoSM.homolane.number_hardware_threads*/;

} /*}}}*/

RegFUG::RegFUG(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , IRF(0) { /*{{{*/
  /*
   * processors have separate architectural register files for each thread.
   * therefore, the bypass buses need to travel across all the register files.
   */
  int data;

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  //**********************************IRF***************************************
  data                             = coredynp.int_data_width;
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = int(ceil(data / 32.0)) * 4;
  interface_ip.cache_sz            = coredynp.num_IRF_entry * interface_ip.line_sz;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 2;
  interface_ip.num_wr_ports        = 1;
  interface_ip.num_se_rd_ports     = 0;

  // a hack
  // interface_ip.ram_cell_tech_type             = 2;
  // interface_ip.peri_global_tech_type          = 2;
  // interface_ip.data_arr_ram_cell_tech_type    = 2;
  // interface_ip.data_arr_peri_global_tech_type = 2;
  // interface_ip.tag_arr_ram_cell_tech_type     = 2;
  // interface_ip.tag_arr_peri_global_tech_type  = 2;

  IRF = new ArrayST(&interface_ip, "Lane Register File");

  IRF->area.set_area(IRF->local_result.area);
  area.set_area(IRF->local_result.area);
  // output_data_csv(IRF.RF.local_result);
  int_regfile_height = IRF->local_result.cache_ht * XML->sys.gpu.homoSM.homolane.number_hardware_threads;

} /*}}}*/

EXECUG::EXECUG(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_, double lsq_height_,
               const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , lsq_height(lsq_height_)
    , coredynp(dyn_p_)
    , rfu(0)
    , fpu(0)
    , exeu(0) { /*{{{*/

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  rfu           = new RegFUG(XML, ithSM, ithLane, &interface_ip, coredynp);
  int num_fpu, num_alu;
  num_fpu = 1;
  num_alu = 2;
  exeu    = new FunctionalUnit(&interface_ip, ALU, num_alu);
  fpu     = new FunctionalUnit(&interface_ip, FPU, num_fpu);
  area.set_area(area.get_area() + exeu->area.get_area() + 0.1 * fpu->area.get_area() /*+ rfu->area.get_area()*/);

  /*
   * broadcast logic, including int-broadcast; int_tag-broadcast; fp-broadcast; fp_tag-broadcast
   * integer by pass has two paths and fp has 3 paths.
   * on the same bus there are multiple tri-state drivers and muxes that go to different components on the same bus
   */
  interface_ip.wire_is_mat_type = 2; // start from semi-global since local wires are already used
  interface_ip.wire_os_mat_type = 2;
  interface_ip.throughput       = 1.0 / clockRate;
  interface_ip.latency          = 1.0 / clockRate;

  int_bypass   = new interconnect("Int Bypass Data", 1, 1, int(ceil(float(XML->sys.machine_bits / 32.0)) * 32),
                                rfu->int_regfile_height + exeu->FU_height, &interface_ip, 3);
  intTagBypass = new interconnect("Int Bypass tag", 1, 1, coredynp.perThreadState,
                                  2 * rfu->int_regfile_height + exeu->FU_height + fpu->FU_height, &interface_ip, 3);
} /*}}}*/

TCD::TCD(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , tc_data(0) { /*{{{*/
  /*
   * processors have separate architectural register files for each thread.
   * therefore, the bypass buses need to travel across all the register files.
   */

  clockRate             = coredynp.clockRate;
  executionTime         = coredynp.executionTime;
  interface_ip.is_cache = true;
  interface_ip.pure_cam = false;
  interface_ip.pure_ram = false;
  // Dcache
  int size                         = XML->sys.gpu.homoSM.homolane.tinycache_data.dcache_config[0];
  int line                         = XML->sys.gpu.homoSM.homolane.tinycache_data.dcache_config[1];
  int assoc                        = XML->sys.gpu.homoSM.homolane.tinycache_data.dcache_config[2];
  int banks                        = XML->sys.gpu.homoSM.homolane.tinycache_data.dcache_config[3];
  int idx                          = int(ceil(log2(size / line / assoc)));
  int tag                          = XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = size;
  interface_ip.line_sz             = line;
  interface_ip.assoc               = assoc;
  interface_ip.nbanks              = banks;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 0; // debug?0:XML->sys.gpu.homoSM.dcache.dcache_config[5];
  interface_ip.throughput          = XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 1;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 0;

  tc_data = new ArrayST(&interface_ip, "Lane Register File");
  tc_data->area.set_area(tc_data->local_result.area);
  area.set_area(tc_data->local_result.area);
  // output_data_csv(tc_data.RF.local_result);

  const char *section = SescConf->getCharPtr("gpuCore", "dfilter");
  if(SescConf->checkDouble(section, "readOp_dyn")) {
    tc_data->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
  }
  if(SescConf->checkDouble(section, "readOp_lkg")) {
    tc_data->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
  }
  if(SescConf->checkDouble(section, "writeOp_dyn")) {
    tc_data->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
  }
  if(SescConf->checkDouble(section, "writeOp_lkg")) {
    tc_data->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
  }

} /*}}}*/

TCI::TCI(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , tc_inst(0) { /*{{{*/

  clockRate             = coredynp.clockRate;
  executionTime         = coredynp.executionTime;
  interface_ip.is_cache = true;
  interface_ip.pure_cam = false;
  interface_ip.pure_ram = false;
  // Dcache
  int size                         = XML->sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[0];
  int line                         = XML->sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[1];
  int assoc                        = XML->sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[2];
  int banks                        = XML->sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[3];
  int idx                          = int(ceil(log2(size / line / assoc)));
  int tag                          = XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = size;
  interface_ip.line_sz             = line;
  interface_ip.assoc               = assoc;
  interface_ip.nbanks              = banks;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 0; // debug?0:XML->sys.gpu.homoSM.dcache.dcache_config[5];
  interface_ip.throughput          = XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 1;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 0;
  tc_inst                          = new ArrayST(&interface_ip, "Lane icache filter");
  tc_inst->area.set_area(tc_inst->local_result.area);
  area.set_area(area.get_area() + tc_inst->local_result.area);
  // output_data_csv(tc_inst.local_result);

  const char *section = SescConf->getCharPtr("gpuCore", "ifilter");
  if(SescConf->checkDouble(section, "readOp_dyn")) {
    tc_inst->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
  }
  if(SescConf->checkDouble(section, "readOp_lkg")) {
    tc_inst->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
  }
  if(SescConf->checkDouble(section, "writeOp_dyn")) {
    tc_inst->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
  }
  if(SescConf->checkDouble(section, "writeOp_lkg")) {
    tc_inst->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
  }

} /*}}}*/

Lane::Lane(ParseXML *XML_interface, int ithSM_, int ithLane_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , ithLane(ithLane_)
    , interface_ip(*interface_ip_)
    , ifu(0)
    , lsu(0)
    , exu(0)
    , corepipe(0)
    , coredynp(dyn_p_) { /*{{{*/
  /*
   * initialize, compute and optimize individual components.
   */

  double pipeline_area_per_unit;

  // clock codes by eka, just to see how much time each method takes

  set_Lane_param();
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  // ifu      = new InstFetchUG(XML, ithSM, ithLane,  &interface_ip,coredynp);
  // lsu      = new LoadStoreUG(XML, ithSM, ithLane,  &interface_ip,coredynp);
  exu      = new EXECUG(XML, ithSM, ithLane, &interface_ip, 0, coredynp);
  corepipe = new PipelinePower(&interface_ip, coredynp);

  pipeline_area_per_unit = corepipe->area.get_area() / 4.0; // ASN: Why is this divided by 4?

  area.set_area(area.get_area() + corepipe->area.get_area());
  // ifu->area.set_area(ifu->area.get_area() + pipeline_area_per_unit);
  // lsu->area.set_area(lsu->area.get_area() + pipeline_area_per_unit);
  exu->area.set_area(exu->area.get_area() + pipeline_area_per_unit);

  area.set_area(exu->area.get_area()
                //+ lsu->area.get_area()
                //+ ifu->area.get_area()
  );

  if(gpu_tcd_present) {
    // Instantiate tcd
    tcdata = new TCD(XML, ithSM, ithLane, &interface_ip, coredynp);
  }

  if(gpu_tci_present) {
    // Instantiate tci
    tcinst = new TCI(XML, ithSM, ithLane, &interface_ip, coredynp);
  }

} /*}}}*/

MemManUG_I::MemManUG_I(ParseXML *XML_interface, int ithSM_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , itlb(0) { /*{{{*/
  int  tag, data;
  bool debug = false;

  clockRate                 = coredynp.clockRate;
  executionTime             = coredynp.executionTime;
  interface_ip.is_cache     = true;
  interface_ip.pure_cam     = false;
  interface_ip.pure_ram     = false;
  interface_ip.specific_tag = 1;
  // Itlb TLBs are partioned among threads according to Nigara and Nehalem
  tag                      = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) + EXTRA_TAG_BITS;
  data                     = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
  interface_ip.tag_w       = tag;
  interface_ip.line_sz     = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz    = XML->sys.gpu.homoSM.itlb.number_entries * interface_ip.line_sz;
  interface_ip.assoc       = 0;
  interface_ip.nbanks      = 1;
  interface_ip.out_w       = interface_ip.line_sz * 8;
  interface_ip.access_mode = 2;
  interface_ip.throughput  = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.icache.icache_config[4] / clockRate;
  interface_ip.latency     = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.icache.icache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 1;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 1;
  itlb                             = new ArrayST(&interface_ip, "SM ITLB");
  itlb->area.set_area(itlb->area.get_area() + itlb->local_result.area);
  area.set_area(area.get_area() + itlb->local_result.area);
  // output_data_csv(itlb.tlb.local_result);
} /*}}}*/

MemManUG_D::MemManUG_D(ParseXML *XML_interface, int ithSM_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , dtlb(0) { /*{{{*/
  int  tag, data;
  bool debug = false;

  clockRate                 = coredynp.clockRate;
  executionTime             = coredynp.executionTime;
  interface_ip.is_cache     = true;
  interface_ip.pure_cam     = false;
  interface_ip.pure_ram     = false;
  interface_ip.specific_tag = 1;

  tag = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) + EXTRA_TAG_BITS;

  data                             = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz            = XML->sys.gpu.homoSM.dtlb.number_entries * interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 1;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 1;

  dtlb = new ArrayST(&interface_ip, "SM DTLB");
  dtlb->area.set_area(dtlb->area.get_area() + dtlb->local_result.area);
  area.set_area(area.get_area() + dtlb->local_result.area);
  // output_data_csv(dtlb.tlb.local_result);

} /*}}}*/

SMSharedCache::SMSharedCache(ParseXML *XML_interface, int ithSM_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_) { /*{{{*/

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  if(SescConf->checkCharPtr("gpuCore", "scratchpad")) {
    const char *section = SescConf->getCharPtr("gpuCore", "scratchpad");
    if(strcmp(section, "None") != 0) {
      // TODO : Read values directly from esesc.conf

      int size                         = XML->sys.gpu.homoSM.scratchpad.number_entries;
      int banks                        = XML->sys.gpu.homoSM.dcache.dcache_config[3];
      interface_ip.is_cache            = false;
      interface_ip.pure_cam            = false;
      interface_ip.pure_ram            = true;
      interface_ip.line_sz             = 1;
      interface_ip.cache_sz            = size;
      interface_ip.assoc               = 1;
      interface_ip.nbanks              = banks;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 1;
      interface_ip.throughput          = 1.0 / clockRate;
      interface_ip.latency             = 1.0 / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = 0;
      interface_ip.num_rd_ports        = 2;
      interface_ip.num_wr_ports        = 2;
      interface_ip.num_se_rd_ports     = 0;

      // a hack
      //  interface_ip.ram_cell_tech_type             = 2;
      //  interface_ip.peri_global_tech_type          = 2;
      //  interface_ip.data_arr_ram_cell_tech_type    = 2;
      //  interface_ip.data_arr_peri_global_tech_type = 2;
      //  interface_ip.tag_arr_ram_cell_tech_type     = 2;
      //  interface_ip.tag_arr_peri_global_tech_type  = 2;

      scratchcache = new ArrayST(&interface_ip, "SM scratchpad");

      // output_data_csv(dcache.caches.local_result);

      // If provided, overwrite the cacti calculate energies with the energies defined in esesc.conf

      if(SescConf->checkDouble(section, "readOp_dyn")) {
        scratchcache->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
      }
      if(SescConf->checkDouble(section, "readOp_lkg")) {
        scratchcache->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
      }
      if(SescConf->checkDouble(section, "writeOp_dyn")) {
        scratchcache->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
      }
      if(SescConf->checkDouble(section, "writeOp_lkg")) {
        scratchcache->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
      }

    } else {
      // There should be a scratchpad for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n scratchpad for the GPU  must point to a valid section, cannot be \"None\" \n");
      exit(-1);
    }
  } else {
    // There should be a scratchpad for the GPU.
    I(0);
    // error;
    fprintf(stderr, "\n scratchpad for the GPU not defined \n");
    exit(-1);
  }
} /*}}}*/

SM::SM(ParseXML *XML_interface, int ithSM_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithSM(ithSM_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_) { /*{{{*/
  /*
   * initialize, compute and optimize individual components.
   */

  set_SM_param();
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  bool debug = false;

  // Assuming homogeneous lanes.
  Lane *ln = 0;
  ln       = new Lane(XML, 0, 0, &interface_ip, coredynp);
  for(int j = 0; j < coredynp.numLanes; j++)
    lanes.push_back(ln);
  area.set_area(area.get_area() + ln->area.get_area() * coredynp.numLanes);

  lane.area.set_area(area.get_area());

  mmu_i = new MemManUG_I(XML, ithSM, &interface_ip, coredynp);
  mmu_d = new MemManUG_D(XML, ithSM, &interface_ip, coredynp);
  mmu_i->area.set_area(mmu_i->area.get_area());
  mmu_d->area.set_area(mmu_d->area.get_area());

  icache.area.set_area(mmu_i->itlb->local_result.area);
  dcache.area.set_area(mmu_d->dtlb->local_result.area);

  // Assuming all L1 caches are virtually idxed physically tagged.
  // cache
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  // Icache
  if(SescConf->checkCharPtr("gpuCore", "SM_IL1")) {
    const char *section = SescConf->getCharPtr("gpuCore", "SM_IL1");
    if(strcmp(section, "None") != 0) {
      int size                      = XML->sys.gpu.homoSM.icache.icache_config[0];
      int line                      = XML->sys.gpu.homoSM.icache.icache_config[1];
      int assoc                     = XML->sys.gpu.homoSM.icache.icache_config[2];
      int banks                     = XML->sys.gpu.homoSM.icache.icache_config[3];
      int idx                       = debug ? 9 : int(ceil(log2(size / line / assoc)));
      int tag                       = debug ? 51 : XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
      interface_ip.specific_tag     = 1;
      interface_ip.tag_w            = tag;
      interface_ip.line_sz          = line;
      interface_ip.cache_sz         = size;
      interface_ip.assoc            = assoc;
      interface_ip.nbanks           = banks;
      interface_ip.out_w            = interface_ip.line_sz * 8;
      interface_ip.access_mode      = 0; // debug?0:XML->sys.gpu.homoSM.icache.icache_config[5];
      interface_ip.throughput       = XML->sys.gpu.homoSM.icache.icache_config[4] / clockRate;
      interface_ip.latency          = XML->sys.gpu.homoSM.icache.icache_config[5] / clockRate;
      interface_ip.is_cache         = true;
      interface_ip.pure_cam         = false;
      interface_ip.pure_ram         = false;
      interface_ip.num_rw_ports     = 1;
      interface_ip.num_rd_ports     = 0;
      interface_ip.num_wr_ports     = 0;
      interface_ip.num_se_rd_ports  = 0;
      interface_ip.num_search_ports = 0;
      icache.caches                 = new ArrayST(&interface_ip, "SM Icache");

      scktRatio         = g_tp.sckt_co_eff;
      chip_PR_overhead  = g_tp.chip_layout_overhead;
      macro_PR_overhead = g_tp.macro_layout_overhead;
      icache.area.set_area(icache.area.get_area() + icache.caches->local_result.area);
      area.set_area(area.get_area() + icache.caches->local_result.area);
      // output_data_csv(icache.caches.local_result);

      /*
       *iCache controllers
       *miss buffer Each MSHR contains enough state
       *to handle one or more accesses of any type to a single memory line.
       *Due to the generality of the MSHR mechanism,
       *the amount of state involved is non-trivial:
       *including the address, pointers to the cache entry and destination register,
       *written data, and various other pieces of state.
       */
      // prefetch buffer
      tag      = XML->sys.physical_address_width + EXTRA_TAG_BITS; // check with previous entries to decide wthether to merge.
      int data = icache.caches->l_ip.line_sz;                      // separate queue to prevent from cache polution.
      interface_ip.specific_tag        = 1;
      interface_ip.tag_w               = tag;
      interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
      interface_ip.cache_sz            = XML->sys.gpu.homoSM.icache.buffer_sizes[2] * interface_ip.line_sz;
      interface_ip.assoc               = 0;
      interface_ip.nbanks              = 1;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 2;
      interface_ip.throughput          = XML->sys.gpu.homoSM.icache.icache_config[4] / clockRate;
      interface_ip.latency             = XML->sys.gpu.homoSM.icache.icache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = 1;
      interface_ip.num_rd_ports        = 0;
      interface_ip.num_wr_ports        = 0;
      interface_ip.num_se_rd_ports     = 0;
      interface_ip.num_search_ports    = 1;
      icache.prefetchb                 = new ArrayST(&interface_ip, "SM icache prefetchBuffer");
      icache.area.set_area(icache.area.get_area() + icache.prefetchb->local_result.area);
      area.set_area(area.get_area() + icache.prefetchb->local_result.area);
      // output_data_csv(icache.prefetchb.local_result);

      float icache_height = sqrt(icache.area.get_area());
      icache.bcast        = new interconnect("SM icache broadcast", 1, 1, int(ceil(float(XML->sys.machine_bits / 32.0)) * 32),
                                      icache_height, &interface_ip, 3);
      icache.area.set_area(icache.area.get_area() + icache.bcast->local_result.area);
      area.set_area(area.get_area() + icache.bcast->local_result.area);

      if(SescConf->checkDouble(section, "readOp_dyn")) {
        icache.caches->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
      }
      if(SescConf->checkDouble(section, "readOp_lkg")) {
        icache.caches->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
      }
      if(SescConf->checkDouble(section, "writeOp_dyn")) {
        icache.caches->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
      }
      if(SescConf->checkDouble(section, "writeOp_lkg")) {
        icache.caches->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
      }
    } else {
      // There should be a SM_IL1 for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n SM_IL1 for the GPU must point to a valid section, cannot be \"None\" \n");
      exit(-1);
    }
  } else {
    // There should be a SM_IL1 for the GPU.
    I(0);
    // error;
    fprintf(stderr, "\n SM_IL1 for the GPU not defined \n");
    exit(-1);
  }

  // Scratchpad
  scratchpad = new SMSharedCache(XML, ithSM, &interface_ip, coredynp);
  scratchpad->area.set_area(scratchpad->area.get_area() + scratchpad->scratchcache->local_result.area);
  area.set_area(area.get_area() + scratchpad->area.get_area());

  // dcache.area.set_area (dcache.area.get_area() + scratchpad.scratchcache->local_result.area);
  // area.set_area(area.get_area() + scratchpad.scratchcache->local_result.area);

  // Dcache
  if(SescConf->checkCharPtr("gpuCore", "SM_DL1")) {
    const char *section = SescConf->getCharPtr("gpuCore", "SM_DL1");
    if(strcmp(section, "None") != 0) {

      interface_ip.num_search_ports = XML->sys.gpu.homoSM.homolane.memory_ports;
      interface_ip.is_cache         = true;
      interface_ip.pure_cam         = false;
      interface_ip.pure_ram         = false;

      // Dcache
      int size                         = XML->sys.gpu.homoSM.dcache.dcache_config[0];
      int line                         = XML->sys.gpu.homoSM.dcache.dcache_config[1];
      int assoc                        = XML->sys.gpu.homoSM.dcache.dcache_config[2];
      int banks                        = XML->sys.gpu.homoSM.dcache.dcache_config[3];
      int idx                          = int(ceil(log2(size / line / assoc)));
      int tag                          = XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
      interface_ip.specific_tag        = 1;
      interface_ip.tag_w               = tag;
      interface_ip.cache_sz            = size;
      interface_ip.line_sz             = line;
      interface_ip.assoc               = assoc;
      interface_ip.nbanks              = banks;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 0; // debug?0:XML->sys.gpu.homoSM.dcache.dcache_config[5];
      interface_ip.throughput          = XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency             = XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.is_cache            = true;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = 2;
      interface_ip.num_rd_ports        = 0;
      interface_ip.num_wr_ports        = 0;
      interface_ip.num_se_rd_ports     = 0;
      interface_ip.num_search_ports    = 0;
      dcache.caches                    = new ArrayST(&interface_ip, "SM dcache");

      dcache.area.set_area(dcache.area.get_area() + dcache.caches->local_result.area);
      area.set_area(area.get_area() + dcache.caches->local_result.area);
      // output_data_csv(dcache.caches.local_result);

      // dCache controllers
      // miss buffer
      tag                       = XML->sys.physical_address_width + EXTRA_TAG_BITS;
      int data                  = (XML->sys.physical_address_width) + int(ceil(log2(size / line))) + dcache.caches->l_ip.line_sz;
      interface_ip.specific_tag = 1;
      interface_ip.tag_w        = tag;
      interface_ip.line_sz      = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
      interface_ip.cache_sz     = XML->sys.gpu.homoSM.dcache.buffer_sizes[0] * interface_ip.line_sz;
      interface_ip.assoc        = 0;
      interface_ip.nbanks       = 1;
      interface_ip.out_w        = interface_ip.line_sz * 8;
      interface_ip.access_mode  = 2;
      interface_ip.throughput   = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency      = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = debug ? 1 : XML->sys.gpu.homoSM.homolane.memory_ports;
      ;
      interface_ip.num_rd_ports     = 0;
      interface_ip.num_wr_ports     = 0;
      interface_ip.num_se_rd_ports  = 0;
      interface_ip.num_search_ports = 1;
      dcache.missb                  = new ArrayST(&interface_ip, "SM dcache MSHR");
      dcache.area.set_area(dcache.area.get_area() + dcache.missb->local_result.area);
      area.set_area(area.get_area() + dcache.missb->local_result.area);
      // output_data_csv(dcache.missb.local_result);

      // fill buffer
      tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
      data                             = dcache.caches->l_ip.line_sz;
      interface_ip.specific_tag        = 1;
      interface_ip.tag_w               = tag;
      interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
      interface_ip.cache_sz            = data * XML->sys.gpu.homoSM.dcache.buffer_sizes[1];
      interface_ip.assoc               = 0;
      interface_ip.nbanks              = 1;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 2;
      interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = debug ? 1 : XML->sys.gpu.homoSM.homolane.memory_ports;
      ;
      interface_ip.num_rd_ports    = 0;
      interface_ip.num_wr_ports    = 0;
      interface_ip.num_se_rd_ports = 0;
      dcache.ifb                   = new ArrayST(&interface_ip, "SM dcache FillBuffer");
      dcache.area.set_area(dcache.area.get_area() + dcache.ifb->local_result.area);
      area.set_area(area.get_area() + dcache.ifb->local_result.area);
      // output_data_csv(dcache.ifb.local_result);

      // prefetch buffer
      tag  = XML->sys.physical_address_width + EXTRA_TAG_BITS; // check with previous entries to decide wthether to merge.
      data = dcache.caches->l_ip.line_sz;                      // separate queue to prevent from cache polution.
      interface_ip.specific_tag        = 1;
      interface_ip.tag_w               = tag;
      interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
      interface_ip.cache_sz            = XML->sys.gpu.homoSM.dcache.buffer_sizes[2] * interface_ip.line_sz;
      interface_ip.assoc               = 0;
      interface_ip.nbanks              = 1;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 2;
      interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = debug ? 1 : XML->sys.gpu.homoSM.homolane.memory_ports;
      interface_ip.num_rd_ports        = 0;
      interface_ip.num_wr_ports        = 0;
      interface_ip.num_se_rd_ports     = 0;
      dcache.prefetchb                 = new ArrayST(&interface_ip, "SM dcache prefetch Buffer");
      dcache.area.set_area(dcache.area.get_area() + dcache.prefetchb->local_result.area);
      area.set_area(area.get_area() + dcache.prefetchb->local_result.area);
      // output_data_csv(dcache.prefetchb.local_result);

      // WBB
      tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
      data                             = dcache.caches->l_ip.line_sz;
      interface_ip.specific_tag        = 1;
      interface_ip.tag_w               = tag;
      interface_ip.line_sz             = data;
      interface_ip.cache_sz            = XML->sys.gpu.homoSM.dcache.buffer_sizes[3] * interface_ip.line_sz;
      interface_ip.assoc               = 0;
      interface_ip.nbanks              = 1;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 2;
      interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = XML->sys.gpu.homoSM.homolane.memory_ports;
      interface_ip.num_rd_ports        = 0;
      interface_ip.num_wr_ports        = 0;
      interface_ip.num_se_rd_ports     = 0;
      dcache.wbb                       = new ArrayST(&interface_ip, "SM dcache WBB");
      dcache.area.set_area(dcache.area.get_area() + dcache.wbb->local_result.area);
      area.set_area(area.get_area() + dcache.wbb->local_result.area);
      // output_data_csv(dcache.wbb.local_result);

      if(SescConf->checkDouble(section, "readOp_dyn")) {
        dcache.caches->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
      }
      if(SescConf->checkDouble(section, "readOp_lkg")) {
        dcache.caches->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
      }
      if(SescConf->checkDouble(section, "writeOp_dyn")) {
        dcache.caches->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
      }
      if(SescConf->checkDouble(section, "writeOp_lkg")) {
        dcache.caches->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
      }

    } else {
      // There should be a SM_DL1 for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n SM_DL1 for the GPU must point to a valid section, cannot be \"None\" \n");
      exit(-1);
    }
  } else {
    // There should be a SM_DL1 for the GPU.
    I(0);
    // error;
    fprintf(stderr, "\n SM_DL1 for the GPU not defined \n");
    exit(-1);
  }

  // XBAR
  if(SescConf->checkCharPtr("gpuCore", "Xbar")) {
    const char *section = SescConf->getCharPtr("gpuCore", "Xbar");
    if(strcmp(section, "None") != 0) {

      int tag = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) +
                int(ceil(log2(XML->sys.gpu.homoSM.homolane.number_hardware_threads))) + EXTRA_TAG_BITS;
      int data                  = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
      interface_ip.specific_tag = 1;
      interface_ip.tag_w        = tag;
      interface_ip.line_sz      = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
      interface_ip.cache_sz     = XML->sys.gpu.homoSM.number_of_lanes < 64 ? 64 : XML->sys.gpu.homoSM.number_of_lanes;
      ;
      interface_ip.assoc               = 0;
      interface_ip.nbanks              = 1;
      interface_ip.out_w               = interface_ip.line_sz * 8;
      interface_ip.access_mode         = 1;
      interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[4] / clockRate;
      interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.gpu.homoSM.dcache.dcache_config[5] / clockRate;
      interface_ip.obj_func_dyn_energy = 0;
      interface_ip.obj_func_dyn_power  = 0;
      interface_ip.obj_func_leak_power = 0;
      interface_ip.obj_func_cycle_t    = 1;
      interface_ip.num_rw_ports        = 1;
      interface_ip.num_rd_ports        = 0; // XML->sys.gpu.homoSM.homolane.LSQ_ports;
      interface_ip.num_wr_ports        = 0; // XML->sys.gpu.homoSM.homolane.LSQ_ports;
      interface_ip.num_se_rd_ports     = 0;
      interface_ip.num_search_ports    = 1; // XML->sys.gpu.homoSM.homolane.LSQ_ports;

      dcache.xbar = new ArrayST(&interface_ip, "SM dcache xbar");
      dcache.xbar->area.set_area(dcache.xbar->area.get_area() + dcache.xbar->local_result.area);
      area.set_area(area.get_area() + dcache.xbar->local_result.area);

      if(SescConf->checkDouble(section, "readOp_dyn")) {
        dcache.xbar->local_result.power.readOp.dynamic = SescConf->getDouble(section, "readOp_dyn");
      }
      if(SescConf->checkDouble(section, "readOp_lkg")) {
        dcache.xbar->local_result.power.readOp.leakage = SescConf->getDouble(section, "readOp_lkg");
      }
      if(SescConf->checkDouble(section, "writeOp_dyn")) {
        dcache.xbar->local_result.power.writeOp.dynamic = SescConf->getDouble(section, "writeOp_dyn");
      }
      if(SescConf->checkDouble(section, "writeOp_lkg")) {
        dcache.xbar->local_result.power.writeOp.leakage = SescConf->getDouble(section, "writeOp_lkg");
      }
    } else {
      // There should be a Xbar for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n Xbar for the GPU must point to a valid section, cannot be \"None\" \n");
      exit(-1);
    }
  } else {
    // There should be a XBar for the GPU.
    I(0);
    // error;
    fprintf(stderr, "\n Xbar for the GPU not defined \n");
    exit(-1);
  }

} /*}}}*/

GPUU::GPUU(ParseXML *XML_interface, InputParameter *interface_ip_)
    : XML(XML_interface)
    , interface_ip(*interface_ip_) { /*{{{*/

  // clock codes by eka, just to see how much time each method takes
  interface_ip.freq = XML->sys.gpu.homoSM.homolane.clock_rate;

  set_GPU_param();

  // for(int i = 0; i<numSMs; i++)
  {
    sms.push_back(new SM(XML, 0, &interface_ip, coredynp));
    // if (coredynp.homoSM)
    {
      for(int j = 1; j < coredynp.numSMs; j++)
        sms.push_back(sms[0]);
      area.set_area(area.get_area() + sms[0]->area.get_area() * coredynp.numSMs);
    }
  }

  l2array = new SharedCache(XML, 0, &interface_ip, L2G);
  area.set_area(area.get_area() + l2array->area.get_area());

} /*}}}*/

void InstFetchUG::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    IB->stats_t.readAc.access = IB->stats_t.writeAc.access = 1;
    IB->tdp_stats                                          = IB->stats_t;

  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    IB->stats_t.readAc.access = IB->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].lanes[ithLane].total_instructions;
    IB->rtp_stats                                          = IB->stats_t;
  }
  IB->power_t.reset();
  // eka, may need to chech the first call
  // icache.power.reset();
  // IB->power.reset();

  IB->power_t.readOp.dynamic += IB->local_result.power.readOp.dynamic * IB->stats_t.readAc.access +
                                IB->stats_t.writeAc.access * IB->local_result.power.writeOp.dynamic;

  if(is_tdp) {

    IB->power = IB->power_t + IB->local_result.power * pppm_lkg;
    power     = power + IB->power;
  } else {
    IB->rt_power = IB->power_t + IB->local_result.power * pppm_lkg;
    rt_power     = rt_power + IB->rt_power;
  }
} /*}}}*/

void LoadStoreUG::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP

    LSQ->stats_t.readAc.access = LSQ->stats_t.writeAc.access = LSQ->l_ip.num_search_ports;
    LSQ->tdp_stats                                           = LSQ->stats_t;
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)

    LSQ->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].lanes[ithLane].LSQ.read_accesses;
    LSQ->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].lanes[ithLane].LSQ.write_accesses;
    LSQ->rtp_stats              = LSQ->stats_t;
  }

  LSQ->power_t.reset();
  // eka, may need to check the first call
  // dcache.power.reset();
  // LSQ->power.reset();
  LSQ->power_t.readOp.dynamic +=
      LSQ->stats_t.readAc.access * (LSQ->local_result.power.searchOp.dynamic + LSQ->local_result.power.readOp.dynamic) +
      LSQ->stats_t.writeAc.access *
          LSQ->local_result.power.writeOp.dynamic; // every memory access invloves at least two operations on LSQ
  if(is_tdp) {
    LSQ->power = LSQ->power_t + LSQ->local_result.power * pppm_lkg;
    power      = power + LSQ->power;

  } else {
    LSQ->rt_power = LSQ->power_t + LSQ->local_result.power * pppm_lkg;
    rt_power      = rt_power + LSQ->rt_power;
  }

} /*}}}*/

void RegFUG::computeEnergy(bool is_tdp) { /*{{{*/
  /*
   * Architecture RF and physical RF cannot be present at the same time.
   * Therefore, the RF stats can only refer to either ARF or PRF;
   * And the same stats can be used for both.
   */

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    IRF->stats_t.readAc.access  = IRF->l_ip.num_rd_ports;
    IRF->stats_t.writeAc.access = IRF->l_ip.num_wr_ports;
    IRF->tdp_stats              = IRF->stats_t;
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    IRF->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].lanes[ithLane].int_regfile_reads; // TODO: no diff on archi and phy
    IRF->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].lanes[ithLane].int_regfile_writes;
    IRF->rtp_stats              = IRF->stats_t;
  }

  IRF->power_t.reset();
  IRF->power_t.readOp.dynamic += (IRF->stats_t.readAc.access * IRF->local_result.power.readOp.dynamic +
                                  IRF->stats_t.writeAc.access * IRF->local_result.power.writeOp.dynamic);

  if(is_tdp) {
    IRF->power = IRF->power_t + IRF->local_result.power * pppm_lkg;
    power      = power + (IRF->power);
  } else {
    IRF->rt_power = IRF->power_t + IRF->local_result.power * pppm_lkg;
    rt_power      = rt_power + (IRF->rt_power);
  }
} /*}}}*/

void EXECUG::computeEnergy(bool is_tdp) { /*{{{*/
  double pppm_t[4] = {1, 1, 1, 1};

  rfu->computeEnergy(is_tdp);
  if(is_tdp) {
    // eka
    power.reset();
    bypass.power.reset();
    set_pppm(pppm_t, 2, 2, 2, 2);
    bypass.power = bypass.power + intTagBypass->power * pppm_t + int_bypass->power * pppm_t;
    power        = power /*+ rfu->power*/ + fpu->power + exeu->power + bypass.power;
  } else {
    // eka
    rt_power.reset();
    bypass.rt_power.reset();
    set_pppm(pppm_t, XML->sys.gpu.SMS[ithSM].lanes[ithLane].bypassbus_access, 2, 2, 2);
    bypass.rt_power               = bypass.rt_power + intTagBypass->power * pppm_t;
    bypass.rt_power               = bypass.rt_power + int_bypass->power * pppm_t;
    exeu->rt_power.readOp.dynamic = exeu->power.readOp.dynamic * XML->sys.gpu.SMS[ithSM].lanes[ithLane].int_instructions;
    fpu->rt_power.readOp.dynamic  = fpu->power.readOp.dynamic * XML->sys.gpu.SMS[ithSM].lanes[ithLane].fp_instructions;
    rt_power                      = rt_power /*+ rfu->rt_power*/ + fpu->rt_power + exeu->rt_power + bypass.rt_power;
  }
} /*}}}*/

void TCD::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    power.reset();
    tc_data->stats_t.readAc.access  = 0.67 * tc_data->l_ip.num_rw_ports;
    tc_data->stats_t.readAc.miss    = 0;
    tc_data->stats_t.readAc.hit     = tc_data->stats_t.readAc.access - tc_data->stats_t.readAc.miss;
    tc_data->stats_t.writeAc.access = 0.33 * tc_data->l_ip.num_rw_ports;
    tc_data->stats_t.writeAc.miss   = 0;
    tc_data->stats_t.writeAc.hit    = tc_data->stats_t.writeAc.access - tc_data->stats_t.writeAc.miss;
    tc_data->tdp_stats              = tc_data->stats_t;
  } else {
    rt_power.reset();
    tc_data->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_data.read_accesses;
    tc_data->stats_t.readAc.miss    = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_data.read_misses;
    tc_data->stats_t.readAc.hit     = tc_data->stats_t.readAc.access - tc_data->stats_t.readAc.miss;
    tc_data->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_data.write_accesses;
    tc_data->stats_t.writeAc.miss   = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_data.write_misses;
    tc_data->stats_t.writeAc.hit    = tc_data->stats_t.writeAc.access - tc_data->stats_t.writeAc.miss;
    tc_data->rtp_stats              = tc_data->stats_t;
  }

  tc_data->power_t.reset();
  tc_data->power_t.readOp.dynamic += (tc_data->stats_t.readAc.hit * tc_data->local_result.power.readOp.dynamic +
                                      tc_data->stats_t.readAc.miss * tc_data->local_result.tag_array2->power.readOp.dynamic +
                                      tc_data->stats_t.writeAc.miss * tc_data->local_result.tag_array2->power.writeOp.dynamic +
                                      tc_data->stats_t.writeAc.access * tc_data->local_result.power.writeOp.dynamic);

  if(is_tdp) {
    tc_data->power = tc_data->power_t + tc_data->local_result.power * pppm_lkg; // add lkg to the power
    power          = power + tc_data->power;
  } else {
    tc_data->rt_power = tc_data->power_t + tc_data->local_result.power * pppm_lkg; // add lkg to the power
    rt_power          = rt_power + tc_data->rt_power;
  }
} /*}}}*/

void TCI::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    power.reset();
    tc_inst->stats_t.readAc.access  = 0.67 * tc_inst->l_ip.num_rw_ports;
    tc_inst->stats_t.readAc.miss    = 0;
    tc_inst->stats_t.readAc.hit     = tc_inst->stats_t.readAc.access - tc_inst->stats_t.readAc.miss;
    tc_inst->stats_t.writeAc.access = 0.33 * tc_inst->l_ip.num_rw_ports;
    tc_inst->stats_t.writeAc.miss   = 0;
    tc_inst->stats_t.writeAc.hit    = tc_inst->stats_t.writeAc.access - tc_inst->stats_t.writeAc.miss;
    tc_inst->tdp_stats              = tc_inst->stats_t;
  } else {
    rt_power.reset();
    tc_inst->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_inst.read_accesses;
    tc_inst->stats_t.readAc.miss    = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_inst.read_misses;
    tc_inst->stats_t.readAc.hit     = tc_inst->stats_t.readAc.access - tc_inst->stats_t.readAc.miss;
    tc_inst->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_inst.write_accesses;
    tc_inst->stats_t.writeAc.miss   = XML->sys.gpu.SMS[ithSM].lanes[ithLane].tinycache_inst.write_misses;
    tc_inst->stats_t.writeAc.hit    = tc_inst->stats_t.writeAc.access - tc_inst->stats_t.writeAc.miss;
    tc_inst->rtp_stats              = tc_inst->stats_t;
  }

  tc_inst->power_t.reset();
  tc_inst->power_t.readOp.dynamic += (tc_inst->stats_t.readAc.hit * tc_inst->local_result.power.readOp.dynamic +
                                      tc_inst->stats_t.readAc.miss * tc_inst->local_result.tag_array2->power.readOp.dynamic +
                                      tc_inst->stats_t.writeAc.miss * tc_inst->local_result.tag_array2->power.writeOp.dynamic +
                                      tc_inst->stats_t.writeAc.access * tc_inst->local_result.power.writeOp.dynamic);

  if(is_tdp) {
    tc_inst->power = tc_inst->power_t + tc_inst->local_result.power * pppm_lkg; // add lkg to the power
    power          = power + tc_inst->power;
  } else {
    tc_inst->rt_power = tc_inst->power_t + tc_inst->local_result.power * pppm_lkg; // add lkg to the power
    rt_power          = rt_power + tc_inst->rt_power;
  }

} /*}}}*/

void Lane::computeEnergy(bool is_tdp) { /*{{{*/
  // power_point_product_masks
  // double pppm_t[4]    = {1,1,1,1};

  if(is_tdp) {
    // eka
    power.reset();
    // ifu->computeEnergy(is_tdp);
    // lsu->computeEnergy(is_tdp);
    exu->computeEnergy(is_tdp);

    // pipeline
    // ifu->power = ifu->power;
    // lsu->power = lsu->power;
    exu->power = exu->power;

    power = exu->power
        //+ lsu->power
        //+ ifu->power
        //+ clockNetwork.total_power.readOp.dynamic
        ;

    if(gpu_tcd_present) {
      tcdata->computeEnergy(is_tdp);
      // power = power + tcdata->power;
    }

    if(gpu_tci_present) {
      tcinst->computeEnergy(is_tdp);
      // power = power + tcinst->power;
    }

  } else {
    // eka
    rt_power.reset();
    // ifu->computeEnergy(is_tdp);
    // lsu->computeEnergy(is_tdp);
    exu->computeEnergy(is_tdp);
    // pipeline
    // ifu->rt_power = ifu->rt_power
    // lsu->rt_power = lsu->rt_power
    exu->rt_power = exu->rt_power;

    rt_power = exu->rt_power
        //+ lsu->rt_power
        //+ ifu->rt_power
        //+ clockNetwork.total_power.readOp.dynamic
        //+ corepipe.power.readOp.dynamic
        //+ branchPredictor.maxDynamicPower
        ;

    if(gpu_tcd_present) {
      tcdata->computeEnergy(is_tdp);
      // rt_power = rt_power + tcdata->rt_power;
    }

    if(gpu_tci_present) {
      tcinst->computeEnergy(is_tdp);
      // rt_power = rt_power + tcinst->rt_power;
    }
  }
} /*}}}*/

void MemManUG_I::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    itlb->stats_t.readAc.access = itlb->l_ip.num_search_ports;
    itlb->stats_t.readAc.miss   = 0;
    itlb->stats_t.readAc.hit    = itlb->stats_t.readAc.access - itlb->stats_t.readAc.miss;
    itlb->tdp_stats             = itlb->stats_t;
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    itlb->stats_t.readAc.access = XML->sys.gpu.SMS[ithSM].itlb.total_accesses;
    itlb->stats_t.readAc.miss   = XML->sys.gpu.SMS[ithSM].itlb.total_misses;
    itlb->stats_t.readAc.hit    = itlb->stats_t.readAc.access - itlb->stats_t.readAc.miss;
    itlb->rtp_stats             = itlb->stats_t;
  }

  itlb->power_t.reset();

  // eka, may need to check the first call
  // itlb->power.reset();
  itlb->power_t.readOp.dynamic += itlb->stats_t.readAc.access * itlb->local_result.power.searchOp.dynamic +
                                  itlb->stats_t.readAc.miss * itlb->local_result.power.writeOp.dynamic;
  // FA spent most power in tag so use access not hit to cover the misses

  if(is_tdp) {
    itlb->power = itlb->power_t + itlb->local_result.power * pppm_lkg;
    power       = power + itlb->power;
  } else {
    itlb->rt_power = itlb->power_t + itlb->local_result.power * pppm_lkg;
    rt_power       = rt_power + itlb->rt_power;
  }

} /*}}}*/

void MemManUG_D::computeEnergy(bool is_tdp) { /*{{{*/

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    dtlb->stats_t.readAc.access = dtlb->l_ip.num_search_ports;
    dtlb->stats_t.readAc.miss   = 0;
    dtlb->stats_t.readAc.hit    = dtlb->stats_t.readAc.access - dtlb->stats_t.readAc.miss;
    dtlb->tdp_stats             = dtlb->stats_t;
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    dtlb->stats_t.readAc.access = XML->sys.gpu.SMS[ithSM].dtlb.total_accesses;
    dtlb->stats_t.readAc.miss   = XML->sys.gpu.SMS[ithSM].dtlb.total_misses;
    dtlb->stats_t.readAc.hit    = dtlb->stats_t.readAc.access - dtlb->stats_t.readAc.miss;
    dtlb->rtp_stats             = dtlb->stats_t;
  }

  dtlb->power_t.reset();
  // eka, may need to check the first call
  // dtlb->power.reset();

  // FA spent most power in tag so use access not hit t ocover the misses
  dtlb->power_t.readOp.dynamic += dtlb->stats_t.readAc.access * dtlb->local_result.power.searchOp.dynamic +
                                  dtlb->stats_t.readAc.miss * dtlb->local_result.power.writeOp.dynamic;

  if(is_tdp) {
    dtlb->power = dtlb->power_t + dtlb->local_result.power * pppm_lkg;
    power       = power + dtlb->power;
  } else {
    dtlb->rt_power = dtlb->power_t + dtlb->local_result.power * pppm_lkg;
    rt_power       = rt_power + dtlb->rt_power;
  }
} /*}}}*/

void SMSharedCache::computeEnergy(bool is_tdp) { /*{{{*/

  // power_point_product_masks

  // double pppm_t[4]    = {1/2.0,1/2.0,1/2.0,1/2.0};

  if(is_tdp) {
    // eka
    power.reset();
    scratchcache->stats_t.readAc.access  = 0.67 * scratchcache->l_ip.num_rw_ports;
    scratchcache->stats_t.readAc.miss    = 0;
    scratchcache->stats_t.readAc.hit     = scratchcache->stats_t.readAc.access - scratchcache->stats_t.readAc.miss;
    scratchcache->stats_t.writeAc.access = 0.33 * scratchcache->l_ip.num_rw_ports;
    scratchcache->stats_t.writeAc.miss   = 0;
    scratchcache->stats_t.writeAc.hit    = scratchcache->stats_t.writeAc.access - scratchcache->stats_t.writeAc.miss;
    scratchcache->tdp_stats              = scratchcache->stats_t;
  } else {
    // eka
    rt_power.reset();
    scratchcache->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].scratchpad.read_accesses;
    scratchcache->stats_t.readAc.miss    = 0;
    scratchcache->stats_t.readAc.hit     = scratchcache->stats_t.readAc.access - scratchcache->stats_t.readAc.miss;
    scratchcache->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].scratchpad.write_accesses;
    scratchcache->stats_t.writeAc.miss   = 0;
    scratchcache->stats_t.writeAc.hit    = scratchcache->stats_t.writeAc.access - scratchcache->stats_t.writeAc.miss;
    scratchcache->rtp_stats              = scratchcache->stats_t;
  }

  // scratchpad
  scratchcache->power_t.reset();
  scratchcache->power_t.readOp.dynamic += scratchcache->stats_t.readAc.hit * scratchcache->local_result.power.readOp.dynamic +
                                          scratchcache->stats_t.writeAc.hit * scratchcache->local_result.power.writeOp.dynamic;

  if(is_tdp) {
    scratchcache->power = scratchcache->power_t + scratchcache->local_result.power * pppm_lkg;
    power               = power + scratchcache->power;
  } else {
    scratchcache->rt_power = scratchcache->power_t + scratchcache->local_result.power * pppm_lkg;
    rt_power               = rt_power + scratchcache->rt_power;
  }
} /*}}}*/

void SM::computeEnergy(bool is_tdp) { /*{{{*/
  // power_point_product_masks

  // double pppm_t[4]    = {1/2.0,1/2.0,1/2.0,1/2.0};

  if(is_tdp) {
    // eka
    power.reset();
    icache.caches->stats_t.readAc.access = icache.caches->l_ip.num_rw_ports;
    icache.caches->stats_t.readAc.miss   = 0;
    icache.caches->stats_t.readAc.hit    = icache.caches->stats_t.readAc.access - icache.caches->stats_t.readAc.miss;
    icache.caches->tdp_stats             = icache.caches->stats_t;

    icache.prefetchb->stats_t.readAc.access = icache.prefetchb->stats_t.readAc.hit = 0; // icache.prefetchb->l_ip.num_search_ports;
    icache.prefetchb->stats_t.writeAc.access = icache.prefetchb->stats_t.writeAc.hit = 0; // icache.ifb->l_ip.num_search_ports;
    icache.prefetchb->tdp_stats                                                      = icache.prefetchb->stats_t;

    dcache.caches->stats_t.readAc.access  = 0.67 * dcache.caches->l_ip.num_rw_ports;
    dcache.caches->stats_t.readAc.miss    = 0;
    dcache.caches->stats_t.readAc.hit     = dcache.caches->stats_t.readAc.access - dcache.caches->stats_t.readAc.miss;
    dcache.caches->stats_t.writeAc.access = 0.33 * dcache.caches->l_ip.num_rw_ports;
    dcache.caches->stats_t.writeAc.miss   = 0;
    dcache.caches->stats_t.writeAc.hit    = dcache.caches->stats_t.writeAc.access - dcache.caches->stats_t.writeAc.miss;
    dcache.caches->tdp_stats              = dcache.caches->stats_t;

    dcache.missb->stats_t.readAc.access  = 0; // dcache.missb->l_ip.num_search_ports;
    dcache.missb->stats_t.writeAc.access = 0; // dcache.missb->l_ip.num_search_ports;
    dcache.missb->tdp_stats              = dcache.missb->stats_t;

    dcache.ifb->stats_t.readAc.access  = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.ifb->stats_t.writeAc.access = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.ifb->tdp_stats              = dcache.ifb->stats_t;

    dcache.prefetchb->stats_t.readAc.access  = 0; // dcache.prefetchb->l_ip.num_search_ports;
    dcache.prefetchb->stats_t.writeAc.access = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.prefetchb->tdp_stats              = dcache.prefetchb->stats_t;

    dcache.wbb->stats_t.readAc.access  = 0; // dcache.wbb->l_ip.num_search_ports;
    dcache.wbb->stats_t.writeAc.access = 0; // dcache.wbb->l_ip.num_search_ports;
    dcache.wbb->tdp_stats              = dcache.wbb->stats_t;

  } else {
    // eka
    rt_power.reset();
    icache.caches->stats_t.readAc.access = XML->sys.gpu.SMS[ithSM].icache.read_accesses;
    icache.caches->stats_t.readAc.miss   = XML->sys.gpu.SMS[ithSM].icache.read_misses;
    icache.caches->stats_t.readAc.hit    = icache.caches->stats_t.readAc.access - icache.caches->stats_t.readAc.miss;
    icache.caches->rtp_stats             = icache.caches->stats_t;

    icache.prefetchb->stats_t.readAc.access  = icache.caches->stats_t.readAc.miss;
    icache.prefetchb->stats_t.writeAc.access = icache.caches->stats_t.readAc.miss;
    icache.prefetchb->rtp_stats              = icache.prefetchb->stats_t;

    dcache.caches->stats_t.readAc.access  = XML->sys.gpu.SMS[ithSM].dcache.read_accesses;
    dcache.caches->stats_t.readAc.miss    = XML->sys.gpu.SMS[ithSM].dcache.read_misses;
    dcache.caches->stats_t.readAc.hit     = dcache.caches->stats_t.readAc.access - dcache.caches->stats_t.readAc.miss;
    dcache.caches->stats_t.writeAc.access = XML->sys.gpu.SMS[ithSM].dcache.write_accesses;
    dcache.caches->stats_t.writeAc.miss   = XML->sys.gpu.SMS[ithSM].dcache.write_misses;
    dcache.caches->stats_t.writeAc.hit    = dcache.caches->stats_t.writeAc.access - dcache.caches->stats_t.writeAc.miss;
    dcache.caches->rtp_stats              = dcache.caches->stats_t;

    // assuming write back policy for data cache TODO: add option for this in XML
    dcache.missb->stats_t.readAc.access  = dcache.caches->stats_t.writeAc.miss;
    dcache.missb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.missb->rtp_stats              = dcache.missb->stats_t;

    dcache.ifb->stats_t.readAc.access  = dcache.caches->stats_t.writeAc.miss;
    dcache.ifb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.ifb->rtp_stats              = dcache.ifb->stats_t;

    dcache.prefetchb->stats_t.readAc.access  = dcache.caches->stats_t.writeAc.miss;
    dcache.prefetchb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.prefetchb->rtp_stats              = dcache.prefetchb->stats_t;

    dcache.wbb->stats_t.readAc.access  = dcache.caches->stats_t.writeAc.miss;
    dcache.wbb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.wbb->rtp_stats              = dcache.wbb->stats_t;
  }

  icache.power_t.reset();
  dcache.power_t.reset();

  icache.power_t.readOp.dynamic +=
      (icache.caches->stats_t.readAc.hit * icache.caches->local_result.power.readOp.dynamic +
       icache.caches->stats_t.readAc.miss * icache.caches->local_result.tag_array2->power.readOp.dynamic +
       icache.caches->stats_t.readAc.miss * icache.caches->local_result.power.writeOp.dynamic +
       icache.caches->stats_t.readAc.hit *
           icache.bcast->local_result.power.readOp.dynamic); // read miss in Icache cause a write to Icache
  /*
     icache.power_t.readOp.dynamic	+=
     icache.prefetchb->stats_t.readAc.access   * icache.prefetchb->local_result.power.searchOp.dynamic +
     icache.prefetchb->stats_t.writeAc.access  * icache.prefetchb->local_result.power.writeOp.dynamic;
  */

  dcache.power_t.readOp.dynamic +=
      (dcache.caches->stats_t.readAc.hit * dcache.caches->local_result.power.readOp.dynamic +
       dcache.caches->stats_t.readAc.miss * dcache.caches->local_result.tag_array2->power.readOp.dynamic +
       dcache.caches->stats_t.writeAc.miss * dcache.caches->local_result.tag_array2->power.writeOp.dynamic +
       dcache.caches->stats_t.writeAc.access *
           dcache.caches->local_result.power.writeOp.dynamic); // write miss will generate a write later

  // cache reads and writes
  dcache.power_t.readOp.dynamic +=
      dcache.missb->stats_t.readAc.access * dcache.missb->local_result.power.searchOp.dynamic +
      dcache.missb->stats_t.writeAc.access *
          dcache.missb->local_result.power.writeOp.dynamic; // each access to missb involves a CAM and a write

  // cache ifb cost
  dcache.power_t.readOp.dynamic += dcache.ifb->stats_t.readAc.access * dcache.ifb->local_result.power.searchOp.dynamic +
                                   dcache.ifb->stats_t.writeAc.access * dcache.ifb->local_result.power.writeOp.dynamic;

  // cache prefetchb cost
  dcache.power_t.readOp.dynamic += dcache.prefetchb->stats_t.readAc.access * dcache.prefetchb->local_result.power.searchOp.dynamic +
                                   dcache.prefetchb->stats_t.writeAc.access * dcache.prefetchb->local_result.power.writeOp.dynamic;

  // cache wbb cost
  dcache.power_t.readOp.dynamic += dcache.wbb->stats_t.readAc.access * dcache.wbb->local_result.power.searchOp.dynamic +
                                   dcache.wbb->stats_t.writeAc.access * dcache.wbb->local_result.power.writeOp.dynamic;

  // xbar
  dcache.power_t.readOp.dynamic += (dcache.caches->stats_t.readAc.access + dcache.caches->stats_t.writeAc.access) *
                                   dcache.xbar->local_result.power.readOp.dynamic;

  if(is_tdp) {
    lane.power.reset();
    for(int i = 0; i < coredynp.numLanes; i++) {
      lanes[i]->update_rtparam(XML, ithSM, i);
      lanes[i]->computeEnergy(is_tdp);
      power      = power + lanes[i]->power;
      lane.power = lane.power + lanes[i]->power;
    }

    mmu_d->update_rtparam(XML, ithSM);
    mmu_i->update_rtparam(XML, ithSM);
    mmu_d->computeEnergy(is_tdp);
    mmu_i->computeEnergy(is_tdp);
    scratchpad->computeEnergy(is_tdp);

    power = power + mmu_d->power + mmu_i->power + scratchpad->power;

    icache.power = icache.power_t + (icache.caches->local_result.power) * pppm_lkg +
                   ( // icache.missb->local_result.power +
                     // icache.ifb->local_result.power +
                       icache.prefetchb->local_result.power) *
                       pppm_Isub;

    dcache.power = dcache.power_t +
                   (dcache.caches->local_result.power
                    /*+ scratchpad->local_result.power*/) *
                       pppm_lkg +
                   (dcache.missb->local_result.power + dcache.ifb->local_result.power + dcache.prefetchb->local_result.power +
                    dcache.wbb->local_result.power) *
                       pppm_Isub;

    power = power + icache.power + dcache.power;

  } else {

    lane.rt_power.reset();
    for(int i = 0; i < coredynp.numLanes; i++) {
      lanes[i]->update_rtparam(XML, ithSM, i);
      lanes[i]->computeEnergy(is_tdp);
      rt_power      = rt_power + lanes[i]->rt_power;
      lane.rt_power = lane.rt_power + lanes[i]->rt_power;
    }

    mmu_d->update_rtparam(XML, ithSM);
    mmu_d->computeEnergy(is_tdp);

    mmu_i->update_rtparam(XML, ithSM);
    mmu_i->computeEnergy(is_tdp);

    scratchpad->update_rtparam(XML, ithSM);
    scratchpad->computeEnergy(is_tdp);

    rt_power = rt_power + mmu_i->rt_power + mmu_d->rt_power + scratchpad->rt_power;

    icache.rt_power = icache.power_t + (icache.caches->local_result.power) * pppm_lkg +
                      ( // icache.missb->local_result.power +
                        // icache.ifb->local_result.power +
                          icache.prefetchb->local_result.power) *
                          pppm_Isub;
    dcache.rt_power =
        dcache.power_t + (dcache.caches->local_result.power + dcache.missb->local_result.power + dcache.ifb->local_result.power +
                          dcache.prefetchb->local_result.power + dcache.wbb->local_result.power + dcache.xbar->local_result.power) *
                             pppm_lkg;

    rt_power = rt_power + icache.rt_power + dcache.rt_power;
  }
} /*}}}*/

void GPUU::computeEnergy(bool is_tdp) { /*{{{*/
  // power_point_product_masks

  executionTime = XML->sys.executionTime;

  if(is_tdp) {
    // eka
    power.reset();

    for(int i = 0; i < coredynp.numSMs; i++) {
      sms[i]->update_rtparam(XML, i);
      sms[i]->computeEnergy(is_tdp);
      power = power + sms[i]->power;
    }

    l2array->computeEnergy(is_tdp);
    power = power + l2array->power;
  } else {
    // eka

    rt_power.reset();

    for(int i = 0; i < coredynp.numSMs; i++) {
      sms[i]->update_rtparam(XML, i);
      sms[i]->computeEnergy(is_tdp);
      rt_power = rt_power + sms[i]->rt_power;
    }

    l2array->computeEnergy(is_tdp);
    rt_power = rt_power + l2array->power;
  }

} /*}}}*/

InstFetchUG ::~InstFetchUG() { /*{{{*/
  if(IB) {
    delete IB;
    IB = 0;
  }
} /*}}}*/

LoadStoreUG ::~LoadStoreUG() { /*{{{*/
  if(LSQ) {
    delete LSQ;
    LSQ = 0;
  }
} /*}}}*/

RegFUG ::~RegFUG() { /*{{{*/
  if(IRF) {
    delete IRF;
    IRF = 0;
  }
} /*}}}*/

MemManUG_I ::~MemManUG_I() { /*{{{*/
  if(itlb) {
    delete itlb;
    itlb = 0;
  }
} /*}}}*/

MemManUG_D ::~MemManUG_D() { /*{{{*/
  if(dtlb) {
    delete dtlb;
    dtlb = 0;
  }
} /*}}}*/

SMSharedCache::~SMSharedCache() { /*{{{*/
} /*}}}*/

EXECUG ::~EXECUG() { /*{{{*/
  if(int_bypass) {
    delete int_bypass;
    int_bypass = 0;
  }
  if(intTagBypass) {
    delete intTagBypass;
    intTagBypass = 0;
  }
  if(fpu) {
    delete fpu;
    fpu = 0;
  }
  if(exeu) {
    delete exeu;
    exeu = 0;
  }
  if(rfu) {
    delete rfu;
    rfu = 0;
  }
} /*}}}*/

SM ::~SM() { /*{{{*/

} /*}}}*/

void Lane::set_Lane_param() { /*{{{*/
  // set pipedynp from coredynp
} /*}}}*/

void SM::set_SM_param() { /*{{{*/
  coredynp.numLanes = XML->sys.gpu.homoSM.number_of_lanes;
} /*}}}*/

void GPUU::set_GPU_param() { /*{{{*/
  coredynp.numSMs  = XML->sys.gpu.number_of_SMs;
  clockRate        = XML->sys.gpu.homoSM.homolane.clock_rate;
  executionTime    = XML->sys.executionTime;
  coredynp.core_ty = Inorder;

  coredynp.fetchW = XML->sys.gpu.homoSM.homolane.fetch_width;

  coredynp.num_hthreads       = XML->sys.gpu.homoSM.homolane.number_hardware_threads;
  coredynp.multithreaded      = coredynp.num_hthreads > 1 ? true : false;
  coredynp.instruction_length = XML->sys.gpu.homoSM.homolane.instruction_length;
  coredynp.pc_width           = XML->sys.virtual_address_width;
  coredynp.opcode_length      = XML->sys.gpu.homoSM.homolane.opcode_width;
  coredynp.pipeline_stages    = XML->sys.gpu.homoSM.homolane.pipeline_depth[0];
  coredynp.int_data_width     = int(ceil(XML->sys.machine_bits / 32.0)) * 32;
  coredynp.fp_data_width      = coredynp.int_data_width;
  coredynp.v_address_width    = XML->sys.virtual_address_width;
  coredynp.p_address_width    = XML->sys.physical_address_width;

  coredynp.arch_ireg_width = int(ceil(log2(XML->sys.gpu.homoSM.homolane.archi_Regs_IRF_size)));
  coredynp.num_IRF_entry   = XML->sys.gpu.homoSM.homolane.phy_Regs_IRF_size;

  if(!((coredynp.core_ty == OOO) || (coredynp.core_ty == Inorder))) {
    cout << "Invalid Core Type" << endl;
    exit(0);
  }

  coredynp.globalCheckpoint   = 32; // best check pointing entries for a 4~8 issue OOO should be 16~48;See TR for reference.
  coredynp.perThreadState     = 8;
  coredynp.instruction_length = 32;
  coredynp.clockRate          = XML->sys.gpu.homoSM.homolane.clock_rate;
  coredynp.clockRate *= 1e6;
  coredynp.executionTime = XML->sys.executionTime;
} /*}}}*/
