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

#ifndef __ROUTER_H__
#define __ROUTER_H__

#include "arbiter.h"
#include "basic_circuit.h"
#include "cacti_interface.h"
#include "component.h"
#include "crossbar.h"
#include "mat.h"
#include "parameter.h"
#include "wire.h"
#include <assert.h>
#include <iostream>

class Router : public Component {
public:
  Router(double flit_size_, double vc_buf, /* vc size = vc_buffer_size * flit_size */
         double vc_count, TechnologyParameter::DeviceType *dt = &(g_tp.peri_global), double I_ = 5, double O_ = 5);
  ~Router();

  void print_router();

  Component arbiter, crossbar, buffer;

  double cycle_time, max_cyc;
  double flit_size;
  double vc_count;
  double vc_buffer_size; /* vc size = vc_buffer_size * flit_size */

private:
  TechnologyParameter::DeviceType *deviceType;
  double                           FREQUENCY; // move this to config file --TODO
  double                           Cw3(double len);
  double                           gate_cap(double w);
  double                           diff_cap(double w, int type /*0 for n-mos and 1 for p-mos*/, double stack);
  enum Wire_type                   wtype;
  enum Wire_placement              wire_placement;
  // corssbar
  double NTtr, PTtr, wt, ht, I, O, NTi, PTi, NTid, PTid, NTod, PTod, TriS1, TriS2;
  double transmission_buf_inpcap();
  double transmission_buf_outcap();
  double transmission_buf_ctrcap();
  double crossbar_inpline();
  double crossbar_outline();
  double crossbar_ctrline();
  double tr_crossbar_power();
  void   cb_stats();
  double arb_power();
  void   arb_stats();
  double buffer_params();
  void   buffer_stats();

  // arbiter

  // buffer

  // router params
  double Vdd;

  void calc_router_parameters();
  void get_router_area();
  void get_router_power();
  void get_router_delay();

  double min_w_pmos;
};

#endif
