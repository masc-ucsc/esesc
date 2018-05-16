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

#include "XML_Parse.h"
#include "SescConf.h"
#include "xmlParser.h"
#include <math.h>
#include <stdio.h>
#include <string>
//#include "arch_const.h"

extern bool gpu_tcd_present;
extern bool gpu_tci_present;

void ParseXML::parse(char *filepath) {
  unsigned int i, j, k, m, n;
  unsigned int NumofCom_4;
  unsigned int itmp;

  // this open and parse the XML file:
  XMLNode xMainNode = XMLNode::openFileHelper(filepath, "component"); // the 'component' in the first layer

  XMLNode xNode2 = xMainNode.getChildNode("component"); // the 'component' in the second layer
  // get all params in the second layer
  itmp = xNode2.nChildNode("param");
  for(i = 0; i < itmp; i++) {
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_cores") == 0) {
      sys.number_of_cores = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_GPU") == 0) {
      sys.number_of_GPU = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_L1Directories") == 0) {
      sys.number_of_L1Directories = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_L2Directories") == 0) {
      sys.number_of_L2Directories = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_L2s") == 0) {
      sys.number_of_L2s = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_STLBs") == 0) {
      sys.number_of_STLBs = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_L3s") == 0) {
      sys.number_of_L3s = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_NoCs") == 0) {
      sys.number_of_NoCs = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_of_dir_levels") == 0) {
      sys.number_of_dir_levels = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "domain_size") == 0) {
      sys.domain_size = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "first_level_dir") == 0) {
      sys.first_level_dir = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_cores") == 0) {
      sys.homogeneous_cores = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "core_tech_node") == 0) {
      sys.core_tech_node = atof(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "target_core_clockrate") == 0) {
      sys.target_core_clockrate = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "target_chip_area") == 0) {
      sys.target_chip_area = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "temperature") == 0) {
      sys.temperature = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "number_cache_levels") == 0) {
      sys.number_cache_levels = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "L1_property") == 0) {
      sys.L1_property = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "L2_property") == 0) {
      sys.L2_property = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_L2s") == 0) {
      sys.homogeneous_L2s = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_STLBs") == 0) {
      sys.homogeneous_STLBs = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_L1Directories") == 0) {
      sys.homogeneous_L1Directories = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_L2Directories") == 0) {
      sys.homogeneous_L2Directories = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "L3_property") == 0) {
      sys.L3_property = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_L3s") == 0) {
      sys.homogeneous_L3s = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_ccs") == 0) {
      sys.homogeneous_ccs = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "homogeneous_NoCs") == 0) {
      sys.homogeneous_NoCs = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "Max_area_deviation") == 0) {
      sys.Max_area_deviation = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "Max_power_deviation") == 0) {
      sys.Max_power_deviation = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "device_type") == 0) {
      sys.device_type = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "opt_dynamic_power") == 0) {
      sys.opt_dynamic_power = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "opt_lakage_power") == 0) {
      sys.opt_lakage_power = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "opt_clockrate") == 0) {
      sys.opt_clockrate = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "opt_area") == 0) {
      sys.opt_area = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "interconnect_projection_type") == 0) {
      sys.interconnect_projection_type = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "machine_bits") == 0) {
      sys.machine_bits = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "virtual_address_width") == 0) {
      sys.virtual_address_width = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "physical_address_width") == 0) {
      sys.physical_address_width = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "virtual_memory_page_size") == 0) {
      sys.virtual_memory_page_size = atoi(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "scale_dynamic") == 0) {
      sys.scale_dyn = atof(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
    if(strcmp(xNode2.getChildNode("param", i).getAttribute("name"), "scale_leakage") == 0) {
      sys.scale_lkg = atof(xNode2.getChildNode("param", i).getAttribute("value"));
      continue;
    }
  }

  itmp = xNode2.nChildNode("stat");
  for(i = 0; i < itmp; i++) {
    if(strcmp(xNode2.getChildNode("stat", i).getAttribute("name"), "total_cycles") == 0) {
      sys.total_cycles = atof(xNode2.getChildNode("stat", i).getAttribute("value"));
      continue;
    }
  }

  // get the number of components within the second layer
  unsigned int NumofCom_3 = xNode2.nChildNode("component");
  XMLNode      xNode3, xNode4; // define the third-layer(system.core0) and fourth-layer(system.core0.predictor) xnodes

  string strtmp;
  char   chtmp[60];
  char   chtmp1[60];
  chtmp1[0]                             = '\0';
  unsigned int OrderofComponents_3layer = 0;
  if(NumofCom_3 > OrderofComponents_3layer) {
    //___________________________get all system.core0-n________________________________________________
    if(sys.homogeneous_cores == 1)
      OrderofComponents_3layer = 0;
    else
      OrderofComponents_3layer = sys.number_of_cores - 1;
    for(i = 0; i <= OrderofComponents_3layer; i++) {
      xNode3 = xNode2.getChildNode("component", i);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_cores or number_of_cores is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "core") != NULL) {
          { // For cpu0-cpui
            // Get all params with system.core?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clock_rate") == 0) {
                sys.core[i].clock_rate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "machine_bits") == 0) {
                sys.core[i].machine_bits = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "virtual_address_width") == 0) {
                sys.core[i].virtual_address_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "physical_address_width") == 0) {
                sys.core[i].physical_address_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_length") == 0) {
                sys.core[i].instruction_length = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "opcode_width") == 0) {
                sys.core[i].opcode_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "machine_type") == 0) {
                sys.core[i].machine_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "internal_datapath_width") == 0) {
                sys.core[i].internal_datapath_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_hardware_threads") == 0) {
                sys.core[i].number_hardware_threads = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "fetch_width") == 0) {
                sys.core[i].fetch_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_instruction_fetch_ports") == 0) {
                sys.core[i].number_instruction_fetch_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "decode_width") == 0) {
                sys.core[i].decode_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "issue_width") == 0) {
                sys.core[i].issue_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "commit_width") == 0) {
                sys.core[i].commit_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "fp_issue_width") == 0) {
                sys.core[i].fp_issue_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "prediction_width") == 0) {
                sys.core[i].prediction_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "scoore") == 0) {
                sys.core[i].scoore = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }

              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "pipelines_per_core") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.core[i].pipelines_per_core[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.core[i].pipelines_per_core[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "pipeline_depth") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.core[i].pipeline_depth[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.core[i].pipeline_depth[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }

              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "FPU") == 0) {
                strcpy(sys.core[i].FPU, xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "divider_multiplier") == 0) {
                strcpy(sys.core[i].divider_multiplier, xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ALU_per_core") == 0) {
                sys.core[i].ALU_per_core = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "FPU_per_core") == 0) {
                sys.core[i].FPU_per_core = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_buffer_size") == 0) {
                sys.core[i].instruction_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "decoded_stream_buffer_size") == 0) {
                sys.core[i].decoded_stream_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_window_scheme") == 0) {
                sys.core[i].instruction_window_scheme = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_window_size") == 0) {
                sys.core[i].instruction_window_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "fp_instruction_window_size") == 0) {
                sys.core[i].fp_instruction_window_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ROB_size") == 0) {
                sys.core[i].ROB_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "archi_Regs_IRF_size") == 0) {
                sys.core[i].archi_Regs_IRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "archi_Regs_FRF_size") == 0) {
                sys.core[i].archi_Regs_FRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "phy_Regs_IRF_size") == 0) {
                sys.core[i].phy_Regs_IRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "phy_Regs_FRF_size") == 0) {
                sys.core[i].phy_Regs_FRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "rename_scheme") == 0) {
                sys.core[i].rename_scheme = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "register_windows_size") == 0) {
                sys.core[i].register_windows_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "LSU_order") == 0) {
                strcpy(sys.core[i].LSU_order, xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "store_buffer_size") == 0) {
                sys.core[i].store_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "load_buffer_size") == 0) {
                sys.core[i].load_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "memory_ports") == 0) {
                sys.core[i].memory_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "LSQ_ports") == 0) {
                sys.core[i].LSQ_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Dcache_dual_pump") == 0) {
                strcpy(sys.core[i].Dcache_dual_pump, xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "RAS_size") == 0) {
                sys.core[i].RAS_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ssit_size") == 0) {
                sys.core[i].ssit_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "lfst_size") == 0) {
                sys.core[i].lfst_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "lfst_width") == 0) {
                sys.core[i].lfst_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "scoore_store_retire_buffer_size") == 0) {
                sys.core[i].scoore_store_retire_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "scoore_load_retire_buffer_size") == 0) {
                sys.core[i].scoore_load_retire_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
            }
          }

          NumofCom_4 = xNode3.nChildNode("component"); // get the number of components within the third layer
          for(j = 0; j < NumofCom_4; j++) {
            xNode4 = xNode3.getChildNode("component", j);
            if(strcmp(xNode4.getAttribute("name"), "PBT") == 0) { // find PBT
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.predictor--PBT
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "prediction_width") == 0) {
                  sys.core[i].predictor.prediction_width = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "prediction_scheme") == 0) {
                  strcpy(sys.core[i].predictor.prediction_scheme, xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "predictor_size") == 0) {
                  sys.core[i].predictor.predictor_size = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "predictor_entries") == 0) {
                  sys.core[i].predictor.predictor_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "local_predictor_size") == 0) {
                  sys.core[i].predictor.local_predictor_size = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "local_predictor_entries") == 0) {
                  sys.core[i].predictor.local_predictor_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "global_predictor_entries") == 0) {
                  sys.core[i].predictor.global_predictor_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "global_predictor_bits") == 0) {
                  sys.core[i].predictor.global_predictor_bits = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "chooser_predictor_entries") == 0) {
                  sys.core[i].predictor.chooser_predictor_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "chooser_predictor_bits") == 0) {
                  sys.core[i].predictor.chooser_predictor_bits = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
              }
              itmp = xNode4.nChildNode("stat");
              for(k = 0; k < itmp; k++) { // get all items of stat in system.core0.predictor--PBT
                if(strcmp(xNode4.getChildNode("stat", k).getAttribute("name"), "predictor_accesses") == 0)
                  sys.core[i].predictor.predictor_accesses = atof(xNode4.getChildNode("stat", k).getAttribute("value"));
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "itlb") == 0) { // find system.core0.itlb
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.itlb--itlb
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_entries") == 0)
                  sys.core[i].itlb.number_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "icache") == 0) { // find system.core0.icache
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.icache--icache
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "icache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].icache.icache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].icache.icache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].icache.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].icache.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "dtlb") == 0) { // find system.core0.dtlb
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.dtlb--dtlb
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_entries") == 0)
                  sys.core[i].dtlb.number_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "VPCfilter") == 0) { // find system.core0.vpc_buffer
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) {
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "dcache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].VPCfilter.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].VPCfilter.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].VPCfilter.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].VPCfilter.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "vpc_buffer") == 0) { // find system.core0.vpc_buffer
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.vpc_buffer--vpc_buffer
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "dcache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].vpc_buffer.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].vpc_buffer.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].vpc_buffer.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].vpc_buffer.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "VPC") == 0) { // find system.core0.vpc
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.vpc--vpc
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "dcache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].dcache.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].dcache.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].dcache.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].dcache.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }

            if(strcmp(xNode4.getAttribute("name"), "dcache") == 0) { // find system.core0.dcache
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.dcache--dcache
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "dcache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].dcache.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].dcache.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].dcache.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].dcache.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "BTB") == 0) { // find system.core0.BTB
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.core0.BTB--BTB
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "BTB_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.core[i].BTB.BTB_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.core[i].BTB.BTB_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
          }
        } else {
          printf("The value of homogeneous_cores or number_of_cores is not correct!");
          exit(0);
        }
      }
    }

    //___________________________get all system.gpu0________________________________________________
    OrderofComponents_3layer++;
    {
      xNode3 = xNode2.getChildNode("component", OrderofComponents_3layer);
      if(xNode3.isEmpty() == 1) {
        printf("The value of number_of_GPU is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "gpu") != NULL) {
          { // For cpu0-cpui
            // Get all params with system.core?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_of_SMs") == 0) {
                sys.gpu.number_of_SMs = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_of_lanes") == 0) {
                sys.gpu.homoSM.number_of_lanes = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clock_rate") == 0) {
                sys.gpu.homoSM.homolane.clock_rate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "machine_bits") == 0) {
                sys.gpu.homoSM.homolane.machine_bits = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "virtual_address_width") == 0) {
                sys.gpu.homoSM.homolane.virtual_address_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "physical_address_width") == 0) {
                sys.gpu.homoSM.homolane.physical_address_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_length") == 0) {
                sys.gpu.homoSM.homolane.instruction_length = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "opcode_width") == 0) {
                sys.gpu.homoSM.homolane.opcode_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "internal_datapath_width") == 0) {
                sys.gpu.homoSM.homolane.internal_datapath_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_hardware_threads") == 0) {
                sys.gpu.homoSM.homolane.number_hardware_threads = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_instruction_fetch_ports") == 0) {
                sys.gpu.homoSM.homolane.number_instruction_fetch_ports =
                    atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }

              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "pipelines_per_core") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.gpu.homoSM.homolane.pipelines_per_core[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.gpu.homoSM.homolane.pipelines_per_core[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "pipeline_depth") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.gpu.homoSM.homolane.pipeline_depth[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.gpu.homoSM.homolane.pipeline_depth[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }

              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "instruction_buffer_size") == 0) {
                sys.gpu.homoSM.homolane.instruction_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "archi_Regs_IRF_size") == 0) {
                sys.gpu.homoSM.homolane.archi_Regs_IRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "phy_Regs_RFG_size") == 0) {
                sys.gpu.homoSM.homolane.phy_Regs_IRF_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "store_buffer_size") == 0) {
                sys.gpu.homoSM.homolane.store_buffer_size = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "memory_ports") == 0) {
                sys.gpu.homoSM.homolane.memory_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "LSQ_ports") == 0) {
                sys.gpu.homoSM.homolane.LSQ_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
            }
          }

          NumofCom_4 = xNode3.nChildNode("component"); // get the number of components within the third layer
          for(j = 0; j < NumofCom_4; j++) {
            xNode4 = xNode3.getChildNode("component", j);
            if(strcmp(xNode4.getAttribute("name"), "itlb") == 0) { // find system.gpu0.itlb
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.gpu0.itlb--itlb
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_entries") == 0)
                  sys.gpu.homoSM.itlb.number_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "icache") == 0) { // find system.gpu0.icache
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.gpu0.icache--icache
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "icache_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.gpu.homoSM.icache.icache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.gpu.homoSM.icache.icache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.gpu.homoSM.icache.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.gpu.homoSM.icache.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
            }
            if(strcmp(xNode4.getAttribute("name"), "dtlb") == 0) { // find system.gpu0.dtlb
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.gpu0.dtlb--dtlb
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_entries") == 0)
                  sys.gpu.homoSM.dtlb.number_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
              }
            }

            if(strcmp(xNode4.getAttribute("name"), "dcache") == 0) { // find system.gpu0.dcache
              itmp = xNode4.nChildNode("param");
              for(k = 0; k < itmp; k++) { // get all items of param in system.gpu0.dcache--dcache
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "dcacheg_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.gpu.homoSM.dcache.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.gpu.homoSM.dcache.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.gpu.homoSM.dcache.buffer_sizes[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.gpu.homoSM.dcache.buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "scratchpad_size") == 0) {
                  sys.gpu.homoSM.scratchpad.number_entries = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                  continue;
                }
                // tinycache_data
                if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "tinycache_data_config") == 0) {
                  strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                  m = 0;
                  for(n = 0; n < strtmp.length(); n++) {
                    if(strtmp[n] != ',') {
                      sprintf(chtmp, "%c", strtmp[n]);
                      strcat(chtmp1, chtmp);
                    } else {
                      sys.gpu.homoSM.homolane.tinycache_data.dcache_config[m] = atoi(chtmp1);
                      m++;
                      chtmp1[0] = '\0';
                    }
                  }
                  sys.gpu.homoSM.homolane.tinycache_data.dcache_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                  continue;
                }
              }
            }
            if(strstr(xNode4.getAttribute("name"), "L2") != NULL) {
              { // For L20
                itmp = xNode4.nChildNode("param");
                for(k = 0; k < itmp; k++) {
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "L2_config") == 0) {
                    strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                    m = 0;
                    for(n = 0; n < strtmp.length(); n++) {
                      if(strtmp[n] != ',') {
                        sprintf(chtmp, "%c", strtmp[n]);
                        strcat(chtmp1, chtmp);
                      } else {
                        sys.gpu.L2.L2_config[m] = atoi(chtmp1);
                        m++;
                        chtmp1[0] = '\0';
                      }
                    }
                    sys.gpu.L2.L2_config[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
                    sys.gpu.L2.clockrate = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
                    strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                    m = 0;
                    for(n = 0; n < strtmp.length(); n++) {
                      if(strtmp[n] != ',') {
                        sprintf(chtmp, "%c", strtmp[n]);
                        strcat(chtmp1, chtmp);
                      } else {
                        sys.gpu.L2.ports[m] = atoi(chtmp1);
                        m++;
                        chtmp1[0] = '\0';
                      }
                    }
                    sys.gpu.L2.ports[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
                    sys.gpu.L2.device_type = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "threeD_stack") == 0) {
                    strcpy(sys.gpu.L2.threeD_stack, (xNode4.getChildNode("param", k).getAttribute("value")));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                    strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                    m = 0;
                    for(n = 0; n < strtmp.length(); n++) {
                      if(strtmp[n] != ',') {
                        sprintf(chtmp, "%c", strtmp[n]);
                        strcat(chtmp1, chtmp);
                      } else {
                        sys.gpu.L2.buffer_sizes[m] = atoi(chtmp1);
                        m++;
                        chtmp1[0] = '\0';
                      }
                    }
                    sys.gpu.L2.buffer_sizes[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                    continue;
                  }
                }
              }
            }
          }
        } else {
          printf("The value of number_of_GPU is not correct!");
          exit(0);
        }
      }
    }

    //__________________________________________Get system.L1Directory0-n____________________________________________
    int w, tmpOrderofComponents_3layer;
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_L1Directories == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_L1Directories;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_L1Directories or number_of_L1Directories is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("id"), "L1Directory") != NULL) {
          itmp = xNode3.nChildNode("param");
          for(k = 0; k < itmp; k++) { // get all items of param in system.L1Directory
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Dir_config") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L1Directory[i].Dir_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L1Directory[i].Dir_config[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L1Directory[i].buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L1Directory[i].buffer_sizes[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }

            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
              sys.L1Directory[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L1Directory[i].ports[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L1Directory[i].ports[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
              sys.L1Directory[i].device_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Directory_type") == 0) {
              sys.L1Directory[i].Directory_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "3D_stack") == 0) {
              strcpy(sys.L1Directory[i].threeD_stack, xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
          }

          w = w + 1;
        } else {
          printf("The value of homogeneous_L1Directories or number_of_L1Directories is not correct!");
          exit(0);
        }
      }
    }

    //__________________________________________Get system.L2Directory0-n____________________________________________
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_L2Directories == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_L2Directories;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_L2Directories or number_of_L2Directories is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("id"), "L2Directory") != NULL) {
          itmp = xNode3.nChildNode("param");
          for(k = 0; k < itmp; k++) { // get all items of param in system.L2Directory
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Dir_config") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L2Directory[i].Dir_config[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L2Directory[i].Dir_config[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L2Directory[i].buffer_sizes[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L2Directory[i].buffer_sizes[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }

            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
              sys.L2Directory[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Directory_type") == 0) {
              sys.L2Directory[i].Directory_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
              strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
              m = 0;
              for(n = 0; n < strtmp.length(); n++) {
                if(strtmp[n] != ',') {
                  sprintf(chtmp, "%c", strtmp[n]);
                  strcat(chtmp1, chtmp);
                } else {
                  sys.L2Directory[i].ports[m] = atoi(chtmp1);
                  m++;
                  chtmp1[0] = '\0';
                }
              }
              sys.L2Directory[i].ports[m] = atoi(chtmp1);
              m++;
              chtmp1[0] = '\0';
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
              sys.L2Directory[i].device_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
            if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "3D_stack") == 0) {
              strcpy(sys.L2Directory[i].threeD_stack, xNode3.getChildNode("param", k).getAttribute("value"));
              continue;
            }
          }
          w = w + 1;
        } else {
          printf("The value of homogeneous_L2Directories or number_of_L2Directories is not correct!");
          exit(0);
        }
      }
    }

    //__________________________________________Get system.L2[0..n]____________________________________________
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_L2s == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_L2s;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_L2s or number_of_L2s is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "L2") != NULL) {
          { // For L20-L2i
            // Get all params with system.L2?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "L2_config") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L2[i].L2_config[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L2[i].L2_config[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
                sys.L2[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L2[i].ports[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L2[i].ports[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
                sys.L2[i].device_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "threeD_stack") == 0) {
                strcpy(sys.L2[i].threeD_stack, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L2[i].buffer_sizes[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L2[i].buffer_sizes[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
            }
          }
          w = w + 1;
        } else {
          printf("The value of homogeneous_L2s or number_of_L2s is not correct!");
          exit(0);
        }
      }
    }
    //__________________________________________Get system.STLB[0..n]____________________________________________
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_STLBs == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_STLBs;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_STLBs or number_of_STLBs is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "STLB") != NULL) {
          { // For STLB0-STLBi
            // Get all params with system.STLB?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "STLB_config") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.STLB[i].STLB_config[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.STLB[i].STLB_config[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
                sys.STLB[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.STLB[i].ports[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.STLB[i].ports[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
                sys.STLB[i].device_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "threeD_stack") == 0) {
                strcpy(sys.STLB[i].threeD_stack, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
            }
          }
          w = w + 1;
        } else {
          printf("The value of homogeneous_STLBs or number_of_STLBs is not correct!");
          exit(0);
        }
      }
    }
    //__________________________________________Get system.L3[0..n]____________________________________________
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_L3s == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_L3s;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_L3s or number_of_L3s is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "L3") != NULL) {
          { // For L30-L3i
            // Get all params with system.L3?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "L3_config") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L3[i].L3_config[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L3[i].L3_config[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
                sys.L3[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L3[i].ports[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L3[i].ports[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_type") == 0) {
                sys.L3[i].device_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "threeD_stack") == 0) {
                strcpy(sys.L3[i].threeD_stack, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "buffer_sizes") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.L3[i].buffer_sizes[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.L3[i].buffer_sizes[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
            }
          }
          w = w + 1;
        } else {
          printf("The value of homogeneous_L3s or number_of_L3s is not correct!");
          exit(0);
        }
      }
    }
    //__________________________________________Get system.NoC[0..n]____________________________________________
    w                           = OrderofComponents_3layer + 1;
    tmpOrderofComponents_3layer = OrderofComponents_3layer;
    if(sys.homogeneous_NoCs == 1)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    else
      OrderofComponents_3layer = OrderofComponents_3layer + sys.number_of_NoCs;

    for(i = 0; i < (OrderofComponents_3layer - tmpOrderofComponents_3layer); i++) {
      xNode3 = xNode2.getChildNode("component", w);
      if(xNode3.isEmpty() == 1) {
        printf("The value of homogeneous_NoCs or number_of_NoCs is not correct!");
        exit(0);
      } else {
        if(strstr(xNode3.getAttribute("name"), "noc") != NULL) {
          { // For NoC0-NoCi
            // Get all params with system.NoC?
            itmp = xNode3.nChildNode("param");
            for(k = 0; k < itmp; k++) {
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "clockrate") == 0) {
                sys.NoC[i].clockrate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "topology") == 0) {
                strcpy(sys.NoC[i].topology, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "horizontal_nodes") == 0) {
                sys.NoC[i].horizontal_nodes = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "vertical_nodes") == 0) {
                sys.NoC[i].vertical_nodes = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "has_global_link") == 0) {
                sys.NoC[i].has_global_link = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "link_throughput") == 0) {
                sys.NoC[i].link_throughput = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "link_latency") == 0) {
                sys.NoC[i].link_latency = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "input_ports") == 0) {
                sys.NoC[i].input_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "output_ports") == 0) {
                sys.NoC[i].output_ports = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "virtual_channel_per_port") == 0) {
                sys.NoC[i].virtual_channel_per_port = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "flit_bits") == 0) {
                sys.NoC[i].flit_bits = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "input_buffer_entries_per_vc") == 0) {
                sys.NoC[i].input_buffer_entries_per_vc = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "dual_pump") == 0) {
                sys.NoC[i].dual_pump = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "ports_of_input_buffer") == 0) {
                strtmp.assign(xNode3.getChildNode("param", k).getAttribute("value"));
                m = 0;
                for(n = 0; n < strtmp.length(); n++) {
                  if(strtmp[n] != ',') {
                    sprintf(chtmp, "%c", strtmp[n]);
                    strcat(chtmp1, chtmp);
                  } else {
                    sys.NoC[i].ports_of_input_buffer[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
                sys.NoC[i].ports_of_input_buffer[m] = atoi(chtmp1);
                m++;
                chtmp1[0] = '\0';
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_of_crossbars") == 0) {
                sys.NoC[i].number_of_crossbars = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "crossbar_type") == 0) {
                strcpy(sys.NoC[i].crossbar_type, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "crosspoint_type") == 0) {
                strcpy(sys.NoC[i].crosspoint_type, (xNode3.getChildNode("param", k).getAttribute("value")));
                continue;
              }
              if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "arbiter_type") == 0) {
                sys.NoC[i].arbiter_type = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
                continue;
              }
            }
            NumofCom_4 = xNode3.nChildNode("component"); // get the number of components within the third layer
            for(j = 0; j < NumofCom_4; j++) {
              xNode4 = xNode3.getChildNode("component", j);
              if(strcmp(xNode4.getAttribute("name"), "xbar0") == 0) { // find PBT
                itmp = xNode4.nChildNode("param");
                for(k = 0; k < itmp; k++) { // get all items of param in system.XoC0.xbar0--xbar0
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_of_inputs_of_crossbars") == 0) {
                    sys.NoC[i].xbar0.number_of_inputs_of_crossbars = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "number_of_outputs_of_crossbars") == 0) {
                    sys.NoC[i].xbar0.number_of_outputs_of_crossbars = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "flit_bits") == 0) {
                    sys.NoC[i].xbar0.flit_bits = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "input_buffer_entries_per_port") == 0) {
                    sys.NoC[i].xbar0.input_buffer_entries_per_port = atoi(xNode4.getChildNode("param", k).getAttribute("value"));
                    continue;
                  }
                  if(strcmp(xNode4.getChildNode("param", k).getAttribute("name"), "ports_of_input_buffer") == 0) {
                    strtmp.assign(xNode4.getChildNode("param", k).getAttribute("value"));
                    m = 0;
                    for(n = 0; n < strtmp.length(); n++) {
                      if(strtmp[n] != ',') {
                        sprintf(chtmp, "%c", strtmp[n]);
                        strcat(chtmp1, chtmp);
                      } else {
                        sys.NoC[i].xbar0.ports_of_input_buffer[m] = atoi(chtmp1);
                        m++;
                        chtmp1[0] = '\0';
                      }
                    }
                    sys.NoC[i].xbar0.ports_of_input_buffer[m] = atoi(chtmp1);
                    m++;
                    chtmp1[0] = '\0';
                  }
                }
              }
            }
          }
          w = w + 1;
        }
      }
    }
    //__________________________________________Get system.mem____________________________________________
    if(OrderofComponents_3layer > 0)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    xNode3 = xNode2.getChildNode("component", OrderofComponents_3layer);
    if(xNode3.isEmpty() == 1) {
      printf("some value(s) of number_of_cores/number_of_L2s/number_of_STLBs/number_of_L3s/number_of_NoCs is/are not correct!");
      exit(0);
    }
    if(strstr(xNode3.getAttribute("id"), "system.mem") != NULL) {

      itmp = xNode3.nChildNode("param");
      for(k = 0; k < itmp; k++) { // get all items of param in system.mem
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "mem_tech_node") == 0) {
          sys.mem.mem_tech_node = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "device_clock") == 0) {
          sys.mem.device_clock = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "peak_transfer_rate") == 0) {
          sys.mem.peak_transfer_rate = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "capacity_per_channel") == 0) {
          sys.mem.capacity_per_channel = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_ranks") == 0) {
          sys.mem.number_ranks = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "num_banks_of_DRAM_chip") == 0) {
          sys.mem.num_banks_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "Block_width_of_DRAM_chip") == 0) {
          sys.mem.Block_width_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "output_width_of_DRAM_chip") == 0) {
          sys.mem.output_width_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "page_size_of_DRAM_chip") == 0) {
          sys.mem.page_size_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "burstlength_of_DRAM_chip") == 0) {
          sys.mem.burstlength_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "internal_prefetch_of_DRAM_chip") == 0) {
          sys.mem.internal_prefetch_of_DRAM_chip = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
      }

    } else {
      printf("some value(s) of number_of_cores/number_of_L2s/number_of_STLBs/number_of_L3s/number_of_NoCs is/are not correct!");
      exit(0);
    }
    //__________________________________________Get system.mc____________________________________________
    if(OrderofComponents_3layer > 0)
      OrderofComponents_3layer = OrderofComponents_3layer + 1;
    xNode3 = xNode2.getChildNode("component", OrderofComponents_3layer);
    if(xNode3.isEmpty() == 1) {
      printf("some value(s) of number_of_cores/number_of_L2s/number_of_STLBs/number_of_L3s/number_of_NoCs is/are not correct!");
      exit(0);
    }
    if(strstr(xNode3.getAttribute("id"), "system.mc") != NULL) {
      itmp = xNode3.nChildNode("param");
      for(k = 0; k < itmp; k++) { // get all items of param in system.mem
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "mc_clock") == 0) {
          sys.mc.mc_clock = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "llc_line_length") == 0) {
          sys.mc.llc_line_length = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "number_mcs") == 0) {
          sys.mc.number_mcs = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "memory_channels_per_mc") == 0) {
          sys.mc.memory_channels_per_mc = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "req_window_size_per_channel") == 0) {
          sys.mc.req_window_size_per_channel = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "IO_buffer_size_per_channel") == 0) {
          sys.mc.IO_buffer_size_per_channel = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "databus_width") == 0) {
          sys.mc.databus_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
        if(strcmp(xNode3.getChildNode("param", k).getAttribute("name"), "addressbus_width") == 0) {
          sys.mc.addressbus_width = atoi(xNode3.getChildNode("param", k).getAttribute("value"));
          continue;
        }
      }

    } else {
      printf("some value(s) of number_of_cores/number_of_L2s/number_of_STLBs/number_of_L3s/number_of_NoCs is/are not correct!");
      exit(0);
    }
  }
}

void ParseXML::initialize(vector<uint32_t> *stats_vector, map<string, int> mcpat_map, vector<uint32_t> *cidx,
                          vector<uint32_t> *gidx) {

  coreIndex   = cidx;
  gpuIndex    = gidx;
  stats_vec   = stats_vector;
  str2pos_map = mcpat_map;

  // All number_of_* at the level of 'system' 03/21/2009
  sys.executionTime           = 1;
  sys.number_of_cores         = 1;
  sys.number_of_L1Directories = 0;
  sys.number_of_L2Directories = 0;
  sys.number_of_L2s           = 1;
  sys.number_of_STLBs         = 1;
  sys.number_of_L3s           = 1;
  sys.number_of_NoCs          = 0;
  // All params at the level of 'system'
  // strcpy(sys.homogeneous_cores,"default");
  sys.core_tech_node            = 1;
  sys.target_core_clockrate     = 1;
  sys.target_chip_area          = 1;
  sys.temperature               = 1;
  sys.number_cache_levels       = 1;
  sys.homogeneous_cores         = 1;
  sys.homogeneous_L1Directories = 1;
  sys.homogeneous_L2Directories = 1;
  sys.homogeneous_L2s           = 1;
  sys.homogeneous_STLBs         = 1;
  sys.homogeneous_L3s           = 1;
  sys.homogeneous_NoCs          = 1;
  sys.homogeneous_ccs           = 1;

  sys.Max_area_deviation           = 1;
  sys.Max_power_deviation          = 1;
  sys.device_type                  = 1;
  sys.opt_dynamic_power            = 1;
  sys.opt_lakage_power             = 1;
  sys.opt_clockrate                = 1;
  sys.opt_area                     = 1;
  sys.interconnect_projection_type = 1;
  int i, j;
  for(i = 0; i < MAX_PROC_NUM; i++) {
    sys.core[i].clock_rate             = 1;
    sys.core[i].machine_bits           = 1;
    sys.core[i].virtual_address_width  = 1;
    sys.core[i].physical_address_width = 1;
    sys.core[i].opcode_width           = 1;
    // strcpy(sys.core[i].machine_type,"default");
    sys.core[i].scoore                         = 0;
    sys.core[i].internal_datapath_width        = 1;
    sys.core[i].number_hardware_threads        = 1;
    sys.core[i].fetch_width                    = 1;
    sys.core[i].number_instruction_fetch_ports = 1;
    sys.core[i].decode_width                   = 1;
    sys.core[i].issue_width                    = 1;
    sys.core[i].commit_width                   = 1;
    for(j = 0; j < 20; j++)
      sys.core[i].pipelines_per_core[j] = 1;
    for(j = 0; j < 20; j++)
      sys.core[i].pipeline_depth[j] = 1;
    strcpy(sys.core[i].FPU, "default");
    strcpy(sys.core[i].divider_multiplier, "default");
    sys.core[i].ALU_per_core               = 1;
    sys.core[i].FPU_per_core               = 1;
    sys.core[i].instruction_buffer_size    = 1;
    sys.core[i].decoded_stream_buffer_size = 1;
    // strcpy(sys.core[i].instruction_window_scheme,"default");
    sys.core[i].instruction_window_size = 1;
    sys.core[i].ROB_size                = 1;
    sys.core[i].archi_Regs_IRF_size     = 1;
    sys.core[i].archi_Regs_FRF_size     = 1;
    sys.core[i].phy_Regs_IRF_size       = 1;
    sys.core[i].phy_Regs_FRF_size       = 1;
    // strcpy(sys.core[i].rename_scheme,"default");
    sys.core[i].register_windows_size = 1;
    strcpy(sys.core[i].LSU_order, "default");
    sys.core[i].store_buffer_size = 1;
    sys.core[i].load_buffer_size  = 1;
    sys.core[i].memory_ports      = 1;
    sys.core[i].LSQ_ports         = 1;
    strcpy(sys.core[i].Dcache_dual_pump, "default");
    sys.core[i].RAS_size                        = 1;
    sys.core[i].ssit_size                       = 8192;
    sys.core[i].lfst_size                       = 512;
    sys.core[i].lfst_width                      = 32;
    sys.core[i].scoore_store_retire_buffer_size = 32;
    sys.core[i].scoore_load_retire_buffer_size  = 32;
    // all stats at the level of system.core(0-n)
    sys.core[i].total_instructions          = 1;
    sys.core[i].int_instructions            = 1;
    sys.core[i].fp_instructions             = 1;
    sys.core[i].branch_instructions         = 1;
    sys.core[i].branch_mispredictions       = 1;
    sys.core[i].committed_instructions      = 1;
    sys.core[i].load_instructions           = 1;
    sys.core[i].store_instructions          = 1;
    sys.core[i].total_cycles                = 1;
    sys.core[i].idle_cycles                 = 1;
    sys.core[i].busy_cycles                 = 1;
    sys.core[i].instruction_buffer_reads    = 1;
    sys.core[i].instruction_buffer_write    = 1;
    sys.core[i].ROB_reads                   = 1;
    sys.core[i].ROB_writes                  = 1;
    sys.core[i].rename_accesses             = 1;
    sys.core[i].inst_window_reads           = 1;
    sys.core[i].inst_window_writes          = 1;
    sys.core[i].inst_window_wakeup_accesses = 1;
    sys.core[i].inst_window_selections      = 1;
    sys.core[i].archi_int_regfile_reads     = 1;
    sys.core[i].archi_float_regfile_reads   = 1;
    sys.core[i].phy_int_regfile_reads       = 1;
    sys.core[i].phy_float_regfile_reads     = 1;
    sys.core[i].windowed_reg_accesses       = 1;
    sys.core[i].windowed_reg_transports     = 1;
    sys.core[i].function_calls              = 1;
    sys.core[i].ialu_access                 = 1;
    sys.core[i].fpu_access                  = 1;
    sys.core[i].bypassbus_access            = 1;
    sys.core[i].load_buffer_reads           = 1;
    sys.core[i].load_buffer_writes          = 1;
    sys.core[i].load_buffer_cams            = 1;
    sys.core[i].store_buffer_reads          = 1;
    sys.core[i].store_buffer_writes         = 1;
    sys.core[i].store_buffer_cams           = 1;
    sys.core[i].store_buffer_forwards       = 1;
    sys.core[i].main_memory_access          = 1;
    sys.core[i].main_memory_read            = 1;
    sys.core[i].main_memory_write           = 1;
    // system.core?.predictor
    sys.core[i].predictor.prediction_width = 1;
    strcpy(sys.core[i].predictor.prediction_scheme, "default");
    sys.core[i].predictor.predictor_size            = 1;
    sys.core[i].predictor.predictor_entries         = 1;
    sys.core[i].predictor.local_predictor_entries   = 1;
    sys.core[i].predictor.local_predictor_size      = 1;
    sys.core[i].predictor.global_predictor_entries  = 1;
    sys.core[i].predictor.global_predictor_bits     = 1;
    sys.core[i].predictor.chooser_predictor_entries = 1;
    sys.core[i].predictor.chooser_predictor_bits    = 1;
    sys.core[i].predictor.predictor_accesses        = 1;
    // system.core?.itlb
    sys.core[i].itlb.number_entries = 1;
    sys.core[i].itlb.total_hits     = 0;
    sys.core[i].itlb.total_accesses = 0;
    sys.core[i].itlb.total_misses   = 0;
    // system.core?.icache
    for(j = 0; j < 20; j++)
      sys.core[i].icache.icache_config[j] = 1;
    // strcpy(sys.core[i].icache.buffer_sizes,"default");
    sys.core[i].icache.total_accesses           = 0;
    sys.core[i].icache.read_accesses            = 0;
    sys.core[i].icache.read_misses              = 0;
    sys.core[i].icache.replacements             = 0;
    sys.core[i].icache.read_hits                = 0;
    sys.core[i].icache.total_hits               = 0;
    sys.core[i].icache.total_misses             = 0;
    sys.core[i].icache.miss_buffer_access       = 0;
    sys.core[i].icache.fill_buffer_accesses     = 0;
    sys.core[i].icache.prefetch_buffer_accesses = 0;
    sys.core[i].icache.prefetch_buffer_writes   = 0;
    sys.core[i].icache.prefetch_buffer_reads    = 0;
    sys.core[i].icache.prefetch_buffer_hits     = 0;
    // system.core?.dtlb
    sys.core[i].dtlb.number_entries = 1;
    sys.core[i].dtlb.total_accesses = 0;
    sys.core[i].dtlb.read_accesses  = 0;
    sys.core[i].dtlb.write_accesses = 0;
    sys.core[i].dtlb.write_hits     = 0;
    sys.core[i].dtlb.read_hits      = 0;
    sys.core[i].dtlb.read_misses    = 0;
    sys.core[i].dtlb.write_misses   = 0;
    sys.core[i].dtlb.total_hits     = 0;
    sys.core[i].dtlb.total_misses   = 0;
    // system.core?.dcache
    for(j = 0; j < 20; j++)
      sys.core[i].dcache.dcache_config[j] = 1;
    // strcpy(sys.core[i].dcache.buffer_sizes,"default");
    sys.core[i].dcache.total_accesses           = 0;
    sys.core[i].dcache.read_accesses            = 0;
    sys.core[i].dcache.write_accesses           = 0;
    sys.core[i].dcache.total_hits               = 0;
    sys.core[i].dcache.total_misses             = 0;
    sys.core[i].dcache.read_hits                = 0;
    sys.core[i].dcache.write_hits               = 0;
    sys.core[i].dcache.read_misses              = 0;
    sys.core[i].dcache.write_misses             = 0;
    sys.core[i].dcache.replacements             = 0;
    sys.core[i].dcache.write_backs              = 0;
    sys.core[i].dcache.miss_buffer_access       = 0;
    sys.core[i].dcache.fill_buffer_accesses     = 0;
    sys.core[i].dcache.prefetch_buffer_accesses = 0;
    sys.core[i].dcache.prefetch_buffer_writes   = 0;
    sys.core[i].dcache.prefetch_buffer_reads    = 0;
    sys.core[i].dcache.prefetch_buffer_hits     = 0;
    sys.core[i].dcache.wbb_writes               = 0;
    sys.core[i].dcache.wbb_reads                = 0;
    // system.core?.BTB
    for(j = 0; j < 20; j++)
      sys.core[i].BTB.BTB_config[j] = 1;
    sys.core[i].BTB.total_accesses = 1;
    sys.core[i].BTB.read_accesses  = 1;
    sys.core[i].BTB.write_accesses = 1;
    sys.core[i].BTB.total_hits     = 1;
    sys.core[i].BTB.total_misses   = 1;
    sys.core[i].BTB.read_hits      = 1;
    sys.core[i].BTB.write_hits     = 1;
    sys.core[i].BTB.read_misses    = 1;
    sys.core[i].BTB.write_misses   = 1;
    sys.core[i].BTB.replacements   = 1;
  }

  // system_L1directory
  for(i = 0; i < MAX_PROC_NUM; i++) {
    for(j = 0; j < 20; j++)
      sys.L1Directory[i].Dir_config[j] = 1;
    for(j = 0; j < 20; j++)
      sys.L1Directory[i].buffer_sizes[j] = 1;
    sys.L1Directory[i].clockrate   = 1;
    sys.L1Directory[i].ports[19]   = 1;
    sys.L1Directory[i].device_type = 1;
    strcpy(sys.L1Directory[i].threeD_stack, "default");
    sys.L1Directory[i].total_accesses = 1;
    sys.L1Directory[i].read_accesses  = 1;
    sys.L1Directory[i].write_accesses = 1;
  }
  // system_L2directory
  for(i = 0; i < MAX_PROC_NUM; i++) {
    for(j = 0; j < 20; j++)
      sys.L2Directory[i].Dir_config[j] = 1;
    for(j = 0; j < 20; j++)
      sys.L2Directory[i].buffer_sizes[j] = 1;
    sys.L2Directory[i].clockrate   = 1;
    sys.L2Directory[i].ports[19]   = 1;
    sys.L2Directory[i].device_type = 1;
    strcpy(sys.L2Directory[i].threeD_stack, "default");
    sys.L2Directory[i].total_accesses = 1;
    sys.L2Directory[i].read_accesses  = 1;
    sys.L2Directory[i].write_accesses = 1;
  }
  for(i = 0; i < MAX_PROC_NUM; i++) {
    // system_L2
    for(j = 0; j < 20; j++)
      sys.L2[i].L2_config[j] = 1;
    sys.L2[i].clockrate = 1;
    for(j = 0; j < 20; j++)
      sys.L2[i].ports[j] = 1;
    sys.L2[i].device_type = 1;
    strcpy(sys.L2[i].threeD_stack, "default");
    for(j = 0; j < 20; j++)
      sys.L2[i].buffer_sizes[j] = 1;
    sys.L2[i].total_accesses           = 1;
    sys.L2[i].read_accesses            = 1;
    sys.L2[i].write_accesses           = 1;
    sys.L2[i].total_hits               = 1;
    sys.L2[i].total_misses             = 1;
    sys.L2[i].read_hits                = 1;
    sys.L2[i].write_hits               = 1;
    sys.L2[i].read_misses              = 1;
    sys.L2[i].write_misses             = 1;
    sys.L2[i].replacements             = 1;
    sys.L2[i].write_backs              = 1;
    sys.L2[i].miss_buffer_accesses     = 1;
    sys.L2[i].fill_buffer_accesses     = 1;
    sys.L2[i].prefetch_buffer_accesses = 1;
    sys.L2[i].prefetch_buffer_writes   = 1;
    sys.L2[i].prefetch_buffer_reads    = 1;
    sys.L2[i].prefetch_buffer_hits     = 1;
    sys.L2[i].wbb_writes               = 1;
    sys.L2[i].wbb_reads                = 1;
  }
  for(i = 0; i < MAX_PROC_NUM; i++) {
    // system_STLB
    for(j = 0; j < 20; j++)
      sys.STLB[i].STLB_config[j] = 1;
    sys.STLB[i].clockrate = 1;
    for(j = 0; j < 20; j++)
      sys.STLB[i].ports[j] = 1;
    sys.STLB[i].device_type = 1;
    strcpy(sys.STLB[i].threeD_stack, "default");
    for(j = 0; j < 20; j++)
      sys.STLB[i].buffer_sizes[j] = 1;
    sys.STLB[i].total_accesses           = 1;
    sys.STLB[i].read_accesses            = 1;
    sys.STLB[i].write_accesses           = 1;
    sys.STLB[i].total_hits               = 1;
    sys.STLB[i].total_misses             = 1;
    sys.STLB[i].read_hits                = 1;
    sys.STLB[i].write_hits               = 1;
    sys.STLB[i].read_misses              = 1;
    sys.STLB[i].write_misses             = 1;
    sys.STLB[i].replacements             = 1;
    sys.STLB[i].write_backs              = 1;
    sys.STLB[i].miss_buffer_accesses     = 1;
    sys.STLB[i].fill_buffer_accesses     = 1;
    sys.STLB[i].prefetch_buffer_accesses = 1;
    sys.STLB[i].prefetch_buffer_writes   = 1;
    sys.STLB[i].prefetch_buffer_reads    = 1;
    sys.STLB[i].prefetch_buffer_hits     = 1;
    sys.STLB[i].wbb_writes               = 1;
    sys.STLB[i].wbb_reads                = 1;
  }
  for(i = 0; i < MAX_PROC_NUM; i++) {
    // system_L3
    for(j = 0; j < 20; j++)
      sys.L3[i].L3_config[j] = 1;
    sys.L3[i].clockrate = 1;
    for(j = 0; j < 20; j++)
      sys.L3[i].ports[j] = 1;
    sys.L3[i].device_type = 1;
    strcpy(sys.L3[i].threeD_stack, "default");
    for(j = 0; j < 20; j++)
      sys.L3[i].buffer_sizes[j] = 1;
    sys.L3[i].total_accesses           = 1;
    sys.L3[i].read_accesses            = 1;
    sys.L3[i].write_accesses           = 1;
    sys.L3[i].total_hits               = 1;
    sys.L3[i].total_misses             = 1;
    sys.L3[i].read_hits                = 1;
    sys.L3[i].write_hits               = 1;
    sys.L3[i].read_misses              = 1;
    sys.L3[i].write_misses             = 1;
    sys.L3[i].replacements             = 1;
    sys.L3[i].write_backs              = 1;
    sys.L3[i].miss_buffer_accesses     = 1;
    sys.L3[i].fill_buffer_accesses     = 1;
    sys.L3[i].prefetch_buffer_accesses = 1;
    sys.L3[i].prefetch_buffer_writes   = 1;
    sys.L3[i].prefetch_buffer_reads    = 1;
    sys.L3[i].prefetch_buffer_hits     = 1;
    sys.L3[i].wbb_writes               = 1;
    sys.L3[i].wbb_reads                = 1;
  }
  // system_NoC
  for(i = 0; i < MAX_PROC_NUM; i++) {
    sys.NoC[i].clockrate = 1;
    strcpy(sys.NoC[i].topology, "default");
    sys.NoC[i].horizontal_nodes            = 1;
    sys.NoC[i].vertical_nodes              = 1;
    sys.NoC[i].input_ports                 = 1;
    sys.NoC[i].output_ports                = 1;
    sys.NoC[i].virtual_channel_per_port    = 1;
    sys.NoC[i].flit_bits                   = 1;
    sys.NoC[i].input_buffer_entries_per_vc = 1;
    for(j = 0; j < 20; j++)
      sys.NoC[i].ports_of_input_buffer[j] = 1;
    sys.NoC[i].number_of_crossbars = 1;
    strcpy(sys.NoC[i].crossbar_type, "default");
    strcpy(sys.NoC[i].crosspoint_type, "default");
    // system.NoC?.xbar0;
    sys.NoC[i].xbar0.number_of_inputs_of_crossbars  = 1;
    sys.NoC[i].xbar0.number_of_outputs_of_crossbars = 1;
    sys.NoC[i].xbar0.flit_bits                      = 1;
    sys.NoC[i].xbar0.input_buffer_entries_per_port  = 1;
    sys.NoC[i].xbar0.ports_of_input_buffer[19]      = 1;
    sys.NoC[i].xbar0.crossbar_accesses              = 1;
  }
  // system_mem
  sys.mem.mem_tech_node                  = 1;
  sys.mem.device_clock                   = 1;
  sys.mem.capacity_per_channel           = 1;
  sys.mem.number_ranks                   = 1;
  sys.mem.num_banks_of_DRAM_chip         = 1;
  sys.mem.Block_width_of_DRAM_chip       = 1;
  sys.mem.output_width_of_DRAM_chip      = 1;
  sys.mem.page_size_of_DRAM_chip         = 1;
  sys.mem.burstlength_of_DRAM_chip       = 1;
  sys.mem.internal_prefetch_of_DRAM_chip = 1;
  sys.mem.memory_accesses                = 1;
  sys.mem.memory_reads                   = 1;
  sys.mem.memory_writes                  = 1;
  // system_mc
  sys.mc.number_mcs                  = 1;
  sys.mc.memory_channels_per_mc      = 1;
  sys.mc.req_window_size_per_channel = 1;
  sys.mc.IO_buffer_size_per_channel  = 1;
  sys.mc.databus_width               = 1;
  sys.mc.addressbus_width            = 1;
  sys.mc.memory_accesses             = 1;
  sys.mc.memory_reads                = 1;
  sys.mc.memory_writes               = 1;
}

int ParseXML::cntr_pos_value(string cntr_name) {

  // find position of the cntr
  it1 = str2pos_map.find(cntr_name);
  if(it1 == str2pos_map.end()) {
    cout << "Did not find counter " << cntr_name << endl;
    exit(-1);
  }

  return stats_vec->at(it1->second);
}

bool ParseXML::check_cntr(string cntr_name) {

  // find position of the cntr
  it1 = str2pos_map.find(cntr_name);
  if(it1 == str2pos_map.end()) {
    return false;
  }

  return true;
}

void ParseXML::updateCntrValues(vector<uint32_t> *stats_vector, map<string, int> mcpat_map) {
  char str[250];
  //  sys->executionTime = double (timeinterval/1e9);
  for(uint32_t ii = 0; ii < sys.number_of_cores; ii++) {
    uint32_t i = (*coreIndex)[ii];
    sprintf(str, "P(%u)_core_total_instructions", i);
    if(check_cntr(str))
      sys.core[i].total_instructions = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_int_instructions", i);
    if(check_cntr(str))
      sys.core[i].int_instructions = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_fp_instructions", i);
    if(check_cntr(str))
      sys.core[i].fp_instructions = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_branch_instructions", i);
    if(check_cntr(str))
      sys.core[i].branch_instructions = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_branch_mispredictions", i);
    if(check_cntr(str))
      sys.core[i].branch_mispredictions = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_committed_instructions", i);
    // sys.core[i].committed_instructions = cntr_pos_value(str); // not used?
    // sprintf(str,"P(%u)_core_committed_int_instructions",i);
    // sys.core[i].committed_int_instructions= cntr_pos_value(str); // not used?
    // sprintf(str,"P(%u)_core_committed_fp_instructions",i);
    // sys.core[i].committed_fp_instructions=cntr_pos_value(str); // not used?
    // sprintf(str,"P(%u)_core_load_instructions",i);
    // sys.core[i].load_instructions= cntr_pos_value(str); // not used?
    // sprintf(str,"P(%u)_core_store_instructions",i);
    // sys.core[i].store_instructions= cntr_pos_value(str); // not used?
    sprintf(str, "P(%u)_core_total_cycles", i);
    if(check_cntr(str))
      sys.core[i].total_cycles = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_ROB_reads", i);
    // sys.core[i].ROB_reads = cntr_pos_value(str)/2; // double width, half the port size and access
    if(check_cntr(str))
      sys.core[i].ROB_reads = (sys.core[i].int_instructions + sys.core[i].fp_instructions + sys.core[i].branch_instructions) / 2;
    sprintf(str, "P(%u)_core_ROB_writes", i);
    if(check_cntr(str))
      sys.core[i].ROB_writes = cntr_pos_value(str) / 2;
    sprintf(str, "P(%u)_core_int_regfile_reads", i);
    if(check_cntr(str))
      sys.core[i].int_regfile_reads = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_float_regfile_reads", i);
    if(check_cntr(str))
      sys.core[i].float_regfile_reads = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_int_regfile_writes", i);
    if(check_cntr(str))
      sys.core[i].int_regfile_writes = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_float_regfile_writes", i);
    if(check_cntr(str))
      sys.core[i].float_regfile_writes = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_function_calls", i);
    if(check_cntr(str))
      sys.core[i].function_calls = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_bypassbus_access", i);

    if(check_cntr(str))
      sys.core[i].bypassbus_access = cntr_pos_value(str);
    if(sys.core[i].scoore) {
      sprintf(str, "P(%u)_core_vpc_buffer_hit", i);
      if(check_cntr(str))
        sys.core[i].vpc_buffer.read_hits = cntr_pos_value(str);
      sprintf(str, "P(%u)_core_vpc_buffer_miss", i);
      if(check_cntr(str))
        sys.core[i].vpc_buffer.read_misses = cntr_pos_value(str);
    }
    sprintf(str, "IL1(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].icache.read_accesses = cntr_pos_value(str);
    sprintf(str, "IL1(%u)_read_misses", i);
    if(check_cntr(str))
      sys.core[i].icache.read_misses = cntr_pos_value(str);
#if 0
    sprintf(str,"%u)_itlb_total_accesses",i);
    sys.core[i].itlb.total_accesses=cntr_pos_value(str);
    sprintf(str,"P(%u)_itlb_total_misses",i);
    sys.core[i].itlb.total_misses=cntr_pos_value(str);
#else
    sys.core[i].itlb.total_accesses = sys.core[i].icache.read_accesses;
    sys.core[i].itlb.total_misses   = 0;
#endif
    // if (sys.core[i].scoore) {
    //   sprintf(str,"VPC(%u)_read_accesses",i);
    //   sys.core[i].dcache.read_accesses = cntr_pos_value(str);
    //   sprintf(str,"VPC(%u)_write_accesses",i);
    //   sys.core[i].dcache.write_accesses = cntr_pos_value(str);
    //   sprintf(str,"VPC(%u)_read_misses",i);
    //   sys.core[i].dcache.read_misses= cntr_pos_value(str);
    //   sprintf(str,"VPC(%u)_write_misses",i);
    //   sys.core[i].dcache.write_misses= cntr_pos_value(str);
    //   sys.core[i].vpc_buffer.read_accesses = sys.core[i].dcache.read_accesses;
    //   sys.core[i].vpc_buffer.write_accesses = sys.core[i].dcache.write_accesses;
    //   sys.core[i].vpc_buffer.read_misses = sys.core[i].dcache.read_misses;
    //   sys.core[i].vpc_buffer.write_misses = sys.core[i].dcache.write_misses;
    // } else {
    sprintf(str, "DL1(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].dcache.read_accesses = cntr_pos_value(str);
    sprintf(str, "DL1(%u)_write_accesses", i);
    if(check_cntr(str))
      sys.core[i].dcache.write_accesses = cntr_pos_value(str);
    sprintf(str, "DL1(%u)_read_misses", i);
    if(check_cntr(str))
      sys.core[i].dcache.read_misses = cntr_pos_value(str);
    sprintf(str, "DL1(%u)_write_misses", i);
    if(check_cntr(str))
      sys.core[i].dcache.write_misses = cntr_pos_value(str);
      //=    }
#if 1
    int tmp = 0;
    sprintf(str, "PTLB(%u)_read_hits", i);
    if(check_cntr(str))
      tmp = cntr_pos_value(str);
    sprintf(str, "PTLB(%u)_read_misses", i);
    if(check_cntr(str))
      tmp += cntr_pos_value(str);
    if(check_cntr(str))
      sys.core[i].dtlb.total_accesses = tmp;
    sprintf(str, "PTLB(%u)_read_misses", i);
    if(check_cntr(str))
      sys.core[i].dtlb.total_misses = cntr_pos_value(str);
#else
    sprintf(str, "DL1(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].dtlb.total_accesses = sys.core[i].dcache.read_accesses;
#endif
    // sprintf(str,"P(%u)_BTB_read_accesses",i);
    // sys.core[i].BTB.read_accesses = cntr_pos_value(str);
    // sprintf(str,"P(%u)_BTB_write_accesses",i);
    // sys.core[i].BTB.write_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_loadq_write_accesses", i);
    if(check_cntr(str))
      sys.core[i].LoadQ.write_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_loadq_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].LoadQ.read_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_storq_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].LSQ.read_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_storq_write_accesses", i);
    if(check_cntr(str))
      sys.core[i].LSQ.write_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_ssit_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].ssit.read_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_ssit_write_accesses", i);
    if(check_cntr(str))
      sys.core[i].ssit.write_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_lfst_read_accesses", i);
    if(check_cntr(str))
      sys.core[i].lfst.read_accesses = cntr_pos_value(str);
    sprintf(str, "P(%u)_core_lfst_write_accesses", i);
    if(check_cntr(str))
      sys.core[i].lfst.write_accesses = cntr_pos_value(str);
    if(sys.core[i].scoore) {
      sprintf(str, "P(%u)_core_strb_read_accesses", i);
      if(check_cntr(str))
        sys.core[i].strb.read_accesses = cntr_pos_value(str);
      sprintf(str, "P(%u)_core_strb_write_accesses", i);
      if(check_cntr(str))
        sys.core[i].strb.write_accesses = cntr_pos_value(str);
      sprintf(str, "P(%u)_core_ldrb_read_accesses", i);
      if(check_cntr(str))
        sys.core[i].ldrb.read_accesses = cntr_pos_value(str);
      sprintf(str, "P(%u)_core_ldrb_write_accesses", i);
      if(check_cntr(str))
        sys.core[i].ldrb.write_accesses = cntr_pos_value(str);
    }
  }

  if(sys.number_of_GPU) {
    int nl = sys.gpu.homoSM.number_of_lanes;
    for(int ii = 0; ii < sys.gpu.number_of_SMs; ii++) {
      int i = (*gpuIndex)[ii];

      // Counters per lane (lane === PE)
      for(int j = 0; j < sys.gpu.homoSM.number_of_lanes; j++) {
        sprintf(str, "P(%u)_core_total_instructions", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].total_instructions = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_int_instructions", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].int_instructions = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_fp_instructions", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].fp_instructions = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_branch_instructions", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].branch_instructions = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_total_cycles", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].total_cycles = cntr_pos_value(str);
        sprintf(str, "P(%u)_core_int_regfile_reads", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].int_regfile_reads = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_int_regfile_writes", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].int_regfile_writes = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_function_calls", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].function_calls = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_bypassbus_access", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].bypassbus_access = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_storq_read_accesses", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].LSQ.read_accesses = cntr_pos_value(str) / nl;
        sprintf(str, "P(%u)_core_storq_write_accesses", i);
        if(check_cntr(str))
          sys.gpu.SMS[ii].lanes[j].LSQ.write_accesses = cntr_pos_value(str) / nl;

        // tinycache_data
        sprintf(str, "cache(%u)_read_accesses", ii * nl + j);
        if(check_cntr(str)) {
          sprintf(str, "cache(%u)_read_accesses", ii * nl + j);
          if(check_cntr(str))
            sys.gpu.SMS[ii].lanes[j].tinycache_data.read_accesses = cntr_pos_value(str);
          sprintf(str, "cache(%u)_read_misses", ii * nl + j);
          if(check_cntr(str))
            sys.gpu.SMS[ii].lanes[j].tinycache_data.read_misses = cntr_pos_value(str);
          sprintf(str, "cache(%u)_write_accesses", ii * nl + j);
          if(check_cntr(str))
            sys.gpu.SMS[ii].lanes[j].tinycache_data.write_accesses = cntr_pos_value(str);
          sprintf(str, "cache(%u)_write_misses", ii * nl + j);
          if(check_cntr(str))
            sys.gpu.SMS[ii].lanes[j].tinycache_data.write_misses = cntr_pos_value(str);
        } else { // no tinycache_data cache
          sys.gpu.homoSM.homolane.tinycache_data.dcache_config[0] = 0;
        }
        /*
                //tinycache_inst
                sprintf(str,"local_icache(%u)_read_accesses",ii*nl + j);
                if (check_cntr(str)) {
                  sprintf(str,"local_icache(%u)_read_accesses",ii*nl + j);
                  if ( check_cntr(str) ) sys.gpu.SMS[ii].lanes[j].tinycache_inst.read_accesses=cntr_pos_value(str);
                  sprintf(str,"local_icache(%u)_read_misses",ii*nl + j);
                  if ( check_cntr(str) ) sys.gpu.SMS[ii].lanes[j].tinycache_inst.read_misses=cntr_pos_value(str);
                  sprintf(str,"local_icache(%u)_write_accesses",ii*nl + j);
                  if ( check_cntr(str) ) sys.gpu.SMS[ii].lanes[j].tinycache_inst.write_accesses=cntr_pos_value(str);
                  sprintf(str,"local_icache(%u)_write_misses",ii*nl + j);
                  if ( check_cntr(str) ) sys.gpu.SMS[ii].lanes[j].tinycache_inst.write_misses=cntr_pos_value(str);
                } else { // no tinycache_inst cache
                  sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[0] = 0;
                }
        */
      }

      // Instruction Cache counters
      sprintf(str, "IL1G(%u)_read_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].icache.read_accesses = cntr_pos_value(str);
      sprintf(str, "IL1G(%u)_read_misses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].icache.read_misses = cntr_pos_value(str);
      sprintf(str, "IL1G(%u)_read_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].itlb.total_accesses = sys.gpu.SMS[ii].icache.read_accesses;
      sprintf(str, "IL1G(%u)_read_misses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].itlb.total_misses = 0.05 * sys.gpu.SMS[ii].icache.read_misses;

      // DL1G Cache counters
      sprintf(str, "DL1G(%u)_read_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].dcache.read_accesses = cntr_pos_value(str);
      sprintf(str, "DL1G(%u)_write_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].dcache.write_accesses = cntr_pos_value(str);
      sprintf(str, "DL1G(%u)_read_misses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].dcache.read_misses = cntr_pos_value(str);
      sprintf(str, "DL1G(%u)_write_misses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].dcache.write_misses = cntr_pos_value(str);

      // Data TLBs
      sys.gpu.SMS[ii].dtlb.total_accesses = sys.gpu.SMS[ii].dcache.read_accesses + sys.gpu.SMS[ii].dcache.write_accesses;
      sys.gpu.SMS[ii].dtlb.total_misses   = 0.05 * (sys.gpu.SMS[ii].dcache.read_misses + sys.gpu.SMS[ii].dcache.write_misses);

      // ScratchPad
      sprintf(str, "scratch(%u)_read_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].scratchpad.read_accesses = cntr_pos_value(str);
      sprintf(str, "scratch(%u)_write_accesses", i);
      if(check_cntr(str))
        sys.gpu.SMS[ii].scratchpad.write_accesses = cntr_pos_value(str);

      // Coalescing units

      // Register Files

      // Instruction Decode
    }

#if 1
    // GPU L2
    int i = 0; // FIXME: find how we declare L2 for GPU
    sprintf(str, "L2G(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.gpu.L2.read_accesses = cntr_pos_value(str);
    sprintf(str, "L2G(%u)_write_accesses", i);
    if(check_cntr(str))
      sys.gpu.L2.write_accesses = cntr_pos_value(str);
    sprintf(str, "L2G(%u)_read_misses", i);
    if(check_cntr(str))
      sys.gpu.L2.read_misses = cntr_pos_value(str);
    sprintf(str, "L2G(%u)_write_misses", i);
    if(check_cntr(str))
      sys.gpu.L2.write_misses = cntr_pos_value(str);
#endif
  }

  for(uint32_t i = 0; i < sys.number_of_L2s; i++) {
    sprintf(str, "L2(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.L2[i].read_accesses = cntr_pos_value(str);
    sprintf(str, "L2(%u)_write_accesses", i);
    if(check_cntr(str))
      sys.L2[i].write_accesses = cntr_pos_value(str);
    sprintf(str, "L2(%u)_read_misses", i);
    if(check_cntr(str))
      sys.L2[i].read_misses = cntr_pos_value(str);
    sprintf(str, "L2(%u)_write_misses", i);
    if(check_cntr(str))
      sys.L2[i].write_misses = cntr_pos_value(str);

    if(sys.core[0].scoore) // FIXME: only homo cores
    {
      if(sys.core[i].VPCfilter.dcache_config[0] > 0) {
        sprintf(str, "FL2(%u)_read_accesses", i);
        if(check_cntr(str))
          sys.core[i].VPCfilter.read_accesses = cntr_pos_value(str);
        sprintf(str, "FL2(%u)_write_accesses", i);
        if(check_cntr(str))
          sys.core[i].VPCfilter.write_accesses = cntr_pos_value(str);
        sprintf(str, "FL2(%u)_read_misses", i);
        if(check_cntr(str))
          sys.core[i].VPCfilter.read_misses = cntr_pos_value(str);
        sprintf(str, "FL2(%u)_write_misses", i);
        if(check_cntr(str))
          sys.core[i].VPCfilter.write_misses = cntr_pos_value(str);
      }
    }
  }

  for(uint32_t i = 0; i < sys.number_of_STLBs; i++) {
    int tmp = 0;
    sprintf(str, "STLB(%u)_read_hits", i);
    if(check_cntr(str))
      tmp = cntr_pos_value(str);
    sprintf(str, "STLB(%u)_read_misses", i);
    if(check_cntr(str)) {
      tmp += cntr_pos_value(str);
      sys.STLB[i].read_accesses = tmp;
    }
    sprintf(str, "STLB(%u)_read_misses", i);
    if(check_cntr(str))
      sys.STLB[i].read_misses = cntr_pos_value(str);
    sprintf(str, "STLB(%u)_write_accesses", i);
    if(check_cntr(str))
      sys.STLB[i].write_accesses = cntr_pos_value(str);
  }

  for(uint32_t i = 0; i < sys.number_of_L3s; i++) {
    sprintf(str, "L3(%u)_read_accesses", i);
    if(check_cntr(str))
      sys.L3[i].read_accesses = cntr_pos_value(str);
    sprintf(str, "L3(%u)_write_accesses", i);
    if(check_cntr(str))
      sys.L3[i].write_accesses = cntr_pos_value(str);
    sprintf(str, "L3(%u)_read_misses", i);
    if(check_cntr(str))
      sys.L3[i].read_misses = cntr_pos_value(str);
    sprintf(str, "L3(%u)_write_misses", i);
    if(check_cntr(str))
      sys.L3[i].write_misses = cntr_pos_value(str);
  }

  for(uint32_t i = 0; i < sys.number_of_NoCs; i++) {
    uint32_t reads = 0, writes = 0;
    sprintf(str, "xbar(%u)_read_accesses", i);
    if(check_cntr(str))
      reads = cntr_pos_value(str);
    sprintf(str, "xbar(%u)_write_accesses", i);
    if(check_cntr(str))
      writes = cntr_pos_value(str);
    sys.NoC[i].total_accesses = reads + writes;
  }

  string str1 = "mc_write_accesses";
  string str2 = "mc_read_accesses";

  if(check_cntr(str1))
    sys.mc.memory_writes = cntr_pos_value(str1);
  if(check_cntr(str2))
    sys.mc.memory_reads = cntr_pos_value(str2);

  return;
}

void ParseXML::parseEsescConf(const char *section_) {
  getGeneralParams();
  getCoreParams();
  getMemParams();
}

void ParseXML::getGeneralParams() {
  const char *section        = SescConf->getCharPtr("", "technology");
  sys.core_tech_node         = SescConf->getInt(section, "tech");
  sys.target_core_clockrate  = SescConf->getDouble(section, "frequency") / 1000000; // Mhz
  sys.device_type            = SescConf->getInt(section, "devType");
  sys.machine_bits           = SescConf->getInt(section, "machineBits");
  sys.physical_address_width = SescConf->getInt(section, "phyAddrBits");
  sys.virtual_address_width  = SescConf->getInt(section, "virAddrBits");
  sys.scale_dyn              = SescConf->getDouble(section, "scaleDynPow");
  sys.scale_lkg              = SescConf->getDouble(section, "scaleLkgPow");
  const char *therm_section  = SescConf->getCharPtr("", "thermal");
  const char *model_section  = SescConf->getCharPtr(therm_section, "model");
  sys.temperature = floor((SescConf->getDouble(model_section, "ambientTemp") / 10)) * 10; // CACTI accepts temperature in steps of
                                                                                          // 10
}

void ParseXML::getmimdGPUParams() {
}

void ParseXML::getGPUParams() {

  if(gpuIndex->size() > 0) {
    const char *section     = SescConf->getCharPtr("", "cpusimu", (*gpuIndex)[0]);
    const char *emu_section = SescConf->getCharPtr("", "cpuemul", (*gpuIndex)[0]);

    //--------------------- GPU CORE CONFIGURATION ----------------------//
    sys.number_of_GPU              = 1; // maximum number of GPUs currently supported my esesc.
    sys.gpu.number_of_SMs          = gpuIndex->size();
    sys.gpu.homoSM.number_of_lanes = SescConf->getInt("", "SP_PER_SM");
    ;
    sys.gpu.homoSM.homolane.clock_rate            = 2000; // FIXME: Hardcoded, read from esesc.conf?
    sys.gpu.homoSM.homolane.machine_bits          = 32;   // FIXME: Hardcoded, read from esesc.conf?
    sys.gpu.homoSM.homolane.virtual_address_width = sys.virtual_address_width;

    /* address width determins the tag_width in Cache, LSQ and buffers in cache controller
       default value is machine_bits, if not set
       */
    sys.gpu.homoSM.homolane.physical_address_width = sys.physical_address_width;

    sys.gpu.homoSM.homolane.instruction_length      = 32; // SescConf->getInt(section, "instWidth"); //FIXME
    sys.gpu.homoSM.homolane.opcode_width            = 11; // SescConf->getInt(section, "opcodeWidth"); //FIXME
    sys.gpu.homoSM.homolane.internal_datapath_width = 0;  // Does not exist in OOO.xml.

    /*number_of_hardware_threads defines how many blocks run on the same SM */
    sys.gpu.homoSM.homolane.number_hardware_threads = SescConf->getInt(emu_section, "max_blocks_sm");

    sys.gpu.homoSM.homolane.number_instruction_fetch_ports = 2; // SescConf->getInt(section, "fetchPorts"); // FIXME

    // sys.gpu.homoSM.homolane.pipelines_per_core       = 1,1;    // integer_pipeline and floating_pipelines, if the
    // floating_pipelines is 0, then the pipeline is shared
    sys.gpu.homoSM.homolane.pipelines_per_core[0] = 1;
    sys.gpu.homoSM.homolane.pipelines_per_core[1] = 1;

    // sys.gpu.homoSM.homolane.pipeline_depth           = 13,13;  // pipeline depth of int and fp, if pipeline is shared, the second
    // number is the average cycles of fp ops
    sys.gpu.homoSM.homolane.pipeline_depth[0] = 13;
    sys.gpu.homoSM.homolane.pipeline_depth[1] = 13;

    sys.gpu.homoSM.homolane.instruction_buffer_size = 16; // SescConf->getInt(section, "issueWidth") * 4;
    sys.gpu.homoSM.homolane.archi_Regs_IRF_size     = 32; // SescConf->getInt(section, "nArchRegs");
    sys.gpu.homoSM.homolane.phy_Regs_IRF_size       = 1024;
    sys.gpu.homoSM.homolane.store_buffer_size       = 32; // SescConf->getInt(section, "maxStores");
    sys.gpu.homoSM.homolane.memory_ports            = 2;  //<!-- number of ports refer to sustainable concurrent memory accesses -->
    sys.gpu.homoSM.homolane.LSQ_ports               = 0;

    //--------------------- GPU ICACHE CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "SM_IL1")) {
      const char *tmpsection = SescConf->getCharPtr("gpuCore", "SM_IL1");
      if(strcmp(tmpsection, "None") != 0) {

        FlowID size   = SescConf->getInt(tmpsection, "size");
        FlowID bsize  = SescConf->getInt(tmpsection, "bsize");
        FlowID assoc  = SescConf->getInt(tmpsection, "assoc");
        FlowID delay  = SescConf->getInt(tmpsection, "hitDelay");
        FlowID nBanks = SescConf->getInt(tmpsection, "numBanks");

        FlowID mshrSize       = 0;
        FlowID fillBuffSize   = 0;
        FlowID pfetchBuffSize = 0;
        FlowID wbBuffSize     = 0;

        const char *mshr_sec;
        if(SescConf->checkCharPtr(tmpsection, "MSHR")) {
          mshr_sec = SescConf->getCharPtr(tmpsection, "MSHR");
          mshrSize = SescConf->getInt(mshr_sec, "size");
        }
        if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
          pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
        }

        sys.gpu.homoSM.icache.icache_config[0] = 32768; // size;    // Capacity
        sys.gpu.homoSM.icache.icache_config[1] = 32;    // bsize;   // BlockWidth
        sys.gpu.homoSM.icache.icache_config[2] = 2;     // assoc;   // associativity
        sys.gpu.homoSM.icache.icache_config[3] = 1;     // nBanks;  // bank
        sys.gpu.homoSM.icache.icache_config[4] = 2;     // 2;       // throughput w.r.t core clock //CHECKME!!!
        sys.gpu.homoSM.icache.icache_config[5] = 3;     // delay;   // latency w.r.t core clock

        // icache does not support MSHR and fillBuffer
        sys.gpu.homoSM.icache.buffer_sizes[0] = 0; // mshrSize; // miss buffer size (MSHR)
        sys.gpu.homoSM.icache.buffer_sizes[1] = 0; // fillBuffSize;        // fill buffer size
        sys.gpu.homoSM.icache.buffer_sizes[2] =
            16; // pfetchBuffSize;    // prefetch buffer size, same as sys.gpu.homoSM.homolane.instruction_buffer_size?
        sys.gpu.homoSM.icache.buffer_sizes[3] = 0; // wbBuffSize;        // wb_buffer size

        sys.gpu.homoSM.itlb.number_entries = 128; // 0;
        if(SescConf->checkCharPtr(tmpsection, "SM_ITLBG")) {
          const char *itlb_sec = SescConf->getCharPtr(tmpsection, "SM_ITLBG");
          if(strcmp(itlb_sec, "None") != 0) {
            FlowID itlb_size                   = SescConf->getInt(itlb_sec, "size");
            FlowID itlb_bsize                  = SescConf->getInt(itlb_sec, "bsize");
            sys.gpu.homoSM.itlb.number_entries = 128; // itlb_size/itlb_bsize;
          }
        }

      } else {
        // There should be a SM_IL1 for the GPU.
        I(0);
        // error;
        fprintf(stderr, "\n SM_IL1 for the GPU must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      // There should be a SM_IL1 for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n SM_IL1 for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU DCACHE CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "SM_DL1")) {
      const char *tmpsection = SescConf->getCharPtr("gpuCore", "SM_DL1");
      if(strcmp(tmpsection, "None") != 0) {

        FlowID size   = SescConf->getInt(tmpsection, "size");
        FlowID bsize  = SescConf->getInt(tmpsection, "bsize");
        FlowID assoc  = SescConf->getInt(tmpsection, "assoc");
        FlowID delay  = SescConf->getInt(tmpsection, "hitDelay");
        FlowID nBanks = SescConf->getInt(tmpsection, "numBanks");

        FlowID mshrSize       = 0;
        FlowID fillBuffSize   = 0;
        FlowID pfetchBuffSize = 0;
        FlowID wbBuffSize     = 0;

        const char *mshr_sec;
        if(SescConf->checkCharPtr(tmpsection, "MSHR")) {
          mshr_sec = SescConf->getCharPtr(tmpsection, "MSHR");
          mshrSize = SescConf->getInt(mshr_sec, "size");
        }
        if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
          pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
        }

        if(SescConf->checkInt(tmpsection, "fillBuffSize")) {
          fillBuffSize = SescConf->getInt(tmpsection, "fillBuffSize");
        }
        if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
          pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
        }
        if(SescConf->checkInt(tmpsection, "wbBuffSize")) {
          wbBuffSize = SescConf->getInt(tmpsection, "wbBuffSize");
        }

        sys.gpu.homoSM.dcache.dcache_config[0] = 16786; // size; // Capacity
        sys.gpu.homoSM.dcache.dcache_config[1] = 32;    // bsize;    // BlockWidth
        sys.gpu.homoSM.dcache.dcache_config[2] = 2;     // assoc;     // associativity
        sys.gpu.homoSM.dcache.dcache_config[3] = 8;     // nBanks;     // bank
        sys.gpu.homoSM.dcache.dcache_config[4] = 1;     // 1; throughput w.r.t core clock
        sys.gpu.homoSM.dcache.dcache_config[5] = 3;     // delay;     // latency w.r.t core clock

        sys.gpu.homoSM.dcache.buffer_sizes[0] = 16; // mshrSize;      // miss buffer size (MSHR)
        sys.gpu.homoSM.dcache.buffer_sizes[1] = 4;  // fillBuffSize;  // fill buffer size
        sys.gpu.homoSM.dcache.buffer_sizes[2] = 16; // pfetchBuffSize;// prefetch buffer size
        sys.gpu.homoSM.dcache.buffer_sizes[3] = 16; // wbBuffSize;    // wb_buffer size

        sys.gpu.homoSM.dtlb.number_entries = 0;
        if(SescConf->checkCharPtr(tmpsection, "SM_DTLBG")) {
          const char *dtlb_sec = SescConf->getCharPtr(tmpsection, "SM_DTLBG");
          if(strcmp(dtlb_sec, "None") != 0) {
            FlowID dtlb_size                   = SescConf->getInt(dtlb_sec, "size");
            FlowID dtlb_bsize                  = SescConf->getInt(dtlb_sec, "bsize");
            sys.gpu.homoSM.dtlb.number_entries = dtlb_size / dtlb_bsize;
          }
        }

      } else {
        // There should be a SM_DL1 for the GPU.
        I(0);
        // error;
        fprintf(stderr, "\n SM_DL1 for the GPU must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      // There should be a SM_DL1 for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n SM_DL1 for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU SCRATCHPAD CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "scratchpad")) {
      const char *section = SescConf->getCharPtr("gpuCore", "scratchpad");
      if(strcmp(section, "None") != 0) {
        sys.gpu.homoSM.scratchpad.number_entries = 24567; // HARDCODED
      } else {
        // There should be a scratchpad for the GPU.
        I(0);
        // error;
        fprintf(stderr, "\n scratchpad for the GPU  must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      // There should be a scratchpad for the GPU.
      I(0);
      // error;
      fprintf(stderr, "\n scratchpad for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU DATA_TINYCACHE CONFIGURATION ----------------------//
    gpu_tcd_present = false;
    if(SescConf->checkCharPtr("gpuCore", "dfilter")) {
      const char *tmpsection = SescConf->getCharPtr("gpuCore", "dfilter");
      if(strcmp(tmpsection, "None") != 0) {
        gpu_tcd_present = true;

        FlowID size   = SescConf->getInt(tmpsection, "size");
        FlowID bsize  = SescConf->getInt(tmpsection, "bsize");
        FlowID assoc  = SescConf->getInt(tmpsection, "assoc");
        FlowID delay  = SescConf->getInt(tmpsection, "hitDelay");
        FlowID nBanks = SescConf->getInt(tmpsection, "numBanks");

        /*
        // tinycache_data does not have any buffers (MSHR, etc)
        FlowID mshrSize = 0;
        FlowID fillBuffSize = 0;
        FlowID pfetchBuffSize = 0;
        FlowID wbBuffSize = 0;

        const char *mshr_sec;
        if(SescConf->checkCharPtr(tmpsection, "MSHR")) {
          mshr_sec = SescConf->getCharPtr(tmpsection, "MSHR");
          mshrSize = SescConf->getInt(mshr_sec, "size");
        }
       if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
          pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
        }

       if(SescConf->checkInt(tmpsection, "fillBuffSize")) {
         fillBuffSize = SescConf->getInt(tmpsection, "fillBuffSize");
       }
       if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
         pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
       }
       if(SescConf->checkInt(tmpsection, "wbBuffSize")) {
         wbBuffSize = SescConf->getInt(tmpsection, "wbBuffSize");
       }
       */

        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[0] = 512; // size;   // Capacity
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[1] = 32;  // bsize;  // BlockWidth
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[2] = 1;   // assoc;  // associativity
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[3] = 1;   // nBanks; // bank
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[4] = 1;   // 1;      //throughput w.r.t core clock
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[5] = 3;   // delay;  // latency w.r.t core clock
      } else {
        // There need not be a data tinycache for the GPU
      }
    } else {
      // There need not be a data tinycache for the GPU
    }

    //--------------------- GPU INST_TINYCACHE CONFIGURATION ----------------------//
    gpu_tci_present = false;
    if(SescConf->checkCharPtr("gpuCore", "ifilter")) {
      const char *tmpsection = SescConf->getCharPtr("gpuCore", "ifilter");
      if(strcmp(tmpsection, "None") != 0) {
        gpu_tci_present = true;

        FlowID size   = SescConf->getInt(tmpsection, "size");
        FlowID bsize  = SescConf->getInt(tmpsection, "bsize");
        FlowID assoc  = SescConf->getInt(tmpsection, "assoc");
        FlowID delay  = SescConf->getInt(tmpsection, "hitDelay");
        FlowID nBanks = SescConf->getInt(tmpsection, "numBanks");

        /*
        // tinycache_inst does not have any buffers (MSHR, etc)
        FlowID mshrSize = 0;
        FlowID fillBuffSize = 0;
        FlowID pfetchBuffSize = 0;
        FlowID wbBuffSize = 0;

        const char *mshr_sec;
        if(SescConf->checkCharPtr(tmpsection, "MSHR")) {
          mshr_sec = SescConf->getCharPtr(tmpsection, "MSHR");
          mshrSize = SescConf->getInt(mshr_sec, "size");
        }
       if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
          pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
        }

       if(SescConf->checkInt(tmpsection, "fillBuffSize")) {
         fillBuffSize = SescConf->getInt(tmpsection, "fillBuffSize");
       }
       if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
         pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
       }
       if(SescConf->checkInt(tmpsection, "wbBuffSize")) {
         wbBuffSize = SescConf->getInt(tmpsection, "wbBuffSize");
       }
       */

        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[0] = 512; // size;   // Capacity
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[1] = 32;  // bsize;    // BlockWidth
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[2] = 1;   // assoc;     // associativity
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[3] = 1;   // nBanks;     // bank
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[4] = 1;   // 1;     // throughput w.r.t core clock
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[5] = 3;   // delay;     // latency w.r.t core clock
      } else {
        // There need not be a inst tinycache for the GPU
      }
    } else {
      // There need not be a inst tinycache for the GPU
    }

    //--------------------- GPU L2 CONFIGURATION ----------------------//

    if(SescConf->checkCharPtr("gpuCore", "GPU_L2")) {
      const char *tmpsection = SescConf->getCharPtr("gpuCore", "GPU_L2");

      FlowID size   = SescConf->getInt(tmpsection, "size");
      FlowID bsize  = SescConf->getInt(tmpsection, "bsize");
      FlowID assoc  = SescConf->getInt(tmpsection, "assoc");
      FlowID delay  = SescConf->getInt(tmpsection, "hitDelay");
      FlowID nBanks = SescConf->getInt(tmpsection, "numBanks");

      // tinycache_inst does not have any buffers (MSHR, etc)
      FlowID mshrSize       = 0;
      FlowID fillBuffSize   = 0;
      FlowID pfetchBuffSize = 0;
      FlowID wbBuffSize     = 0;

      const char *mshr_sec;
      if(SescConf->checkCharPtr(tmpsection, "MSHR")) {
        mshr_sec = SescConf->getCharPtr(tmpsection, "MSHR");
        mshrSize = SescConf->getInt(mshr_sec, "size");
      }
      if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
        pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
      }

      if(SescConf->checkInt(tmpsection, "fillBuffSize")) {
        fillBuffSize = SescConf->getInt(tmpsection, "fillBuffSize");
      }
      if(SescConf->checkInt(tmpsection, "pfetchBuffSize")) {
        pfetchBuffSize = SescConf->getInt(tmpsection, "pfetchBuffSize");
      }
      if(SescConf->checkInt(tmpsection, "wbBuffSize")) {
        wbBuffSize = SescConf->getInt(tmpsection, "wbBuffSize");
      }

      sys.gpu.L2.L2_config[0] = 262144; // size;      // Capacity
      sys.gpu.L2.L2_config[1] = 64;     // bsize;     // BlockWidth
      sys.gpu.L2.L2_config[2] = 16;     // assoc;     // associativity
      sys.gpu.L2.L2_config[3] = 1;      // nBanks;    // bank
      sys.gpu.L2.L2_config[4] = 3;      // 3;         // throughput w.r.t core clock
      sys.gpu.L2.L2_config[5] = 50;     // delay;     // latency w.r.t core clock

      sys.gpu.L2.buffer_sizes[0] = 8; // mshrSize;      // miss buffer size (MSHR)
      sys.gpu.L2.buffer_sizes[1] = 8; // fillBuffSize;  // fill buffer size
      sys.gpu.L2.buffer_sizes[2] = 8; // pfetchBuffSize;// prefetch buffer size
      sys.gpu.L2.buffer_sizes[3] = 8; // wbBuffSize;    // wb_buffer size

      sys.gpu.L2.clockrate = 2000;

      sys.gpu.L2.ports[0] = 1; // # Read ports
      sys.gpu.L2.ports[1] = 1; // # Write ports
      sys.gpu.L2.ports[2] = 1; // # Read-Write ports

      sys.gpu.L2.device_type = 2; // ???
    }

    /*
       sys.gpu.L2.threeD_stack     = 0;      // ??? Not defined in the original OOO.xml, what was this flag?
    */
  }
}
#if 0
void ParseXML::getGPUParams() {

  if (gpuIndex->size()>0) {


    //--------------------- GPU CORE CONFIGURATION ----------------------//
    sys.number_of_GPU = 1; //maximum number of GPUs currently supported my esesc.
    sys.gpu.number_of_SMs                                  = gpuIndex->size();
    sys.gpu.homoSM.number_of_lanes                         = SescConf->getInt("", "SP_PER_SM");;
    sys.gpu.homoSM.homolane.clock_rate                     = 2000; //FIXME: Hardcoded, read from esesc.conf?
    sys.gpu.homoSM.homolane.machine_bits                   = 32;   //FIXME: Hardcoded, read from esesc.conf?
//    sys.gpu.homoSM.homolane.machine_bits                   = SescConf->getInt(section, "machineBits");
    sys.gpu.homoSM.homolane.virtual_address_width          = sys.virtual_address_width;

    /* address width determins the tag_width in Cache, LSQ and buffers in cache controller
       default value is machine_bits, if not set
       */
    sys.gpu.homoSM.homolane.physical_address_width         = sys.physical_address_width;

    sys.gpu.homoSM.homolane.instruction_length             = 32;   //FIXME: Hardcoded, read from esesc.conf?
    sys.gpu.homoSM.homolane.opcode_width                   = 11;   //FIXME: Hardcoded, read from esesc.conf?
    sys.gpu.homoSM.homolane.internal_datapath_width        = 0;    // Does not exist in OOO.xml.

    /*number_of_hardware_threads defines how many blocks run on the same SM */
    const char *section = SescConf->getCharPtr("","cpuemul",(*gpuIndex)[0]);
    sys.gpu.homoSM.homolane.number_hardware_threads        = SescConf->getInt(section, "max_blocks_sm");

    const char *gpucore_section = SescConf->getCharPtr("","cpusimu",(*gpuIndex)[0]);
    //sys.gpu.homoSM.homolane.number_instruction_fetch_ports = 2;   // After discussion with professor Renau
    sys.gpu.homoSM.homolane.number_instruction_fetch_ports = SescConf->getInt(gpucore_section, "fetchPorts");


    //sys.gpu.homoSM.homolane.pipelines_per_core       = 1,1;    // integer_pipeline and floating_pipelines, if the floating_pipelines is 0, then the pipeline is shared
    sys.gpu.homoSM.homolane.pipelines_per_core[0] = 1;
    sys.gpu.homoSM.homolane.pipelines_per_core[1] = 1;

    //sys.gpu.homoSM.homolane.pipeline_depth           = 13,13;  // pipeline depth of int and fp, if pipeline is shared, the second number is the average cycles of fp ops
    sys.gpu.homoSM.homolane.pipeline_depth[0] = 13;
    sys.gpu.homoSM.homolane.pipeline_depth[1] = 13;

//    sys.gpu.homoSM.homolane.instruction_buffer_size  = 16; //<!-- buffer between IF and ID stage -->
    sys.gpu.homoSM.homolane.instruction_buffer_size  = SescConf->getInt(gpucore_section, "issueWidth") * 4
    sys.gpu.homoSM.homolane.archi_Regs_IRF_size      = 32;
    sys.gpu.homoSM.homolane.phy_Regs_IRF_size        = 1024;
    sys.gpu.homoSM.homolane.store_buffer_size        = 32;
    sys.gpu.homoSM.homolane.memory_ports             = 2; //<!-- number of ports refer to sustainable concurrent memory accesses -->
    sys.gpu.homoSM.homolane.LSQ_ports                = 0;

    sys.gpu.homoSM.itlb.number_entries               = 128;

    //--------------------- GPU ICACHE CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "SM_IL1")){
      const char *tmpsection = SescConf->getCharPtr("gpuCore","SM_IL1");
      if (strcmp(tmpsection, "None") != 0 ) {
        sys.gpu.homoSM.icache.icache_config[0] = 32768; // Capacity
        sys.gpu.homoSM.icache.icache_config[1] = 32;    // BlockWidth
        sys.gpu.homoSM.icache.icache_config[2] = 2;     // associativity
        sys.gpu.homoSM.icache.icache_config[3] = 1;     // bank
        sys.gpu.homoSM.icache.icache_config[4] = 2;     // throughput w.r.t core clock
        sys.gpu.homoSM.icache.icache_config[5] = 3;     // latency w.r.t core clock

        // icache does not support MSHR and fillBuffer
        sys.gpu.homoSM.icache.buffer_sizes[0]  = 0;     // miss buffer size (MSHR)
        sys.gpu.homoSM.icache.buffer_sizes[1]  = 0;     // fill buffer size
        sys.gpu.homoSM.icache.buffer_sizes[2]  = 16;    // prefetch buffer size, same as sys.gpu.homoSM.homolane.instruction_buffer_size?
        sys.gpu.homoSM.icache.buffer_sizes[3]  = 0;     // wb_buffer size
      } else {
        //There should be a SM_IL1 for the GPU.
        I(0);
        //error;
        fprintf(stderr,"\n SM_IL1 for the GPU must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      //There should be a SM_IL1 for the GPU.
      I(0);
      //error;
      fprintf(stderr,"\n SM_IL1 for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU DCACHE CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "SM_DL1")){
      const char *tmpsection = SescConf->getCharPtr("gpuCore","SM_IL1");
      if (strcmp(tmpsection, "None") != 0 ) {
        sys.gpu.homoSM.dtlb.number_entries               = 128;
        sys.gpu.homoSM.dcache.dcache_config[0] = 16786; // Capacity
        sys.gpu.homoSM.dcache.dcache_config[1] = 32;    // BlockWidth
        sys.gpu.homoSM.dcache.dcache_config[2] = 2;     // associativity
        sys.gpu.homoSM.dcache.dcache_config[3] = 8;     // bank
        sys.gpu.homoSM.dcache.dcache_config[4] = 1;     // throughput w.r.t core clock
        sys.gpu.homoSM.dcache.dcache_config[5] = 3;     // latency w.r.t core clock

        sys.gpu.homoSM.dcache.buffer_sizes[0]  = 16;    // miss buffer size (MSHR)
        sys.gpu.homoSM.dcache.buffer_sizes[1]  = 4;     // fill buffer size
        sys.gpu.homoSM.dcache.buffer_sizes[2]  = 16;    // prefetch buffer size
        sys.gpu.homoSM.dcache.buffer_sizes[3]  = 16;    // wb_buffer size
      } else {
        //There should be a SM_DL1 for the GPU.
        I(0);
        //error;
        fprintf(stderr,"\n SM_DL1 for the GPU must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      //There should be a SM_DL1 for the GPU.
      I(0);
      //error;
      fprintf(stderr,"\n SM_DL1 for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU SCRATCHPAD CONFIGURATION ----------------------//
    if(SescConf->checkCharPtr("gpuCore", "scratchpad")){
      const char *section = SescConf->getCharPtr("gpuCore","scratchpad");
      if (strcmp(section, "None") != 0 ) {
        sys.gpu.homoSM.scratchpad.number_entries = 24567;
      } else {
        //There should be a scratchpad for the GPU.
        I(0);
        //error;
        fprintf(stderr,"\n scratchpad for the GPU  must point to a valid section, cannot be \"None\" \n");
        exit(-1);
      }
    } else {
      //There should be a scratchpad for the GPU.
      I(0);
      //error;
      fprintf(stderr,"\n scratchpad for the GPU not defined \n");
      exit(-1);
    }

    //--------------------- GPU DATA_TINYCACHE CONFIGURATION ----------------------//
    gpu_tcd_present = false;
    if(SescConf->checkCharPtr("gpuCore", "dfilter")){
      const char *section = SescConf->getCharPtr("gpuCore","dfilter");
      if (strcmp(section, "None") != 0 ) {
        gpu_tcd_present = true;
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[0] = 512;   // Capacity
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[1] = 32;    // BlockWidth
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[2] = 1;     // associativity
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[3] = 1;     // bank
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[4] = 1;     // throughput w.r.t core clock
        sys.gpu.homoSM.homolane.tinycache_data.dcache_config[5] = 3;     // latency w.r.t core clock
        // tinycache_data does not have any buffers (MSHR, etc)
      } else {
        //There need not be a data tinycache for the GPU
      }
    } else {
      //There need not be a data tinycache for the GPU
    }

    //--------------------- GPU INST_TINYCACHE CONFIGURATION ----------------------//
    gpu_tci_present = false;
    if(SescConf->checkCharPtr("gpuCore", "ifilter")){
      const char *section = SescConf->getCharPtr("gpuCore","ifilter");
      if (strcmp(section, "None") != 0 ) {
        gpu_tci_present = true;
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[0] = 512;   // Capacity
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[1] = 32;    // BlockWidth
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[2] = 1;     // associativity
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[3] = 1;     // bank
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[4] = 1;     // throughput w.r.t core clock
        sys.gpu.homoSM.homolane.tinycache_inst.dcache_config[5] = 3;     // latency w.r.t core clock
        // tinycache_data does not have any buffers (MSHR, etc)
      } else {
        //There need not be a inst tinycache for the GPU
      }
    } else {
      //There need not be a inst tinycache for the GPU
    }


    //--------------------- GPU L2 CONFIGURATION ----------------------//
    sys.gpu.L2.L2_config[0]    = 262144; // Capacity
    sys.gpu.L2.L2_config[1]    = 64;     // BlockWidth
    sys.gpu.L2.L2_config[2]    = 16;     // associativity
    sys.gpu.L2.L2_config[3]    = 1;      // bank
    sys.gpu.L2.L2_config[4]    = 3;      // throughput w.r.t core clock
    sys.gpu.L2.L2_config[5]    = 50;     // latency w.r.t core clock

    sys.gpu.L2.buffer_sizes[0] = 8;      // miss buffer size (MSHR)
    sys.gpu.L2.buffer_sizes[1] = 8;      // fill buffer size
    sys.gpu.L2.buffer_sizes[2] = 8;      // prefetch buffer size
    sys.gpu.L2.buffer_sizes[3] = 8;      // wb_buffer size

    sys.gpu.L2.clockrate       = 2000;

    sys.gpu.L2.ports[0]        = 1;      // # Read ports
    sys.gpu.L2.ports[1]        = 1;      // # Write ports
    sys.gpu.L2.ports[2]        = 1;      // # Read-Write ports

    sys.gpu.L2.device_type     = 2;      // ???

    /*
       sys.gpu.L2.threeD_stack     = 0;      // ??? Not defined in the original OOO.xml, what was this flag?
    */
  }
}
#endif

void ParseXML::getCoreParams() {

  FlowID nsimu = SescConf->getRecordSize("", "cpusimu");
  // sys.number_of_cores = nsimu;
  sys.number_of_cores = (*coreIndex).size();

  getGPUParams();

  const char *first_section = SescConf->getCharPtr("", "cpusimu", 0);
  sys.homogeneous_cores     = 1;
  for(FlowID i = 0; i < nsimu; i++) {
    const char *section = SescConf->getCharPtr("", "cpusimu", i);

    if(strcmp(first_section, section) != 0)
      sys.homogeneous_cores = 0;

    uint64_t throttlingRatio                   = SescConf->getDouble(section, "throttlingRatio");
    sys.core[i].clock_rate                     = sys.target_core_clockrate / throttlingRatio;
    sys.core[i].machine_bits                   = sys.machine_bits;
    sys.core[i].virtual_address_width          = sys.virtual_address_width;
    sys.core[i].physical_address_width         = sys.physical_address_width;
    sys.core[i].instruction_length             = SescConf->getInt(section, "instWidth");
    sys.core[i].opcode_width                   = SescConf->getInt(section, "opcodeWidth");
    sys.core[i].machine_type                   = static_cast<int>(SescConf->getBool(section, "inorder"));
    sys.core[i].fetch_width                    = SescConf->getInt(section, "fetchWidth");
    sys.core[i].number_instruction_fetch_ports = SescConf->getInt(section, "fetchPorts");
    sys.core[i].decode_width                   = sys.core[i].fetch_width;
    sys.core[i].issue_width                    = SescConf->getInt(section, "issueWidth");
    sys.core[i].commit_width                   = SescConf->getInt(section, "retireWidth");
    sys.core[i].fp_issue_width                 = 1; // extract from # of fpu units
    sys.core[i].scoore                         = static_cast<int>(SescConf->getBool(section, "scooreCore"));
    sys.core[i].pipeline_depth[0]              = 12;
    sys.core[i].pipeline_depth[1]              = 12;

    FlowID nClusters = SescConf->getRecordSize(section, "cluster");
    FlowID nfpu      = 0;
    FlowID nIRegs    = 0;
    FlowID nFRegs    = 0;

    for(FlowID j = 0; j < nClusters; j++) {
      FlowID      nalu  = 0;
      const char *sec   = SescConf->getCharPtr(section, "cluster", j);
      FlowID      nRegs = SescConf->getInt(sec, "nRegs");
      if(strstr(sec, "AUNIT") != NULL) {
        if(nalu && nRegs != nIRegs)
          I(0); // number of regs in the clusters do not match!
        nalu++;
        nIRegs = nRegs;
      }
      if(strstr(sec, "CUNIT") != NULL) {
        if(nfpu && nRegs != nFRegs)
          I(0); // number of regs in the clusters do not match!
        nfpu++;
        nFRegs = nRegs;
      }
    }

    sys.core[i].ALU_per_core               = nClusters - nfpu;
    sys.core[i].FPU_per_core               = nfpu;
    sys.core[i].instruction_buffer_size    = sys.core[i].issue_width * 4;
    sys.core[i].decoded_stream_buffer_size = sys.core[i].fetch_width * 4;
    sys.core[i].instruction_window_scheme  = 0;
    sys.core[i].instruction_window_size    = SescConf->getInt(section, "instQueueSize");
    sys.core[i].fp_instruction_window_size = sys.core[i].instruction_window_size;
    sys.core[i].ROB_size                   = SescConf->getInt(section, "robSize");
    sys.core[i].archi_Regs_IRF_size        = SescConf->getInt(section, "nArchRegs");
    sys.core[i].archi_Regs_FRF_size        = sys.core[i].archi_Regs_IRF_size;
    sys.core[i].phy_Regs_IRF_size          = nIRegs;
    sys.core[i].phy_Regs_FRF_size          = nFRegs;
    sys.core[i].rename_scheme              = 0;
    strcpy(sys.core[i].LSU_order, "inorder"); // FIXME
    if(sys.core[i].scoore) {
      sys.core[i].ssit_size                       = SescConf->getInt(section, "StoreSetSize");
      sys.core[i].lfst_size                       = 32; // FIXME
      sys.core[i].lfst_width                      = SescConf->getInt(section, "StoreSetSize");
      sys.core[i].scoore_store_retire_buffer_size = SescConf->getInt(section, "specBufSize");
      sys.core[i].scoore_load_retire_buffer_size  = SescConf->getInt(section, "specBufSize");
    } else {

      sys.core[i].store_buffer_size = SescConf->getInt(section, "maxStores");
      sys.core[i].load_buffer_size  = SescConf->getInt(section, "maxLoads");
      sys.core[i].memory_ports      = 2; // FIXME
      sys.core[i].LSQ_ports         = 1; // FIXME
    }

    // FIXME: different branch predictor types?
    const char *sec      = SescConf->getCharPtr(section, "bpred");
    sys.core[i].RAS_size = SescConf->getInt(sec, "rasSize");
    if(!sys.core[i].RAS_size) {
      sys.core[i].RAS_size = 128;
    }

    int bpd                                         = SescConf->getInt(section, "bpredDelay");
    sys.core[i].predictor.prediction_width          = sys.core[i].fetch_width / bpd;
    sys.core[i].prediction_width                    = sys.core[i].predictor.prediction_width;
    sys.core[i].predictor.local_predictor_size      = SescConf->getInt(sec, "tbits");
    sys.core[i].predictor.local_predictor_entries   = SescConf->getInt(sec, "tsize");
    sys.core[i].predictor.global_predictor_entries  = SescConf->getInt(sec, "tsize");
    sys.core[i].predictor.global_predictor_bits     = SescConf->getInt(sec, "tbits");
    sys.core[i].predictor.chooser_predictor_entries = SescConf->getInt(sec, "tsize");
    sys.core[i].predictor.chooser_predictor_bits    = SescConf->getInt(sec, "tbits");
    sys.core[i].BTB.BTB_config[0]                   = SescConf->getInt(sec, "btbSize");
    sys.core[i].BTB.BTB_config[1]                   = sys.physical_address_width;
    sys.core[i].BTB.BTB_config[2]                   = SescConf->getInt(sec, "btbAssoc");
    sys.core[i].BTB.BTB_config[3]                   = SescConf->getInt(sec, "numBanks");
    sys.core[i].BTB.BTB_config[4]                   = 1;
    sys.core[i].BTB.BTB_config[5]                   = SescConf->getInt(section, "bpredDelay");
  }
}

void ParseXML::getMemParams() {

  FlowID nsimu = SescConf->getRecordSize("", "cpusimu");
  for(FlowID i = 0; i < nsimu; i++) {
    const char *def_block = SescConf->getCharPtr("", "cpusimu", i);

    if(strcmp(def_block, "gpuCORE") != 0) {
      fprintf(stderr, "\n Initializing CPU Memory objects");
      getMemoryObj(def_block, "IL1", i);
      if(SescConf->checkCharPtr("cpusimu", "VPC", i)) {
        getMemoryObj(def_block, "VPC", i);
        getMemoryObj(def_block, "DL1", i);
      } else {
        getMemoryObj(def_block, "DL1", i);
      }
    } else {
      // fprintf(stderr, "\n  Skipping initializing GPU Memory objects, they will be initiated separately by the gpuCore");
    }
  }
}

void ParseXML::getMemoryObj(const char *block, const char *field, FlowID Id) {

  std::vector<char *> vPars = SescConf->getSplitCharPtr(block, field);

  getConfMemObj(vPars, Id);
}

void ParseXML::getConfMemObj(std::vector<char *> vPars, FlowID Id) {

  bool   shared   = false; // Private by default
  FlowID nId      = 0;
  FlowID sharedBy = 1;

  const char *device_descr_section = vPars[0];
  char *      device_name          = (vPars.size() > 1) ? vPars[1] : 0;

  if(vPars.size() > 2) {

    if(strcasecmp(vPars[2], "shared") == 0) {

      I(vPars.size() == 3);
      shared   = true;
      sharedBy = sys.number_of_cores;

    } else if(strcasecmp(vPars[2], "sharedBy") == 0) {

      I(vPars.size() == 4);
      sharedBy = atoi(vPars[3]);
      GMSG(sharedBy <= 0, "ERROR: SharedBy should be bigger than zero (field %s)", device_name);

      nId    = Id / sharedBy;
      shared = true;
    }
  }

  SescConf->isCharPtr(device_descr_section, "lowerlevel");
  std::vector<char *> nvPars = SescConf->getSplitCharPtr(vPars[0], "lowerlevel");
  if(strcmp(nvPars[0], "voidDevice")) {
    // fprintf(stderr,"\n....*****..... %s",nvPars[0]);
    getConfMemObj(nvPars, Id);
  }

  // fprintf(stderr,"\n....*****..... Configuring %s",vPars[0]);
  // fprintf(stderr,"\n");

  SescConf->isCharPtr(device_descr_section, "deviceType");
  const char *device_type = SescConf->getCharPtr(device_descr_section, "deviceType");
  SescConf->isCharPtr(device_descr_section, "blockName");
  const char *blockName = SescConf->getCharPtr(device_descr_section, "blockName");

  if(strcmp(device_type, "cache") == 0 || strcmp(device_type, "icache") == 0 || strcmp(device_type, "dcache") == 0 ||
     strcmp(device_type, "tlb") == 0) {

    FlowID size   = SescConf->getInt(device_descr_section, "size");
    FlowID bsize  = SescConf->getInt(device_descr_section, "bsize");
    FlowID assoc  = SescConf->getInt(device_descr_section, "assoc");
    FlowID delay  = SescConf->getInt(device_descr_section, "hitDelay");
    FlowID nBanks = SescConf->getInt(device_descr_section, "numBanks");
    // FlowID rport = SescConf->getInt(device_descr_section, "maxReads");
    // FlowID wport = SescConf->getInt(device_descr_section, "maxWrites");

    // FlowID throttle = SescConf->getInt(device_descr_section, "throttle");
    // FIXME: We need a throttling ratio for the cache structures.
    FlowID throttle = 1;
    if(SescConf->checkBool(device_descr_section, "coreCoupledFreq")) {
      if(SescConf->getBool(device_descr_section, "coreCoupledFreq") == false) {
        throttle = 2;
      }
    } else {
      SescConf->notCorrect();
    }

    FlowID clockRate      = sys.target_core_clockrate / throttle;
    FlowID mshrSize       = 0;
    FlowID fillBuffSize   = 0;
    FlowID pfetchBuffSize = 0;
    FlowID wbBuffSize     = 0;

    const char *mshr_sec;
    if(SescConf->checkCharPtr(device_descr_section, "MSHR")) {
      mshr_sec = SescConf->getCharPtr(device_descr_section, "MSHR");
      mshrSize = SescConf->getInt(mshr_sec, "size");
    }
    if(SescConf->checkInt(device_descr_section, "fillBuffSize")) {
      fillBuffSize = SescConf->getInt(device_descr_section, "fillBuffSize");
    }
    if(SescConf->checkInt(device_descr_section, "pfetchBuffSize")) {
      pfetchBuffSize = SescConf->getInt(device_descr_section, "pfetchBuffSize");
    }
    if(SescConf->checkInt(device_descr_section, "wbBuffSize")) {
      wbBuffSize = SescConf->getInt(device_descr_section, "wbBuffSize");
    }
    FlowID entries = size / bsize;
    // FlowID ports;
    // if (rport == 0 || wport == 0)
    //  ports = sys.core[Id].issue_width;
    // else
    //  ports = rport;

    double force_lkg_w   = 0.0;
    double force_rddyn_w = 0.0;
    double force_wrdyn_w = 0.0;
    bool   force_lkg     = false;
    bool   force_rddyn   = false;
    bool   force_wrdyn   = false;

    if(SescConf->checkDouble(device_descr_section, "forceLkg")) {
      force_lkg_w = SescConf->getDouble(device_descr_section, "forceLkg");
      force_lkg   = true;
    }

    if(SescConf->checkDouble(device_descr_section, "forceRdDyn")) {
      force_rddyn_w = SescConf->getDouble(device_descr_section, "forceRdDyn");
      force_rddyn   = true;
    }

    if(SescConf->checkDouble(device_descr_section, "forceWrDyn")) {
      force_wrdyn_w = SescConf->getDouble(device_descr_section, "forceWrDyn");
      force_wrdyn   = true;
    }

    if(strstr(device_type, "tlb") != NULL) {
      if(strstr(blockName, "STLB") != NULL) {
        if(shared && !nId) {
          sys.number_of_STLBs   = 1;
          sys.homogeneous_STLBs = 1;
        } else {
          if(sys.number_of_cores >= sharedBy)
            sys.number_of_STLBs = ceil(sys.number_of_cores / sharedBy);
          else
            sys.number_of_STLBs = sys.number_of_cores;
          sys.homogeneous_STLBs = 1;
        }

        sys.STLB[nId].clockrate      = clockRate;
        sys.STLB[nId].device_type    = 2;
        sys.STLB[nId].STLB_config[0] = entries;
        sys.STLB[nId].STLB_config[1] = bsize;
        sys.STLB[nId].STLB_config[2] = assoc;
        sys.STLB[nId].STLB_config[3] = nBanks;
        sys.STLB[nId].STLB_config[4] = delay;
        sys.STLB[nId].STLB_config[5] = delay;
        //        sys.core[Id].STLB[nId].force_lkg_w       = force_lkg_w;
        //        sys.core[Id].STLB[nId].force_rddyn_w     = force_rddyn_w;
        //        sys.core[Id].STLB[nId].force_wrdyn_w     = force_wrdyn_w;
        //        sys.core[Id].STLB[nId].force_lkg         = force_lkg;
        //        sys.core[Id].STLB[nId].force_rddyn       = force_rddyn;
        //        sys.core[Id].STLB[nId].force_wrdyn       = force_wrdyn;

      } else {
        sys.core[Id].itlb.number_entries = sys.core[Id].dtlb.number_entries = entries;
      }

    } else if(strstr(device_type, "icache") != NULL) {
      sys.core[Id].icache.icache_config[0] = size;
      sys.core[Id].icache.icache_config[1] = bsize;
      sys.core[Id].icache.icache_config[2] = assoc;
      sys.core[Id].icache.icache_config[3] = nBanks;
      sys.core[Id].icache.icache_config[4] = 1.0;
      sys.core[Id].icache.icache_config[5] = delay;
      sys.core[Id].icache.buffer_sizes[0]  = mshrSize;
      sys.core[Id].icache.buffer_sizes[2]  = pfetchBuffSize;
      sys.core[Id].icache.force_lkg_w      = force_lkg_w;
      sys.core[Id].icache.force_rddyn_w    = force_rddyn_w;
      sys.core[Id].icache.force_wrdyn_w    = force_wrdyn_w;
      sys.core[Id].icache.force_lkg        = force_lkg;
      sys.core[Id].icache.force_rddyn      = force_rddyn;
      sys.core[Id].icache.force_wrdyn      = force_wrdyn;

    } else if(strstr(device_type, "VPC") != NULL) {
      sys.core[Id].dcache.dcache_config[0] = size;
      sys.core[Id].dcache.dcache_config[1] = bsize;
      sys.core[Id].dcache.dcache_config[2] = assoc;
      sys.core[Id].dcache.dcache_config[3] = nBanks;
      sys.core[Id].dcache.dcache_config[4] = 1.0;
      sys.core[Id].dcache.dcache_config[5] = delay;
      sys.core[Id].dcache.buffer_sizes[0]  = mshrSize;
      sys.core[Id].dcache.buffer_sizes[1]  = fillBuffSize;
      sys.core[Id].dcache.buffer_sizes[2]  = pfetchBuffSize;
      sys.core[Id].dcache.buffer_sizes[3]  = wbBuffSize;
      sys.core[Id].dcache.force_lkg_w      = force_lkg_w;
      sys.core[Id].dcache.force_rddyn_w    = force_rddyn_w;
      sys.core[Id].dcache.force_wrdyn_w    = force_wrdyn_w;
      sys.core[Id].dcache.force_lkg        = force_lkg;
      sys.core[Id].dcache.force_rddyn      = force_rddyn;
      sys.core[Id].dcache.force_wrdyn      = force_wrdyn;

    } else if(strstr(device_type, "cache") != NULL) {
      if(strstr(blockName, "dcache") != NULL) {
        sys.core[Id].dcache.dcache_config[0] = size;
        sys.core[Id].dcache.dcache_config[1] = bsize;
        sys.core[Id].dcache.dcache_config[2] = assoc;
        sys.core[Id].dcache.dcache_config[3] = nBanks;
        sys.core[Id].dcache.dcache_config[4] = 1.0;
        sys.core[Id].dcache.dcache_config[5] = delay;
        sys.core[Id].dcache.buffer_sizes[0]  = mshrSize;
        sys.core[Id].dcache.buffer_sizes[1]  = fillBuffSize;
        sys.core[Id].dcache.buffer_sizes[2]  = pfetchBuffSize;
        sys.core[Id].dcache.buffer_sizes[3]  = wbBuffSize;
        sys.core[Id].dcache.force_lkg_w      = force_lkg_w;
        sys.core[Id].dcache.force_rddyn_w    = force_rddyn_w;
        sys.core[Id].dcache.force_wrdyn_w    = force_wrdyn_w;
        sys.core[Id].dcache.force_lkg        = force_lkg;
        sys.core[Id].dcache.force_rddyn      = force_rddyn;
        sys.core[Id].dcache.force_wrdyn      = force_wrdyn;

      } else if(strstr(blockName, "FL2") != NULL) {
        I(sys.core[Id].scoore == 2);
        sys.core[Id].VPCfilter.dcache_config[0] = size;
        sys.core[Id].VPCfilter.dcache_config[1] = bsize;
        sys.core[Id].VPCfilter.dcache_config[2] = assoc;
        sys.core[Id].VPCfilter.dcache_config[3] = nBanks;
        sys.core[Id].VPCfilter.dcache_config[4] = 1.0;
        sys.core[Id].VPCfilter.dcache_config[5] = delay;
        sys.core[Id].VPCfilter.buffer_sizes[0]  = mshrSize;
        sys.core[Id].VPCfilter.buffer_sizes[1]  = fillBuffSize;
        sys.core[Id].VPCfilter.buffer_sizes[2]  = pfetchBuffSize;
        sys.core[Id].VPCfilter.buffer_sizes[3]  = wbBuffSize;
        sys.core[Id].VPCfilter.force_lkg_w      = force_lkg_w;
        sys.core[Id].VPCfilter.force_rddyn_w    = force_rddyn_w;
        sys.core[Id].VPCfilter.force_wrdyn_w    = force_wrdyn_w;
        sys.core[Id].VPCfilter.force_lkg        = force_lkg;
        sys.core[Id].VPCfilter.force_rddyn      = force_rddyn;
        sys.core[Id].VPCfilter.force_wrdyn      = force_wrdyn;

      } else if(strstr(blockName, "L2") != NULL) {
        if(sys.number_of_cores >= sharedBy)
          sys.number_of_L2s = ceil(sys.number_of_cores / sharedBy);
        else
          sys.number_of_L2s = sys.number_of_cores;
        sys.L2[nId].clockrate       = clockRate / 4;
        sys.L2[nId].device_type     = 2;
        sys.L2[nId].L2_config[0]    = size;
        sys.L2[nId].L2_config[1]    = bsize;
        sys.L2[nId].L2_config[2]    = assoc;
        sys.L2[nId].L2_config[3]    = nBanks;
        sys.L2[nId].L2_config[4]    = delay;
        sys.L2[nId].L2_config[5]    = delay;
        sys.L2[nId].buffer_sizes[0] = mshrSize;
        sys.L2[nId].buffer_sizes[1] = fillBuffSize;
        sys.L2[nId].buffer_sizes[2] = pfetchBuffSize;
        sys.L2[nId].buffer_sizes[3] = wbBuffSize;
        sys.L2[nId].force_lkg_w     = force_lkg_w;
        sys.L2[nId].force_rddyn_w   = force_rddyn_w;
        sys.L2[nId].force_wrdyn_w   = force_wrdyn_w;
        sys.L2[nId].force_lkg       = force_lkg;
        sys.L2[nId].force_rddyn     = force_rddyn;
        sys.L2[nId].force_wrdyn     = force_wrdyn;

      } else if(strstr(blockName, "L3") != NULL) {
        if(sys.number_of_cores >= sharedBy)
          sys.number_of_L3s = ceil(sys.number_of_cores / sharedBy);
        else
          sys.number_of_L3s = sys.number_of_cores;
        sys.L3[nId].clockrate       = clockRate / 4;
        sys.L3[nId].device_type     = 2;
        sys.L3[nId].L3_config[0]    = size;
        sys.L3[nId].L3_config[1]    = bsize;
        sys.L3[nId].L3_config[2]    = assoc;
        sys.L3[nId].L3_config[3]    = nBanks;
        sys.L3[nId].L3_config[4]    = delay;
        sys.L3[nId].L3_config[5]    = delay;
        sys.L3[nId].buffer_sizes[0] = mshrSize;
        sys.L3[nId].buffer_sizes[1] = fillBuffSize;
        sys.L3[nId].buffer_sizes[2] = pfetchBuffSize;
        sys.L3[nId].buffer_sizes[3] = wbBuffSize;
        sys.mc.llc_line_length      = bsize;
        sys.L3[nId].force_lkg_w     = force_lkg_w;
        sys.L3[nId].force_rddyn_w   = force_rddyn_w;
        sys.L3[nId].force_wrdyn_w   = force_wrdyn_w;
        sys.L3[nId].force_lkg       = force_lkg;
        sys.L3[nId].force_rddyn     = force_rddyn;
        sys.L3[nId].force_wrdyn     = force_wrdyn;
      }
    }
  } else if(strcmp(device_type, "memcontroller") == 0 || strcmp(device_type, "bus") == 0) {
    FlowID busWidth     = SescConf->getInt(device_descr_section, "busWidth");
    FlowID numPorts     = SescConf->getInt(device_descr_section, "numPorts");
    FlowID portOccp     = SescConf->getInt(device_descr_section, "portOccp");
    FlowID dramPageSize = SescConf->getInt(device_descr_section, "dramPageSize");
    // FlowID throttle = SescConf->getInt(device_descr_section, "throttle");
    FlowID throttle = 1;
    if(SescConf->checkBool(device_descr_section, "coreCoupledFreq")) {
      if(SescConf->getBool(device_descr_section, "coreCoupledFreq") == false) {
        throttle = 2;
      }
    } else {
      SescConf->notCorrect();
    }

    sys.mc.mc_clock                    = sys.target_core_clockrate / throttle / 4;
    sys.mc.memory_channels_per_mc      = numPorts;
    sys.mc.databus_width               = busWidth;
    sys.mc.addressbus_width            = sys.physical_address_width;
    sys.mc.req_window_size_per_channel = 32;
    sys.mc.IO_buffer_size_per_channel  = 32;

    sys.mem.peak_transfer_rate   = sys.mc.mc_clock * busWidth / portOccp;
    sys.virtual_memory_page_size = dramPageSize;
  }
}
