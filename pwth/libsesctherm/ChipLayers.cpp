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
File name:      ChipLayers.cpp
Description: This stores all the information for each materials layer in the model
    Each layer can have one or more of the following:
    1) Assigned Chip floorplan
    2) Assigned Ucooler floorplan (This is currently untested)
    3) Assignmed BULK materials
    4) Assigned governing equations
    5) Assigned modes of heat transfer (conduction,convection etc)
    6) Accumulated temperature/power data for the graphics library
********************************************************************************/

#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#include "sescthermMacro.h"

#include "ClassDeclare.h"
#include "sesctherm3Ddefine.h"

#include "ChipFloorplan.h"
#include "ChipLayers.h"
#include "ChipMaterial.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ModelUnit.h"
#include "Utilities.h"

#include "nanassert.h"

ChipLayers::ChipLayers(DataLibrary *datalibrary, int num_layers) {
  floorplan_layer_dyn_ = new DynamicArray<ModelUnit>(datalibrary_);
  std::vector<ModelUnit *> located_units_layer;
  located_units_layer.clear();
  num_layers_ = num_layers;
  for(int i = 0; i < num_layers; i++) {
    located_units_.push_back(located_units_layer);
  }
  layer_number_               = -1;
  name_                       = "-1";
  type_                       = -1;
  thickness_                  = -1.0;
  height_                     = -1.0;
  width_                      = -1.0;
  leftx_                      = -1.0;
  bottomy_                    = -1.0;
  rightx_                     = -1.0;
  topy_                       = -1.0;
  horizontal_conductivity_    = -1.0;
  vertical_conductivity_down_ = -1.0;
  vertical_conductivity_up_   = -1.0;
  spec_heat_                  = -1.0;
  density_                    = -1.0;
  convection_coefficient_     = -1.0;
  granularity_                = 1.0;
  chip_floorplan_             = NULL;
  datalibrary_                = datalibrary;
  unused_dyn_count_           = 0;
  layer_used_                 = true;
  temp_locking_enabled_       = false;
  lock_temp_                  = -1.0;
}

ChipLayers::~ChipLayers() {
  // FIXME: this causes memory problems
  //              delete floorplan_layer_dyn_;
}

void ChipLayers::print(bool detailed) {
  std::cerr << "Printing Layer Number [" << layer_number_ << "]" << std::endl;
  std::cerr << "Enabled? =" << layer_used_ << std::endl;
  std::cerr << "[name_] =" << name_ << std::endl;
  std::cerr << "[type_] =" << type_ << std::endl;
  std::cerr << "[thickness_] =" << thickness_ << std::endl;

  std::cerr << "[height_]=" << height_ << std::endl;
  std::cerr << "[width_]=" << width_ << std::endl;
  std::cerr << "[leftx_]=" << leftx_ << std::endl;     // calculated
  std::cerr << "[bottomy_]=" << bottomy_ << std::endl; // calculated
  std::cerr << "[rightx_]=" << rightx_ << std::endl;
  std::cerr << "[topy_]=" << topy_ << std::endl;

  // these are the effective material properties for the layer (averaged)

  std::cerr << "[horizontal_conductivity_]=" << horizontal_conductivity_ << std::endl;
  std::cerr << "[vertical_conductivity_down_]=" << vertical_conductivity_down_ << std::endl;
  std::cerr << "[vertical_conductivity_up_]=" << vertical_conductivity_up_ << std::endl;
  std::cerr << "[spec_heat_]=" << spec_heat_ << std::endl;
  std::cerr << "[density_]=" << density_ << std::endl;
  std::cerr << "[convection_coefficient_]=" << convection_coefficient_ << std::endl; // used for fluid/air layers
  std::cerr << "[granularity_]=" << granularity_ << std::endl;
  std::cerr << "[flp_num_]=" << flp_num_ << std::endl;
  // cerr << "[heat transfer methods]=" << std::hex << heat_transfer_methods_ << std::endl;

  ASSERT_ESESCTHERM(datalibrary_ != NULL, "datalibrary_ == NULL", "ChipLayers", "operator<<");

  if(chip_floorplan_ != NULL) {
    std::cerr << std::endl;
    std::cerr << *chip_floorplan_; // print chip floorplan
    std::cerr << std::endl;
  }
  if(detailed) {
    // print the dynamic array that corresponds to this layer
    ModelUnit::print_dyn_layer(layer_number_, datalibrary_);
  }
  std::cerr << std::endl << std::endl;
}

void ChipLayers::print_lumped_metrics() {
  MATRIX_DATA lumped_vertical_resistance_above = 0;
  MATRIX_DATA lumped_vertical_resistance_below = 0;
  MATRIX_DATA lumped_ambient_resistance        = 0;
  MATRIX_DATA lumped_lateral_resistance        = 0;
  MATRIX_DATA lumped_capacitance               = 0;
  MATRIX_DATA lumped_rc_layer_above            = 0;
  MATRIX_DATA lumped_rc_layer_below            = 0;
  MATRIX_DATA lumped_rc_ambient                = 0;
  MATRIX_DATA lumped_rc_within_layer           = 0;

  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++) {
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < floorplan_layer_dyn_->nrows(); y_itor++) {
        for(unsigned int x_itor = 0; x_itor < floorplan_layer_dyn_->ncols(); x_itor++) {
          ModelUnit &munit = (*floorplan_layer_dyn_)[x_itor][y_itor];
          if(munit.defined_ == false)
            continue; // skip if unit is not defined

          if(munit.t_ptr[dir_top])
            lumped_vertical_resistance_above -= (*munit.t_ptr[dir_top]);
          if(munit.t_ptr[dir_bottom])
            lumped_vertical_resistance_below -= (*munit.t_ptr[dir_bottom]);

          // if(munit.tINF_mno!=0){
          //      lumped_ambient_resistance+=(*munit.tINF_mno)*-1.0;
          //}
          MATRIX_DATA tmp_lateral_resistance = 0;

          int lateral_dirs[4] = {dir_left, dir_right, dir_down, dir_up};
          for(int dirNum = 0; dirNum < 4; dirNum++) {
            int dir = lateral_dirs[dirNum];
            if(munit.t_ptr[dir])
              tmp_lateral_resistance -= *(munit.t_ptr[dir]);
          }
          lumped_lateral_resistance += tmp_lateral_resistance / 4;

          lumped_capacitance += munit.specific_heat_ * munit.x2_ * munit.y2_ * munit.z2_ * munit.row_;
        }
      }
    }
  } // for each layer

  if(lumped_vertical_resistance_above != 0)
    lumped_vertical_resistance_above = (1 / lumped_vertical_resistance_above);
  else
    lumped_vertical_resistance_above = 0;

  if(lumped_vertical_resistance_below != 0)
    lumped_vertical_resistance_below = (1 / lumped_vertical_resistance_below);
  else
    lumped_vertical_resistance_below = 0;

  if(lumped_ambient_resistance != 0)
    lumped_ambient_resistance = (1 / lumped_ambient_resistance);
  else
    lumped_ambient_resistance = 0;

  if(lumped_lateral_resistance != 0)
    lumped_lateral_resistance = (1 / lumped_lateral_resistance);
  else
    lumped_lateral_resistance = 0;

  lumped_rc_layer_above  = lumped_vertical_resistance_above * lumped_capacitance;
  lumped_rc_layer_below  = lumped_vertical_resistance_below * lumped_capacitance;
  lumped_rc_ambient      = lumped_rc_ambient * lumped_capacitance;
  lumped_rc_within_layer = lumped_lateral_resistance * lumped_capacitance;

  std::cerr << "Printing Layer Number  [" << layer_number_ << "] Lumped Metric Data" << std::endl;
  std::cerr << "[Lumped Vertical Resistance to layer above (K/W):]" << lumped_vertical_resistance_above << std::endl;
  std::cerr << "[Lumped Vertical Resistance to layer below (K/W):]" << lumped_vertical_resistance_below << std::endl;
  std::cerr << "[Lumped Resistance to ambient (K/W):]" << lumped_ambient_resistance << std::endl;
  std::cerr << "[Lumped Lateral Resistance within layer (K/W):]" << lumped_lateral_resistance << std::endl;
  std::cerr << "[Lumped Capacitance (W/K):]" << lumped_capacitance << std::endl;
  std::cerr << "[Lumped RC-time constant to layer above (s):]" << lumped_rc_layer_above << std::endl;
  std::cerr << "[Lumped RC-time constant to layer below (s):]" << lumped_rc_layer_below << std::endl;
  std::cerr << "[Lumped RC-time constant to layer ambient (s):]" << lumped_rc_ambient << std::endl;
  std::cerr << "[Lumped RC-time constant within layer (s):]" << lumped_rc_within_layer << std::endl;
  std::cerr << std::endl;
}

// 1) computes the absolute coordinates for the various layers
// 2) determines the amount of space necessary for each layer, then allocates the space
void ChipLayers::allocate_layers(DataLibrary *datalibrary_) {
  // generate the leftx, rightx, bottomy, and topy coordinates for each layer
  // first we find the widest and tallest layers (this is used to generate the the absolute coordinates)
  // the algorithm simple centers each layer with respect to the model width and height
  MATRIX_DATA                       model_width  = 0;
  MATRIX_DATA                       model_height = 0;
  std::vector<temporary_layer_info> layerinfo; // granularities to index

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    model_width  = MAX(model_width, datalibrary_->all_layers_info_[i]->width_);
    model_height = MAX(model_height, datalibrary_->all_layers_info_[i]->height_);
  }

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->leftx_ = (model_width / 2) - (datalibrary_->all_layers_info_[i]->width_ / 2);
    datalibrary_->all_layers_info_[i]->rightx_ =
        datalibrary_->all_layers_info_[i]->leftx_ + datalibrary_->all_layers_info_[i]->width_;

    datalibrary_->all_layers_info_[i]->bottomy_ = (model_height / 2) - (datalibrary_->all_layers_info_[i]->height_ / 2);
    datalibrary_->all_layers_info_[i]->topy_ =
        datalibrary_->all_layers_info_[i]->bottomy_ + datalibrary_->all_layers_info_[i]->height_;
  }

  // Now fill in all the missing granularities
  // If a particular layer's granularity is defined as -1, then it should be the same the finest granularity of the adjacent layers
  // if the granularity is zero, this means that the granularity is effectively infinity (coarsest granularity)

  for(int j = 0; j < (int)datalibrary_->all_layers_info_.size(); j++) {
    if(EQ(datalibrary_->all_layers_info_[j]->granularity_, 0.0))
      datalibrary_->all_layers_info_[j]->granularity_ = pow(10, 5.0); // set the granularities to infinity
    else if(EQ(datalibrary_->all_layers_info_[j]->granularity_, -1.0))
      datalibrary_->all_layers_info_[j]->granularity_ = pow(10, 6.0); // set the granularities to minimum of adjacent
  }

  // This requires an O(N^2) operation where we propagate the granularities to the missing layers
  for(int j = 0; j < (int)datalibrary_->all_layers_info_.size(); j++) {
    for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
      int         layer_below = MAX(0, i - 1);
      int         layer_above = MIN((int)datalibrary_->all_layers_info_.size() - 1, i + 1);
      MATRIX_DATA min =
          MIN(datalibrary_->all_layers_info_[layer_below]->granularity_, datalibrary_->all_layers_info_[layer_above]->granularity_);
      if(EQ(datalibrary_->all_layers_info_[j]->granularity_, pow(10, 6.0)) && LT(min, pow(10, 5.0)))
        datalibrary_->all_layers_info_[i]->granularity_ = min;
    }
  }

  for(int j = 0; j < (int)datalibrary_->all_layers_info_.size(); j++) {
    if(EQ(datalibrary_->all_layers_info_[j]->granularity_, pow(10, 5.0)))
      datalibrary_->all_layers_info_[j]->granularity_ = 99999999; // set the granularities to infinity
  }

  // make a guess as to the size of the dynamic arrays for each layer
  int flp_count = 0;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    temporary_layer_info newinfo(i, datalibrary_->all_layers_info_[i]->granularity_, datalibrary_->all_layers_info_[i]->height_,
                                 datalibrary_->all_layers_info_[i]->width_,
                                 datalibrary_->all_layers_info_[i]->height_ * datalibrary_->all_layers_info_[i]->width_, 0,
                                 datalibrary_->all_layers_info_[i]->height_ * datalibrary_->all_layers_info_[i]->width_, 0);
    layerinfo.push_back(newinfo);
    if(datalibrary_->all_layers_info_[i]->chip_floorplan_)
      flp_count += datalibrary_->all_layers_info_[layerinfo[i].layer_num_]->chip_floorplan_->flp_units_.size();
  }

  // sort by granularity and then size
  // std::sort (layerinfo.begin (), layerinfo.end (), temporary_layer_info::cmpGranularityThenSize ());
  sort(layerinfo.begin(), layerinfo.end(), temporary_layer_info::cmpgranularitythensize());

  unsigned int i;
  /*
     for( i = 0; i < layerinfo.size(); i++){
     std::cerr << i << "\t" << layerinfo[i].layer_num_ << "\t" << layerinfo[i].granularity_ << "\t" << layerinfo[i].height_ << "\t"
     << layerinfo[i].width_ << "\t" << layerinfo[i].size_ << std::endl;
     }
   */

  for(i = 0; i != layerinfo.size(); i++) {
    layerinfo[i].nregions_ += (int)(layerinfo[i].areas_ / (powf(layerinfo[i].granularity_, 2.0))) + flp_count;

    unsigned int j = i;
    j++;
    for(; j < layerinfo.size(); j++) {
      MATRIX_DATA width = MIN(datalibrary_->all_layers_info_[layerinfo[j].layer_num_]->rightx_,
                              datalibrary_->all_layers_info_[layerinfo[i].layer_num_]->rightx_) -
                          MAX(datalibrary_->all_layers_info_[layerinfo[j].layer_num_]->leftx_,
                              datalibrary_->all_layers_info_[layerinfo[i].layer_num_]->leftx_);
      ;
      MATRIX_DATA height = MIN(datalibrary_->all_layers_info_[layerinfo[j].layer_num_]->topy_,
                               datalibrary_->all_layers_info_[layerinfo[i].layer_num_]->topy_) -
                           MAX(datalibrary_->all_layers_info_[layerinfo[j].layer_num_]->bottomy_,
                               datalibrary_->all_layers_info_[layerinfo[i].layer_num_]->bottomy_);

      layerinfo[j].nregions_ += (int)((height * width - layerinfo[j].areas_removed_) / (powf(layerinfo[i].granularity_, 2.0)));
      layerinfo[j].areas_ -= width * height;
      layerinfo[j].areas_removed_ += width * height;
    } // for each layer info
  }

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->floorplan_layer_dyn_ =
        new DynamicArray<ModelUnit>((int)sqrt(layerinfo[i].nregions_), (int)sqrt(layerinfo[i].nregions_), datalibrary_);
  }
}

void ChipLayers::offset_layer() {
  switch(type_) {
  case DIE_TRANSISTOR_LAYER:
    ASSERT_ESESCTHERM(chip_floorplan_ != NULL,
                      "DIE_TRANSISTOR_LAYER must have an associated floorplan defined (currently set to -1)", "ChipLayers",
                      "offset_layer");
    break;
  case UCOOL_LAYER:
    I(0);
    break;
  }

  if(chip_floorplan_)
    chip_floorplan_->offset_floorplan(leftx_, bottomy_);
}

void ChipLayers::determine_layer_properties() {
  switch(type_) {
  case MAINBOARD_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_pcb_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built mainboard_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case PINS_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_pins_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built pins_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case PWB_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_pwb_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built pwb_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case FCPBGA_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_package_substrate_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built fcpbga_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case C4_UNDERFILL_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_c4underfill_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built c4_underfill_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case INTERCONNECT_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_interconnect_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built interconnect_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case DIE_TRANSISTOR_LAYER:
    ASSERT_ESESCTHERM(flp_num_ != -1, "floorplan value must not be equal to -1 for transistor layer", "sesctherm3Dlayer",
                      "determine_layer_properties");
    chip_floorplan_ = new ChipFloorplan(datalibrary_);
    chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);

    // Offset the datapoints of the chip floorplan to reflect the relative size of the heat_sink
    Material::calc_die_transistor_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built chip floorplan for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case BULK_SI_LAYER:
    if(flp_num_ != -1) {
      chip_floorplan_ = new ChipFloorplan(datalibrary_);
      chip_floorplan_->get_floorplan(datalibrary_->config_data_->get_chipflp_from_sescconf_, flp_num_);
    }
    Material::calc_silicon_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built builk_si_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case OIL_LAYER:
    Material::calc_oil_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built oil_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case AIR_LAYER:
    Material::calc_air_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built air_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case UCOOL_LAYER:
    I(0);
    break;
  case HEAT_SPREADER_LAYER:
    Material::calc_heat_spreader_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built heat_spreader_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case HEAT_SINK_LAYER:
    Material::calc_heat_sink_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built heat_sink_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  case HEAT_SINK_FINS_LAYER:
    Material::calc_heat_sink_fins_layer_properties(*this, datalibrary_);
#ifdef _ESESCTHERM_DEBUG
    std::cerr << "Built heat_sink_fins_layer for layer number [" << layer_number_ << "]" << std::endl;
#endif
    break;
  default:
    std::cerr << "FATAL: ChipLayers::determine_layer_properties -- undefined layer type [" << type_ << "]" << std::endl;
    exit(1);
  }
}

// this simply computes and returns the average temperature across the entire layer of the model
MATRIX_DATA ChipLayers::compute_average_temp() {
  MATRIX_DATA average_temp = 0;
  int         num_units    = 0;
  if(!layer_used_)
    return (-1);
  for(unsigned int y_itor = 0; y_itor < (*floorplan_layer_dyn_).nrows(); y_itor++)
    for(unsigned int x_itor = 0; x_itor < (*floorplan_layer_dyn_).ncols(); x_itor++) {
      if(!(*floorplan_layer_dyn_)[x_itor][y_itor].defined_)
        continue;
      if((*floorplan_layer_dyn_)[x_itor][y_itor].get_temperature() != NULL) {
        average_temp += *((*floorplan_layer_dyn_)[x_itor][y_itor].get_temperature());
        num_units++;
      }
    }
  average_temp /= num_units;
  return (average_temp);
}

// this returns an array which contains the averaged temperature value of the model units that correspond to each floorplan unit
// ex: model units 1,4,9 may be the model units contained within the floorplan unit zero, therefore the 0th element of the returned
// array will contain the averaged temperature values of those model units
std::vector<MATRIX_DATA> ChipLayers::compute_average_temps(int flp_layer) {
  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] doesnt exist", "ChipLayers", "computer_average_temps");
  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has undefined chip_floorplan_!", "ChipLayers", "computer_average_temps");
  std::vector<MATRIX_DATA> temperature_map;
  MATRIX_DATA              average_temp = 0;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(); i++) {
    ChipFloorplan_Unit &flp_unit = datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[i];

    if(flp_unit.located_units_[layer_number_].empty()) {
      ModelUnit::locate_model_units(layer_number_, flp_unit.located_units_[layer_number_], flp_unit.leftx_, flp_unit.bottomy_,
                                    flp_unit.leftx_ + flp_unit.width_, flp_unit.bottomy_ + flp_unit.height_, datalibrary_);
    }
    average_temp = 0;

#ifdef _ESESCTHERM_DEBUG
    //      cout << "[FLPUNIT:" << i << "][layer" << layer_number_ << "][located_units:" <<
    //      flp_unit.located_units_[layer_number_].size() << " ]" << std::endl;
#endif

    // compute average temperature for the units
    for(int j = 0; j < (int)flp_unit.located_units_[layer_number_].size(); j++) {
      if(flp_unit.located_units_[layer_number_][j]->get_temperature() != NULL)
        average_temp += *(flp_unit.located_units_[layer_number_][j]->get_temperature());
      else
        average_temp += 0;
    }
    average_temp /= flp_unit.located_units_[layer_number_].size();
    temperature_map.push_back(average_temp);
  }
  return (temperature_map);
}

// eka: compute_temps returns the non-average temperatures
std::vector<MATRIX_DATA> ChipLayers::compute_temps(int flp_layer) {
  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] doesnt exist", "ChipLayers", "computer_average_temps");
  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has undefined chip_floorplan_!", "ChipLayers", "computer_average_temps");
  std::vector<MATRIX_DATA> temperature_map;
  // MATRIX_DATA average_temp = 0;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(); i++) {
    ChipFloorplan_Unit &flp_unit = datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[i];

    if(flp_unit.located_units_[layer_number_].empty()) {
      ModelUnit::locate_model_units(layer_number_, flp_unit.located_units_[layer_number_], flp_unit.leftx_, flp_unit.bottomy_,
                                    flp_unit.leftx_ + flp_unit.width_, flp_unit.bottomy_ + flp_unit.height_, datalibrary_);
    }
    // average_temp = 0;

#ifdef _ESESCTHERM_DEBUG
    //      cout << "[FLPUNIT:" << i << "][layer" << layer_number_ << "][located_units:" <<
    //      flp_unit.located_units_[layer_number_].size() << " ]" << std::endl;
#endif

    // compute average temperature for the units
    for(int j = 0; j < (int)flp_unit.located_units_[layer_number_].size(); j++) {
      if(flp_unit.located_units_[layer_number_][j]->get_temperature() != NULL) {
        temperature_map.push_back(*(flp_unit.located_units_[layer_number_][j]->get_temperature()));
        //        std::cout<<"FLP "<< i <<"unit "<<j<<std::endl;
      }
    }
  }
  return (temperature_map);
}

std::vector<MATRIX_DATA> ChipLayers::compute_average_powers(int flp_layer) {
  std::vector<MATRIX_DATA> power_map;
  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] doesnt exist", "ChipLayers", "compute_average_powers");
  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has undefined chip_floorplan_!", "ChipLayers", "compute_average_powers");
  power_map_.clear();
  MATRIX_DATA average_temp = 0;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(); i++) {
    ChipFloorplan_Unit &flp_unit = datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[i];
    if(flp_unit.located_units_[layer_number_].empty()) {
      ModelUnit::locate_model_units(layer_number_, flp_unit.located_units_[layer_number_], flp_unit.leftx_, flp_unit.bottomy_,
                                    flp_unit.leftx_ + flp_unit.width_, flp_unit.bottomy_ + flp_unit.height_, datalibrary_);
    }
    average_temp = 0;
    // compute average temperature for the units
    for(int j = 0; j < (int)flp_unit.located_units_[layer_number_].size(); j++) {
      if(flp_unit.located_units_[layer_number_][j]->get_temperature() != NULL)
        average_temp += *(flp_unit.located_units_[layer_number_][j]->get_temperature());
      else
        average_temp += 0;
    }
    average_temp /= flp_unit.located_units_[layer_number_].size();
    power_map.push_back(average_temp);
  }
  return (power_map);
}

// this computes average temperatures and powers across all of the layers, returns in celcius
DynamicArray<ModelUnit> ChipLayers::compute_layer_averages(std::vector<int> &layers, DataLibrary *datalibrary_) {
  DynamicArray<ModelUnit> temp_layer_dyn(datalibrary_);
  int                     biggest_layer_num = 0;
  MATRIX_DATA             max_x             = -1.0;
  MATRIX_DATA             max_y             = -1.0;
  for(unsigned int i = 0; i < layers.size(); i++) {
    int layer_num = layers[i];
    // Note: we assume that there will ALWAYS be a large layer than encloses the others
    if(datalibrary_->all_layers_info_[layer_num]->height_ > max_y || datalibrary_->all_layers_info_[layer_num]->width_ > max_x) {
      max_x             = datalibrary_->all_layers_info_[layer_num]->width_;
      max_y             = datalibrary_->all_layers_info_[layer_num]->height_;
      biggest_layer_num = layer_num;
    }
  }
  // copy the largest layer to temp_layer_dyn
  for(unsigned int itor_y = 0; itor_y < datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_->nrows();
      itor_y++) {
    for(unsigned int itor_x = 0; itor_x < datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_->ncols();
        itor_x++) {
      temp_layer_dyn[itor_x][itor_y] = (*datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_)[itor_x][itor_y];
    }
  }

  // go through the temp_layer_dyn and compute the average over all the relevent layers
  for(unsigned int itor_y = 0; itor_y < temp_layer_dyn.nrows(); itor_y++)
    for(unsigned int itor_x = 0; itor_x < temp_layer_dyn.ncols(); itor_x++) {
      // set averages to zero
      temp_layer_dyn[itor_x][itor_y].energy_data_        = 0;
      temp_layer_dyn[itor_x][itor_y].sample_temperature_ = 0;
      for(unsigned int i = 0; i < layers.size(); i++) {
        int layer_num = layers[i];
        temp_layer_dyn[itor_x][itor_y].energy_data_ +=
            (ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_, temp_layer_dyn[itor_x][itor_y].bottomy_, layer_num,
                                        datalibrary_))
                ->energy_data_;
        SUElement_t *temperature_ptr =
            (ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_, temp_layer_dyn[itor_x][itor_y].bottomy_, layer_num,
                                        datalibrary_))
                ->get_temperature();
        if(temperature_ptr != NULL)
          temp_layer_dyn[itor_x][itor_y].sample_temperature_ += *temperature_ptr;
      } // for each layer
      temp_layer_dyn[itor_x][itor_y].energy_data_ /= (layers.size());
      temp_layer_dyn[itor_x][itor_y].sample_temperature_ /= (layers.size());
    }
  return (temp_layer_dyn);
}

// this computes the difference in power and temperature between two layers, returns in celcius
DynamicArray<ModelUnit> ChipLayers::compute_layer_diff(int layer1, int layer2, DataLibrary *datalibrary_) {
  DynamicArray<ModelUnit> temp_layer_dyn(datalibrary_);
  int                     biggest_layer_num = 0;
  // Note: we assume that there will ALWAYS be a large layer than encloses the others
  if(datalibrary_->all_layers_info_[layer1]->height_ > datalibrary_->all_layers_info_[layer2]->height_ ||
     datalibrary_->all_layers_info_[layer1]->width_ > datalibrary_->all_layers_info_[layer2]->width_) {
    biggest_layer_num = layer1;
  } else {
    biggest_layer_num = layer2;

    // copy the largest layer to temp_layer_dyn
    for(unsigned int itor_y = 0; itor_y < datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_->nrows();
        itor_y++) {
      for(unsigned int itor_x = 0; itor_x < datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_->ncols();
          itor_x++) {
        temp_layer_dyn[itor_x][itor_y] = (*datalibrary_->all_layers_info_[biggest_layer_num]->floorplan_layer_dyn_)[itor_x][itor_y];
      }
    }

    // go through the temp_layer_dyn and compute the absolute difference between the two layers
    for(unsigned int itor_y = 0; itor_y < temp_layer_dyn.nrows(); itor_y++)
      for(unsigned int itor_x = 0; itor_x < temp_layer_dyn.ncols(); itor_x++) {
        // set averages to zero
        temp_layer_dyn[itor_x][itor_y].energy_data_        = 0;
        temp_layer_dyn[itor_x][itor_y].sample_temperature_ = 0;

        temp_layer_dyn[itor_x][itor_y].energy_data_ =
            ABS_DIF(ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_, temp_layer_dyn[itor_x][itor_y].bottomy_,
                                               layer1, datalibrary_)
                        ->energy_data_,
                    ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_, temp_layer_dyn[itor_x][itor_y].bottomy_,
                                               layer2, datalibrary_)
                        ->energy_data_);
        SUElement_t *temperature_ptr1 = ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_,
                                                                   temp_layer_dyn[itor_x][itor_y].bottomy_, layer1, datalibrary_)
                                            ->get_temperature();
        SUElement_t *temperature_ptr2 = ModelUnit::find_model_unit(temp_layer_dyn[itor_x][itor_y].leftx_,
                                                                   temp_layer_dyn[itor_x][itor_y].bottomy_, layer2, datalibrary_)
                                            ->get_temperature();

        MATRIX_DATA temp1                                  = temperature_ptr1 ? (*temperature_ptr1) : 0.0;
        MATRIX_DATA temp2                                  = temperature_ptr2 ? (*temperature_ptr2) : 0.0;
        temp_layer_dyn[itor_x][itor_y].sample_temperature_ = ABS_DIF(temp1, temp2);
      }
  }
  return (temp_layer_dyn);
}

void ChipLayers::accumulate_data_for_sample() {
  DynamicArray<ModelUnit> &dyn_array = *floorplan_layer_dyn_;
  for(unsigned int y_itor = 0; y_itor < dyn_array.nrows(); y_itor++)
    for(unsigned int x_itor = 0; x_itor < dyn_array.ncols(); x_itor++) {
      if(!dyn_array[x_itor][y_itor].defined_)
        continue;
      running_average_temperature_map_[x_itor][y_itor] += *dyn_array[x_itor][y_itor].get_temperature() * datalibrary_->timestep_;
      running_average_energy_map_[x_itor][y_itor] += dyn_array[x_itor][y_itor].energy_data_ * datalibrary_->timestep_;
      running_max_temperature_map_[x_itor][y_itor] =
          MAX(running_max_temperature_map_[x_itor][y_itor], *dyn_array[x_itor][y_itor].get_temperature());
      running_min_temperature_map_[x_itor][y_itor] =
          MIN(running_min_temperature_map_[x_itor][y_itor], *dyn_array[x_itor][y_itor].get_temperature());
      running_max_energy_map_[x_itor][y_itor] =
          MAX(running_max_energy_map_[x_itor][y_itor], dyn_array[x_itor][y_itor].energy_data_);
      running_min_energy_map_[x_itor][y_itor] =
          MIN(running_min_energy_map_[x_itor][y_itor], dyn_array[x_itor][y_itor].energy_data_);
    }
}

std::vector<MATRIX_DATA> ChipLayers::compute_energy() {
  I(chip_floorplan_ != NULL);
  std::vector<MATRIX_DATA> power_map;
  power_map.clear();
  for(int i = 0; i < (int)chip_floorplan_->flp_units_.size(); i++) {
    power_map.push_back(chip_floorplan_->flp_units_[i].get_power());
  }
  return (power_map);
}

/*
//Note: these offsets are only useful if we assume that the regions are overlapping
//If there is no overlap, these won't work
void ChipLayers::compute_offsets(int layer_num, DataLibrary* datalibrary_)
{
MATRIX_DATA x_itor=0;
MATRIX_DATA y_itor=0;
MATRIX_DATA x_offset=0;
MATRIX_DATA y_offset=0;

ChipLayers& layer = datalibrary_->all_layers_info_(layer_num);

DynamicArray<ModelUnit>& dyn_array = layer.floorplan_layer_dyn_;

//check to ensure layer number is in range
if(layer.layer_number_ < 0 || layer.layer_number_ > datalibrary_->all_layers_info_.size()-1){
cerr << "ChipLayers::compute_offsets => layer [" << layernum <<  "].layer_number_ is out of range" << std::endl;
exit(1);
}

//if we are on the bottom layer, no offset below
if(layer.layer_number_ == 0){
ChipLayers& layer_above = datalibrary_->all_layers_info_[layer_num+1];
DynamicArray<ModelUnit>& dyn_array_above = layer_above.floorplan_layer_dyn_;


layer.offset_layer_below_y_=0;
layer.offset_layer_below_x_=0;



//FIND OFFSET ABOVE X
x_itor=0;
//if the left datapoint of the above layer is smaller than the current layer, then specify pcerritive offset
if(LT(layer_above.leftx_, layer.leftx_)){
while(!EQ(dyn_array_above[x_itor][0].leftx_, dyn_array[0][0].leftx_)){
x_itor++;
}
layer.offset_layer_above_x_=x_itor;
}
x_itor=0;
//if the left datapoint of the above layer is bigger than the current layer, then specify negative offset
else if(GT(layer_above.leftx_, layer.leftx_)){
while(!EQ(dyn_array[x_itor][0].leftx_, dyn_array_above[0][0].leftx_)){
x_itor++;
}
layer.offset_layer_above_x_=-1*x_itor;
}
else
layer.offset_layer_above_x_=0;
y_itor=0;


//FIND OFFSET ABOVE Y
//if the bottom datapoint of the above layer is smaller than the current layer, then specify pcerritive offset
if(LT(layer_above.bottomy_, layer.bottomy_)){
while(!EQ(dyn_array_above[0][y_itor].bottomy_, dyn_array[0][0].bottomy_)){
y_itor++;
}
layer.offset_layer_above_y_=y_itor;
}
y_itor=0;
//if the bottom datapoint of the above layer is bigger than the current layer, then specify negative offset
else if(GT(layer_above.bottomy_, layer.bottomy_)){
while(!EQ(dyn_array[0][y_itor].bottomy_, dyn_array_above[0][0].bottomy_)){
y_itor++;
}
layer.offset_layer_above_y_=-1*y_itor;
}
else
layer.offset_layer_above_y_=0;

}
else if(layer.layer_number == datalibrary_->all_layers_info_.size()-1){
  ChipLayers& layer_above = datalibrary_->all_layers_info_[layer_num+1];
  DynamicArray<ModelUnit>& dyn_array_above = layer_above.floorplan_layer_dyn_;

  layer.offset_layer_above_y_=0;
  layer.offset_layer_above_x_=0;


  //FIND OFFSET BELOW X
  x_itor=0;
  //if the left datapoint of the below layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_below.leftx_, layer.leftx_)){
    while(!EQ(dyn_array_below[x_itor][0].leftx_, dyn_array[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_below_x_=x_itor;
  }
  x_itor=0;
  //if the left datapoint of the below layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_below.leftx_, layer.leftx_)){
    while(!EQ(dyn_array[x_itor][0].leftx_, dyn_array_below[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_below_x_=-1*x_itor;
  }
  else
    layer.offset_layer_below_x_=0;
  y_itor=0;


  //FIND OFFSET BELOW Y
  //if the bottom datapoint of the below layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_below.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array_below[0][y_itor].bottomy_, dyn_array[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_below_y_=y_itor;
  }
  y_itor=0;
  //if the bottom datapoint of the below layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_below.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array[0][y_itor].bottomy_, dyn_array_below[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_below_y_=-1*y_itor;
  }
  else
    layer.offset_layer_below_y_=0;
}
else{
  ChipLayers& layer_above = datalibrary_->all_layers_info_[layer_num+1];
  DynamicArray<ModelUnit>& dyn_array_above = layer_above.floorplan_layer_dyn_;
  ChipLayers& layer_above = datalibrary_->all_layers_info_[layer_num+1];
  DynamicArray<ModelUnit>& dyn_array_above = layer_above.floorplan_layer_dyn_;

  //FIND OFFSET ABOVE X
  x_itor=0;
  //if the left datapoint of the above layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_above.leftx_, layer.leftx_)){
    while(!EQ(dyn_array_above[x_itor][0].leftx_, dyn_array[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_above_x_=x_itor;
  }
  x_itor=0;
  //if the left datapoint of the above layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_above.leftx_, layer.leftx_)){
    while(!EQ(dyn_array[x_itor][0].leftx_, dyn_array_above[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_above_x_=-1*x_itor;
  }
  else
    layer.offset_layer_above_x_=0;
  y_itor=0;


  //FIND OFFSET ABOVE Y
  //if the bottom datapoint of the above layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_above.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array_above[0][y_itor].bottomy_, dyn_array[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_above_y_=y_itor;
  }
  y_itor=0;
  //if the bottom datapoint of the above layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_above.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array[0][y_itor].bottomy_, dyn_array_above[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_above_y_=-1*y_itor;
  }
  else
    layer.offset_layer_above_y_=0;


  //FIND OFFSET BELOW X
  x_itor=0;
  //if the left datapoint of the below layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_below.leftx_, layer.leftx_)){
    while(!EQ(dyn_array_below[x_itor][0].leftx_, dyn_array[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_below_x_=x_itor;
  }
  x_itor=0;
  //if the left datapoint of the below layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_below.leftx_, layer.leftx_)){
    while(!EQ(dyn_array[x_itor][0].leftx_, dyn_array_below[0][0].leftx_)){
      x_itor++;
    }
    layer.offset_layer_below_x_=-1*x_itor;
  }
  else
    layer.offset_layer_below_x_=0;
  y_itor=0;


  //FIND OFFSET BELOW Y
  //if the bottom datapoint of the below layer is smaller than the current layer, then specify pcerritive offset
  if(LT(layer_below.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array_below[0][y_itor].bottomy_, dyn_array[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_below_y_=y_itor;
  }
  y_itor=0;
  //if the bottom datapoint of the below layer is bigger than the current layer, then specify negative offset
  else if(GT(layer_below.bottomy_, layer.bottomy_)){
    while(!EQ(dyn_array[0][y_itor].bottomy_, dyn_array_below[0][0].bottomy_)){
      y_itor++;
    }
    layer.offset_layer_below_y_=-1*y_itor;
  }
  else
    layer.offset_layer_below_y_=0;
}
}
*/
