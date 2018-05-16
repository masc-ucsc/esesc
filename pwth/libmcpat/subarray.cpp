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

#include <assert.h>
#include <iostream>
#include <math.h>

#include "subarray.h"

Subarray::Subarray(const DynamicParameter &dp_, bool is_fa_)
    : dp(dp_)
    , num_rows(dp.num_r_subarray)
    , num_cols(dp.num_c_subarray)
    , num_cols_fa_cam(dp.tag_num_c_subarray)
    , num_cols_fa_ram(dp.data_num_c_subarray)
    , cell(dp.cell)
    , cam_cell(dp.cam_cell)
    , is_fa(is_fa_) {
  // num_cols=7;
  // cout<<"num_cols ="<< num_cols <<endl;
  if(!(is_fa || dp.pure_cam)) {
    num_cols += (g_ip->add_ecc_b_ ? (int)ceil(num_cols / num_bits_per_ecc_b_) : 0); // ECC overhead
    uint32_t ram_num_cells_wl_stitching =
        (dp.ram_cell_tech_type == lp_dram)
            ? dram_num_cells_wl_stitching_
            : (dp.ram_cell_tech_type == comm_dram) ? comm_dram_num_cells_wl_stitching_ : sram_num_cells_wl_stitching_;

    area.h = cell.h * num_rows;
    area.w =
        cell.w * num_cols + ceil(num_cols / ram_num_cells_wl_stitching) * g_tp.ram_wl_stitching_overhead_; // stitching overhead
  } else                                                                                                   // cam fa
  {

    // should not add dummy row here since the dummy row do not need decoder
    if(is_fa) // fully associative cache
    {
      num_cols_fa_cam += g_ip->add_ecc_b_ ? (int)ceil(num_cols_fa_cam / num_bits_per_ecc_b_) : 0;
      num_cols_fa_ram += (g_ip->add_ecc_b_ ? (int)ceil(num_cols_fa_ram / num_bits_per_ecc_b_) : 0);
      num_cols = num_cols_fa_cam + num_cols_fa_ram;
    } else {
      num_cols_fa_cam += g_ip->add_ecc_b_ ? (int)ceil(num_cols_fa_cam / num_bits_per_ecc_b_) : 0;
      num_cols_fa_ram = 0;
      num_cols        = num_cols_fa_cam;
    }

    area.h = cam_cell.h *
             (num_rows + 1); // height of subarray is decided by CAM array. blank space in sram array are filled with dummy cells
    area.w = cam_cell.w * num_cols_fa_cam + cell.w * num_cols_fa_ram +
             ceil((num_cols_fa_cam + num_cols_fa_ram) / sram_num_cells_wl_stitching_) * g_tp.ram_wl_stitching_overhead_ +
             16 * g_tp.wire_local.pitch     // the overhead for the NAND gate to connect the two halves
             + 128 * g_tp.wire_local.pitch; // the overhead for the drivers from matchline to wordline of RAM
  }

  assert(area.h > 0);
  assert(area.w > 0);
  compute_C();
}

Subarray::~Subarray() {
}

double Subarray::get_total_cell_area() {
  //  return (is_fa==false? cell.get_area() * num_rows * num_cols
  //		  //: cam_cell.h*(num_rows+1)*(num_cols_fa_cam + sram_cell.get_area()*num_cols_fa_ram));
  //		  : cam_cell.get_area()*(num_rows+1)*(num_cols_fa_cam + num_cols_fa_ram));
  //		  //: cam_cell.get_area()*(num_rows+1)*num_cols_fa_cam + sram_cell.get_area()*(num_rows+1)*num_cols_fa_ram);//for FA, this
  // area does not include the dummy cells in SRAM arrays.

  if(!(is_fa || dp.pure_cam))
    return (cell.get_area() * num_rows * num_cols);
  else if(is_fa) { // for FA, this area includes the dummy cells in SRAM arrays.
    // return (cam_cell.get_area()*(num_rows+1)*(num_cols_fa_cam + num_cols_fa_ram));
    // cout<<"diff" <<cam_cell.get_area()*(num_rows+1)*(num_cols_fa_cam + num_cols_fa_ram)-
    // cam_cell.h*(num_rows+1)*(cam_cell.w*num_cols_fa_cam + cell.w*num_cols_fa_ram)<<endl;
    return (cam_cell.h * (num_rows + 1) * (cam_cell.w * num_cols_fa_cam + cell.w * num_cols_fa_ram));
  } else
    return (cam_cell.get_area() * (num_rows + 1) * num_cols_fa_cam);
}

void Subarray::compute_C() {
  double c_w_metal = cell.w * g_tp.wire_local.C_per_um;
  double r_w_metal = cell.w * g_tp.wire_local.R_per_um;
  double C_b_metal = cell.h * g_tp.wire_local.C_per_um;
  double C_b_row_drain_C;

  if(dp.is_dram) {
    C_wl = (gate_C_pass(g_tp.dram.cell_a_w, g_tp.dram.b_w, true, true) + c_w_metal) * num_cols;

    if(dp.ram_cell_tech_type == comm_dram) {
      C_bl = num_rows * C_b_metal;
    } else {
      C_b_row_drain_C = drain_C_(g_tp.dram.cell_a_w, NCH, 1, 0, cell.w, true, true) / 2.0; // due to shared contact
      C_bl            = num_rows * (C_b_row_drain_C + C_b_metal);
    }
  } else {
    if(!(is_fa || dp.pure_cam)) {
      C_wl =
          (gate_C_pass(g_tp.sram.cell_a_w, (g_tp.sram.b_w - 2 * g_tp.sram.cell_a_w) / 2.0, false, true) * 2 + c_w_metal) * num_cols;
      C_b_row_drain_C = drain_C_(g_tp.sram.cell_a_w, NCH, 1, 0, cell.w, false, true) / 2.0; // due to shared contact
      C_bl            = num_rows * (C_b_row_drain_C + C_b_metal);
    } else {
      // Following is wordline not matchline
      // CAM portion
      c_w_metal = cam_cell.w * g_tp.wire_local.C_per_um;
      r_w_metal = cam_cell.w * g_tp.wire_local.R_per_um;
      C_wl_cam  = (gate_C_pass(g_tp.cam.cell_a_w, (g_tp.cam.b_w - 2 * g_tp.cam.cell_a_w) / 2.0, false, true) * 2 + c_w_metal) *
                 num_cols_fa_cam;
      R_wl_cam = (r_w_metal)*num_cols_fa_cam;

      if(!dp.pure_cam) {
        // RAM portion
        c_w_metal = cell.w * g_tp.wire_local.C_per_um;
        r_w_metal = cell.w * g_tp.wire_local.R_per_um;
        C_wl_ram  = (gate_C_pass(g_tp.sram.cell_a_w, (g_tp.sram.b_w - 2 * g_tp.sram.cell_a_w) / 2.0, false, true) * 2 + c_w_metal) *
                   num_cols_fa_ram;
        R_wl_ram = (r_w_metal)*num_cols_fa_ram;
      } else {
        C_wl_ram = R_wl_ram = 0;
      }
      C_wl = C_wl_cam + C_wl_ram;
      C_wl += (16 + 128) * g_tp.wire_local.pitch * g_tp.wire_local.C_per_um;

      R_wl = R_wl_cam + R_wl_ram;
      R_wl += (16 + 128) * g_tp.wire_local.pitch * g_tp.wire_local.R_per_um;

      // there are two ways to write to a FA,
      // 1) Write to CAM array then force a match on match line to active the corresponding wordline in RAM;
      // 2) using separate wordline for read/write and search in RAM.
      // We are using the second approach.

      // Bitline CAM portion This is bitline not searchline. We assume no sharing between bitline and searchline according to SUN's
      // implementations.
      C_b_metal       = cam_cell.h * g_tp.wire_local.C_per_um;
      C_b_row_drain_C = drain_C_(g_tp.cam.cell_a_w, NCH, 1, 0, cam_cell.w, false, true) / 2.0; // due to shared contact
      C_bl_cam        = (num_rows + 1) * (C_b_row_drain_C + C_b_metal);
      // height of subarray is decided by CAM array. blank space in sram array are filled with dummy cells
      C_b_row_drain_C = drain_C_(g_tp.sram.cell_a_w, NCH, 1, 0, cell.w, false, true) / 2.0; // due to shared contact
      C_bl            = (num_rows + 1) * (C_b_row_drain_C + C_b_metal);
    }
  }
}
