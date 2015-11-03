/* License & includes  */
/*
ESESC: Super ESCalar simulator
Copyright (C) 2005 University California, Santa Cruz.

Contributed by  Alamelu Sankaranarayanan
Jose Renau
Gabriel Southern

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <sys/types.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <fstream>
#include <iostream>
#include <string>
//#include <cstdlib>
//#include <map>
using namespace std;

#include "EmuSampler.h"
#include "SescConf.h"
#include "GPUReader.h"
#include "GPUInterface.h"
//#include "SPARCInstruction.h"
#include "Snippets.h"
#include "callback.h"
#include "DInst.h"

extern uint32_t CUDAKernel::max_dfl_reg32;
extern uint32_t CUDAKernel::max_dfl_reg64;
extern uint32_t CUDAKernel::max_dfl_regFP;
extern uint32_t CUDAKernel::max_dfl_regFP64;

extern uint32_t CUDAKernel::max_sm_reg32;
extern uint32_t CUDAKernel::max_sm_reg64;
extern uint32_t CUDAKernel::max_sm_regFP;
extern uint32_t CUDAKernel::max_sm_regFP64;
extern uint32_t CUDAKernel::max_sm_addr;
extern uint32_t CUDAKernel::max_tracesize;


GPUReader::GPUReader(const char *section, EmulInterface * eint_)
  :Reader(section), eint(eint_) {

    class CUDAKernel *kernel = NULL;
    numFlows = 0;
    FlowID nemul = SescConf->getRecordSize("", "cpuemul");
    for (FlowID i = 0; i < nemul; i++) {
      const char *section = SescConf->getCharPtr("", "cpuemul", i);
      const char *type = SescConf->getCharPtr(section, "type");
      if (strcasecmp(type, "GPU") == 0) {
        numFlows++;
      }
    }
    MSG("GPU NumFlows = %d", numFlows);
    started = false;

    /* read the different GPU related parameters from the file */
    int paramcount = SescConf->getRecordSize(section, "params");
    if (paramcount > 1) {
      cout << "Only a single benchmark is supported." << endl;
      exit(-1);
    }

    char *s =
      (char *) (SescConf->getCharPtr(section, "params", paramcount - 1));
    ifstream file;
    file.open(s);

    if (!file) {
      cout <<
        "Error in opening benchmark description (*.info) file : " << s
        << endl;
      exit(-1);
    }

    string lineread;
    int instcount = 0;
    int realinstcount = 0;
    int bbcount = 0;
    int maxbbcount = 0;
    string tempstr;
    bool filedone = false;

    uint32_t kId = 0;

    while (std::getline(file, lineread))  // Read line by line
    {

      MSG("%s", lineread.c_str());
      bbcount = 1;

      while ((lineread.find("[KERNEL]") == string::npos)) {
        //          cout <<"Loop 1: " << lineread <<endl;
        lineread.clear();
        if (!std::getline(file, lineread)) {
          filedone = true;
          break;
        }
      }

      if (kernel != NULL) {
        string teststr = kernel->kernel_name;
        // MSG("Inserting..%s",teststr.c_str());
        kernels.insert(std::pair < string, CUDAKernel * >(teststr, kernel));
        kernelsId.insert(std::pair < string, uint32_t >(teststr, kId++));
        // MSG("Inserted %s",kernels[teststr]->kernel_name.c_str());
      }

      kernel = new CUDAKernel();
      kernel->predicates = 0;
      kernel->tracesize = 0;

      do {
        // cout <<"Loop 2: " << lineread <<endl;
        if (lineread.find("Name", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->kernel_name = lineread;
        } else if (lineread.find("SM_20", 0) != string::npos) {

          if (lineread.find("GMEM", 0) != string::npos) {
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            kernel->sm20.regs_per_thread = atoi(lineread.c_str());
          }else if (lineread.find("SMEM", 0) != string::npos) {
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            kernel->sm20.shmem_per_kernel = atoi(lineread.c_str());
          }else if (lineread.find("CMEM", 0) != string::npos) {
            CMEM tempcmem;
            string bankread = lineread.substr(0, lineread.find("]"));
            bankread = bankread.substr(bankread.find("[")+1,bankread.size());
            tempcmem.bank = atoi(bankread.c_str());
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            tempcmem.size= atoi(lineread.c_str());
            kernel->sm20.cmem.push_back(tempcmem);
          }
        } else if (lineread.find("SM_10", 0) != string::npos) {

          if (lineread.find("GMEM", 0) != string::npos) {
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            kernel->sm10.regs_per_thread = atoi(lineread.c_str());
          }else if (lineread.find("SMEM", 0) != string::npos) {
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            kernel->sm10.shmem_per_kernel = atoi(lineread.c_str());
          }else if (lineread.find("CMEM", 0) != string::npos) {
            CMEM tempcmem;
            string bankread = lineread.substr(0, lineread.find("]"));
            bankread = bankread.substr(bankread.find("[")+1,bankread.size());
            tempcmem.bank = atoi(bankread.c_str());
            lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
            tempcmem.size= atoi(lineread.c_str());
            kernel->sm10.cmem.push_back(tempcmem);
          }

        } else if (lineread.find("STARTPC", 0) != string::npos) {

          // Dont use this for anything
        } else if (lineread.find("NUMBER_BB", 0) != string::npos) {

          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          maxbbcount = atoi(lineread.c_str());
          kernel->bb = new BB[maxbbcount + 1];
          kernel->numBBs = maxbbcount;
          // MSG("New kernel, with %d basic
          // blocks",atoi(lineread.c_str()));
          // exit(-1);
        } else if (lineread.find("DFL_REG32", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->dfl_reg32 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_dfl_reg32 < kernel->dfl_reg32) {
            CUDAKernel::max_dfl_reg32 = kernel->dfl_reg32;
          }

        } else if (lineread.find("DFL_REG64", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size()); kernel->dfl_reg64 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_dfl_reg64 < kernel->dfl_reg64) {
            CUDAKernel::max_dfl_reg64 = kernel->dfl_reg64;
          }

        } else if (lineread.find("DFL_REGFP64", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->dfl_regFP64 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_dfl_regFP64 < kernel->dfl_regFP64) {
            CUDAKernel::max_dfl_regFP64 = kernel->dfl_regFP64;
          }

        } else if (lineread.find("DFL_REGFP", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->dfl_regFP = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_dfl_regFP < kernel->dfl_regFP) {
            CUDAKernel::max_dfl_regFP = kernel->dfl_regFP;
          }

        } else if (lineread.find("SM_REG32", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->sm_reg32 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_sm_reg32 < kernel->sm_reg32) {
            CUDAKernel::max_sm_reg32 = kernel->sm_reg32;
          }

        } else if (lineread.find("SM_REG64", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->sm_reg64 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_sm_reg64 < kernel->sm_reg64) {
            CUDAKernel::max_sm_reg64 = kernel->sm_reg64;
          }

        } else if (lineread.find("SM_REGFP64", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->sm_regFP64 = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_sm_regFP64 < kernel->sm_regFP64) {
            CUDAKernel::max_sm_regFP64 = kernel->sm_regFP64;
          }

        } else if (lineread.find("SM_REGFP", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->sm_regFP = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_sm_regFP < kernel->sm_regFP) {
            CUDAKernel::max_sm_regFP = kernel->sm_regFP;
          }
        } else if (lineread.find("SM_ADRR", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->sm_addr = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_sm_addr < kernel->sm_addr) {
            CUDAKernel::max_sm_addr = kernel->sm_addr;
          }

        } else if (lineread.find("TRACESIZE", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->tracesize = atoi(lineread.c_str());

          if ((uint32_t) CUDAKernel::max_tracesize < kernel->tracesize) {
            CUDAKernel::max_tracesize = kernel->tracesize;
          }

        } else if (lineread.find("PREDICATES", 0) != string::npos) {
          lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
          kernel->predicates = atoi(lineread.c_str());
        }

        if (!std::getline(file, lineread)) {
          filedone = true;
          break;
        }

      } while (lineread.find("[BB]") == string::npos);

      if (!filedone) {
        do {

          while ((lineread.find("[END_KERNEL]") == string::npos)
              && (lineread.find("[BB]") == string::npos)
              && (std::getline(file, lineread))) {
            // cout <<"Loop 3: " << lineread <<endl;
          }

          if (lineread.find("[BB]") != string::npos) {
            std::getline(file, lineread);
            // cout <<"Loop 4: " << lineread <<endl;
          }

          instcount = 0;
          realinstcount = 0;
          int tempint = 0;
          while ((lineread.find("[END_BB]") == string::npos)
              && (lineread.find("[END_KERNEL]") == string::npos)) {
            // cout<<lineread<<endl;
            if (lineread.find("ID", 0) != string::npos) {
              lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
              kernel->bb[bbcount].id = atoi(lineread.c_str());
            } else if (lineread.find("Label", 0) != string::npos) {
              lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
              kernel->bb[bbcount].label = lineread;
            } else if (lineread.find("Number_insts", 0) != string::npos) {
              lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
              kernel->bb[bbcount].number_of_insts = atoi(lineread.c_str());
              kernel->bb[bbcount].insts = new CudaInst[kernel->bb[bbcount].  number_of_insts];

              /* MSG("lineread = %s",lineread.c_str());
                 MSG("Basic block %d has %d
                 instructions",bbcount,kernel->bb[bbcount].number_of_insts);
                 */
              instcount = 0;
            } else if (lineread.find("BARRIER", 0) != string::npos) {
              //MSG("%s",lineread.c_str());
              lineread = lineread.substr(lineread.find("=") + 1, lineread.size());
              kernel->bb[bbcount].barrier = ( (atoi(lineread.c_str()) == 0) ? false:true);
              //if (kernel->isBarrierBlock(bbcount)) MSG("BB %d is a BARRIER BLOCK",bbcount);

            } else if (lineread.find(",", 0) != string::npos) {
              /* PC */
              kernel->bb[bbcount].insts [instcount].pc = atoi(lineread.substr(0, lineread.find(",")).c_str());
              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* Opcode */
              kernel->bb[bbcount].insts[instcount].setOpcode(lineread.substr(0, lineread.find(",")));
              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* Kind */
              kernel->bb[bbcount].insts [instcount].opkind = atoi(lineread.substr(0, lineread.find(",")).c_str());
              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* Jumplabel */
              // FIXME: Notice that there are more opcodes, no
              // need to use
              // jumpLabel (LBRANCH vs RBRANCH)
              kernel->bb[bbcount].insts[instcount].jumpLabel = atoi(lineread.substr(0, lineread.find(",")).c_str());
              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* Memaccesstype */

              tempint = atoi(lineread.substr(0, lineread.find(",")).c_str());

              switch (tempint) {
                case 0:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = RegisterMem;
                  break;

                case 1:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = GlobalMem;
                  break;

                case 2:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = ParamMem;
                  //MSG("%s",lineread.c_str());
                  break;

                case 3:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = LocalMem;
                  break;

                case 4:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = SharedMem;
                  break;
                case 5:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = ConstantMem;
                  break;
                case 6:
                  kernel->bb[bbcount].insts[instcount].memaccesstype = TextureMem;
                  break;

              }

              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* SRC1 */
              tempstr = lineread.substr(0, lineread.find(","));

              if ((tempstr != "") && (kernel->bb[bbcount].insts[instcount].  memaccesstype == ConstantMem)) {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_CONSTANT);
                //MSG("CONST:%s",tempstr.c_str());
              } else if ((tempstr != "") && (kernel->bb[bbcount].insts[instcount].memaccesstype == ParamMem)) {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_PARAM);
                //MSG("PARAM1:%s",tempstr.c_str());
              } else if (tempstr.find("__cuda") != string::npos){
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_PARAM);
                //MSG("PARAM2:%s",tempstr.c_str());
              } else if ((tempstr != "") && (kernel->bb[bbcount].insts[instcount].memaccesstype == TextureMem)) {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_TEXTURE);
              } else if (tempstr.find("0f") != string::npos) {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_NoDependence);
              } else if (tempstr.find("id") != string::npos) {
                // Special register
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_TID);
                if (kernel->bb[bbcount].insts[instcount].opcode == iLALU_LD){
                  //If its a load  with a special register, then can we treat this as a move?
                  kernel->bb[bbcount].insts[instcount].opcode = iAALU;
                  kernel->bb[bbcount].insts[instcount].opkind = 0;
                }

                // Ideally should be a unique SpRegister
              } else if (tempstr.find("f") != string::npos) {
                //MSG("F:%s",tempstr.c_str());
                tempstr = tempstr.erase(tempstr.find("f"), 1);
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDAFPSTART + atoi(tempstr.c_str()));
              } else if (tempstr.find("rd") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rd"), 2);
                //MSG("RD:%s",tempstr.c_str());
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDAINT2START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("rh") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rh"), 2);
                //MSG("RH:%s",tempstr.c_str());
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDAINT3START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("r") != string::npos) {
                //MSG("R:%s",tempstr.c_str());
                tempstr = tempstr.erase(tempstr.find("r"), 1);
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDAINTSTART + 1 + atoi(tempstr.c_str()));
              } else if (tempstr == "") {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_NoDependence);
              } else {
                //Immediate values;
                //MSG("SRC1---->%s<------",tempstr.c_str());
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_NoDependence);
              }

              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());
              /* SRC2 */
              tempstr = lineread.substr(0, lineread.find(","));
              if ((tempstr != "") && (kernel->bb[bbcount].insts[instcount].memaccesstype == ConstantMem)) {
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDA_CONSTANT);
              } else if ((tempstr != "") && (kernel->bb [bbcount].insts [instcount].memaccesstype == TextureMem)) {
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDA_TEXTURE);
              } else if ((tempstr != "") && (kernel->bb[bbcount].insts[instcount].memaccesstype == ParamMem)) {
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_PARAM);
                //MSG("PARAM1:%s",tempstr.c_str());
              } else if (tempstr.find("__cuda") != string::npos){
                kernel->bb[bbcount].insts[instcount].src1 = static_cast < RegType > (LREG_CUDA_PARAM);
                //MSG("PARAM2:%s",tempstr.c_str());
              } else if (tempstr.find("0f") != string::npos) {
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_NoDependence);
              } else if (tempstr.find("id") != string::npos) {
                // Special register
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDA_TID);  // Ideally
                // should be a unique SpRegister
              } else if (tempstr.find("f") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("f"), 1);
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDAFPSTART + atoi(tempstr.c_str()));
              } else if (tempstr.find("rd") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rd"), 2);
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDAINT2START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("rh") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rh"), 2);
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDAINT3START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("r") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("r"), 1);
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_CUDAINTSTART + 1 + atoi(tempstr.c_str()));
              } else if (tempstr == "") {
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_NoDependence);
              } else {
                //Immediate values;
                //MSG("SRC2---->%s<------",tempstr.c_str());
                kernel->bb[bbcount].insts[instcount].src2 = static_cast < RegType > (LREG_NoDependence);
              }


              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());
              /* SRC3 */
              tempstr = lineread.substr(0, lineread.find(","));
              if (tempstr.find("id") != string::npos) {
                // Special register
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_CUDA_TID);  // Ideally
                // should be a unique SpRegister
              } else if (tempstr.find("f") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("f"), 1);
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_CUDAFPSTART + atoi(tempstr.c_str()));
              } else if (tempstr.find("rd") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rd"), 2);
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_CUDAINT2START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("rh") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rh"), 2);
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_CUDAINT3START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("r") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("r"), 1);
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_CUDAINTSTART + 1 + atoi(tempstr.c_str()));
              } else if (tempstr == "") {
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_InvalidOutput);
              } else {
                //Immediate values;
                kernel->bb[bbcount].insts[instcount].src3 = static_cast < RegType > (LREG_NoDependence);
              }


              lineread = lineread.substr(lineread.find(",") + 1, lineread.size());

              /* DEST */
              // if (lineread.find(",") != string::npos){
              tempstr = lineread.substr(0, lineread.find(","));
              if (tempstr.find("id") != string::npos) { // Special register
                MSG("SHOULD NEVER HAPPEN :1");
                exit(1);
              } else if (tempstr.find("f") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("f"), 1);
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_CUDAFPSTART + atoi(tempstr.c_str()));
              } else if (tempstr.find("rd") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rd"), 2);
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_CUDAINT2START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("rh") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("rh"), 2);
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_CUDAINT3START + 1 + atoi(tempstr.c_str()));
              } else if (tempstr.find("r") != string::npos) {
                tempstr = tempstr.erase(tempstr.find("r"), 1);
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_CUDAINTSTART + 1 + atoi(tempstr.c_str()));
              } else if (kernel->bb[bbcount].insts[instcount].opcode == iRALU) {
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_InvalidOutput); //Invalid?
              } else if (tempstr == "") {
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_InvalidOutput);
              } else {
                //Predicates, Labels(branches),
                //MSG("DEST---->%s<------",tempstr.c_str());
                kernel->bb[bbcount].insts[instcount].dest1 = static_cast < RegType > (LREG_InvalidOutput);
              }
              instcount++;
              realinstcount++;
            }
            if (!std::getline(file, lineread)) {
              filedone = true;
              break;
            }
            }

            if (filedone) {
              break;
            }
            if (bbcount <= maxbbcount) {
              if (kernel->bb[bbcount].number_of_insts != instcount) {
                (kernel->bb[bbcount].number_of_insts = instcount);
              }
            }
            bbcount++;
            I(bbcount < 32668);
          } while (lineread.find("END_KERNEL") == string::npos);
        }      // End of if (!filedone)
      }

    }

    void GPUReader::start() {
      if (started)
        return;
      started = true;
      IS(MSG("GPUReader has started..."));
    }

    GPUReader::~GPUReader() {
    }

    //void GPUReader::queueInstruction(uint32_t insn, AddrType pc, AddrType addr, DataType data,char thumb,  FlowID fid, void *env, bool inEmuTiming) {
    void GPUReader::queueInstruction(uint32_t insn, AddrType pc, AddrType addr, char thumb,  FlowID fid, void *env, bool inEmuTiming) {

      if (cuda_execution_complete) {
        I(0);      // Should not happen
        return;
      }

      if (tsfifo[fid].full()) {
        istsfifoBlocked[fid] = true;
        //gsampler->decCount->add(gsampler->dec_icount);
        gsampler->AtomicDecrPhasenInst(1);
        gsampler->AtomicDecrTotalnInst(1);
        return;
      }

      RAWDInst *rinst = tsfifo[fid].getTailRef();
      I(rinst);
      I(insn);

      //float L1clkRatio = 1.0;
      //float L3clkRatio = EmuSampler::getTurboRatioGPU();
      //rinst->set(insn, pc, addr, data, L1clkRatio, L3clkRatio, inEmuTiming);

      //MSG("Creating RINST with addr =%llx ", addr);
      rinst->set(insn, pc, addr,inEmuTiming);

      //`MSG("esec_disas_cuda_inst uses addr =%llx ", addr);
      esesc_disas_cuda_inst(rinst,addr);

      I(rawInst.size() > fid);
      rawInst[fid]->inc();
      //rawInst_nosampler[fid]->inc();
      tsfifo[fid].push();
    }

    DInst *GPUReader::executeHead(FlowID fid){
      /*
         if (cuda_execution_started == 0 && cuda_execution_complete == 0) {
         I(0);
         return 0;
         }
         */
      // executeHead is called by the sampler, which knows only globafid
      uint64_t conta = 0;
      //bool breaknow = true;

      if (ruffer[fid].empty()) {
        while ((!tsfifo[fid].full()) /* && (breaknow == false) */ ) {
          pthread_yield();

          if (tsfifo[fid].empty()) {
            pthread_yield();
#if 0
            if (tsfifo[fid].empty()) {
              struct timespec ts = {0,1000};
              nanosleep(&ts, 0);
            }
#endif
            if (tsfifo[fid].empty()) {
              gsampler->pauseThread(fid);
              return 0;
            }
          }
          conta++;
          if (conta > 10) {
            break;
          }
        }

        if(tsfifo[fid].full()) {
          // To minimize false sharing (start by 7, enough for a long cache line)
          for (int i = 7; i < tsfifo[fid].size(); i++) {
            RAWDInst *rinst = tsfifo[fid].getHeadRef();

            if (rinst->getNumInst() == 0) {
              static int conta = 0;
              if (conta > 10) {
                I(0);
                return 0;
              }
              conta++;
              I(0);
              MSG("ERROR: 2.Assembly instruction not cracked:pc=0x%llx insn = 0x%x", rinst->getPC(), rinst->getInsn());
            }

            for (size_t j = 0; j < rinst->getNumInst(); j++) {
              DInst **dinsth = ruffer[fid].getInsertPointRef();  // DInst
              // ruffer holder(not a dinst, but a place to put the dinst *)
              AddrType addr = rinst->getAddr();
              //MSG("1. Creating DINST with addr =%llx ", addr);
              *dinsth = DInst::create(rinst->getInstRef(j), rinst, addr, fid);
              fillCudaSpecficfields(*dinsth, rinst, fid);
              ruffer[fid].add();
            }

            tsfifo[fid].pop();
          }
        }else{
          I(!tsfifo[fid].empty());
          RAWDInst *rinst = tsfifo[fid].getHeadRef();
          I(rinst->getNumInst());
          for (size_t j = 0; j < rinst->getNumInst(); j++) {
            DInst **dinsth = ruffer[fid].getInsertPointRef();  // DInst
            // ruffer holder(not a dinst, but a place to put the dinst *)
            AddrType addr = rinst->getAddr();
            //MSG("2. Creating DINST with addr =%llx ", addr);
            *dinsth = DInst::create(rinst->getInstRef(j), rinst, addr , fid);
            fillCudaSpecficfields(*dinsth, rinst, fid);
            ruffer[fid].add();
          }

          tsfifo[fid].pop();
        }
      }

      DInst *dinst = ruffer[fid].getHead();

      //I(dinst->isInEmu());

      if (dinst->isSharedAddress()){
        ((LD_shared).at(fid))->add(1);
      } else {
        ((LD_global).at(fid))->add(1);
      }

      ruffer[fid].popHead();
      //I(!dinst->isInEmu());

      I(dinst);      // We just added, there should be one or more

      //dinst->dump("GPUReader::executeHead");
      return dinst;
    }

    void GPUReader::fillCudaSpecficfields(DInst* dinst, RAWDInst* rinst, FlowID fid){
      uint64_t local_addr  = rinst->getAddr();
      uint32_t l_warpid    = (uint32_t) getspBits64(local_addr,32,36);
      uint32_t l_peid      = (uint32_t) getspBits64(local_addr,37,43);
			uint32_t l_blockid   = (uint32_t) getspBits64(local_addr,44,59);
			dinst->setcudastats(rinst, l_peid, l_warpid, l_blockid);
    }



    /*  */

    void GPUReader::reexecuteTail(FlowID fid) {
      /* safe/retire advance of execution  */
      ruffer[fid].advanceTail();
    }

    /*  */

    void GPUReader::syncHeadTail(FlowID fid) {
      /* replay triggers a syncHeadTail  */
      ruffer[fid].moveHead2Tail();
    }

    /*  */

    void GPUReader::drainFIFO(FlowID fid)
      /* Drain the tsfifo as much as possible due to a mode change  */
    {
      while (!tsfifo[fid].empty() && !ruffer[fid].empty()) {
        printf(".");
        pthread_yield();
      }
    }

    /*  */

    bool GPUReader::drainedFIFO(FlowID fid)
      /* Is the tsfifo drained? */
    {
      IS(fprintf(stderr,"(f:%d,r:%llu,t:%llu)\n",(int)fid,(int64_t)ruffer[fid].size(),(int64_t)tsfifo[fid].realsize()));
      if ((tsfifo[fid].realsize()<32 && (ruffer[fid].size() < 32 ))) {
        return true;
      }
      return false;
    }

