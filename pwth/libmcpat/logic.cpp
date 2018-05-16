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

#include "logic.h"
#include "XML_Parse.h"
#include "arch_const.h"
#include "basic_circuit.h"
#include "decoder.h"
#include "parameter.h"
#include "time.h"
#include "xmlParser.h"
#include <assert.h>
#include <iostream>
#include <math.h>
#include <string.h>

// selection_logic
selection_logic::selection_logic(bool _is_default, int win_entries_, int issue_width_, const InputParameter *configure_interface)
    // const ParseXML *_XML_interface)
    : is_default(_is_default)
    , win_entries(win_entries_)
    , issue_width(issue_width_) {
  // uca_org_t result2;
  l_ip         = *configure_interface;
  local_result = init_interface(&l_ip);
  // init_tech_params(l_ip.F_sz_um, false);
  // win_entries=numIBEntries;//IQentries;
  // issue_width=issueWidth;
  selection_power();
  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
}

void selection_logic::selection_power() { // based on cost effective superscalar processor TR pp27-31
  double Ctotal, Cor, Cpencode;
  int num_arbiter;
  double WSelORn, WSelORprequ, WSelPn, WSelPp, WSelEnn, WSelEnp;

  // TODO: the 0.8um process data is used.
  WSelORn     = 12.5 * l_ip.F_sz_um;  // this was 10 micron for the 0.8 micron process
  WSelORprequ = 50 * l_ip.F_sz_um;    // this was 40 micron for the 0.8 micron process
  WSelPn      = 12.5 * l_ip.F_sz_um;  // this was 10mcron for the 0.8 micron process
  WSelPp      = 18.75 * l_ip.F_sz_um; // this was 15 micron for the 0.8 micron process
  WSelEnn     = 6.25 * l_ip.F_sz_um;  // this was 5 micron for the 0.8 micron process
  WSelEnp     = 12.5 * l_ip.F_sz_um;  // this was 10 micron for the 0.8 micron process

  Ctotal      = 0;
  num_arbiter = 1;
  while(win_entries > 4) {
    win_entries = (int)ceil((double)win_entries / 4.0);
    num_arbiter += win_entries;
  }
  // the 4-input OR logic to generate anyreq
  Cor                       = 4 * drain_C_(WSelORn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(WSelORprequ, PCH, 1, 1, g_tp.cell_h_def);
  power.readOp.gate_leakage = cmos_Ig_leakage(WSelORn, WSelORprequ, 4, nor) * g_tp.peri_global.Vdd;

  // The total capacity of the 4-bit priority encoder
  Cpencode = drain_C_(WSelPn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(WSelPp, PCH, 1, 1, g_tp.cell_h_def) +
             2 * drain_C_(WSelPn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(WSelPp, PCH, 2, 1, g_tp.cell_h_def) +
             3 * drain_C_(WSelPn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(WSelPp, PCH, 3, 1, g_tp.cell_h_def) +
             4 * drain_C_(WSelPn, NCH, 1, 1, g_tp.cell_h_def) +
             drain_C_(WSelPp, PCH, 4, 1, g_tp.cell_h_def) + // precompute priority logic
             2 * 4 * gate_C(WSelEnn + WSelEnp, 20.0) + 4 * drain_C_(WSelEnn, NCH, 1, 1, g_tp.cell_h_def) +
             2 * 4 * drain_C_(WSelEnp, PCH, 1, 1, g_tp.cell_h_def) +      // enable logic
             (2 * 4 + 2 * 3 + 2 * 2 + 2) * gate_C(WSelPn + WSelPp, 10.0); // requests signal

  Ctotal += issue_width * num_arbiter * (Cor + Cpencode);

  power.readOp.dynamic =
      Ctotal * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd * 2; // 2 means the abitration signal need to travel round trip
  power.readOp.leakage =
      issue_width * num_arbiter *
      (cmos_Isub_leakage(WSelPn, WSelPp, 2, nor) /*approximate precompute with a nor gate*/ // grant1p
       + cmos_Isub_leakage(WSelPn, WSelPp, 3, nor)                                          // grant2p
       + cmos_Isub_leakage(WSelPn, WSelPp, 4, nor)                                          // grant3p
       + cmos_Isub_leakage(WSelEnn, WSelEnp, 2, nor) * 4                                    // enable logic
       + cmos_Isub_leakage(WSelEnn, WSelEnp, 1, inv) * 2 * 3 // for each grant there are two inverters, there are 3 grant sIsubnals
       ) *
      g_tp.peri_global.Vdd;
  power.readOp.gate_leakage =
      issue_width * num_arbiter *
      (cmos_Ig_leakage(WSelPn, WSelPp, 2, nor) /*approximate precompute with a nor gate*/ // grant1p
       + cmos_Ig_leakage(WSelPn, WSelPp, 3, nor)                                          // grant2p
       + cmos_Ig_leakage(WSelPn, WSelPp, 4, nor)                                          // grant3p
       + cmos_Ig_leakage(WSelEnn, WSelEnp, 2, nor) * 4                                    // enable logic
       + cmos_Ig_leakage(WSelEnn, WSelEnp, 1, inv) * 2 * 3 // for each grant there are two inverters, there are 3 grant signals
       ) *
      g_tp.peri_global.Vdd;
}

dep_resource_conflict_check::dep_resource_conflict_check(const InputParameter *configure_interface, const CoreDynParam &dyn_p_,
                                                         int compare_bits_, bool _is_default)
    : l_ip(*configure_interface)
    , coredynp(dyn_p_)
    , compare_bits(compare_bits_)
    , is_default(_is_default) {
  Wcompn      = 25 * l_ip.F_sz_um;     // this was 20.0 micron for the 0.8 micron process
  Wevalinvp   = 25 * l_ip.F_sz_um;     // this was 20.0 micron for the 0.8 micron process
  Wevalinvn   = 100 * l_ip.F_sz_um;    // this was 80.0 mcron for the 0.8 micron process
  Wcomppreequ = 50 * l_ip.F_sz_um;     // this was 40.0  micron for the 0.8 micron process
  WNORn       = 6.75 * l_ip.F_sz_um;   // this was 5.4 micron for the 0.8 micron process
  WNORp       = 38.125 * l_ip.F_sz_um; // this was 30.5 micron for the 0.8 micron process

  local_result = init_interface(&l_ip);

  if(coredynp.core_ty == Inorder)
    compare_bits += 16 + 8; // TODO: opcode bits + log(shared resources)-->opcode comparator
  else
    compare_bits += 16 + 8;

  conflict_check_power();
  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
  local_result.power = power;
}

void dep_resource_conflict_check::conflict_check_power() {
  double Ctotal;
  int    num_comparators;
  num_comparators =
      3 * ((coredynp.decodeW) * (coredynp.decodeW) -
           coredynp.decodeW); // 2(N*N-N) is used for source to dest comparison, (N*N-N) is used for dest to dest comparision.
  // When decode-width ==1, no dcl logic

  Ctotal = num_comparators * compare_cap();
  // printf("%i,%s\n",XML_interface->sys.core[0].predictor.predictor_entries,XML_interface->sys.core[0].predictor.prediction_scheme);

  power.readOp.dynamic      = Ctotal * /*CLOCKRATE*/ g_tp.peri_global.Vdd * g_tp.peri_global.Vdd /*AF*/;
  power.readOp.leakage      = num_comparators * compare_bits * 2 * simplified_nmos_leakage(Wcompn, false);
  power.readOp.gate_leakage = num_comparators * compare_bits * 2 * cmos_Ig_leakage(Wcompn, 0, 2, nmos);
}

/* estimate comparator power consumption (this comparator is similar
   to the tag-match structure in a CAM */
double dep_resource_conflict_check::compare_cap() {
  double c1, c2;

  WNORp = WNORp * compare_bits / 2.0; // resize the big NOR gate at the DCL according to fan in.
  /* bottom part of comparator */
  c2 = (compare_bits) * (drain_C_(Wcompn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(Wcompn, NCH, 2, 1, g_tp.cell_h_def)) +
       drain_C_(Wevalinvp, PCH, 1, 1, g_tp.cell_h_def) + drain_C_(Wevalinvn, NCH, 1, 1, g_tp.cell_h_def);

  /* top part of comparator */
  c1 = (compare_bits) * (drain_C_(Wcompn, NCH, 1, 1, g_tp.cell_h_def) + drain_C_(Wcompn, NCH, 2, 1, g_tp.cell_h_def) +
                         drain_C_(Wcomppreequ, NCH, 1, 1, g_tp.cell_h_def)) +
       gate_C(WNORn + WNORp, 10.0) + drain_C_(WNORp, NCH, 2, 1, g_tp.cell_h_def) +
       compare_bits * drain_C_(WNORn, NCH, 2, 1, g_tp.cell_h_def);
  return (c1 + c2);
}

// TODO: add inverter and transmission gate base DFF.

DFFCell::DFFCell(bool _is_dram, double _WdecNANDn, double _WdecNANDp, double _cell_load, const InputParameter *configure_interface)
    : is_dram(_is_dram)
    , cell_load(_cell_load)
    , WdecNANDn(_WdecNANDn)
    , WdecNANDp(_WdecNANDp) { // this model is based on the NAND2 based DFF.
  l_ip = *configure_interface;
  area.set_area(280 * l_ip.F_sz_um * l_ip.F_sz_um);
}

double DFFCell::fpfp_node_cap(unsigned int fan_in, unsigned int fan_out) {
  double Ctotal = 0;
  // printf("WdecNANDn = %E\n", WdecNANDn);

  /* part 1: drain cap of NAND gate */
  Ctotal +=
      drain_C_(WdecNANDn, NCH, 2, 1, g_tp.cell_h_def, is_dram) + fan_in * drain_C_(WdecNANDp, PCH, 1, 1, g_tp.cell_h_def, is_dram);

  /* part 2: gate cap of NAND gates */
  Ctotal += fan_out * gate_C(WdecNANDn + WdecNANDp, 0, is_dram);

  return Ctotal;
}

void DFFCell::compute_DFF_cell() {
  double c1, c2, c3, c4, c5, c6;
  /* node 5 and node 6 are identical to node 1 in capacitance */
  c1 = c5 = c6 = fpfp_node_cap(2, 1);
  c2           = fpfp_node_cap(2, 3);
  c3           = fpfp_node_cap(3, 2);
  c4           = fpfp_node_cap(2, 2);

  // cap-load of the clock signal in each Dff, actually the clock signal only connected to one NAND2
  clock_cap = 2 * gate_C(WdecNANDn + WdecNANDp, 0, is_dram);
  e_switch.readOp.dynamic += (c4 + c1 + c2 + c3 + c5 + c6 + 2 * cell_load) * 0.5 * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd;
  ;

  /* no 1/2 for e_keep and e_clock because clock signal switches twice in one cycle */
  e_keep_1.readOp.dynamic += c3 * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd;
  e_keep_0.readOp.dynamic += c2 * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd;
  e_clock.readOp.dynamic += clock_cap * g_tp.peri_global.Vdd * g_tp.peri_global.Vdd;
  ;

  /* static power */
  e_switch.readOp.leakage += (cmos_Isub_leakage(WdecNANDn, WdecNANDp, 2, nand) * 5 // 5 NAND2 and 1 NAND3 in a DFF
                              + cmos_Isub_leakage(WdecNANDn, WdecNANDn, 3, nand)) *
                             g_tp.peri_global.Vdd;
  e_switch.readOp.gate_leakage += (cmos_Ig_leakage(WdecNANDn, WdecNANDp, 2, nand) * 5 // 5 NAND2 and 1 NAND3 in a DFF
                                   + cmos_Ig_leakage(WdecNANDn, WdecNANDn, 3, nand)) *
                                  g_tp.peri_global.Vdd;
  // printf("leakage =%E\n",cmos_Ileak(1, is_dram) );
}

PipelinePower::PipelinePower(const InputParameter *configure_interface, const CoreDynParam &dyn_p_, bool _is_core_pipeline,
                             bool _is_default)
    : l_ip(*configure_interface)
    , coredynp(dyn_p_)
    , is_core_pipeline(_is_core_pipeline)
    , is_default(_is_default)
    , num_piperegs(0.0)

{
  local_result = init_interface(&l_ip);
  process_ind  = true;
  WNANDn       = (process_ind) ? 25 * l_ip.F_sz_um : 2 * g_tp.min_w_nmos_; // this was  20 micron for the 0.8 micron process
  WNANDp       = (process_ind) ? 37.5 * l_ip.F_sz_um
                         : g_tp.min_w_nmos_ * pmos_to_nmos_sz_ratio(); // this was  30 micron for the 0.8 micron process
  load_per_pipeline_stage = 2 * gate_C(WNANDn + WNANDp, 0, false);
  compute();
}

void PipelinePower::compute() {
  compute_stage_vector();
  DFFCell pipe_reg(false, WNANDn, WNANDp, load_per_pipeline_stage, &l_ip);
  pipe_reg.compute_DFF_cell();

  double clock_power_pipereg = num_piperegs * pipe_reg.e_clock.readOp.dynamic;
  //******************pipeline power: currently, we average all the possibilities of the states of DFFs in the pipeline. A better
  // way to do it is to consider the harming distance of two consecutive signals, However McPAT does not have plan to do this in
  // near
  // future as it focuses on worst case power.
  double pipe_reg_power =
      num_piperegs * (pipe_reg.e_switch.readOp.dynamic + pipe_reg.e_keep_0.readOp.dynamic + pipe_reg.e_keep_1.readOp.dynamic) / 3 +
      clock_power_pipereg;
  double pipe_reg_leakage      = num_piperegs * pipe_reg.e_switch.readOp.leakage;
  double pipe_reg_gate_leakage = num_piperegs * pipe_reg.e_switch.readOp.gate_leakage;
  power.readOp.dynamic += pipe_reg_power;
  power.readOp.leakage += pipe_reg_leakage;
  power.readOp.gate_leakage += pipe_reg_gate_leakage;
  area.set_area(num_piperegs * pipe_reg.area.get_area());

  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
  double macro_layout_overhead = g_tp.macro_layout_overhead;
  area.set_area(area.get_area() * macro_layout_overhead);
}

void PipelinePower::compute_stage_vector() {
  double num_stages, tot_stage_vector, per_stage_vector;
  // Hthread = thread_clock_gated? 1:num_thread;

  if(!is_core_pipeline) {
    num_piperegs = l_ip.pipeline_stages * l_ip.per_stage_vector; // The number of pipeline stages are calculated based on the
                                                                 // achievable throughput and required throughput
  } else {
    if(coredynp.core_ty == Inorder) {
      /* assume 6 pipe stages and try to estimate bits per pipe stage */
      /* pipe stage 0/IF */
      num_piperegs += coredynp.pc_width * 2 * coredynp.num_hthreads;
      /* pipe stage IF/ID */
      num_piperegs += coredynp.fetchW * (coredynp.instruction_length + coredynp.pc_width) * coredynp.num_hthreads;
      /* pipe stage IF/ThreadSEL */
      if(coredynp.multithreaded)
        num_piperegs += coredynp.num_hthreads * coredynp.perThreadState; // 8 bit thread states
      /* pipe stage ID/EXE */
      num_piperegs +=
          coredynp.decodeW *
          (coredynp.instruction_length + coredynp.pc_width + powers(2, coredynp.opcode_length) + 2 * coredynp.int_data_width) *
          coredynp.num_hthreads;
      /* pipe stage EXE/MEM */
      num_piperegs += coredynp.issueW * (3 * coredynp.arch_ireg_width + powers(2, coredynp.opcode_length) +
                                         8 * 2 * coredynp.int_data_width /*+2*powers (2,reg_length)*/);
      /* pipe stage MEM/WB the 2^opcode_length means the total decoded signal for the opcode*/
      num_piperegs += coredynp.issueW * (2 * coredynp.int_data_width + powers(2, coredynp.opcode_length) +
                                         8 * 2 * coredynp.int_data_width /*+2*powers (2,reg_length)*/);
      //		/* pipe stage 5/6 */
      //		num_piperegs += issueWidth*(data_width + powers (2,opcode_length)/*+2*powers (2,reg_length)*/);
      //		/* pipe stage 6/7 */
      //		num_piperegs += issueWidth*(data_width + powers (2,opcode_length)/*+2*powers (2,reg_length)*/);
      //		/* pipe stage 7/8 */
      //		num_piperegs += issueWidth*(data_width + powers (2,opcode_length)/**2*powers (2,reg_length)*/);
      //		/* assume 50% extra in control signals (rule of thumb) */
      num_stages = 6;

    } else {
      /* assume 12 stage pipe stages and try to estimate bits per pipe stage */
      /*OOO: Fetch, decode, rename, IssueQ, dispatch, regread, EXE, MEM, WB, CM */

      /* pipe stage 0/1F*/
      num_piperegs += coredynp.pc_width * 2 * coredynp.num_hthreads; // PC and Next PC
      /* pipe stage IF/ID */
      num_piperegs += coredynp.fetchW * (coredynp.instruction_length + coredynp.pc_width) *
                      coredynp.num_hthreads; // PC is used to feed branch predictor in ID
      /* pipe stage 1D/Renaming*/
      num_piperegs += coredynp.decodeW * (coredynp.instruction_length + coredynp.pc_width) *
                      coredynp.num_hthreads; // PC is for branch exe in later stage.
      /* pipe stage Renaming/wire_drive */
      num_piperegs += coredynp.decodeW * (coredynp.instruction_length + coredynp.pc_width);
      /* pipe stage Renaming/IssueQ */
      num_piperegs +=
          coredynp.issueW * (coredynp.instruction_length + coredynp.pc_width + 3 * coredynp.phy_ireg_width) * coredynp.num_hthreads;
      /* pipe stage IssueQ/Dispatch */
      num_piperegs += coredynp.issueW * (coredynp.instruction_length + 3 * coredynp.phy_ireg_width);
      /* pipe stage Dispatch/EXE */
      num_piperegs += coredynp.issueW * (3 * coredynp.phy_ireg_width + coredynp.pc_width +
                                         powers(2, coredynp.opcode_length) /*+2*powers (2,reg_length)*/);
      /* 2^opcode_length means the total decoded signal for the opcode*/
      num_piperegs +=
          coredynp.issueW * (2 * coredynp.int_data_width + powers(2, coredynp.opcode_length) /*+2*powers (2,reg_length)*/);
      /*2 source operands in EXE; Assume 2EXE stages* since we do not really distinguish OP*/
      num_piperegs +=
          coredynp.issueW * (2 * coredynp.int_data_width + powers(2, coredynp.opcode_length) /*+2*powers (2,reg_length)*/);
      /* pipe stage EXE/MEM, data need to be read/write, address*/
      num_piperegs += coredynp.issueW *
                      (coredynp.int_data_width + coredynp.v_address_width +
                       powers(2, coredynp.opcode_length) /*+2*powers (2,reg_length)*/); // memory Opcode still need to be passed
      /* pipe stage MEM/WB; result data, writeback regs */
      num_piperegs +=
          coredynp.issueW * (coredynp.int_data_width +
                             coredynp.phy_ireg_width /* powers (2,opcode_length) + (2,opcode_length)+2*powers (2,reg_length)*/);
      /* pipe stage WB/CM ; result data, regs need to be updated, address for resolve memory ops in ROB's top*/
      num_piperegs += coredynp.commitW *
                      (coredynp.int_data_width + coredynp.v_address_width +
                       coredynp.phy_ireg_width /*+ powers (2,opcode_length)*2*powers (2,reg_length)*/) *
                      coredynp.num_hthreads;
      //		if (multithreaded)
      //		{
      //
      //		}
      num_stages = 12;
    }

    /* assume 50% extra in control registers and interrupt registers (rule of thumb) */
    num_piperegs     = int(ceil(num_piperegs * 1.5));
    tot_stage_vector = num_piperegs;
    per_stage_vector = tot_stage_vector / num_stages;

    if(coredynp.core_ty == Inorder) {
      if(coredynp.pipeline_stages > 6)
        num_piperegs = per_stage_vector * coredynp.pipeline_stages;
    } else { // OOO
      if(coredynp.pipeline_stages > 12)
        num_piperegs = per_stage_vector * coredynp.pipeline_stages;
    }
  }
}

FunctionalUnit::FunctionalUnit(const InputParameter *configure_interface, enum FU_type fu_type, int n_fu, bool _is_default)
    : l_ip(*configure_interface)
    , num_fu(n_fu)
    , is_default(_is_default) {
  double area_t, energy, leakage, gate_leakage;
  double pmos_to_nmos_sizing_r = pmos_to_nmos_sz_ratio();

  exist = true;
  // XML_interface=_XML_interface;
  // uca_org_t result2;
  local_result = init_interface(&l_ip);

  if(fu_type == FPU) {
    area_t  = 4.47 * 1e6 * g_tp.scaling_factor.logic_scaling_co_eff; // this is um^2
    leakage = area_t * (g_tp.scaling_factor.core_tx_density / 10) *
              cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) * g_tp.peri_global.Vdd /
              2; // unit W
    gate_leakage = area_t * (g_tp.scaling_factor.core_tx_density / 10) *
                   cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) * g_tp.peri_global.Vdd /
                   2;               // unit W
    energy    = 0.3529 / 10 * 1e-9; // this is the energy(nJ) for a FP instruction in FPU usually it can have up to 20 cycles.
    FU_height = (38667 * num_fu) * l_ip.F_sz_um; // FPU from Sun's data
  } else {
    area_t  = 280 * 206 * g_tp.scaling_factor.logic_scaling_co_eff; // this is um^2
    leakage = area_t * (g_tp.scaling_factor.core_tx_density / 10) *
              cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) * g_tp.peri_global.Vdd /
              2; // unit W
    gate_leakage = area_t * (g_tp.scaling_factor.core_tx_density / 10) *
                   cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) * g_tp.peri_global.Vdd / 2;
    energy    = 0.00649 * 1e-9;                        // This is per cycle energy(nJ)
    FU_height = (4111 * num_fu + 6904) * l_ip.F_sz_um; // integer ALU + divider/mul from Intel's data
  }
  // IEXEU, simple ALU and FPU
  //  double C_ALU, C_EXEU, C_FPU; //Lum Equivalent capacitance of IEXEU and FPU. Based on Intel and Sun 90nm process fabracation.
  //
  //  C_ALU	  = 0.025e-9;//F
  //  C_EXEU  = 0.05e-9; //F
  //  C_FPU	  = 0.35e-9;//F
  area.set_area(area_t * num_fu);
  power.readOp.dynamic      = energy * num_fu;
  power.readOp.leakage      = leakage * num_fu;
  power.readOp.gate_leakage = gate_leakage * num_fu;

  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
  double macro_layout_overhead = g_tp.macro_layout_overhead;
  area.set_area(area.get_area() * macro_layout_overhead);
  local_result.power = power;
}

UndiffCore::UndiffCore(const InputParameter *configure_interface, enum Core_type core_ty_, double pipeline_stage_,
                       double num_hthreads_, double issue_width_, bool embedded_, bool _is_default)
    : l_ip(*configure_interface)
    , core_ty(core_ty_)
    , pipeline_stage(pipeline_stage_)
    , num_hthreads(num_hthreads_)
    , issue_width(issue_width_)
    , embedded(embedded_)
    , is_default(_is_default) {
  double undifferentiated_core = 0;
  double core_tx_density       = 0;
  double pmos_to_nmos_sizing_r = pmos_to_nmos_sz_ratio();
  // XML_interface=_XML_interface;
  // uca_org_t result2;
  local_result = init_interface(&l_ip);

  // Compute undifferentiated core area at 90nm.
  if(embedded == false) {
    // Based on the results of polynomial/log curve fitting based on undifferentiated core of Niagara, Niagara2, Merom, Penyrn,
    // Prescott die measurements
    if(core_ty == OOO)
      undifferentiated_core = (0.0306 * pipeline_stage * pipeline_stage - 0.2393 * pipeline_stage + 29.405) * 0.5; // OOO
    else if(core_ty == Inorder)
      undifferentiated_core = (0.1238 * pipeline_stage + 7.2572) * 0.9; // inorder
    else {
      cout << "invalid core type" << endl;
      exit(0);
    }
    undifferentiated_core *= (1 + logtwo(num_hthreads) * 0.0716);
  } else {
    // Based on the results in paper "parametrized processor models" Sandia Labs
    undifferentiated_core = 0.4109 * pipeline_stage - 0.776;
    undifferentiated_core *= (1 + logtwo(num_hthreads) * 0.0426);
  }

  undifferentiated_core *= g_tp.scaling_factor.logic_scaling_co_eff * 1e6; // change from mm^2 to um^2
  core_tx_density = g_tp.scaling_factor.core_tx_density;

  power.readOp.leakage = undifferentiated_core * (core_tx_density / 10) *
                         cmos_Isub_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                         g_tp.peri_global.Vdd / 2; // unit W
  power.readOp.gate_leakage = undifferentiated_core * (core_tx_density / 10) *
                              cmos_Ig_leakage(g_tp.min_w_nmos_, g_tp.min_w_nmos_ * pmos_to_nmos_sizing_r, 1, inv) *
                              g_tp.peri_global.Vdd / 2;
  area.set_area(undifferentiated_core);

  double sckRation = g_tp.sckt_co_eff;
  power.readOp.dynamic *= sckRation;
  power.writeOp.dynamic *= sckRation;
  power.searchOp.dynamic *= sckRation;
  double macro_layout_overhead = g_tp.macro_layout_overhead;
  area.set_area(area.get_area() * macro_layout_overhead);

  local_result.power = power;

  //		double vt=g_tp.peri_global.Vth;
  //		double velocity_index=1.1;
  //		double c_in=gate_C(g_tp.min_w_nmos_, g_tp.min_w_nmos_*pmos_to_nmos_sizing_r , 0.0, false);
  //		double c_out= drain_C_(g_tp.min_w_nmos_, NCH, 2, 1, g_tp.cell_h_def, false) +
  // drain_C_(g_tp.min_w_nmos_*pmos_to_nmos_sizing_r, PCH, 1, 1, g_tp.cell_h_def, false) + c_in; 		double w_nmos=g_tp.min_w_nmos_;
  //		double w_pmos=g_tp.min_w_nmos_*pmos_to_nmos_sizing_r;
  //		double i_on_n=1.0;
  //		double i_on_p=1.0;
  //		double i_on_n_in=1.0;
  //		double i_on_p_in=1;
  //		double vdd=g_tp.peri_global.Vdd;

  //		power.readOp.sc=shortcircuit_simple(vt, velocity_index, c_in, c_out, w_nmos,w_pmos, i_on_n, i_on_p,i_on_n_in, i_on_p_in,
  // vdd); 		power.readOp.dynamic=c_out*vdd*vdd/2;

  //		cout<<power.readOp.dynamic << "dynamic" <<endl;
  //		cout<<power.readOp.sc << "sc" << endl;

  //		power.readOp.sc=shortcircuit(vt, velocity_index, c_in, c_out, w_nmos,w_pmos, i_on_n, i_on_p,i_on_n_in, i_on_p_in, vdd);
  //		power.readOp.dynamic=c_out*vdd*vdd/2;
  //
  //		cout<<power.readOp.dynamic << "dynamic" <<endl;
  //		cout<<power.readOp.sc << "sc" << endl;
}
