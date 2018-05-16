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

#ifndef __HTREE2_H__
#define __HTREE2_H__

#include "assert.h"
#include "basic_circuit.h"
#include "cacti_interface.h"
#include "component.h"
#include "parameter.h"
#include "subarray.h"
#include "wire.h"

// leakge power includes entire htree in a bank (when uca_tree == false)
// leakge power includes only part to one bank when uca_tree == true

class Htree2 : public Component {
public:
  Htree2(enum Wire_type wire_model, double mat_w, double mat_h, int add, int data_in, int search_data_in, int data_out,
         int search_data_out, int bl, int wl, enum Htree_type h_type, bool uca_tree_ = false, bool search_tree_ = false,
         TechnologyParameter::DeviceType *dt = &(g_tp.peri_global));
  ~Htree2(){};

  void in_htree();
  void out_htree();

  // repeaters only at h-tree nodes
  void limited_in_htree();
  void limited_out_htree();
  void input_nand(double s1, double s2, double l);
  void output_buffer(double s1, double s2, double l);

  double in_rise_time, out_rise_time;

  void set_in_rise_time(double rt) {
    in_rise_time = rt;
  }

  double   max_unpipelined_link_delay;
  powerDef power_bit;

private:
  double          wire_bw;
  double          init_wire_bw; // bus width at root
  enum Htree_type tree_type;
  double          htree_hnodes;
  double          htree_vnodes;
  double          mat_width;
  double          mat_height;
  int             add_bits, data_in_bits, search_data_in_bits, data_out_bits, search_data_out_bits;
  int             ndbl, ndwl;
  bool            uca_tree; // should have full bandwidth to access all banks in the array simultaneously
  bool            search_tree;

  enum Wire_type wt;
  double         min_w_nmos;
  double         min_w_pmos;

  TechnologyParameter::DeviceType *deviceType;
};

#endif
