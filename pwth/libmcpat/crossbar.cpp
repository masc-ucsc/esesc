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

#include "crossbar.h"

#define CB_ADJ 2 // height of the cell (layout) in crossbar
// buffer is adjusted to get an aspect ratio close to one;
// when adjust the ratio, the number of wires route over the
// tri-state buffers does not change, however, the effective wiring pitch changes

Crossbar::Crossbar(double n_inp_, double n_out_, double flit_size_, TechnologyParameter::DeviceType *dt)
    : n_inp(n_inp_)
    , n_out(n_out_)
    , flit_size(flit_size_)
    , deviceType(dt) {
  min_w_pmos = deviceType->n_to_p_eff_curr_drv_ratio * g_tp.min_w_nmos_;
  Vdd        = dt->Vdd;
}

Crossbar::~Crossbar() {
}

double Crossbar::output_buffer() {
  double l_eff = n_inp * flit_size * g_tp.wire_outside_mat.pitch;
  Wire   w1(g_ip->wt, l_eff);
  double s1        = w1.repeater_size;
  double pton_size = deviceType->n_to_p_eff_curr_drv_ratio;
  // the model assumes input capacitance of the wire driver = input capacitance of nand + nor = input cap of the driver transistor
  TriS1 = s1 * (1 + pton_size) / (2 + pton_size + 1 + 2 * pton_size);
  TriS2 = s1; // driver transistor

  if(TriS1 < 1)
    TriS1 = 1;

  double input_cap =
      gate_C(TriS1 * (2 * min_w_pmos + g_tp.min_w_nmos_), 0) + gate_C(TriS1 * (min_w_pmos + 2 * g_tp.min_w_nmos_), 0);
  input_cap += drain_C_(TriS1 * g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) +
               drain_C_(TriS1 * min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) * 2 + gate_C(TriS2 * g_tp.min_w_nmos_, 0) +
               drain_C_(TriS1 * min_w_pmos, NCH, 1, 1, g_tp.cell_h_def) * 2 +
               drain_C_(TriS1 * min_w_pmos, PCH, 1, 1, g_tp.cell_h_def) + gate_C(TriS2 * min_w_pmos, 0);
  double output_cap =
      drain_C_(TriS2 * g_tp.min_w_nmos_, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(TriS2 * min_w_pmos, PCH, 1, 1, g_tp.cell_h_def);
  double ctr_cap = gate_C(TriS2 * (min_w_pmos + g_tp.min_w_nmos_), 0);

  tri_inp_cap = input_cap;
  tri_out_cap = output_cap;
  tri_ctr_cap = ctr_cap;
  return input_cap + output_cap + ctr_cap;
}

void Crossbar::compute_power() {
  // double tri_cap = output_buffer();
  // area of a tristate logic
  double g_area = compute_gate_area(INV, 1, TriS2 * g_tp.min_w_nmos_, TriS2 * min_w_pmos, g_tp.cell_h_def);
  g_area *= 2; // to model area of output transistors
  g_area += compute_gate_area(NAND, 2, TriS1 * 2 * g_tp.min_w_nmos_, TriS1 * min_w_pmos, g_tp.cell_h_def);
  g_area += compute_gate_area(NOR, 2, TriS1 * g_tp.min_w_nmos_, TriS1 * 2 * min_w_pmos, g_tp.cell_h_def);
  double width /*per tristate*/ = g_area / (CB_ADJ * g_tp.cell_h_def);
  // effective no. of tristate buffers that need to be laid side by side
  int    ntri     = (int)ceil(g_tp.cell_h_def / (g_tp.wire_outside_mat.pitch));
  double wire_len = MAX(width * ntri * n_out, flit_size * g_tp.wire_outside_mat.pitch * n_out);
  Wire   w1(g_ip->wt, wire_len);

  area.w = wire_len;
  area.h = g_tp.wire_outside_mat.pitch * n_inp * flit_size * CB_ADJ;
  Wire w2(g_ip->wt, area.h);

  power.readOp.dynamic =
      (w1.power.readOp.dynamic + w2.power.readOp.dynamic + (tri_inp_cap * n_out + tri_out_cap * n_inp + tri_ctr_cap) * Vdd * Vdd) *
      flit_size;
  power.readOp.leakage = n_inp * n_out * flit_size *
                         (cmos_Isub_leakage(g_tp.min_w_nmos_ * TriS2 * 2, min_w_pmos * TriS2 * 2, 1, inv) * Vdd +
                          cmos_Isub_leakage(g_tp.min_w_nmos_ * TriS1 * 3, min_w_pmos * TriS1 * 3, 2, nand) * Vdd +
                          cmos_Isub_leakage(g_tp.min_w_nmos_ * TriS1 * 3, min_w_pmos * TriS1 * 3, 2, nor) * Vdd +
                          w1.power.readOp.leakage + w2.power.readOp.leakage);
  power.readOp.gate_leakage = n_inp * n_out * flit_size *
                              (cmos_Ig_leakage(g_tp.min_w_nmos_ * TriS2 * 2, min_w_pmos * TriS2 * 2, 1, inv) * Vdd +
                               cmos_Ig_leakage(g_tp.min_w_nmos_ * TriS1 * 3, min_w_pmos * TriS1 * 3, 2, nand) * Vdd +
                               cmos_Ig_leakage(g_tp.min_w_nmos_ * TriS1 * 3, min_w_pmos * TriS1 * 3, 2, nor) * Vdd +
                               w1.power.readOp.gate_leakage + w2.power.readOp.gate_leakage);
}

void Crossbar::print_crossbar() {
  cout << "\nCrossbar Stats (" << n_inp << "x" << n_out << ")\n\n";
  cout << "Flit size        : " << flit_size << " bits" << endl;
  cout << "Width            : " << area.w << " u" << endl;
  cout << "Height           : " << area.h << " u" << endl;
  cout << "Dynamic Power    : " << power.readOp.dynamic * 1e9 * MIN(n_inp, n_out) << " (nJ)" << endl;
  cout << "Leakage Power    : " << power.readOp.leakage * 1e3 << " (mW)" << endl;
}
