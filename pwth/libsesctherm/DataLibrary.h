// Contributed by Ian Lee
//                Joseph Nayfach - Battilana
//                Jose Renau
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
File name:      DataLibrary.h
Classes:        DataLibrary
********************************************************************************/

#ifndef DATA_LIBRARY_H
#define DATA_LIBRARY_H

#include <fstream>
#include <vector>

#include "ChipMaterial.h"
#include "Utilities.h"
#include "sesctherm3Ddefine.h"

// This is the main class that allows access to all of the other data elements
// This is done to ensure that the data is properly maintained, and to ensure
// That any new class may be easily able to access any of the other data elements
// Important: while all of the other classes may be able to access these elements,
// none have been declared global
class ConfigData;
class DataLibrary;
class layerinfo;
class RegressionLine;
class sescthermRK4Matrix;
class ThermGraphics;

class DataLibrary {
public:
  DataLibrary(bool useRK4);
  ~DataLibrary();

  void solve_matrix(MATRIX_DATA timestep, std::vector<ModelUnit *> &matrix_model_units);

  inline int               find_unsolved_matrix_row_index(int layer, int x_coord, int y_coord);
  void                     store_pointer(int x_coord, int y_coord, SUElement_t *&pointer_location, bool set_pointer);
  void                     initialize_unsolved_matrix();
  void                     initialize_unsolved_matrix_row(int y_itor_global, int layer, int x_itor, int y_itor, bool set_pointer);
  DynamicArray<ModelUnit> &get_dyn_array(int layer);

  void set_timestep(double step);

  // Data
  MATRIX_DATA              timestep_;            // this is the global timestep
  MATRIX_DATA              time_;                // this is the global time of the model
  MATRIX_DATA              cur_sample_end_time_; // this is the endtime of the current sample (to know where it ends)
  const static MATRIX_DATA technology_parameters_[91][5];
  static int               tech_metal_index(int layer);
  std::ifstream            if_flpfile_;
  std::ifstream            if_cfgfile_;
  std::ofstream            of_outfile_;

  // Configuration data
  ConfigData *config_data_; // changed "chip_configuration" to "ConfigData" to be consistent

  // Graphics Data
  ThermGraphics *graphics_;

  // Reliability data
  // sesctherm_reliability* reliability_data_;

  // Performance data
  // performance_data* performance_data_;

  // Interconnect Model
  // sesctherm3Dinterconnect* interconnect_model_;

  // Materials Library
  Material_List layer_materials_;

  // this array holds the pointers to the dynamic floorplans
  // the allocation of these dynamic floorplans depends upon the defined layers
  // Layer Models
  // This stores ALL the relevent information for that particular model layer
  // depending upon the layer type, different information may be stored
  std::vector<ChipLayers *> all_layers_info_;

  // this is the x-distance between the first chip-layer ModelUnit and first heat-spreader ModelUnit
  unsigned int chip_to_spreader_offset_x_;

  // this is the y-distance between the first chip-layer ModelUnit and first heat-spreader ModelUnit
  unsigned int chip_to_spreader_offset_y_;

  // this is the x-distance between the first heat-spreader ModelUnit and first heat-sink ModelUnit
  unsigned int spreader_to_sink_offset_x_;

  // this is the y-distance between the first heat-spreader ModelUnit and first heat-sink ModelUnit
  unsigned int spreader_to_sink_offset_y_;

  // stores the solved temperatures for each iteration (each model unit links into this)
  SUElement_t *temperature_matrix_;

  // stores the previously solved temperatures for each iteration (each model unit links into this)
  SUElement_t *previous_temperature_matrix_;
  int          temperature_matrix_size_;

  int num_locktemp_rows_;

  // These are different kinds of solver data.
  sescthermRK4Matrix *_rk4Matrix; // dense rk4 matrix

  std::ofstream of_outfile_floorplan_; // the SVG to draw the floorplan
  // Note the floorplan file can have a top-down view of a single floorplan,
  // or it can have both the chip and the ucooler floorplan
  // or it can output the logical units from each of the layers
  // ofstream of_outfile_map_;		//the SVG to draw the thermal/power map

  bool accumulate_rc_;
  bool use_rc_;
};

#endif
