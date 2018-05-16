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

#include "area.h"
#include "decoder.h"
#include "parameter.h"
//#include "io.h"
#include "CacheEq.h"
#include "SRAM.h"
#include "SescConf.h"
#include "array.h"
#include "globalvar.h"
#include <assert.h>
#include <iostream>
#include <math.h>
#include <time.h>

using namespace std;

ArrayST::ArrayST(const InputParameter *configure_interface, string _name, bool _is_default, bool precomputed_power)
    : l_ip(*configure_interface)
    , name(_name)
    , is_default(_is_default) {

  fprintf(stderr, "\nAttempting to set up %s", _name.c_str());
  maxDynPower = new GStatsMax("maxpwr_%s", _name.c_str());
  if(l_ip.cache_sz < 64)
    l_ip.cache_sz = 64;
  l_ip.error_checking(); // not only do the error checking but also fill some missing parameters
  //#ifdef ENABLE_PEQ
  bool        doPeq;
  const char *pwrsection = SescConf->getCharPtr("", "pwrmodel", 0);
  doPeq                  = SescConf->getBool(pwrsection, "doPeq", 0);

  if(precomputed_power) {
    I(fprintf(stderr, "Energy values provided for structure %s, ESESC does not compute them", name.c_str()));
    // useProvidedEnergyNumbers();
  } else if(doPeq) {
    optimize_array_peq();
  } else {
    optimize_array();
  }

  fprintf(stderr, "\nDone setting up %s", _name.c_str());
}

//#ifdef ENABLE_PEQ
void ArrayST::optimize_array_peq() {
  local_result.power.reset();
  local_result.power.readOp.dynamic  = 15.9e-12;
  local_result.power.readOp.leakage  = 0;
  local_result.power.writeOp.dynamic = 15.9e-12;
  local_result.power.writeOp.leakage = 0;

  local_result.tag_array2  = new mem_array;
  local_result.data_array2 = new mem_array;
  double assoc             = l_ip.assoc;
  if(l_ip.is_cache && assoc != 0.0) { // It is a cache
    double  tech      = l_ip.F_sz_um;
    double  line_size = l_ip.line_sz;
    double  size      = l_ip.cache_sz;
    CacheEq cache(tech, assoc, line_size, size);
    cache.SetEquation("dynamic");
    local_result.power.readOp.dynamic  = cache.EvalEquation("dynamic");
    local_result.power.writeOp.dynamic = local_result.power.readOp.dynamic;
  } else if((l_ip.pure_ram) || (l_ip.is_cache && assoc == 0.0)) // It is an SRAM
  {
    double tech  = l_ip.F_sz_um;
    double ports = l_ip.num_rw_ports + l_ip.num_rd_ports + l_ip.num_wr_ports + l_ip.num_search_ports;
    double width = l_ip.line_sz;
    double size  = l_ip.cache_sz;

    SRAM sram(tech, ports, width, size);
    sram.SetEquation("dynamic");
    local_result.power.readOp.dynamic  = sram.EvalEquation("dynamic");
    local_result.power.writeOp.dynamic = local_result.power.readOp.dynamic;
  } else // It is neither SRAM nor Cache
  {
    //		local_result.power.readOp.dynamic = 0;
    //		local_result.power.writeOp.dynamic = 0;
    optimize_array();
  }
}
//#endif

void ArrayST::compute_base_power() {
  // l_ip.out_w                =l_ip.line_sz*8;
  local_result = cacti_interface(&l_ip);
}

void ArrayST::optimize_array() {
  list<uca_org_t>           candidate_solutions(0);
  list<uca_org_t>::iterator candidate_iter, min_dynamic_energy_iter;
  //	final_results ret_result;
  //	ret_result.valid=false;
  local_result.valid = false;

  double throughput = l_ip.throughput, latency = l_ip.latency;
  bool   throughput_overflow = true, latency_overflow = true;
  compute_base_power();

  if((local_result.cycle_time - throughput) <= 1e-10)
    throughput_overflow = false;
  if((local_result.access_time - latency) <= 1e-10)
    latency_overflow = false;

  if(opt_for_clk) {
    if(throughput_overflow || latency_overflow) {
      l_ip.ed = 0;

      l_ip.delay_wt      = 100; // Fixed number, make sure timing can be satisfied.
      l_ip.cycle_time_wt = 100;

      l_ip.area_wt          = 10; // Fixed number, This is used to exhaustive search for individual components.
      l_ip.dynamic_power_wt = 10; // Fixed number, This is used to exhaustive search for individual components.
      l_ip.leakage_power_wt = 10;

      l_ip.delay_dev      = 1000000; // Fixed number, make sure timing can be satisfied.
      l_ip.cycle_time_dev = 100;

      l_ip.area_dev          = 1000000; // Fixed number, This is used to exhaustive search for individual components.
      l_ip.dynamic_power_dev = 1000000; // Fixed number, This is used to exhaustive search for individual components.
      l_ip.leakage_power_dev = 1000000;
    }

    while((throughput_overflow || latency_overflow) && l_ip.cycle_time_dev > 10) // && l_ip.delay_dev > 10
    {
      //		from best area to worst area -->worst timing to best timing
      compute_base_power();
      if(((local_result.cycle_time - throughput) <= 1e-10) && (local_result.access_time - latency) <= 1e-10) {
        candidate_solutions.push_back(local_result);
        // output_data_csv(candidate_solutions.back());
        throughput_overflow = false;
        latency_overflow    = false;

      } else {
        // TODO: whether checking the partial satisfied results too, or just change the mark???
        if((local_result.cycle_time - throughput) <= 1e-10)
          throughput_overflow = false;
        if((local_result.access_time - latency) <= 1e-10)
          latency_overflow = false;
      }

      l_ip.cycle_time_dev -= 10;
      // l_ip.delay_dev-=10;
    }

    static bool first = true;
    if(first) {
      IS(printf("\nnOptimization done for"));
      first = false;
    }
    IS(printf("\t'%s' ", name.c_str()));

    // Give warning but still provide a result with best timing found
    if(throughput_overflow == true)
      cout << "Warning: " << name << " array structure cannot satisfy throughput constraint." << endl;
    if(latency_overflow == true)
      cout << "Warning: " << name << " array structure cannot satisfy latency constraint." << endl;

    // double min_dynamic_energy, min_dynamic_power, min_leakage_power, min_cycle_time;
    double min_dynamic_energy = BIGNUM;
    if(candidate_solutions.empty() == false) {
      local_result.valid = true;
      for(candidate_iter = candidate_solutions.begin(); candidate_iter != candidate_solutions.end(); ++candidate_iter)

      {
        if(min_dynamic_energy > (candidate_iter)->power.readOp.dynamic) {
          min_dynamic_energy      = (candidate_iter)->power.readOp.dynamic;
          min_dynamic_energy_iter = candidate_iter;
          local_result            = *(min_dynamic_energy_iter);
          // TODO: since results are reordered results and l_ip may miss match. Therefore, the final output spread sheets may show
          // the miss match.
        }
      }
    }
  }
  double sckRation = g_tp.sckt_co_eff;
  local_result.power.readOp.dynamic *= sckRation;
  local_result.power.writeOp.dynamic *= sckRation;
  local_result.power.searchOp.dynamic *= sckRation;

  local_result.data_array2->power.readOp.dynamic *= sckRation;
  local_result.data_array2->power.writeOp.dynamic *= sckRation;
  local_result.data_array2->power.searchOp.dynamic *= sckRation;

  if(!(l_ip.pure_cam || l_ip.pure_ram || l_ip.fully_assoc) && l_ip.is_cache) {
    local_result.tag_array2->power.readOp.dynamic *= sckRation;
    local_result.tag_array2->power.writeOp.dynamic *= sckRation;
    local_result.tag_array2->power.searchOp.dynamic *= sckRation;
  }

  maxDynPower->sample(
      (local_result.power.readOp.dynamic + local_result.power.searchOp.dynamic + local_result.power.writeOp.dynamic) * l_ip.freq *
          1e6 * 100,
      true);

  double macro_layout_overhead = g_tp.macro_layout_overhead;
  local_result.area *= macro_layout_overhead;
}
