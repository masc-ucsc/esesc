/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2008 University of California, Santa Cruz.
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Ian Lee
                  Joseph Nayfach - Battilana
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
File name:      ClassDeclare.h
Description:    This handles the forward declarations of all the classes used
********************************************************************************/

#ifndef CLASS_DECLARE_H
#define CLASS_DECLARE_H

//layer info
class ChipLayers;

//library
class DataLibrary;

//base
class ModelUnit;

//chip
class ChipFloorplan_Unit;
class ChipFloorplan;

//config data
class ConfigData;

//graphics
class ThermGraphics;
class ThermGraphics_FileType;
class ThermGraphics_Color;

//material
class Material;
class Material_List;

//model
class ThermModel;

//utilities
class Utilities;
class RegressionLine;
template<typename T1, typename T2> class ValueEquals;
template <class T> class DynamicArray;
template <class T> class dynamic_array_row;

#endif
