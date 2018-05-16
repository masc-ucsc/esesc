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

#include <math.h>
#include <time.h>

#include "Ucache.h"
#include "area.h"
#include "basic_circuit.h"
#include "cacti_interface.h"
#include "component.h"
#include "const.h"
#include "parameter.h"

#include <algorithm>
#include <iostream>
#include <pthread.h>

using namespace std;

bool mem_array::lt(const mem_array *m1, const mem_array *m2) {
  if(m1->Nspd < m2->Nspd)
    return true;
  else if(m1->Nspd > m2->Nspd)
    return false;
  else if(m1->Ndwl < m2->Ndwl)
    return true;
  else if(m1->Ndwl > m2->Ndwl)
    return false;
  else if(m1->Ndbl < m2->Ndbl)
    return true;
  else if(m1->Ndbl > m2->Ndbl)
    return false;
  else if(m1->deg_bl_muxing < m2->deg_bl_muxing)
    return true;
  else if(m1->deg_bl_muxing > m2->deg_bl_muxing)
    return false;
  else if(m1->Ndsam_lev_1 < m2->Ndsam_lev_1)
    return true;
  else if(m1->Ndsam_lev_1 > m2->Ndsam_lev_1)
    return false;
  else if(m1->Ndsam_lev_2 < m2->Ndsam_lev_2)
    return true;
  else
    return false;
}

void uca_org_t::find_delay() {
  mem_array *data_arr = data_array2;
  mem_array *tag_arr  = tag_array2;

  // check whether it is a regular cache or scratch ram
  if(g_ip->pure_ram || g_ip->pure_cam || g_ip->fully_assoc) {
    access_time = data_arr->access_time;
  }
  // Both tag and data lookup happen in parallel
  // and the entire set is sent over the data array h-tree without
  // waiting for the way-select signal --TODO add the corresponding
  // power overhead Nav
  else if(g_ip->fast_access == true) {
    access_time = MAX(tag_arr->access_time, data_arr->access_time);
  }
  // Tag is accessed first. On a hit, way-select signal along with the
  // address is sent to read/write the appropriate block in the data
  // array
  else if(g_ip->is_seq_acc == true) {
    access_time = tag_arr->access_time + data_arr->access_time;
  }
  // Normal access: tag array access and data array access happen in parallel.
  // But, the data array will wait for the way-select and transfer only the
  // appropriate block over the h-tree.
  else {
    access_time = MAX(tag_arr->access_time + data_arr->delay_senseamp_mux_decoder, data_arr->delay_before_subarray_output_driver) +
                  data_arr->delay_from_subarray_output_driver_to_output;
  }
}

void uca_org_t::find_energy() {
  if(!(g_ip->pure_ram || g_ip->pure_cam || g_ip->fully_assoc)) //(g_ip->is_cache)
    power = data_array2->power + tag_array2->power;
  else
    power = data_array2->power;
}

void uca_org_t::find_area() {
  if(g_ip->pure_ram || g_ip->pure_cam || g_ip->fully_assoc) //(g_ip->is_cache == false)
  {
    cache_ht  = data_array2->height;
    cache_len = data_array2->width;
  } else {
    cache_ht  = MAX(tag_array2->height, data_array2->height);
    cache_len = tag_array2->width + data_array2->width;
  }
  area = cache_ht * cache_len;
}

void uca_org_t::find_cyc() {
  if((g_ip->pure_ram || g_ip->pure_cam || g_ip->fully_assoc)) //(g_ip->is_cache == false)
  {
    cycle_time = data_array2->cycle_time;
  } else {
    cycle_time = MAX(tag_array2->cycle_time, data_array2->cycle_time);
  }
}

void uca_org_t ::cleanup() {
  //	uca_org_t * it_uca_org;
  if(data_array2 != 0) {
    delete data_array2;
    data_array2 = 0;
  }

  if(tag_array2 != 0) {
    delete tag_array2;
    tag_array2 = 0;
  }

  std::vector<uca_org_t *>::size_type sz = uca_q.size();
  for(int i = sz - 1; i >= 0; i--) {
    if(uca_q[i]->data_array2 != 0) {
      delete uca_q[i]->data_array2;
      uca_q[i]->data_array2 = 0;
    }
    if(uca_q[i]->tag_array2 != 0) {
      delete uca_q[i]->tag_array2;
      uca_q[i]->tag_array2 = 0;
    }
    delete uca_q[i];
    uca_q[i] = 0;
    uca_q.pop_back();
  }

  if(uca_pg_reference != 0) {
    if(uca_pg_reference->data_array2 != 0) {
      delete uca_pg_reference->data_array2;
      uca_pg_reference->data_array2 = 0;
    }
    if(uca_pg_reference->tag_array2 != 0) {
      delete uca_pg_reference->tag_array2;
      uca_pg_reference->tag_array2 = 0;
    }
    delete uca_pg_reference;
    uca_pg_reference = 0;
  }
}

#if NEW_BOOST
template <class Archive> void InputParameter::serialize(Archive &ar, unsigned int version) {
  ar &cache_sz;
  ar &line_sz;
  ar &assoc;
  ar &nbanks;
  ar &out_w;
  ar &specific_tag;
  ar &tag_w;
  ar &access_mode;
  // ar & obj_func_dyn_energy;
  // ar & obj_func_dyn_power;
  // ar & obj_func_leak_power;
  // ar & obj_func_cycle_t;
  ar &F_sz_nm;
  ar &F_sz_um;
  ar &num_rw_ports;
  ar &num_rd_ports;
  ar &num_wr_ports;
  ar &num_se_rd_ports;
  ar &num_search_ports;
  ar &is_main_mem;
  ar &is_cache;
  ar &pure_ram;
  ar &pure_cam;
  ar &rpters_in_htree;
  ar &ver_htree_wires_over_array;
  ar &broadcast_addr_din_over_ver_htrees;
  ar &temp;
  ar &ram_cell_tech_type;
  ar &peri_global_tech_type;
  ar &data_arr_ram_cell_tech_type;
  ar &data_arr_peri_global_tech_type;
  ar &tag_arr_ram_cell_tech_type;
  ar &tag_arr_peri_global_tech_type;
  ar &burst_len;
  ar &int_prefetch_w;
  ar &page_sz_bits;
  ar &ic_proj_type;
  ar &wire_is_mat_type;
  ar &wire_os_mat_type;
  ar &force_wiretype;
  ar &print_input_args;
  ar &nuca_cache_sz;
  ar &ndbl;
  ar &ndwl;
  ar &nspd;
  ar &ndsam1;
  ar &ndsam2;
  ar &ndcm;
  ar &force_cache_config;
  ar &cache_level;
  ar &cores;
  ar &nuca_bank_count;
  ar &force_nuca_bank;
  ar &delay_wt;
  ar &dynamic_power_wt;
  ar &leakage_power_wt, ar &cycle_time_wt;
  ar &area_wt;
  ar &delay_wt_nuca;
  ar &dynamic_power_wt_nuca;
  ar &leakage_power_wt_nuca, ar &cycle_time_wt_nuca;
  ar &area_wt_nuca;
  ar &delay_dev;
  ar &dynamic_power_dev;
  ar &leakage_power_dev, ar &cycle_time_dev;
  ar &area_dev;
  ar &delay_dev_nuca;
  ar &dynamic_power_dev_nuca;
  ar &leakage_power_dev_nuca, ar &cycle_time_dev_nuca;
  ar &area_dev_nuca;
  ar &ed;
  ar &nuca;
  ar &fast_access;
  ar &block_sz;
  ar &tag_assoc;
  ar &data_assoc;
  ar &is_seq_acc;
  ar &fully_assoc;
  ar &nsets;
  ar &print_detail;
  ar &add_ecc_b_;
  ar &throughput;
  ar &latency;
  ar &pipelinable;
  ar &pipeline_stages;
  ar &per_stage_vector;
  ar &with_clock_grid;
  ar &freq;
}

template void InputParameter::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void InputParameter::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);

template <class Archive> void results_mem_array::serialize(Archive &ar, unsigned int version) {
  ar &Ndwl;
  ar &Ndbl;
  ar &Nspd;
  ar &deg_bl_muxing;
  ar &Ndsam_lev_1;
  ar &Ndsam_lev_2;
  ar &number_activated_mats_horizontal_direction;
  ar &number_subbanks;
  ar &page_size_in_bits;
  ar &delay_route_to_bank;
  ar &delay_crossbar;
  ar &delay_addr_din_horizontal_htree;
  ar &delay_addr_din_vertical_htree;
  ar &delay_row_predecode_driver_and_block;
  ar &delay_row_decoder;
  ar &delay_bitlines;
  ar &delay_sense_amp;
  ar &delay_subarray_output_driver;
  ar &delay_bit_mux_predecode_driver_and_block;
  ar &delay_bit_mux_decoder;
  ar &delay_senseamp_mux_lev_1_predecode_driver_and_block;
  ar &delay_senseamp_mux_lev_1_decoder;
  ar &delay_senseamp_mux_lev_2_predecode_driver_and_block;
  ar &delay_senseamp_mux_lev_2_decoder;
  ar &delay_input_htree;
  ar &delay_output_htree;
  ar &delay_dout_vertical_htree;
  ar &delay_dout_horizontal_htree;
  ar &delay_comparator;
  ar &access_time;
  ar &cycle_time;
  ar &multisubbank_interleave_cycle_time;
  ar &delay_request_network;
  ar &delay_inside_mat;
  ar &delay_reply_network;
  ar &trcd;
  ar &cas_latency;
  ar &precharge_delay;
  ar &power_routing_to_bank;
  ar &power_addr_input_htree;
  ar &power_data_input_htree;
  ar &power_data_output_htree;
  ar &power_addr_horizontal_htree;
  ar &power_datain_horizontal_htree;
  ar &power_dataout_horizontal_htree;
  ar &power_addr_vertical_htree;
  ar &power_datain_vertical_htree;
  ar &power_row_predecoder_drivers;
  ar &power_row_predecoder_blocks;
  ar &power_row_decoders;
  ar &power_bit_mux_predecoder_drivers;
  ar &power_bit_mux_predecoder_blocks;
  ar &power_bit_mux_decoders;
  ar &power_senseamp_mux_lev_1_predecoder_drivers;
  ar &power_senseamp_mux_lev_1_predecoder_blocks;
  ar &power_senseamp_mux_lev_1_decoders;
  ar &power_senseamp_mux_lev_2_predecoder_drivers;
  ar &power_senseamp_mux_lev_2_predecoder_blocks;
  ar &power_senseamp_mux_lev_2_decoders;
  ar &power_bitlines;
  ar &power_sense_amps;
  ar &power_prechg_eq_drivers;
  ar &power_output_drivers_at_subarray;
  ar &power_dataout_vertical_htree;
  ar &power_comparators;
  ar &power_crossbar;
  ar &total_power;
  ar &area;
  ar &all_banks_height;
  ar &all_banks_width;
  ar &bank_height;
  ar &bank_width;
  ar &subarray_memory_cell_area_height;
  ar &subarray_memory_cell_area_width;
  ar &mat_height;
  ar &mat_width;
  ar &routing_area_height_within_bank;
  ar &routing_area_width_within_bank;
  ar &area_efficiency;
  ar &refresh_power;
  ar &dram_refresh_period;
  ar &dram_array_availability;
  ar &dyn_read_energy_from_closed_page;
  ar &dyn_read_energy_from_open_page;
  ar &leak_power_subbank_closed_page;
  ar &leak_power_subbank_open_page;
  ar &leak_power_request_and_reply_networks;
  ar &activate_energy;
  ar &read_energy;
  ar &write_energy;
  ar &precharge_energy;
}

template void results_mem_array::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void results_mem_array::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);

template <class Archive> void powerComponents::serialize(Archive &ar, unsigned int version) {
  ar &dynamic;
  ar &leakage;
  ar &gate_leakage;
  ar &short_circuit;
}

template void powerComponents::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void powerComponents::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);

template <class Archive> void powerDef::serialize(Archive &ar, unsigned int version) {
  ar &readOp;
  ar &writeOp;
  ar &searchOp;
}

template void powerDef::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void powerDef::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);

template <class Archive> void uca_org_t::serialize(Archive &ar, unsigned int version) {
  ar &tag_array2;
  ar &data_array2;
  ar &access_time;
  ar &cycle_time;
  ar &area;
  ar &area_efficiency;
  ar &power;
  ar &leak_power_with_sleep_transistors_in_mats;
  ar &cache_ht;
  ar &cache_len;
  ar &file_n;
  ar &vdd_periph_global;
  ar &valid;
  ar &tag_array;
  ar &data_array;
}

template void uca_org_t::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void uca_org_t::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);

template <class Archive> void mem_array::serialize(Archive &ar, unsigned int version) {
  ar &Ndwl;
  ar &Ndbl;
  ar &Nspd;
  ar &deg_bl_muxing;
  ar &Ndsam_lev_1;
  ar &Ndsam_lev_2;
  ar &access_time;
  ar &cycle_time;
  ar &multisubbank_interleave_cycle_time;
  ar &area_ram_cells;
  ar &area;
  ar &power;
  ar &delay_senseamp_mux_decoder;
  ar &delay_before_subarray_output_driver;
  ar &delay_from_subarray_output_driver_to_output;
  ar &height;
  ar &width;
  ar &delay_route_to_bank;
  ar &delay_input_htree;
  ar &delay_row_predecode_driver_and_block;
  ar &delay_row_decoder;
  ar &delay_bitlines;
  ar &delay_sense_amp;
  ar &delay_subarray_output_driver;
  ar &delay_dout_htree;
  ar &delay_comparator;
  ar &delay_matchlines;
  ar &all_banks_height, ar &all_banks_width, ar &area_efficiency;
  ar &power_routing_to_bank;
  ar &power_addr_input_htree;
  ar &power_data_input_htree;
  ar &power_data_output_htree;
  ar &power_htree_in_search;
  ar &power_htree_out_search;
  ar &power_row_predecoder_drivers;
  ar &power_row_predecoder_blocks;
  ar &power_row_decoders;
  ar &power_bit_mux_predecoder_drivers;
  ar &power_bit_mux_predecoder_blocks;
  ar &power_bit_mux_decoders;
  ar &power_senseamp_mux_lev_1_predecoder_drivers;
  ar &power_senseamp_mux_lev_1_predecoder_blocks;
  ar &power_senseamp_mux_lev_1_decoders;
  ar &power_senseamp_mux_lev_2_predecoder_drivers;
  ar &power_senseamp_mux_lev_2_predecoder_blocks;
  ar &power_senseamp_mux_lev_2_decoders;
  ar &power_bitlines;
  ar &power_sense_amps;
  ar &power_prechg_eq_drivers;
  ar &power_output_drivers_at_subarray;
  ar &power_dataout_vertical_htree;
  ar &power_comparators;
  ar &power_cam_bitline_precharge_eq_drv;
  ar &power_searchline;
  ar &power_searchline_precharge;
  ar &power_matchlines;
  ar &power_matchline_precharge;
  ar &power_matchline_to_wordline_drv;
  ar &arr_min;
  ar &wt;
  ar &activate_energy;
  ar &read_energy;
  ar &write_energy;
  ar &precharge_energy;
  ar &refresh_power;
  ar &leak_power_subbank_closed_page;
  ar &leak_power_subbank_open_page;
  ar &leak_power_request_and_reply_networks;
  ar &precharge_delay;
}

template void mem_array::serialize<boost::archive::text_oarchive>(boost::archive::text_oarchive &, unsigned int);
template void mem_array::serialize<boost::archive::text_iarchive>(boost::archive::text_iarchive &, unsigned int);
#endif
