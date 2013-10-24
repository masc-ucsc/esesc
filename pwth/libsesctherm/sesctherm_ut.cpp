/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Ehsan K.Ardestani
                  Ian Lee
                  Jose Renau

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

********************************************************************************
File name:      sesctherm_ut.cpp
Description:    Wrapper class structure creates the interface between SescTherm 
                and the eSESC performance simulator.
********************************************************************************/

#include <iostream>
#include <fstream>
#include <map>

#include "SescConf.h"
#include "SescThermWrapper.h"

void print_usage(const char * argv0);

int main(int argc, const char **argv){
  const char* section = 0;
	ChipEnergyBundle *energyBundle = new ChipEnergyBundle;
  std::vector<float> *temperatures = NULL;

  SescThermWrapper *sesctherm = new SescThermWrapper;
  SescConf = new SConfig(argc, argv);

  for (int i=0; i<30; i++){
		char name[128];
		sprintf(name, "b(%d)", i);
		char *conn="b(0)";
    float dyn   = 0.5;
    float lkg   = 0.001;
    int devType = 0;
    energyBundle->cntrs.push_back(Container((const char *)name, (char *)conn, (char *)conn, devType, 0, 0, dyn, lkg));
	}

	uint32_t tl;
  sesctherm->plug(section, energyBundle);
  std::cout << "PLUG DONE" << std::endl;
  sesctherm->calcTemp(energyBundle, temperatures, 10000, tl);
  std::cout << "CALC DONE" << std::endl;

  return 0;
}
