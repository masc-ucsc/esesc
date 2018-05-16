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
#include "memoryctrl.h"
#include "XML_Parse.h"
#include "basic_circuit.h"
#include "basic_components.h"
#include "const.h"
#include "io.h"
#include "logic.h"
#include "parameter.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>

MCBackend::MCBackend(ParseXML *XML_interface, InputParameter *interface_ip_, const MCParam &mcp_)
    : XML(XML_interface)
    , l_ip(*interface_ip_)
    , mcp(mcp_) {

  local_result = init_interface(&l_ip);
  compute();
}

void MCBackend::compute() {
  // double max_row_addr_width = 20.0;//Current address 12~18bits
  double C_MCB, mc_power; /*refresh_period,refresh_freq*/
  ;                       // Equivalent per bit Cap for backend,
  double pmos_to_nmos_sizing_r = pmos_to_nmos_sz_ratio();
  // area = (2.2927*log(peakDataTransferRate)-14.504)*memDataWidth/144.0*(l_ip.F_sz_um/0.09);
  area.set_area((2.7927 * log(mcp.peakDataTransferRate) - 19.862) / 2.0 * mcp.dataBusWidth / 128.0 * (l_ip.F_sz_um / 0.09) *
                mcp.num_channels * 1e6); // um^2
  // assuming the approximately same scaling factor as seen in processors.//TODO merge this into undifferentiated core
  // C_MCB=0.2/1.3/1.3/266/64/0.09*g_ip.F_sz_um;//based on AMD Geode processor which has a very basic mc on chip.
  // C_MCB = 1.6/200/1e6/144/1.2/1.2*g_ip.F_sz_um/0.19;//Based on Niagara power numbers.The base power (W) is divided by device
  // frequency and vdd and scale to target process.
  mc_power = 0.0291; // 29.1mW@200MHz @130nm From Power Analysis of SystemLevel OnChip Communication Architectures by Lahiri et
  C_MCB    = mc_power / 20e6 / 32 / 1.3 / 1.3 * l_ip.F_sz_um /
          0.13; // Normalize from .13 um process where vdd=1.3. 10% memory traffic to the peak bw
                //  refresh_period = 0.064;//64ms
                // refresh_freq =1.0/(refresh_period/pow(2.0,max_row_addr_width));
  // routinePower.readOp.dynamic  =
  // C_MCB*refresh_freq*g_tp.peri_global.Vdd*g_tp.peri_global.Vdd*memRank*(memDataWidth+memDataWidth/num_bits_per_ecc_b_);//this is
  // the routine power for routine refresh; power.readOp.dynamic +=
  // C_MCB*memAccesses*memDataWidth*llcBlocksize*8/memDataWidth*g_tp.peri_global.Vdd*g_tp.peri_global.Vdd;
  power.readOp.dynamic += C_MCB * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd * (mcp.dataBusWidth + mcp.addressBusWidth) *
                          (1 + 1 / num_bits_per_ecc_b_); // per access energy in memory controller
  power.readOp.leakage = area.get_area() * (g_tp.scaling_factor.core_tx_density / 10) *
                         cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                         g_tp.peri_global.Vdd / 2; // unit W
  power.readOp.gate_leakage = area.get_area() * (g_tp.scaling_factor.core_tx_density / 10) *
                              cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                              g_tp.peri_global.Vdd / 2; // unit W
  power_t = power;
}

void MCBackend::computeEnergy(bool is_tdp) {
  if(is_tdp) {
    // init stats for TDP
    stats_t.readAc.access  = 0.5 * mcp.num_channels;
    stats_t.writeAc.access = 0.5 * mcp.num_channels;
    tdp_stats              = stats_t;
  } else {
    // init stats for runtime power (RTP)
    stats_t.readAc.access  = XML->sys.mc.memory_reads;
    stats_t.writeAc.access = XML->sys.mc.memory_writes;
    tdp_stats              = stats_t;
  }
  if(is_tdp) {
    // eka
    // power.reset();
    power.readOp.dynamic = (stats_t.readAc.access * power_t.readOp.dynamic + stats_t.writeAc.access * power_t.readOp.dynamic);

  } else {
    // eka
    // rt_power.reset();
    rt_power.readOp.dynamic = (stats_t.readAc.access * power_t.readOp.dynamic + stats_t.writeAc.access * power_t.readOp.dynamic);
    rt_power                = rt_power + power_t * pppm_lkg;
  }
}

MCPHY::MCPHY(ParseXML *XML_interface, InputParameter *interface_ip_, const MCParam &mcp_)
    : XML(XML_interface)
    , l_ip(*interface_ip_)
    , mcp(mcp_) {

  local_result = init_interface(&l_ip);
  compute();
}

void MCPHY::compute() {
  double pmos_to_nmos_sizing_r = pmos_to_nmos_sz_ratio();
  // Based on die photos from Niagara 1 and 2.
  // TODO merge this into undifferentiated core.PHY only achieves square root of the ideal scaling.
  // area = (6.4323*log(peakDataTransferRate)-34.76)*memDataWidth/128.0*(l_ip.F_sz_um/0.09);
  area.set_area((6.4323 * log(mcp.peakDataTransferRate) - 48.134) * mcp.dataBusWidth / 128.0 * (l_ip.F_sz_um / 0.09) *
                mcp.num_channels * 1e6); // TODO:/2
  // This is from curve fitting based on Niagara 1 and 2's PHY die photo.
  // power.readOp.dynamic = 0.02*memAccesses*llcBlocksize*8;//change from Bytes to bits.
  power.readOp.dynamic = 0.002 * 1e-9; // This is power not energy, 20mw/Gb/s for each channel;
  power.readOp.leakage = area.get_area() * (g_tp.scaling_factor.core_tx_density / 10) *
                         cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                         g_tp.peri_global.Vdd / 2; // unit W
  power.readOp.gate_leakage = area.get_area() * (g_tp.scaling_factor.core_tx_density / 10) *
                              cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                              g_tp.peri_global.Vdd / 2; // unit W
  power_t = power;
}

void MCPHY::computeEnergy(bool is_tdp) {
  if(is_tdp) {
    // init stats for TDP
    stats_t.readAc.access  = 0.5 * mcp.num_channels;
    stats_t.writeAc.access = 0.5 * mcp.num_channels;
    tdp_stats              = stats_t;
  } else {
    // init stats for runtime power (RTP)
    stats_t.readAc.access  = XML->sys.mc.memory_reads;
    stats_t.writeAc.access = XML->sys.mc.memory_writes;
    tdp_stats              = stats_t;
  }

  if(is_tdp) {
    // eka
    // power.reset();
    // power_t.reset();
    power.readOp.dynamic = (stats_t.readAc.access * power_t.readOp.dynamic + stats_t.writeAc.access * power_t.readOp.dynamic);

  } else {
    // eka
    // rt_power.reset();
    // power_t.reset();
    power.readOp.dynamic = (stats_t.readAc.access * power_t.readOp.dynamic + stats_t.writeAc.access * power_t.readOp.dynamic);
    rt_power             = power;
  }
}

MCFrontEnd::MCFrontEnd(ParseXML *XML_interface, InputParameter *interface_ip_, const MCParam &mcp_)
    : XML(XML_interface)
    , interface_ip(*interface_ip_)
    , mcp(mcp_)
    , MC_arb(0)
    , frontendBuffer(0)
    , readBuffer(0)
    , writeBuffer(0) {
  /* All computations are for a single MC
   *
   */

  int  tag, data;
  bool is_default = true; // indication for default setup

  /* MC frontend engine channels share the same engines but logically partitioned
   * For all hardware inside MC. different channels do not share resources.
   * TODO: add docodeing/mux stage to steer memory requests to different channels.
   */

  // memory request reorder buffer
  tag                              = mcp.addressBusWidth + EXTRA_TAG_BITS + mcp.opcodeW;
  data                             = int(ceil((XML->sys.physical_address_width + mcp.opcodeW) / 8.0));
  interface_ip.cache_sz            = data * XML->sys.mc.req_window_size_per_channel;
  interface_ip.line_sz             = data;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = 1.0 / mcp.clockRate;
  interface_ip.latency             = 1.0 / mcp.clockRate;
  interface_ip.is_cache            = true;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = false;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = XML->sys.mc.memory_channels_per_mc;
  interface_ip.num_wr_ports        = interface_ip.num_rd_ports;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = XML->sys.mc.memory_channels_per_mc;
  frontendBuffer                   = new ArrayST(&interface_ip, "MC ReorderBuffer");
  frontendBuffer->area.set_area(frontendBuffer->area.get_area() +
                                frontendBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);
  area.set_area(area.get_area() + frontendBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);

  // selection logic
  MC_arb = new selection_logic(is_default, XML->sys.mc.req_window_size_per_channel, 1, &interface_ip);

  // read buffers.
  data                             = mcp.dataBusWidth / 8; // Support key words first operation //8 means converting bit to Byte
  interface_ip.cache_sz            = data * XML->sys.mc.IO_buffer_size_per_channel; //*llcBlockSize;
  interface_ip.line_sz             = data;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / mcp.clockRate;
  interface_ip.latency             = 1.0 / mcp.clockRate;
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0; // XML->sys.mc.memory_channels_per_mc*2>2?2:XML->sys.mc.memory_channels_per_mc*2;
  interface_ip.num_rd_ports        = XML->sys.mc.memory_channels_per_mc;
  interface_ip.num_wr_ports        = interface_ip.num_rd_ports;
  interface_ip.num_se_rd_ports     = 0;
  readBuffer                       = new ArrayST(&interface_ip, "MC ReadBuffer");
  readBuffer->area.set_area(readBuffer->area.get_area() + readBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);
  area.set_area(area.get_area() + readBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);

  // write buffer
  data                             = mcp.dataBusWidth / 8; // Support key words first operation //8 means converting bit to Byte
  interface_ip.cache_sz            = data * XML->sys.mc.IO_buffer_size_per_channel; //*llcBlockSize;
  interface_ip.line_sz             = data;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 0;
  interface_ip.throughput          = 1.0 / mcp.clockRate;
  interface_ip.latency             = 1.0 / mcp.clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = XML->sys.mc.memory_channels_per_mc;
  interface_ip.num_wr_ports        = interface_ip.num_rd_ports;
  interface_ip.num_se_rd_ports     = 0;
  writeBuffer                      = new ArrayST(&interface_ip, "MC writeBuffer");
  writeBuffer->area.set_area(writeBuffer->area.get_area() + writeBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);
  area.set_area(area.get_area() + writeBuffer->local_result.area * XML->sys.mc.memory_channels_per_mc);
}

void MCFrontEnd::computeEnergy(bool is_tdp) {
  if(is_tdp) {
    // init stats for TDP
    frontendBuffer->stats_t.readAc.access  = frontendBuffer->l_ip.num_search_ports;
    frontendBuffer->stats_t.writeAc.access = frontendBuffer->l_ip.num_wr_ports;
    frontendBuffer->tdp_stats              = frontendBuffer->stats_t;

    readBuffer->stats_t.readAc.access  = readBuffer->l_ip.num_rd_ports;
    readBuffer->stats_t.writeAc.access = readBuffer->l_ip.num_wr_ports;
    readBuffer->tdp_stats              = readBuffer->stats_t;

    writeBuffer->stats_t.readAc.access  = writeBuffer->l_ip.num_rd_ports;
    writeBuffer->stats_t.writeAc.access = writeBuffer->l_ip.num_wr_ports;
    writeBuffer->tdp_stats              = writeBuffer->stats_t;

  } else {
    // init stats for runtime power (RTP)
    frontendBuffer->stats_t.readAc.access  = XML->sys.mc.memory_reads;
    frontendBuffer->stats_t.writeAc.access = XML->sys.mc.memory_writes;
    frontendBuffer->tdp_stats              = frontendBuffer->stats_t;

    readBuffer->stats_t.readAc.access  = XML->sys.mc.memory_reads;
    readBuffer->stats_t.writeAc.access = XML->sys.mc.memory_reads;
    readBuffer->tdp_stats              = readBuffer->stats_t;

    writeBuffer->stats_t.readAc.access  = XML->sys.mc.memory_writes;
    writeBuffer->stats_t.writeAc.access = XML->sys.mc.memory_writes;
    writeBuffer->tdp_stats              = writeBuffer->stats_t;
  }

  frontendBuffer->power_t.reset();
  readBuffer->power_t.reset();
  writeBuffer->power_t.reset();
  frontendBuffer->power_t.readOp.dynamic +=
      (frontendBuffer->stats_t.readAc.access *
           (frontendBuffer->local_result.power.searchOp.dynamic + frontendBuffer->local_result.power.readOp.dynamic) +
       frontendBuffer->stats_t.writeAc.access * frontendBuffer->local_result.power.writeOp.dynamic);
  readBuffer->power_t.readOp.dynamic += (readBuffer->stats_t.readAc.access * readBuffer->local_result.power.readOp.dynamic +
                                         readBuffer->stats_t.writeAc.access * readBuffer->local_result.power.writeOp.dynamic);
  writeBuffer->power_t.readOp.dynamic += (writeBuffer->stats_t.readAc.access * writeBuffer->local_result.power.readOp.dynamic +
                                          writeBuffer->stats_t.writeAc.access * writeBuffer->local_result.power.writeOp.dynamic);

  if(is_tdp) {
    // eka
    power.reset();
    power = power + frontendBuffer->power_t + readBuffer->power_t + writeBuffer->power_t +
            (frontendBuffer->local_result.power + readBuffer->local_result.power + writeBuffer->local_result.power) * pppm_lkg;

  } else {
    // eka
    rt_power.reset();
    rt_power = rt_power + frontendBuffer->power_t + readBuffer->power_t + writeBuffer->power_t +
               (frontendBuffer->local_result.power + readBuffer->local_result.power + writeBuffer->local_result.power) * pppm_lkg;
  }
}

void MCFrontEnd::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << indent_str << "Front End ROB:" << endl;
    cout << indent_str_next << "Area = " << frontendBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << frontendBuffer->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << frontendBuffer->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << frontendBuffer->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << frontendBuffer->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;

    cout << endl;
    cout << indent_str << "Read Buffer:" << endl;
    cout << indent_str_next << "Area = " << readBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << readBuffer->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << readBuffer->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << readBuffer->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << readBuffer->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Write Buffer:" << endl;
    cout << indent_str_next << "Area = " << writeBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << writeBuffer->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << writeBuffer->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << writeBuffer->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << writeBuffer->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
  } else {
    cout << indent_str << "Front End ROB:" << endl;
    cout << indent_str_next << "Area = " << frontendBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << frontendBuffer->rt_power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << frontendBuffer->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << frontendBuffer->rt_power.readOp.gate_leakage << " W" << endl;
    cout << endl;
    cout << indent_str << "Read Buffer:" << endl;
    cout << indent_str_next << "Area = " << readBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << readBuffer->rt_power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << readBuffer->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << readBuffer->rt_power.readOp.gate_leakage << " W" << endl;
    cout << endl;
    cout << indent_str << "Write Buffer:" << endl;
    cout << indent_str_next << "Area = " << writeBuffer->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << writeBuffer->rt_power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << writeBuffer->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << writeBuffer->rt_power.readOp.gate_leakage << " W" << endl;
  }
}

MemoryController::MemoryController(ParseXML *XML_interface, InputParameter *interface_ip_)
    : XML(XML_interface)
    , interface_ip(*interface_ip_)
    , frontend(0)
    , transecEngine(0)
    , PHY(0)
    , pipeLogic(0)
    , maxDynPower("maxpwr_mc") {
  /* All computations are for a single MC
   *
   */
  set_mc_param();
  frontend = new MCFrontEnd(XML, &interface_ip, mcp);
  area.set_area(area.get_area() + frontend->area.get_area());
  transecEngine = new MCBackend(XML, &interface_ip, mcp);
  area.set_area(area.get_area() + transecEngine->area.get_area());
  PHY = new MCPHY(XML, &interface_ip, mcp);
  area.set_area(area.get_area() + PHY->area.get_area());
  maxDynPower.sample(power.readOp.dynamic * mcp.clockRate, true);
  //+++++++++Transaction engine +++++++++++++++++ ////TODO needs better numbers, Run the RTL code from OpenSparc.
  //  transecEngine.initialize(&interface_ip);
  //  transecEngine.peakDataTransferRate = XML->sys.mem.peak_transfer_rate;
  //  transecEngine.memDataWidth = dataBusWidth;
  //  transecEngine.memRank = XML->sys.mem.number_ranks;
  //  //transecEngine.memAccesses=XML->sys.mc.memory_accesses;
  //  //transecEngine.llcBlocksize=llcBlockSize;
  //  transecEngine.compute();
  //  transecEngine.area.set_area(XML->sys.mc.memory_channels_per_mc*transecEngine.area.get_area()) ;
  //  area.set_area(area.get_area()+ transecEngine.area.get_area());
  //  ///cout<<"area="<<area<<endl;
  ////
  //  //++++++++++++++PHY ++++++++++++++++++++++++++ //TODO needs better numbers
  //  PHY.initialize(&interface_ip);
  //  PHY.peakDataTransferRate = XML->sys.mem.peak_transfer_rate;
  //  PHY.memDataWidth = dataBusWidth;
  //  //PHY.memAccesses=PHY.peakDataTransferRate;//this is the max power
  //  //PHY.llcBlocksize=llcBlockSize;
  //  PHY.compute();
  //  PHY.area.set_area(XML->sys.mc.memory_channels_per_mc*PHY.area.get_area()) ;
  //  area.set_area(area.get_area()+ PHY.area.get_area());
  /// cout<<"area="<<area<<endl;
  //
  //  interface_ip.pipeline_stages = 5;//normal memory controller has five stages in the pipeline.
  //  interface_ip.per_stage_vector = addressBusWidth + XML->sys.core[0].opcode_width + dataBusWidth;
  //  pipeLogic = new pipeline(is_default, &interface_ip);
  //  //pipeLogic.init_pipeline(is_default, &interface_ip);
  //  pipeLogic->compute_pipeline();
  //  area.set_area(area.get_area()+ pipeLogic->area.get_area()*1e-6);
  //  area.set_area((area.get_area()+mc_area*1e-6)*1.1);//placement and routing overhead
  //
  //
  ////  //clock
  ////  clockNetwork.init_wire_external(is_default, &interface_ip);
  ////  clockNetwork.clk_area           =area*1.1;//10% of placement overhead. rule of thumb
  ////  clockNetwork.end_wiring_level   =5;//toplevel metal
  ////  clockNetwork.start_wiring_level =5;//toplevel metal
  ////  clockNetwork.num_regs           = pipeLogic.tot_stage_vector;
  ////  clockNetwork.optimize_wire();
}
void MemoryController::computeEnergy(bool is_tdp) {

  mcp.executionTime = XML->sys.executionTime;

  frontend->computeEnergy(is_tdp);
  transecEngine->computeEnergy(is_tdp);
  PHY->computeEnergy(is_tdp);
  if(is_tdp) {
    // eka
    power.reset();
    power = power + frontend->power + transecEngine->power + PHY->power;
  } else {
    // eka
    rt_power.reset();
    rt_power = rt_power + frontend->rt_power + transecEngine->rt_power + PHY->rt_power;
  }
}

void MemoryController::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << "Memory Controller:" << endl;
    cout << indent_str << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str << "TDP Dynamic = " << power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << indent_str << "Runtime Dynamic = " << rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Front End Engine:" << endl;
    cout << indent_str_next << "Area = " << frontend->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << frontend->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << frontend->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << frontend->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << frontend->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      frontend->displayEnergy(indent + 4, is_tdp);
    }
    cout << indent_str << "Transaction Engine:" << endl;
    cout << indent_str_next << "Area = " << transecEngine->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << transecEngine->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << transecEngine->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << transecEngine->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << transecEngine->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "PHY:" << endl;
    cout << indent_str_next << "Area = " << PHY->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << PHY->power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << PHY->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << PHY->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << PHY->rt_power.readOp.dynamic / mcp.executionTime << " W" << endl;
    cout << endl;
  } else {
    cout << "Memory Controller:" << endl;
    cout << indent_str_next << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << power.readOp.dynamic * mcp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << endl;
  }
}

void MemoryController::set_mc_param() {
  mcp.clockRate = XML->sys.mc.mc_clock * 2; // DDR
  mcp.clockRate *= 1e6;
  mcp.executionTime = XML->sys.executionTime;

  mcp.llcBlockSize    = int(ceil(XML->sys.mc.llc_line_length / 8.0)) + XML->sys.mc.llc_line_length; // ecc overhead
  mcp.dataBusWidth    = int(ceil(XML->sys.mc.databus_width / 8)) + XML->sys.mc.databus_width;
  mcp.addressBusWidth = int(ceil(XML->sys.mc.addressbus_width)); // XML->sys.physical_address_width;
  mcp.opcodeW         = 16;
  mcp.num_channels    = XML->sys.mc.memory_channels_per_mc;
  mcp.reads           = XML->sys.mc.memory_reads;
  mcp.writes          = XML->sys.mc.memory_writes;
  //+++++++++Transaction engine +++++++++++++++++ ////TODO needs better numbers, Run the RTL code from OpenSparc.
  mcp.peakDataTransferRate = XML->sys.mem.peak_transfer_rate;
  mcp.memRank              = XML->sys.mem.number_ranks;
  //++++++++++++++PHY ++++++++++++++++++++++++++ //TODO needs better numbers
  // PHY.memAccesses=PHY.peakDataTransferRate;//this is the max power
  // PHY.llcBlocksize=llcBlockSize;
}

// eka, update runtime parameters
void MemoryController::update_rtparam(ParseXML *XML_interface, int ithCache_, InputParameter *interface_ip_,
                                      MemoryController *mmu) {
  XML = XML_interface;
  // ithCache     = ithCache_;
  // interface_ip = interface_ip_
  mmu->frontend->XML = XML;
}

MCFrontEnd ::~MCFrontEnd() {

  if(MC_arb) {
    delete MC_arb;
    MC_arb = 0;
  }
  if(frontendBuffer) {
    delete frontendBuffer;
    frontendBuffer = 0;
  }
  if(readBuffer) {
    delete readBuffer;
    readBuffer = 0;
  }
  if(writeBuffer) {
    delete writeBuffer;
    writeBuffer = 0;
  }
}

MemoryController ::~MemoryController() {

  if(frontend) {
    delete frontend;
    frontend = 0;
  }
  if(transecEngine) {
    delete transecEngine;
    transecEngine = 0;
  }
  if(PHY) {
    delete PHY;
    PHY = 0;
  }
  if(pipeLogic) {
    delete pipeLogic;
    pipeLogic = 0;
  }
}
