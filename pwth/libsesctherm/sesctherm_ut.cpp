//  Contributed by Ehsan K.Ardestani
//                 Ian Lee
//                 Jose Renau
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
File name:      sesctherm_ut.cpp
Description:    Wrapper class structure creates the interface between SescTherm
                and the eSESC performance simulator.
********************************************************************************/

#include <fstream>
#include <iostream>
#include <map>

#include "SescConf.h"
#include "SescThermWrapper.h"

void print_usage(const char *argv0);

int main(int argc, const char **argv) {
  const char *        section      = 0;
  ChipEnergyBundle *  energyBundle = new ChipEnergyBundle;
  std::vector<float> *temperatures = NULL;

  SescThermWrapper *sesctherm = new SescThermWrapper;
  SescConf                    = new SConfig(argc, argv);

  for(int i = 0; i < 30; i++) {
    char name[128];
    sprintf(name, "b(%d)", i);
    char *conn    = "b(0)";
    float dyn     = 0.5;
    float lkg     = 0.001;
    int   devType = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, devType, 0, 0, dyn, lkg));
  }

  uint32_t tl;
  sesctherm->plug(section, energyBundle);
  std::cout << "PLUG DONE" << std::endl;
  sesctherm->calcTemp(energyBundle, temperatures, 10000, tl);
  std::cout << "CALC DONE" << std::endl;

  return 0;
}
