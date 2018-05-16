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

#ifndef CORE_H_
#define CORE_H_

#include "XML_Parse.h"
#include "array.h"
#include "basic_components.h"
#include "interconnect.h"
#include "logic.h"
#include "parameter.h"

class BranchPredictor : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  CoreDynParam   coredynp;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;
  ArrayST *      globalBPT;
  ArrayST *      L1_localBPT;
  ArrayST *      L2_localBPT;
  ArrayST *      chooser;
  ArrayST *      RAS;

  bool exist;

  BranchPredictor(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~BranchPredictor();
};

class InstFetchU : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  CoreDynParam   coredynp;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;

  InstCache        icache;
  InstCache        icachecc;
  ArrayST *        IB;
  ArrayST *        BTB;
  BranchPredictor *BPT;

  void buildInstCache();
  void buildInstBuffer();
  void buildBranchPredictor();

  bool exist;

  InstFetchU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~InstFetchU();
};

class SchedulerU : public Component {
public:
  ParseXML *       XML;
  int              ithCore;
  InputParameter   interface_ip;
  CoreDynParam     coredynp;
  double           clockRate, executionTime;
  double           scktRatio, chip_PR_overhead, macro_PR_overhead;
  double           Iw_height, fp_Iw_height, ROB_height;
  ArrayST *        int_inst_window;
  ArrayST *        fp_inst_window;
  ArrayST *        ROB;
  selection_logic *instruction_selection;

  void buildInorderScheduler();
  void buildIntInstWindow();
  void buildFPInstWindow();
  void buildROB();
  void buildSelectionU();

  bool exist;

  SchedulerU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~SchedulerU();
};

class RENAMINGU : public Component {
public:
  ParseXML *                   XML;
  int                          ithCore;
  InputParameter               interface_ip;
  double                       clockRate, executionTime;
  CoreDynParam                 coredynp;
  ArrayST *                    iFRAT;
  ArrayST *                    fFRAT;
  ArrayST *                    iRRAT;
  ArrayST *                    fRRAT;
  ArrayST *                    ifreeL;
  ArrayST *                    ffreeL;
  dep_resource_conflict_check *idcl;
  dep_resource_conflict_check *fdcl;

  void buildIFRATRam();
  void buildFFRATRam();
  void buildIFRATCam();
  void buildFFRATCam();
  void buildIRRAT();
  void buildFRRAT();
  void buildIFreeL();
  void buildFFreeL();

  bool exist;

  RENAMINGU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~RENAMINGU();
};

class LoadStoreU : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  CoreDynParam   coredynp;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;
  double         lsq_height;
  DataCache      dcache;
  DataCache      dcachecc;
  ArrayST *      LSQ; // it is actually the store queue but for inorder processors it serves as both loadQ and StoreQ
  ArrayST *      LoadQ;
  ArrayST *      ssit;
  ArrayST *      lfst;
  DataCache      VPCfilter;
  ArrayST *      vpc_buffer;
  ArrayST *      vpc_wb;
  ArrayST *      vpc_specbw;
  ArrayST *      vpc_specbr;
  ArrayST *      strb;
  ArrayST *      ldrb;

  void buildDataCache();
  void buildVPCBuffer();
  void buildVPCSpecBuffers();
  void buildL2Filter();
  void buildLSQ();
  void buildLoadQ();
  void buildStoreSets();

  bool exist;

  LoadStoreU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~LoadStoreU();
};

class MemManU : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  CoreDynParam   coredynp;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;
  ArrayST *      itlb;
  ArrayST *      dtlb;

  void buildITLB();
  void buildDTLB();

  bool exist;

  MemManU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~MemManU();
};

class RegFU : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  CoreDynParam   coredynp;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;
  double         int_regfile_height, fp_regfile_height;
  ArrayST *      IRF;
  ArrayST *      FRF;
  ArrayST *      RFWIN;

  void buildIRF();
  void buildFRF();

  bool exist;

  RegFU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~RegFU();
};

class EXECU : public Component {
public:
  ParseXML *      XML;
  int             ithCore;
  InputParameter  interface_ip;
  double          clockRate, executionTime;
  double          scktRatio, chip_PR_overhead, macro_PR_overhead;
  double          lsq_height;
  CoreDynParam    coredynp;
  RegFU *         rfu;
  SchedulerU *    scheu;
  FunctionalUnit *fp_u;
  FunctionalUnit *exeu;
  interconnect *  int_bypass;
  interconnect *  intTagBypass;
  interconnect *  fp_bypass;
  interconnect *  fpTagBypass;

  Component bypass;

  bool exist;

  EXECU(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, double lsq_height_, const CoreDynParam &dyn_p_);
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~EXECU();
};

class Core : public Component {
public:
  ParseXML *     XML;
  int            ithCore;
  InputParameter interface_ip;
  double         clockRate, executionTime;
  double         scktRatio, chip_PR_overhead, macro_PR_overhead;

  InstFetchU *   ifu;
  LoadStoreU *   lsu;
  MemManU *      mmu;
  EXECU *        exu;
  RENAMINGU *    rnu;
  PipelinePower *corepipe;

  bool exist;

  UndiffCore * undiffCore;
  CoreDynParam coredynp;
  // full_decoder 	inst_decoder;
  // clock_network	clockNetwork;
  Core(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_);
  void set_core_param();
  void update_rtparam(ParseXML *XML_interface, int ithCore_, InputParameter *interface_ip_, Core *core); // eka
  void computeEnergy(bool is_tdp = true);
  void displayEnergy(uint32_t indent = 0, int plevel = 100, bool is_tdp = true);
  ~Core();
};

#endif /* CORE_H_ */
