//  Contributed by Ian Lee
//                 Joseph Nayfach - Battilana
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
File name:      ThermModel.h
Classes:        ThermModel
********************************************************************************/

#ifndef THERM_MODEL_H
#define THERM_MODEL_H

#include "sescthermMacro.h"

class ThermModel {
private:
  DynamicArray<ModelUnit> &get_floorplan_layer(int layer);
  void                     print_unsolved_model();
  void                     check_matrices();
  void                     print_unsolved_model_row(int row);
  void                     set_initial_temperatures();
  void                     create_solution_matrices();
  void                     find_unused_model_units();

  void         compute_model_units(bool calc_solution_only, MATRIX_DATA time_increment, bool accumulate_rc, bool use_rc);
  void         partition_floorplans();
  MATRIX_DATA  get_governing_equation(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height, int layer);
  unsigned int determine_heat_transfer_methods(ModelUnit &ModelUnit_source, ModelUnit &ModelUnit_dest);
  void         store_model_unit(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height, int x_itor, int y_itor,
                                int layer);
  void         generate_pins_layer(MATRIX_DATA width, MATRIX_DATA height);
  void         recompute_material_properties();
  void         recompute_material_properties(int layer);
  void         run_sampler(MATRIX_DATA timestep, bool use_rc, bool recompute_material_properties);

public:
  ThermModel(); // eka, to have static declaration
  ThermModel(const char *flp_filename, const char *ucool_filename, bool useRK4, const char *output_filename = "sesctherm.out",
             bool get_chipflp_from_sescconf = true, bool get_ucoolflp_from_sescconf = false);
  ~ThermModel();
  // eka, to initialize when declared with static constructor
  void ThermModelInit(const char *flp_filename, const char *ucool_filename, bool useRK4,
                      const char *output_filename = "sesctherm.out", bool get_chipflp_from_sescconf = true,
                      bool get_ucoolflp_from_sescconf = false);

  void                     compute(MATRIX_DATA timestep, bool recompute_material_properties);
  void                     compute(bool recompute_material_properties);
  void                     compute_rc_start();
  void                     compute_rc_stop();
  void                     fast_forward(MATRIX_DATA timestep);
  void                     set_time(MATRIX_DATA timestep);
  void                     determine_gridsize();
  void                     determine_timestep();
  void                     print_graphics();
  void                     accumulate_sample_data();
  void                     compute_sample();
  void                     set_power_flp(int flp_pos, int layer, MATRIX_DATA power);
  void                     set_power_layer(int layer, std::vector<MATRIX_DATA> &power_map);
  void                     set_temperature_flp(int flp_pos, int flp_layer, int source_layer, MATRIX_DATA temperature);
  void                     set_temperature_layer(int flp_layer, int source_layer, std::vector<MATRIX_DATA> &temperature_map);
  void                     set_temperature_layer(int source_layer, MATRIX_DATA temperature);
  void                     enable_temp_locking(int layer, MATRIX_DATA temperature);
  void                     disable_temp_locking(int layer);
  MATRIX_DATA              get_temperature_layer_average(int source_layer);
  std::vector<MATRIX_DATA> get_temperature_layer_map(int flp_layer, int source_layer);
  std::vector<MATRIX_DATA> get_temperature_layer_noavg(int flp_layer, int source_layer);
  MATRIX_DATA              get_temperature_flp(int flp_pos, int flp_layer, int source_layer);
  // vector<MATRIX_DATA> get_power_layer(int flp_layer);
  // MATRIX_DATA get_power_flp(int flp_layer, int flp_pos);
  void         set_ambient_temp(MATRIX_DATA temp);
  void         set_initial_temp(MATRIX_DATA temp);
  MATRIX_DATA  get_time();
  MATRIX_DATA  get_max_timestep();
  MATRIX_DATA  get_recommended_timestep();
  unsigned int chip_flp_count(int layer);
  std::string  get_chip_flp_name(int flp_pos, int layer);
  void         set_max_power_flp(int flp_pos, int layer, MATRIX_DATA power);
  void         dump_temp_data(); // FIXME: rename dump
  void         run_model(MATRIX_DATA timestep, bool accumulate_rc, bool use_rc, bool recompute_material_properties);
  int          get_modelunit_count(int layer);
  int          get_modelunit_rows(int layer);
  int          get_modelunit_cols(int layer);

  void dump_flp_temp_data();    // TODO
  void print_flp_temp_labels(); // TODO
  void warmup_chip();           // TODO
  void dump_3d_graph();         // TODO
  void print_model_units();

  DynamicArray<ModelUnit> &get_dyn_array(int layer) {
    return datalibrary_->get_dyn_array(layer);
  }

  // this contains all of the data
  DataLibrary *datalibrary_;

  MATRIX_DATA time_;
  MATRIX_DATA time_step_;
  MATRIX_DATA cur_sample_end_time_;

  std::vector<ModelUnit *> _matrix_model_units; // one to one link, i-th entry is i-th used unit
  std::vector<MATRIX_DATA> leftx_;              // holds compiled list of leftx_values
  std::vector<MATRIX_DATA> bottomy_;            // holds compiled list of bottomy_values
  bool                     _useRK4;             // whether RK4 is to be used with complete coefficient matrices
};

#endif
