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
File name:      ChipLayers.h
Classes:        ChipLayers
********************************************************************************/

#ifndef CHIP_LAYERS_H
#define CHIP_LAYERS_H

#include <map>
#include <vector>

#include "Utilities.h"

class ModelUnit;

class ChipLayers {
public:
  ChipLayers(DataLibrary *datalibrary, int num_layers);
  ~ChipLayers();
  void                     determine_layer_properties();
  void                     offset_layer();
  MATRIX_DATA              compute_average_temp();
  std::vector<MATRIX_DATA> compute_average_temps(int flp_layer);
  std::vector<MATRIX_DATA> compute_temps(int flp_layer);
  std::vector<MATRIX_DATA> compute_average_powers(int flp_layer);
  std::vector<MATRIX_DATA> compute_energy();

  static void allocate_layers(DataLibrary *datalibrary_);

  // this computes the average of both temperature and power across all of the layers
  static DynamicArray<ModelUnit> compute_layer_averages(std::vector<int> &layers, DataLibrary *datalibrary_);
  static DynamicArray<ModelUnit> compute_layer_diff(int layer1, int layer2, DataLibrary *datalibrary_);

  void print(bool detailed);
  void print_lumped_metrics();
  // static void ChipLayers::compute_offsets(int layer_num, DataLibrary* datalibrary_);

  void        accumulate_data_for_sample();
  void        compute_sample();
  int         flp_num_;
  int         layer_number_;
  std::string name_;
  int         type_; // the layer types are defined in the sesctherm3Ddefine.h
  MATRIX_DATA thickness_;

  MATRIX_DATA height_;
  MATRIX_DATA width_;
  MATRIX_DATA leftx_;   // calculated
  MATRIX_DATA bottomy_; // calculated
  MATRIX_DATA rightx_;
  MATRIX_DATA topy_;

  // these are the effective material properties for the layer (averaged)
  MATRIX_DATA horizontal_conductivity_;
  MATRIX_DATA vertical_conductivity_down_;
  MATRIX_DATA vertical_conductivity_up_;
  MATRIX_DATA spec_heat_;
  MATRIX_DATA density_;
  MATRIX_DATA convection_coefficient_; // used for fluid/air layers
  MATRIX_DATA emissivity_;

  // These are binary values, use DEFINES: HEAT_CONVECTION_TRANSFER, HEAT_CONDUCTION_TRANSFER, HEAT_EMMISION_TRANSFER for transfer
  // types
  unsigned int heat_transfer_methods_;

  MATRIX_DATA granularity_;

  ChipFloorplan *chip_floorplan_; // if this is a chip layer, this will be defined, NULL otherwise

  DataLibrary *datalibrary_; // link to the datalibrary

  std::vector<std::vector<ModelUnit *>> located_units_;
  DynamicArray<ModelUnit> *             floorplan_layer_dyn_; // this is the dynamic array which corresponds to this layer
  int                                   unused_dyn_count_;    // this stores the number of unused dynamic array entries

  std::map<MATRIX_DATA, int> coord_to_index_x_;
  std::map<MATRIX_DATA, int> coord_to_index_y_;

  // this is the temperature map for the layer (related to chip floorplan)
  // this is only currently used for the transistor layer these are temperatures averaged by flp unit
  std::vector<MATRIX_DATA> temperature_map_;

  std::vector<MATRIX_DATA> power_map_;

  DynamicArray<MATRIX_DATA> running_average_temperature_map_; // this is used to create the running average temperatures
  DynamicArray<MATRIX_DATA> running_average_energy_map_;
  DynamicArray<MATRIX_DATA> running_max_temperature_map_;
  DynamicArray<MATRIX_DATA> running_min_temperature_map_;
  DynamicArray<MATRIX_DATA> running_max_energy_map_;
  DynamicArray<MATRIX_DATA> running_min_energy_map_;
  DynamicArray<MATRIX_DATA> sample_temperature_map_; // this is the average temps over the sample period

  bool layer_used_; // indicates if the layer is used in the stackup
  int  num_layers_;

  bool        temp_locking_enabled_; // lock the temperature to lock_temp_
  MATRIX_DATA lock_temp_;

private:
  class temporary_layer_info {
  public:
    struct cmpgranularitythensize {
      bool operator()(const temporary_layer_info &a, const temporary_layer_info &b) const {
        if(EQ(a.granularity_, b.granularity_)) {
          return (LT(a.size_, b.size_));
        } else
          return (LT(a.granularity_, b.granularity_));
      }
    };
    temporary_layer_info(int layer_num, MATRIX_DATA granularity, MATRIX_DATA height, MATRIX_DATA width, MATRIX_DATA size,
                         int nregions, MATRIX_DATA areas, MATRIX_DATA areas_removed) {
      layer_num_     = layer_num;
      granularity_   = granularity;
      height_        = height;
      width_         = width;
      size_          = size;
      nregions_      = nregions;
      areas_         = areas;
      areas_removed_ = areas_removed;
    }
    int         layer_num_;
    MATRIX_DATA granularity_;
    MATRIX_DATA height_;
    MATRIX_DATA width_;
    MATRIX_DATA size_;
    int         nregions_;
    MATRIX_DATA areas_;
    MATRIX_DATA areas_removed_;
  };
};

#endif
