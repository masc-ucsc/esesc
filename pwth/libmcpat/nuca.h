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

#ifndef __NUCA_H__
#define __NUCA_H__

#include "assert.h"
#include "basic_circuit.h"
#include "cacti_interface.h"
#include "component.h"
#include "io.h"
#include "mat.h"
#include "parameter.h"
#include "router.h"
#include "wire.h"
#include <iostream>

class nuca_org_t {
public:
  ~nuca_org_t();
  //    int size;
  /* area, power, access time, and cycle time stats */
  Component nuca_pda;
  Component bank_pda;
  Component wire_pda;
  Wire *    h_wire;
  Wire *    v_wire;
  Router *  router;
  /* for particular network configuration
   * calculated based on a cycle accurate
   * simulation Ref: CACTI 6 - Tech report
   */
  double contention;

  /* grid network stats */
  double avg_hops;
  int    rows;
  int    columns;
  int    bank_count;
};

class Nuca : public Component {
public:
  Nuca(TechnologyParameter::DeviceType *dt);
  void print_router();
  ~Nuca();
  void        sim_nuca();
  void        init_cont();
  int         calc_cycles(double lat, double oper_freq);
  void        calculate_nuca_area(nuca_org_t *nuca);
  int         check_nuca_org(nuca_org_t *n, min_values_t *minval);
  nuca_org_t *find_optimal_nuca(list<nuca_org_t *> *n, min_values_t *minval);
  void        print_nuca(nuca_org_t *n);
  void        print_cont_stats();

private:
  TechnologyParameter::DeviceType *deviceType;
  int                              wt_min, wt_max;
  Wire *                           wire_vertical[WIRE_TYPES], *wire_horizontal[WIRE_TYPES];
};

#endif
