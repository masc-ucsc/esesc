//  Contributed by Ian Lee
//                 Joseph Nayfach - Battilana
//                 Jose Renau
//							 	 Ehsan K.Ardestani
//
// The ESESC/BSD License
//
// Copyright (c) 2005-2013, Regents of the University of California and
// the ESESC Project.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
//   - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
//   - Neither the name of the University of California, Santa Cruz nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/*******************************************************************************
File name:      ThermTrace.cpp
Description:
********************************************************************************/

#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <regex.h>
#include <string>

#include "Report.h"
#include "SescConf.h"
#include "ThermTrace.h"
#include "sescthermMacro.h"

void ThermTrace::tokenize(const char *str, TokenVectorType &tokens) {
  I(tokens.empty());

  const char *new_pos;
  do {
    if(str[0] == ' ' || str[0] == '\t' || str[0] == '\n') {
      str++;
      continue;
    }

    new_pos = strchr(str, ' ');
    if(new_pos == 0)
      new_pos = strchr(str, '\t');
    else {
      const char *tab_pos = strchr(str, '\t');
      if(tab_pos && tab_pos < new_pos)
        new_pos = tab_pos;
    }
    if(new_pos == 0)
      new_pos = strchr(str, '\n');
    else {
      const char *ret_pos = strchr(str, '\n');
      if(ret_pos && ret_pos < new_pos)
        new_pos = ret_pos;
    }

    if(*str == '+')
      str++;

    if(new_pos == 0) {
      // last string
      tokens.push_back(strdup(str));
      break;
    }

    int32_t len     = new_pos - str;
    char *  new_str = (char *)malloc(len + 4);
    strncpy(new_str, str, len);
    new_str[len] = 0;
    tokens.push_back(new_str);

    str = new_pos;

  } while(str[0] != 0);
}

// eka, read the energy counters from a vector
// TODO: (check the TODO list in the header file)
//
void ThermTrace::read_sesc_variable() {

  mapping.resize(getCounterSize());
  for(size_t j = 0; j < getCounterSize(); j++) {
    // LOG("variable[%d]=[%s] ",(int) j, energyCntrNames_->at(j));
    mapping[j].name = energyBundle->cntrs[j].getName();
  }
}
bool ThermTrace::grep(const char *line, const char *pattern) {

  int32_t status;
  regex_t re;

  if(regcomp(&re, pattern, REG_NOSUB | REG_ICASE) != 0)
    return false;

  status = regexec(&re, line, (size_t)0, NULL, 0);
  regfree(&re);
  if(status != 0)
    return false;

  return true;
}

const ThermTrace::FLPUnit *ThermTrace::findBlock(const char *name) const {

  for(size_t id = 0; id < flp.size(); id++) {
    if(strcasecmp(name, flp[id]->name) == 0)
      return flp[id];
  }

  return 0; // Not found
}

void ThermTrace::read_floorplan_mapping() {

  const char *thermSec = SescConf->getCharPtr("", "thermal");
  const char *flpSec   = SescConf->getCharPtr(thermSec, "floorplan", 0);
  const char *matchSec = SescConf->getCharPtr(thermSec, "layoutDescr", 0);
  std::cout << std::endl << "FLOORPLAN: " << flpSec << std::endl;
  // const char *flpSec = SescConf->getCharPtr("","floorplan");
  size_t min = SescConf->getRecordMin(flpSec, "blockDescr");
  size_t max = SescConf->getRecordMax(flpSec, "blockDescr");
  // SescConf->dump(); only for stand alone

  // Floor plan parameters
  for(size_t id = min; id <= max; id++) {
    if(!SescConf->checkCharPtr(flpSec, "blockDescr", id)) {
      MSG("There is a HOLE on the floorplan. This can create problems blockDescr[%d]", (int)id);
      exit(-1);
      continue;
    }

    const char *blockDescr = SescConf->getCharPtr(flpSec, "blockDescr", id);

    TokenVectorType descr;
    tokenize(blockDescr, descr);

    FLPUnit *xflp = new FLPUnit(strdup(descr[0]));
    xflp->id      = id;
    xflp->area    = atof(descr[1]) * atof(descr[2]);

    xflp->x       = atof(descr[3]);
    xflp->y       = atof(descr[4]);
    xflp->delta_x = atof(descr[1]);
    xflp->delta_y = atof(descr[2]);

// read the blockMatch from config, or not!
#if 1
    const char *blockMatch;
    if(SescConf->checkCharPtr(matchSec, "blockMatch", id))
      blockMatch = SescConf->getCharPtr(matchSec, "blockMatch", id);
    else {
      MSG("The layoutDescr section (%s) seems to be incomplete or wrong\n", matchSec);
      blockMatch = "Nadarim";
    }
#else
    // char blockMatch[1024];
    // sprintf(blockMatch, "blockMatch[%d] = %s", id, descr[0]);
#endif
    if(strcmp(blockMatch, "") != 0) {
      tokenize(blockMatch, xflp->match);
      flp.push_back(xflp);
    }
  }

  // Find mappings between variables and flp

  for(size_t id = 0; id < flp.size(); id++) {
    for(size_t i = 0; i < flp[id]->match.size(); i++) {
      for(size_t j = 0; j < mapping.size(); j++) {
        if(grep(mapping[j].name, flp[id]->match[i])) {

          LOG("mapping[%d].map[%d]=%d (%s -> %s)", (int)j, (int)mapping[j].map.size(), (int)id, flp[id]->match[i], mapping[j].name);

          I(id < flp.size());
          flp[id]->units++;
          I(j < mapping.size());
          mapping[j].area += flp[id]->area;
          mapping[j].map.push_back(id);
        }
      }
    }
  }

  for(size_t i = 0; i < mapping.size(); i++) {
    for(size_t j = 0; j < mapping[i].map.size(); j++) {
      float ratio = flp[mapping[i].map[j]]->area / mapping[i].area;
      mapping[i].ratio.push_back(ratio);
    }
  }

  for(size_t j = 0; j < mapping.size(); j++) {
    I(mapping[j].map.size() == mapping[j].ratio.size());
    if(mapping[j].map.empty()) {
      MSG("Error: sesc variable %s [%d] does not update any block", mapping[j].name, (int)j);
      // exit(3);
    }
  }
}

// eka, for static declaration
ThermTrace::ThermTrace() {
  energyCntrValues_ = 0;
  energyCntrNames_  = 0;
  energyCntrSample_ = 0;
  totalPowerSamples = 0;
  loglkg            = NULL;
}

void ThermTrace::dump() const {

  for(size_t i = 0; i < flp.size(); i++) {
    flp[i]->dump();
  }

  for(size_t i = 0; i < mapping.size(); i++) {
    mapping[i].dump();
  }
}

// initialize when declared as static
void ThermTrace::ThermTraceInit() {

  energyCntrSample_ = 0;
  read_sesc_variable();
  read_floorplan_mapping();
  scaledLkgCntrValues_ = new std::vector<float>(energyBundle->cntrs.size());
  energyCntrValues_    = new std::vector<std::vector<float>>(1);
  energyCntrValues_->at(0).resize(energyBundle->cntrs.size());
  hpThLeakageCoefs.set(-122.1, 19155.107, 1.214, 0.18);
  lstpThLeakageCoefs.set(19.5294, -5209.1702, 0.9928, -0.49);
  lpThLeakageCoefs.set(8.4025, -2213.1390, 0.9966, -0.184);
}

void ThermTrace::loadValues(ChipEnergyBundle *energyBundle, std::vector<float> *temperatures) {
  I(energyBundle->cntrs.size() != 0);

  for(size_t i = 0; i < mapping.size(); i++) {
    float lkg  = 0.0;
    float slkg = 0.0;
    for(size_t k = 0; k < mapping[i].map.size(); k++) {
      int32_t flp_id = mapping[i].map[k];
      slkg           = scaleLeakage((*temperatures)[flp_id], energyBundle->cntrs[i].getLkg(), energyBundle->cntrs[i].getDevType());

      energyBundle->cntrs[i].setScaledLkg(slkg);
      lkg += slkg;
    }
    (*scaledLkgCntrValues_)[i] = lkg;

    (*energyCntrValues_)[0][i] = (lkg + (*energyBundle).cntrs[i].getDyn());
  }
  dumpLeakage();

  energyCntrSample_ = 0;
  return;
}

// eka, changed to support vector as well as input_file
bool ThermTrace::read_energy(bool File_nVector) {

  if(File_nVector == true) {

#if 0
     // BINARY FILE
    float buffer[mapping.size()];

    I(input_fd_!=NULL);

    int32_t s = fread(buffer,sizeof(float),mapping.size(),input_fd_);

    if (s != (int)(sizeof(float)*mapping.size()))
      return false;

		totalPowerSamples++;
    // Do mapping
    for(size_t k=0;k<flp.size();k++) {
      flp[k]->energy = 0;
    }

    for(size_t i=0;i<mapping.size();i++) {

      for(size_t k=0;k<mapping[i].map.size();k++) {
        int32_t flp_id  = mapping[i].map[k];

        flp[flp_id]->energy += mapping[i].ratio[k]*buffer[i];
        std::cout<<"BUFFER: "<<buffer[i]<<std::endl;
      }
    }

#endif

#if 1
    // TEXT FILE
    float buffer[mapping.size()];
    float bufferA[mapping.size()];

    I(input_fd_ != NULL);

    // initialize buffer
    for(size_t i = 0; i < mapping.size(); i++)
      buffer[i] = 0;

    // read nFastForward number of samples
    int  j;
    bool moreSmpls = true;
    for(j = 0; j < (nFastForward * nFastForwardCoeff); j++) {
      for(size_t i = 0; i < mapping.size(); i++) {
        if(EOF == fscanf(input_fd_, "%e", &bufferA[i]))
          moreSmpls = false;

        if(!moreSmpls)
          break;

        buffer[i] += bufferA[i];
      }
      if(!moreSmpls)
        break;
    }
    totalPowerSamples += j; // j at max equals nFastForward
    if(!moreSmpls)
      if(j == 0)
        return false;

    actualnFastForward = j;

    // Average the read nFastForward samples
    for(size_t i = 0; i < mapping.size(); i++)
      buffer[i] /= static_cast<float>(j);

      // Now buffer contains valid energy numbers
#if 0
    FILE * dynlog = fopen("rDynPow","a");

    // dump dynamic power
     for(size_t i=0;i<mapping.size();i++) {
      fprintf(dynlog, "%g\t ",buffer[i]);
    }

		 fprintf(dynlog,"\n");
		 fclose(dynlog);

#endif
    // Do mapping
    for(size_t k = 0; k < flp.size(); k++) {
      flp[k]->energy = 0;
    }

    for(size_t i = 0; i < mapping.size(); i++) {

      for(size_t k = 0; k < mapping[i].map.size(); k++) {
        int32_t flp_id = mapping[i].map[k];

        // add dyn and lkg for total power
        flp[flp_id]->energy += (mapping[i].ratio[k] * (buffer[i] + scaledLkgCntrValues_->at(i)));

        // save in energyCntrValues_ as well
        energyCntrValues_->at(0)[i] = (buffer[i] + scaledLkgCntrValues_->at(i));
        // printf("Buffer: %e\n",buffer[i]);
      }
    }

#endif

#if 0
    printf("[");
    for(size_t i=0;i<mapping.size();i++) {
      printf(" %g",buffer[i]);
    }
    printf("]\n");
#endif

#if 0
    printf("[");
    for(size_t i=0;i<flp.size();i++) {
      printf(" %g",flp[i]->area);
    }
    printf("]\n");
#endif

    return true;
  } else {

    if(energyCntrValues_->size() <= energyCntrSample_)
      return false;

    I(energyCntrValues_->at(0).size() == mapping.size());

    totalPowerSamples++;
    // Do mapping
    for(size_t k = 0; k < flp.size(); k++) {
      flp[k]->energy = 0;
    }

    for(size_t i = 0; i < mapping.size(); i++) {

      for(size_t k = 0; k < mapping[i].map.size(); k++) {
        int32_t flp_id = mapping[i].map[k];
        flp[flp_id]->energy += mapping[i].ratio[k] * energyCntrValues_->at(energyCntrSample_).at(i);
      }
    }
#if 0
    FILE * flplog = fopen("rflpPow","a");

    // dump dynamic power
     for(size_t i=0;i<flp.size();i++) {
       fprintf(flplog, "%g\t",flp[i]-> energy);
     }

     fprintf(flplog,"\n"); fclose(flplog);
#endif
    energyCntrSample_++;
  }
  return true;
}

// eka, to dump temperature in transistor layer and map it
// back to structures in power model
void ThermTrace::BlockT2StrcT(const std::vector<MATRIX_DATA> *blkTemp, std::vector<float> *strTemp) {

  for(size_t id = 0; id < flp.size(); id++) {
    for(size_t i = 0; i < flp[id]->blk2strMap.size(); i++) {
      strTemp->at(flp[id]->blk2strMap[i]) = blkTemp->at(id);
      // printf("strTemp[%d]=%f, %d\t", index, strTemp->at(index), id);
    }
  }
}

// eka, to dump temperature in transistor layer and map it
// back to structures in power model
void ThermTrace::BlockT2StrcTMap() {

  // strTemp->resize(energyCntrNames_->size());
  // eka,Find mappings between variables and flp and
  // copy temperature to each match (power model structure)
  // FIXME: Move the mapping to config, avoid doing it in runtime
  for(size_t id = 0; id < flp.size(); id++) {
    for(size_t i = 0; i < flp[id]->match.size(); i++) {
      for(size_t j = 0; j < mapping.size(); j++) {
        if(grep(mapping[j].name, flp[id]->match[i])) {

          // LOG("mapping[%d].map[%d]=%d (%s -> %s)",
          //    (int) j, (int) mapping[j].map.size(), (int) id,
          //    flp[id]->match[i], mapping[j].name);
          // find index of match in energyCntrNames
          int index = -1;
          for(int ii = 0; ii < (int)energyBundle->cntrs.size(); ii++) {
            string name = energyBundle->cntrs[ii].getName();
            if(name.compare(flp[id]->match[i]) == 0) {
              index = ii;
              break;
            }
          }
          if(index == -1) {
            LOG("Could not find the map for <%s> in Power Counters.", flp[id]->match[i]);
          } else {
            flp[id]->blk2strMap.push_back(index);
          }
        }
      }
    }
  }
}

ThermTrace::~ThermTrace() {

  if(input_fd_) {
    fclose(input_fd_);
    input_fd_ = NULL;
  }
  if(loglkg != NULL) {
    fclose(loglkg);
    loglkg = NULL;
  }

  scaledLkgCntrValues_->clear();
}

void ThermTrace::loadLkgCoefs() {
  // eka,Find mappings between variables and flp and
  // to find the device type for each flp block
  for(size_t id = 0; id < flp.size(); id++) {
    for(size_t i = 0; i < flp[id]->match.size(); i++) {
      for(size_t j = 0; j < mapping.size(); j++) {
        if(grep(mapping[j].name, flp[id]->match[i])) {
          // set the device type
          switch(energyBundle->cntrs[j].getDevType()) {
          case 0:
            flp[id]->devType.set(hpThLeakageCoefs);
            break;
          case 1:
            flp[id]->devType.set(lstpThLeakageCoefs);
            break;
          case 2:
            flp[id]->devType.set(lpThLeakageCoefs);
            break;
          default:
            break;
          }
        }
      }
    }
  }
}

float ThermTrace::scaleLeakage(float temperature, float leakage, uint32_t device_type) {
  LeakageCoefs coefs;
  coefs.a = coefs.b = coefs.c = 0;
  coefs.offset                = 0.0;
  float temp                  = temperature;
  switch(device_type) {
  case 0:
    coefs.set(hpThLeakageCoefs);
    break;
  case 1:
    coefs.set(lstpThLeakageCoefs);
    break;
  case 2:
    coefs.set(lpThLeakageCoefs);
    break;
  default:
    break;
  }

  float scaleFactor = exp(coefs.a + (coefs.b / temp)) * pow(coefs.c, temp);
  if(scaleFactor > 5)
    scaleFactor = 5;
  float scaledLeakage = (coefs.offset + scaleFactor) * leakage;
  // std::cout<< temperature <<"\t"<<scaleFactor + coefs.offset<<"\t"<<device_type<<"\t"<<leakage<<"\t"<<scaledLeakage<<"\n";

  return scaledLeakage;
}
void ThermTrace::initDumpLeakage() {

  if(dumppwth) {
    char *fname_lkg = (char *)malloc(1023);
    sprintf(fname_lkg, "sesctherm_scaled_lkg_%s", Report::getNameID());
    loglkg = fopen(fname_lkg, "w");
    GMSG(loglkg == 0, "ERROR: could not open loglkg file \"%s\" (ignoring it)", fname_lkg);
    if(loglkg) {
      for(size_t ii = 0; ii < energyBundle->cntrs.size(); ii++) {
        fprintf(loglkg, "%s\t", energyBundle->cntrs[ii].getName());
      }
      fprintf(loglkg, "\n");
    }
  }
  return;
}

void ThermTrace::dumpLeakage() {
  if(dumppwth) {
    for(size_t i = 0; i < scaledLkgCntrValues_->size(); i++) {
      fprintf(loglkg, "%e\t", scaledLkgCntrValues_->at(i));
    }
    fprintf(loglkg, "\n");
    fflush(loglkg);
  }
}

void ThermTrace::initLoglkg(const char *tfilename) {

  FILE *tmploglkg;
  char *lkgfname = (char *)alloca(256);
  sprintf(lkgfname, "scaled_lkg_%s", tfilename);
  tmploglkg = fopen(lkgfname, "w");
  GMSG(tmploglkg == 0, "ERROR: could not open loglkg file \"%s\" (ignoring it)", lkgfname);
}
