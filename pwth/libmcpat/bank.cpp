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

#include "bank.h"
#include <iostream>

Bank::Bank(const DynamicParameter &dyn_p)
    : dp(dyn_p)
    , mat(dp)
    , num_addr_b_mat(dyn_p.number_addr_bits_mat)
    , num_mats_hor_dir(dyn_p.num_mats_h_dir)
    , num_mats_ver_dir(dyn_p.num_mats_v_dir) {
  int RWP;
  int ERP;
  int EWP;
  int SCHP;

  if(dp.use_inp_params) {
    RWP  = dp.num_rw_ports;
    ERP  = dp.num_rd_ports;
    EWP  = dp.num_wr_ports;
    SCHP = dp.num_search_ports;
  } else {
    RWP  = g_ip->num_rw_ports;
    ERP  = g_ip->num_rd_ports;
    EWP  = g_ip->num_wr_ports;
    SCHP = g_ip->num_search_ports;
  }

  int total_addrbits = (dp.number_addr_bits_mat + dp.number_subbanks_decode) * (RWP + ERP + EWP);
  int datainbits     = dp.num_di_b_bank_per_port * (RWP + EWP);
  int dataoutbits    = dp.num_do_b_bank_per_port * (RWP + ERP);
  int searchinbits;
  int searchoutbits;

  if(dp.fully_assoc || dp.pure_cam) {
    datainbits    = dp.num_di_b_bank_per_port * (RWP + EWP);
    dataoutbits   = dp.num_do_b_bank_per_port * (RWP + ERP);
    searchinbits  = dp.num_si_b_bank_per_port * SCHP;
    searchoutbits = dp.num_so_b_bank_per_port * SCHP;
  }

  if(!(dp.fully_assoc || dp.pure_cam)) {
    if(g_ip->fast_access && dp.is_tag == false) {
      dataoutbits *= g_ip->data_assoc;
    }

    htree_in_add   = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, 0, dataoutbits, 0,
                              num_mats_ver_dir * 2, num_mats_hor_dir * 2, Add_htree);
    htree_in_data  = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, 0, dataoutbits, 0,
                               num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_in_htree);
    htree_out_data = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, 0, dataoutbits, 0,
                                num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_out_htree);

    area.w = htree_in_data->area.w;
    area.h = htree_in_data->area.h;
  } else {
    htree_in_add     = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, searchinbits,
                              dataoutbits, searchoutbits, num_mats_ver_dir * 2, num_mats_hor_dir * 2, Add_htree);
    htree_in_data    = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, searchinbits,
                               dataoutbits, searchoutbits, num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_in_htree);
    htree_out_data   = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, searchinbits,
                                dataoutbits, searchoutbits, num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_out_htree);
    htree_in_search  = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, searchinbits,
                                 dataoutbits, searchoutbits, num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_in_htree, true, true);
    htree_out_search = new Htree2(g_ip->wt, (double)mat.area.w, (double)mat.area.h, total_addrbits, datainbits, searchinbits,
                                  dataoutbits, searchoutbits, num_mats_ver_dir * 2, num_mats_hor_dir * 2, Data_out_htree, true);

    area.w = htree_in_data->area.w;
    area.h = htree_in_data->area.h;
  }

  num_addr_b_row_dec                    = _log2(mat.subarray.num_rows);
  num_addr_b_routed_to_mat_for_act      = num_addr_b_row_dec;
  num_addr_b_routed_to_mat_for_rd_or_wr = num_addr_b_mat - num_addr_b_row_dec;
}

Bank::~Bank() {
  delete htree_in_add;
  delete htree_out_data;
  delete htree_in_data;
  if(dp.fully_assoc || dp.pure_cam) {
    delete htree_in_search;
    delete htree_out_search;
  }
}

double Bank::compute_delays(double inrisetime) {
  return mat.compute_delays(inrisetime);
}

void Bank::compute_power_energy() {
  mat.compute_power_energy();

  if(!(dp.fully_assoc || dp.pure_cam)) {
    power.readOp.dynamic += mat.power.readOp.dynamic * dp.num_act_mats_hor_dir;
    power.readOp.leakage += mat.power.readOp.leakage * dp.num_mats;
    power.readOp.gate_leakage += mat.power.readOp.gate_leakage * dp.num_mats;

    power.readOp.dynamic += htree_in_add->power.readOp.dynamic;
    power.readOp.dynamic += htree_out_data->power.readOp.dynamic;

    power.readOp.leakage += htree_in_add->power.readOp.leakage;
    power.readOp.leakage += htree_in_data->power.readOp.leakage;
    power.readOp.leakage += htree_out_data->power.readOp.leakage;
    power.readOp.gate_leakage += htree_in_add->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_in_data->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_out_data->power.readOp.gate_leakage;
  } else {

    power.readOp.dynamic += mat.power.readOp.dynamic; // for fa and cam num_act_mats_hor_dir is 1 for plain r/w
    power.readOp.leakage += mat.power.readOp.leakage * dp.num_mats;
    power.readOp.gate_leakage += mat.power.readOp.gate_leakage * dp.num_mats;

    power.searchOp.dynamic += mat.power.searchOp.dynamic * dp.num_mats;
    power.searchOp.dynamic += mat.power_bl_precharge_eq_drv.searchOp.dynamic + mat.power_sa.searchOp.dynamic +
                              mat.power_bitline.searchOp.dynamic + mat.power_subarray_out_drv.searchOp.dynamic +
                              mat.ml_to_ram_wl_drv->power.readOp.dynamic;

    power.readOp.dynamic += htree_in_add->power.readOp.dynamic;
    power.readOp.dynamic += htree_out_data->power.readOp.dynamic;

    power.searchOp.dynamic += htree_in_search->power.searchOp.dynamic;
    power.searchOp.dynamic += htree_out_search->power.searchOp.dynamic;

    power.readOp.leakage += htree_in_add->power.readOp.leakage;
    power.readOp.leakage += htree_in_data->power.readOp.leakage;
    power.readOp.leakage += htree_out_data->power.readOp.leakage;
    power.readOp.leakage += htree_in_search->power.readOp.leakage;
    power.readOp.leakage += htree_out_search->power.readOp.leakage;

    power.readOp.gate_leakage += htree_in_add->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_in_data->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_out_data->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_in_search->power.readOp.gate_leakage;
    power.readOp.gate_leakage += htree_out_search->power.readOp.gate_leakage;

    // WARNING: Uggly match between CAM and SRAM. CACTI has two different power models, they are not correlated. We used the average
    // across 4 common SRAM/CAM sizes in the core. Not the best, but good to avoid even more buggy data
    power.readOp.leakage /= 160.0 /*80.0*/;
    power.readOp.gate_leakage /= 160 /*80.0*/;
    // END WARNING
  }
  // printf("leak:%d:%d:%g:%g\n",g_ip->cache_sz, g_ip->assoc, power.readOp.leakage, power.readOp.gate_leakage);
}
