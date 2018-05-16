/*
 * processor.cpp
 *
 *  Created on: Jul 1, 2009
 *      Author: lisheng
 *
 *
 *              Ehsan K.Ardestani
 *              Elnaz Ebrahimi
 */

//#include "io.h"
#include "processor.h"
#include "SescConf.h"
#include "XML_Parse.h"
#include "array.h"
#include "basic_circuit.h"
#include "cacti_interface.h"
#include "const.h"
#include "core.h"
#include "parameter.h"
#include "router.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// eka, TEMPORARY
//#define PRINT_ALL 1

extern bool gpu_tcd_present;
extern bool gpu_tci_present;

// eka, to be able to declare the object with no processing
Processor::Processor() {
}

Processor::Processor(ParseXML *XML_interface)
    : XML(XML_interface) // TODO: using one global copy may have problems.
{

  /*
   *  placement and routing overhead is 10%, core scales worse than cache 40% is accumulated from 90 to 22nm
   *  There is no point to have heterogeneous memory controller on chip,
   || ()   *  thus McPAT only support homogeneous memory controllers.
  */
  int    i;
  double pppm_t[4] = {1, 1, 1, 1};
  set_proc_param();
  if(procdynp.homoCore)
    numCore = procdynp.numCore == 0 ? 0 : 1;
  else
    numCore = procdynp.numCore;

  if(procdynp.homoL2)
    numL2 = procdynp.numL2 == 0 ? 0 : 1;
  else
    numL2 = procdynp.numL2;

  if(procdynp.homoSTLB)
    numSTLB = procdynp.numSTLB == 0 ? 0 : 1;
  else
    numSTLB = procdynp.numSTLB;

  if(procdynp.homoL3)
    numL3 = procdynp.numL3 == 0 ? 0 : 1;
  else
    numL3 = procdynp.numL3;

  if(procdynp.homoNOC)
    numNOC = procdynp.numNOC == 0 ? 0 : 1;
  else
    numNOC = procdynp.numNOC;

  if(procdynp.homoL1Dir)
    numL1Dir = procdynp.numL1Dir == 0 ? 0 : 1;
  else
    numL1Dir = procdynp.numL1Dir;

  if(procdynp.homoL2Dir)
    numL2Dir = procdynp.numL2Dir == 0 ? 0 : 1;
  else
    numL2Dir = procdynp.numL2Dir;

  for(i = 0; i < numCore; i++) {
    cores.push_back(new Core(XML, i, &interface_ip));

    cores[i]->computeEnergy();

    cores[i]->computeEnergy(false);
    if(procdynp.homoCore) {
      for(int j = 1; j < procdynp.numCore; j++)
        cores.push_back(cores[i]);
      core.area.set_area(core.area.get_area() + cores[i]->area.get_area() * procdynp.numCore);
      set_pppm(pppm_t, cores[i]->clockRate * procdynp.numCore, procdynp.numCore, procdynp.numCore, procdynp.numCore);
      core.power = core.power + cores[i]->power * pppm_t;
      set_pppm(pppm_t, 1 / cores[i]->executionTime, procdynp.numCore, procdynp.numCore, procdynp.numCore);
      core.rt_power = core.rt_power + cores[i]->rt_power * pppm_t;
      area.set_area(area.get_area() + core.area.get_area()); // placement and routing overhead is 10%, core scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
      power    = power + core.power;
      rt_power = rt_power + core.rt_power;
    } else {
      core.area.set_area(core.area.get_area() + cores[i]->area.get_area());
      set_pppm(pppm_t, cores[i]->clockRate, 1, 1, 1);
      core.power = core.power + cores[i]->power * pppm_t;
      set_pppm(pppm_t, 1 / cores[i]->executionTime, 1, 1, 1);
      core.rt_power = core.rt_power + cores[i]->rt_power * pppm_t;
      area.set_area(area.get_area() + core.area.get_area()); // placement and routing overhead is 10%, core scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
      power    = power + core.power;
      rt_power = rt_power + core.rt_power;
    }
  }
#if 1
  if(procdynp.numGPU > 0) {
    gpu = new GPUU(XML, &interface_ip);
    gpu->computeEnergy();
    gpu->computeEnergy(false);
    set_pppm(pppm_t, gpu->clockRate, 1, 1, 1);
    gpu->power = gpu->power * pppm_t;
    set_pppm(pppm_t, 1 / gpu->executionTime, 1, 1, 1);
    gpu->rt_power = gpu->rt_power * pppm_t;
    area.set_area(area.get_area() + gpu->area.get_area()); // placement and routing overhead is 10%, gpu scales worse than cache 40%
                                                           // is accumulated from 90 to 22nm
    power    = power + gpu->power;
    rt_power = rt_power + gpu->rt_power;
  }
#endif
  if(numL2 > 0)
    for(i = 0; i < numL2; i++) {
      l2array.push_back(new SharedCache(XML, i, &interface_ip));
      if(XML->sys.L2[i].force_lkg == true) {
        l2array[i]->unicache.caches->local_result.power.readOp.leakage      = XML->sys.L2[i].force_lkg_w;
        l2array[i]->unicache.caches->local_result.power.readOp.gate_leakage = 0;

        l2array[i]->unicache.missb->local_result.power.readOp.leakage     = 0;
        l2array[i]->unicache.ifb->local_result.power.readOp.leakage       = 0;
        l2array[i]->unicache.prefetchb->local_result.power.readOp.leakage = 0;
        l2array[i]->unicache.wbb->local_result.power.readOp.leakage       = 0;
      }
      if(XML->sys.L2[i].force_rddyn == true) {
        l2array[i]->unicache.caches->local_result.power.readOp.dynamic = XML->sys.L2[i].force_rddyn_w;

        l2array[i]->unicache.missb->local_result.power.readOp.dynamic     = 0;
        l2array[i]->unicache.ifb->local_result.power.readOp.dynamic       = 0;
        l2array[i]->unicache.prefetchb->local_result.power.readOp.dynamic = 0;
        l2array[i]->unicache.wbb->local_result.power.readOp.dynamic       = 0;
      }
      if(XML->sys.L2[i].force_wrdyn == true) {
        l2array[i]->unicache.caches->local_result.power.writeOp.dynamic = XML->sys.L2[i].force_wrdyn_w;

        l2array[i]->unicache.missb->local_result.power.writeOp.dynamic     = 0;
        l2array[i]->unicache.ifb->local_result.power.writeOp.dynamic       = 0;
        l2array[i]->unicache.prefetchb->local_result.power.writeOp.dynamic = 0;
        l2array[i]->unicache.wbb->local_result.power.writeOp.dynamic       = 0;
      }

      l2array[i]->computeEnergy();
      l2array[i]->computeEnergy(false);

      if(procdynp.homoL2) {
        for(int j = 1; j < procdynp.numL2; j++)
          l2array.push_back(l2array[i]);
        l2.area.set_area(l2.area.get_area() + l2array[i]->area.get_area() * procdynp.numL2);
        set_pppm(pppm_t, l2array[i]->cachep.clockRate * procdynp.numL2, procdynp.numL2, procdynp.numL2, procdynp.numL2);
        l2.power = l2.power + l2array[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l2array[i]->cachep.executionTime, procdynp.numL2, procdynp.numL2, procdynp.numL2);
        l2.rt_power = l2.rt_power + l2array[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l2.area.get_area()); // placement and routing overhead is 10%, l2 scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
        power    = power + l2.power;
        rt_power = rt_power + l2.rt_power;
      } else {
        l2.area.set_area(l2.area.get_area() + l2array[i]->area.get_area());
        set_pppm(pppm_t, l2array[i]->cachep.clockRate, 1, 1, 1);
        l2.power = l2.power + l2array[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l2array[i]->cachep.executionTime, 1, 1, 1);
        l2.rt_power = l2.rt_power + l2array[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l2.area.get_area()); // placement and routing overhead is 10%, l2 scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
        power    = power + l2.power;
        rt_power = rt_power + l2.rt_power;
      }
    }

  if(numSTLB > 0)
    for(i = 0; i < numSTLB; i++) {
      stlbarray.push_back(new SharedCache(XML, i, &interface_ip, STLB));
      stlbarray[i]->computeEnergy();
      stlbarray[i]->computeEnergy(false);
      if(procdynp.homoSTLB) {
        for(int j = 1; j < procdynp.numSTLB; j++)
          stlbarray.push_back(stlbarray[i]);
        stlb.area.set_area(stlb.area.get_area() + stlbarray[i]->area.get_area() * procdynp.numSTLB);
        set_pppm(pppm_t, stlbarray[i]->cachep.clockRate * procdynp.numSTLB, procdynp.numSTLB, procdynp.numSTLB, procdynp.numSTLB);
        stlb.power = stlb.power + stlbarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / stlbarray[i]->cachep.executionTime, procdynp.numSTLB, procdynp.numSTLB, procdynp.numSTLB);
        stlb.rt_power = stlb.rt_power + stlbarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + stlb.area.get_area()); // placement and routing overhead is 10%, stlb scales worse than
                                                               // cache 40% is accumulated from 90 to 22nm
        power    = power + stlb.power;
        rt_power = rt_power + stlb.rt_power;

      } else {
        stlb.area.set_area(stlb.area.get_area() + stlbarray[i]->area.get_area());
        set_pppm(pppm_t, stlbarray[i]->cachep.clockRate, 1, 1, 1);
        stlb.power = stlb.power + stlbarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / stlbarray[i]->cachep.executionTime, 1, 1, 1);
        stlb.rt_power = stlb.rt_power + stlbarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + stlb.area.get_area()); // placement and routing overhead is 10%, stlb scales worse than
                                                               // cache 40% is accumulated from 90 to 22nm
        power    = power + stlb.power;
        rt_power = rt_power + stlb.rt_power;
      }
    }

  if(numL3 > 0)
    for(i = 0; i < numL3; i++) {
      l3array.push_back(new SharedCache(XML, i, &interface_ip, L3));
      if(XML->sys.L3[i].force_lkg == true) {
        l3array[i]->unicache.caches->local_result.power.readOp.leakage      = XML->sys.L3[i].force_lkg_w;
        l3array[i]->unicache.caches->local_result.power.readOp.gate_leakage = 0;

        l3array[i]->unicache.missb->local_result.power.readOp.leakage     = 0;
        l3array[i]->unicache.ifb->local_result.power.readOp.leakage       = 0;
        l3array[i]->unicache.prefetchb->local_result.power.readOp.leakage = 0;
        l3array[i]->unicache.wbb->local_result.power.readOp.leakage       = 0;
      }
      if(XML->sys.L3[i].force_rddyn == true) {
        l3array[i]->unicache.caches->local_result.power.readOp.dynamic = XML->sys.L3[i].force_rddyn_w;

        l3array[i]->unicache.missb->local_result.power.readOp.dynamic     = 0;
        l3array[i]->unicache.ifb->local_result.power.readOp.dynamic       = 0;
        l3array[i]->unicache.prefetchb->local_result.power.readOp.dynamic = 0;
        l3array[i]->unicache.wbb->local_result.power.readOp.dynamic       = 0;
      }
      if(XML->sys.L3[i].force_wrdyn == true) {
        l3array[i]->unicache.caches->local_result.power.writeOp.dynamic = XML->sys.L3[i].force_wrdyn_w;

        l3array[i]->unicache.missb->local_result.power.writeOp.dynamic     = 0;
        l3array[i]->unicache.ifb->local_result.power.writeOp.dynamic       = 0;
        l3array[i]->unicache.prefetchb->local_result.power.writeOp.dynamic = 0;
        l3array[i]->unicache.wbb->local_result.power.writeOp.dynamic       = 0;
      }

      l3array[i]->computeEnergy();
      l3array[i]->computeEnergy(false);
      if(procdynp.homoL3) {
        for(int j = 1; j < procdynp.numL3; j++)
          l3array.push_back(l3array[i]);
        l3.area.set_area(l3.area.get_area() + l3array[i]->area.get_area() * procdynp.numL3);
        set_pppm(pppm_t, l3array[i]->cachep.clockRate * procdynp.numL3, procdynp.numL3, procdynp.numL3, procdynp.numL3);
        l3.power = l3.power + l3array[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l3array[i]->cachep.executionTime, procdynp.numL3, procdynp.numL3, procdynp.numL3);
        l3.rt_power = l3.rt_power + l3array[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l3.area.get_area()); // placement and routing overhead is 10%, l3 scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
        power    = power + l3.power;
        rt_power = rt_power + l3.rt_power;

      } else {
        l3.area.set_area(l3.area.get_area() + l3array[i]->area.get_area());
        set_pppm(pppm_t, l3array[i]->cachep.clockRate, 1, 1, 1);
        l3.power = l3.power + l3array[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l3array[i]->cachep.executionTime, 1, 1, 1);
        l3.rt_power = l3.rt_power + l3array[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l3.area.get_area()); // placement and routing overhead is 10%, l3 scales worse than cache
                                                             // 40% is accumulated from 90 to 22nm
        power    = power + l3.power;
        rt_power = rt_power + l3.rt_power;
      }
    }

  if(numL1Dir > 0)
    for(i = 0; i < numL1Dir; i++) {
      l1dirarray.push_back(new SharedCache(XML, i, &interface_ip, L1Directory));
      l1dirarray[i]->computeEnergy();
      l1dirarray[i]->computeEnergy(false);
      if(procdynp.homoL1Dir) {
        for(int j = 1; j < procdynp.numL1Dir; j++)
          l1dirarray.push_back(l1dirarray[i]);
        l1dir.area.set_area(l1dir.area.get_area() + l1dirarray[i]->area.get_area() * procdynp.numL1Dir);
        set_pppm(pppm_t, l1dirarray[i]->cachep.clockRate * procdynp.numL1Dir, procdynp.numL1Dir, procdynp.numL1Dir,
                 procdynp.numL1Dir);
        l1dir.power = l1dir.power + l1dirarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l1dirarray[i]->cachep.executionTime, procdynp.numL1Dir, procdynp.numL1Dir, procdynp.numL1Dir);
        l1dir.rt_power = l1dir.rt_power + l1dirarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l1dir.area.get_area()); // placement and routing overhead is 10%, l1dir scales worse than
                                                                // cache 40% is accumulated from 90 to 22nm
        power    = power + l1dir.power;
        rt_power = rt_power + l1dir.rt_power;

      } else {
        l1dir.area.set_area(l1dir.area.get_area() + l1dirarray[i]->area.get_area());
        set_pppm(pppm_t, l1dirarray[i]->cachep.clockRate, 1, 1, 1);
        l1dir.power = l1dir.power + l1dirarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l1dirarray[i]->cachep.executionTime, 1, 1, 1);
        l1dir.rt_power = l1dir.rt_power + l1dirarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l1dir.area.get_area());
        power    = power + l1dir.power;
        rt_power = rt_power + l1dir.rt_power;
      }
    }

  if(numL2Dir > 0)
    for(i = 0; i < numL2Dir; i++) {
      l2dirarray.push_back(new SharedCache(XML, i, &interface_ip, L2Directory));
      l2dirarray[i]->computeEnergy();
      l2dirarray[i]->computeEnergy(false);
      if(procdynp.homoL2Dir) {
        for(int j = 1; j < procdynp.numL2Dir; j++)
          l2dirarray.push_back(l2dirarray[i]);
        l2dir.area.set_area(l2dir.area.get_area() + l2dirarray[i]->area.get_area() * procdynp.numL2Dir);
        set_pppm(pppm_t, l2dirarray[i]->cachep.clockRate * procdynp.numL2Dir, procdynp.numL2Dir, procdynp.numL2Dir,
                 procdynp.numL2Dir);
        l2dir.power = l2dir.power + l2dirarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l2dirarray[i]->cachep.executionTime, procdynp.numL2Dir, procdynp.numL2Dir, procdynp.numL2Dir);
        l2dir.rt_power = l2dir.rt_power + l2dirarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l2dir.area.get_area()); // placement and routing overhead is 10%, l2dir scales worse than
                                                                // cache 40% is accumulated from 90 to 22nm
        power    = power + l2dir.power;
        rt_power = rt_power + l2dir.rt_power;

      } else {
        l2dir.area.set_area(l2dir.area.get_area() + l2dirarray[i]->area.get_area());
        set_pppm(pppm_t, l2dirarray[i]->cachep.clockRate, 1, 1, 1);
        l2dir.power = l2dir.power + l2dirarray[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / l2dirarray[i]->cachep.executionTime, 1, 1, 1);
        l2dir.rt_power = l2dir.rt_power + l2dirarray[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + l2dir.area.get_area());
        power    = power + l2dir.power;
        rt_power = rt_power + l2dir.rt_power;
      }
    }

  if(XML->sys.mc.number_mcs > 0 && XML->sys.mc.memory_channels_per_mc > 0) {
    mc = new MemoryController(XML, &interface_ip);
    mc->computeEnergy();
    mc->computeEnergy(false);
    mcs.area.set_area(mcs.area.get_area() + mc->area.get_area() * XML->sys.mc.number_mcs);
    area.set_area(area.get_area() + mc->area.get_area() * XML->sys.mc.number_mcs);
    set_pppm(pppm_t, XML->sys.mc.number_mcs * mc->mcp.clockRate, XML->sys.mc.number_mcs, XML->sys.mc.number_mcs,
             XML->sys.mc.number_mcs);
    mcs.power = mc->power * pppm_t;
    power     = power + mcs.power;
    set_pppm(pppm_t, 1 / mc->mcp.executionTime, XML->sys.mc.number_mcs, XML->sys.mc.number_mcs, XML->sys.mc.number_mcs);
    mcs.rt_power = mc->rt_power * pppm_t;
    power        = power + mcs.power;
    rt_power     = rt_power + mcs.rt_power;
  }

  if(numNOC > 0)
    for(i = 0; i < numNOC; i++) {
      nocs.push_back(new NoC(XML, i, &interface_ip));
      nocs[i]->computeEnergy();
      nocs[i]->computeEnergy(false);
      if(procdynp.homoNOC) {
        for(int j = 1; j < procdynp.numNOC; j++)
          nocs.push_back(nocs[i]);
        noc.area.set_area(noc.area.get_area() + nocs[i]->area.get_area() * procdynp.numNOC);
        set_pppm(pppm_t, procdynp.numNOC * nocs[i]->nocdynp.clockRate, procdynp.numNOC, procdynp.numNOC, procdynp.numNOC);
        noc.power = noc.power + nocs[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / nocs[i]->nocdynp.executionTime, procdynp.numNOC, procdynp.numNOC, procdynp.numNOC);
        noc.rt_power = noc.rt_power + nocs[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + noc.area.get_area());
        power    = power + noc.power;
        rt_power = rt_power + noc.rt_power;
      } else {
        noc.area.set_area(noc.area.get_area() + nocs[i]->area.get_area());
        set_pppm(pppm_t, nocs[i]->nocdynp.clockRate, 1, 1, 1);
        noc.power = noc.power + nocs[i]->power * pppm_t;
        set_pppm(pppm_t, 1 / nocs[i]->nocdynp.executionTime, 1, 1, 1);
        noc.rt_power = noc.rt_power + nocs[i]->rt_power * pppm_t;
        area.set_area(area.get_area() + noc.area.get_area());
        power    = power + noc.power;
        rt_power = rt_power + noc.rt_power;
      }
    }
}

// by eka, all the calls after first call will use this function
// to avoid allocating a new object, the local results will be
// preserved from the last call.
void Processor::Processor2(ParseXML *XML_interface)
// TODO: using one global copy may have problems.
{
  /*
   *  placement and routing overhead is 10%, core scales worse than cache 40% is accumulated from 90 to 22nm
   *  There is no point to have heterogeneous memory controller on chip,
   *  thus McPAT only support homogeneous memory controllers.
   */
  int    i;
  double pppm_t[4] = {1, 1, 1, 1};
  XML              = XML_interface; // eka, to copy the input XML info to the local XML

  static bool first_time = true;
  if(first_time) {
    set_proc_param();
    first_time = false;
  }

  // if (procdynp.homoCore)
  //  numCore = procdynp.numCore==0? 0:1;
  // else
  numCore = procdynp.numCore;

  // if (procdynp.homoL2)
  //  numL2 = procdynp.numL2==0? 0:1;
  // else
  numL2 = procdynp.numL2;

  // if (procdynp.homoL3)
  //  numL3 = procdynp.numL3==0? 0:1;
  // else
  numL3 = procdynp.numL3;

  // if (procdynp.homoNOC)
  //	  numNOC = procdynp.numNOC==0? 0:1;
  // else
  numNOC = procdynp.numNOC;

  // if (procdynp.homoL1Dir)
  //  numL1Dir = procdynp.numL1Dir==0? 0:1;
  // else
  numL1Dir = procdynp.numL1Dir;

  // if (procdynp.homoL2Dir)
  //  numL2Dir = procdynp.numL2Dir==0? 0:1;
  // else
  numL2Dir = procdynp.numL2Dir;

  // eka, reset the power and area buffers to have a fresh buffer for the
  // new call.
  // Some of the reset calls are unnecessary now since they are called
  // inside the computeEnergy call for each block
  rt_power.reset();

  core.rt_power.reset();

  //
  for(i = 0; i < numCore; i++) {
    // cores[i]->power.reset();
    // cores[i]->rt_power.reset();
    // eka, update runtime parameters
    // cores.push_back(new Core(XML,i, &interface_ip));

    cores[i]->update_rtparam(XML, i, &interface_ip, cores[i]);

    cores[i]->computeEnergy(false);

    set_pppm(pppm_t, 1 / cores[i]->executionTime, 1, 1, 1);

    core.rt_power = core.rt_power + cores[i]->rt_power * pppm_t;

    dumpCoreDyn(i);
  }

  rt_power = rt_power + core.rt_power;

  if(procdynp.numGPU > 0) {
    gpu->rt_power.reset();

    gpu->executionTime = XML->sys.executionTime;

    for(int i = 0; i < procdynp.numSM; i++) {
      gpu->sms[i]->update_rtparam(XML, i);
      gpu->sms[i]->computeEnergy(false);
      gpu->rt_power = gpu->rt_power + gpu->sms[i]->rt_power;
      dumpGPUDyn(i);
    }

    // not dumped at the moment
    gpu->l2array->computeEnergy(false);
    gpu->rt_power = gpu->rt_power + gpu->l2array->power;
    dumpL2GDyn(); // dummy

    set_pppm(pppm_t, 1 / gpu->executionTime, 1, 1, 1);
    gpu->rt_power = gpu->rt_power * pppm_t;
    rt_power      = rt_power + gpu->rt_power;
  }

  if(XML->sys.mc.number_mcs > 0 && XML->sys.mc.memory_channels_per_mc > 0) {
    // eka, reset the power and area buffers to have a fresh buffer for the
    // new call.
    mcs.rt_power.reset();
    mcs.area.set_area(0);
    // eka, update runtime parameters
    // mc = new MemoryController(XML, &interface_ip);
    mc->update_rtparam(XML, i, &interface_ip, mc);
    mc->computeEnergy(false);
    set_pppm(pppm_t, 1 / mc->mcp.executionTime, XML->sys.mc.number_mcs, XML->sys.mc.number_mcs, XML->sys.mc.number_mcs);
    mcs.rt_power = mc->rt_power * pppm_t;
    rt_power     = rt_power + mcs.rt_power;
    dumpMCDyn();
  }

  if(numNOC > 0) {
    // eka, reset the power and area buffers to have a fresh buffer for the
    // new call.
    noc.rt_power.reset();
    for(i = 0; i < numNOC; i++) {
      nocs[i]->power.reset();
      nocs[i]->rt_power.reset();
      nocs[i]->update_rtparam(XML, i, &interface_ip);
      nocs[i]->computeEnergy(false);
      set_pppm(pppm_t, 1 / nocs[i]->executionTime, 1, 1, 1);
      noc.rt_power = noc.rt_power + nocs[i]->rt_power * pppm_t;
      dumpNoCDyn(i);
    }
    rt_power = rt_power + noc.rt_power;
  }

  if(numL3 > 0) {
    // eka, reset the power and area buffers to have a fresh buffer for the
    // new call.
    l3.rt_power.reset();
    for(i = 0; i < numL3; i++) {
      // l3array[i]->power.reset();
      // l3array[i]->rt_power.reset();
      // eka, update runtime parameters
      // l3array.push_back(new SharedCache(XML,i, &interface_ip, L3));
      l3array[i]->update_rtparam(XML, i, &interface_ip);
      l3array[i]->computeEnergy(false);
      set_pppm(pppm_t, 1 / l3array[i]->executionTime, 1, 1, 1);
      l3.rt_power = l3.rt_power + l3array[i]->rt_power * pppm_t;
      dumpL3Dyn(i);
    }
    rt_power = rt_power + l3.rt_power;
  }

  if(numL2 > 0) {
    // eka, reset the power and area buffers to have a fresh buffer for the
    // new call.
    l2.rt_power.reset();
    for(i = 0; i < numL2; i++) {
      l2array[i]->rt_power.reset();

      // eka, update runtime parameters
      // l2array.push_back(new SharedCache(XML,i, &interface_ip));
      l2array[i]->update_rtparam(XML, i, &interface_ip);
      //
      l2array[i]->computeEnergy(false);
      set_pppm(pppm_t, 1 / l2array[i]->executionTime, 1, 1, 1);
      l2.rt_power = l2.rt_power + l2array[i]->rt_power * pppm_t;
      dumpL2Dyn(i);
    }
    rt_power = rt_power + l2.rt_power;
  }

  if(numSTLB > 0) {
    // eka, reset the power and area buffers to have a fresh buffer for the
    // new call.
    stlb.rt_power.reset();
    for(i = 0; i < numSTLB; i++) {
      // stlbarray[i]->power.reset();
      // stlbarray[i]->rt_power.reset();
      // eka, update runtime parameters
      // stlbarray.push_back(new SharedCache(XML,i, &interface_ip, stlb));
      stlbarray[i]->update_rtparam(XML, i, &interface_ip);
      stlbarray[i]->computeEnergy(false);
      set_pppm(pppm_t, 1 / stlbarray[i]->executionTime, 1, 1, 1);
      stlb.rt_power = stlb.rt_power + stlbarray[i]->rt_power * pppm_t;
      // dumpSTLBDyn(i);
    }
    rt_power = rt_power + stlb.rt_power;
  }

  //  //clock power
  //  globalClock.init_wire_external(is_default, &interface_ip);
  //  globalClock.clk_area           =area*1e6; //change it from mm^2 to um^2
  //  globalClock.end_wiring_level   =5;//toplevel metal
  //  globalClock.start_wiring_level =5;//toplevel metal
  //  globalClock.l_ip.with_clock_grid=false;//global clock does not drive local final nodes
  //  globalClock.optimize_wire();
}
void Processor::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  int    i;
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {

    cout << "\nMcPAT 0.6 results (current print level is " << plevel
         << ", please increase print level to see the details in components): " << endl;
    cout << "*****************************************************************************************" << endl;
    cout << indent_str << "Technology " << XML->sys.core_tech_node << " nm" << endl;
    cout << indent_str << "Device Type= " << XML->sys.device_type << endl;
    cout << indent_str << "Interconnect metal projection= " << XML->sys.interconnect_projection_type << endl;
    cout << indent_str << "Core clock Rate(MHz) " << XML->sys.core[0].clock_rate << endl;
    cout << endl;
    cout << "*****************************************************************************************" << endl;
    cout << "Processor: " << endl;
    cout << indent_str << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str << "TDP Dynamic = " << power.readOp.dynamic << " W" << endl;
    cout << indent_str << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << indent_str << "Runtime Power = " << rt_power.readOp.dynamic << " W" << endl;
    cout << endl;
    cout << indent_str << "Total Cores: " << endl;
    cout << indent_str_next << "Area = " << core.area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << core.power.readOp.dynamic << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << core.power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << core.power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Power = " << core.rt_power.readOp.dynamic << " W" << endl;
    cout << endl;
    if(numL2 > 0) {
      cout << indent_str << "Total L2s: " << endl;
      cout << indent_str_next << "Area = " << l2.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << l2.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << l2.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << l2.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << l2.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(numSTLB > 0) {
      cout << indent_str << "Total stlbs: " << endl;
      cout << indent_str_next << "Area = " << stlb.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << stlb.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << stlb.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << stlb.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << stlb.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(numL3 > 0) {
      cout << indent_str << "Total L3s: " << endl;
      cout << indent_str_next << "Area = " << l3.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << l3.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << l3.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << l3.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << l3.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(numL1Dir > 0) {
      cout << indent_str << "Total First Level Directory: " << endl;
      cout << indent_str_next << "Area = " << l1dir.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << l1dir.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << l1dir.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << l1dir.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << l1dir.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(numL2Dir > 0) {
      cout << indent_str << "Total First Level Directory: " << endl;
      cout << indent_str_next << "Area = " << l2dir.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << l2dir.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << l2dir.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << l2dir.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << l2dir.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(numNOC > 0) {
      cout << indent_str << "Total NoCs: " << endl;
      cout << indent_str_next << "Area = " << noc.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << noc.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << noc.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << noc.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << noc.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    if(XML->sys.mc.number_mcs > 0 && XML->sys.mc.memory_channels_per_mc > 0) {
      cout << indent_str << "Total MCs: " << endl;
      cout << indent_str_next << "Area = " << mcs.area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << mcs.power.readOp.dynamic << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << mcs.power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << mcs.power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Power = " << mcs.rt_power.readOp.dynamic << " W" << endl;
      cout << endl;
    }
    cout << "*****************************************************************************************" << endl;
    if(plevel > 1) {
      for(i = 0; i < numCore; i++) {
        cores[i]->displayEnergy(indent + 4, plevel, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      for(i = 0; i < numL2; i++) {
        l2array[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      for(i = 0; i < numSTLB; i++) {
        stlbarray[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      for(i = 0; i < numL3; i++) {
        l3array[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      for(i = 0; i < numL1Dir; i++) {
        l1dirarray[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      for(i = 0; i < numL2Dir; i++) {
        l2dirarray[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
      mc->displayEnergy(indent + 4, is_tdp);
      cout << "*****************************************************************************************" << endl;

      for(i = 0; i < numNOC; i++) {
        nocs[i]->displayEnergy(indent + 4, is_tdp);
        cout << "*****************************************************************************************" << endl;
      }
    }
  } else {
  }
}

void Processor::set_proc_param() {
  procdynp.freq      = XML->sys.target_core_clockrate;
  bool debug         = false;
  coreIndex          = XML->coreIndex;
  gpuIndex           = XML->gpuIndex;
  procdynp.homoCore  = bool(debug ? 1 : XML->sys.homogeneous_cores);
  procdynp.homoL2    = bool(debug ? 1 : XML->sys.homogeneous_L2s);
  procdynp.homoSTLB  = bool(debug ? 1 : XML->sys.homogeneous_STLBs);
  procdynp.homoL3    = bool(debug ? 1 : XML->sys.homogeneous_L3s);
  procdynp.homoNOC   = bool(debug ? 1 : XML->sys.homogeneous_NoCs);
  procdynp.homoL1Dir = bool(debug ? 1 : XML->sys.homogeneous_L1Directories);
  procdynp.homoL2Dir = bool(debug ? 1 : XML->sys.homogeneous_L2Directories);

  procdynp.numCore  = XML->sys.number_of_cores;
  procdynp.numGPU   = XML->sys.number_of_GPU;
  procdynp.numSM    = XML->sys.gpu.number_of_SMs;
  procdynp.numLane  = XML->sys.gpu.homoSM.number_of_lanes;
  procdynp.numL2    = XML->sys.number_of_L2s;
  procdynp.numSTLB  = XML->sys.number_of_STLBs;
  procdynp.numL3    = XML->sys.number_of_L3s;
  procdynp.numNOC   = XML->sys.number_of_NoCs;
  procdynp.numL1Dir = XML->sys.number_of_L1Directories;
  procdynp.numL2Dir = XML->sys.number_of_L2Directories;

  if(procdynp.numGPU == 0) {
    procdynp.numSM   = 0;
    procdynp.numLane = 0;
  }

  POW_SCALE_DYN = XML->sys.scale_dyn;
  POW_SCALE_LKG = XML->sys.scale_lkg;
  if(procdynp.numCore < 1) {
    cout << " The target processor should at least have one core on chip." << endl;
    exit(0);
  }

  //  if (numNOCs<0 || numNOCs>2)
  //    {
  //  	  cout <<"number of NOCs must be 1 (only global NOCs) or 2 (both global and local NOCs)"<<endl;
  //  	  exit(0);
  //    }

  /* Basic parameters*/
  interface_ip.data_arr_ram_cell_tech_type    = debug ? 0 : XML->sys.device_type;
  interface_ip.data_arr_peri_global_tech_type = debug ? 0 : XML->sys.device_type;
  interface_ip.tag_arr_ram_cell_tech_type     = debug ? 0 : XML->sys.device_type;
  interface_ip.tag_arr_peri_global_tech_type  = debug ? 0 : XML->sys.device_type;

  interface_ip.ic_proj_type     = debug ? 0 : XML->sys.interconnect_projection_type;
  interface_ip.wire_is_mat_type = 2;
  interface_ip.wire_os_mat_type = 2;
  interface_ip.delay_wt         = 100; // Fixed number, make sure timing can be satisfied.
  interface_ip.area_wt          = 0;   // Fixed number, This is used to exhaustive search for individual components.
  interface_ip.dynamic_power_wt = 100; // Fixed number, This is used to exhaustive search for individual components.
  interface_ip.leakage_power_wt = 0;
  interface_ip.cycle_time_wt    = 0;

  interface_ip.delay_dev         = 10000; // Fixed number, make sure timing can be satisfied.
  interface_ip.area_dev          = 10000; // Fixed number, This is used to exhaustive search for individual components.
  interface_ip.dynamic_power_dev = 10;    // Fixed number, This is used to exhaustive search for individual components.
  interface_ip.leakage_power_dev = 10000;
  interface_ip.cycle_time_dev    = 10000;

  interface_ip.ed             = 2;
  interface_ip.burst_len      = 1; // parameters are fixed for processor section, since memory is processed separately
  interface_ip.int_prefetch_w = 1;
  interface_ip.page_sz_bits   = 0;
  interface_ip.temp           = debug ? 360 : XML->sys.temperature;
  interface_ip.F_sz_nm        = debug ? 90 : XML->sys.core_tech_node; // XML->sys.core_tech_node;
  interface_ip.F_sz_um        = interface_ip.F_sz_nm / 1000;

  //***********This section of code does not have real meaning, they are just to ensure all data will have initial value to prevent
  // errors. They will be overridden  during each components initialization
  interface_ip.cache_sz     = 64;
  interface_ip.line_sz      = 1;
  interface_ip.assoc        = 1;
  interface_ip.nbanks       = 1;
  interface_ip.out_w        = interface_ip.line_sz * 8;
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = 64;
  interface_ip.access_mode  = 2;

  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;

  interface_ip.is_main_mem                        = false;
  interface_ip.rpters_in_htree                    = true;
  interface_ip.ver_htree_wires_over_array         = 0;
  interface_ip.broadcast_addr_din_over_ver_htrees = 0;

  interface_ip.num_rw_ports       = 1;
  interface_ip.num_rd_ports       = 0;
  interface_ip.num_wr_ports       = 0;
  interface_ip.num_se_rd_ports    = 0;
  interface_ip.num_search_ports   = 1;
  interface_ip.nuca               = 0;
  interface_ip.nuca_bank_count    = 0;
  interface_ip.is_cache           = true;
  interface_ip.pure_ram           = false;
  interface_ip.pure_cam           = false;
  interface_ip.force_cache_config = false;
  interface_ip.wt                 = Global;
  interface_ip.force_wiretype     = false;
  interface_ip.print_detail       = 1;
  interface_ip.add_ecc_b_         = true;
}

void Processor::DeleteProcessor() {
  while(!cores.empty()) {
    delete cores.back();
    cores.pop_back();
  }
  while(!l2array.empty()) {
    delete l2array.back();
    l2array.pop_back();
  }
  while(!stlbarray.empty()) {
    delete stlbarray.back();
    stlbarray.pop_back();
  }
  while(!l3array.empty()) {
    delete l3array.back();
    l3array.pop_back();
  }
  while(!nocs.empty()) {
    delete nocs.back();
    nocs.pop_back();
  }
  if(mc != 0) {
    delete mc;
  }
};

Processor::~Processor() {
  while(!cores.empty()) {
    delete cores.back();
    cores.pop_back();
  }
  while(!l2array.empty()) {
    delete l2array.back();
    l2array.pop_back();
  }
  while(!stlbarray.empty()) {
    delete stlbarray.back();
    stlbarray.pop_back();
  }
  while(!l3array.empty()) {
    delete l3array.back();
    l3array.pop_back();
  }
  while(!nocs.empty()) {
    delete nocs.back();
    nocs.pop_back();
  }
  if(mc != 0) {
    delete mc;
  }
};

void Processor::dumpStatics(ChipEnergyBundle *eBundle) {

  energyBundle = eBundle; // set the processor pointer

  char name[244];
  char conn[244];
  int  corePerL2 = ceil(procdynp.numCore / procdynp.numL2);
  // int L2PerL3 = ceil(procdynp.numL2/procdynp.numL3);

  float area, tdp, lkg;
  int   type  = 0;
  bool  isGPU = true;

  for(int ii = 0; ii < (procdynp.numCore); ii++) {

    // allocate a Container
    // set name
    // push the container to energyBundle

    // get the container based on the name
    // set conn
    // set area
    // set tdp
    // set lkg

    int i = procdynp.homoCore ? 0 : ii;

    double pipeline = cores[i]->corepipe->area.get_area() / XML->sys.core[0].pipeline_depth[0];

    //#define FINE_GRAINED 1

    // Renaming Unit

#ifdef FINE_GRAINED

    float areaiFRAT  = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->iFRAT->area.get_area());
    float areaiRRAT  = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->iRRAT->area.get_area());
    float areafFRAT  = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->fFRAT->area.get_area());
    float areafRRAT  = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->fRRAT->area.get_area());
    float areaifreeL = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->ifreeL->area.get_area());
    float areaffreeL = 1.25e-12 * (2 * pipeline / 6.0 + cores[i]->rnu->ffreeL->area.get_area());

    float tdpiFRAT  = cores[i]->rnu->iFRAT->power.readOp.dynamic;
    float tdpiRRAT  = cores[i]->rnu->iRRAT->power.readOp.dynamic;
    float tdpfFRAT  = cores[i]->rnu->fFRAT->power.readOp.dynamic;
    float tdpfRRAT  = cores[i]->rnu->fRRAT->power.readOp.dynamic;
    float tdpifreeL = cores[i]->rnu->ifreeL->power.readOp.dynamic;
    float tdpffreeL = cores[i]->rnu->ffreeL->power.readOp.dynamic;

    float lkgiFRAT  = cores[i]->rnu->iFRAT->rt_power.readOp.get_leak();
    float lkgiRRAT  = cores[i]->rnu->iRRAT->rt_power.readOp.get_leak();
    float lkgfFRAT  = cores[i]->rnu->fFRAT->rt_power.readOp.get_leak();
    float lkgfRRAT  = cores[i]->rnu->fRRAT->rt_power.readOp.get_leak();
    float lkgifreeL = cores[i]->rnu->ifreeL->rt_power.readOp.get_leak();
    float lkgffreeL = cores[i]->rnu->ffreeL->rt_power.readOp.get_leak();

    type = 0;

    sprintf(name, "P(%d)_iFRAT", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaiFRAT, tdpiFRAT, 0.0, lkgiFRAT));

    sprintf(name, "P(%d)_iRRAT", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaiRRAT, tdpiRRAT, 0.0, lkgiRRAT));

    sprintf(name, "P(%d)_fFRAT", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areafFRAT, tdpfFRAT, 0.0, lkgfFRAT));

    sprintf(name, "P(%d)_fRRAT", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areafRRAT, tdpfRRAT, 0.0, lkgfRRAT));

    sprintf(name, "P(%d)_iFreeL", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaifreeL, tdpifreeL, 0.0, lkgifreeL));

    sprintf(name, "P(%d)_fFreeL", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaffreeL, tdpffreeL, 0.0, lkgffreeL));

#else

    if(cores[i]->coredynp.core_ty == OOO) {

      sprintf(name, "P(%d)_RNU", ii);
      sprintf(conn, "P(%d)_fetch", ii);
      area = 2e-12 * (2 * pipeline + cores[i]->rnu->iFRAT->area.get_area() + cores[i]->rnu->iRRAT->area.get_area() +
                      cores[i]->rnu->ifreeL->area.get_area() + cores[i]->rnu->fFRAT->area.get_area() +
                      cores[i]->rnu->fRRAT->area.get_area() + cores[i]->rnu->ffreeL->area.get_area());
      tdp  = (cores[i]->rnu->iFRAT->power.readOp.dynamic + cores[i]->rnu->iRRAT->power.readOp.dynamic +
             cores[i]->rnu->ifreeL->power.readOp.dynamic + cores[i]->rnu->fFRAT->power.readOp.dynamic +
             cores[i]->rnu->fRRAT->power.readOp.dynamic + cores[i]->rnu->ffreeL->power.readOp.dynamic);
      lkg  = cores[i]->rnu->iFRAT->rt_power.readOp.get_leak() + cores[i]->rnu->iRRAT->rt_power.readOp.get_leak() +
            cores[i]->rnu->ifreeL->rt_power.readOp.get_leak() + cores[i]->rnu->fFRAT->rt_power.readOp.get_leak() +
            cores[i]->rnu->fRRAT->rt_power.readOp.get_leak() + cores[i]->rnu->ffreeL->rt_power.readOp.get_leak();
      type = 0;
      // FIXME: decide on the deviceType
      energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
      // Visibility of the Container
    }

#endif

#ifdef FINE_GRAINED
    float areaGlobalBPT = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->BPT->globalBPT->area.get_area());
    float arealocalBPT  = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->BPT->L1_localBPT->area.get_area());
    float areaChooser   = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->BPT->chooser->area.get_area());
    float areaRAS       = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->BPT->RAS->area.get_area());
    float areaBTB       = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->BTB->area.get_area());
    float areaIB        = 1.25e-12 * (pipeline / 6.0 + cores[i]->ifu->IB->area.get_area());

    float tdpGlobalBPT = cores[i]->ifu->BPT->globalBPT->power.readOp.dynamic;
    float tdplocalBPT  = cores[i]->ifu->BPT->L1_localBPT->power.readOp.dynamic;
    float tdpChooser   = cores[i]->ifu->BPT->chooser->power.readOp.dynamic;
    float tdpRAS       = cores[i]->ifu->BPT->RAS->power.readOp.dynamic;
    float tdpBTB       = cores[i]->ifu->BTB->power.readOp.dynamic;
    float tdpIB        = cores[i]->ifu->IB->power.readOp.dynamic;

    float lkgGlobalBPT = cores[i]->ifu->BPT->globalBPT->rt_power.readOp.get_leak();
    float lkglocalBPT  = cores[i]->ifu->BPT->L1_localBPT->rt_power.readOp.get_leak();
    float lkgChooser   = cores[i]->ifu->BPT->chooser->rt_power.readOp.get_leak();
    float lkgRAS       = cores[i]->ifu->BPT->RAS->rt_power.readOp.get_leak();
    float lkgBTB       = cores[i]->ifu->BTB->rt_power.readOp.get_leak();
    float lkgIB        = cores[i]->ifu->IB->rt_power.readOp.get_leak();

    type = 0;

    sprintf(name, "P(%d)_globalBPT", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaGlobalBPT, tdpGlobalBPT, 0.0, lkgGlobalBPT));

    sprintf(name, "P(%d)_localBPT", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, arealocalBPT, tdplocalBPT, 0.0, lkglocalBPT));

    sprintf(name, "P(%d)_Chooser", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaChooser, tdpChooser, 0.0, lkgChooser));

    sprintf(name, "P(%d)_RAS", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaRAS, tdpRAS, 0.0, lkgRAS));

    sprintf(name, "P(%d)_BTB", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaBTB, tdpBTB, 0.0, lkgBTB));

    sprintf(name, "P(%d)_IB", ii);
    sprintf(conn, "P(%d)_icache", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaIB, tdpIB, 0.0, lkgIB));

#else

    // Instruction Fetch Unit
    sprintf(name, "P(%d)_fetch", ii);

    area = 1.25e-12 * (pipeline + cores[i]->ifu->IB->area.get_area());
    tdp  = cores[i]->ifu->IB->power.readOp.dynamic;
    lkg  = cores[i]->ifu->IB->rt_power.readOp.get_leak();

    if(cores[i]->coredynp.predictionW > 0) {
      if(cores[i]->coredynp.core_ty == OOO) {
        sprintf(conn, "P(%d)_icache P(%d)_RNU", ii, ii);

      } else {
        sprintf(conn, "P(%d)_icache", ii);
      }

      area += 1.25e-12 * (cores[i]->ifu->BPT->globalBPT->area.get_area() + cores[i]->ifu->BPT->L1_localBPT->area.get_area() +
                          cores[i]->ifu->BPT->chooser->area.get_area() + cores[i]->ifu->BPT->RAS->area.get_area() +
                          cores[i]->ifu->BTB->area.get_area());

      tdp += cores[i]->ifu->BPT->globalBPT->power.readOp.dynamic + cores[i]->ifu->BPT->L1_localBPT->power.readOp.dynamic +
             cores[i]->ifu->BPT->chooser->power.readOp.dynamic + cores[i]->ifu->BPT->RAS->power.readOp.dynamic +
             cores[i]->ifu->BTB->power.readOp.dynamic;

      lkg += cores[i]->ifu->BPT->globalBPT->rt_power.readOp.get_leak() +
             cores[i]->ifu->BPT->L1_localBPT->rt_power.readOp.get_leak() + cores[i]->ifu->BPT->chooser->rt_power.readOp.get_leak() +
             cores[i]->ifu->BPT->RAS->rt_power.readOp.get_leak() + cores[i]->ifu->BTB->rt_power.readOp.get_leak();

    } else {
      sprintf(conn, "P(%d)_icache", ii);
    }

    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
    //}
#endif

#ifdef FINE_GRAINED
    float areaitlb     = 1.25e-12 * (pipeline / 2.0 + cores[i]->mmu->itlb->area.get_area());
    float areaicache   = 1.25e-12 * (pipeline / 2.0 + cores[i]->ifu->icache.area.get_area());
    float areaicachecc = 1.25e-12 * (pipeline / 2.0 + cores[i]->ifu->icachecc.area.get_area());

    float tdpitlb     = cores[i]->mmu->itlb->power.readOp.dynamic;
    float tdpicache   = cores[i]->ifu->icache.power.readOp.dynamic;
    float tdpicachecc = cores[i]->ifu->icachecc.power.readOp.dynamic;

    float lkgitlb     = cores[i]->mmu->itlb->rt_power.readOp.get_leak();
    float lkgicache   = cores[i]->ifu->icache.rt_power.readOp.get_leak();
    float lkgicachecc = cores[i]->ifu->icachecc.rt_power.readOp.get_leak();

    type = 0;

    sprintf(name, "P(%d)_itlb", ii);
    sprintf(conn, "L2(%d) P(%d)_icache", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaitlb, tdpitlb, 0.0, lkgitlb));

    sprintf(name, "P(%d)_icache", ii);
    sprintf(conn, "L2(%d) P(%d)_itlb", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaicache, tdpicache, 0.0, lkgicache));

    sprintf(name, "P(%d)_icachecc", ii);
    sprintf(conn, "L2(%d) P(%d)_itlb", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areaicachecc, tdpicachecc, 0.0, lkgicachecc));
#else

    sprintf(name, "P(%d)_icache", ii);
    sprintf(conn, "L2(%d) P(%d)_fetch", int(ii / corePerL2), ii);
    area = 1.25e-12 * (pipeline + cores[i]->mmu->itlb->area.get_area() + cores[i]->ifu->icache.area.get_area() +
                       cores[i]->ifu->icachecc.area.get_area());
    tdp  = (cores[i]->mmu->itlb->power.readOp.dynamic + cores[i]->ifu->icache.power.readOp.dynamic +
           cores[i]->ifu->icachecc.power.readOp.dynamic);
    lkg  = cores[i]->mmu->itlb->rt_power.readOp.get_leak() + cores[i]->ifu->icache.rt_power.readOp.get_leak() +
          cores[i]->ifu->icachecc.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
#endif

#ifdef FINE_GRAINED
    float areadtlb     = 1.25e-12 * (pipeline / 2.0 + cores[i]->mmu->dtlb->area.get_area());
    float areadcache   = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->dcache.area.get_area());
    float areadcachecc = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->dcachecc.area.get_area());

    float tdpdtlb     = cores[i]->mmu->dtlb->power.readOp.dynamic;
    float tdpdcache   = cores[i]->lsu->dcache.power.readOp.dynamic;
    float tdpdcachecc = cores[i]->lsu->dcachecc.power.readOp.dynamic;

    float lkgdtlb     = cores[i]->mmu->dtlb->rt_power.readOp.get_leak();
    float lkgdcache   = cores[i]->lsu->dcache.rt_power.readOp.get_leak();
    float lkgdcachecc = cores[i]->lsu->dcachecc.rt_power.readOp.get_leak();

    type = 0;

    sprintf(name, "P(%d)_dtlb", ii);
    sprintf(conn, "L2(%d) P(%d)_dcache", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areadtlb, tdpdtlb, 0.0, lkgdtlb));

    sprintf(name, "P(%d)_dcache", ii);
    sprintf(conn, "L2(%d) P(%d)_dtlb", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areadcache, tdpdcache, 0.0, lkgdcache));

    sprintf(name, "P(%d)_dcachecc", ii);
    sprintf(conn, "L2(%d) P(%d)_dtlb", int(ii / corePerL2), ii);
    energyBundle->cntrs.push_back(
        Container((const char *)name, (char *)conn, (char *)conn, type, areadcachecc, tdpdcachecc, 0.0, lkgdcachecc));
#else

    sprintf(name, "P(%d)_dcache", ii);
    sprintf(conn, "L2(%d) P(%d)_LSU", int(ii / corePerL2), ii);
    area = 1.25e-12 * (pipeline + cores[i]->mmu->dtlb->area.get_area() + cores[i]->lsu->dcache.area.get_area() +
                       cores[i]->lsu->dcachecc.area.get_area());
    tdp  = (cores[i]->mmu->dtlb->power.readOp.dynamic + cores[i]->lsu->dcache.power.readOp.dynamic +
           cores[i]->lsu->dcachecc.power.readOp.dynamic);
    lkg  = cores[i]->mmu->dtlb->rt_power.readOp.get_leak() + cores[i]->lsu->dcache.rt_power.readOp.get_leak() +
          cores[i]->lsu->dcachecc.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
#endif

#ifdef FINE_GRAINED
    if(XML->sys.core[i].scoore) {
      float areaVPCfilter  = 1.25e-12 * (pipeline / 2.0 + cores[i]->lsu->VPCfilter.area.get_area());
      float areavpc_buffer = 1.25e-12 * (pipeline / 2.0 + cores[i]->lsu->vpc_buffer->area.get_area());

      float tdpVPCfilter  = cores[i]->lsu->VPCfilter.power.readOp.dynamic;
      float tdpvpc_buffer = cores[i]->lsu->vpc_buffer->power.readOp.dynamic;

      float lkgVPCfilter  = cores[i]->lsu->VPCfilter.rt_power.readOp.get_leak();
      float lkgvpc_buffer = cores[i]->lsu->vpc_buffer->rt_power.readOp.get_leak();

      type = 0;
      sprintf(name, "P(%d)_VPCfilter", ii);
      sprintf(conn, "P(%d)_dcache", ii);
      energyBundle->cntrs.push_back(
          Container((const char *)name, (char *)conn, (char *)conn, type, areaVPCfilter, tdpVPCfilter, 0.0, lkgVPCfilter));

      sprintf(name, "P(%d)_vpc_buffer", ii);
      sprintf(conn, "P(%d)_dcache", ii);
      energyBundle->cntrs.push_back(
          Container((const char *)name, (char *)conn, (char *)conn, type, areavpc_buffer, tdpvpc_buffer, 0.0, lkgvpc_buffer));
    } else {
      if(XML->sys.core[i].machine_type == 0) { // OoO

        float areaLSQ   = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->LSQ->area.get_area());
        float areaLoadQ = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->LoadQ->area.get_area());
        float areassit  = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->ssit->area.get_area());
        float arealfst  = 1.25e-12 * (pipeline / 4.0 + cores[i]->lsu->lfst->area.get_area());

        float tdpLSQ   = cores[i]->lsu->LSQ->power.readOp.dynamic;
        float tdpLoadQ = cores[i]->lsu->LoadQ->power.readOp.dynamic;
        float tdpssit  = cores[i]->lsu->ssit->power.readOp.dynamic;
        float tdplfst  = cores[i]->lsu->lfst->power.readOp.dynamic;

        float lkgLSQ   = cores[i]->lsu->LSQ->rt_power.readOp.get_leak();
        float lkgLoadQ = cores[i]->lsu->LoadQ->rt_power.readOp.get_leak();
        float lkgssit  = cores[i]->lsu->ssit->rt_power.readOp.get_leak();
        float lkglfst  = cores[i]->lsu->lfst->rt_power.readOp.get_leak();

        sprintf(name, "P(%d)_LSQ", ii);
        sprintf(conn, "P(%d)_dcache", ii);
        energyBundle->cntrs.push_back(
            Container((const char *)name, (char *)conn, (char *)conn, type, areaLSQ, tdpLSQ, 0.0, lkgLSQ));

        sprintf(name, "P(%d)_LoadQ", ii);
        sprintf(conn, "P(%d)_dcache", ii);
        energyBundle->cntrs.push_back(
            Container((const char *)name, (char *)conn, (char *)conn, type, areaLoadQ, tdpLoadQ, 0.0, lkgLoadQ));

        sprintf(name, "P(%d)_ssit", ii);
        sprintf(conn, "P(%d)_dcache", ii);
        energyBundle->cntrs.push_back(
            Container((const char *)name, (char *)conn, (char *)conn, type, areassit, tdpssit, 0.0, lkgssit));

        sprintf(name, "P(%d)_lfst", ii);
        sprintf(conn, "P(%d)_dcache", ii);
        energyBundle->cntrs.push_back(
            Container((const char *)name, (char *)conn, (char *)conn, type, arealfst, tdplfst, 0.0, lkglfst));
      } else {
        float areaLSQ = 1.25e-12 * (pipeline + cores[i]->lsu->LSQ->area.get_area());
        float tdpLSQ  = cores[i]->lsu->LSQ->power.readOp.dynamic;
        float lkgLSQ  = cores[i]->lsu->LSQ->rt_power.readOp.get_leak();

        sprintf(name, "P(%d)_LSQ", ii);
        sprintf(conn, "P(%d)_dcache", ii);
        energyBundle->cntrs.push_back(
            Container((const char *)name, (char *)conn, (char *)conn, type, areaLSQ, tdpLSQ, 0.0, lkgLSQ));
      }
    }
#else

    sprintf(name, "P(%d)_LSU", ii);
    sprintf(conn, "P(%d)_dcache", ii);
    if(XML->sys.core[i].scoore) {

      area = 1.25e-12 * (pipeline + cores[i]->lsu->VPCfilter.area.get_area() + cores[i]->lsu->vpc_buffer->area.get_area()); // +
      // cores[i]->lsu->strb->area.get_area()         +
      // cores[i]->lsu->ldrb->area.get_area())
      tdp = (cores[i]->lsu->VPCfilter.power.readOp.dynamic + cores[i]->lsu->vpc_buffer->power.readOp.dynamic); // +
      // cores[i]->lsu->strb->power.readOp.dynamic       +
      // cores[i]->lsu->ldrb->power.readOp.dynamic));
      lkg = cores[i]->lsu->VPCfilter.rt_power.readOp.get_leak() + cores[i]->lsu->vpc_buffer->rt_power.readOp.get_leak(); // +
      // cores[i]->lsu->strb->rt_power.readOp.get_leak()       +
      // cores[i]->lsu->ldrb->rt_power.readOp.get_leak();

    } else {

      if(XML->sys.core[i].machine_type == 0) { // OoO

        area = 1.25e-12 * (pipeline + cores[i]->lsu->LSQ->area.get_area() + cores[i]->lsu->LoadQ->area.get_area() +
                           cores[i]->lsu->ssit->area.get_area() + cores[i]->lsu->lfst->area.get_area());
        tdp  = (cores[i]->lsu->LSQ->power.readOp.dynamic + cores[i]->lsu->LoadQ->power.readOp.dynamic +
               cores[i]->lsu->ssit->power.readOp.dynamic + cores[i]->lsu->lfst->power.readOp.dynamic);
        lkg  = cores[i]->lsu->LSQ->rt_power.readOp.get_leak() + cores[i]->lsu->LoadQ->rt_power.readOp.get_leak() +
              cores[i]->lsu->ssit->rt_power.readOp.get_leak() + cores[i]->lsu->lfst->rt_power.readOp.get_leak();

      } else {

        area = 1.25e-12 * (pipeline + cores[i]->lsu->LSQ->area.get_area());
        tdp  = (cores[i]->lsu->LSQ->power.readOp.dynamic);
        lkg  = cores[i]->lsu->LSQ->rt_power.readOp.get_leak();
      }
    }
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    type = 0;
    area = tdp = lkg = 0;

#endif

#ifdef FINE_GRAINED
    float areaexeu            = 1.5e-12 * (pipeline + cores[i]->exu->exeu->area.get_area());
    float areafp_u            = 1.5e-12 * (pipeline + cores[i]->exu->fp_u->area.get_area());
    float areaint_inst_window = 1.5e-12 * (pipeline + cores[i]->exu->scheu->int_inst_window->area.get_area());
    float areafp_inst_window  = 1.5e-12 * (pipeline + cores[i]->exu->scheu->fp_inst_window->area.get_area());

    float tdpexeu            = cores[i]->exu->exeu->power.readOp.dynamic;
    float tdpfp_u            = cores[i]->exu->fp_u->power.readOp.dynamic;
    float tdpint_inst_window = cores[i]->exu->scheu->int_inst_window->power.readOp.dynamic;
    float tdpfp_inst_window  = cores[i]->exu->scheu->fp_inst_window->power.readOp.dynamic;

    float lkgexeu            = cores[i]->exu->exeu->rt_power.readOp.get_leak();
    float lkgfp_u            = cores[i]->exu->fp_u->rt_power.readOp.get_leak();
    float lkgint_inst_window = cores[i]->exu->scheu->int_inst_window->rt_power.readOp.get_leak();
    float lkgfp_inst_window  = cores[i]->exu->scheu->fp_inst_window->rt_power.readOp.get_leak();

    type = 0;
    sprintf(name, "P(%d)_exeu", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaexeu, tdpexeu, 0.0, lkgexeu));

    sprintf(name, "P(%d)_fp_u", ii);
    sprintf(conn, "P(%d)_FRF", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areafp_u, tdpfp_u, 0.0, lkgfp_u));

    sprintf(name, "P(%d)_int_inst_window", ii);
    sprintf(conn, "P(%d)_IRF", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaint_inst_window,
                                            tdpint_inst_window, 0.0, lkgint_inst_window));

    sprintf(name, "P(%d)_fp_inst_window", ii);
    sprintf(conn, "P(%d)_FRF", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areafp_inst_window,
                                            tdpfp_inst_window, 0.0, lkgfp_inst_window));
#else

    sprintf(name, "P(%d)_EXE", ii);
    sprintf(conn, "P(%d)_RF", ii);
    area = 1.5e-12 * (pipeline + cores[i]->exu->exeu->area.get_area() + pipeline + cores[i]->exu->fp_u->area.get_area());

    tdp = cores[i]->exu->exeu->power.readOp.dynamic + cores[i]->exu->fp_u->power.readOp.dynamic;

    lkg = cores[i]->exu->exeu->rt_power.readOp.get_leak() + cores[i]->exu->fp_u->rt_power.readOp.get_leak();

    if(cores[i]->coredynp.core_ty == OOO) {
      area += 1.5e-12 * (pipeline + cores[i]->exu->scheu->fp_inst_window->area.get_area());
      tdp += cores[i]->exu->scheu->fp_inst_window->power.readOp.dynamic;
      lkg += cores[i]->exu->scheu->fp_inst_window->rt_power.readOp.get_leak();
    }

    if(cores[i]->coredynp.multithreaded) {
      area += 1.5e-12 * (pipeline + cores[i]->exu->scheu->int_inst_window->area.get_area());
      tdp += cores[i]->exu->scheu->int_inst_window->power.readOp.dynamic;
      lkg += cores[i]->exu->scheu->int_inst_window->rt_power.readOp.get_leak();
    }

    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;

#endif

    idx_iRF.push_back(energyBundle->cntrs.size());

#ifdef FINE_GRAINED

    float areaIRF = 1.5e-12 * (pipeline + cores[i]->exu->rfu->IRF->area.get_area());
    float areaFRF = 1.5e-12 * (pipeline + cores[i]->exu->rfu->FRF->area.get_area());

    float tdpIRF = cores[i]->exu->rfu->IRF->power.readOp.dynamic;
    float tdpFRF = cores[i]->exu->rfu->FRF->power.readOp.dynamic;

    float lkgIRF = cores[i]->exu->rfu->IRF->rt_power.readOp.get_leak();
    float lkgFRF = cores[i]->exu->rfu->FRF->rt_power.readOp.get_leak();
    type         = 0;

    sprintf(name, "P(%d)_IRF", ii);
    sprintf(conn, "P(%d)_exeu", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaIRF, tdpIRF, 0.0, lkgIRF));

    sprintf(name, "P(%d)_FRF", ii);
    sprintf(conn, "P(%d)_exeu", ii);
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, areaFRF, tdpFRF, 0.0, lkgFRF));
#else

    sprintf(name, "P(%d)_RF", ii);
    sprintf(conn, "P(%d)_EXE", ii);
    area = 1.5e-12 * (pipeline + cores[i]->exu->rfu->IRF->area.get_area() + pipeline + cores[i]->exu->rfu->FRF->area.get_area());
    tdp  = cores[i]->exu->rfu->IRF->power.readOp.dynamic + cores[i]->exu->rfu->FRF->power.readOp.dynamic;
    lkg  = cores[i]->exu->rfu->IRF->rt_power.readOp.get_leak() + cores[i]->exu->rfu->FRF->rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
#endif

#ifdef FINE_GRAINED
    sprintf(conn, "P(%d)_dtlb P(%d)_exeu P(%d)_fp_u", ii, ii, ii);
#else
    sprintf(conn, "P(%d)_LSU P(%d)_EXE", ii, ii);
#endif

    if((cores[i]->coredynp.core_ty == OOO)) {
      sprintf(name, "P(%d)_ROB", ii);
      area = 1.25e-12 * (pipeline + cores[i]->exu->scheu->ROB->area.get_area());
      tdp  = (cores[i]->exu->scheu->ROB->power.readOp.dynamic);
      lkg  = cores[i]->exu->scheu->ROB->rt_power.readOp.get_leak();
      type = 0;
      energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
      area = tdp = lkg = 0;
    }

#ifdef FINE_GRAINED
    sprintf(conn, "L2(%d) P(%d)_dtlb", int(ii / corePerL2), ii);
#else
    if((cores[i]->coredynp.core_ty == OOO)) {
      sprintf(conn, "L2(%d) P(%d)_LSU", int(ii / corePerL2), ii);
    }

#endif

    if(XML->sys.core[0].scoore) { // FIXME: this only works for one core
      sprintf(name, "FL2(%d)", ii);
      area = 1.5e-12 * cores[i]->lsu->VPCfilter.area.get_area();
      tdp  = cores[i]->lsu->VPCfilter.power.readOp.dynamic;
      lkg  = cores[i]->lsu->VPCfilter.rt_power.readOp.get_leak();
      type = 0;
      energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
      area = tdp = lkg = 0;
    }
  }

  for(int ii = 0; ii < (procdynp.numSM); ii++) {

    int i = procdynp.homoCore ? 0 : ii;

    /* -------------------------------------- TEST COUNTER      ----------------------------------------*/
    sprintf(name, "P(%d)_RFGTest", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.25e-12 * gpu->sms[i]->lanes[0]->exu->rfu->area.get_area() * procdynp.numLane;
    tdp  = (gpu->sms[i]->lanes[0]->exu->rfu->power.readOp.dynamic) * procdynp.numLane;
    lkg  = gpu->sms[i]->lanes[0]->exu->rfu->power.readOp.get_leak() * procdynp.numLane;
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU Register File ----------------------------------------*/
    sprintf(name, "P(%d)_RFG", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.25e-12 * gpu->sms[i]->lanes[0]->exu->rfu->area.get_area() * procdynp.numLane;
    tdp  = (gpu->sms[i]->lanes[0]->exu->rfu->power.readOp.dynamic) * procdynp.numLane;
    lkg  = gpu->sms[i]->lanes[0]->exu->rfu->power.readOp.get_leak() * procdynp.numLane;
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU Tinycache_D   ----------------------------------------*/

    if(gpu_tcd_present) {
      sprintf(name, "P(%d)_TCD", (*gpuIndex)[ii]);
      sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
      area = 1.25e-12 * gpu->sms[i]->lanes[0]->tcdata->area.get_area() * procdynp.numLane;
      tdp  = (gpu->sms[i]->lanes[0]->tcdata->power.readOp.dynamic) * procdynp.numLane;
      lkg  = gpu->sms[i]->lanes[0]->tcdata->power.readOp.get_leak() * procdynp.numLane;
      type = 0;
      energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
      area = tdp = lkg = 0;
    }

    /* -------------------------------------- GPU Tinycache_I   ----------------------------------------*/
    if(gpu_tci_present) {
      sprintf(name, "P(%d)_TCI", (*gpuIndex)[ii]);
      sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
      area = 1.25e-12 * gpu->sms[i]->lanes[0]->exu->rfu->area.get_area() * procdynp.numLane;
      tdp  = (gpu->sms[i]->lanes[0]->tcinst->power.readOp.dynamic) * procdynp.numLane;
      lkg  = gpu->sms[i]->lanes[0]->tcinst->power.readOp.get_leak() * procdynp.numLane;
      type = 0;
      energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
      area = tdp = lkg = 0;
    }

    /* -------------------------------------- GPU Lanes         ----------------------------------------*/
    sprintf(name, "P(%d)_lanesG", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_IL1G", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->lane.area.get_area();
    tdp  = gpu->sms[i]->lane.power.readOp.dynamic;
    lkg  = gpu->sms[i]->lane.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU ICACHE        ----------------------------------------*/
    sprintf(name, "P(%d)_IL1G", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->icache.area.get_area();
    tdp  = gpu->sms[i]->icache.power.readOp.dynamic;
    lkg  = gpu->sms[i]->icache.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU IFU           ----------------------------------------*/
#if 0
	sprintf(name, "IFUG(%d)",(*gpuIndex)[ii]);
    sprintf(conn, "lanesG(%d)",(*gpuIndex)[ii]);
    area = 1.5e-12*gpu->sms[i]->icache.area.get_area();
    tdp  = gpu->sms[i]->icache.power.readOp.dynamic;
    lkg  = gpu->sms[i]->icache.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;
#endif

    /* -------------------------------------- GPU ITLB          ----------------------------------------*/
    sprintf(name, "P(%d)_ITLBG", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->mmu_i->area.get_area();
    tdp  = gpu->sms[i]->mmu_i->power.readOp.dynamic;
    lkg  = gpu->sms[i]->mmu_i->rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU DCACHE        ----------------------------------------*/
    sprintf(name, "P(%d)_DL1G", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->dcache.area.get_area();
    tdp  = gpu->sms[i]->dcache.power.readOp.dynamic;
    lkg  = gpu->sms[i]->dcache.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU DTLB          ----------------------------------------*/

    sprintf(name, "P(%d)_DTLBG", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->mmu_d->area.get_area();
    tdp  = gpu->sms[i]->mmu_d->power.readOp.dynamic;
    lkg  = gpu->sms[i]->mmu_d->rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

    /* -------------------------------------- GPU Coalescer     ----------------------------------------*/
#if 0
    sprintf(name, "P(%d)_Coal",(*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG",(*gpuIndex)[ii]);
    area = 1.5e-12*gpu->sms[i]->icache.area.get_area();
    tdp  = gpu->sms[i]->icache.power.readOp.dynamic;
    lkg  = gpu->sms[i]->icache.rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;
#endif

    /* -------------------------------------- GPU Scratch Pad   ----------------------------------------*/
    sprintf(name, "P(%d)_ScratchG", (*gpuIndex)[ii]);
    sprintf(conn, "P(%d)_lanesG", (*gpuIndex)[ii]);
    area = 1.5e-12 * gpu->sms[i]->scratchpad->area.get_area();
    tdp  = gpu->sms[i]->scratchpad->power.readOp.dynamic;
    lkg  = gpu->sms[i]->scratchpad->rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;

#if 0
    sprintf(name, "P(%d)_pipeG",(*gpuIndex)[ii]);
    sprintf(conn, "lanesG(%d) DL1G(%d) IL1G(%d) RFG(%d)",(*gpuIndex)[ii],(*gpuIndex)[ii],(*gpuIndex)[ii],(*gpuIndex)[ii]);
    area = 1.5e-12*gpu->sms[i]->lanes[0]->corepipe->area.get_area()*procdynp.numLane;
    tdp  = gpu->sms[i]->lanes[0]->corepipe->power.readOp.dynamic;
    lkg  = gpu->sms[i]->lanes[0]->corepipe->power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;
#endif
  }

  energyBundle->coreEIdx = energyBundle->cntrs.size();

  // L2s
  idx_start_L2 = energyBundle->cntrs.size();
  for(int ii = 0; ii < procdynp.numL2; ii++) {
    int i = procdynp.homoL3 ? 0 : ii;
    sprintf(name, "L2(%d)", ii);
    sprintf(conn, "L3(0)");
    area = 1.25e-12 * (l2array[i]->area.get_area());
    tdp  = l2array[i]->power.readOp.dynamic;
    lkg  = l2array[i]->rt_power.readOp.get_leak();
    type = XML->sys.L2[0].device_type;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
  }
  idx_end_L2 = energyBundle->cntrs.size();

  // GPU L2
  if(procdynp.numGPU) {
    sprintf(name, "L2G(0)");
    sprintf(conn, "L3(0)");
    area = (1.25e-12) * (gpu->l2array[0].area.get_area());
    tdp  = gpu->l2array[0].power.readOp.dynamic;
    lkg  = gpu->l2array[0].rt_power.readOp.get_leak();
    type = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg, isGPU));
    area = tdp = lkg = 0;
  }

#if 0
  //STLBs
  idx_start_STLB = energyCntrNames->size();
  for(int i = 0; i <procdynp.numSTLB; i++){
    sprintf(str, "STLB(%d)", i);
    energyCntrNames->push_back(str);
  }
  idx_end_STLB = energyCntrNames->size();
#endif

  // L3s
  idx_start_L3 = energyBundle->cntrs.size();

  for(int ii = 0; ii < procdynp.numL3; ii++) {
    int i = procdynp.homoL3 ? 0 : ii;
    sprintf(name, "L3(%d)", ii);
    sprintf(conn, "MemBus(0)");
    if(numSTLB) {
      area = 1.25e-12 * (l3array[i]->area.get_area() + stlbarray[i]->area.get_area());
      tdp  = (l3array[i]->power.readOp.dynamic + stlbarray[i]->power.readOp.dynamic);
      lkg  = l3array[i]->rt_power.readOp.get_leak() + stlbarray[i]->rt_power.readOp.get_leak();
    } else {
      area = 1.25e-12 * (l3array[i]->area.get_area());
      tdp  = (l3array[i]->power.readOp.dynamic);
      lkg  = l3array[i]->rt_power.readOp.get_leak();
    }
    type = XML->sys.L3[0].device_type;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
  }
  idx_end_L3 = energyBundle->cntrs.size();

#if 1
  for(int ii = 0; ii < (procdynp.numNOC); ii++) {
    int i = procdynp.homoNOC ? 0 : ii;

    sprintf(name, "xbar(%d)", ii);
    sprintf(conn, "L3(0)");
    area = 1.25e-12 * (nocs[i]->area.get_area());
    tdp  = nocs[i]->power.readOp.dynamic;
    lkg  = nocs[i]->rt_power.readOp.get_leak();
    type = XML->sys.L2[0].device_type;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
    area = tdp = lkg = 0;
  }
#endif

  // Memory Controller
  sprintf(name, "MemBus(0)");
  sprintf(conn, "L3(0)");
  area = (1.25e-12) * (mc->area.get_area());
  tdp  = mc->power.readOp.dynamic;
  lkg  = mc->rt_power.readOp.get_leak();
  type = 0;
  energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, type, area, tdp, 0.0, lkg));
  area = tdp = lkg = 0;
  return;
}

void Processor::dumpCoreDyn(int i) {

  /*========================================Dump Dynamic===================================================*/

  double pppm_t[4] = {1, 1, 1, 1};
  double cccm_t[4] = {1, 1, 1, 1};
  // double dddm_t[4]    = {0.3,0.3,0.3,0.3};

  float dyn;
  char  name[244];

  float cyc = (*clockInterval)[i];

  set_pppm(pppm_t, (*clockInterval)[i] / XML->sys.core[0].pipeline_depth[0], 1 / XML->sys.core[0].pipeline_depth[0],
           1 / XML->sys.core[0].pipeline_depth[0], 1 / XML->sys.core[0].pipeline_depth[0]);
  if((*clockInterval)[i] >= 50)
    set_pppm(cccm_t, double(POW_SCALE_DYN * energyBundle->getFreq() / (double((*clockInterval)[i]))), 1, 1, 1);
  else {
    set_pppm(cccm_t, 0, 1, 1, 1);
    cyc = 0;
  }

#ifdef FINE_GRAINED
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->iFRAT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_iFRAT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->iRRAT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_iRRAT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->ifreeL->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_iFreeL", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->fFRAT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_fFRAT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->fRRAT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_fRRAT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = ((cores[i]->corepipe->power * dddm_t + cores[i]->rnu->ffreeL->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_fFreeL", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else
  // Renaming Unit
  if(cores[i]->coredynp.core_ty == OOO) {
    dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->rnu->iFRAT->rt_power + cores[i]->rnu->iRRAT->rt_power +
            cores[i]->rnu->ifreeL->rt_power) *
           cccm_t)
              .readOp.get_dyn() +
          (((cores[i]->corepipe->power * pppm_t) + cores[i]->rnu->fFRAT->rt_power + cores[i]->rnu->fRRAT->rt_power +
            cores[i]->rnu->ffreeL->rt_power) *
           cccm_t)
              .readOp.get_dyn();
    sprintf(name, "P(%d)_RNU", i);
    energyBundle->setDynCycByName(name, dyn, cyc);
  }
#endif

#ifdef FINE_GRAINED
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->BPT->globalBPT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_globalBPT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->BPT->L1_localBPT->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_localBPT", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->BPT->chooser->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_Chooser", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->BPT->RAS->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_RAS", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->BTB->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_BTB", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->ifu->IB->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_IB", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else

  // Instruction Fetch Unit
  dyn = // 3
      (((cores[i]->corepipe->power * pppm_t) + cores[i]->ifu->BPT->globalBPT->rt_power + cores[i]->ifu->BPT->L1_localBPT->rt_power +
        cores[i]->ifu->BPT->chooser->rt_power + cores[i]->ifu->BPT->RAS->rt_power + cores[i]->ifu->BTB->rt_power +
        cores[i]->ifu->IB->rt_power) *
       cccm_t)
          .readOp.get_dyn();
  sprintf(name, "P(%d)_fetch", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

#ifdef FINE_GRAINED
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->mmu->itlb->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_itlb", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->ifu->icache.rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_icache", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->ifu->icachecc.rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_icachecc", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else

  // icache + itlb
  dyn = // 4
      (((cores[i]->corepipe->power * pppm_t) + cores[i]->mmu->itlb->rt_power + cores[i]->ifu->icache.rt_power +
        cores[i]->ifu->icachecc.rt_power) *
       cccm_t)
          .readOp.get_dyn();
  sprintf(name, "P(%d)_icache", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

#ifdef FINE_GRAINED
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->mmu->dtlb->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_dtlb", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->dcache.rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_dcache", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->dcachecc.rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_dcachecc", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else
  // dcache + dtlb
  dyn = // 5
      (((cores[i]->corepipe->power * pppm_t) + cores[i]->mmu->dtlb->rt_power + cores[i]->lsu->dcache.rt_power +
        cores[i]->lsu->dcachecc.rt_power) *
       cccm_t)
          .readOp.get_dyn();
  sprintf(name, "P(%d)_dcache", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

#ifdef FINE_GRAINED
  if(XML->sys.core[i].scoore) {
    dyn = (((cores[i]->corepipe->power * pppm_t) +
            // cores[i]->lsu->VPCfilter.rt_power         +
            cores[i]->lsu->vpc_buffer->rt_power + cores[i]->lsu->vpc_specbw->rt_power + cores[i]->lsu->vpc_specbr->rt_power) *
           cccm_t)
              .readOp.get_dyn();
    sprintf(name, "P(%d)_VPCfilter", i);
    energyBundle->setDynCycByName(name, dyn, cyc);

  } else {
    if(XML->sys.core[i].machine_type == 0) // OoO
    {
      dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->LSQ->rt_power) * cccm_t).readOp.get_dyn();
      sprintf(name, "P(%d)_LSQ", i);
      energyBundle->setDynCycByName(name, dyn, cyc);
      dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->LoadQ->rt_power) * cccm_t).readOp.get_dyn();
      sprintf(name, "P(%d)_LoadQ", i);
      energyBundle->setDynCycByName(name, dyn, cyc);
      dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->ssit->rt_power) * cccm_t).readOp.get_dyn();
      sprintf(name, "P(%d)_ssit", i);
      energyBundle->setDynCycByName(name, dyn, cyc);
      dyn = (((cores[i]->corepipe->power * dddm_t) + cores[i]->lsu->lfst->rt_power) * cccm_t).readOp.get_dyn();
      sprintf(name, "P(%d)_lfst", i);
      energyBundle->setDynCycByName(name, dyn, cyc);
    } else {
      dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->lsu->LSQ->rt_power) * cccm_t).readOp.get_dyn();
      sprintf(name, "P(%d)_LSQ", i);
      energyBundle->setDynCycByName(name, dyn, cyc);
    }
  }
#else
  // Load Store Unit
  if(XML->sys.core[i].scoore) {
    dyn = // 6
        (((cores[i]->corepipe->power * pppm_t) +
          // cores[i]->lsu->VPCfilter.rt_power         +
          cores[i]->lsu->vpc_buffer->rt_power + cores[i]->lsu->vpc_specbw->rt_power + cores[i]->lsu->vpc_specbr->rt_power) *
         cccm_t)
            .readOp.get_dyn();

  } else {
    if(XML->sys.core[i].machine_type == 0) // OoO
    {
      dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->lsu->LSQ->rt_power + cores[i]->lsu->LoadQ->rt_power +
              cores[i]->lsu->ssit->rt_power + cores[i]->lsu->lfst->rt_power) *
             cccm_t)
                .readOp.get_dyn();
    } else {
      dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->lsu->LSQ->rt_power) * cccm_t).readOp.get_dyn();
    }
  }
  sprintf(name, "P(%d)_LSU", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

#ifdef FINE_GRAINED
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->exeu->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_exeu", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->fp_u->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_fp_u", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->scheu->int_inst_window->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_int_inst_window", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->scheu->fp_inst_window->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_fp_inst_window", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else

  // Execution Unit I
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->exeu->rt_power) * cccm_t).readOp.get_dyn() +
        (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->fp_u->rt_power) * cccm_t).readOp.get_dyn();

  if(cores[i]->coredynp.multithreaded) {
    dyn += (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->scheu->int_inst_window->rt_power) * cccm_t).readOp.get_dyn();
  }

  if(cores[i]->coredynp.core_ty == OOO) {
    dyn += (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->scheu->fp_inst_window->rt_power) * cccm_t).readOp.get_dyn();
  }
  sprintf(name, "P(%d)_EXE", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

#ifdef FINE_GRAINED
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->rfu->IRF->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_IRF", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->rfu->FRF->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_FRF", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#else

  // Register File Unit I
  dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->rfu->IRF->rt_power) * cccm_t).readOp.get_dyn() +
        (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->rfu->FRF->rt_power) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_RF", i);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

  if(cores[i]->coredynp.core_ty == OOO) {
    dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->exu->scheu->ROB->rt_power) * cccm_t).readOp.get_dyn();
    sprintf(name, "P(%d)_ROB", i);
    energyBundle->setDynCycByName(name, dyn, cyc);
  }

  // Result Broadcast Bus
  // dyn = cores[i]->exu->bypass.rt_power.readOp.get_dyn();
  if(XML->sys.core[i].scoore) {
    dyn = (((cores[i]->corepipe->power * pppm_t) + cores[i]->lsu->VPCfilter.rt_power) * cccm_t).readOp.get_dyn();
    sprintf(name, "FL2(%d)", i);
    energyBundle->setDynCycByName(name, dyn, cyc);
  }
}

void Processor::dumpL2Dyn(int i) {

  /*========================================Dump Dynamic===================================================*/

  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
// int j,k=0;
// FIXME: For now we only support private L2 cache
#if 0
  for(j  = 0;j<procdynp.numCore;j++) {
    if ((*clockInterval)[j] >= 50) {
      continue;
    }
    k++;
    clockIntAvg      += (*clockInterval)[j];
  }
  clockIntAvg    = k == 0? 0 : clockIntAvg/k;
#endif

  if((*clockInterval)[i] >= 50)
    clockIntAvg = (*clockInterval)[i];
  else
    clockIntAvg = 0;

  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * procdynp.freq / clockIntAvg;

  // L2
  dyn = // 16
      l2array[i]->rt_power.readOp.get_dyn() * adjust;
  sprintf(name, "L2(%d)", i);
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}

void Processor::dumpSTLBDyn(int i) {

  /*========================================Dump Dynamic===================================================*/

  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
  int   j, k = 0;
  for(j = 0; j < procdynp.numCore; j++) {
    if((*clockInterval)[j] <= 1) {
      continue;
    }
    k++;
    clockIntAvg += (*clockInterval)[j];
  }

  if(procdynp.numGPU) {
    for(j = 0; j < procdynp.numSM; j++) {
      if(XML->sys.gpu.SMS[j].lanes[0].total_cycles > 100) {
        clockIntAvg += XML->sys.gpu.SMS[j].lanes[0].total_cycles;
        k++;
      }
    }
  }

  clockIntAvg   = k == 0 ? 0 : clockIntAvg / k;
  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * procdynp.freq / clockIntAvg;

  dyn = stlbarray[i]->rt_power.readOp.get_dyn() * adjust;
  sprintf(name, "STLB(%d)", i);
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}

void Processor::dumpNoCDyn(int i) {

  /*========================================Dump Dynamic===================================================*/
  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
  int   j, k = 0;
  for(j = 0; j < procdynp.numNOC; j++) {
    if((*clockInterval)[j] <= 1) {
      continue;
    }
    k++;
    clockIntAvg += (*clockInterval)[j];
  }

  if(procdynp.numGPU) {
    for(j = 0; j < procdynp.numSM; j++) {
      if(XML->sys.gpu.SMS[j].lanes[0].total_cycles > 100) {
        clockIntAvg += XML->sys.gpu.SMS[j].lanes[0].total_cycles;
        k++;
      }
    }
  }

  clockIntAvg   = k == 0 ? 0 : clockIntAvg / k;
  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * procdynp.freq / clockIntAvg;

  dyn = // 17
      (nocs[i]->rt_power.readOp.get_dyn()) * adjust;
  sprintf(name, "xbar(%d)", i);
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}
void Processor::dumpL3Dyn(int i) {

  /*========================================Dump Dynamic===================================================*/
  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
  int   j, k = 0;
  for(j = 0; j < procdynp.numCore; j++) {
#if 1
    if((*clockInterval)[j] <= 1) {
      continue;
    }
    k++;
    clockIntAvg += (*clockInterval)[j];
#else
    if((*clockInterval)[j] > clockIntAvg)
      clockIntAvg = (*clockInterval)[j];
#endif
  }

  if(procdynp.numGPU) {
    for(j = 0; j < procdynp.numSM; j++) {
      if(XML->sys.gpu.SMS[j].lanes[0].total_cycles > 100) {
        clockIntAvg += XML->sys.gpu.SMS[j].lanes[0].total_cycles;
        k++;
      }
    }
  }

  clockIntAvg   = k == 0 ? 0 : clockIntAvg / k;
  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * procdynp.freq / clockIntAvg;

  if(numSTLB)
    dyn = (l3array[i]->rt_power.readOp.get_dyn() + stlbarray[i]->rt_power.readOp.get_dyn()) * adjust;
  else
    dyn = (l3array[i]->rt_power.readOp.get_dyn()) * adjust;
  sprintf(name, "L3(%d)", i);
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}

void Processor::dumpMCDyn() {

  /*========================================Dump Dynamic===================================================*/
  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
  int   j, k = 0;
  for(j = 0; j < procdynp.numCore; j++) {
    if((*clockInterval)[j] <= 1) {
      continue;
    }
    k++;
    clockIntAvg += (*clockInterval)[j];
  }

  if(procdynp.numGPU) {
    for(j = 0; j < procdynp.numSM; j++) {
      if(XML->sys.gpu.SMS[j].lanes[0].total_cycles > 100) {
        clockIntAvg += XML->sys.gpu.SMS[j].lanes[0].total_cycles;
        k++;
      }
    }
  }

  clockIntAvg   = k == 0 ? 0 : clockIntAvg / k;
  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * procdynp.freq / clockIntAvg;

  // Memory Controller
  dyn = // 18
      mc->rt_power.readOp.get_dyn() * adjust;
  sprintf(name, "MemBus(0)");
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}

void Processor::dumpGPUDyn(int i) {

  /*========================================Dump Dynamic===================================================*/

  double pppm_t[4] = {1, 1, 1, 1};
  double cccm_t[4] = {1, 1, 1, 1};

  float dyn;
  char  name[244];

  float cyc = XML->sys.gpu.SMS[i].lanes[0].total_cycles;

  set_pppm(pppm_t, cyc / 4, cyc / 4, cyc / 4, cyc / 4);

  if(XML->sys.gpu.SMS[i].lanes[0].total_cycles >= 100)
    set_pppm(cccm_t, double(POW_SCALE_DYN * energyBundle->getFreq() / (double(XML->sys.gpu.SMS[i].lanes[0].total_cycles))), 1, 1,
             1);
  else
    set_pppm(cccm_t, 0, 1, 1, 1);

  /* -------------------------------------- TEST COUNTER      ----------------------------------------*/
  dyn = 100;
  sprintf(name, "P(%d)_RFGTest", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU Register File ----------------------------------------*/
  dyn = ((gpu->sms[i]->lanes[0]->exu->rfu->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn() *
        procdynp.numLane;
  sprintf(name, "P(%d)_RFG", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU Tinycache_D   ----------------------------------------*/
  dyn = 0;
  if(gpu_tcd_present) {
    dyn = ((gpu->sms[i]->lanes[0]->tcdata->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn() *
          procdynp.numLane;
    sprintf(name, "P(%d)_TCD", (*gpuIndex)[i]);
    energyBundle->setDynCycByName(name, dyn, cyc);
  }

  /* -------------------------------------- GPU Tinycache_I   ----------------------------------------*/
  dyn = 0;
  if(gpu_tci_present) {
    dyn = ((gpu->sms[i]->lanes[0]->tcinst->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn() *
          procdynp.numLane;
    sprintf(name, "P(%d)_TCI", (*gpuIndex)[i]);
    energyBundle->setDynCycByName(name, dyn, cyc);
  }

  /* -------------------------------------- GPU Lanes         ----------------------------------------*/
  dyn = ((gpu->sms[i]->lane.rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_lanesG", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU ICACHE        ----------------------------------------*/
  dyn = ((gpu->sms[i]->icache.rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_IL1G", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU IFU           ----------------------------------------*/
#if 0
  dyn = 0; //TODO
  sprintf(name, "IFUG(%d)",(*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

  /* -------------------------------------- GPU ITLB          ----------------------------------------*/
  dyn = ((gpu->sms[i]->mmu_i->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_ITLBG", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU DCACHE        ----------------------------------------*/
  dyn = ((gpu->sms[i]->dcache.rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_DL1G", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU DTLB          ----------------------------------------*/
  dyn = ((gpu->sms[i]->mmu_d->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_DTLBG", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

  /* -------------------------------------- GPU Coalescer     ----------------------------------------*/
#if 0
  dyn = 0; //TODO
  dyn = ((gpu->sms[i]->dcache.xbar->rt_power + gpu->sms[i]->lanes[0]->corepipe->power*pppm_t)*cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_Coal",(*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif

  /* -------------------------------------- GPU Scratch Pad   ----------------------------------------*/
  dyn = 0; // TODO
  dyn = ((gpu->sms[i]->scratchpad->rt_power + gpu->sms[i]->lanes[0]->corepipe->power * pppm_t) * cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_ScratchG", (*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);

#if 0
  dyn = ((gpu->sms[i]->lanes[0]->corepipe->power*pppm_t)*cccm_t).readOp.get_dyn();
  sprintf(name, "P(%d)_pipeG",(*gpuIndex)[i]);
  energyBundle->setDynCycByName(name, dyn, cyc);
#endif
}

void Processor::dumpL2GDyn() {

  float dyn;
  char  name[244];

  float clockIntAvg = 0.0;
  int   j;
  int   k = 0;
  for(j = 0; j < procdynp.numSM; j++) {
    if(XML->sys.gpu.SMS[j].lanes[0].total_cycles == 0) {
      continue;
    }

    k++;
    clockIntAvg += XML->sys.gpu.SMS[j].lanes[0].total_cycles;
  }
  double adjust = clockIntAvg == 0 ? 0 : POW_SCALE_DYN * energyBundle->getFreq() / (k * clockIntAvg);

  // L2
  dyn = // 16
      gpu->l2array[0].rt_power.readOp.get_dyn() * adjust;
  sprintf(name, "L2G(0)");
  energyBundle->setDynCycByName(name, dyn, clockIntAvg);
}
