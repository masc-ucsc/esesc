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
File name:      ChipMaterial.h
Classes:        Material
                Material_List
********************************************************************************/
 
#ifndef CHIP_MATERIAL_H
#define CHIP_MATERIAL_H

#include <fstream>
#include <map>

class ChipLayers;

class Material{
  public:
    Material(){}
    //These are separate equations for recomputing the conductivity of the materials based upon temperature
    Material(std::string& name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity, MATRIX_DATA emmisivity);

    static void calc_air_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_air_layer_properties_dyn(int layer, int x, int y, DataLibrary* datalibrary_);

    static void calc_pcb_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_silicon_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_silicon_layer_properties_dyn(int layer, int x, int y, DataLibrary* datalibrary_);

    static void calc_oil_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_pins_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_package_substrate_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_pwb_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_c4underfill_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_heat_spreader_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_heat_sink_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void  calc_heat_sink_fins_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_interconnect_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_interconnect_layer_properties_dyn(int layer, int x, int y, DataLibrary* datalibrary_);

    static void calc_die_transistor_layer_properties(ChipLayers& layer, DataLibrary* datalibrary_);

    static void calc_die_transistor_layer_properties_dyn(int layer, int x, int y, DataLibrary* datalibrary_);

    static void calc_material_properties_dyn_density(int layer, int x, int y, DataLibrary* datalibrary_);

    static void calc_material_properties_dyn_temp(int layer, int x, int y, DataLibrary* datalibrary_);

    static MATRIX_DATA calc_effective_interconnect_layer_horizontal_conductivity(int material_layer, int layer, MATRIX_DATA density, DataLibrary* datalibrary_);

    static MATRIX_DATA calc_effective_interconnect_layer_vertical_conductivity(int layer, MATRIX_DATA density, DataLibrary* datalibrary_);

    static MATRIX_DATA calc_interlayer_conductivity(MATRIX_DATA dielectric_constant, int material, DataLibrary* datalibrary_);

    std::string name_;					//name of material
    MATRIX_DATA density_;				//density (row) kg/m^3
    MATRIX_DATA spec_heat_;				//specific heat (c) (J/kg*K)
    MATRIX_DATA conductivity_;			//conductivity (k) (W/m*K)
    MATRIX_DATA emissivity_;
    int type_;						//defined based upon sesctherm3Ddefine.h

    friend std::ostream& operator<<(std::ostream & os, const Material& tmpmaterial){

      os << "Material:[" << tmpmaterial.name_ << "] Density:[" << tmpmaterial.density_ << "]" << std::endl;
      os << "Material:[" << tmpmaterial.name_ << "] Specific Heat:[" << tmpmaterial.spec_heat_ << "]" << std::endl;
      os << "Material:[" << tmpmaterial.name_ << "] conductivity:[" << tmpmaterial.conductivity_ << "]" << std::endl;
      return(os);
    }
};

class Material_List : public Material{
  public:
    Material_List(){
    }
    void create(std::string& name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity, MATRIX_DATA emmisivity);
    Material& find(std::string name);
    friend std::ostream& operator<<(std::ostream &os, Material_List &tmpmaterial_list) {
      std::map<std::string, Material *>::iterator it = tmpmaterial_list.storage.begin();

      while(it != tmpmaterial_list.storage.end()){
        os << it->second;
      }
      return(os);
    }
    std::map<std::string, Material *> storage;
};

#endif
