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

#include "interconnect.h"
#include "wire.h"
#include <assert.h>
#include <iostream>

interconnect::interconnect(string name_, double base_w, double base_h, int data_w, double len,
                           const InputParameter *configure_interface, int start_wiring_level_, enum Wire_type wire_model,
                           double width_s, double space_s, TechnologyParameter::DeviceType *dt)
    : name(name_)
    , in_rise_time(0)
    , out_rise_time(0)
    , l_ip(*configure_interface)
    , base_width(base_w)
    , base_height(base_h)
    , data_width(data_w)
    , wt(wire_model)
    , width_scaling(width_s)
    , space_scaling(space_s)
    , start_wiring_level(start_wiring_level_)
    , length(len)
    , interconnect_latency(10.0)
    , deviceType(dt) {

  max_unpipelined_link_delay = 0; // TODO
  min_w_nmos                 = g_tp.min_w_nmos_;
  min_w_pmos                 = deviceType->n_to_p_eff_curr_drv_ratio * min_w_nmos;

  wt = Global;

  latency          = l_ip.latency;
  throughput       = l_ip.throughput;
  latency_overflow = false; // throughput_overflow=false,

  compute();
  while(delay > latency && width_scaling < 5.0) {
    width_scaling *= 2;
    space_scaling *= 2;
    compute();
  }
  power_bit = power;
  power.readOp.dynamic *= data_width;
  power.readOp.leakage *= data_width;
  power.readOp.gate_leakage *= data_width;

  area.h *= data_width;

  if(latency_overflow == true)
    cout << "Warning: " << name << " wire structure cannot satisfy latency constraint." << endl;

  assert(power.readOp.dynamic > 0);
  assert(power.readOp.leakage > 0);
  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
  local_result.power = power;
  local_result.area  = area.h * area.w;
}

void interconnect::compute() {
  Wire *wtemp1 = 0;

  local_result = init_interface(&l_ip);

  wtemp1                    = new Wire(wt, length, 1, width_scaling, space_scaling);
  delay                     = wtemp1->delay;
  power.readOp.dynamic      = wtemp1->power.readOp.dynamic;
  power.readOp.leakage      = wtemp1->power.readOp.leakage;
  power.readOp.gate_leakage = wtemp1->power.readOp.gate_leakage;
  area.h                    = width_scaling * (wtemp1->wire_width + wtemp1->wire_spacing);
  area.w                    = length;
  delete wtemp1;
}
