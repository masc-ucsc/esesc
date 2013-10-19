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
File name:      SescThermWrapper.h
Classes:        SescThermWrapper
********************************************************************************/

#ifndef SESCTHERM_WRAPPER_H
#define SESCTHERM_WRAPPER_H

#include "SescTherm.h"


class ChipEnergyBundle;

//eka TODO:
//1- make the class definition cleaner in terms of
//public and private variables and methods
class SescThermWrapper {

  public: 
    SescThermWrapper();
    SescTherm sesctherm;
    //eka, plugs the sesctherm, energyCntrNames is passed so that the maping 
    //can be made during initialization as well as floorplaning
    void plug(const char *section, 
              ChipEnergyBundle *energyBundle);
    //deallocate structures, empty for now     
    void unplug() {
      return;
    }
    int calcTemp(ChipEnergyBundle *energyBundle, std::vector<float> *temperatures, 
				uint64_t timeinterval, uint32_t &throttleLength);
};
#endif
