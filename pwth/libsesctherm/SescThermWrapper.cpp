/*
    ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ehsan K.Ardestani 
                    Ian Lee
                    Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

********************************************************************************
File name:      SescThermWrapper.cpp
Description:    Wrapper class for integration with Esesc / McPat Power Simulation
********************************************************************************/

#include <fstream>
#include <map>

#include "SescThermWrapper.h"

class ChipEnergyBundle;

SescThermWrapper::SescThermWrapper (){
}


void SescThermWrapper::plug(const char* section, ChipEnergyBundle *energyBundle) {
  sesctherm.configure(energyBundle);
}  
   
int SescThermWrapper::calcTemp(ChipEnergyBundle *energyBundle,
		std::vector<float> * temperatures, uint64_t timeinterval, uint32_t &throttleLength){
	int return_signal = 0;
	return_signal = sesctherm.computeTemp(energyBundle,temperatures,timeinterval);
	throttleLength = sesctherm.get_throttleLength();
	return (return_signal);
}

