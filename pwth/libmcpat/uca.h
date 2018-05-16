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

#ifndef __UCA_H__
#define __UCA_H__

#include "area.h"
#include "bank.h"
#include "component.h"
#include "htree2.h"
#include "parameter.h"

class UCA : public Component {
public:
  UCA(const DynamicParameter &dyn_p);
  ~UCA();
  double compute_delays(double inrisetime); // returns outrisetime
  void   compute_power_energy();

  DynamicParameter dp;
  Bank             bank;

  Htree2 *htree_in_add;
  Htree2 *htree_in_data;
  Htree2 *htree_out_data;
  Htree2 *htree_in_search;
  Htree2 *htree_out_search;

  powerDef power_routing_to_bank;

  uint32_t nbanks;

  int    num_addr_b_bank;
  int    num_di_b_bank;
  int    num_do_b_bank;
  int    num_si_b_bank;
  int    num_so_b_bank;
  int    RWP, ERP, EWP, SCHP;
  double area_all_dataramcells;

  double dyn_read_energy_from_closed_page;
  double dyn_read_energy_from_open_page;
  double dyn_read_energy_remaining_words_in_burst;

  double refresh_power; // only for DRAM
  double activate_energy;
  double read_energy;
  double write_energy;
  double precharge_energy;
  double leak_power_subbank_closed_page;
  double leak_power_subbank_open_page;
  double leak_power_request_and_reply_networks;

  double delay_array_to_sa_mux_lev_1_decoder;
  double delay_array_to_sa_mux_lev_2_decoder;
  double delay_before_subarray_output_driver;
  double delay_from_subarray_out_drv_to_out;
  double access_time;
  double precharge_delay;
  double multisubbank_interleave_cycle_time;
};

#endif
