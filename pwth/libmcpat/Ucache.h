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

#ifndef __UCACHE_H__
#define __UCACHE_H__

#include "area.h"
#include "nuca.h"
#include "router.h"
#include <list>

#if NEW_BOOST
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/access.hpp>
#endif

class min_values_t {
#if NEW_BOOST
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version);
#endif
public:
  double min_delay;
  double min_dyn;
  double min_leakage;
  double min_area;
  double min_cyc;

  min_values_t()
      : min_delay(BIGNUM)
      , min_dyn(BIGNUM)
      , min_leakage(BIGNUM)
      , min_area(BIGNUM)
      , min_cyc(BIGNUM) {
  }

  void update_min_values(const min_values_t *val);
  void update_min_values(const uca_org_t &res);
  void update_min_values(const nuca_org_t *res);
  void update_min_values(const mem_array *res);
};

struct solution {
  int                         tag_array_index;
  int                         data_array_index;
  list<mem_array *>::iterator tag_array_iter;
  list<mem_array *>::iterator data_array_iter;
  double                      access_time;
  double                      cycle_time;
  double                      area;
  double                      efficiency;
  powerDef                    total_power;
};

bool calculate_time(bool is_tag, int pure_ram, bool pure_cam, double Nspd, unsigned int Ndwl, unsigned int Ndbl, unsigned int Ndcm,
                    unsigned int Ndsam_lev_1, unsigned int Ndsam_lev_2, mem_array *ptr_array, int flag_results_populate,
                    results_mem_array *ptr_results, uca_org_t *ptr_fin_res, bool is_main_mem);

void solve(uca_org_t *fin_res);
void init_tech_params(double tech, bool is_tag);

struct calc_time_mt_wrapper_struct {
  uint32_t tid;
  bool     is_tag;
  bool     pure_ram;
  bool     pure_cam;
  bool     is_main_mem;
  double   Nspd_min;

  min_values_t *data_res;
  min_values_t *tag_res;

  list<mem_array *> data_arr;
  list<mem_array *> tag_arr;
};

void *calc_time_mt_wrapper(void *void_obj);

#endif
