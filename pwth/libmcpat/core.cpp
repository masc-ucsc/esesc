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
#include "core.h"
#include "XML_Parse.h"
#include "basic_circuit.h"
#include "const.h"
#include "io.h"
#include "parameter.h"
#include <algorithm>
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>

InstFetchU::InstFetchU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , IB(0)
    , BTB(0) {

  exist         = true;
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  buildInstCache();
  buildInstBuffer();
  buildBranchPredictor();
}

SchedulerU::SchedulerU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , int_inst_window(0)
    , fp_inst_window(0)
    , ROB(0)
    , instruction_selection(0) {

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  if((coredynp.core_ty == Inorder && coredynp.multithreaded)) {
    buildInorderScheduler();
  } else if(coredynp.core_ty == OOO) {
    buildIntInstWindow();
    buildFPInstWindow();
    buildROB();
    buildSelectionU();
  }
}

LoadStoreU::LoadStoreU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , LSQ(0) {
  exist         = true;
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  buildDataCache();

  /*
   * LSU--in-order processors do not have separate load queue: unified lsq
   * partitioned among threads
   * it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
   */

  if(XML->sys.core[ithCore].scoore) {

    buildVPCBuffer();
    buildVPCSpecBuffers();
    buildL2Filter();
    // buildStoreSets();

  } else // not scoore
  {

    buildLSQ();
    if(coredynp.core_ty == OOO) {
      buildLoadQ();
      buildStoreSets();
    }
  }
}

MemManU::MemManU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , itlb(0)
    , dtlb(0) {

  exist = true;

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  buildITLB();
  buildDTLB();
}

RegFU::RegFU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , IRF(0)
    , FRF(0)
    , RFWIN(0) {
  /*
   * processors have separate architectural register files for each thread.
   * therefore, the bypass buses need to travel across all the register files.
   */

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  buildIRF();
  buildFRF();
}

EXECU::EXECU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, double lsq_height_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , lsq_height(lsq_height_)
    , coredynp(dyn_p_)
    , rfu(0)
    , scheu(0)
    , fp_u(0)
    , exeu(0)
    , int_bypass(0)
    , intTagBypass(0)
    , fp_bypass(0)
    , fpTagBypass(0) {

  exist = true;

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;

  rfu   = new RegFU(XML, ithCore, &interface_ip, coredynp);
  scheu = new SchedulerU(XML, ithCore, &interface_ip, coredynp);
  int num_fpu, num_alu;
  num_fpu = XML->sys.core[ithCore].FPU_per_core;
  num_alu = XML->sys.core[ithCore].ALU_per_core;

  exeu = new FunctionalUnit(&interface_ip, ALU, num_alu);

  fp_u = new FunctionalUnit(&interface_ip, FPU, num_fpu);
  area.set_area(area.get_area() + exeu->area.get_area() + fp_u->area.get_area() + rfu->area.get_area() + scheu->area.get_area());

  /*
   * broadcast logic, including int-broadcast; int_tag-broadcast; fp-broadcast; fp_tag-broadcast
   * integer by pass has two paths and fp has 3 paths.
   * on the same bus there are multiple tri-state drivers and muxes that go to different components on the same bus
   */
  interface_ip.wire_is_mat_type = 2; // start from semi-global since local wires are already used
  interface_ip.wire_os_mat_type = 2;
  interface_ip.throughput       = 1.0 / clockRate;
  interface_ip.latency          = 1.0 / clockRate;

  if(coredynp.core_ty == Inorder) {
    int_bypass   = new interconnect("Int Bypass Data", 1, 1, int(ceil(XML->sys.machine_bits / 32.0) * 32),
                                  rfu->int_regfile_height + exeu->FU_height + lsq_height, &interface_ip, 3);
    intTagBypass = new interconnect("Int Bypass tag", 1, 1, coredynp.perThreadState,
                                    rfu->int_regfile_height + exeu->FU_height + lsq_height + scheu->Iw_height, &interface_ip, 3);
    fp_bypass    = new interconnect("FP Bypass Data", 1, 1, int(ceil(XML->sys.machine_bits / 32.0) * 32 * 1.5),
                                 rfu->fp_regfile_height + fp_u->FU_height, &interface_ip, 3);
    fpTagBypass  = new interconnect("FP Bypass tag", 1, 1, coredynp.perThreadState,
                                   rfu->fp_regfile_height + fp_u->FU_height + lsq_height + scheu->Iw_height, &interface_ip, 3);
  } else { // OOO

    // a hack
    interface_ip.ram_cell_tech_type             = 2;
    interface_ip.peri_global_tech_type          = 2;
    interface_ip.data_arr_ram_cell_tech_type    = 2;
    interface_ip.data_arr_peri_global_tech_type = 2;
    interface_ip.tag_arr_ram_cell_tech_type     = 2;
    interface_ip.tag_arr_peri_global_tech_type  = 2;

    if(coredynp.scheu_ty == PhysicalRegFile) {
      /* For physical register based OOO,
       * data broadcast interconnects cover across functional units, lsq, inst windows and register files,
       * while tag broadcast interconnects also cover across ROB
       */
      int_bypass   = new interconnect("Int Bypass Data", 1, 1, int(ceil(coredynp.int_data_width)),
                                    rfu->int_regfile_height + exeu->FU_height + lsq_height, &interface_ip, 3);
      intTagBypass = new interconnect("Int Bypass tag", 1, 1, coredynp.phy_ireg_width,
                                      rfu->int_regfile_height + exeu->FU_height + lsq_height + scheu->Iw_height + scheu->ROB_height,
                                      &interface_ip, 3);
      fp_bypass    = new interconnect("FP Bypass Data", 1, 1, int(ceil(coredynp.fp_data_width)),
                                   rfu->fp_regfile_height + fp_u->FU_height, &interface_ip, 3);
      fpTagBypass  = new interconnect(
          "FP Bypass tag", 1, 1, coredynp.phy_freg_width,
          rfu->fp_regfile_height + fp_u->FU_height + lsq_height + scheu->fp_Iw_height + scheu->ROB_height, &interface_ip, 3);

    } else {
      /*
       * In RS based processor both data and tag are broadcast together,
       * covering functional units, lsq, nst windows, register files, and ROBs
       */
      int_bypass   = new interconnect("Int Bypass Data", 1, 1, int(ceil(coredynp.int_data_width)),
                                    rfu->int_regfile_height + exeu->FU_height + lsq_height + scheu->Iw_height + scheu->ROB_height,
                                    &interface_ip, 3);
      intTagBypass = new interconnect("Int Bypass tag", 1, 1, coredynp.phy_ireg_width,
                                      rfu->int_regfile_height + exeu->FU_height + lsq_height + scheu->Iw_height + scheu->ROB_height,
                                      &interface_ip, 3);
      fp_bypass    = new interconnect("FP Bypass Data", 1, 1, int(ceil(coredynp.fp_data_width)),
                                   rfu->fp_regfile_height + fp_u->FU_height + lsq_height + scheu->fp_Iw_height + scheu->ROB_height,
                                   &interface_ip, 3);
      fpTagBypass  = new interconnect(
          "FP Bypass tag", 1, 1, coredynp.phy_freg_width,
          rfu->fp_regfile_height + fp_u->FU_height + lsq_height + scheu->fp_Iw_height + scheu->ROB_height, &interface_ip, 3);
    }
  }
}

RENAMINGU::RENAMINGU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , iFRAT(0)
    , fFRAT(0)
    , iRRAT(0)
    , fRRAT(0)
    , ifreeL(0)
    , ffreeL(0)
    , idcl(0)
    , fdcl(0) {
  /*
   * Although renaming logic maybe be used in in-order processors,
   * McPAT assumes no renaming logic is used since the performance gain is very limited and
   * the only major inorder processor with renaming logic is Itainium
   * that is a VLIW processor and different from current McPAT's model.
   * physical register base OOO must have Dual-RAT architecture or equivalent structure.FRAT:FrontRAT, RRAT:RetireRAT;
   * i,f prefix mean int and fp
   * RAT for all Renaming logic, random accessible checkpointing is used, but only update when instruction retires.
   * FRAT will be read twice and written once per instruction;
   * RRAT will be write once per instruction when committing and reads out all when context switch
   * checkpointing is implicit
   * Renaming logic is duplicated for each different hardware threads
   *
   * No Dual-RAT is needed in RS-based OOO processors,
   * however, RAT needs to do associative search in RAT, when instruction commits and ROB release the entry,
   * to make sure all the renamings associated with the ROB to be released are updated at the same time.
   * RAM scheme has # ARchi Reg entry with each entry hold phy reg tag,
   * CAM scheme has # Phy Reg entry with each entry hold ARchi reg tag,
   *
   * Both RAM and CAM have same DCL
   */

  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  if(coredynp.core_ty == OOO) {
    // integer pipeline
    if(coredynp.scheu_ty == PhysicalRegFile) {
      if(coredynp.rm_ty == RAMbased) { // FRAT with global checkpointing (GCs) please see paper tech report for detailed
                                       // explaintions
        buildIFRATRam();
        buildFFRATRam();
      } else if((coredynp.rm_ty == CAMbased)) {
        buildIFRATCam();
        buildFFRATCam();
      }

      buildIRRAT();
      buildFRRAT();

      buildIFreeL();
      buildFFreeL();

      idcl = new dep_resource_conflict_check(&interface_ip, coredynp, coredynp.phy_ireg_width); // TODO:Separate 2 sections See TR
      fdcl = new dep_resource_conflict_check(&interface_ip, coredynp, coredynp.phy_freg_width);
    }
  }
  if(coredynp.core_ty == Inorder && coredynp.issueW > 1) {
    /* Dependency check logic will only present when decode(issue) width>1.
     *  Multiple issue in order processor can do without renaming, but dcl is a must.
     */
    idcl = new dep_resource_conflict_check(&interface_ip, coredynp, coredynp.phy_ireg_width); // TODO:Separate 2 sections See TR
    fdcl = new dep_resource_conflict_check(&interface_ip, coredynp, coredynp.phy_freg_width);
  }
}

Core::Core(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , ifu(0)
    , lsu(0)
    , mmu(0)
    , exu(0)
    , rnu(0)
    , corepipe(0)
    , undiffCore(0) {

  exist             = true;
  interface_ip.freq = XML->sys.core[ithCore].clock_rate;
  double pipeline_area_per_unit;

  // clock codes by eka, just to see how much time each method takes

  set_core_param();
  clockRate     = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  ifu           = new InstFetchU(XML, ithCore, &interface_ip, coredynp);
  lsu           = new LoadStoreU(XML, ithCore, &interface_ip, coredynp);
  mmu           = new MemManU(XML, ithCore, &interface_ip, coredynp);
  exu           = new EXECU(XML, ithCore, &interface_ip, lsu->lsq_height, coredynp);
  if(coredynp.core_ty == OOO) {
    rnu = new RENAMINGU(XML, ithCore, &interface_ip, coredynp);
  }
  corepipe = new PipelinePower(&interface_ip, coredynp);

  if(coredynp.core_ty == OOO) {
    pipeline_area_per_unit = corepipe->area.get_area() / 5.0;
    rnu->area.set_area(rnu->area.get_area() + pipeline_area_per_unit);
  } else {
    pipeline_area_per_unit = corepipe->area.get_area() / 4.0;
  }

  area.set_area(area.get_area() + corepipe->area.get_area());

  ifu->area.set_area(ifu->area.get_area() + pipeline_area_per_unit);

  if(lsu->exist) {
    lsu->area.set_area(lsu->area.get_area() + pipeline_area_per_unit);
  }
  if(exu->exist) {
    exu->area.set_area(exu->area.get_area() + pipeline_area_per_unit);
  }

  if(mmu->exist) {
    mmu->area.set_area(mmu->area.get_area() + pipeline_area_per_unit);
  }

  area.set_area(exu->area.get_area() + lsu->area.get_area() + ifu->area.get_area() + mmu->area.get_area());
  if(coredynp.core_ty == OOO) {
    area.set_area(area.get_area() + rnu->area.get_area());
  }

  //  undifferentiated core
  //  undifferentiatedCore = new UndifferentiatedCore(is_default, &interface_ip);
  //  undifferentiatedCore->inOrder=true;
  //  undifferentiatedCore->opt_performance=true;//TODO: add this into XML interface?
  //  undifferentiatedCore->embedded=false;//TODO: add this into XML interface but after 1.x release
  //  undifferentiatedCore->pipeline_stage=XML->sys.core[ithCore].pipeline_depth[0];
  //  undifferentiatedCore->num_hthreads=XML->sys.core[ithCore].number_hardware_threads;
  //  undifferentiatedCore->issue_width=XML->sys.core[ithCore].issue_width;
  //  //undifferentiatedCore.technology =XML->sys.core_tech_node;
  //  undifferentiatedCore->compute();
  //  core_area += undifferentiatedCore->area.get_area()*1e6;

  //  //clock power
  //  clockNetwork.init_wire_external(is_default, &interface_ip);
  //  clockNetwork.clk_area           =area*1.1;//10% of placement overhead. rule of thumb
  //  clockNetwork.end_wiring_level   =5;//toplevel metal
  //  clockNetwork.start_wiring_level =5;//toplevel metal
  //  clockNetwork.num_regs           = corepipe.tot_stage_vector;
  //  clockNetwork.optimize_wire();
}

/*

Core::Core(ParseXML* XML_interface, int ithCore_, InputParameter* interface_ip_)
:XML(XML_interface),
 ithCore(ithCore_),
 interface_ip(*interface_ip_),
 ifu  (0),
 lsu  (0),
 mmu  (0),
 exu  (0),
 rnu  (0),
 corepipe (0),
 undiffCore (0),
 l2cache (0)
{


  bool exit_flag = true;

  double pipeline_area_per_unit;
  //  interface_ip.wire_is_mat_type = 2;
  //  interface_ip.wire_os_mat_type = 2;
  //  interface_ip.wt               =Global_30;
  set_core_param();

  if (XML->sys.Private_L2)
  {
    l2cache = new SharedCache(XML,ithCore, &interface_ip);

  }

  clockRate = coredynp.clockRate;
  executionTime = coredynp.executionTime;
  ifu          = new InstFetchU(XML, ithCore, &interface_ip,coredynp,exit_flag);
  lsu          = new LoadStoreU(XML, ithCore, &interface_ip,coredynp,exit_flag);
  mmu          = new MemManU   (XML, ithCore, &interface_ip,coredynp,exit_flag);
  exu          = new EXECU     (XML, ithCore, &interface_ip,lsu->lsq_height, coredynp,exit_flag);
  undiffCore   = new UndiffCore(XML, ithCore, &interface_ip,coredynp,exit_flag);
  if (coredynp.core_ty==OOO)
  {
    rnu = new RENAMINGU(XML, ithCore, &interface_ip,coredynp);
  }
  corepipe = new PipelinePower(&interface_ip,coredynp);

  if (coredynp.core_ty==OOO)
  {
    pipeline_area_per_unit    = (corepipe->area.get_area()*coredynp.num_pipelines)/5.0;
    if (rnu->exist)
    {
      rnu->area.set_area(rnu->area.get_area() + pipeline_area_per_unit);
    }
  }
  else {
    pipeline_area_per_unit    = (corepipe->area.get_area()*coredynp.num_pipelines)/4.0;
  }

  //area.set_area(area.get_area()+ corepipe->area.get_area());
  if (ifu->exist)
  {
    ifu->area.set_area(ifu->area.get_area() + pipeline_area_per_unit);
    area.set_area(area.get_area() + ifu->area.get_area());
  }
  if (lsu->exist)
  {
    lsu->area.set_area(lsu->area.get_area() + pipeline_area_per_unit);
      area.set_area(area.get_area() + lsu->area.get_area());
  }
  if (exu->exist)
  {
    exu->area.set_area(exu->area.get_area() + pipeline_area_per_unit);
    area.set_area(area.get_area()+exu->area.get_area());
  }
  if (mmu->exist)
  {
    mmu->area.set_area(mmu->area.get_area() + pipeline_area_per_unit);
      area.set_area(area.get_area()+mmu->area.get_area());
  }

  if (coredynp.core_ty==OOO)
  {
    if (rnu->exist)
    {

      area.set_area(area.get_area() + rnu->area.get_area());
    }
  }

  if (undiffCore->exist)
  {
    area.set_area(area.get_area() + undiffCore->area.get_area());
  }

  if (XML->sys.Private_L2)
  {
    area.set_area(area.get_area() + l2cache->area.get_area());

  }
//  //clock power
//  clockNetwork.init_wire_external(is_default, &interface_ip);
//  clockNetwork.clk_area           =area*1.1;//10% of placement overhead. rule of thumb
//  clockNetwork.end_wiring_level   =5;//toplevel metal
//  clockNetwork.start_wiring_level =5;//toplevel metal
//  clockNetwork.num_regs           = corepipe.tot_stage_vector;
//  clockNetwork.optimize_wire();
}

*/

void BranchPredictor::computeEnergy(bool is_tdp) {

  double r_access;
  double w_access;
  if(is_tdp) {
    // eka
    power.reset();
    r_access                          = coredynp.predictionW;
    w_access                          = 0;
    globalBPT->stats_t.readAc.access  = r_access;
    globalBPT->stats_t.writeAc.access = w_access;
    globalBPT->tdp_stats              = globalBPT->stats_t;

    L1_localBPT->stats_t.readAc.access  = r_access;
    L1_localBPT->stats_t.writeAc.access = w_access;
    L1_localBPT->tdp_stats              = L1_localBPT->stats_t;

    chooser->stats_t.readAc.access  = r_access;
    chooser->stats_t.writeAc.access = w_access;
    chooser->tdp_stats              = chooser->stats_t;

    RAS->stats_t.readAc.access  = r_access;
    RAS->stats_t.writeAc.access = 0;
    RAS->tdp_stats              = RAS->stats_t;
  } else {
    // eka
    rt_power.reset();
    r_access                          = XML->sys.core[ithCore].branch_instructions;
    w_access                          = XML->sys.core[ithCore].branch_mispredictions;
    globalBPT->stats_t.readAc.access  = r_access;
    globalBPT->stats_t.writeAc.access = w_access;
    globalBPT->rtp_stats              = globalBPT->stats_t;

    L1_localBPT->stats_t.readAc.access  = r_access;
    L1_localBPT->stats_t.writeAc.access = w_access;
    L1_localBPT->rtp_stats              = L1_localBPT->stats_t;

    chooser->stats_t.readAc.access  = r_access;
    chooser->stats_t.writeAc.access = w_access;
    chooser->rtp_stats              = chooser->stats_t;

    RAS->stats_t.readAc.access  = XML->sys.core[ithCore].function_calls;
    RAS->stats_t.writeAc.access = XML->sys.core[ithCore].function_calls;
    RAS->rtp_stats              = RAS->stats_t;
  }

  globalBPT->power_t.reset();
  L1_localBPT->power_t.reset();
  chooser->power_t.reset();
  RAS->power_t.reset();
  //	globalBPT->power.reset();
  //	L1_localBPT->power.reset();
  //	chooser->power.reset();
  //	RAS->power.reset();

  globalBPT->power_t.readOp.dynamic += globalBPT->local_result.power.readOp.dynamic * globalBPT->stats_t.readAc.access +
                                       globalBPT->stats_t.writeAc.access * globalBPT->local_result.power.writeOp.dynamic;
  L1_localBPT->power_t.readOp.dynamic += L1_localBPT->local_result.power.readOp.dynamic * L1_localBPT->stats_t.readAc.access +
                                         L1_localBPT->stats_t.writeAc.access * L1_localBPT->local_result.power.writeOp.dynamic;
  chooser->power_t.readOp.dynamic += chooser->local_result.power.readOp.dynamic * chooser->stats_t.readAc.access +
                                     chooser->stats_t.writeAc.access * chooser->local_result.power.writeOp.dynamic;
  RAS->power_t.readOp.dynamic += RAS->local_result.power.readOp.dynamic * RAS->stats_t.readAc.access +
                                 RAS->stats_t.writeAc.access * RAS->local_result.power.writeOp.dynamic;

  if(is_tdp) {
    globalBPT->power   = globalBPT->power_t + globalBPT->local_result.power * pppm_lkg;
    L1_localBPT->power = L1_localBPT->power_t + L1_localBPT->local_result.power * pppm_lkg;
    chooser->power     = chooser->power_t + chooser->local_result.power * pppm_lkg;
    RAS->power         = RAS->power_t + RAS->local_result.power * pppm_lkg;

    power = power + globalBPT->power + L1_localBPT->power + chooser->power + RAS->power;
  } else {
    globalBPT->rt_power   = globalBPT->power_t + globalBPT->local_result.power * pppm_lkg;
    L1_localBPT->rt_power = L1_localBPT->power_t + L1_localBPT->local_result.power * pppm_lkg;
    chooser->rt_power     = chooser->power_t + chooser->local_result.power * pppm_lkg;
    RAS->rt_power         = RAS->power_t + RAS->local_result.power * pppm_lkg;
    rt_power              = rt_power + globalBPT->rt_power + L1_localBPT->rt_power + chooser->rt_power + RAS->rt_power;
  }
}

void BranchPredictor::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');
  if(is_tdp) {
    cout << indent_str << "Global Predictor:" << endl;
    cout << indent_str_next << "Area = " << globalBPT->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << globalBPT->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << globalBPT->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << globalBPT->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << globalBPT->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Local Predictor:" << endl;
    cout << indent_str_next << "Area = " << L1_localBPT->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << L1_localBPT->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << L1_localBPT->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << L1_localBPT->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << L1_localBPT->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Chooser:" << endl;
    cout << indent_str_next << "Area = " << chooser->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << chooser->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << chooser->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << chooser->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << chooser->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "RAS:" << endl;
    cout << indent_str_next << "Area = " << RAS->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << RAS->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << RAS->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << RAS->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << RAS->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
  } else {
    //		cout << indent_str_next << "Global Predictor    TDP Dynamic = " << globalBPT->rt_power.readOp.dynamic*clockRate << " W" <<
    // endl; 		cout << indent_str_next << "Global Predictor    Subthreshold Leakage = " << globalBPT->rt_power.readOp.leakage <<" W"
    // <<
    // endl; 		cout << indent_str_next << "Global Predictor    Gate Leakage = " << globalBPT->rt_power.readOp.gate_leakage << " W"
    // <<
    // endl; 		cout << indent_str_next << "Local Predictor   TDP Dynamic = " << L1_localBPT->rt_power.readOp.dynamic*clockRate  << "
    // W"
    //<< endl; 		cout << indent_str_next << "Local Predictor   Subthreshold Leakage = " << L1_localBPT->rt_power.readOp.leakage <<
    //" W" << endl; 		cout << indent_str_next << "Local Predictor   Gate Leakage = " << L1_localBPT->rt_power.readOp.gate_leakage
    // << " W" << endl; 		cout << indent_str_next << "Chooser   TDP Dynamic = " << chooser->rt_power.readOp.dynamic*clockRate  << "
    // W" << endl; 		cout << indent_str_next << "Chooser   Subthreshold Leakage = " << chooser->rt_power.readOp.leakage  << " W" <<
    // endl; 		cout << indent_str_next << "Chooser   Gate Leakage = " << chooser->rt_power.readOp.gate_leakage  << " W" << endl; 		cout
    //<< indent_str_next << "RAS   TDP Dynamic = " << RAS->rt_power.readOp.dynamic*clockRate  << " W" << endl; 		cout <<
    //indent_str_next << "RAS   Subthreshold Leakage = " << RAS->rt_power.readOp.leakage  << " W" << endl; 		cout << indent_str_next
    //<< "RAS   Gate Leakage = " << RAS->rt_power.readOp.gate_leakage  << " W" << endl;
  }
}

void InstFetchU::computeEnergy(bool is_tdp) {

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    icache.caches->stats_t.readAc.access = icache.caches->l_ip.num_rw_ports;
    icache.caches->stats_t.readAc.miss   = 0;
    icache.caches->stats_t.readAc.hit    = icache.caches->stats_t.readAc.access - icache.caches->stats_t.readAc.miss;
    icache.caches->tdp_stats             = icache.caches->stats_t;

    icache.missb->stats_t.readAc.access = icache.missb->stats_t.readAc.hit = 0;   // icache.missb->l_ip.num_search_ports;
    icache.missb->stats_t.writeAc.access = icache.missb->stats_t.writeAc.hit = 0; // icache.missb->l_ip.num_search_ports;
    icache.missb->tdp_stats                                                  = icache.missb->stats_t;

    // icache.ifb->stats_t.readAc.access  = icache.ifb->stats_t.readAc.hit=0;//icache.ifb->l_ip.num_search_ports;
    // icache.ifb->stats_t.writeAc.access = icache.ifb->stats_t.writeAc.hit=0;//icache.ifb->l_ip.num_search_ports;
    // icache.ifb->tdp_stats = icache.ifb->stats_t;

    // icache.prefetchb->stats_t.readAc.access  = icache.prefetchb->stats_t.readAc.hit=0;//icache.prefetchb->l_ip.num_search_ports;
    // icache.prefetchb->stats_t.writeAc.access = icache.prefetchb->stats_t.writeAc.hit=0;//icache.ifb->l_ip.num_search_ports;
    // icache.prefetchb->tdp_stats = icache.prefetchb->stats_t;

    IB->stats_t.readAc.access = IB->stats_t.writeAc.access = XML->sys.core[ithCore].issue_width;
    IB->tdp_stats                                          = IB->stats_t;

    if(coredynp.predictionW > 0) {
      BTB->stats_t.readAc.access  = coredynp.predictionW; // XML->sys.core[ithCore].BTB.read_accesses;
      BTB->stats_t.writeAc.access = 0;                    // XML->sys.core[ithCore].BTB.write_accesses;
    }

  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    icache.caches->stats_t.readAc.access = XML->sys.core[ithCore].icache.read_accesses;
    icache.caches->stats_t.readAc.miss   = XML->sys.core[ithCore].icache.read_misses;
    icache.caches->stats_t.readAc.hit    = icache.caches->stats_t.readAc.access - icache.caches->stats_t.readAc.miss;
    icache.caches->rtp_stats             = icache.caches->stats_t;

    icache.missb->stats_t.readAc.access  = icache.caches->stats_t.readAc.miss;
    icache.missb->stats_t.writeAc.access = icache.caches->stats_t.readAc.miss;
    icache.missb->rtp_stats              = icache.missb->stats_t;

    // icache.ifb->stats_t.readAc.access  = icache.caches->stats_t.readAc.miss;
    // icache.ifb->stats_t.writeAc.access = icache.caches->stats_t.readAc.miss;
    // icache.ifb->rtp_stats = icache.ifb->stats_t;

    // icache.prefetchb->stats_t.readAc.access  = icache.caches->stats_t.readAc.miss;
    // icache.prefetchb->stats_t.writeAc.access = icache.caches->stats_t.readAc.miss;
    // icache.prefetchb->rtp_stats = icache.prefetchb->stats_t;

    IB->stats_t.readAc.access = IB->stats_t.writeAc.access =
        XML->sys.core[ithCore].total_instructions / XML->sys.core[ithCore].issue_width;
    IB->rtp_stats = IB->stats_t;

    if(coredynp.predictionW > 0) {
      BTB->stats_t.readAc.access  = XML->sys.core[ithCore].branch_instructions;   // XML->sys.core[ithCore].BTB.read_accesses;
      BTB->stats_t.writeAc.access = XML->sys.core[ithCore].branch_mispredictions; // XML->sys.core[ithCore].BTB.write_accesses;
      BTB->rtp_stats              = BTB->stats_t;
    }
  }
  icache.power_t.reset();
  icachecc.power_t.reset();
  IB->power_t.reset();
  // eka, may need to chech the first call
  // icache.power.reset();
  // IB->power.reset();
  if(coredynp.predictionW > 0) {
    BTB->power_t.reset();
    // BTB->power.reset(); //eka, may need to check the first call
  }
  icache.power_t.readOp.dynamic +=
      (icache.caches->stats_t.readAc.hit * icache.caches->local_result.power.readOp.dynamic +
       icache.caches->stats_t.readAc.miss * icache.caches->local_result.tag_array2->power.readOp.dynamic +
       icache.caches->stats_t.readAc.miss *
           icache.caches->local_result.power.writeOp.dynamic); // read miss in Icache cause a write to Icache
  icachecc.power_t.readOp.dynamic +=
      icache.missb->stats_t.readAc.access * icache.missb->local_result.power.searchOp.dynamic +
      icache.missb->stats_t.writeAc.access *
          icache.missb->local_result.power.writeOp.dynamic; // each access to missb involves a CAM and a write
  // icache.power_t.readOp.dynamic	+=  icache.ifb->stats_t.readAc.access*icache.ifb->local_result.power.searchOp.dynamic +
  //        icache.ifb->stats_t.writeAc.access*icache.ifb->local_result.power.writeOp.dynamic;
  // icache.power_t.readOp.dynamic	+= icache.prefetchb->stats_t.readAc.access*icache.prefetchb->local_result.power.searchOp.dynamic
  // +
  //  icache.prefetchb->stats_t.writeAc.access*icache.prefetchb->local_result.power.writeOp.dynamic;

  IB->power_t.readOp.dynamic += IB->local_result.power.readOp.dynamic * IB->stats_t.readAc.access +
                                IB->stats_t.writeAc.access * IB->local_result.power.writeOp.dynamic;

  if(coredynp.predictionW > 0) {
    BTB->power_t.readOp.dynamic += BTB->local_result.power.readOp.dynamic * BTB->stats_t.readAc.access +
                                   BTB->stats_t.writeAc.access * BTB->local_result.power.writeOp.dynamic;

    BPT->computeEnergy(is_tdp);
  }

  if(is_tdp) {
    icache.power   = icache.power_t + (icache.caches->local_result.power) * pppm_lkg;
    icachecc.power = icachecc.power_t + (icache.missb->local_result.power
                                         //+icache.ifb->local_result.power
                                         //+icache.prefetchb->local_result.power
                                         ) * pppm_Isub;

    IB->power = IB->power_t + IB->local_result.power * pppm_lkg;
    power     = power + icache.power + IB->power + icachecc.power;
    if(coredynp.predictionW > 0) {
      BTB->power = BTB->power_t + BTB->local_result.power * pppm_lkg;
      power      = power + BTB->power + BPT->power;
    }
  } else {
    icache.rt_power   = icache.power_t + (icache.caches->local_result.power) * pppm_lkg;
    icachecc.rt_power = icachecc.power_t + (icache.missb->local_result.power
                                            //+icache.ifb->local_result.power
                                            //+icache.prefetchb->local_result.power
                                            ) * pppm_Isub;

    IB->rt_power = IB->power_t + IB->local_result.power * pppm_lkg;
    rt_power     = rt_power + icache.rt_power + IB->rt_power + icachecc.rt_power;
    if(coredynp.predictionW > 0) {
      BTB->rt_power = BTB->power_t + BTB->local_result.power * pppm_lkg;
      rt_power      = rt_power + BTB->rt_power + BPT->rt_power;
    }
  }
}

void InstFetchU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {

    cout << indent_str << "Instruction Cache:" << endl;
    cout << indent_str_next << "Area = " << icache.area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << icache.power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << icache.power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << icache.power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << icache.rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Instruction Buffer:   Area = " << IB->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "Area = " << IB->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << IB->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << IB->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << IB->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << IB->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(coredynp.predictionW > 0) {
      cout << indent_str << "Branch Target Buffer: Area = " << BTB->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "Area = " << BTB->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << BTB->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << BTB->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << BTB->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << BTB->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << "Branch Predictor:     Area = " << BPT->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "Area = " << BPT->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << BPT->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << BPT->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << BPT->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << BPT->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      if(plevel > 3) {
        BPT->displayEnergy(indent + 4, plevel, is_tdp);
      }
    }
  } else {
    //		cout << indent_str_next << "Instruction Cache    TDP Dynamic = " << icache.rt_power.readOp.dynamic*clockRate << " W" <<
    // endl; 		cout << indent_str_next << "Instruction Cache    Subthreshold Leakage = " << icache.rt_power.readOp.leakage <<" W"
    // << endl; 		cout << indent_str_next << "Instruction Cache    Gate Leakage = " << icache.rt_power.readOp.gate_leakage << " W"
    // << endl; 		cout << indent_str_next << "Instruction Buffer   TDP Dynamic = " << IB->rt_power.readOp.dynamic*clockRate  << " W"
    // << endl; 		cout << indent_str_next << "Instruction Buffer   Subthreshold Leakage = " << IB->rt_power.readOp.leakage  << " W"
    // << endl; 		cout << indent_str_next << "Instruction Buffer   Gate Leakage = " << IB->rt_power.readOp.gate_leakage  << " W" <<
    // endl; 		cout << indent_str_next << "Branch Target Buffer   TDP Dynamic = " << BTB->rt_power.readOp.dynamic*clockRate  << " W" <<
    // endl; 		cout << indent_str_next << "Branch Target Buffer   Subthreshold Leakage = " << BTB->rt_power.readOp.leakage  << " W"
    // << endl; 		cout << indent_str_next << "Branch Target Buffer   Gate Leakage = " << BTB->rt_power.readOp.gate_leakage  << " W"
    // << endl; 		cout << indent_str_next << "Branch Predictor   TDP Dynamic = " << BPT->rt_power.readOp.dynamic*clockRate  << " W"
    // << endl; 		cout << indent_str_next << "Branch Predictor   Subthreshold Leakage = " << BPT->rt_power.readOp.leakage  << " W"
    // << endl; 		cout << indent_str_next << "Branch Predictor   Gate Leakage = " << BPT->rt_power.readOp.gate_leakage  << " W" <<
    // endl;
  }
}

void RENAMINGU::computeEnergy(bool is_tdp) {
  double pppm_t[4] = {1, 1, 1, 1};

  if(is_tdp) { // init stats for TDP
    // eka
    power.reset();
    if(coredynp.core_ty == OOO) {
      if(coredynp.scheu_ty == PhysicalRegFile) {
        if(coredynp.rm_ty == RAMbased) {
          iFRAT->stats_t.readAc.access  = iFRAT->l_ip.num_rd_ports;
          iFRAT->stats_t.writeAc.access = iFRAT->l_ip.num_wr_ports;
          iFRAT->tdp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = fFRAT->l_ip.num_rd_ports;
          fFRAT->stats_t.writeAc.access = fFRAT->l_ip.num_wr_ports;
          fFRAT->tdp_stats              = fFRAT->stats_t;

        } else if((coredynp.rm_ty == CAMbased)) {
          iFRAT->stats_t.readAc.access  = iFRAT->l_ip.num_search_ports;
          iFRAT->stats_t.writeAc.access = iFRAT->l_ip.num_wr_ports;
          iFRAT->tdp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = fFRAT->l_ip.num_search_ports;
          fFRAT->stats_t.writeAc.access = fFRAT->l_ip.num_wr_ports;
          fFRAT->tdp_stats              = fFRAT->stats_t;
        }

        iRRAT->stats_t.readAc.access  = iRRAT->l_ip.num_rd_ports;
        iRRAT->stats_t.writeAc.access = iRRAT->l_ip.num_wr_ports;
        iRRAT->tdp_stats              = iRRAT->stats_t;

        fRRAT->stats_t.readAc.access  = fRRAT->l_ip.num_rd_ports;
        fRRAT->stats_t.writeAc.access = fRRAT->l_ip.num_wr_ports;
        fRRAT->tdp_stats              = fRRAT->stats_t;

        ifreeL->stats_t.readAc.access  = ifreeL->l_ip.num_rd_ports;
        ifreeL->stats_t.writeAc.access = ifreeL->l_ip.num_wr_ports;
        ifreeL->tdp_stats              = ifreeL->stats_t;

        ffreeL->stats_t.readAc.access  = ffreeL->l_ip.num_rd_ports;
        ffreeL->stats_t.writeAc.access = ffreeL->l_ip.num_wr_ports;
        ffreeL->tdp_stats              = ffreeL->stats_t;
      } else if(coredynp.scheu_ty == ReservationStation) {
        if(coredynp.rm_ty == RAMbased) {
          iFRAT->stats_t.readAc.access   = iFRAT->l_ip.num_rd_ports;
          iFRAT->stats_t.writeAc.access  = iFRAT->l_ip.num_wr_ports;
          iFRAT->stats_t.searchAc.access = iFRAT->l_ip.num_search_ports;
          iFRAT->tdp_stats               = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access   = fFRAT->l_ip.num_rd_ports;
          fFRAT->stats_t.writeAc.access  = fFRAT->l_ip.num_wr_ports;
          fFRAT->stats_t.searchAc.access = fFRAT->l_ip.num_search_ports;
          fFRAT->tdp_stats               = fFRAT->stats_t;

        } else if((coredynp.rm_ty == CAMbased)) {
          iFRAT->stats_t.readAc.access  = iFRAT->l_ip.num_search_ports;
          iFRAT->stats_t.writeAc.access = iFRAT->l_ip.num_wr_ports;
          iFRAT->tdp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = fFRAT->l_ip.num_search_ports;
          fFRAT->stats_t.writeAc.access = fFRAT->l_ip.num_wr_ports;
          fFRAT->tdp_stats              = fFRAT->stats_t;
        }
        // Unified free list for both int and fp
        ifreeL->stats_t.readAc.access  = ifreeL->l_ip.num_rd_ports;
        ifreeL->stats_t.writeAc.access = ifreeL->l_ip.num_wr_ports;
        ifreeL->tdp_stats              = ifreeL->stats_t;
      }

    } else {
      if(coredynp.issueW > 1) {
        idcl->stats_t.readAc.access = coredynp.decodeW;
        fdcl->stats_t.readAc.access = coredynp.decodeW;
        idcl->tdp_stats             = idcl->stats_t;
        fdcl->tdp_stats             = fdcl->stats_t;
      }
    }

  } else { // init stats for Runtime Dynamic (RTP)
    // eka
    rt_power.reset();
    if(coredynp.core_ty == OOO) {
      if(coredynp.scheu_ty == PhysicalRegFile) {
        if(coredynp.rm_ty == RAMbased) {
          iFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions;
          iFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions;
          iFRAT->rtp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].fp_instructions;
          fFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].fp_instructions;
          fFRAT->rtp_stats              = fFRAT->stats_t;
        } else if((coredynp.rm_ty == CAMbased)) {
          iFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions;
          iFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions;
          iFRAT->rtp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].fp_instructions;
          fFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].fp_instructions;
          fFRAT->rtp_stats              = fFRAT->stats_t;
        }

        iRRAT->stats_t.readAc.access  = 0;
        iRRAT->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions;
        iRRAT->rtp_stats              = iRRAT->stats_t;

        fRRAT->stats_t.readAc.access  = 0;
        fRRAT->stats_t.writeAc.access = XML->sys.core[ithCore].fp_instructions;
        fRRAT->rtp_stats              = fRRAT->stats_t;

        ifreeL->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions;
        ifreeL->stats_t.writeAc.access = 2 * XML->sys.core[ithCore].int_instructions;
        ifreeL->rtp_stats              = ifreeL->stats_t;

        ffreeL->stats_t.readAc.access  = XML->sys.core[ithCore].fp_instructions;
        ffreeL->stats_t.writeAc.access = 2 * XML->sys.core[ithCore].fp_instructions;
        ffreeL->rtp_stats              = ffreeL->stats_t;
      } else if(coredynp.scheu_ty == ReservationStation) {
        if(coredynp.rm_ty == RAMbased) {
          iFRAT->stats_t.readAc.access   = XML->sys.core[ithCore].int_instructions;
          iFRAT->stats_t.writeAc.access  = XML->sys.core[ithCore].int_instructions;
          iFRAT->stats_t.searchAc.access = XML->sys.core[ithCore].int_instructions;
          iFRAT->rtp_stats               = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access   = XML->sys.core[ithCore].fp_instructions;
          fFRAT->stats_t.writeAc.access  = XML->sys.core[ithCore].fp_instructions;
          fFRAT->stats_t.searchAc.access = XML->sys.core[ithCore].fp_instructions;
          fFRAT->rtp_stats               = fFRAT->stats_t;
        } else if((coredynp.rm_ty == CAMbased)) {
          iFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions;
          iFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions;
          iFRAT->rtp_stats              = iFRAT->stats_t;

          fFRAT->stats_t.readAc.access  = XML->sys.core[ithCore].fp_instructions;
          fFRAT->stats_t.writeAc.access = XML->sys.core[ithCore].fp_instructions;
          fFRAT->rtp_stats              = fFRAT->stats_t;
        }
        // Unified free list for both int and fp
        ifreeL->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions + XML->sys.core[ithCore].fp_instructions;
        ifreeL->stats_t.writeAc.access = 2 * (XML->sys.core[ithCore].int_instructions + XML->sys.core[ithCore].fp_instructions);
        ifreeL->rtp_stats              = ifreeL->stats_t;
      }
    } else {
      if(coredynp.issueW > 1) {
        idcl->stats_t.readAc.access = XML->sys.core[ithCore].int_instructions;
        fdcl->stats_t.readAc.access = XML->sys.core[ithCore].fp_instructions;
        idcl->rtp_stats             = idcl->stats_t;
        fdcl->rtp_stats             = fdcl->stats_t;
      }
    }
  }
  /* Compute engine */
  if(coredynp.core_ty == OOO) {
    if(coredynp.scheu_ty == PhysicalRegFile) {

      if(coredynp.rm_ty == RAMbased) {
        iFRAT->power_t.reset();
        fFRAT->power_t.reset();
        // eka, may need to check the first call
        // iFRAT->power.reset();
        // fFRAT->power.reset();
        iFRAT->power_t.readOp.dynamic +=
            (iFRAT->stats_t.readAc.access * (iFRAT->local_result.power.readOp.dynamic + idcl->power.readOp.dynamic) +
             iFRAT->stats_t.writeAc.access * iFRAT->local_result.power.writeOp.dynamic);
        fFRAT->power_t.readOp.dynamic +=
            (fFRAT->stats_t.readAc.access * (fFRAT->local_result.power.readOp.dynamic + fdcl->power.readOp.dynamic) +
             fFRAT->stats_t.writeAc.access * fFRAT->local_result.power.writeOp.dynamic);
      } else if((coredynp.rm_ty == CAMbased)) {
        iFRAT->power_t.reset();
        fFRAT->power_t.reset();
        // eka, may need to check the first call
        // iFRAT->power.reset();
        // fFRAT->power.reset();
        iFRAT->power_t.readOp.dynamic +=
            (iFRAT->stats_t.readAc.access * (iFRAT->local_result.power.searchOp.dynamic + idcl->power.readOp.dynamic) +
             iFRAT->stats_t.writeAc.access * iFRAT->local_result.power.writeOp.dynamic);
        fFRAT->power_t.readOp.dynamic +=
            (fFRAT->stats_t.readAc.access * (fFRAT->local_result.power.searchOp.dynamic + fdcl->power.readOp.dynamic) +
             fFRAT->stats_t.writeAc.access * fFRAT->local_result.power.writeOp.dynamic);
      }

      iRRAT->power_t.reset();
      fRRAT->power_t.reset();
      ifreeL->power_t.reset();
      ffreeL->power_t.reset();
      // eka, may need to check the first call
      // iRRAT->power.reset();
      // fRRAT->power.reset();
      // ifreeL->power.reset();
      // ffreeL->power.reset();

      iRRAT->power_t.readOp.dynamic += (iRRAT->stats_t.readAc.access * iRRAT->local_result.power.readOp.dynamic +
                                        iRRAT->stats_t.writeAc.access * iRRAT->local_result.power.writeOp.dynamic);
      fRRAT->power_t.readOp.dynamic += (fRRAT->stats_t.readAc.access * fRRAT->local_result.power.readOp.dynamic +
                                        fRRAT->stats_t.writeAc.access * fRRAT->local_result.power.writeOp.dynamic);
      ifreeL->power_t.readOp.dynamic += (ifreeL->stats_t.readAc.access * ifreeL->local_result.power.readOp.dynamic +
                                         ifreeL->stats_t.writeAc.access * ifreeL->local_result.power.writeOp.dynamic);
      ffreeL->power_t.readOp.dynamic += (ffreeL->stats_t.readAc.access * ffreeL->local_result.power.readOp.dynamic +
                                         ffreeL->stats_t.writeAc.access * ffreeL->local_result.power.writeOp.dynamic);

    } else if(coredynp.scheu_ty == ReservationStation) {
      if(coredynp.rm_ty == RAMbased) {
        iFRAT->power_t.reset();
        fFRAT->power_t.reset();
        // eka, may need to check the first call
        // iFRAT->power.reset();
        // fFRAT->power.reset();

        iFRAT->power_t.readOp.dynamic +=
            (iFRAT->stats_t.readAc.access * (iFRAT->local_result.power.readOp.dynamic + idcl->power.readOp.dynamic) +
             iFRAT->stats_t.writeAc.access * iFRAT->local_result.power.writeOp.dynamic +
             iFRAT->stats_t.searchAc.access * iFRAT->local_result.power.searchOp.dynamic);
        fFRAT->power_t.readOp.dynamic +=
            (fFRAT->stats_t.readAc.access * (fFRAT->local_result.power.readOp.dynamic + fdcl->power.readOp.dynamic) +
             fFRAT->stats_t.writeAc.access * fFRAT->local_result.power.writeOp.dynamic +
             fFRAT->stats_t.searchAc.access * fFRAT->local_result.power.searchOp.dynamic);
      } else if((coredynp.rm_ty == CAMbased)) {
        iFRAT->power_t.reset();
        fFRAT->power_t.reset();
        // eka, may need to check the first call
        // iFRAT->power.reset();
        // fFRAT->power.reset();
        iFRAT->power_t.readOp.dynamic +=
            (iFRAT->stats_t.readAc.access * (iFRAT->local_result.power.searchOp.dynamic + idcl->power.readOp.dynamic) +
             iFRAT->stats_t.writeAc.access * iFRAT->local_result.power.writeOp.dynamic);
        fFRAT->power_t.readOp.dynamic +=
            (fFRAT->stats_t.readAc.access * (fFRAT->local_result.power.searchOp.dynamic + fdcl->power.readOp.dynamic) +
             fFRAT->stats_t.writeAc.access * fFRAT->local_result.power.writeOp.dynamic);
      }
      ifreeL->power_t.reset();
      // eka, may need to check the first call
      // ifreeL->power.reset();
      ifreeL->power_t.readOp.dynamic += (ifreeL->stats_t.readAc.access * ifreeL->local_result.power.readOp.dynamic +
                                         ifreeL->stats_t.writeAc.access * ifreeL->local_result.power.writeOp.dynamic);
    }

  } else {
    if(coredynp.issueW > 1)

    {
      idcl->power_t.reset();
      fdcl->power_t.reset();
      // eka, may need to check the first call
      // idcl->power.reset();
      // fdcl->power.reset();
      set_pppm(pppm_t, idcl->stats_t.readAc.access, 1, 1, 1);
      idcl->power_t = idcl->local_result.power * pppm_t;
      set_pppm(pppm_t, fdcl->stats_t.readAc.access, 1, 1, 1);
      fdcl->power_t = fdcl->local_result.power * pppm_t;
    }
  }

  // assign value to tpd and rtp
  if(is_tdp) {
    if(coredynp.core_ty == OOO) {
      if(coredynp.scheu_ty == PhysicalRegFile) {
        iFRAT->power  = iFRAT->power_t + (iFRAT->local_result.power) * pppm_lkg + idcl->power_t;
        fFRAT->power  = fFRAT->power_t + (fFRAT->local_result.power) * pppm_lkg + fdcl->power_t;
        iRRAT->power  = iRRAT->power_t + iRRAT->local_result.power * pppm_lkg;
        fRRAT->power  = fRRAT->power_t + fRRAT->local_result.power * pppm_lkg;
        ifreeL->power = ifreeL->power_t + ifreeL->local_result.power * pppm_lkg;
        ffreeL->power = ffreeL->power_t + ffreeL->local_result.power * pppm_lkg;
        power = power + (iFRAT->power_t + fFRAT->power_t) + (iRRAT->power_t + fRRAT->power_t) + (ifreeL->power_t + ffreeL->power_t);
      } else if(coredynp.scheu_ty == ReservationStation) {
        iFRAT->power  = iFRAT->power_t + (iFRAT->local_result.power) * pppm_lkg + idcl->power_t;
        fFRAT->power  = fFRAT->power_t + (fFRAT->local_result.power) * pppm_lkg + fdcl->power_t;
        ifreeL->power = ifreeL->power_t + ifreeL->local_result.power * pppm_lkg;
        power         = power + (iFRAT->power_t + fFRAT->power_t) + ifreeL->power_t;
      }
    } else {
      power = power + idcl->power_t + fdcl->power_t;
    }

  } else {
    if(coredynp.core_ty == OOO) {
      if(coredynp.scheu_ty == PhysicalRegFile) {

        iFRAT->rt_power  = iFRAT->power_t + (iFRAT->local_result.power) * pppm_lkg + idcl->power_t;
        fFRAT->rt_power  = fFRAT->power_t + (fFRAT->local_result.power) * pppm_lkg + fdcl->power_t;
        iRRAT->rt_power  = iRRAT->power_t + iRRAT->local_result.power * pppm_lkg;
        fRRAT->rt_power  = fRRAT->power_t + fRRAT->local_result.power * pppm_lkg;
        ifreeL->rt_power = ifreeL->power_t + ifreeL->local_result.power * pppm_lkg;
        ffreeL->rt_power = ffreeL->power_t + ffreeL->local_result.power * pppm_lkg;
        rt_power         = rt_power + (iFRAT->rt_power + fFRAT->rt_power) + (iRRAT->rt_power + fRRAT->rt_power) +
                   (ifreeL->rt_power + ffreeL->rt_power);
      } else if(coredynp.scheu_ty == ReservationStation) {
        iFRAT->rt_power  = iFRAT->power_t + (iFRAT->local_result.power) * pppm_lkg + idcl->power_t;
        fFRAT->rt_power  = fFRAT->power_t + (fFRAT->local_result.power) * pppm_lkg + fdcl->power_t;
        ifreeL->rt_power = ifreeL->power_t + ifreeL->local_result.power * pppm_lkg;
        rt_power         = rt_power + (iFRAT->rt_power + fFRAT->rt_power) + ifreeL->rt_power;
      }
    } else {
      rt_power = rt_power + idcl->power_t + fdcl->power_t;
    }
  }
}

void RENAMINGU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {

    if(coredynp.core_ty == OOO) {
      cout << indent_str << "Int Front End RAT:" << endl;
      cout << indent_str_next << "Area = " << iFRAT->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << iFRAT->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << iFRAT->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << iFRAT->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << iFRAT->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << "FP Front End RAT:" << endl;
      cout << indent_str_next << "Area = " << fFRAT->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << fFRAT->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << fFRAT->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << fFRAT->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << fFRAT->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << "Free List:" << endl;
      cout << indent_str_next << "Area = " << ifreeL->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << ifreeL->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << ifreeL->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << ifreeL->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << ifreeL->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;

      if(coredynp.scheu_ty == PhysicalRegFile) {
        cout << indent_str << "Int Retire RAT: " << endl;
        cout << indent_str_next << "Area = " << iRRAT->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << iRRAT->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << iRRAT->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << iRRAT->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << iRRAT->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << endl;
        cout << indent_str << "FP Retire RAT:" << endl;
        cout << indent_str_next << "Area = " << fRRAT->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << fRRAT->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << fRRAT->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << fRRAT->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << fRRAT->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << endl;
        cout << indent_str << "FP Free List:" << endl;
        cout << indent_str_next << "Area = " << ffreeL->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << ffreeL->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << ffreeL->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << ffreeL->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << ffreeL->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << endl;
      }
    } else {
      cout << indent_str << "Int DCL   TDP Dynamic:" << endl;
      cout << indent_str_next << "TDP Dynamic = " << idcl->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << idcl->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << idcl->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << idcl->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << indent_str << "FP DCL:" << endl;
      cout << indent_str_next << "TDP Dynamic = " << fdcl->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << fdcl->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << fdcl->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << fdcl->rt_power.readOp.dynamic / executionTime << " W" << endl;
    }
  } else {
    if(coredynp.core_ty == OOO) {
      cout << indent_str_next << "Int Front End RAT    TDP Dynamic = " << iFRAT->rt_power.readOp.dynamic * clockRate << " W"
           << endl;
      cout << indent_str_next << "Int Front End RAT    Subthreshold Leakage = " << iFRAT->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Int Front End RAT    Gate Leakage = " << iFRAT->rt_power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "FP Front End RAT   TDP Dynamic = " << fFRAT->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "FP Front End RAT   Subthreshold Leakage = " << fFRAT->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "FP Front End RAT   Gate Leakage = " << fFRAT->rt_power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Free List   TDP Dynamic = " << ifreeL->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Free List   Subthreshold Leakage = " << ifreeL->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Free List   Gate Leakage = " << fFRAT->rt_power.readOp.gate_leakage << " W" << endl;
      if(coredynp.scheu_ty == PhysicalRegFile) {
        cout << indent_str_next << "Int Retire RAT   TDP Dynamic = " << iRRAT->rt_power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Int Retire RAT   Subthreshold Leakage = " << iRRAT->rt_power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Int Retire RAT   Gate Leakage = " << iRRAT->rt_power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "FP Retire RAT   TDP Dynamic = " << fRRAT->rt_power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "FP Retire RAT   Subthreshold Leakage = " << fRRAT->rt_power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "FP Retire RAT   Gate Leakage = " << fRRAT->rt_power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "FP Free List   TDP Dynamic = " << ffreeL->rt_power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "FP Free List   Subthreshold Leakage = " << ffreeL->rt_power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "FP Free List   Gate Leakage = " << fFRAT->rt_power.readOp.gate_leakage << " W" << endl;
      }
    } else {
      cout << indent_str_next << "Int DCL   TDP Dynamic = " << idcl->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Int DCL   Subthreshold Leakage = " << idcl->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Int DCL   Gate Leakage = " << idcl->rt_power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "FP DCL   TDP Dynamic = " << fdcl->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "FP DCL   Subthreshold Leakage = " << fdcl->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "FP DCL   Gate Leakage = " << fdcl->rt_power.readOp.gate_leakage << " W" << endl;
    }
  }
}

void SchedulerU::computeEnergy(bool is_tdp) {
  // init stats

  if(is_tdp) {
    // eka
    power.reset();
    if(coredynp.core_ty == OOO) {
      int_inst_window->stats_t.readAc.access  = int_inst_window->l_ip.num_search_ports;
      int_inst_window->stats_t.writeAc.access = int_inst_window->l_ip.num_wr_ports;
      int_inst_window->tdp_stats              = int_inst_window->stats_t;
      fp_inst_window->stats_t.readAc.access   = fp_inst_window->l_ip.num_search_ports;
      fp_inst_window->stats_t.writeAc.access  = fp_inst_window->l_ip.num_wr_ports;
      fp_inst_window->tdp_stats               = fp_inst_window->stats_t;
      ROB->stats_t.readAc.access              = ROB->l_ip.num_rd_ports;
      ROB->stats_t.writeAc.access             = ROB->l_ip.num_wr_ports;
      ROB->tdp_stats                          = ROB->stats_t;

    } else if(coredynp.multithreaded) {
      int_inst_window->stats_t.readAc.access  = int_inst_window->l_ip.num_search_ports;
      int_inst_window->stats_t.writeAc.access = int_inst_window->l_ip.num_wr_ports;
      int_inst_window->tdp_stats              = int_inst_window->stats_t;
    }

  } else { // rtp
    // eka
    rt_power.reset();
    if(coredynp.core_ty == OOO) {
      int_inst_window->stats_t.readAc.access  = XML->sys.core[ithCore].int_instructions;
      int_inst_window->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions;
      int_inst_window->rtp_stats              = int_inst_window->stats_t;
      fp_inst_window->stats_t.readAc.access   = XML->sys.core[ithCore].fp_instructions;
      fp_inst_window->stats_t.writeAc.access  = XML->sys.core[ithCore].fp_instructions;
      fp_inst_window->rtp_stats               = fp_inst_window->stats_t;
      ROB->stats_t.readAc.access              = XML->sys.core[ithCore].ROB_reads; // TODO: ROB need to be updated in RS based OOO
      ROB->stats_t.writeAc.access             = XML->sys.core[ithCore].ROB_writes;
      ROB->rtp_stats                          = ROB->stats_t;

    } else if(coredynp.multithreaded) {
      int_inst_window->stats_t.readAc.access =
          2 * XML->sys.core[ithCore].int_instructions + 2 * XML->sys.core[ithCore].fp_instructions;
      int_inst_window->stats_t.writeAc.access = XML->sys.core[ithCore].int_instructions + XML->sys.core[ithCore].fp_instructions;
      int_inst_window->rtp_stats              = int_inst_window->stats_t;
    }
  }

  // computation engine
  if(coredynp.core_ty == OOO) {
    int_inst_window->power_t.reset();
    fp_inst_window->power_t.reset();
    ROB->power_t.reset();
    // eka, may need to check the first call
    // int_inst_window->power.reset();
    // fp_inst_window->power.reset();
    // ROB->power.reset();
    int_inst_window->power_t.readOp.dynamic +=
        int_inst_window->local_result.power.searchOp.dynamic * int_inst_window->stats_t.readAc.access +
        +int_inst_window->local_result.power.writeOp.dynamic * int_inst_window->stats_t.writeAc.access +
        int_inst_window->stats_t.writeAc.access * instruction_selection->power.readOp.dynamic;

    fp_inst_window->power_t.readOp.dynamic +=
        fp_inst_window->local_result.power.searchOp.dynamic * fp_inst_window->stats_t.readAc.access +
        +fp_inst_window->local_result.power.writeOp.dynamic * fp_inst_window->stats_t.writeAc.access +
        fp_inst_window->stats_t.writeAc.access * instruction_selection->power.readOp.dynamic;

    ROB->power_t.readOp.dynamic += ROB->local_result.power.readOp.dynamic * ROB->stats_t.readAc.access +
                                   ROB->stats_t.writeAc.access * ROB->local_result.power.writeOp.dynamic;

  } else if(coredynp.multithreaded) {
    int_inst_window->power_t.reset();
    // eka, may need to check the first call
    // int_inst_window->power.reset();
    int_inst_window->power_t.readOp.dynamic +=
        int_inst_window->local_result.power.searchOp.dynamic * int_inst_window->stats_t.readAc.access +
        +int_inst_window->local_result.power.writeOp.dynamic * int_inst_window->stats_t.writeAc.access +
        int_inst_window->stats_t.writeAc.access * instruction_selection->power.readOp.dynamic;
  }

  // assign values
  if(is_tdp) {
    if(coredynp.core_ty == OOO) {
      int_inst_window->power =
          int_inst_window->power_t + (int_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      fp_inst_window->power =
          fp_inst_window->power_t + (fp_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      ROB->power = ROB->power_t + ROB->local_result.power * pppm_lkg;
      power      = power + int_inst_window->power + fp_inst_window->power + ROB->power;

    } else if(coredynp.multithreaded) {
      //			set_pppm(pppm_t, XML->sys.core[ithCore].issue_width,1, 1, 1);
      int_inst_window->power =
          int_inst_window->power_t + (int_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      power = power + int_inst_window->power;
    }

  } else { // rtp
    if(coredynp.core_ty == OOO) {
      int_inst_window->rt_power =
          int_inst_window->power_t + (int_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      fp_inst_window->rt_power =
          fp_inst_window->power_t + (fp_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      ROB->rt_power = ROB->power_t + ROB->local_result.power * pppm_lkg;

      rt_power = rt_power + int_inst_window->rt_power + fp_inst_window->rt_power + ROB->rt_power;

    } else if(coredynp.multithreaded) {
      //			set_pppm(pppm_t, XML->sys.core[ithCore].issue_width,1, 1, 1);
      int_inst_window->rt_power =
          int_inst_window->power_t + (int_inst_window->local_result.power + instruction_selection->power) * pppm_lkg;
      rt_power = rt_power + int_inst_window->rt_power;
    }
  }
  //	set_pppm(pppm_t, XML->sys.core[ithCore].issue_width,1, 1, 1);
  //	cout<<"Scheduler power="<<power.readOp.dynamic<<"leakage="<<power.readOp.leakage<<endl;
  //	cout<<"IW="<<int_inst_window->local_result.power.searchOp.dynamic * int_inst_window->stats_t.readAc.access +
  //    + int_inst_window->local_result.power.writeOp.dynamic *
  //    int_inst_window->stats_t.writeAc.access<<"leakage="<<int_inst_window->local_result.power.readOp.leakage<<endl;
  //	cout<<"selection"<<instruction_selection->power.readOp.dynamic<<"leakage"<<instruction_selection->power.readOp.leakage<<endl;
}

void SchedulerU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    if(coredynp.core_ty == OOO) {
      cout << indent_str << "Instruction Window:" << endl;
      cout << indent_str_next << "Area = " << int_inst_window->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << int_inst_window->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << int_inst_window->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << int_inst_window->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << int_inst_window->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << "FP Instruction Window:" << endl;
      cout << indent_str_next << "Area = " << fp_inst_window->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << fp_inst_window->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << fp_inst_window->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << fp_inst_window->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << fp_inst_window->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      cout << indent_str << "ROB:" << endl;
      cout << indent_str_next << "Area = " << ROB->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << ROB->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << ROB->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << ROB->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << ROB->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
    } else if(coredynp.multithreaded) {
      cout << indent_str << "Instruction Window:" << endl;
      cout << indent_str_next << "Area = " << int_inst_window->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << int_inst_window->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << int_inst_window->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << int_inst_window->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << int_inst_window->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
    }
  } else {
    if(coredynp.core_ty == OOO) {
      cout << indent_str_next << "Instruction Window    TDP Dynamic = " << int_inst_window->rt_power.readOp.dynamic * clockRate
           << " W" << endl;
      cout << indent_str_next << "Instruction Window    Subthreshold Leakage = " << int_inst_window->rt_power.readOp.leakage << " W"
           << endl;
      cout << indent_str_next << "Instruction Window    Gate Leakage = " << int_inst_window->rt_power.readOp.gate_leakage << " W"
           << endl;
      cout << indent_str_next << "FP Instruction Window   TDP Dynamic = " << fp_inst_window->rt_power.readOp.dynamic * clockRate
           << " W" << endl;
      cout << indent_str_next << "FP Instruction Window   Subthreshold Leakage = " << fp_inst_window->rt_power.readOp.leakage
           << " W" << endl;
      cout << indent_str_next << "FP Instruction Window   Gate Leakage = " << fp_inst_window->rt_power.readOp.gate_leakage << " W"
           << endl;
      cout << indent_str_next << "ROB   TDP Dynamic = " << ROB->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "ROB   Subthreshold Leakage = " << ROB->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "ROB   Gate Leakage = " << ROB->rt_power.readOp.gate_leakage << " W" << endl;
    } else if(coredynp.multithreaded) {
      cout << indent_str_next << "Instruction Window    TDP Dynamic = " << int_inst_window->rt_power.readOp.dynamic * clockRate
           << " W" << endl;
      cout << indent_str_next << "Instruction Window    Subthreshold Leakage = " << int_inst_window->rt_power.readOp.leakage << " W"
           << endl;
      cout << indent_str_next << "Instruction Window    Gate Leakage = " << int_inst_window->rt_power.readOp.gate_leakage << " W"
           << endl;
    }
  }
}

void LoadStoreU::computeEnergy(bool is_tdp) {

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    dcache.caches->stats_t.readAc.access  = 0.67 * dcache.caches->l_ip.num_rw_ports;
    dcache.caches->stats_t.readAc.miss    = 0;
    dcache.caches->stats_t.readAc.hit     = dcache.caches->stats_t.readAc.access - dcache.caches->stats_t.readAc.miss;
    dcache.caches->stats_t.writeAc.access = 0.33 * dcache.caches->l_ip.num_rw_ports;
    dcache.caches->stats_t.writeAc.miss   = 0;
    dcache.caches->stats_t.writeAc.hit    = dcache.caches->stats_t.writeAc.access - dcache.caches->stats_t.writeAc.miss;
    dcache.caches->tdp_stats              = dcache.caches->stats_t;

    dcache.missb->stats_t.readAc.access  = 0; // dcache.missb->l_ip.num_search_ports;
    dcache.missb->stats_t.writeAc.access = 0; // dcache.missb->l_ip.num_search_ports;
    dcache.missb->tdp_stats              = dcache.missb->stats_t;

    dcache.ifb->stats_t.readAc.access  = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.ifb->stats_t.writeAc.access = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.ifb->tdp_stats              = dcache.ifb->stats_t;

    dcache.prefetchb->stats_t.readAc.access  = 0; // dcache.prefetchb->l_ip.num_search_ports;
    dcache.prefetchb->stats_t.writeAc.access = 0; // dcache.ifb->l_ip.num_search_ports;
    dcache.prefetchb->tdp_stats              = dcache.prefetchb->stats_t;

    dcache.wbb->stats_t.readAc.access  = 0; // dcache.wbb->l_ip.num_search_ports;
    dcache.wbb->stats_t.writeAc.access = 0; // dcache.wbb->l_ip.num_search_ports;
    dcache.wbb->tdp_stats              = dcache.wbb->stats_t;

    if(XML->sys.core[ithCore].scoore) {
      if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
        VPCfilter.caches->stats_t.readAc.access  = 0.67 * VPCfilter.caches->l_ip.num_rw_ports;
        VPCfilter.caches->stats_t.readAc.miss    = 0;
        VPCfilter.caches->stats_t.readAc.hit     = VPCfilter.caches->stats_t.readAc.access - VPCfilter.caches->stats_t.readAc.miss;
        VPCfilter.caches->stats_t.writeAc.access = 0.33 * VPCfilter.caches->l_ip.num_rw_ports;
        VPCfilter.caches->stats_t.writeAc.miss   = 0;
        VPCfilter.caches->stats_t.writeAc.hit = VPCfilter.caches->stats_t.writeAc.access - VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.caches->tdp_stats           = VPCfilter.caches->stats_t;

        VPCfilter.missb->stats_t.readAc.access  = 0; // VPCfilter.missb->l_ip.num_search_ports;
        VPCfilter.missb->stats_t.writeAc.access = 0; // VPCfilter.missb->l_ip.num_search_ports;
        VPCfilter.missb->tdp_stats              = VPCfilter.missb->stats_t;

        VPCfilter.ifb->stats_t.readAc.access  = 0; // VPCfilter.ifb->l_ip.num_search_ports;
        VPCfilter.ifb->stats_t.writeAc.access = 0; // VPCfilter.ifb->l_ip.num_search_ports;
        VPCfilter.ifb->tdp_stats              = VPCfilter.ifb->stats_t;

        VPCfilter.prefetchb->stats_t.readAc.access  = 0; // VPCfilter.prefetchb->l_ip.num_search_ports;
        VPCfilter.prefetchb->stats_t.writeAc.access = 0; // VPCfilter.ifb->l_ip.num_search_ports;
        VPCfilter.prefetchb->tdp_stats              = VPCfilter.prefetchb->stats_t;

        VPCfilter.wbb->stats_t.readAc.access  = 0; // VPCfilter.wbb->l_ip.num_search_ports;
        VPCfilter.wbb->stats_t.writeAc.access = 0; // VPCfilter.wbb->l_ip.num_search_ports;
        VPCfilter.wbb->tdp_stats              = VPCfilter.wbb->stats_t;
      }
      vpc_buffer->stats_t.readAc.access = vpc_buffer->stats_t.writeAc.access = vpc_buffer->l_ip.num_search_ports;
      vpc_buffer->tdp_stats                                                  = vpc_buffer->stats_t;
      // strb->stats_t.readAc.access = strb->stats_t.writeAc.access = strb->l_ip.num_search_ports;
      // strb->tdp_stats = strb->stats_t;
      // ldrb->stats_t.readAc.access = ldrb->stats_t.writeAc.access = ldrb->l_ip.num_search_ports;
      // ldrb->tdp_stats = ldrb->stats_t;
    } else {
      LSQ->stats_t.readAc.access = LSQ->stats_t.writeAc.access = LSQ->l_ip.num_search_ports;
      LSQ->tdp_stats                                           = LSQ->stats_t;
      if(coredynp.core_ty == OOO) {
        LoadQ->stats_t.readAc.access = LoadQ->stats_t.writeAc.access = LoadQ->l_ip.num_search_ports;
        LoadQ->tdp_stats                                             = LoadQ->stats_t;
        ssit->stats_t.readAc.access                                  = ssit->l_ip.num_wr_ports;
        ssit->stats_t.writeAc.access                                 = ssit->l_ip.num_rd_ports;
        ssit->tdp_stats                                              = ssit->stats_t;
        lfst->stats_t.readAc.access = lfst->stats_t.writeAc.access = lfst->l_ip.num_search_ports;
        lfst->tdp_stats                                            = lfst->stats_t;
      }
    }
  } else {
    // eka <=awesome
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    dcache.caches->stats_t.readAc.access  = XML->sys.core[ithCore].dcache.read_accesses;
    dcache.caches->stats_t.readAc.miss    = XML->sys.core[ithCore].dcache.read_misses;
    dcache.caches->stats_t.readAc.hit     = dcache.caches->stats_t.readAc.access - dcache.caches->stats_t.readAc.miss;
    dcache.caches->stats_t.writeAc.access = XML->sys.core[ithCore].dcache.write_accesses;
    dcache.caches->stats_t.writeAc.miss   = XML->sys.core[ithCore].dcache.write_misses;
    dcache.caches->stats_t.writeAc.hit    = dcache.caches->stats_t.writeAc.access - dcache.caches->stats_t.writeAc.miss;
    dcache.caches->rtp_stats              = dcache.caches->stats_t;

    dcache.missb->stats_t.readAc.access  = dcache.caches->stats_t.readAc.miss;
    dcache.missb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
#if 0
      if (XML->sys.core[ithCore].scoore){
        dcache.missb->stats_t.readAc.access  = 0;
        dcache.missb->stats_t.writeAc.access = 0;
        //0.4 comes from CACTI as the tag bank dynamic power for the VPC, which drops write_misses on the floor and only accesses the tag bank.
        dcache.caches->stats_t.writeAc.miss   = 0.4*XML->sys.core[ithCore].dcache.write_misses;
      }
#endif

    dcache.missb->rtp_stats            = dcache.missb->stats_t;
    dcache.ifb->stats_t.readAc.access  = dcache.caches->stats_t.readAc.miss;
    dcache.ifb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.ifb->rtp_stats              = dcache.ifb->stats_t;

    dcache.prefetchb->stats_t.readAc.access  = 0; // dcache.caches->stats_t.writeAc.miss;
    dcache.prefetchb->stats_t.writeAc.access = 0; // dcache.caches->stats_t.writeAc.miss;
    dcache.prefetchb->rtp_stats              = dcache.prefetchb->stats_t;

    dcache.wbb->stats_t.readAc.access  = dcache.caches->stats_t.readAc.miss;
    dcache.wbb->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.miss;
    dcache.wbb->rtp_stats              = dcache.wbb->stats_t;

    if(XML->sys.core[ithCore].scoore)

    {

      vpc_buffer->stats_t.readAc.access =
          XML->sys.core[ithCore].vpc_buffer.read_hits + XML->sys.core[ithCore].vpc_buffer.read_misses;
      vpc_buffer->rtp_stats = vpc_buffer->stats_t;

      vpc_specbw->stats_t.readAc.access  = dcache.caches->stats_t.writeAc.access;
      vpc_specbw->stats_t.writeAc.access = dcache.caches->stats_t.writeAc.access;
      vpc_specbw->rtp_stats              = vpc_specbw->stats_t;

      vpc_specbr->stats_t.readAc.access  = dcache.caches->stats_t.readAc.access;
      vpc_specbr->stats_t.writeAc.access = dcache.caches->stats_t.readAc.access;
      vpc_specbr->rtp_stats              = vpc_specbr->stats_t;

      if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
        VPCfilter.caches->stats_t.readAc.access  = XML->sys.core[ithCore].VPCfilter.read_accesses;
        VPCfilter.caches->stats_t.readAc.miss    = XML->sys.core[ithCore].VPCfilter.read_misses;
        VPCfilter.caches->stats_t.readAc.hit     = VPCfilter.caches->stats_t.readAc.access - VPCfilter.caches->stats_t.readAc.miss;
        VPCfilter.caches->stats_t.writeAc.access = XML->sys.core[ithCore].VPCfilter.write_accesses;
        VPCfilter.caches->stats_t.writeAc.miss   = XML->sys.core[ithCore].VPCfilter.write_misses;
        VPCfilter.caches->stats_t.writeAc.hit   = VPCfilter.caches->stats_t.writeAc.access - VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.caches->rtp_stats             = VPCfilter.caches->stats_t;
        VPCfilter.missb->stats_t.readAc.access  = 0; // VPCfilter.caches->stats_t.readAc.access;
        VPCfilter.missb->stats_t.writeAc.access = 0; // VPCfilter.caches->stats_t.writeAc.access;
        VPCfilter.missb->rtp_stats              = VPCfilter.missb->stats_t;

        VPCfilter.ifb->stats_t.readAc.access  = VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.ifb->stats_t.writeAc.access = VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.ifb->rtp_stats              = VPCfilter.ifb->stats_t;

        VPCfilter.prefetchb->stats_t.readAc.access  = 0; // VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.prefetchb->stats_t.writeAc.access = 0; // VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.prefetchb->rtp_stats              = VPCfilter.prefetchb->stats_t;

        VPCfilter.wbb->stats_t.readAc.access  = VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.wbb->stats_t.writeAc.access = VPCfilter.caches->stats_t.writeAc.miss;
        VPCfilter.wbb->rtp_stats              = VPCfilter.wbb->stats_t;
      }
      // strb->stats_t.readAc.access  = XML->sys.core[ithCore].strb.read_accesses;
      // strb->stats_t.writeAc.access = XML->sys.core[ithCore].strb.write_accesses;
      // strb->rtp_stats = strb->stats_t;
      // ldrb->stats_t.readAc.access  = XML->sys.core[ithCore].ldrb.read_accesses;
      // ldrb->stats_t.writeAc.access = XML->sys.core[ithCore].ldrb.write_accesses;
      // ldrb->rtp_stats = ldrb->stats_t;
    } else {
      LSQ->stats_t.readAc.access  = XML->sys.core[ithCore].LSQ.read_accesses;
      LSQ->stats_t.writeAc.access = XML->sys.core[ithCore].LSQ.write_accesses;
      LSQ->rtp_stats              = LSQ->stats_t;

      if(coredynp.core_ty == OOO) {
        LoadQ->stats_t.readAc.access  = XML->sys.core[ithCore].LoadQ.read_accesses;
        LoadQ->stats_t.writeAc.access = XML->sys.core[ithCore].LoadQ.write_accesses;
        LoadQ->rtp_stats              = LoadQ->stats_t;
        ssit->stats_t.readAc.access   = XML->sys.core[ithCore].ssit.read_accesses;
        ssit->stats_t.writeAc.access  = XML->sys.core[ithCore].ssit.write_accesses;
        ssit->rtp_stats               = ssit->stats_t;
        lfst->stats_t.readAc.access   = XML->sys.core[ithCore].lfst.read_accesses;
        lfst->stats_t.writeAc.access  = XML->sys.core[ithCore].lfst.write_accesses;
        lfst->rtp_stats               = lfst->stats_t;
      }
    }
  }

  dcache.power_t.reset();
  dcachecc.power_t.reset();
  VPCfilter.power_t.reset();
  if(XML->sys.core[ithCore].scoore) {
    if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
      VPCfilter.power_t.reset();
    }
    vpc_buffer->power_t.reset();
    // strb->power_t.reset();
    // ldrb->power_t.reset();
  } else {
    LSQ->power_t.reset();
  }
  // eka, may need to check the first call
  // dcache.power.reset();
  // LSQ->power.reset();
  if(XML->sys.core[ithCore].scoore) {
    dcache.power_t.readOp.dynamic +=
        (dcache.caches->stats_t.readAc.access * dcache.caches->local_result.power.readOp.dynamic +
         0.4 * dcache.caches->stats_t.readAc.miss * dcache.caches->local_result.tag_array2->power.readOp.dynamic +
         0.4 * dcache.caches->stats_t.writeAc.miss * dcache.caches->local_result.tag_array2->power.writeOp.dynamic +
         dcache.caches->stats_t.writeAc.access *
             dcache.caches->local_result.power.writeOp.dynamic); // write miss will generate a write later
  } else {
    dcache.power_t.readOp.dynamic +=
        (dcache.caches->stats_t.readAc.access * dcache.caches->local_result.power.readOp.dynamic +
         dcache.caches->stats_t.readAc.miss * dcache.caches->local_result.tag_array2->power.readOp.dynamic +
         dcache.caches->stats_t.writeAc.miss * dcache.caches->local_result.tag_array2->power.writeOp.dynamic +
         dcache.caches->stats_t.writeAc.access *
             dcache.caches->local_result.power.writeOp.dynamic); // write miss will generate a write later
  }

  dcachecc.power_t.readOp.dynamic =
      (dcache.missb->stats_t.readAc.access * dcache.missb->local_result.power.searchOp.dynamic +
       dcache.missb->stats_t.writeAc.access *
           dcache.missb->local_result.power.writeOp.dynamic); // each access to missb involves a CAM and a write

  dcachecc.power_t.readOp.dynamic += (dcache.ifb->stats_t.readAc.access * dcache.ifb->local_result.power.searchOp.dynamic +
                                      dcache.ifb->stats_t.writeAc.access * dcache.ifb->local_result.power.writeOp.dynamic);

  dcachecc.power_t.readOp.dynamic +=
      (dcache.prefetchb->stats_t.readAc.access * dcache.prefetchb->local_result.power.searchOp.dynamic +
       dcache.prefetchb->stats_t.writeAc.access * dcache.prefetchb->local_result.power.writeOp.dynamic);

  dcachecc.power_t.readOp.dynamic += (dcache.wbb->stats_t.readAc.access * dcache.wbb->local_result.power.searchOp.dynamic +
                                      dcache.wbb->stats_t.writeAc.access * dcache.wbb->local_result.power.writeOp.dynamic);

  if(XML->sys.core[ithCore].scoore) {
    if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
      VPCfilter.power_t.readOp.dynamic +=
          (VPCfilter.caches->stats_t.readAc.access * VPCfilter.caches->local_result.power.readOp.dynamic +
           VPCfilter.caches->stats_t.readAc.miss * VPCfilter.caches->local_result.tag_array2->power.readOp.dynamic +
           VPCfilter.caches->stats_t.writeAc.miss * VPCfilter.caches->local_result.tag_array2->power.writeOp.dynamic +
           VPCfilter.caches->stats_t.writeAc.access *
               VPCfilter.caches->local_result.power.writeOp.dynamic); // write miss will generate a write later

      VPCfilter.power_t.readOp.dynamic +=
          VPCfilter.missb->stats_t.readAc.access * VPCfilter.missb->local_result.power.searchOp.dynamic +
          VPCfilter.missb->stats_t.writeAc.access *
              VPCfilter.missb->local_result.power.writeOp.dynamic; // each access to missb involves a CAM and a write

      VPCfilter.power_t.readOp.dynamic +=
          VPCfilter.ifb->stats_t.readAc.access * VPCfilter.ifb->local_result.power.searchOp.dynamic +
          VPCfilter.ifb->stats_t.writeAc.access * VPCfilter.ifb->local_result.power.writeOp.dynamic;

      VPCfilter.power_t.readOp.dynamic +=
          VPCfilter.prefetchb->stats_t.readAc.access * VPCfilter.prefetchb->local_result.power.searchOp.dynamic +
          VPCfilter.prefetchb->stats_t.writeAc.access * VPCfilter.prefetchb->local_result.power.writeOp.dynamic;

      VPCfilter.power_t.readOp.dynamic +=
          VPCfilter.wbb->stats_t.readAc.access * VPCfilter.wbb->local_result.power.searchOp.dynamic +
          VPCfilter.wbb->stats_t.writeAc.access * VPCfilter.wbb->local_result.power.writeOp.dynamic;
    }
    vpc_buffer->power_t.readOp.dynamic += vpc_buffer->stats_t.readAc.access * (vpc_buffer->local_result.power.searchOp.dynamic +
                                                                               vpc_buffer->local_result.power.readOp.dynamic);
    // strb->power_t.readOp.dynamic  +=  strb->stats_t.readAc.access*(strb->local_result.power.searchOp.dynamic +
    // strb->local_result.power.readOp.dynamic) + strb->stats_t.writeAc.access*strb->local_result.power.writeOp.dynamic;//every
    // memory access invloves at least two operations on strb ldrb->power_t.readOp.dynamic  +=
    // ldrb->stats_t.readAc.access*(ldrb->local_result.power.searchOp.dynamic + ldrb->local_result.power.readOp.dynamic) +
    // ldrb->stats_t.writeAc.access*ldrb->local_result.power.writeOp.dynamic;//every memory access invloves at least two operations
    // on ldrb
  } else {
    LSQ->power_t.readOp.dynamic +=
        LSQ->stats_t.readAc.access * (LSQ->local_result.power.searchOp.dynamic + LSQ->local_result.power.readOp.dynamic) +
        LSQ->stats_t.writeAc.access *
            LSQ->local_result.power.writeOp.dynamic; // every memory access invloves at least two operations on LSQ
    if(coredynp.core_ty == OOO) {
      LoadQ->power_t.reset();
      // eka, may need to check the first call
      // LoadQ->power.reset();
      LoadQ->power_t.readOp.dynamic +=
          LoadQ->stats_t.readAc.access * (LoadQ->local_result.power.searchOp.dynamic + LoadQ->local_result.power.readOp.dynamic) +
          LoadQ->stats_t.writeAc.access *
              LoadQ->local_result.power.writeOp.dynamic; // every memory access invloves at least two operations on LoadQ

      ssit->power_t.reset();
      // eka, may need to check the first call
      // ssit->power.reset();
      ssit->power_t.readOp.dynamic +=
          ssit->stats_t.readAc.access * (ssit->local_result.power.readOp.dynamic) +
          ssit->stats_t.writeAc.access *
              ssit->local_result.power.writeOp.dynamic; // every memory access invloves at least two operations on LoadQ

      lfst->power_t.reset();
      // eka, may need to check the first call
      // lfst->power.reset();
      lfst->power_t.readOp.dynamic +=
          lfst->stats_t.readAc.access * (lfst->local_result.power.readOp.dynamic) +
          lfst->stats_t.writeAc.access *
              lfst->local_result.power.writeOp.dynamic; // every memory access invloves at least two operations on LoadQ
    }
  }
  if(is_tdp) {
    dcache.power   = dcache.power_t + (dcache.caches->local_result.power) * pppm_lkg;
    dcachecc.power = dcachecc.power_t + (dcache.missb->local_result.power + dcache.ifb->local_result.power +
                                         // dcache.prefetchb->local_result.power +
                                         dcache.wbb->local_result.power) *
                                            pppm_Isub;

    if(XML->sys.core[ithCore].scoore) {
      if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
        VPCfilter.power = VPCfilter.power_t + (VPCfilter.caches->local_result.power) * pppm_lkg +
                          (VPCfilter.missb->local_result.power + VPCfilter.ifb->local_result.power +
                           // VPCfilter.prefetchb->local_result.power +
                           VPCfilter.wbb->local_result.power) *
                              pppm_Isub;
      }
      vpc_buffer->power = vpc_buffer->power_t + vpc_buffer->local_result.power * pppm_lkg;
      power             = power + dcache.power + dcachecc.power + vpc_buffer->power + VPCfilter.power;
      // strb->power = strb->power_t + strb->local_result.power *pppm_lkg;
      // power     = power + dcache.power + strb->power;
      // ldrb->power = ldrb->power_t + ldrb->local_result.power *pppm_lkg;
      // power     += ldrb->power;
    } else {
      LSQ->power = LSQ->power_t + LSQ->local_result.power * pppm_lkg;
      power      = power + dcache.power + dcachecc.power + LSQ->power;

      if(coredynp.core_ty == OOO) {
        LoadQ->power = LoadQ->power_t + LoadQ->local_result.power * pppm_lkg;
        power        = power + LoadQ->power;
        ssit->power  = ssit->power_t + ssit->local_result.power * pppm_lkg;
        power        = power + ssit->power;
        lfst->power  = lfst->power_t + lfst->local_result.power * pppm_lkg;
        power        = power + lfst->power;
      }
    }
  } else {
    dcache.rt_power   = dcache.power_t + (dcache.caches->local_result.power) * pppm_lkg;
    dcachecc.rt_power = dcachecc.power_t + (dcache.missb->local_result.power + dcache.ifb->local_result.power +
                                            // dcache.prefetchb->local_result.power +
                                            dcache.wbb->local_result.power) *
                                               pppm_lkg;
    if(XML->sys.core[ithCore].scoore) {
      if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
        VPCfilter.rt_power = VPCfilter.power_t + (VPCfilter.caches->local_result.power + VPCfilter.missb->local_result.power +
                                                  VPCfilter.ifb->local_result.power +
                                                  // VPCfilter.prefetchb->local_result.power +
                                                  VPCfilter.wbb->local_result.power) *
                                                     pppm_lkg;

        rt_power = rt_power + VPCfilter.rt_power;
      }
      vpc_specbw->rt_power.readOp.dynamic = vpc_specbw->stats_t.readAc.access * vpc_specbw->local_result.power.searchOp.dynamic +
                                            vpc_specbw->stats_t.writeAc.access * vpc_specbw->local_result.power.writeOp.dynamic;

      vpc_specbr->rt_power.readOp.dynamic = vpc_specbr->stats_t.readAc.access * vpc_specbr->local_result.power.searchOp.dynamic +
                                            vpc_specbr->stats_t.writeAc.access * vpc_specbr->local_result.power.writeOp.dynamic;

      vpc_buffer->rt_power = vpc_buffer->power_t + vpc_buffer->local_result.power * pppm_lkg;
      rt_power             = rt_power + vpc_buffer->rt_power + vpc_specbr->rt_power + vpc_specbw->rt_power;
      // strb->rt_power = strb->power_t + strb->local_result.power*pppm_lkg;
      // rt_power     = rt_power + dcache.rt_power;// + strb->rt_power;
      // rb->rt_power = ldrb->power_t + ldrb->local_result.power*pppm_lkg;
      // rt_power     += ldrb->rt_power;
    } else {
      LSQ->rt_power = LSQ->power_t + LSQ->local_result.power * pppm_lkg;
      rt_power      = rt_power + dcache.rt_power + LSQ->rt_power;

      if(coredynp.core_ty == OOO) {
        LoadQ->rt_power = LoadQ->power_t + LoadQ->local_result.power * pppm_lkg;
        rt_power        = rt_power + LoadQ->rt_power;
        ssit->rt_power  = ssit->power_t + ssit->local_result.power * pppm_lkg;
        rt_power        = rt_power + ssit->rt_power;
        lfst->rt_power  = lfst->power_t + lfst->local_result.power * pppm_lkg;
        rt_power        = rt_power + lfst->rt_power;
      }
    }
  }
}

void LoadStoreU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << indent_str << "Data Cache:" << endl;
    cout << indent_str_next << "Area = " << dcache.area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << dcache.power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << dcache.power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << dcache.power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << dcache.rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << indent_str_next << "Dynamic per read access = "
         << dcache.caches->local_result.power.readOp.dynamic + dcache.caches->local_result.tag_array2->power.readOp.dynamic << " W"
         << endl;
    if(coredynp.core_ty == Inorder) {
      cout << indent_str << "Load/Store Queue:" << endl;
      cout << indent_str_next << "Area = " << LSQ->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << LSQ->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << LSQ->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << LSQ->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << LSQ->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << indent_str_next << "Dynamic per read access = " << LSQ->local_result.power.readOp.dynamic << " W" << endl;

      cout << endl;
    } else

    {

      if(XML->sys.core[ithCore].scoore) {
        cout << indent_str << "vpc:" << endl;
        if(XML->sys.core[ithCore].VPCfilter.dcache_config[0] > 0) {
          cout << indent_str_next << "Area = " << VPCfilter.area.get_area() * 1e-6 << " mm^2" << endl;
          cout << indent_str_next << "TDP Dynamic = " << VPCfilter.power.readOp.dynamic * clockRate << " W" << endl;
          cout << indent_str_next << "Subthreshold Leakage = " << VPCfilter.power.readOp.leakage << " W" << endl;
          cout << indent_str_next << "Gate Leakage = " << VPCfilter.power.readOp.gate_leakage << " W" << endl;
          cout << indent_str_next << "Runtime Dynamic = " << VPCfilter.rt_power.readOp.dynamic / executionTime << " W" << endl;
          cout << endl;
        }
        cout << indent_str << "StoreQ:" << endl;
        cout << indent_str_next << "Area = " << vpc_buffer->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << vpc_buffer->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << vpc_buffer->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << vpc_buffer->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << vpc_buffer->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << endl;
      } else {

        cout << indent_str << "LoadQ:" << endl;
        cout << indent_str_next << "Area = " << LoadQ->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << LoadQ->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << LoadQ->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << LoadQ->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << LoadQ->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << indent_str_next << "Dynamic per read access = " << LoadQ->local_result.power.readOp.dynamic << " W" << endl;
        cout << endl;
        cout << indent_str << "StoreQ:" << endl;
        cout << indent_str_next << "Area = " << LSQ->area.get_area() * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << LSQ->power.readOp.dynamic * clockRate << " W" << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << LSQ->power.readOp.leakage << " W" << endl;
        cout << indent_str_next << "Gate Leakage = " << LSQ->power.readOp.gate_leakage << " W" << endl;
        cout << indent_str_next << "Runtime Dynamic = " << LSQ->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << indent_str_next << "Dynamic per read access = " << LSQ->local_result.power.readOp.dynamic << " W" << endl;

        cout << endl;
        cout << indent_str << "StoreSet:" << endl;
        cout << indent_str_next << "Area = " << (ssit->area.get_area() + lfst->area.get_area()) * 1e-6 << " mm^2" << endl;
        cout << indent_str_next << "TDP Dynamic = " << (ssit->power.readOp.dynamic + lfst->power.readOp.dynamic) * clockRate << " W"
             << endl;
        cout << indent_str_next << "Subthreshold Leakage = " << ssit->power.readOp.leakage + lfst->power.readOp.leakage << " W"
             << endl;
        cout << indent_str_next << "Gate Leakage = " << ssit->power.readOp.gate_leakage + lfst->power.readOp.gate_leakage << " W"
             << endl;
        cout << indent_str_next << "Runtime Dynamic = "
             << ssit->rt_power.readOp.dynamic / executionTime + lfst->rt_power.readOp.dynamic / executionTime << " W" << endl;
        cout << indent_str_next
             << "Dynamic per read access = " << ssit->local_result.power.readOp.dynamic + lfst->local_result.power.readOp.dynamic
             << " W" << endl;
        cout << endl;
      }
    }
  } else {
    cout << indent_str_next << "Data Cache    TDP Dynamic = " << dcache.rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Data Cache    Subthreshold Leakage = " << dcache.rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Data Cache    Gate Leakage = " << dcache.rt_power.readOp.gate_leakage << " W" << endl;
    if(coredynp.core_ty == Inorder) {
      cout << indent_str_next << "Load/Store Queue   TDP Dynamic = " << LSQ->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Load/Store Queue   Subthreshold Leakage = " << LSQ->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Load/Store Queue   Gate Leakage = " << LSQ->rt_power.readOp.gate_leakage << " W" << endl;
    } else {
      cout << indent_str_next << "LoadQ   TDP Dynamic = " << LoadQ->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "LoadQ   Subthreshold Leakage = " << LoadQ->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "LoadQ   Gate Leakage = " << LoadQ->rt_power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "StoreQ   TDP Dynamic = " << LSQ->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "StoreQ   Subthreshold Leakage = " << LSQ->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "StoreQ   Gate Leakage = " << LSQ->rt_power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "TDP Dynamic = " << (ssit->rt_power.readOp.dynamic + lfst->power.readOp.dynamic) * clockRate
           << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << ssit->rt_power.readOp.leakage + lfst->rt_power.readOp.leakage << " W"
           << endl;
      cout << indent_str_next << "Gate Leakage = " << ssit->rt_power.readOp.gate_leakage + lfst->rt_power.readOp.gate_leakage
           << " W" << endl;
    }
  }
}

void MemManU::computeEnergy(bool is_tdp) {

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    itlb->stats_t.readAc.access = itlb->l_ip.num_search_ports;
    itlb->stats_t.readAc.miss   = 0;
    itlb->stats_t.readAc.hit    = itlb->stats_t.readAc.access - itlb->stats_t.readAc.miss;
    itlb->tdp_stats             = itlb->stats_t;

    dtlb->stats_t.readAc.access = dtlb->l_ip.num_search_ports;
    dtlb->stats_t.readAc.miss   = 0;
    dtlb->stats_t.readAc.hit    = dtlb->stats_t.readAc.access - dtlb->stats_t.readAc.miss;
    dtlb->tdp_stats             = dtlb->stats_t;
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    itlb->stats_t.readAc.access = XML->sys.core[ithCore].itlb.total_accesses;
    itlb->stats_t.readAc.miss   = XML->sys.core[ithCore].itlb.total_misses;
    itlb->stats_t.readAc.hit    = itlb->stats_t.readAc.access - itlb->stats_t.readAc.miss;
    itlb->rtp_stats             = itlb->stats_t;

    dtlb->stats_t.readAc.access = XML->sys.core[ithCore].dtlb.total_accesses;
    dtlb->stats_t.readAc.miss   = XML->sys.core[ithCore].dtlb.total_misses;
    dtlb->stats_t.readAc.hit    = dtlb->stats_t.readAc.access - dtlb->stats_t.readAc.miss;
    dtlb->rtp_stats             = dtlb->stats_t;
  }

  itlb->power_t.reset();
  dtlb->power_t.reset();
  // eka, may need to check the first call
  // itlb->power.reset();
  // dtlb->power.reset();
  itlb->power_t.readOp.dynamic +=
      itlb->stats_t.readAc.access *
          itlb->local_result.power.searchOp.dynamic // FA spent most power in tag so use access not hit t ocover the misses
      + itlb->stats_t.readAc.miss * itlb->local_result.power.writeOp.dynamic;
  dtlb->power_t.readOp.dynamic +=
      dtlb->stats_t.readAc.access *
          dtlb->local_result.power.searchOp.dynamic // FA spent most power in tag so use access not hit t ocover the misses
      + dtlb->stats_t.readAc.miss * dtlb->local_result.power.writeOp.dynamic;

  if(is_tdp) {
    itlb->power = itlb->power_t + itlb->local_result.power * pppm_lkg;
    dtlb->power = dtlb->power_t + dtlb->local_result.power * pppm_lkg;
    power       = power + itlb->power + dtlb->power;
  } else {
    itlb->rt_power = itlb->power_t + itlb->local_result.power * pppm_lkg;
    dtlb->rt_power = dtlb->power_t + dtlb->local_result.power * pppm_lkg;
    rt_power       = rt_power + itlb->rt_power + dtlb->rt_power;
  }
}

void MemManU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << indent_str << "Itlb:" << endl;
    cout << indent_str_next << "Area = " << itlb->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << itlb->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << itlb->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << itlb->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << itlb->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Dtlb:" << endl;
    cout << indent_str_next << "Area = " << dtlb->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << dtlb->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << dtlb->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << dtlb->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << dtlb->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << dtlb->local_result.power.readOp.dynamic

         << " W" << endl;

    cout << endl;
  } else {
    cout << indent_str_next << "Itlb    TDP Dynamic = " << itlb->rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Itlb    Subthreshold Leakage = " << itlb->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Itlb    Gate Leakage = " << itlb->rt_power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Dtlb   TDP Dynamic = " << dtlb->rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Dtlb   Subthreshold Leakage = " << dtlb->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Dtlb   Gate Leakage = " << dtlb->rt_power.readOp.gate_leakage << " W" << endl;
  }
}

void RegFU::computeEnergy(bool is_tdp) {
  /*
   * Architecture RF and physical RF cannot be present at the same time.
   * Therefore, the RF stats can only refer to either ARF or PRF;
   * And the same stats can be used for both.
   */

  if(!exist)
    return;

  if(is_tdp) {
    // eka
    power.reset();
    // init stats for TDP
    IRF->stats_t.readAc.access  = IRF->l_ip.num_rd_ports;
    IRF->stats_t.writeAc.access = IRF->l_ip.num_wr_ports;
    IRF->tdp_stats              = IRF->stats_t;

    FRF->stats_t.readAc.access  = FRF->l_ip.num_rd_ports;
    FRF->stats_t.writeAc.access = FRF->l_ip.num_wr_ports;
    FRF->tdp_stats              = FRF->stats_t;
    if(coredynp.regWindowing) {
      RFWIN->stats_t.readAc.access  = 0.67 * RFWIN->l_ip.num_rw_ports;
      RFWIN->stats_t.writeAc.access = 0.33 * RFWIN->l_ip.num_rw_ports;
      RFWIN->tdp_stats              = RFWIN->stats_t;
    }
  } else {
    // eka
    rt_power.reset();
    // init stats for Runtime Dynamic (RTP)
    IRF->stats_t.readAc.access  = XML->sys.core[ithCore].int_regfile_reads; // TODO: no diff on archi and phy
    IRF->stats_t.writeAc.access = XML->sys.core[ithCore].int_regfile_writes;
    IRF->rtp_stats              = IRF->stats_t;

    FRF->stats_t.readAc.access  = XML->sys.core[ithCore].float_regfile_reads;
    FRF->stats_t.writeAc.access = XML->sys.core[ithCore].float_regfile_writes;
    FRF->rtp_stats              = FRF->stats_t;
    if(coredynp.regWindowing) {
      RFWIN->stats_t.readAc.access  = XML->sys.core[ithCore].function_calls;
      RFWIN->stats_t.writeAc.access = XML->sys.core[ithCore].function_calls;
      RFWIN->rtp_stats              = RFWIN->stats_t;

      IRF->stats_t.readAc.access  = XML->sys.core[ithCore].int_regfile_reads + XML->sys.core[ithCore].function_calls * 16;
      IRF->stats_t.writeAc.access = XML->sys.core[ithCore].int_regfile_writes + XML->sys.core[ithCore].function_calls * 16;
      IRF->rtp_stats              = IRF->stats_t;

      FRF->stats_t.readAc.access = XML->sys.core[ithCore].float_regfile_reads + XML->sys.core[ithCore].function_calls * 16;
      ;
      FRF->stats_t.writeAc.access = XML->sys.core[ithCore].float_regfile_writes + XML->sys.core[ithCore].function_calls * 16;
      ;
      FRF->rtp_stats = FRF->stats_t;
    }
  }
  IRF->power_t.reset();
  FRF->power_t.reset();
  // eka, may need to check the first call
  // IRF->power.reset();
  // FRF->power.reset();
  IRF->power_t.readOp.dynamic += (IRF->stats_t.readAc.access * IRF->local_result.power.readOp.dynamic +
                                  IRF->stats_t.writeAc.access * IRF->local_result.power.writeOp.dynamic);
  FRF->power_t.readOp.dynamic += (FRF->stats_t.readAc.access * FRF->local_result.power.readOp.dynamic +
                                  FRF->stats_t.writeAc.access * FRF->local_result.power.writeOp.dynamic);
  if(coredynp.regWindowing) {
    RFWIN->power_t.reset();
    RFWIN->power_t.readOp.dynamic += (RFWIN->stats_t.readAc.access * RFWIN->local_result.power.readOp.dynamic +
                                      RFWIN->stats_t.writeAc.access * RFWIN->local_result.power.writeOp.dynamic);
  }

  if(is_tdp) {
    IRF->power = IRF->power_t + IRF->local_result.power * pppm_lkg;
    FRF->power = FRF->power_t + FRF->local_result.power * pppm_lkg;
    power      = power + (IRF->power_t + FRF->power_t);
    if(coredynp.regWindowing) {
      RFWIN->power = RFWIN->power_t + RFWIN->local_result.power * pppm_lkg;
      power        = power + RFWIN->power;
    }
  } else {
    IRF->rt_power = IRF->power_t + IRF->local_result.power * pppm_lkg;
    FRF->rt_power = FRF->power_t + FRF->local_result.power * pppm_lkg;
    rt_power      = rt_power + (IRF->power_t + FRF->power_t);
    if(coredynp.regWindowing) {
      RFWIN->rt_power = RFWIN->power_t + RFWIN->local_result.power * pppm_lkg;
      rt_power        = rt_power + RFWIN->rt_power;
    }
  }
}

void RegFU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  if(is_tdp) {
    cout << indent_str << "Integer RF:" << endl;
    cout << indent_str_next << "Area = " << IRF->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << IRF->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << IRF->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << IRF->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << IRF->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Floating Point RF:" << endl;
    cout << indent_str_next << "Area = " << FRF->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << FRF->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << FRF->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << FRF->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << FRF->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(coredynp.regWindowing) {
      cout << indent_str << "Register Windows:" << endl;
      cout << indent_str_next << "Area = " << RFWIN->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << RFWIN->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << RFWIN->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << RFWIN->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << RFWIN->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
    }
  } else {
    cout << indent_str_next << "Integer RF    TDP Dynamic = " << IRF->rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Integer RF    Subthreshold Leakage = " << IRF->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Integer RF    Gate Leakage = " << IRF->rt_power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Floating Point RF   TDP Dynamic = " << FRF->rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Floating Point RF   Subthreshold Leakage = " << FRF->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Floating Point RF   Gate Leakage = " << FRF->rt_power.readOp.gate_leakage << " W" << endl;
    if(coredynp.regWindowing) {
      cout << indent_str_next << "Register Windows   TDP Dynamic = " << RFWIN->rt_power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Register Windows   Subthreshold Leakage = " << RFWIN->rt_power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Register Windows   Gate Leakage = " << RFWIN->rt_power.readOp.gate_leakage << " W" << endl;
    }
  }
}

void EXECU::computeEnergy(bool is_tdp) {
  double pppm_t[4] = {1, 1, 1, 1};

  rfu->computeEnergy(is_tdp);

  scheu->computeEnergy(is_tdp);

  if(is_tdp) {
    // eka
    power.reset();
    bypass.power.reset();
    set_pppm(pppm_t, 2, 2, 2, 2);
    bypass.power = bypass.power + intTagBypass->power * pppm_t + int_bypass->power * pppm_t;
    set_pppm(pppm_t, 3, 3, 3, 3);
    bypass.power = bypass.power + fp_bypass->power * pppm_t + fpTagBypass->power * pppm_t;
    power        = power + rfu->power + fp_u->power + exeu->power + bypass.power + scheu->power;
  } else {
    // eka
    rt_power.reset();
    bypass.rt_power.reset();
    set_pppm(pppm_t, XML->sys.core[ithCore].bypassbus_access, 2, 2, 2);
    bypass.rt_power = bypass.rt_power + intTagBypass->power * pppm_t;
    bypass.rt_power = bypass.rt_power + int_bypass->power * pppm_t;
    bypass.rt_power = bypass.rt_power + fp_bypass->power * pppm_t;
    bypass.rt_power = bypass.rt_power + fpTagBypass->power * pppm_t;
    exeu->rt_power.readOp.dynamic =
        exeu->power.readOp.dynamic * XML->sys.core[ithCore].int_instructions / XML->sys.core[0].ALU_per_core;
    exeu->rt_power.readOp.leakage = exeu->local_result.power.readOp.leakage;
    fp_u->rt_power.readOp.dynamic =
        fp_u->power.readOp.dynamic * XML->sys.core[ithCore].fp_instructions / XML->sys.core[0].FPU_per_core;
    fp_u->rt_power.readOp.leakage = fp_u->local_result.power.readOp.leakage;
    rt_power                      = rt_power + rfu->rt_power + fp_u->rt_power + exeu->rt_power + bypass.rt_power + scheu->rt_power;
  }
}

void EXECU::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');

  //	cout << indent_str_next << "Results Broadcast Bus Area = " << bypass->area.get_area() *1e-6 << " mm^2" << endl;
  if(is_tdp) {
    cout << indent_str << "Register Files:" << endl;
    cout << indent_str_next << "Area = " << rfu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << rfu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << rfu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << rfu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << rfu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 3) {
      rfu->displayEnergy(indent + 4, is_tdp);
    }
    cout << indent_str << "Instruction Scheduler:" << endl;
    cout << indent_str_next << "Area = " << scheu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << scheu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << scheu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << scheu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << scheu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 3) {
      scheu->displayEnergy(indent + 4, is_tdp);
    }
    cout << indent_str << "Results Broadcast Bus:" << endl;
    cout << indent_str_next << "TDP Dynamic = " << bypass.power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << bypass.power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << bypass.power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << bypass.rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
  } else {
    cout << indent_str_next << "Register Files    TDP Dynamic = " << rfu->rt_power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Register Files    Subthreshold Leakage = " << rfu->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Register Files    Gate Leakage = " << rfu->rt_power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Instruction Sheduler   TDP Dynamic = " << scheu->rt_power.readOp.dynamic * clockRate << " W"
         << endl;
    cout << indent_str_next << "Instruction Sheduler   Subthreshold Leakage = " << scheu->rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Instruction Sheduler   Gate Leakage = " << scheu->rt_power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Results Broadcast Bus   TDP Dynamic = " << bypass.rt_power.readOp.dynamic * clockRate << " W"
         << endl;
    cout << indent_str_next << "Results Broadcast Bus   Subthreshold Leakage = " << bypass.rt_power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Results Broadcast Bus   Gate Leakage = " << bypass.rt_power.readOp.gate_leakage << " W" << endl;
  }
}

void Core::computeEnergy(bool is_tdp) {
  // power_point_product_masks
  double pppm_t[4] = {1, 1, 1, 1};
  executionTime    = XML->sys.executionTime;

  if(is_tdp) {
    // eka
    power.reset();
    ifu->computeEnergy(is_tdp);
    lsu->computeEnergy(is_tdp);
    mmu->computeEnergy(is_tdp);
    exu->computeEnergy(is_tdp);
    // pipeline
    if(coredynp.core_ty == OOO) {
      rnu->computeEnergy();
      power = power + rnu->power;
      set_pppm(pppm_t, 1 / 5.0, 1 / 5.0, 1 / 5.0, 1 / 5.0);
    } else {
      set_pppm(pppm_t, 1 / 4.0, 1 / 4.0, 1 / 4.0, 1 / 4.0);
    }

    if(ifu->exist) {
      ifu->power = ifu->power + corepipe->power * pppm_t;
      power      = power + ifu->power;
    }
    if(lsu->exist) {
      lsu->power = lsu->power + corepipe->power * pppm_t;
      power      = power + lsu->power;
    }
    if(exu->exist) {
      exu->power = exu->power + corepipe->power * pppm_t;
      power      = power + exu->power;
    }
    if(mmu->exist) {
      mmu->power = mmu->power + corepipe->power * pppm_t;
      power      = power + mmu->power;
    }

    // power    = exu->power +lsu->power + ifu->power + mmu->power + rnu->rt_power;// + clockNetwork.total_power.readOp.dynamic;// +
    // corepipe.power.readOp.dynamic; + branchPredictor.maxDynamicPower
  } else {
    // eka
    rt_power.reset();
    ifu->computeEnergy(is_tdp);
    lsu->computeEnergy(is_tdp);
    mmu->computeEnergy(is_tdp);
    exu->computeEnergy(is_tdp);
    // pipeline
    if(coredynp.core_ty == OOO) {
      rnu->computeEnergy(is_tdp);
      set_pppm(pppm_t, 1 / 5.0, 1 / 5.0, 1 / 5.0, 1 / 5.0);
      rnu->rt_power = rnu->rt_power + corepipe->power * pppm_t;
    } else {
      set_pppm(pppm_t, 1 / 4.0, 1 / 4.0, 1 / 4.0, 1 / 4.0);
    }

    if(ifu->exist) {
      ifu->rt_power = ifu->rt_power + corepipe->power * pppm_t;
      rt_power      = rt_power + ifu->rt_power;
    }
    if(lsu->exist) {
      lsu->rt_power = lsu->rt_power + corepipe->power * pppm_t;
      rt_power      = rt_power + lsu->rt_power;
    }
    if(exu->exist) {
      exu->rt_power = exu->rt_power + corepipe->power * pppm_t;
      rt_power      = rt_power + exu->rt_power;
    }
    if(mmu->exist) {
      mmu->rt_power = mmu->rt_power + corepipe->power * pppm_t;
      rt_power      = rt_power + mmu->rt_power;
    }

    // rt_power     = rt_power + ifu->rt_power + lsu->rt_power + mmu->rt_power + exu->rt_power + rnu->rt_power;
  }
}

void Core::displayEnergy(uint32_t indent, int plevel, bool is_tdp) {
  string indent_str(indent, ' ');
  string indent_str_next(indent + 2, ' ');
  if(is_tdp) {
    cout << "Core:" << endl;
    cout << indent_str << "Area = " << area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str << "TDP Dynamic = " << power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str << "Subthreshold Leakage = " << power.readOp.leakage << " W" << endl;
    cout << indent_str << "Gate Leakage = " << power.readOp.gate_leakage << " W" << endl;
    cout << indent_str << "Runtime Dynamic = " << rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    cout << indent_str << "Instruction Fetch Unit:" << endl;
    cout << indent_str_next << "Area = " << ifu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << ifu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << ifu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << ifu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << ifu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      ifu->displayEnergy(indent + 4, plevel, is_tdp);
    }
    if(coredynp.core_ty == OOO) {
      cout << indent_str << "Renaming Unit:" << endl;
      cout << indent_str_next << "Area = " << rnu->area.get_area() * 1e-6 << " mm^2" << endl;
      cout << indent_str_next << "TDP Dynamic = " << rnu->power.readOp.dynamic * clockRate << " W" << endl;
      cout << indent_str_next << "Subthreshold Leakage = " << rnu->power.readOp.leakage << " W" << endl;
      cout << indent_str_next << "Gate Leakage = " << rnu->power.readOp.gate_leakage << " W" << endl;
      cout << indent_str_next << "Runtime Dynamic = " << rnu->rt_power.readOp.dynamic / executionTime << " W" << endl;
      cout << endl;
      if(plevel > 2) {
        rnu->displayEnergy(indent + 4, plevel, is_tdp);
      }
    }
    cout << indent_str << "Load Store Unit:" << endl;
    cout << indent_str_next << "Area = " << lsu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << lsu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << lsu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << lsu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << lsu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      lsu->displayEnergy(indent + 4, plevel, is_tdp);
    }
    cout << indent_str << "Memory Management Unit:" << endl;
    cout << indent_str_next << "Area = " << mmu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << mmu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << mmu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << mmu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << mmu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      mmu->displayEnergy(indent + 4, plevel, is_tdp);
    }
    cout << indent_str << "Execution Unit:" << endl;
    cout << indent_str_next << "Area = " << exu->area.get_area() * 1e-6 << " mm^2" << endl;
    cout << indent_str_next << "TDP Dynamic = " << exu->power.readOp.dynamic * clockRate << " W" << endl;
    cout << indent_str_next << "Subthreshold Leakage = " << exu->power.readOp.leakage << " W" << endl;
    cout << indent_str_next << "Gate Leakage = " << exu->power.readOp.gate_leakage << " W" << endl;
    cout << indent_str_next << "Runtime Dynamic = " << exu->rt_power.readOp.dynamic / executionTime << " W" << endl;
    cout << endl;
    if(plevel > 2) {
      exu->displayEnergy(indent + 4, plevel, is_tdp);
    }

  } else {
  }
}
InstFetchU ::~InstFetchU() {

  if(IB) {
    delete IB;
    IB = 0;
  }
  if(coredynp.predictionW > 0) {
    if(BTB) {
      delete BTB;
      BTB = 0;
    }
    if(BPT) {
      delete BPT;
      BPT = 0;
    }
  }
}

BranchPredictor ::~BranchPredictor() {

  if(globalBPT) {
    delete globalBPT;
    globalBPT = 0;
  }
  if(L1_localBPT) {
    delete L1_localBPT;
    L1_localBPT = 0;
  }
  if(L2_localBPT) {
    delete L2_localBPT;
    L2_localBPT = 0;
  }
  if(chooser) {
    delete chooser;
    chooser = 0;
  }
  if(RAS) {
    delete RAS;
    RAS = 0;
  }
}

RENAMINGU ::~RENAMINGU() {

  if(iFRAT) {
    delete iFRAT;
    iFRAT = 0;
  }
  if(fFRAT) {
    delete fFRAT;
    fFRAT = 0;
  }
  if(iRRAT) {
    delete iRRAT;
    iRRAT = 0;
  }
  if(iFRAT) {
    delete iFRAT;
    iFRAT = 0;
  }
  if(ifreeL) {
    delete ifreeL;
    ifreeL = 0;
  }
  if(ffreeL) {
    delete ffreeL;
    ffreeL = 0;
  }
  if(idcl) {
    delete idcl;
    idcl = 0;
  }
  if(fdcl) {
    delete fdcl;
    fdcl = 0;
  }
}

LoadStoreU ::~LoadStoreU() {

  if(LSQ) {
    delete LSQ;
    LSQ = 0;
  }
}

MemManU ::~MemManU() {

  if(itlb) {
    delete itlb;
    itlb = 0;
  }
  if(dtlb) {
    delete dtlb;
    dtlb = 0;
  }
}

RegFU ::~RegFU() {

  if(IRF) {
    delete IRF;
    IRF = 0;
  }
  if(FRF) {
    delete FRF;
    FRF = 0;
  }
  if(RFWIN) {
    delete RFWIN;
    RFWIN = 0;
  }
}

SchedulerU ::~SchedulerU() {

  if(int_inst_window) {
    delete int_inst_window;
    int_inst_window = 0;
  }
  if(fp_inst_window) {
    delete int_inst_window;
    int_inst_window = 0;
  }
  if(ROB) {
    delete ROB;
    ROB = 0;
  }
  if(instruction_selection) {
    delete instruction_selection;
    instruction_selection = 0;
  }
}

EXECU ::~EXECU() {

  if(int_bypass) {
    delete int_bypass;
    int_bypass = 0;
  }
  if(intTagBypass) {
    delete intTagBypass;
    intTagBypass = 0;
  }
  if(fp_bypass) {
    delete fp_bypass;
    fp_bypass = 0;
  }
  if(fpTagBypass) {
    delete fpTagBypass;
    fpTagBypass = 0;
  }
  if(fp_u) {
    delete fp_u;
    fp_u = 0;
  }
  if(exeu) {
    delete exeu;
    exeu = 0;
  }
  if(rfu) {
    delete rfu;
    rfu = 0;
  }
  if(scheu) {
    delete scheu;
    scheu = 0;
  }
}

Core ::~Core() {

  if(ifu) {
    delete ifu;
    ifu = 0;
  }
  if(lsu) {
    delete lsu;
    lsu = 0;
  }
  if(rnu) {
    delete rnu;
    rnu = 0;
  }
  if(mmu) {
    delete mmu;
    mmu = 0;
  }
  if(exu) {
    delete exu;
    exu = 0;
  }
  if(corepipe) {
    delete corepipe;
    corepipe = 0;
  }
  if(undiffCore) {
    delete undiffCore;
    undiffCore = 0;
  }
}

void Core::set_core_param() {
  coredynp.core_ty     = (enum Core_type)XML->sys.core[ithCore].machine_type;
  coredynp.rm_ty       = (enum Renaming_type)XML->sys.core[ithCore].rename_scheme;
  coredynp.fetchW      = XML->sys.core[ithCore].fetch_width;
  coredynp.decodeW     = XML->sys.core[ithCore].decode_width;
  coredynp.issueW      = XML->sys.core[ithCore].issue_width;
  coredynp.commitW     = XML->sys.core[ithCore].commit_width;
  coredynp.predictionW = XML->sys.core[ithCore].prediction_width;
  coredynp.fp_issueW   = XML->sys.core[ithCore].fp_issue_width;
  coredynp.fp_decodeW  = XML->sys.core[ithCore].fp_issue_width;

  coredynp.num_hthreads       = XML->sys.core[ithCore].number_hardware_threads;
  coredynp.multithreaded      = coredynp.num_hthreads > 1 ? true : false;
  coredynp.instruction_length = XML->sys.core[ithCore].instruction_length;
  coredynp.pc_width           = XML->sys.virtual_address_width;
  coredynp.opcode_length      = XML->sys.core[ithCore].opcode_width;
  coredynp.pipeline_stages    = XML->sys.core[ithCore].pipeline_depth[0];
  coredynp.int_data_width     = int(ceil(XML->sys.machine_bits / 32.0)) * 32;
  coredynp.fp_data_width      = coredynp.int_data_width;
  coredynp.v_address_width    = XML->sys.virtual_address_width;
  coredynp.p_address_width    = XML->sys.physical_address_width;

  coredynp.scheu_ty        = (enum Scheduler_type)XML->sys.core[ithCore].instruction_window_scheme;
  coredynp.arch_ireg_width = int(ceil(log2(XML->sys.core[ithCore].archi_Regs_IRF_size)));
  coredynp.arch_freg_width = int(ceil(log2(XML->sys.core[ithCore].archi_Regs_FRF_size)));
  coredynp.num_IRF_entry   = XML->sys.core[ithCore].archi_Regs_IRF_size;
  coredynp.num_FRF_entry   = XML->sys.core[ithCore].archi_Regs_FRF_size;

  if(!((coredynp.core_ty == OOO) || (coredynp.core_ty == Inorder))) {
    cout << "Invalid Core Type" << endl;
    exit(0);
  }
  if(!((coredynp.scheu_ty == PhysicalRegFile) || (coredynp.scheu_ty == ReservationStation))) {
    cout << "Invalid OOO Scheduler Type" << endl;
    exit(0);
  }

  if(!((coredynp.rm_ty == RAMbased) || (coredynp.rm_ty == CAMbased))) {
    cout << "Invalid OOO Renaming Type" << endl;
    exit(0);
  }

  if(coredynp.core_ty == OOO) {
    if(coredynp.scheu_ty == PhysicalRegFile) {
      coredynp.phy_ireg_width        = int(ceil(log2(XML->sys.core[ithCore].phy_Regs_IRF_size)));
      coredynp.phy_freg_width        = int(ceil(log2(XML->sys.core[ithCore].phy_Regs_FRF_size)));
      coredynp.num_ifreelist_entries = coredynp.num_IRF_entry = XML->sys.core[ithCore].phy_Regs_IRF_size;
      coredynp.num_ffreelist_entries = coredynp.num_FRF_entry = XML->sys.core[ithCore].phy_Regs_FRF_size;
    } else if(coredynp.scheu_ty == ReservationStation) { // ROB serves as Phy RF in RS based OOO
      coredynp.phy_ireg_width        = int(ceil(log2(XML->sys.core[ithCore].ROB_size)));
      coredynp.phy_freg_width        = int(ceil(log2(XML->sys.core[ithCore].ROB_size)));
      coredynp.num_ifreelist_entries = XML->sys.core[ithCore].ROB_size;
      coredynp.num_ffreelist_entries = XML->sys.core[ithCore].ROB_size;
    }
  }
  coredynp.globalCheckpoint   = 32; // best check pointing entries for a 4~8 issue OOO should be 16~48;See TR for reference.
  coredynp.perThreadState     = 8;
  coredynp.instruction_length = 32;
  coredynp.clockRate          = XML->sys.core[ithCore].clock_rate;
  //  coredynp.clockRate          *= 1e6;
  coredynp.regWindowing  = (XML->sys.core[ithCore].register_windows_size > 0 && coredynp.core_ty == Inorder) ? true : false;
  coredynp.executionTime = XML->sys.executionTime;
}

// eka, to update runtime parameters
void Core::update_rtparam(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, Core *core) {
  ithCore            = ithCore_;
  core->ifu->ithCore = ithCore_;
  if(core->coredynp.predictionW > 0) {
    core->ifu->BPT->ithCore = ithCore_;
  }
  core->lsu->ithCore        = ithCore_;
  core->mmu->ithCore        = ithCore_;
  core->exu->ithCore        = ithCore_;
  core->exu->rfu->ithCore   = ithCore_;
  core->exu->scheu->ithCore = ithCore_;
  if(core->coredynp.core_ty == OOO) {
    core->rnu->ithCore = ithCore_;
  }

  XML = XML_interface;

  // ithCore     = ithCore_;
  // interface_ip = interface_ip_

  core->ifu->XML        = XML;
  core->ifu->BPT->XML   = XML;
  core->lsu->XML        = XML;
  core->mmu->XML        = XML;
  core->exu->XML        = XML;
  core->exu->rfu->XML   = XML;
  core->exu->scheu->XML = XML;
  if(core->coredynp.core_ty == OOO) {
    core->rnu->XML = XML;
  }

  XML->sys.core[ithCore].scoore                     = XML->sys.core[0].scoore;
  XML->sys.core[ithCore].VPCfilter.dcache_config[0] = XML->sys.core[0].VPCfilter.dcache_config[0];
}

void InstFetchU::buildInstCache() {
  // Assuming all L1 caches are virtually indexed physically tagged.
  // cache

  int  idx, tag, data, size, line, assoc, banks;
  bool debug                         = false;
  size                               = XML->sys.core[ithCore].icache.icache_config[0];
  line                               = XML->sys.core[ithCore].icache.icache_config[1];
  assoc                              = XML->sys.core[ithCore].icache.icache_config[2];
  banks                              = XML->sys.core[ithCore].icache.icache_config[3];
  idx                                = debug ? 9 : int(ceil(log2(size / line / assoc)));
  tag                                = debug ? 51 : XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag          = 1;
  interface_ip.tag_w                 = tag;
  interface_ip.cache_sz              = debug ? 32768 : size;
  interface_ip.line_sz               = debug ? 64 : line;
  interface_ip.assoc                 = debug ? 8 : assoc;
  interface_ip.nbanks                = debug ? 1 : banks;
  interface_ip.out_w                 = interface_ip.line_sz * 8;
  interface_ip.access_mode           = 0; // debug?0:XML->sys.core[ithCore].icache.icache_config[5];
  interface_ip.throughput            = debug ? 1.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[4] / clockRate;
  interface_ip.latency               = debug ? 3.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[5] / clockRate;
  interface_ip.is_cache              = true;
  interface_ip.pure_cam              = false;
  interface_ip.pure_ram              = false;
  interface_ip.ram_cell_tech_type    = 1;
  interface_ip.peri_global_tech_type = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  //  interface_ip.obj_func_dyn_energy = 0;
  //  interface_ip.obj_func_dyn_power  = 0;
  //  interface_ip.obj_func_leak_power = 0;
  //  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports;
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  icache.caches                = new ArrayST(&interface_ip, "icache");

  scktRatio         = g_tp.sckt_co_eff;
  chip_PR_overhead  = g_tp.chip_layout_overhead;
  macro_PR_overhead = g_tp.macro_layout_overhead;
  icache.area.set_area(icache.area.get_area() + icache.caches->local_result.area);
  area.set_area(area.get_area() + icache.caches->local_result.area);
  // output_data_csv(icache.caches.local_result);

  /*
   *iCache controllers
   *miss buffer Each MSHR contains enough state
   *to handle one or more accesses of any type to a single memory line.
   *Due to the generality of the MSHR mechanism,
   *the amount of state involved is non-trivial:
   *including the address, pointers to the cache entry and destination register,
   *written data, and various other pieces of state.
   */
  interface_ip.num_search_ports = debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports;
  tag                           = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  data                          = (XML->sys.physical_address_width) + int(ceil(log2(size / line))) + icache.caches->l_ip.line_sz;
  interface_ip.specific_tag     = 1;
  interface_ip.tag_w            = tag;
  interface_ip.line_sz          = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz         = (XML->sys.core[ithCore].icache.buffer_sizes[0] * interface_ip.line_sz < 64
                               ? 64
                               : XML->sys.core[ithCore].icache.buffer_sizes[0] * interface_ip.line_sz);
  interface_ip.assoc            = 0;
  interface_ip.nbanks           = 1;
  interface_ip.out_w            = interface_ip.line_sz * 8;
  interface_ip.access_mode      = 2;
  interface_ip.throughput = debug ? 1.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[4] / clockRate; // means cycle
                                                                                                                  // time
  interface_ip.latency = debug ? 1.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[5] / clockRate; // means access time
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.num_rw_ports                   = debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports;
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_wr_ports                   = 0;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].number_instruction_fetch_ports;
  icache.missb                                = new ArrayST(&interface_ip, "icacheMissBuffer");
  icachecc.area.set_area(icachecc.area.get_area() + icache.missb->local_result.area);
  area.set_area(area.get_area() + icache.missb->local_result.area);
  // output_data_csv(icache.missb.local_result);

#if 0 // no mshr for icache

  //fill buffer
  tag							   = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  data							   = icache.caches->l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = data;//int(pow(2.0,ceil(log2(data))));
  interface_ip.cache_sz            = data*XML->sys.core[ithCore].icache.buffer_sizes[1];
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug?1.0/clockRate:XML->sys.core[ithCore].icache.icache_config[4]/clockRate;
  interface_ip.latency             = debug?1.0/clockRate:XML->sys.core[ithCore].icache.icache_config[5]/clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = debug?1:XML->sys.core[ithCore].number_instruction_fetch_ports;
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  interface_ip.num_search_ports = XML->sys.core[ithCore].number_instruction_fetch_ports;
  icache.ifb = new ArrayST(&interface_ip, "icacheFillBuffer");
  icache.area.set_area(icache.area.get_area()+ icache.ifb->local_result.area);
  area.set_area(area.get_area()+ icache.ifb->local_result.area);
  //output_data_csv(icache.ifb.local_result);

  //prefetch buffer
  tag							   = XML->sys.physical_address_width + EXTRA_TAG_BITS;//check with previous entries to decide wthether to merge.
  data							   = icache.caches->l_ip.line_sz;//separate queue to prevent from cache polution.
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = data;//int(pow(2.0,ceil(log2(data))));
  interface_ip.cache_sz            = XML->sys.core[ithCore].icache.buffer_sizes[2]*interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug?1.0/clockRate:XML->sys.core[ithCore].icache.icache_config[4]/clockRate;
  interface_ip.latency             = debug?1.0/clockRate:XML->sys.core[ithCore].icache.icache_config[5]/clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = debug?1:XML->sys.core[ithCore].number_instruction_fetch_ports;
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  interface_ip.num_search_ports = XML->sys.core[ithCore].number_instruction_fetch_ports;
  icache.prefetchb = new ArrayST(&interface_ip, "icacheprefetchBuffer");
  icache.area.set_area(icache.area.get_area()+ icache.prefetchb->local_result.area);
  area.set_area(area.get_area()+ icache.prefetchb->local_result.area);
  //output_data_csv(icache.prefetchb.local_result);

#endif
}

void InstFetchU::buildInstBuffer() {
  // Instruction buffer
  bool debug = false;
  int  data =
      XML->sys.core[ithCore].instruction_length *
      XML->sys.core[ithCore].issue_width; // icache.caches.l_ip.line_sz; //multiple threads timing sharing the instruction buffer.
  interface_ip.is_cache = false;
  interface_ip.pure_ram = true;
  interface_ip.pure_cam = false;
  interface_ip.line_sz  = int(ceil(data / 8.0));
  interface_ip.cache_sz = XML->sys.core[ithCore].instruction_buffer_size * interface_ip.line_sz > 64
                              ? XML->sys.core[ithCore].instruction_buffer_size * interface_ip.line_sz
                              : 64;
  interface_ip.assoc       = 1;
  interface_ip.nbanks      = 1;
  interface_ip.out_w       = interface_ip.line_sz * 8;
  interface_ip.access_mode = 0;
  // interface_ip.throughput          = 1.0/clockRate;
  // interface_ip.latency             = 1.0/clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  // NOTE: Assuming IB is time slice shared among threads, every fetch op will at least fetch "fetch width" instructions.
  interface_ip.num_rw_ports =
      debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports; // XML->sys.core[ithCore].fetch_width;
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;
  IB                           = new ArrayST(&interface_ip, "InstBuffer");
  IB->area.set_area(IB->area.get_area() + IB->local_result.area);
  area.set_area(area.get_area() + IB->local_result.area);
  // output_data_csv(IB.IB.local_result);

  //	  inst_decoder.opcode_length = XML->sys.core[ithCore].opcode_width;
  //	  inst_decoder.init_decoder(is_default, &interface_ip);
  //	  inst_decoder.full_decoder_power();
}

void InstFetchU::buildBranchPredictor() {

  int  idx, tag, size, line, assoc, banks;
  bool debug = false;
  if(coredynp.predictionW > 0) {
    /*
     * BTB branch target buffer, accessed during IF stage. Virtually indexed and virtually tagged
     * It is only a cache without all the buffers in the cache controller since it is more like a
     * look up table than a cache with cache controller. When access miss, no load from other places
     * such as main memory, it is passively updated when BPT/EXEU find out wrong predictions
     * not actively fill the misses.
     */
    size        = XML->sys.core[ithCore].BTB.BTB_config[0];
    line        = XML->sys.core[ithCore].BTB.BTB_config[1];
    assoc       = XML->sys.core[ithCore].BTB.BTB_config[2];
    banks       = XML->sys.core[ithCore].BTB.BTB_config[3];
    int   csize = size / line / assoc;
    float index = log2(csize);
    idx         = int(ceil(index));
    //      idx                                 = debug?9:int(ceil(log2(size/line/assoc)));
    tag                                = debug ? 51 : XML->sys.virtual_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
    interface_ip.is_cache              = true;
    interface_ip.pure_ram              = false;
    interface_ip.pure_cam              = false;
    interface_ip.specific_tag          = 1;
    interface_ip.tag_w                 = tag;
    interface_ip.cache_sz              = debug ? 32768 : size;
    interface_ip.line_sz               = debug ? 64 : ceil(line / 8);
    interface_ip.assoc                 = debug ? 8 : assoc;
    interface_ip.nbanks                = debug ? 1 : banks;
    interface_ip.out_w                 = interface_ip.line_sz * 8;
    interface_ip.access_mode           = 0; // debug?0:XML->sys.core[ithCore].dcache.dcache_config[5];
    interface_ip.throughput            = debug ? 1.0 / clockRate : XML->sys.core[ithCore].BTB.BTB_config[4] / clockRate;
    interface_ip.latency               = debug ? 3.0 / clockRate : XML->sys.core[ithCore].BTB.BTB_config[5] / clockRate;
    interface_ip.obj_func_dyn_energy   = 0;
    interface_ip.obj_func_dyn_power    = 0;
    interface_ip.obj_func_leak_power   = 0;
    interface_ip.obj_func_cycle_t      = 1;
    interface_ip.ram_cell_tech_type    = 1;
    interface_ip.peri_global_tech_type = 1;
    interface_ip.data_arr_ram_cell_tech_type    = 1;
    interface_ip.data_arr_peri_global_tech_type = 1;
    interface_ip.tag_arr_ram_cell_tech_type     = 1;
    interface_ip.tag_arr_peri_global_tech_type  = 1;
    interface_ip.num_rw_ports                   = 0;
    interface_ip.num_rd_ports                   = coredynp.predictionW;
    interface_ip.num_wr_ports                   = coredynp.predictionW;
    interface_ip.num_se_rd_ports                = 0;
    BTB                                         = new ArrayST(&interface_ip, "BT_Buffer");
    BTB->area.set_area(BTB->area.get_area() + BTB->local_result.area);
    area.set_area(area.get_area() + BTB->local_result.area);
    /// cout<<"area                              = "<<area<<endl;

    BPT = new BranchPredictor(XML, ithCore, &interface_ip, coredynp);
    area.set_area(area.get_area() + BPT->area.get_area());
  }
}

void SchedulerU::buildInorderScheduler() {
  int  tag, data;
  bool is_default = true;
  // Instruction issue queue, in-order multi-issue or multithreaded processor also has this structure. Unified window for Inorder
  // processors
  tag                       = int(log2(XML->sys.core[ithCore].number_hardware_threads) *
            coredynp.perThreadState); // This is the normal thread state bits based on Niagara Design
  data                      = XML->sys.core[ithCore].instruction_length; // NOTE: x86 inst can be very lengthy, up to 15B
  interface_ip.is_cache     = true;
  interface_ip.pure_cam     = false;
  interface_ip.pure_ram     = false;
  interface_ip.line_sz      = int(ceil(data / 8.0));
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = tag;
  interface_ip.cache_sz     = XML->sys.core[ithCore].instruction_window_size * interface_ip.line_sz > 64
                              ? XML->sys.core[ithCore].instruction_window_size * interface_ip.line_sz
                              : 64;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 1;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].issue_width;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].issue_width;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].issue_width;
  int_inst_window                             = new ArrayST(&interface_ip, "InstFetchQueue");
  int_inst_window->area.set_area(int_inst_window->area.get_area() + int_inst_window->local_result.area);
  area.set_area(area.get_area() + int_inst_window->local_result.area);
  // output_data_csv(iRS.RS.local_result);
  Iw_height = int_inst_window->local_result.cache_ht;

  /*
   * selection logic
   * In a single-issue Inorder multithreaded processor like Niagara, issue width=1*number_of_threads since the processor does need
   * to pick up instructions from multiple ready ones(although these ready ones are from different threads).While SMT processors do
   * not distinguish which thread belongs to who at the issue stage.
   */

  instruction_selection =
      new selection_logic(is_default, XML->sys.core[ithCore].instruction_window_size,
                          XML->sys.core[ithCore].issue_width * XML->sys.core[ithCore].number_hardware_threads, &interface_ip);
}

void SchedulerU::buildIntInstWindow() {
  int tag, data;
  /*
   * CAM based instruction window
   * For physicalRegFilebased OOO it is the instruction issue queue, where only tags of phy regs are stored
   * For RS based OOO it is the Reservation station, where both tags and values of phy regs are stored
   * It is written once and read twice(two operands) before an instruction can be issued.
   * X86 instruction can be very long up to 15B. add instruction length in XML
   */
  if(coredynp.scheu_ty == PhysicalRegFile) {
    tag  = 2 * coredynp.phy_ireg_width; // TODO: each time only half of the tag is compared
    data = int(ceil((coredynp.instruction_length + 2 * (coredynp.phy_ireg_width - coredynp.arch_ireg_width)) / 8.0));
    // tmp_name = "InstIssueQueue";
  } else {
    tag  = 2 * coredynp.phy_ireg_width;
    data = int(ceil(
        (coredynp.instruction_length + 2 * (coredynp.phy_ireg_width - coredynp.arch_ireg_width) + 2 * coredynp.int_data_width) /
        8.0)); // TODO: need divided by 2? means only after both operands avaible, the whole data will be read out.
    // tmp_name = "IntReservationStation";
  }
  interface_ip.is_cache                       = true;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = false;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * XML->sys.core[ithCore].instruction_window_size;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.specific_tag                   = 1;
  interface_ip.tag_w                          = tag;
  interface_ip.access_mode                    = 0;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 1; // XML->sys.core[ithCore].issue_width;
  interface_ip.num_wr_ports                   = 2; // XML->sys.core[ithCore].issue_width;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].issue_width;
  int_inst_window                             = new ArrayST(&interface_ip, "iIW");
  int_inst_window->area.set_area(int_inst_window->area.get_area() + int_inst_window->local_result.area);
  area.set_area(area.get_area() + int_inst_window->local_result.area);
  Iw_height = int_inst_window->local_result.cache_ht;
}

void SchedulerU::buildFPInstWindow() {
  int tag, data;
  // FU inst window
  if(coredynp.scheu_ty == PhysicalRegFile) {
    tag  = 2 * coredynp.phy_freg_width; // TODO: each time only half of the tag is compared
    data = int(ceil((coredynp.instruction_length + 2 * (coredynp.phy_freg_width - coredynp.arch_freg_width)) / 8.0));
    // tmp_name = "FPIssueQueue";
  } else {
    tag  = 2 * coredynp.phy_ireg_width;
    data = int(
        ceil((coredynp.instruction_length + 2 * (coredynp.phy_freg_width - coredynp.arch_freg_width) + 2 * coredynp.fp_data_width) /
             8.0));
    // tmp_name = "FPReservationStation";
  }
  interface_ip.is_cache                       = true;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = false;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * XML->sys.core[ithCore].fp_instruction_window_size;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.specific_tag                   = 1;
  interface_ip.tag_w                          = tag;
  interface_ip.access_mode                    = 0;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 1; // coredynp.fp_issueW;
  interface_ip.num_wr_ports                   = 2; // coredynp.fp_issueW;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = coredynp.fp_issueW;
  fp_inst_window                              = new ArrayST(&interface_ip, "fIW");
  fp_inst_window->area.set_area(fp_inst_window->area.get_area() + fp_inst_window->local_result.area);
  area.set_area(area.get_area() + fp_inst_window->local_result.area);
  fp_Iw_height = fp_inst_window->local_result.cache_ht;
}

void SchedulerU::buildROB() {
  int data;
  /*
   *  ROB.ROB size = inflight inst. ROB is unified for int and fp inst.
   *  One old approach is to combine the RAT and ROB as a huge CAM structure as in AMD K7.
   *  However, this approach is abandoned due to its high power and poor scalability.
   *	McPAT uses current implementation of ROB as circular buffer.
   *	ROB is written once when instruction is issued and read once when the instruction is committed.         *
   */
  int robExtra = int(ceil(5 + log2(coredynp.num_hthreads)));
  // 5 bits are: busy, Issued, Finished, speculative, valid
  if(coredynp.scheu_ty == PhysicalRegFile) {
    // PC is to id the instruction for recover exception.
    // inst is used to map the renamed dest. registers.so that commit stage can know which reg/RRAT to update
    data = int(ceil((robExtra + coredynp.pc_width +
                     /*coredynp.instruction_length*/ 4 + 1 * coredynp.phy_ireg_width) /
                    8.0));
  } else {
    // in RS based OOO, ROB also contains value of destination reg
    data = int(ceil(
        (robExtra + coredynp.pc_width + coredynp.instruction_length + 2 * coredynp.phy_ireg_width + coredynp.fp_data_width) / 8.0));
  }
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].ROB_size; // The XML ROB size is for all threads
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  // a hack
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].commit_width / 2;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].issue_width / 2;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = 0;

  ROB = new ArrayST(&interface_ip, "ROB");
  ROB->area.set_area(ROB->area.get_area() + ROB->local_result.area);
  area.set_area(area.get_area() + ROB->local_result.area);
  ROB_height = ROB->local_result.cache_ht;
}

void SchedulerU::buildSelectionU() {
  bool is_default       = true;
  instruction_selection = new selection_logic(is_default, XML->sys.core[ithCore].instruction_window_size,
                                              XML->sys.core[ithCore].issue_width, &interface_ip);
}

void LoadStoreU::buildDataCache() {
  int  idx, tag, data, size, line, assoc;
  bool debug = false;

  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;

  interface_ip.num_search_ports = XML->sys.core[ithCore].memory_ports;
  interface_ip.is_cache         = true;
  interface_ip.pure_cam         = false;
  interface_ip.pure_ram         = false;
  // Dcache
  size                             = XML->sys.core[ithCore].dcache.dcache_config[0];
  line                             = XML->sys.core[ithCore].dcache.dcache_config[1];
  assoc                            = XML->sys.core[ithCore].dcache.dcache_config[2];
  idx                              = debug ? 9 : int(ceil(log2(size / line / assoc)));
  tag                              = debug ? 51 : XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.cache_sz            = debug ? 32768 : XML->sys.core[ithCore].dcache.dcache_config[0];
  interface_ip.line_sz             = debug ? 64 : XML->sys.core[ithCore].dcache.dcache_config[1];
  interface_ip.assoc               = debug ? 8 : XML->sys.core[ithCore].dcache.dcache_config[2];
  interface_ip.nbanks              = debug ? 1 : XML->sys.core[ithCore].dcache.dcache_config[3];
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 0; // debug?0:XML->sys.core[ithCore].dcache.dcache_config[5];
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 3.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.is_cache            = true;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports    = debug ? 1 : XML->sys.core[ithCore].memory_ports; // usually In-order has 1 and OOO has 2 at least.
  interface_ip.num_rd_ports    = 0;
  interface_ip.num_wr_ports    = 0;
  interface_ip.num_se_rd_ports = 0;

  dcache.caches = new ArrayST(&interface_ip, "dcache");
  dcache.area.set_area(dcache.area.get_area() + dcache.caches->local_result.area);
  area.set_area(area.get_area() + dcache.caches->local_result.area);
  // output_data_csv(dcache.caches.local_result);

  // dCache controllers
  // miss buffer
  tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  data                             = (XML->sys.physical_address_width) + int(ceil(log2(size / line))) + dcache.caches->l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz            = XML->sys.core[ithCore].dcache.buffer_sizes[0] * interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = debug ? 1 : XML->sys.core[ithCore].memory_ports;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.ram_cell_tech_type  = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  dcache.missb                                = new ArrayST(&interface_ip, "dcacheMissBuffer");
  dcachecc.area.set_area(dcachecc.area.get_area() + dcache.missb->local_result.area);
  area.set_area(area.get_area() + dcache.missb->local_result.area);
  // output_data_csv(dcache.missb.local_result);

  // fill buffer
  tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  data                             = dcache.caches->l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].dcache.buffer_sizes[1];
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = debug ? 1 : XML->sys.core[ithCore].memory_ports;
  ;
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_wr_ports                   = 0;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  dcache.ifb                                  = new ArrayST(&interface_ip, "dcacheFillBuffer");
  dcachecc.area.set_area(dcachecc.area.get_area() + dcache.ifb->local_result.area);
  area.set_area(area.get_area() + dcache.ifb->local_result.area);
  // output_data_csv(dcache.ifb.local_result);

  // prefetch buffer
  tag  = XML->sys.physical_address_width + EXTRA_TAG_BITS; // check with previous entries to decide wthether to merge.
  data = dcache.caches->l_ip.line_sz;                      // separate queue to prevent from cache polution.
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
  interface_ip.cache_sz            = XML->sys.core[ithCore].dcache.buffer_sizes[2] * interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = debug ? 1 : XML->sys.core[ithCore].memory_ports;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  dcache.prefetchb                 = new ArrayST(&interface_ip, "dcacheprefetchBuffer");
  // dcache.area.set_area(dcache.area.get_area()+ dcache.prefetchb->local_result.area);
  // area.set_area(area.get_area()+ dcache.prefetchb->local_result.area);
  // output_data_csv(dcache.prefetchb.local_result);

  // WBB
  tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
  data                             = dcache.caches->l_ip.line_sz;
  interface_ip.specific_tag        = 1;
  interface_ip.tag_w               = tag;
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = XML->sys.core[ithCore].dcache.buffer_sizes[3] * interface_ip.line_sz;
  interface_ip.assoc               = 0;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = XML->sys.core[ithCore].memory_ports;
  interface_ip.num_rd_ports        = 0;
  interface_ip.num_wr_ports        = 0;
  interface_ip.num_se_rd_ports     = 0;
  dcache.wbb                       = new ArrayST(&interface_ip, "dcacheWBB");
  dcachecc.area.set_area(dcachecc.area.get_area() + dcache.wbb->local_result.area);
  area.set_area(area.get_area() + dcache.wbb->local_result.area);
  // output_data_csv(dcache.wbb.local_result);
}

void LoadStoreU::buildVPCBuffer() {
  // VPC_buffer
  interface_ip.is_cache    = false;
  interface_ip.pure_cam    = false;
  interface_ip.pure_ram    = true;
  int data                 = XML->sys.machine_bits;
  interface_ip.line_sz     = ceil(data / 8);
  interface_ip.cache_sz    = XML->sys.core[ithCore].vpc_buffer.dcache_config[0] * interface_ip.line_sz; // FIXME: add to conf file
  interface_ip.assoc       = 1;
  interface_ip.nbanks      = 1;
  interface_ip.out_w       = interface_ip.line_sz * 8;
  interface_ip.access_mode = 1;
  interface_ip.throughput  = 1.0 / clockRate;
  interface_ip.latency     = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 1;
  interface_ip.num_wr_ports        = 1;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = XML->sys.core[ithCore].commit_width;
  vpc_buffer                       = new ArrayST(&interface_ip, "VPC_buffer");
  vpc_buffer->area.set_area(vpc_buffer->area.get_area() + vpc_buffer->local_result.area);
  area.set_area(area.get_area() + vpc_buffer->local_result.area);
}

void LoadStoreU::buildVPCSpecBuffers() {
  // VPC-SpecBW
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  int data                                    = 2 * XML->sys.machine_bits;
  interface_ip.line_sz                        = ceil(data / 8);
  interface_ip.cache_sz                       = 48 * interface_ip.line_sz; // FIXME: add to conf file
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 1;
  interface_ip.num_wr_ports                   = 1;
  interface_ip.num_se_rd_ports                = 0;
  vpc_specbw                                  = new ArrayST(&interface_ip, "VPC_SpecBW");
  dcache.area.set_area(dcache.area.get_area() + vpc_specbw->local_result.area);
  area.set_area(area.get_area() + vpc_specbw->local_result.area);

  // VPC-SpecBR
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  data                                        = 2 * XML->sys.machine_bits;
  interface_ip.line_sz                        = ceil(data / 8);
  interface_ip.cache_sz                       = 32 * interface_ip.line_sz; // FIXME: add to conf file
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 1;
  interface_ip.num_wr_ports                   = 1;
  interface_ip.num_se_rd_ports                = 0;
  vpc_specbr                                  = new ArrayST(&interface_ip, "VPC_SpecBR");
  dcache.area.set_area(dcache.area.get_area() + vpc_specbr->local_result.area);
  area.set_area(area.get_area() + vpc_specbr->local_result.area);
}

void LoadStoreU::buildL2Filter() {
  int  idx, tag, data, size, line, assoc;
  bool debug = false;

  // L2 Filter
  size = XML->sys.core[ithCore].VPCfilter.dcache_config[0];
  if(size > 0) {
    interface_ip.is_cache = true;
    interface_ip.pure_cam = false;
    interface_ip.pure_ram = false;

    line                             = XML->sys.core[ithCore].VPCfilter.dcache_config[1];
    assoc                            = XML->sys.core[ithCore].VPCfilter.dcache_config[2];
    idx                              = debug ? 9 : int(ceil(log2(size / line / assoc)));
    tag                              = debug ? 51 : XML->sys.physical_address_width - idx - int(ceil(log2(line))) + EXTRA_TAG_BITS;
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.cache_sz            = debug ? 32768 : XML->sys.core[ithCore].VPCfilter.dcache_config[0];
    interface_ip.line_sz             = debug ? 64 : XML->sys.core[ithCore].VPCfilter.dcache_config[1];
    interface_ip.assoc               = debug ? 8 : XML->sys.core[ithCore].VPCfilter.dcache_config[2];
    interface_ip.nbanks              = debug ? 1 : XML->sys.core[ithCore].VPCfilter.dcache_config[3];
    interface_ip.out_w               = interface_ip.line_sz * 8;
    interface_ip.access_mode         = 0; // debug?0:XML->sys.core[ithCore].VPCfilter.dcache_config[5];
    interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[4] / clockRate;
    interface_ip.latency             = debug ? 3.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[5] / clockRate;
    interface_ip.is_cache            = true;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.ram_cell_tech_type  = 2;
    interface_ip.peri_global_tech_type          = 2;
    interface_ip.data_arr_ram_cell_tech_type    = 2;
    interface_ip.data_arr_peri_global_tech_type = 2;
    interface_ip.tag_arr_ram_cell_tech_type     = 2;
    interface_ip.tag_arr_peri_global_tech_type  = 2;
    interface_ip.num_rw_ports = debug ? 1 : XML->sys.core[ithCore].memory_ports; // usually In-order has 1 and OOO has 2 at least.
    interface_ip.num_rd_ports = 0;
    interface_ip.num_wr_ports = 0;
    interface_ip.num_se_rd_ports = 0;

    VPCfilter.caches = new ArrayST(&interface_ip, "FL2");
    VPCfilter.area.set_area(VPCfilter.area.get_area() + VPCfilter.caches->local_result.area);
    area.set_area(area.get_area() + VPCfilter.caches->local_result.area);
    // output_data_csv(VPCfilter.caches.local_result);

    // dCache controllers
    // miss buffer
    tag                       = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                      = (XML->sys.physical_address_width) + int(ceil(log2(size / line))) + VPCfilter.caches->l_ip.line_sz;
    interface_ip.specific_tag = 1;
    interface_ip.tag_w        = tag;
    interface_ip.line_sz      = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
    interface_ip.cache_sz     = XML->sys.core[ithCore].VPCfilter.buffer_sizes[0] * interface_ip.line_sz;
    interface_ip.assoc        = 0;
    interface_ip.nbanks       = 1;
    interface_ip.out_w        = interface_ip.line_sz * 8;
    interface_ip.access_mode  = 2;
    interface_ip.throughput   = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[4] / clockRate;
    interface_ip.latency      = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[5] / clockRate;
    interface_ip.obj_func_dyn_energy            = 0;
    interface_ip.obj_func_dyn_power             = 0;
    interface_ip.obj_func_leak_power            = 0;
    interface_ip.obj_func_cycle_t               = 1;
    interface_ip.ram_cell_tech_type             = 1;
    interface_ip.peri_global_tech_type          = 1;
    interface_ip.data_arr_ram_cell_tech_type    = 1;
    interface_ip.data_arr_peri_global_tech_type = 1;
    interface_ip.tag_arr_ram_cell_tech_type     = 1;
    interface_ip.tag_arr_peri_global_tech_type  = 1;
    interface_ip.num_rw_ports                   = debug ? 1 : XML->sys.core[ithCore].memory_ports;
    interface_ip.num_rd_ports                   = 0;
    interface_ip.num_wr_ports                   = 0;
    interface_ip.num_se_rd_ports                = 0;
    VPCfilter.missb                             = new ArrayST(&interface_ip, "FL2MissBuffer");
    VPCfilter.area.set_area(VPCfilter.area.get_area() + VPCfilter.missb->local_result.area);
    area.set_area(area.get_area() + VPCfilter.missb->local_result.area);
    // output_data_csv(VPCfilter.missb.local_result);

    // fill buffer
    tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                             = VPCfilter.caches->l_ip.line_sz;
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
    interface_ip.cache_sz            = data * XML->sys.core[ithCore].VPCfilter.buffer_sizes[1];
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8;
    interface_ip.access_mode         = 2;
    interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[4] / clockRate;
    interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[5] / clockRate;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = debug ? 1 : XML->sys.core[ithCore].memory_ports;
    ;
    interface_ip.num_rd_ports    = 0;
    interface_ip.num_wr_ports    = 0;
    interface_ip.num_se_rd_ports = 0;
    VPCfilter.ifb                = new ArrayST(&interface_ip, "FL2FillBuffer");
    VPCfilter.area.set_area(VPCfilter.area.get_area() + VPCfilter.ifb->local_result.area);
    area.set_area(area.get_area() + VPCfilter.ifb->local_result.area);
    // output_data_csv(VPCfilter.ifb.local_result);

    // prefetch buffer
    tag  = XML->sys.physical_address_width + EXTRA_TAG_BITS; // check with previous entries to decide wthether to merge.
    data = VPCfilter.caches->l_ip.line_sz;                   // separate queue to prevent from cache polution.
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data; // int(pow(2.0,ceil(log2(data))));
    interface_ip.cache_sz            = XML->sys.core[ithCore].VPCfilter.buffer_sizes[2] * interface_ip.line_sz;
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8;
    interface_ip.access_mode         = 2;
    interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[4] / clockRate;
    interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[5] / clockRate;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = debug ? 1 : XML->sys.core[ithCore].memory_ports;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    VPCfilter.prefetchb              = new ArrayST(&interface_ip, "FL2prefetchBuffer");
    // VPCfilter.area.set_area(VPCfilter.area.get_area()+ VPCfilter.prefetchb->local_result.area);
    // area.set_area(area.get_area()+ VPCfilter.prefetchb->local_result.area);
    // output_data_csv(VPCfilter.prefetchb.local_result);

    // WBB
    tag                              = XML->sys.physical_address_width + EXTRA_TAG_BITS;
    data                             = VPCfilter.caches->l_ip.line_sz;
    interface_ip.specific_tag        = 1;
    interface_ip.tag_w               = tag;
    interface_ip.line_sz             = data;
    interface_ip.cache_sz            = XML->sys.core[ithCore].VPCfilter.buffer_sizes[3] * interface_ip.line_sz;
    interface_ip.assoc               = 0;
    interface_ip.nbanks              = 1;
    interface_ip.out_w               = interface_ip.line_sz * 8;
    interface_ip.access_mode         = 2;
    interface_ip.throughput          = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[4] / clockRate;
    interface_ip.latency             = debug ? 1.0 / clockRate : XML->sys.core[ithCore].VPCfilter.dcache_config[5] / clockRate;
    interface_ip.obj_func_dyn_energy = 0;
    interface_ip.obj_func_dyn_power  = 0;
    interface_ip.obj_func_leak_power = 0;
    interface_ip.obj_func_cycle_t    = 1;
    interface_ip.num_rw_ports        = XML->sys.core[ithCore].memory_ports;
    interface_ip.num_rd_ports        = 0;
    interface_ip.num_wr_ports        = 0;
    interface_ip.num_se_rd_ports     = 0;
    VPCfilter.wbb                    = new ArrayST(&interface_ip, "FL2_WBB");
    VPCfilter.area.set_area(VPCfilter.area.get_area() + VPCfilter.wbb->local_result.area);
    area.set_area(area.get_area() + VPCfilter.wbb->local_result.area);
    // output_data_csv(VPCfilter.wbb.local_result);
  } // end check for VPC Filter size 0
}

void LoadStoreU::buildStoreSets() {
  // SSIT
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;

  interface_ip.is_cache = false;
  interface_ip.pure_cam = false;
  interface_ip.pure_ram = true;
  interface_ip.line_sz =
      log2(XML->sys.core[ithCore].lfst_size) * XML->sys.core[ithCore].issue_width / 2; // should be divided by BB4cycle
  interface_ip.cache_sz            = XML->sys.core[ithCore].ssit_size;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 2; // should be BB4cycle
  interface_ip.num_wr_ports        = 2;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 0;
  ssit                             = new ArrayST(&interface_ip, "SSIT");
  ssit->area.set_area(ssit->area.get_area() + ssit->local_result.area);
  area.set_area(area.get_area() + ssit->local_result.area);
  // LFST
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = XML->sys.core[ithCore].lfst_width;
  interface_ip.cache_sz            = XML->sys.core[ithCore].lfst_size;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = 1;
  interface_ip.num_wr_ports        = 1;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = 0;
  lfst                             = new ArrayST(&interface_ip, "LFST");
  lfst->area.set_area(lfst->area.get_area() + lfst->local_result.area);
  area.set_area(area.get_area() + lfst->local_result.area);
#if 0
  // // strb
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = 2*data;
  interface_ip.cache_sz            = 2*data*XML->sys.core[ithCore].scoore_store_retire_buffer_size;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0/clockRate;
  interface_ip.latency             = 1.0/clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = XML->sys.core[ithCore].commit_width;
  interface_ip.num_wr_ports        = XML->sys.core[ithCore].issue_width;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = XML->sys.core[ithCore].commit_width;
  strb                             = new ArrayST(&interface_ip, "strb");
  strb->area.set_area(strb->area.get_area()+ strb->local_result.area);
  area.set_area(area.get_area()+ strb->local_result.area);
  //
  // ldrb
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data*XML->sys.core[ithCore].scoore_load_retire_buffer_size;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz*8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 1.0/clockRate;
  interface_ip.latency             = 1.0/clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = XML->sys.core[ithCore].commit_width;
  interface_ip.num_wr_ports        = XML->sys.core[ithCore].issue_width;
  interface_ip.num_se_rd_ports     = 0;
  interface_ip.num_search_ports    = XML->sys.core[ithCore].commit_width;
  ldrb                             = new ArrayST(&interface_ip, "ldrb");
  ldrb->area.set_area(ldrb->area.get_area()+ ldrb->local_result.area);
  area.set_area(area.get_area()+ ldrb->local_result.area);
  lsq_height                       = 0;
#endif
}

void LoadStoreU::buildLSQ() {
  int tag = XML->sys.core[ithCore].opcode_width + XML->sys.virtual_address_width +
            int(ceil(log2(XML->sys.core[ithCore].number_hardware_threads))) + EXTRA_TAG_BITS;
  int data                  = XML->sys.machine_bits; // 64 is the data width
  interface_ip.is_cache     = true;
  interface_ip.line_sz      = int(ceil(data / 32.0)) * 4;
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = tag;
  interface_ip.cache_sz =
      XML->sys.core[ithCore].store_buffer_size * interface_ip.line_sz * XML->sys.core[ithCore].number_hardware_threads;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 1;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].LSQ_ports;
  LSQ                                         = new ArrayST(&interface_ip, "LSQ");
  LSQ->area.set_area(LSQ->area.get_area() + LSQ->local_result.area);
  area.set_area(area.get_area() + LSQ->local_result.area);
  // output_data_csv(LSQ.LSQ.local_result);
  lsq_height = LSQ->local_result.cache_ht /* XML->sys.core[ithCore].number_hardware_threads */;
}

void LoadStoreU::buildLoadQ() {
  int tag = XML->sys.core[ithCore].opcode_width + XML->sys.virtual_address_width +
            int(ceil(log2(XML->sys.core[ithCore].number_hardware_threads))) + EXTRA_TAG_BITS;
  int data                  = XML->sys.machine_bits; // 64 is the data width
  interface_ip.is_cache     = true;
  interface_ip.line_sz      = int(ceil(data / 32.0)) * 4;
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = tag;
  interface_ip.cache_sz =
      XML->sys.core[ithCore].load_buffer_size * interface_ip.line_sz * XML->sys.core[ithCore].number_hardware_threads;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 1;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.peri_global_tech_type          = 1;
  interface_ip.data_arr_ram_cell_tech_type    = 1;
  interface_ip.data_arr_peri_global_tech_type = 1;
  interface_ip.tag_arr_ram_cell_tech_type     = 1;
  interface_ip.tag_arr_peri_global_tech_type  = 1;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].LSQ_ports;
  LoadQ                                       = new ArrayST(&interface_ip, "LoadQueue");
  LoadQ->area.set_area(LoadQ->area.get_area() + LoadQ->local_result.area);
  area.set_area(area.get_area() + LoadQ->local_result.area);
  // output_data_csv(LoadQ.LoadQ.local_result);
  lsq_height += LSQ->local_result.cache_ht /* XML->sys.core[ithCore].number_hardware_threads */;
}

BranchPredictor::BranchPredictor(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_)
    : XML(XML_interface)
    , ithCore(ithCore_)
    , interface_ip(*interface_ip_)
    , coredynp(dyn_p_)
    , globalBPT(0)
    , L1_localBPT(0)
    , L2_localBPT(0)
    , chooser(0)
    , RAS(0) {
  /*
   * Branch Predictor, accessed during ID stage.
   * McPAT's branch predictor model is the tournament branch predictor used in Alpha 21264,
   * including global predictor, local two level predictor, and Chooser.
   * The Brach predictor also includes a RAS (return address stack) for function calls
   * Branch predictors are tagged by thread ID and modeled as 1-way associative $
   * However RAS return address stacks are duplicated for each thread.
   * TODO:Data Width need to be computed more precisely	 *
   */
  int tag, data;

  clockRate             = coredynp.clockRate;
  executionTime         = coredynp.executionTime;
  interface_ip.assoc    = 1;
  interface_ip.pure_cam = false;
  if(coredynp.multithreaded) {

    tag                       = int(log2(coredynp.num_hthreads) + EXTRA_TAG_BITS);
    interface_ip.specific_tag = 1;
    interface_ip.tag_w        = tag;

    interface_ip.is_cache = true;
    interface_ip.pure_ram = false;
  } else {
    interface_ip.is_cache = false;
    interface_ip.pure_ram = true;
  }
  // Global predictor
  data                             = int(ceil(XML->sys.core[ithCore].predictor.global_predictor_bits / 8.0));
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].predictor.global_predictor_entries;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = coredynp.predictionW;
  interface_ip.num_wr_ports        = coredynp.predictionW;
  interface_ip.num_se_rd_ports     = 0;
  globalBPT                        = new ArrayST(&interface_ip, "GlobalPredictor");
  globalBPT->area.set_area(globalBPT->area.get_area() + globalBPT->local_result.area);
  area.set_area(area.get_area() + globalBPT->local_result.area);

  // Local BPT (Only model single level)
  data                             = int(ceil(XML->sys.core[ithCore].predictor.local_predictor_size / 8.0));
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].predictor.local_predictor_entries;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = coredynp.predictionW;
  interface_ip.num_wr_ports        = coredynp.predictionW;
  interface_ip.num_se_rd_ports     = 0;
  L1_localBPT                      = new ArrayST(&interface_ip, "L1localPredictor");
  L1_localBPT->area.set_area(L1_localBPT->area.get_area() + L1_localBPT->local_result.area);
  area.set_area(area.get_area() + L1_localBPT->local_result.area);

  // Chooser
  data                             = int(ceil(XML->sys.core[ithCore].predictor.chooser_predictor_bits / 8.0));
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].predictor.chooser_predictor_entries;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = coredynp.predictionW;
  interface_ip.num_wr_ports        = coredynp.predictionW;
  interface_ip.num_se_rd_ports     = 0;
  chooser                          = new ArrayST(&interface_ip, "PredictorChooser");
  chooser->area.set_area(chooser->area.get_area() + chooser->local_result.area);
  area.set_area(area.get_area() + chooser->local_result.area);

  // RAS return address stacks are Duplicated for each thread.
  interface_ip.is_cache            = false;
  interface_ip.pure_ram            = true;
  data                             = int(ceil(coredynp.pc_width / 8.0));
  interface_ip.line_sz             = data;
  interface_ip.cache_sz            = data * XML->sys.core[ithCore].RAS_size;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 2;
  interface_ip.throughput          = 1.0 / clockRate;
  interface_ip.latency             = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0;
  interface_ip.num_rd_ports        = coredynp.predictionW;
  interface_ip.num_wr_ports        = coredynp.predictionW;
  interface_ip.num_se_rd_ports     = 0;
  RAS                              = new ArrayST(&interface_ip, "RAS");
  RAS->area.set_area(RAS->area.get_area() + RAS->local_result.area * coredynp.num_hthreads);
  area.set_area(area.get_area() + RAS->local_result.area);
}

void MemManU::buildITLB() {
  int  tag, data;
  bool debug = false;

  interface_ip.is_cache     = true;
  interface_ip.pure_cam     = false;
  interface_ip.pure_ram     = false;
  interface_ip.specific_tag = 1;
  // Itlb TLBs are partioned among threads according to Nigara and Nehalem
  tag = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) +
        int(ceil(log2(XML->sys.core[ithCore].number_hardware_threads))) + EXTRA_TAG_BITS;
  data                 = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
  interface_ip.tag_w   = tag;
  interface_ip.line_sz = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz =
      XML->sys.core[ithCore].itlb.number_entries * interface_ip.line_sz * XML->sys.core[ithCore].number_hardware_threads;
  interface_ip.assoc                       = 0;
  interface_ip.nbanks                      = 1;
  interface_ip.out_w                       = interface_ip.line_sz * 8;
  interface_ip.access_mode                 = 2;
  interface_ip.throughput                  = debug ? 1.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[4] / clockRate;
  interface_ip.latency                     = debug ? 1.0 / clockRate : XML->sys.core[ithCore].icache.icache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy         = 0;
  interface_ip.obj_func_dyn_power          = 0;
  interface_ip.obj_func_leak_power         = 0;
  interface_ip.obj_func_cycle_t            = 1;
  interface_ip.ram_cell_tech_type          = 2;
  interface_ip.peri_global_tech_type       = 2;
  interface_ip.data_arr_ram_cell_tech_type = 2;
  interface_ip.data_arr_peri_global_tech_type = 2;
  interface_ip.tag_arr_ram_cell_tech_type     = 2;
  interface_ip.tag_arr_peri_global_tech_type  = 2;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_wr_ports                   = debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = debug ? 1 : XML->sys.core[ithCore].number_instruction_fetch_ports;
  itlb                                        = new ArrayST(&interface_ip, "ITLB");
  itlb->area.set_area(itlb->area.get_area() + itlb->local_result.area);
  area.set_area(area.get_area() + itlb->local_result.area);
  // output_data_csv(itlb.tlb.local_result);
}

void MemManU::buildDTLB() {
  int  tag, data;
  bool debug = false;
  // dtlb
  tag = XML->sys.virtual_address_width - int(floor(log2(XML->sys.virtual_memory_page_size))) +
        int(ceil(log2(XML->sys.core[ithCore].number_hardware_threads))) + EXTRA_TAG_BITS;
  data                      = XML->sys.physical_address_width - int(floor(log2(XML->sys.virtual_memory_page_size)));
  interface_ip.specific_tag = 1;
  interface_ip.tag_w        = tag;
  interface_ip.line_sz      = int(ceil(data / 8.0)); // int(ceil(pow(2.0,ceil(log2(data)))/8.0));
  interface_ip.cache_sz     = XML->sys.core[ithCore].dtlb.number_entries * interface_ip.line_sz *
                          XML->sys.core[ithCore].number_hardware_threads; // TODO:fix bug in XML
  interface_ip.assoc                       = 0;
  interface_ip.nbanks                      = 1;
  interface_ip.out_w                       = interface_ip.line_sz * 8;
  interface_ip.access_mode                 = 2;
  interface_ip.throughput                  = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[4] / clockRate;
  interface_ip.latency                     = debug ? 1.0 / clockRate : XML->sys.core[ithCore].dcache.dcache_config[5] / clockRate;
  interface_ip.obj_func_dyn_energy         = 0;
  interface_ip.obj_func_dyn_power          = 0;
  interface_ip.obj_func_leak_power         = 0;
  interface_ip.obj_func_cycle_t            = 1;
  interface_ip.peri_global_tech_type       = 0;
  interface_ip.data_arr_ram_cell_tech_type = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].LSQ_ports;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = XML->sys.core[ithCore].LSQ_ports;

  if(XML->sys.core[ithCore].scoore) {
    interface_ip.ram_cell_tech_type             = 1;
    interface_ip.peri_global_tech_type          = 1;
    interface_ip.data_arr_ram_cell_tech_type    = 1;
    interface_ip.data_arr_peri_global_tech_type = 1;
    interface_ip.tag_arr_ram_cell_tech_type     = 1;
    interface_ip.tag_arr_peri_global_tech_type  = 1;
    // interface_ip.assoc                        = 4;
  }

  dtlb = new ArrayST(&interface_ip, "DTLB");
  dtlb->area.set_area(dtlb->area.get_area() + dtlb->local_result.area);
  area.set_area(area.get_area() + dtlb->local_result.area);
  // output_data_csv(dtlb.tlb.local_result);
}

void RegFU::buildIRF() {

  //**********************************IRF***************************************
  int data                         = coredynp.int_data_width;
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = int(ceil(data / 32.0)) * 4;
  interface_ip.cache_sz            = coredynp.num_IRF_entry * interface_ip.line_sz;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 0.3 / clockRate;
  interface_ip.latency             = 0.3 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  interface_ip.num_rw_ports        = 0; // this is the transfer port for saving/restoring states when exceptions happen.
  interface_ip.num_rd_ports        = 5; // 2*XML->sys.core[ithCore].issue_width;
  interface_ip.num_wr_ports        = 1; // XML->sys.core[ithCore].issue_width;
  interface_ip.num_se_rd_ports     = 0;

  // a hack
  interface_ip.ram_cell_tech_type             = 2;
  interface_ip.peri_global_tech_type          = 2;
  interface_ip.data_arr_ram_cell_tech_type    = 2;
  interface_ip.data_arr_peri_global_tech_type = 2;
  interface_ip.tag_arr_ram_cell_tech_type     = 2;
  interface_ip.tag_arr_peri_global_tech_type  = 2;

  IRF = new ArrayST(&interface_ip, "iRF");
  IRF->area.set_area(IRF->area.get_area() + IRF->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + IRF->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  // output_data_csv(IRF.RF.local_result);
}

void RegFU::buildFRF() {

  //**********************************FRF***************************************
  int data                         = coredynp.fp_data_width;
  interface_ip.is_cache            = false;
  interface_ip.pure_cam            = false;
  interface_ip.pure_ram            = true;
  interface_ip.line_sz             = int(ceil(data / 32.0)) * 4;
  interface_ip.cache_sz            = coredynp.num_FRF_entry * interface_ip.line_sz;
  interface_ip.assoc               = 1;
  interface_ip.nbanks              = 1;
  interface_ip.out_w               = interface_ip.line_sz * 8;
  interface_ip.access_mode         = 1;
  interface_ip.throughput          = 0.3 / clockRate;
  interface_ip.latency             = 0.3 / clockRate;
  interface_ip.obj_func_dyn_energy = 0;
  interface_ip.obj_func_dyn_power  = 0;
  interface_ip.obj_func_leak_power = 0;
  interface_ip.obj_func_cycle_t    = 1;
  // a hack
  interface_ip.ram_cell_tech_type             = 2;
  interface_ip.peri_global_tech_type          = 2;
  interface_ip.data_arr_ram_cell_tech_type    = 2;
  interface_ip.data_arr_peri_global_tech_type = 2;
  interface_ip.tag_arr_ram_cell_tech_type     = 2;
  interface_ip.tag_arr_peri_global_tech_type  = 2;
  interface_ip.num_rw_ports                   = 0; // this is the transfer port for saving/restoring states when exceptions happen.
  interface_ip.num_rd_ports                   = 3;
  interface_ip.num_wr_ports                   = 1;
  interface_ip.num_se_rd_ports                = 0;
  FRF                                         = new ArrayST(&interface_ip, "fRF");
  FRF->area.set_area(FRF->area.get_area() + FRF->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + FRF->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  // output_data_csv(FRF.RF.local_result);
  int_regfile_height = IRF->local_result.cache_ht * XML->sys.core[ithCore].number_hardware_threads;
  fp_regfile_height  = FRF->local_result.cache_ht * XML->sys.core[ithCore].number_hardware_threads;
}

void RENAMINGU::buildIFRATRam() {

  int data, out_w;

  data                                        = int(ceil(coredynp.phy_ireg_width / 8.0));
  out_w                                       = int(ceil(coredynp.phy_ireg_width / 8.0));
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = 2 * data * XML->sys.core[ithCore].archi_Regs_IRF_size;
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = out_w * 8;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 2.0 / clockRate;
  interface_ip.latency                        = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 1; // the extra one port is for GCs
  interface_ip.num_rd_ports                   = 2 * coredynp.decodeW;
  interface_ip.num_wr_ports                   = coredynp.decodeW;
  interface_ip.num_se_rd_ports                = 0;
  iFRAT                                       = new ArrayST(&interface_ip, "iFRAT");
  iFRAT->area.set_area(iFRAT->area.get_area() + iFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + iFRAT->area.get_area() +
                iFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildFFRATRam() {
  int data, out_w;
  // FRAT floating point
  data                                        = int(ceil(coredynp.phy_freg_width / 8.0));
  out_w                                       = int(ceil(coredynp.phy_freg_width / 8.0));
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = 2 * data * XML->sys.core[ithCore].archi_Regs_FRF_size;
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = out_w * 8;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 2.0 / clockRate;
  interface_ip.latency                        = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 1; // the extra one port is for GCs
  interface_ip.num_rd_ports                   = 2 * coredynp.fp_decodeW;
  interface_ip.num_wr_ports                   = coredynp.fp_decodeW;
  interface_ip.num_se_rd_ports                = 0;
  fFRAT                                       = new ArrayST(&interface_ip, "fFRAT");
  fFRAT->area.set_area(fFRAT->area.get_area() + fFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + fFRAT->area.get_area() +
                fFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildIFRATCam() {
  int tag, data, out_w;
  // FRAT
  tag   = coredynp.arch_ireg_width;
  data  = int(ceil(coredynp.arch_ireg_width + 1 * coredynp.globalCheckpoint / 8.0)); // the address of CAM needed to be sent out
  out_w = int(ceil(coredynp.arch_ireg_width / 8.0));
  interface_ip.is_cache                       = true;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = false;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * XML->sys.core[ithCore].phy_Regs_IRF_size;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = out_w;
  interface_ip.specific_tag                   = 1;
  interface_ip.tag_w                          = tag;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 2.0 / clockRate;
  interface_ip.latency                        = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 1; // for GCs
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_wr_ports                   = coredynp.decodeW;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = 2 * coredynp.decodeW;
  iFRAT                                       = new ArrayST(&interface_ip, "iFRAT");
  iFRAT->area.set_area(iFRAT->area.get_area() + iFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + iFRAT->area.get_area() +
                iFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildFFRATCam() {
  int tag, data, out_w;
  // FRAT for FP
  tag   = coredynp.arch_freg_width;
  data  = int(ceil(coredynp.arch_freg_width + 1 * coredynp.globalCheckpoint / 8.0)); // the address of CAM needed to be sent out
  out_w = int(ceil(coredynp.arch_freg_width / 8.0));
  interface_ip.is_cache                       = true;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = false;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * XML->sys.core[ithCore].phy_Regs_FRF_size;
  interface_ip.assoc                          = 0;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = out_w;
  interface_ip.specific_tag                   = 1;
  interface_ip.tag_w                          = tag;
  interface_ip.access_mode                    = 2;
  interface_ip.throughput                     = 1.0 / clockRate;
  interface_ip.latency                        = 1.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 1; // for GCs
  interface_ip.num_rd_ports                   = 0;
  interface_ip.num_wr_ports                   = coredynp.fp_decodeW;
  interface_ip.num_se_rd_ports                = 0;
  interface_ip.num_search_ports               = 2 * coredynp.fp_decodeW;
  fFRAT                                       = new ArrayST(&interface_ip, "fFRAT");
  fFRAT->area.set_area(fFRAT->area.get_area() + fFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + fFRAT->area.get_area() +
                fFRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildIRRAT() {

  // RRAT is always RAM based, does not have GCs, and is used only for record latest non-speculative mapping
  int data                                 = int(ceil(coredynp.phy_ireg_width / 8.0));
  interface_ip.is_cache                    = false;
  interface_ip.pure_cam                    = false;
  interface_ip.pure_ram                    = true;
  interface_ip.line_sz                     = data;
  interface_ip.cache_sz                    = data * XML->sys.core[ithCore].archi_Regs_IRF_size * 2; // HACK to make it as least 64B
  interface_ip.assoc                       = 1;
  interface_ip.nbanks                      = 1;
  interface_ip.out_w                       = interface_ip.line_sz * 8;
  interface_ip.access_mode                 = 1;
  interface_ip.throughput                  = 2.0 / clockRate;
  interface_ip.latency                     = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy         = 0;
  interface_ip.obj_func_dyn_power          = 0;
  interface_ip.obj_func_leak_power         = 0;
  interface_ip.obj_func_cycle_t            = 1;
  interface_ip.ram_cell_tech_type          = 0;
  interface_ip.peri_global_tech_type       = 0;
  interface_ip.data_arr_ram_cell_tech_type = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = XML->sys.core[ithCore].commit_width;
  interface_ip.num_wr_ports                   = XML->sys.core[ithCore].commit_width;
  interface_ip.num_se_rd_ports                = 0;
  iRRAT                                       = new ArrayST(&interface_ip, "iRRAT");
  iRRAT->area.set_area(iRRAT->area.get_area() + iRRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + iRRAT->area.get_area() +
                iRRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildFRRAT() {
  // RRAT for FP
  int data                                 = int(ceil(coredynp.phy_freg_width / 8.0));
  interface_ip.is_cache                    = false;
  interface_ip.pure_cam                    = false;
  interface_ip.pure_ram                    = true;
  interface_ip.line_sz                     = data;
  interface_ip.cache_sz                    = 2 * data * XML->sys.core[ithCore].archi_Regs_FRF_size; // HACK to make it as least 64B
  interface_ip.assoc                       = 1;
  interface_ip.nbanks                      = 1;
  interface_ip.out_w                       = interface_ip.line_sz * 8;
  interface_ip.access_mode                 = 1;
  interface_ip.throughput                  = 2.0 / clockRate;
  interface_ip.latency                     = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy         = 0;
  interface_ip.obj_func_dyn_power          = 0;
  interface_ip.obj_func_leak_power         = 0;
  interface_ip.obj_func_cycle_t            = 1;
  interface_ip.ram_cell_tech_type          = 0;
  interface_ip.peri_global_tech_type       = 0;
  interface_ip.data_arr_ram_cell_tech_type = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0;
  interface_ip.num_rd_ports                   = coredynp.fp_decodeW;
  interface_ip.num_wr_ports                   = coredynp.fp_decodeW;
  interface_ip.num_se_rd_ports                = 0;
  fRRAT                                       = new ArrayST(&interface_ip, "fRRAT");
  fRRAT->area.set_area(fRRAT->area.get_area() + fRRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + fRRAT->area.get_area() +
                fRRAT->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildIFreeL() {
  // Freelist of renaming unit always RAM based
  // Recycle happens at two places: 1)when DCL check there are WAW, the Phyregisters/ROB directly recycles into freelist
  // 2)When instruction commits the Phyregisters/ROB needed to be recycled.
  // therefore num_wr port = decode-1(-1 means at least one phy reg will be used for the current renaming group) + commit width
  int data                                    = int(ceil(coredynp.phy_ireg_width / 8.0));
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * coredynp.num_ifreelist_entries;
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 1;
  interface_ip.throughput                     = 2.0 / clockRate;
  interface_ip.latency                        = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 0; // TODO
  interface_ip.num_rd_ports                   = coredynp.decodeW;
  interface_ip.num_wr_ports                   = coredynp.decodeW - 1 + XML->sys.core[ithCore].commit_width;
  interface_ip.num_se_rd_ports                = 0;
  ifreeL                                      = new ArrayST(&interface_ip, "iFree");
  ifreeL->area.set_area(ifreeL->area.get_area() + ifreeL->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + ifreeL->area.get_area() +
                ifreeL->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}

void RENAMINGU::buildFFreeL() {
  // freelist for FP
  int data                                    = int(ceil((coredynp.phy_freg_width / 8.0)));
  interface_ip.is_cache                       = false;
  interface_ip.pure_cam                       = false;
  interface_ip.pure_ram                       = true;
  interface_ip.line_sz                        = data;
  interface_ip.cache_sz                       = data * coredynp.num_ffreelist_entries;
  interface_ip.assoc                          = 1;
  interface_ip.nbanks                         = 1;
  interface_ip.out_w                          = interface_ip.line_sz * 8;
  interface_ip.access_mode                    = 1;
  interface_ip.throughput                     = 2.0 / clockRate;
  interface_ip.latency                        = 2.0 / clockRate;
  interface_ip.obj_func_dyn_energy            = 0;
  interface_ip.obj_func_dyn_power             = 0;
  interface_ip.obj_func_leak_power            = 0;
  interface_ip.obj_func_cycle_t               = 1;
  interface_ip.ram_cell_tech_type             = 0;
  interface_ip.peri_global_tech_type          = 0;
  interface_ip.data_arr_ram_cell_tech_type    = 0;
  interface_ip.data_arr_peri_global_tech_type = 0;
  interface_ip.tag_arr_ram_cell_tech_type     = 0;
  interface_ip.tag_arr_peri_global_tech_type  = 0;
  interface_ip.num_rw_ports                   = 1;
  interface_ip.num_rd_ports                   = coredynp.fp_decodeW;
  interface_ip.num_wr_ports                   = coredynp.fp_decodeW - 1 + XML->sys.core[ithCore].commit_width;
  interface_ip.num_se_rd_ports                = 0;
  ffreeL                                      = new ArrayST(&interface_ip, "fFree");
  ffreeL->area.set_area(ffreeL->area.get_area() + ffreeL->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
  area.set_area(area.get_area() + ffreeL->area.get_area() +
                ffreeL->local_result.area * XML->sys.core[ithCore].number_hardware_threads);
}
