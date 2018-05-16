#ifndef BOUNDLE_H
#define BOUNDLE_H
/* License & includes {{{1 */
/*
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by
                  Jose Renau
                  Ehsan K.Ardestani

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <string.h>

class Container {
private:
  const char *name;
  char *      sysConn;
  char *      coreConn;
  int         devType;
  float       area;
  float       tdp;
  float       dyn;
  float       lkg;
  float       scaledLkg;
  uint32_t    cyc;
  bool        gpu;
  bool        header;

public:
  Container(const char *name_, char *sysConn_, char *coreConn_, int devType_, float area_, float tdp_, float dyn_, float lkg_,
            bool isGPU_ = false) {
    name      = strdup(name_);
    sysConn   = strdup(sysConn_);
    coreConn  = strdup(coreConn_);
    devType   = devType_;
    area      = area_;
    tdp       = tdp_;
    dyn       = dyn_;
    lkg       = lkg_;
    scaledLkg = lkg_;
    gpu       = isGPU_;
  }

  const char *getName() {
    return name;
  };
  const char *getCoreConn() {
    return coreConn;
  };
  float getDyn() {
    return dyn;
  };
  float getLkg() {
    return cyc == 0 ? 0 : lkg;
  };
  float getScaledLkg() {
    return cyc == 0 ? 0 : scaledLkg;
  };
  float getArea() {
    return area;
  };
  float getTdp() {
    return tdp;
  };
  int getDevType() {
    return devType;
  };
  uint32_t getCyc() {
    return cyc;
  };

  void setDyn(float dyn_) {
    dyn = dyn_;
  };
  void setLkg(float lkg_) {
    lkg = lkg_;
  };
  void setScaledLkg(float lkg_) {
    scaledLkg = lkg_;
  };
  void setCyc(float cyc_) {
    cyc = cyc_;
  };
  void setDevType(int dev) {
    devType = dev;
  };

  bool isGPU() {
    return gpu;
  };
  void setSPart(char p) {
    header = p == '0' ? false : true;
  };
  bool isInHeader() {
    return header;
  };
};

class ChipEnergyBundle {
public:
  std::vector<Container> cntrs;
  uint32_t               coreEIdx;
  uint32_t               ncores;
  uint32_t               nL2;
  uint32_t               nL3;
  std::vector<uint32_t>  coreIndex;
  float                  freq;
  std::vector<uint32_t>  gpuIndex;

  ChipEnergyBundle(){};

  Container *getContainer(const char *name) {
    for(size_t i = 0; i < cntrs.size(); i++) {
      if(strcmp(cntrs[i].getName(), name) == 0) {
        return &cntrs[i];
      }
    }
    printf("Something is wrong! Cannot find the boundle by name (%s).\n", name);
    exit(-1);
  }

  void setDynCycByName(const char *name, float dyn, uint32_t cyc) {
    Container *cont = getContainer(name);
    cont->setDyn(dyn);
    cont->setCyc(cyc);
  }
  void setFreq(float _freq) {
    freq = _freq;
  }
  float getFreq() {
    return freq;
  }
};
#endif
