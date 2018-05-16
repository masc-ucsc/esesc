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

#include "noc.h"
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

NoC::NoC(ParseXML *XML_interface, int ithNoC_, InputParameter *interface_ip_)
    : XML(XML_interface)
    , ithNoC(ithNoC_)
    , interface_ip(*interface_ip_)
    , router(0) {
  /*
   * initialize, compute and optimize individual components.
   */

  set_noc_param();
  local_result = init_interface(&interface_ip);
  scktRatio    = g_tp.sckt_co_eff;
  router       = new Router(nocdynp.flit_size, nocdynp.virtual_channel_per_port * nocdynp.input_buffer_entries_per_vc,
                      nocdynp.virtual_channel_per_port, &(g_tp.peri_global), nocdynp.input_ports, nocdynp.output_ports);
  // router->print_router();
  area.set_area(area.get_area() + router->area.get_area() * nocdynp.total_nodes);

  //  //clock power
  //  clockNetwork.init_wire_external(is_default, &interface_ip);
  //  clockNetwork.clk_area           =area*1.1;//10% of placement overhead. rule of thumb
  //  clockNetwork.end_wiring_level   =5;//toplevel metal
  //  clockNetwork.start_wiring_level =5;//toplevel metal
  //  clockNetwork.num_regs           = corepipe.tot_stage_vector;
  //  clockNetwork.optimize_wire();
}

void NoC::computeEnergy(bool is_tdp) {
  // power_point_product_masks
  double pppm_t[4] = {1, 1, 1, 1};
  executionTime    = XML->sys.executionTime;
  if(is_tdp) {
    power.reset();
    // init stats for TDP
    stats_t.readAc.access = 1;
    tdp_stats             = stats_t;
    set_pppm(pppm_t, nocdynp.total_nodes, nocdynp.total_nodes, nocdynp.total_nodes, nocdynp.total_nodes);
    power = power + router->power * pppm_t;
  } else {
    rt_power.reset();
    router->buffer.rt_power.reset();
    router->crossbar.rt_power.reset();
    router->arbiter.rt_power.reset();
    router->rt_power.reset();
    // init stats for runtime power (RTP)
    stats_t.readAc.access = XML->sys.NoC[ithNoC].total_accesses;
    rtp_stats             = stats_t;

    router->buffer.rt_power.readOp.dynamic =
        (router->buffer.power.readOp.dynamic + router->buffer.power.writeOp.dynamic) * rtp_stats.readAc.access;
    router->crossbar.rt_power.readOp.dynamic = router->crossbar.power.readOp.dynamic * rtp_stats.readAc.access;
    router->arbiter.rt_power.readOp.dynamic  = router->arbiter.power.readOp.dynamic * rtp_stats.readAc.access;
    set_pppm(pppm_t, 1, 0, 0, 0);
    router->rt_power = router->rt_power +
                       (router->buffer.rt_power + router->crossbar.rt_power + router->arbiter.rt_power) * pppm_t +
                       router->power * pppm_lkg; // TDP power must be calculated first!
    rt_power = rt_power + router->rt_power;
  }
}

void NoC::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');
  double M = .6;
  if(is_tdp) {
    cout << "NoC " << endl;
    cout << indent_str << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str << "TDP Dynamic = " << power.readOp.dynamic * nocdynp.clockRate << " W" << endl;
    cout << indent_str << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << indent_str << "Runtime Dynamic = " << rt_power.readOp.dynamic / nocdynp.executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Router: " << endl;
    cout << indent_str_next << "Area = " << router->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << router->power.readOp.dynamic * nocdynp.clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << router->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << router->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << router->rt_power.readOp.dynamic / nocdynp.executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      cout << indent_str << indent_str << "Virtual Channel Buffer:" << endl;
      cout << indent_str << indent_str_next << "Area = " << router->buffer.area.get_area() * 1e-6 * nocdynp.input_ports << " mm^2"
           << endl;
      cout << indent_str << indent_str_next << "TDP Dynamic = "
           << (router->buffer.power.readOp.dynamic + router->buffer.power.writeOp.dynamic) * nocdynp.input_ports * M *
                  nocdynp.clockRate
           << " W" << endl;
      cout << indent_str << indent_str_next
           << "Subthreshold Leakage = " << router->buffer.power.readOp.leakage * nocdynp.input_ports << " W" << endl;
      cout << indent_str << indent_str_next << "Gate Leakage = " << router->buffer.power.readOp.gate_leakage * nocdynp.input_ports
           << " W" << endl;
      cout << indent_str << indent_str_next
           << "Runtime Dynamic = " << router->buffer.rt_power.readOp.dynamic / nocdynp.executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << indent_str << "Crossbar:" << endl;
      cout << indent_str << indent_str_next << "Area = " << router->crossbar.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str << indent_str_next
           << "TDP Dynamic = " << router->crossbar.power.readOp.dynamic * nocdynp.clockRate * nocdynp.input_ports * M << " W"
           << endl;
      cout << indent_str << indent_str_next << "Subthreshold Leakage = " << router->crossbar.power.readOp.leakage << " W" << endl;
      cout << indent_str << indent_str_next << "Gate Leakage = " << router->crossbar.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str << indent_str_next
           << "Runtime Dynamic = " << router->crossbar.rt_power.readOp.dynamic / nocdynp.executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << indent_str << "Arbiter:" << endl;
      cout << indent_str << indent_str_next
           << "TDP Dynamic = " << router->arbiter.power.readOp.dynamic * nocdynp.clockRate * nocdynp.input_ports * M << " W"
           << endl;
      cout << indent_str << indent_str_next << "Subthreshold Leakage = " << router->arbiter.power.readOp.leakage << " W" << endl;
      cout << indent_str << indent_str_next << "Gate Leakage = " << router->arbiter.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str << indent_str_next
           << "Runtime Dynamic = " << router->arbiter.rt_power.readOp.dynamic / nocdynp.executionTime << " W" << endl;
      cout << endl;
    }
  } else {
    //		cout << indent_str_next << "Instruction Fetch Unit    TDP Dynamic = " << ifu->rt_power.readOp.dynamic*clockRate << " W" <<
    // endl; 		cout << indent_str_next << "Instruction Fetch Unit    Subthreshold Leakage = " << ifu->rt_power.readOp.leakage <<" W"
    // <<
    // endl; 		cout << indent_str_next << "Instruction Fetch Unit    Gate Leakage = " << ifu->rt_power.readOp.gate_leakage << " W"
    // << endl; 		cout << indent_str_next << "Load Store Unit   TDP Dynamic = " << lsu->rt_power.readOp.dynamic*clockRate  << " W"
    // << endl; 		cout << indent_str_next << "Load Store Unit   Subthreshold Leakage = " << lsu->rt_power.readOp.leakage  << " W" <<
    // endl; 		cout << indent_str_next << "Load Store Unit   Gate Leakage = " << lsu->rt_power.readOp.gate_leakage  << " W" << endl;
    //		cout << indent_str_next << "Memory Management Unit   TDP Dynamic = " << mmu->rt_power.readOp.dynamic*clockRate  << " W" <<
    // endl; 		cout << indent_str_next << "Memory Management Unit   Subthreshold Leakage = " << mmu->rt_power.readOp.leakage  << "
    // W"
    //<< endl; 		cout << indent_str_next << "Memory Management Unit   Gate Leakage = " << mmu->rt_power.readOp.gate_leakage  << "
    //W"
    //<< endl; 		cout << indent_str_next << "Execution Unit   TDP Dynamic = " << exu->rt_power.readOp.dynamic*clockRate  << " W" <<
    // endl; 		cout << indent_str_next << "Execution Unit   Subthreshold Leakage = " << exu->rt_power.readOp.leakage  << " W" <<
    // endl; 		cout << indent_str_next << "Execution Unit   Gate Leakage = " << exu->rt_power.readOp.gate_leakage  << " W" << endl;
  }
}

void NoC::set_noc_param() {

  nocdynp.clockRate = XML->sys.NoC[ithNoC].clockrate;
  nocdynp.clockRate *= 1e6;
  nocdynp.executionTime = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
  //	currentChipArea=currentChipArea_*1e6;//change mm to um
  //	hasGlobalLink=bool(XML->sys.NoC[ithNoC].has_global_link);

  nocdynp.flit_size                   = XML->sys.NoC[ithNoC].flit_bits;
  nocdynp.input_ports                 = XML->sys.NoC[ithNoC].input_ports;
  nocdynp.output_ports                = XML->sys.NoC[ithNoC].output_ports;
  nocdynp.virtual_channel_per_port    = XML->sys.NoC[ithNoC].virtual_channel_per_port;
  nocdynp.input_buffer_entries_per_vc = XML->sys.NoC[ithNoC].input_buffer_entries_per_vc;

  nocdynp.horizontal_nodes = XML->sys.NoC[ithNoC].horizontal_nodes;
  nocdynp.vertical_nodes   = XML->sys.NoC[ithNoC].vertical_nodes;
  nocdynp.total_nodes      = nocdynp.horizontal_nodes * nocdynp.vertical_nodes;
}

// eka, update runtime parameters
void NoC::update_rtparam(ParseXML *XML_interface, int ithNoC_, InputParameter *interface_ip_) {
  ithNoC = ithNoC_;
  XML    = XML_interface;
  // ithCache     = ithCache_;
  // interface_ip = interface_ip_
}

NoC ::~NoC() {

  if(router) {
    delete router;
    router = 0;
  }
}
