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

#include "sharedcache.h"
#include "XML_Parse.h"
#include "arbiter.h"
#include "array.h"
#include "basic_circuit.h"
#include "const.h"
#include "io.h"
#include "logic.h"
#include "parameter.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string.h>

SharedCache::SharedCache(ParseXML *XML_interface, int ithCache_, InputParameter *interface_ip_, enum cache_level cacheL_)
    : XML(XML_interface)
    , ithCache(ithCache_)
    , interface_ip(*interface_ip_)
    , cacheL(cacheL_) {
  int  idx;
  int  tag, data;
  bool debug;
  debug = false;
  set_cache_param();

  // All lower level cache are physically indexed and tagged.
  int size, line, assoc, banks;
  size  = cachep.capacity;
  line  = cachep.blockW;
  assoc = cachep.assoc;
  banks = cachep.nbanks;
  if((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory)) {
    assoc = 0;
    tag   = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  } else {
    idx = debug ? 9 : int(ceil(log2(size / line / assoc)));
    tag = debug ? 51 : XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  }

  //  if (XML->sys.first_level_dir==2)
  //	  tag += int(XML->sys.domain_size + 5);
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = size;
  interface_ip.line_sz             = line;
  interface_ip.assoc               = assoc;
  interface_ip.nbanks              = banks;
  interface_ip.out_w               = interface_ip.line_sz * 8 / 2;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = cachep.throughput;
  interface_ip.latency             = cachep.latency;
  interface_ip.is_cache            = true;
  interface_ip.pure_ram            = false;
  interface_ip.pure_cam            = false;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 1; // lower level cache usually has one port.
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 1;

  switch(cacheL) {
  case L2:
    interface_ip.freq = XML->sys.L2[0].clockrate;
    break;
  case L3:
    interface_ip.freq = XML->sys.L3[0].clockrate;
    break;
  case L1Directory:
    interface_ip.freq = XML->sys.L1Directory[0].clockrate;
    break;
  case L2Directory:
    interface_ip.freq = XML->sys.L2Directory[0].clockrate;
    break;
  case STLB:
    interface_ip.freq = XML->sys.STLB[0].clockrate;
    break;
  case L2G:
    interface_ip.freq = XML->sys.gpu.L2.clockrate;
  default:
    break;
  }
  if(cacheL == STLB) {

    interface_ip.specific_tag = 1;
    tag                  = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) + EXTRA_TAG_BITS;
    data                 = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
    interface_ip.tag_w   = tag;
    interface_ip.line_sz = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
    // if (XML->sys.homogeneous_STLBs)
    //	interface_ip.cache_sz            *= XML->sys.number_of_cores;

    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8;
    interface_ip.access_mode         = 2;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = 0;
    interface_ip.num_rd_ports        = 1;
    interface_ip.num_wr_ports        = 1;
    interface_ip.num_se_rd_ports     = 0;
    interface_ip.num_search_ports    = 1;
  }
  unicache.caches = new ArrayST(&interface_ip, cachep.name + "cache");
  unicache.area.set_area(unicache.area.get_area() + unicache.caches->local_result.area);
  area.set_area(area.get_area() + unicache.caches->local_result.area);
  interface_ip.force_cache_config = false;

  if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {

    tag                       = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                      = (XML->sys.physical_address_width) + int(ceil(log2(size / line))) + unicache.caches->l_ip.line_sz;
    interface_ip.specific_tag = 1;
    interface_ip.tag_w        = tag;
    interface_ip.line_sz      = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
    interface_ip.cache_sz     = cachep.missb_size * interface_ip.line_sz;
    interface_ip.assoc        = 0;
    interface_ip.is_cache     = true;
    interface_ip.pure_ram     = false;
    interface_ip.pure_cam     = false;
    interface_ip.nbanks       = 1;
    interface_ip.out_w        = interface_ip.line_sz * 8 / 2;
    interface_ip.access_mode  = 0;
    interface_ip.throughput   = cachep.throughput; // means cycle time
    interface_ip.latency      = cachep.latency;    // means access time
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = 1;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    unicache.missb                   = new ArrayST(&interface_ip, cachep.name + "MissB");
    unicache.area.set_area(unicache.area.get_area() + unicache.missb->local_result.area);
    area.set_area(area.get_area() + unicache.missb->local_result.area);
    // fill buffer
    tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                             = unicache.caches->l_ip.line_sz;
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
    interface_ip.cache_sz            = data * cachep.fu_size;
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8 / 2;
    interface_ip.access_mode         = 0;
    interface_ip.throughput          = cachep.throughput;
    interface_ip.latency             = cachep.latency;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = 1;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    unicache.ifb                     = new ArrayST(&interface_ip, cachep.name + "FillB");
    unicache.area.set_area(unicache.area.get_area() + unicache.ifb->local_result.area);
    area.set_area(area.get_area() + unicache.ifb->local_result.area);
    // prefetch buffer
    tag  = XML->sys.physical_address_width + EXTRA_TAG_BITS; // check with previous entries to decide wthether to merge.
    data = unicache.caches->l_ip.line_sz;                    // separate queue to prevent from cache polution.
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
    interface_ip.cache_sz            = cachep.prefetchb_size * interface_ip.line_sz;
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8 / 2;
    interface_ip.access_mode         = 0;
    interface_ip.throughput          = cachep.throughput;
    interface_ip.latency             = cachep.latency;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = 1;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    unicache.prefetchb               = new ArrayST(&interface_ip, cachep.name + "PrefetchB");
    unicache.area.set_area(unicache.area.get_area() + unicache.prefetchb->local_result.area);
    area.set_area(area.get_area() + unicache.prefetchb->local_result.area);
    // WBB
    tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                             = unicache.caches->l_ip.line_sz;
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data;
    interface_ip.cache_sz            = cachep.wbb_size * interface_ip.line_sz;
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8 / 2;
    interface_ip.access_mode         = 0;
    interface_ip.throughput          = cachep.throughput;
    interface_ip.latency             = cachep.latency;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = 1;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    unicache.wbb                     = new ArrayST(&interface_ip, cachep.name + "WBB");
    unicache.area.set_area(unicache.area.get_area() + unicache.wbb->local_result.area);
    area.set_area(area.get_area() + unicache.wbb->local_result.area);
  }
}

void SharedCache::computeEnergy(bool is_tdp) {
  executionTime = XML->sys.executionTime;
  if(is_tdp) {
    // eka
    // power.reset();
    if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {
      // init stats for TDP
      unicache.caches->stats_t.readAc.access  = 0.67 * unicache.caches->l_ip.num_rw_ports;
      unicache.caches->stats_t.readAc.miss    = 0;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = 0.33 * unicache.caches->l_ip.num_rw_ports;
      unicache.caches->stats_t.writeAc.miss   = 0;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->tdp_stats              = unicache.caches->stats_t;

      unicache.missb->stats_t.readAc.access  = 0; // unicache.missb->l_ip.num_search_ports;
      unicache.missb->stats_t.writeAc.access = 0; // unicache.missb->l_ip.num_search_ports;
      unicache.missb->tdp_stats              = unicache.missb->stats_t;

      unicache.ifb->stats_t.readAc.access  = 0; // unicache.ifb->l_ip.num_search_ports;
      unicache.ifb->stats_t.writeAc.access = 0; // unicache.ifb->l_ip.num_search_ports;
      unicache.ifb->tdp_stats              = unicache.ifb->stats_t;

      unicache.prefetchb->stats_t.readAc.access  = 0; // unicache.prefetchb->l_ip.num_search_ports;
      unicache.prefetchb->stats_t.writeAc.access = 0; // unicache.ifb->l_ip.num_search_ports;
      unicache.prefetchb->tdp_stats              = unicache.prefetchb->stats_t;

      unicache.wbb->stats_t.readAc.access  = 0; // unicache.wbb->l_ip.num_search_ports;
      unicache.wbb->stats_t.writeAc.access = 0; // unicache.wbb->l_ip.num_search_ports;
      unicache.wbb->tdp_stats              = unicache.wbb->stats_t;
    } else {
      unicache.caches->stats_t.readAc.access  = unicache.caches->l_ip.num_search_ports;
      unicache.caches->stats_t.readAc.miss    = 0;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = 0;
      unicache.caches->stats_t.writeAc.miss   = 0;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->tdp_stats              = unicache.caches->stats_t;
    }

  } else {
    // init stats for runtime power (RTP)
    // eka
    rt_power.reset();
    if(cacheL == L2) {
      unicache.caches->stats_t.readAc.access  = XML->sys.L2[ithCache].read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.L2[ithCache].read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.L2[ithCache].write_accesses;
      unicache.caches->stats_t.writeAc.miss   = XML->sys.L2[ithCache].write_misses;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    } else if(cacheL == L2G) {
      unicache.caches->stats_t.readAc.access  = XML->sys.gpu.L2.read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.gpu.L2.read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.gpu.L2.write_accesses;
      unicache.caches->stats_t.writeAc.miss   = XML->sys.gpu.L2.write_misses;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    } else if(cacheL == STLB) {
      unicache.caches->stats_t.readAc.access  = XML->sys.STLB[ithCache].read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.STLB[ithCache].read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.STLB[ithCache].write_accesses;
      unicache.caches->stats_t.writeAc.miss   = 0;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    } else if(cacheL == L3) {
      unicache.caches->stats_t.readAc.access  = XML->sys.L3[ithCache].read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.L3[ithCache].read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.L3[ithCache].write_accesses;
      unicache.caches->stats_t.writeAc.miss   = XML->sys.L3[ithCache].write_misses;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    } else if(cacheL == L1Directory) {
      unicache.caches->stats_t.readAc.access  = XML->sys.L1Directory[ithCache].read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.L1Directory[ithCache].read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.L1Directory[ithCache].write_accesses;
      unicache.caches->stats_t.writeAc.miss   = XML->sys.L1Directory[ithCache].write_misses;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    } else if(cacheL == L2Directory) {
      unicache.caches->stats_t.readAc.access  = XML->sys.L2Directory[ithCache].read_accesses;
      unicache.caches->stats_t.readAc.miss    = XML->sys.L2Directory[ithCache].read_misses;
      unicache.caches->stats_t.readAc.hit     = unicache.caches->stats_t.readAc.access - unicache.caches->stats_t.readAc.miss;
      unicache.caches->stats_t.writeAc.access = XML->sys.L2Directory[ithCache].write_accesses;
      unicache.caches->stats_t.writeAc.miss   = XML->sys.L2Directory[ithCache].write_misses;
      unicache.caches->stats_t.writeAc.hit    = unicache.caches->stats_t.writeAc.access - unicache.caches->stats_t.writeAc.miss;
      unicache.caches->rtp_stats              = unicache.caches->stats_t;
    }
    if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {
      // assuming write back policy for data cache TODO: add option for this in XML
      unicache.missb->stats_t.readAc.access  = unicache.caches->stats_t.writeAc.miss;
      unicache.missb->stats_t.writeAc.access = unicache.caches->stats_t.writeAc.miss;
      unicache.missb->rtp_stats              = unicache.missb->stats_t;

      unicache.ifb->stats_t.readAc.access  = unicache.caches->stats_t.writeAc.miss;
      unicache.ifb->stats_t.writeAc.access = unicache.caches->stats_t.writeAc.miss;
      unicache.ifb->rtp_stats              = unicache.ifb->stats_t;

      unicache.prefetchb->stats_t.readAc.access  = unicache.caches->stats_t.writeAc.miss;
      unicache.prefetchb->stats_t.writeAc.access = unicache.caches->stats_t.writeAc.miss;
      unicache.prefetchb->rtp_stats              = unicache.prefetchb->stats_t;

      unicache.wbb->stats_t.readAc.access  = unicache.caches->stats_t.writeAc.miss;
      unicache.wbb->stats_t.writeAc.access = unicache.caches->stats_t.writeAc.miss;
      unicache.wbb->rtp_stats              = unicache.wbb->stats_t;
    }
  }

  unicache.power_t.reset();
  if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {
    unicache.power_t.readOp.dynamic +=
        (unicache.caches->stats_t.readAc.hit * unicache.caches->local_result.power.readOp.dynamic +
         unicache.caches->stats_t.readAc.miss * unicache.caches->local_result.tag_array2->power.readOp.dynamic +
         unicache.caches->stats_t.writeAc.miss * unicache.caches->local_result.tag_array2->power.writeOp.dynamic +
         unicache.caches->stats_t.writeAc.access *
             unicache.caches->local_result.power.writeOp.dynamic); // write miss will generate a write later
    unicache.power_t.readOp.dynamic +=
        unicache.missb->stats_t.readAc.access * unicache.missb->local_result.power.searchOp.dynamic +
        unicache.missb->stats_t.writeAc.access *
            unicache.missb->local_result.power.writeOp.dynamic; // each access to missb involves a CAM and a write
    unicache.power_t.readOp.dynamic += unicache.ifb->stats_t.readAc.access * unicache.ifb->local_result.power.searchOp.dynamic +
                                       unicache.ifb->stats_t.writeAc.access * unicache.ifb->local_result.power.writeOp.dynamic;
    unicache.power_t.readOp.dynamic +=
        unicache.prefetchb->stats_t.readAc.access * unicache.prefetchb->local_result.power.searchOp.dynamic +
        unicache.prefetchb->stats_t.writeAc.access * unicache.prefetchb->local_result.power.writeOp.dynamic;
    unicache.power_t.readOp.dynamic += unicache.wbb->stats_t.readAc.access * unicache.wbb->local_result.power.searchOp.dynamic +
                                       unicache.wbb->stats_t.writeAc.access * unicache.wbb->local_result.power.writeOp.dynamic;
  } else {
    unicache.power_t.readOp.dynamic +=
        (unicache.caches->stats_t.readAc.access * unicache.caches->local_result.power.searchOp.dynamic +
         unicache.caches->stats_t.writeAc.access * unicache.caches->local_result.power.writeOp.dynamic);
  }

  if(is_tdp) {
    unicache.power = unicache.power_t + (unicache.caches->local_result.power) * pppm_lkg;
    if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {
      unicache.power = unicache.power + (unicache.missb->local_result.power + unicache.ifb->local_result.power +
                                         unicache.prefetchb->local_result.power + unicache.wbb->local_result.power) *
                                            pppm_lkg;
    }
    power = power + unicache.power;
  } else {
    unicache.rt_power = unicache.power_t + (unicache.caches->local_result.power) * pppm_lkg;
    if(!((cachep.dir_ty == ST && cacheL == L1Directory) || (cachep.dir_ty == ST && cacheL == L2Directory) || (cacheL == STLB))) {
      unicache.rt_power = unicache.rt_power + (unicache.missb->local_result.power + unicache.ifb->local_result.power +
                                               unicache.prefetchb->local_result.power + unicache.wbb->local_result.power) *
                                                  pppm_lkg;
    }
    rt_power = rt_power + unicache.rt_power;
  }
}

void SharedCache::displayEnergy(uint32_t indent, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << cachep.name << endl;
    cout << indent_str << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str << "TDP Dynamic = " << power.readOp.dynamic * cachep.clockRate << " W" << endl;
    cout << indent_str << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << indent_str << "Runtime Dynamic = " << rt_power.readOp.dynamic / cachep.executionTime << " W" << endl;
    cout << endl;
  } else {
  }
}

void SharedCache::set_cache_param() {
  if(cacheL == L2) {
    cachep.name      = "L2";
    cachep.dir_ty    = NonDir;
    cachep.clockRate = XML->sys.L2[ithCache].clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.L2[ithCache].device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.L2[ithCache].device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.L2[ithCache].device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.L2[ithCache].device_type;
    cachep.capacity                             = XML->sys.L2[ithCache].L2_config[0];
    cachep.blockW                               = XML->sys.L2[ithCache].L2_config[1];
    cachep.assoc                                = XML->sys.L2[ithCache].L2_config[2];
    cachep.nbanks                               = XML->sys.L2[ithCache].L2_config[3];
    cachep.throughput                           = XML->sys.L2[ithCache].L2_config[4] / cachep.clockRate;
    cachep.latency                              = XML->sys.L2[ithCache].L2_config[5] / cachep.clockRate;
    cachep.missb_size                           = XML->sys.L2[ithCache].buffer_sizes[0];
    cachep.fu_size                              = XML->sys.L2[ithCache].buffer_sizes[1];
    cachep.prefetchb_size                       = XML->sys.L2[ithCache].buffer_sizes[2];
    cachep.wbb_size                             = XML->sys.L2[ithCache].buffer_sizes[3];
  }
  if(cacheL == L2G) {
    cachep.name      = "L2G";
    cachep.dir_ty    = NonDir;
    cachep.clockRate = XML->sys.gpu.L2.clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.gpu.L2.device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.gpu.L2.device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.gpu.L2.device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.gpu.L2.device_type;
    cachep.capacity                             = XML->sys.gpu.L2.L2_config[0];
    cachep.blockW                               = XML->sys.gpu.L2.L2_config[1];
    cachep.assoc                                = XML->sys.gpu.L2.L2_config[2];
    cachep.nbanks                               = XML->sys.gpu.L2.L2_config[3];
    cachep.throughput                           = XML->sys.gpu.L2.L2_config[4] / cachep.clockRate;
    cachep.latency                              = XML->sys.gpu.L2.L2_config[5] / cachep.clockRate;
    cachep.missb_size                           = XML->sys.gpu.L2.buffer_sizes[0];
    cachep.fu_size                              = XML->sys.gpu.L2.buffer_sizes[1];
    cachep.prefetchb_size                       = XML->sys.gpu.L2.buffer_sizes[2];
    cachep.wbb_size                             = XML->sys.gpu.L2.buffer_sizes[3];
  } else if(cacheL == STLB) {
    cachep.name      = "STLB";
    cachep.dir_ty    = NonDir;
    cachep.clockRate = XML->sys.STLB[ithCache].clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.STLB[ithCache].device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.STLB[ithCache].device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.STLB[ithCache].device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.STLB[ithCache].device_type;
    cachep.capacity                             = XML->sys.STLB[ithCache].STLB_config[0];
    cachep.blockW                               = XML->sys.STLB[ithCache].STLB_config[1];
    cachep.assoc                                = XML->sys.STLB[ithCache].STLB_config[2];
    cachep.nbanks                               = XML->sys.STLB[ithCache].STLB_config[3];
    cachep.throughput                           = XML->sys.STLB[ithCache].STLB_config[4] / cachep.clockRate;
    cachep.latency                              = XML->sys.STLB[ithCache].STLB_config[5] / cachep.clockRate;
  } else if(cacheL == L3) {
    cachep.name      = "L3";
    cachep.dir_ty    = NonDir;
    cachep.clockRate = XML->sys.L3[ithCache].clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.L3[ithCache].device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.L3[ithCache].device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.L3[ithCache].device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.L3[ithCache].device_type;
    cachep.capacity                             = XML->sys.L3[ithCache].L3_config[0];
    //	if (XML->sys.homogeneous_L3s)
    //			cachep.capacity           *= XML->sys.number_of_cores;
    cachep.blockW         = XML->sys.L3[ithCache].L3_config[1];
    cachep.assoc          = XML->sys.L3[ithCache].L3_config[2];
    cachep.nbanks         = XML->sys.L3[ithCache].L3_config[3];
    cachep.throughput     = XML->sys.L3[ithCache].L3_config[4] / cachep.clockRate;
    cachep.latency        = XML->sys.L3[ithCache].L3_config[5] / cachep.clockRate;
    cachep.missb_size     = XML->sys.L3[ithCache].buffer_sizes[0];
    cachep.fu_size        = XML->sys.L3[ithCache].buffer_sizes[1];
    cachep.prefetchb_size = XML->sys.L3[ithCache].buffer_sizes[2];
    cachep.wbb_size       = XML->sys.L3[ithCache].buffer_sizes[3];
  } else if(cacheL == L1Directory) {
    cachep.name      = "First Level Directory";
    cachep.dir_ty    = (enum Dir_type)XML->sys.L1Directory[ithCache].Directory_type;
    cachep.clockRate = XML->sys.L1Directory[ithCache].clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.total_cycles / (XML->sys.target_core_clockrate * 1e6);
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.L1Directory[ithCache].device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.L1Directory[ithCache].device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.L1Directory[ithCache].device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.L1Directory[ithCache].device_type;
    cachep.capacity                             = XML->sys.L1Directory[ithCache].Dir_config[0];
    cachep.blockW                               = XML->sys.L1Directory[ithCache].Dir_config[1];
    cachep.assoc                                = XML->sys.L1Directory[ithCache].Dir_config[2];
    cachep.nbanks                               = XML->sys.L1Directory[ithCache].Dir_config[3];
    cachep.throughput                           = XML->sys.L1Directory[ithCache].Dir_config[4] / cachep.clockRate;
    cachep.latency                              = XML->sys.L1Directory[ithCache].Dir_config[5] / cachep.clockRate;
    cachep.missb_size                           = XML->sys.L1Directory[ithCache].buffer_sizes[0];
    cachep.fu_size                              = XML->sys.L1Directory[ithCache].buffer_sizes[1];
    cachep.prefetchb_size                       = XML->sys.L1Directory[ithCache].buffer_sizes[2];
    cachep.wbb_size                             = XML->sys.L1Directory[ithCache].buffer_sizes[3];
  } else if(cacheL == L2Directory) {
    cachep.name      = "Second Level Directory";
    cachep.dir_ty    = (enum Dir_type)XML->sys.L2Directory[ithCache].Directory_type;
    cachep.clockRate = XML->sys.L2Directory[ithCache].clockrate;
    cachep.clockRate *= 1e6;
    cachep.executionTime                        = XML->sys.executionTime;
    interface_ip.data_arr_ram_cell_tech_type    = XML->sys.L2Directory[ithCache].device_type; // long channel device LSTP
    interface_ip.data_arr_peri_global_tech_type = XML->sys.L2Directory[ithCache].device_type;
    interface_ip.tag_arr_ram_cell_tech_type     = XML->sys.L2Directory[ithCache].device_type;
    interface_ip.tag_arr_peri_global_tech_type  = XML->sys.L2Directory[ithCache].device_type;
    cachep.capacity                             = XML->sys.L2Directory[ithCache].Dir_config[0];
    cachep.blockW                               = XML->sys.L2Directory[ithCache].Dir_config[1];
    cachep.assoc                                = XML->sys.L2Directory[ithCache].Dir_config[2];
    cachep.nbanks                               = XML->sys.L2Directory[ithCache].Dir_config[3];
    cachep.throughput                           = XML->sys.L2Directory[ithCache].Dir_config[4] / cachep.clockRate;
    cachep.latency                              = XML->sys.L2Directory[ithCache].Dir_config[5] / cachep.clockRate;
    cachep.missb_size                           = XML->sys.L2Directory[ithCache].buffer_sizes[0];
    cachep.fu_size                              = XML->sys.L2Directory[ithCache].buffer_sizes[1];
    cachep.prefetchb_size                       = XML->sys.L2Directory[ithCache].buffer_sizes[2];
    cachep.wbb_size                             = XML->sys.L2Directory[ithCache].buffer_sizes[3];
  }
}
// eka, to update runtime parameters
void SharedCache::update_rtparam(ParseXML *XML_interface, int ithCache_, InputParameter *interface_ip_) {
  ithCache = ithCache_;
  XML      = XML_interface;
  // ithCache     = ithCache_;
  // interface_ip = interface_ip_
}
