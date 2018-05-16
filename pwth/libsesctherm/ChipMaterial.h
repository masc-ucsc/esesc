//  Contributed by  Ian Lee
//                  Joseph Nayfach - Battilana
//                  Jose Renau
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
File name:      ChipMaterial.h
Classes:        Material
                Material_List
********************************************************************************/

#ifndef CHIP_MATERIAL_H
#define CHIP_MATERIAL_H

#include <fstream>
#include <map>

class ChipLayers;
class DataLibrary;

class Material {
public:
  Material() {
  }
  // These are separate equations for recomputing the conductivity of the materials based upon temperature
  Material(std::string &name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity, MATRIX_DATA emmisivity);

  static void calc_air_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_air_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_);

  static void calc_pcb_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_silicon_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_silicon_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_);

  static void calc_oil_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_pins_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_package_substrate_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_pwb_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_c4underfill_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_heat_spreader_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_heat_sink_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_heat_sink_fins_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_interconnect_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_interconnect_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_);

  static void calc_die_transistor_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_);

  static void calc_die_transistor_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_);

  static void calc_material_properties_dyn_density(int layer, int x, int y, DataLibrary *datalibrary_);

  static void calc_material_properties_dyn_temp(int layer, int x, int y, DataLibrary *datalibrary_);

  static MATRIX_DATA calc_effective_interconnect_layer_horizontal_conductivity(int material_layer, int layer, MATRIX_DATA density,
                                                                               DataLibrary *datalibrary_);

  static MATRIX_DATA calc_effective_interconnect_layer_vertical_conductivity(int layer, MATRIX_DATA density,
                                                                             DataLibrary *datalibrary_);

  static MATRIX_DATA calc_interlayer_conductivity(MATRIX_DATA dielectric_constant, int material, DataLibrary *datalibrary_);

  std::string name_;         // name of material
  MATRIX_DATA density_;      // density (row) kg/m^3
  MATRIX_DATA spec_heat_;    // specific heat (c) (J/kg*K)
  MATRIX_DATA conductivity_; // conductivity (k) (W/m*K)
  MATRIX_DATA emissivity_;
  int         type_; // defined based upon sesctherm3Ddefine.h

  friend std::ostream &operator<<(std::ostream &os, const Material &tmpmaterial) {

    os << "Material:[" << tmpmaterial.name_ << "] Density:[" << tmpmaterial.density_ << "]" << std::endl;
    os << "Material:[" << tmpmaterial.name_ << "] Specific Heat:[" << tmpmaterial.spec_heat_ << "]" << std::endl;
    os << "Material:[" << tmpmaterial.name_ << "] conductivity:[" << tmpmaterial.conductivity_ << "]" << std::endl;
    return (os);
  }
};

class Material_List : public Material {
public:
  Material_List() {
  }
  void      create(std::string &name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity, MATRIX_DATA emmisivity);
  Material &find(std::string name);
  friend std::ostream &operator<<(std::ostream &os, Material_List &tmpmaterial_list) {
    std::map<std::string, Material *>::iterator it = tmpmaterial_list.storage.begin();

    while(it != tmpmaterial_list.storage.end()) {
      os << it->second;
    }
    return (os);
  }
  std::map<std::string, Material *> storage;
};

#endif
