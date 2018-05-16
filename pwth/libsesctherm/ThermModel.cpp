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
File name:      ThermModel.cpp
Description:    This is the main portion of the model. All model initialization,
                including the reading of configuration files occurs in the
                constructor. See the thesis for additional information.
********************************************************************************/

#include <algorithm>
#include <complex>
#include <iostream>

#include "sesctherm3Ddefine.h"
#include "sescthermMacro.h"

#include "ChipFloorplan.h"
#include "ChipLayers.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ThermGraphics.h"
#include "ThermModel.h"
#include "Utilities.h"

#include "nanassert.h"

static inline double double_pow(double a, double b) {
  return pow(a, b);
}

#include "RK4Matrix.h"

// Destructor for the 3D model for thermal simulation
ThermModel::~ThermModel() {
  bool get_chipflp_from_sescconf = datalibrary_->config_data_->get_chipflp_from_sescconf_;

  // open chip floorplan file
  if(!get_chipflp_from_sescconf) {
    datalibrary_->if_flpfile_.close();
  }
  datalibrary_->if_cfgfile_.close();
  datalibrary_->of_outfile_.close();
  // FIXME: One of these delete/free statements appears to be trying to deallocate a memory block
  //              that has not been allocated
  /*
     if(datalibrary_) {

     if(datalibrary_->temperature_matrix_ && *datalibrary_->temperature_matrix_)
     delete[] datalibrary_->temperature_matrix_;
     if(datalibrary_->previous_temperature_matrix_)
     delete[] datalibrary_->previous_temperature_matrix_;
     if(datalibrary_->non_zero_vals_)
     delete[] datalibrary_->non_zero_vals_;
     if(datalibrary_->row_indices_)
     delete[] datalibrary_->row_indices_;
     if(datalibrary_->col_begin_indices_)
     delete[] datalibrary_->col_begin_indices_;

     if(datalibrary_->unsolved_matrix_dyn_)
     delete datalibrary_->unsolved_matrix_dyn_;

     }
   */
}
// eka, static constructor
ThermModel::ThermModel() {
}

// eka, to initialize after declaration, when declared
// with static constructor
void ThermModel::ThermModelInit(const char *flp_filename, const char *ucool_flp_filename, bool useRK4, const char *outfile_filename,
                                bool get_chipflp_from_sescconf, bool get_ucoolflp_from_sescconf) {
  _useRK4 = useRK4; // With this set, a completely different flow is followed for solutions

  datalibrary_         = new DataLibrary(useRK4);
  time_                = 0;
  time_step_           = 0;
  cur_sample_end_time_ = 0;

  // store settings to datalibrary
  datalibrary_->config_data_->get_chipflp_from_sescconf_ = get_chipflp_from_sescconf;

  // open chip floorplan file
  if(!get_chipflp_from_sescconf) {
    datalibrary_->if_flpfile_.open(flp_filename, std::ifstream::in);

    if(!datalibrary_->if_flpfile_) {
      Utilities::fatal("Cannot open floorplan file\n");
    }
  }
  // create output file
  if(outfile_filename == NULL)
    outfile_filename = "sesctherm.out";
  datalibrary_->of_outfile_.open(outfile_filename, std::ifstream::out);
  if(!datalibrary_->of_outfile_) {
    Utilities::fatal("Cannot create output file\n");
  }

  // open the configuration file, store the information to ConfigData structure
  // this data will always be gotten from sesc.conf
  datalibrary_->config_data_->get_config_data();

  std::cerr << "Initial Temperature " << datalibrary_->config_data_->init_temp_ << std::endl;
#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << std::endl;
  //      std::cerr << *(datalibrary_->config_data_);
  //      std::cerr << std::endl;
#endif

  // determine all the properties for the given layer, load chip floorplans floorplans
  // acquire ALL data necessary for each layer (material properties computation etc etc)
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->determine_layer_properties();
  }

  // Allocate space for the layers dynamic arrays
  // Also determine the leftx,rightx,bottomy,topy coordinates for the layers
  // This must be done after the layer widths and height have been determined
  // Hence, this must come after the layer properties have been determined
  ChipLayers::allocate_layers(datalibrary_);

  // Now that leftx/bottomy datapoints have been determined for each layer, offset the flp units accordingly
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->offset_layer();
  }

#ifdef _ESESCTHERM_DEBUG
  std::cerr << "******** Layer Properties Determined, Printing Basic Layer Information: ******** " << std::endl;

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->print(false); // don't print detailed information (the dynamic arrays)
  }
#endif

  // partition the floorplans into different sections
  partition_floorplans();

  find_unused_model_units(); // determine the number of unused model-units in the virtual and heat_sink_fins layers

  // allocate SUElement_t* temperature_matrix and SUElement_t* previous_temperature vectors
  create_solution_matrices();

  // then associate the *solution_data and *previous_solution_data links to elements in these arrays
  set_initial_temperatures();
  recompute_material_properties();

  // this will set the coefficient pointers in the model units to point to the correct location in the unsolved matrix
  datalibrary_->initialize_unsolved_matrix();

  compute_model_units(false, datalibrary_->config_data_->default_time_increment_, false,
                      false); // calculate all the data for the model units.

#ifdef _ESESCTHERM_DEBUG
  std::cerr
      << "******** Thermal Model Generated, Printing Lumped Resistance, Capacitance and RC-time constant for each layer: ********"
      << std::endl;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->print_lumped_metrics();
  }
#endif

  // and store that data to unsolved matrix
#ifdef _ESESCTHERM_DEBUG
  /*
     std::cerr << "******** Layers Have Been Partitioned, Printing Dynamic Layers: ********" << std::endl;
     for(int i=0;i<(int)datalibrary_->all_layers_info_.size();i++)
     if(datalibrary_->all_layers_info_[i]->layer_used_)
     ModelUnit::print_dyn_layer(i, datalibrary_);
   */

  // NOTE: in order for the check_matrices feature to work, temp-locking must be set to 1C (and actually needs to be completely
  // turned off for part of the checking)
  check_matrices();
#endif
}

// Constructor for the 3D model for thermal simulation
ThermModel::ThermModel(const char *flp_filename, const char *ucool_flp_filename, bool useRK4, const char *outfile_filename,
                       bool get_chipflp_from_sescconf, bool get_ucoolflp_from_sescconf) {
  _useRK4 = useRK4; // With this set, a completely different flow is followed for solutions

  datalibrary_         = new DataLibrary(useRK4);
  time_                = 0;
  time_step_           = 0;
  cur_sample_end_time_ = 0;

  // store settings to datalibrary
  datalibrary_->config_data_->get_chipflp_from_sescconf_ = get_chipflp_from_sescconf;

  // open chip floorplan file
  if(!get_chipflp_from_sescconf) {
    datalibrary_->if_flpfile_.open(flp_filename, std::ifstream::in);

    if(!datalibrary_->if_flpfile_) {
      Utilities::fatal("Cannot open floorplan file\n");
    }
  }
  // create output file
  if(outfile_filename == NULL)
    outfile_filename = "sesctherm.out";
  datalibrary_->of_outfile_.open(outfile_filename, std::ifstream::out);
  if(!datalibrary_->of_outfile_) {
    Utilities::fatal("Cannot create output file\n");
  }

  // open the configuration file, store the information to ConfigData structure
  // this data will always be gotten from sesc.conf
  datalibrary_->config_data_->get_config_data();

  std::cerr << "Initial Temperature " << datalibrary_->config_data_->init_temp_;
#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << std::endl;
  //      std::cerr << *(datalibrary_->config_data_);
  //      std::cerr << std::endl;
#endif

  // determine all the properties for the given layer, load chip floorplans floorplans
  // acquire ALL data necessary for each layer (material properties computation etc etc)
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->determine_layer_properties();
  }

  // Allocate space for the layers dynamic arrays
  // Also determine the leftx,rightx,bottomy,topy coordinates for the layers
  // This must be done after the layer widths and height have been determined
  // Hence, this must come after the layer properties have been determined
  ChipLayers::allocate_layers(datalibrary_);

  // Now that leftx/bottomy datapoints have been determined for each layer, offset the flp units accordingly
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->offset_layer();
  }

  std::cerr << "******** Layer Properties Determined, Printing Basic Layer Information: ******** " << std::endl;

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->print(false); // don't print detailed information (the dynamic arrays)
  }

  // partition the floorplans into different sections
  partition_floorplans();

  find_unused_model_units(); // determine the number of unused model-units in the virtual and heat_sink_fins layers

  // allocate SUElement_t* temperature_matrix and SUElement_t* previous_temperature vectors
  create_solution_matrices();

  // then associate the *solution_data and *previous_solution_data links to elements in these arrays
  set_initial_temperatures();
  recompute_material_properties();

  // this will set the coefficient pointers in the model units to point to the correct location in the unsolved matrix
  datalibrary_->initialize_unsolved_matrix();

  compute_model_units(false, datalibrary_->config_data_->default_time_increment_, false,
                      false); // calculate all the data for the model units.

  std::cerr
      << "******** Thermal Model Generated, Printing Lumped Resistance, Capacitance and RC-time constant for each layer: ********"
      << std::endl;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    datalibrary_->all_layers_info_[i]->print_lumped_metrics();
  }

  // and store that data to unsolved matrix
#ifdef _ESESCTHERM_DEBUG
  /*
     std::cerr << "******** Layers Have Been Partitioned, Printing Dynamic Layers: ********" << std::endl;
     for(int i=0;i<(int)datalibrary_->all_layers_info_.size();i++)
     if(datalibrary_->all_layers_info_[i]->layer_used_)
     ModelUnit::print_dyn_layer(i, datalibrary_);
   */

  // NOTE: in order for the check_matrices feature to work, temp-locking must be set to 1C (and actually needs to be completely
  // turned off for part of the checking)
  check_matrices();
#endif
}

// This allocates the solution_data and previous_solution_data matrices
// Then it links the *solution_data and *previous_solution_data pointers in each model unit to a location in each array
// These arrays will be swapped on each iteration (previous_solution_data=solution_data)
void ThermModel::create_solution_matrices() {
  // make sure that we haven't already allocated these matrices
  if(datalibrary_->temperature_matrix_ != NULL || datalibrary_->previous_temperature_matrix_ != NULL)
    return;

  int size = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++) {
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++)
          if(get_dyn_array(layer)[x_itor][y_itor].defined_)
            size++;
    }
  }

  datalibrary_->temperature_matrix_          = new SUElement_t[size];
  datalibrary_->previous_temperature_matrix_ = new SUElement_t[size];

  // Note: need to create last entry (where last entry is equal to T_ambient, as T_inf must be equal to T_ambient)
  int y_itor_global = 0;
  _matrix_model_units.clear();
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_) {
            munit.temperature_index_ = -1;
            continue; // skip if unit is not defined
          }

          _matrix_model_units.push_back(&munit);
          munit.temperature_index_    = y_itor_global;
          munit.temperature_          = &(datalibrary_->temperature_matrix_);
          munit.previous_temperature_ = &(datalibrary_->previous_temperature_matrix_);
          y_itor_global++;
        }
    }

  datalibrary_->temperature_matrix_size_ = y_itor_global;
}

// This can ONLY be called once the temperature and previous_temperature matrices have been allocated
// Since the pointers to matrix elems are used, it works both with RK4 or the original Super LU solver.
void ThermModel::set_initial_temperatures() {
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++) {
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_ != false) {
            ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
            if(datalibrary_->all_layers_info_[layer]->temp_locking_enabled_) {
              munit.lock_temp_ = datalibrary_->all_layers_info_[layer]->lock_temp_;
            }
            *(munit.get_previous_temperature()) = datalibrary_->config_data_->init_temp_;
            *(munit.get_temperature())          = datalibrary_->config_data_->init_temp_;
          }
        }
    }
  }
}

// this ensures that all of the equations were properly stored in memory
// this is done by setting the heat generation to zero in all units, and setting the previous temperatures to 1.
// this means that no heat is moving between units and the temperature at Tmno_{p+1}==Tmno_{p}
// If there is no error, then both sides of the equation will be equal
void ThermModel::check_matrices() {
  // bool error = false;
  char input_char = 0;
  std::cerr << "check_matrices() => Would you like to print the equations for each temperature node on each layer? [y/N]"
            << std::endl;
  std::cin >> input_char;
  if(input_char == 'y') {
    for(unsigned int i = 0; i < datalibrary_->all_layers_info_.size(); i++) {
      std::cerr << "check_matrices() => Would you like to print the equations for each temperature node on layer" << i << "? [y/N]"
                << std::endl;
      std::cin >> input_char;
      if(input_char == 'y')
        ModelUnit::print_dyn_equations(i, datalibrary_);
      std::cout << "\n\n";
      std::cin.clear();
    }
  }

  std::cerr << "check_matrices() => Would you like to print all the model parameters for each temperature node on each layer? [y/N]"
            << std::endl;
  std::cin >> input_char;
  if(input_char == 'y') {
    for(unsigned int i = 0; i < datalibrary_->all_layers_info_.size(); i++) {
      std::cerr << "check_matrices() => Would you like to print the model parameters for each temperature node on layer" << i
                << "? [y/N]" << std::endl;
      std::cin >> input_char;
      if(input_char == 'y')
        ModelUnit::print_dyn_layer(i, datalibrary_);
      std::cout << "\n\n";
      std::cin.clear();
    }
  }

  std::cerr << "check_matrices() => Would you like to print the unsolved model matrix? [y/N]" << std::endl;
  ;
  if(std::cin.get() == 'y')
    print_unsolved_model();
  std::cout << "\n\n";
  std::cin.clear();

  // set heat generation to zero in all layers and set the layer temperatures to one
  for(unsigned int j = 0; j < datalibrary_->all_layers_info_.size(); j++) {
    set_temperature_layer(j, 1.0); // set the temperature to 1K
    if(datalibrary_->all_layers_info_[j]->chip_floorplan_ != NULL) {
      for(unsigned int i = 0; i < chip_flp_count(j); i++) {
        set_power_flp(i, j, 0.0); // set the power generation to zero
      }
    }
  }

  std::cerr << "check_matrices() => Set Power Generation To Zero and Temperature to 1K For All Units" << std::endl;

  // recompute the material properties
  recompute_material_properties();

  std::cerr << "check_matrices() => Recomputed Material Properties" << std::endl;

  // recompute the matrix
  int y_itor_global = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_)
            continue;

          munit.calculate_terms(datalibrary_->config_data_->default_time_increment_, false, false);
        }
    }

  std::cerr << "check_matrices() => ******** Checking unsolved_matrix_dyn_ and temperature_matrix: ********" << std::endl;
  std::cerr << "check_matrices() => NOTE: IN ORDER FOR THIS TO WORK, LOCK_TEMP MUST EITHER BE DISABLED FOR ALL LAYERS OR ELSE THE "
               "LOCK_TEMP VALUE MUST BE SET TO 1"
            << std::endl;

  // Now we create the labels for all of the various units
  std::vector<std::string> tempVector;
  std::vector<int>         vector_x, vector_y, vector_layer;
  tempVector.push_back("0,");
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_)
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_ == false)
            continue; // skip if unit is not defined
          tempVector.push_back("L" + Utilities::stringify(layer) + "[" + Utilities::stringify(x_itor) + "][" +
                               Utilities::stringify(y_itor) + "],");
          vector_x.push_back(x_itor);
          vector_y.push_back(y_itor);
          vector_layer.push_back(layer);
        }
  // for(unsigned int itor=0;itor<tempVector.size();itor++)
  //      std::cerr << tempVector[itor];
  std::cerr << std::endl;

  std::cerr << "check_matrices() => ******** Checking For Proper Temperature Pointer Locations from unsolved_matrix_dyn_ to "
               "temperature_matrix: ********"
            << std::endl;
  std::cerr << "check_matrices() => Number of elements in vector_layer:" << vector_layer.size() << std::endl;
  std::cerr << "check_matrices() => Number of elements in the solution temperature column:"
            << datalibrary_->temperature_matrix_size_ << std::endl;
  // error = false;
  // make sure to skip the lock-temp equations at the bottom of temperature column, these are not mapped to any of the model units

  // error = false;
  std::cerr << "check_matrices() => PRESS ANY KEY TO CONTINUE" << std::endl;
  std::cin.get();
  std::cout << "\n\n";
  std::cin.clear();

  std::cerr << "check_matrices() ******** Checking For Proper Equation Generation (LHS and RHS must be equal): ********"
            << std::endl;

  std::cerr << "check_matrices() => PRESS ANY KEY TO CONTINUE" << std::endl;
  std::cin.get();
  std::cout << "\n\n";
  std::cin.clear();
  std::cerr << "check_matrices() ******** Checking For Steady-State Solution (temperature should be 1 K for every unit): ********"
            << std::endl;
  std::cerr << "check_matrices() ********              MAKE SURE THAT LOCK-TEMP IS DISABLED FOR ALL ROWS (OTHERWISE THIS WILL "
               "FAIL) ********"
            << std::endl;
  // set heat generation to zero in all layers and set the layer temperatures to one
  for(unsigned int j = 0; j < datalibrary_->all_layers_info_.size(); j++) {
    set_temperature_layer(j, 1.0); // set the temperature to 25 celcius
    if(datalibrary_->all_layers_info_[j]->chip_floorplan_ != NULL) {
      for(unsigned int i = 0; i < chip_flp_count(j); i++) {
        set_power_flp(i, j, 0.0); // set the power generation to zero
      }
    }
  }

  std::cerr << "check_matrices() => Set Power Generation To Zero and Temperature to 1K For All Units" << std::endl;

  // recompute the material properties
  recompute_material_properties();

  std::cerr << "check_matrices() => Recomputed Material Properties" << std::endl;

  // recompute the matrix
  y_itor_global = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_)
            continue;

          munit.calculate_terms(datalibrary_->config_data_->default_time_increment_, false, false);
          y_itor_global++;
        }
    }

  // solve the matrix, generating new temperatures
  datalibrary_->timestep_ = .00001;
  // advance the model by timestep
  datalibrary_->time_ += .00001;
  datalibrary_->solve_matrix(datalibrary_->timestep_, _matrix_model_units); // run the model

  // error = false;

  std::cerr << "check_matrices() => PRESS ANY KEY TO CONTINUE" << std::endl;
  std::cin.get();
  std::cout << "\n\n";
  std::cin.clear();
}

// by default, the time increment config_data_->default_time_increment_ is used
void ThermModel::compute(bool recompute_material_properties) {
  compute(datalibrary_->config_data_->default_time_increment_, recompute_material_properties);
}

// make a call to the solver, incrementing the current time by the time increment
// if the sampler is enabled, then accumulate data for each sample
void ThermModel::compute(MATRIX_DATA timestep, bool recompute_material_properties) {
  run_model(timestep, datalibrary_->accumulate_rc_, datalibrary_->use_rc_, recompute_material_properties); // run the model
}

// just run the model without handling samples
void ThermModel::run_model(MATRIX_DATA timestep, bool accumulate_rc, bool use_rc, bool recompute_material_properties) {

  datalibrary_->time_ += timestep;

  if(timestep != datalibrary_->timestep_) {
    datalibrary_->timestep_ = timestep;
    // Note: false here means that we should recompute
    // Whenever the timestep changes we need to recompute

    // compute all the new temperature data given the new energy data
    compute_model_units(false, timestep, accumulate_rc, use_rc);
  } else {
    // variable inverted for desired behavior (true=recompute as input value is "calc_solution_only"--should be false for
    // recomputation)
    compute_model_units(!recompute_material_properties, timestep, accumulate_rc,
                        use_rc); // compute all the new temperature data given the new energy data
  }

  datalibrary_->solve_matrix(timestep, _matrix_model_units);
}

// store power data to the functional unit with the given name
void ThermModel::set_power_flp(int flp_pos, int layer, MATRIX_DATA power) {
  ASSERT_ESESCTHERM(layer >= 0 && layer < (int)datalibrary_->all_layers_info_.size(), "layer [" << layer << "] doesnt exist",
                    "ThermModel", "set_power_flp");

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];

  ASSERT_ESESCTHERM(layerInfo->chip_floorplan_ != NULL, "layer [" << layer << "] has undefined ChipFloorplan", "ThermModel",
                    "set_power_flp");

  ASSERT_ESESCTHERM(flp_pos >= 0 && flp_pos < (int)layerInfo->chip_floorplan_->flp_units_.size(),
                    "flp_pos [" << flp_pos << "] doesnt exist", "ThermModel", "set_power_flp");

  layerInfo->chip_floorplan_->flp_units_[flp_pos].set_power(power, datalibrary_->all_layers_info_[layer]->thickness_);
}

void ThermModel::set_power_layer(int layer, std::vector<MATRIX_DATA> &power_map) {
  ASSERT_ESESCTHERM(layer >= 0 && layer < (int)datalibrary_->all_layers_info_.size(), "layer [" << layer << "] doesnt exist",
                    "ThermModel", "set_power_layer");

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];
  ASSERT_ESESCTHERM(layerInfo->chip_floorplan_ != NULL, "layer [" << layer << "] has undefined ChipFloorplan", "ThermModel",
                    "set_power_layer");

  ASSERT_ESESCTHERM(power_map.size() == layerInfo->chip_floorplan_->flp_units_.size(),
                    "power map is of different size than that of layer [" << layer << "]", "ThermModel", "set_power_layer");

  for(int itr1 = 0; itr1 < (int)layerInfo->chip_floorplan_->flp_units_.size(); itr1++) {
    layerInfo->chip_floorplan_->flp_units_[itr1].set_power(power_map[itr1], layerInfo->thickness_);
  }
}

// store max power data to the functional unit with the given name
void ThermModel::set_max_power_flp(int flp_pos, int layer, MATRIX_DATA power) {

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];
  ASSERT_ESESCTHERM(layerInfo->chip_floorplan_ == NULL, "layer [" << layer << "] has undefined ChipFloorplan", "ThermModel",
                    "set_max_power_flp");

  I(flp_pos > 0 && flp_pos < (int)layerInfo->chip_floorplan_->flp_units_.size());
  layerInfo->chip_floorplan_->flp_units_[flp_pos].set_max_power(power);
}

// just return the average temperature of the layer as a while
MATRIX_DATA ThermModel::get_temperature_layer_average(int source_layer) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "layer [" << source_layer << "] does not exist", "ThermModel", "get_temperature_layer_average");

  return (datalibrary_->all_layers_info_[source_layer]->compute_average_temp());
}

// return the average temperature for each floorplan in a map
std::vector<MATRIX_DATA> ThermModel::get_temperature_layer_map(int flp_layer, int source_layer) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] doesnt exist", "ThermModel", "get_temperature_layer_map");

  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] doesnt exist", "ThermModel", "get_temperature_layer_map");

  return (datalibrary_->all_layers_info_[source_layer]->compute_average_temps(flp_layer));
}
std::vector<MATRIX_DATA> ThermModel::get_temperature_layer_noavg(int flp_layer, int source_layer) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] doesnt exist", "ThermModel", "get_temperature_layer_map");

  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] doesnt exist", "ThermModel", "get_temperature_layer_map");

  return (datalibrary_->all_layers_info_[source_layer]->compute_temps(flp_layer));
}
MATRIX_DATA ThermModel::get_temperature_flp(int flp_pos, int flp_layer, int source_layer) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] does not exist", "ThermModel", "get_temperature_flp");

  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] does not exist", "ThermModel", "get_temperature_flp");

  ASSERT_ESESCTHERM(flp_pos >= 0 && flp_pos < (int)datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(),
                    "flp_pos [" << flp_pos << "] does not exist", "ThermModel", "get_temperature_flp");

  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has no chip floorplan defined", "ThermModel", "get_temperature_flp");

  return (datalibrary_->all_layers_info_[source_layer]->compute_average_temps(flp_layer)[flp_pos]);
}

MATRIX_DATA ThermModel::get_time() {
  return (datalibrary_->time_);
}

std::string ThermModel::get_chip_flp_name(int flp_pos, int layer) {
  ASSERT_ESESCTHERM(layer >= 0 && layer < (int)datalibrary_->all_layers_info_.size(), "layer [" << layer << "] does not exist",
                    "ThermModel", "get_chip_flp_name");

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];

  ASSERT_ESESCTHERM(layerInfo->chip_floorplan_ != NULL, "layer [" << layer << "] has no chip floorplan defined", "ThermModel",
                    "get_chip_flp_name");

  ASSERT_ESESCTHERM(flp_pos >= 0 && flp_pos < (int)layerInfo->chip_floorplan_->flp_units_.size(),
                    "flp_pos [" << flp_pos << "] does not exist", "ThermModel", "get_chip_flp_name");

  return (layerInfo->chip_floorplan_->flp_units_[flp_pos].name_);
}

void ThermModel::set_temperature_flp(int flp_pos, int flp_layer, int source_layer, MATRIX_DATA temp) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] does not exist", "ThermModel", "set_temperature_flp");

  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] does not exist", "ThermModel", "set_temperature_flp");

  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has no chip floorplan defined", "ThermModel", "set_temperature_flp");

  ASSERT_ESESCTHERM(flp_pos >= 0 && flp_pos < (int)datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(),
                    "flp_pos [" << flp_pos << "] does not exist", "ThermModel", "set_temperature_flp");

  // find the floorplan unit
  ChipFloorplan_Unit &flp_unit = datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[flp_pos];

  DynamicArray<ModelUnit> &layer_dyn = (*datalibrary_->all_layers_info_[source_layer]->floorplan_layer_dyn_);

  // find all the model units that correspond to this floorplan unit
  if(flp_unit.located_units_[source_layer].empty()) {
    ModelUnit::locate_model_units(source_layer, flp_unit.located_units_[source_layer], flp_unit.leftx_, flp_unit.bottomy_,
                                  flp_unit.leftx_ + flp_unit.width_, flp_unit.bottomy_ + flp_unit.height_, datalibrary_);
  }
  // set the temperature to each model unit that cooresponds to the particular floorplan unit
  // Note: no need to check if the temperature_ pointer has been set (locate_model_units will only return non-zero pointers)
  for(int j = 0; j < (int)flp_unit.located_units_[source_layer].size(); j++) {
    *layer_dyn[flp_unit.located_units_[source_layer][j]->x_coord_][flp_unit.located_units_[source_layer][j]->y_coord_]
         .get_temperature() = temp;
    *layer_dyn[flp_unit.located_units_[source_layer][j]->x_coord_][flp_unit.located_units_[source_layer][j]->y_coord_]
         .get_previous_temperature() = temp;
  }

  // recompute the material properties as a function of temperature
  recompute_material_properties(source_layer);
}

// set the layer temperature based upon the input temperature-to-floorplan_unit map
// flp_layer is the model layer that has the particular floorplan that you wish you use when assigning temperatures
// source_layer is the layer which you wish to assign the temperature values to
void ThermModel::set_temperature_layer(int flp_layer, int source_layer, std::vector<MATRIX_DATA> &temperature_map) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] does not exist", "ThermModel", "set_temperature_layer");

  ASSERT_ESESCTHERM(flp_layer >= 0 && flp_layer < (int)datalibrary_->all_layers_info_.size(),
                    "flp_layer [" << flp_layer << "] does not exist", "ThermModel", "set_temperature_layer");

  ASSERT_ESESCTHERM(datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_ != NULL,
                    "flp_layer [" << flp_layer << "] has no chip floorplan defined", "ThermModel", "set_temperature_layer");

  DynamicArray<ModelUnit> &layer_dyn = (*datalibrary_->all_layers_info_[source_layer]->floorplan_layer_dyn_);

  for(int itr1 = 0; itr1 < (int)temperature_map.size(); itr1++) {
    // find the floorplan unit
    ChipFloorplan_Unit &flp_unit = datalibrary_->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[itr1];

    // find all the model units that correspond to this floorplan unit
    if(flp_unit.located_units_[source_layer].empty()) {
      ModelUnit::locate_model_units(source_layer, flp_unit.located_units_[source_layer], flp_unit.leftx_, flp_unit.bottomy_,
                                    flp_unit.leftx_ + flp_unit.width_, flp_unit.bottomy_ + flp_unit.height_, datalibrary_);
    }

    // set the temperatures in each model unit to correspond to the value in the temperature map
    // Note: no need to check if the temperature_ pointer has been set (locate_model_units will only return non-zero pointers)
    for(int j = 0; j < (int)flp_unit.located_units_[source_layer].size(); j++) {
      int x_coord                                             = flp_unit.located_units_[source_layer][j]->x_coord_;
      int y_coord                                             = flp_unit.located_units_[source_layer][j]->y_coord_;
      *layer_dyn[x_coord][y_coord].get_temperature()          = temperature_map[itr1];
      *layer_dyn[x_coord][y_coord].get_previous_temperature() = temperature_map[itr1];
    }
  }

  // recompute the material properties as a function of temperature
  recompute_material_properties(source_layer);
}

void ThermModel::set_temperature_layer(int source_layer, MATRIX_DATA temperature) {
  ASSERT_ESESCTHERM(source_layer >= 0 && source_layer < (int)datalibrary_->all_layers_info_.size(),
                    "source_layer [" << source_layer << "] does not exist", "ThermModel", "set_temperature_layer");

  DynamicArray<ModelUnit> &layer_dyn = (*datalibrary_->all_layers_info_[source_layer]->floorplan_layer_dyn_);
  // set the temperatures in each model unit to correspond to the value in the temperature map
  // Note: no need to check if the temperature_ pointer has been set (locate_model_units will only return non-zero pointers)
  for(unsigned int itor_x = 0; itor_x < layer_dyn.ncols(); itor_x++)
    for(unsigned int itor_y = 0; itor_y < layer_dyn.nrows(); itor_y++)
      if((layer_dyn)[itor_x][itor_y].defined_ != false) {
        *(layer_dyn[itor_x][itor_y].get_temperature())          = temperature;
        *(layer_dyn[itor_x][itor_y].get_previous_temperature()) = temperature;
        if(layer_dyn[itor_x][itor_y].temp_locking_enabled_)
          layer_dyn[itor_x][itor_y].lock_temp_ = temperature;
      }
  // recompute the material properties as a function of temperature
  recompute_material_properties(source_layer);
}

unsigned int ThermModel::chip_flp_count(int layer) {
  ASSERT_ESESCTHERM(layer >= 0 && layer < (int)datalibrary_->all_layers_info_.size(), "layer [" << layer << "] does not exist",
                    "ThermModel", "chip_flp_count");

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];
  ASSERT_ESESCTHERM(layerInfo->chip_floorplan_ != NULL, "layer [" << layer << "] has no chip floorplan defined", "ThermModel",
                    "chip_flp_count");

  return (layerInfo->chip_floorplan_->flp_units_.size());
}

void ThermModel::set_ambient_temp(MATRIX_DATA temp) {
  datalibrary_->config_data_->ambient_temp_ = temp;
}

void ThermModel::set_initial_temp(MATRIX_DATA temp) {
  datalibrary_->config_data_->init_temp_ = temp;
}

// FIXME: this data should be dumped to an [all temperatures] section of the outfile
void ThermModel::dump_temp_data() {
  I(0);
}

// FIXME: this function needs to be written
// It will calculate a weighted average of all of the model units that belong to each flp
// It will then dump the temperature data of each flp in the order that the labels were printed
// This will be dumped to an [average temps] section of the outfile
void ThermModel::dump_flp_temp_data() {
  I(0);
}

// FIXME: this function needs to be written
// It will print the labels of the flp in lab-deliminated fashion at the top of the outfile
void ThermModel::print_flp_temp_labels() {
  I(0);
}

// FIXME: this function needs to be written
// It will create an running average of the temperature for each of the ModelUnits
// This will be a very fine-grained method of warmup as the entire chip will be warmed up
void ThermModel::warmup_chip() {
  I(0);
}

// This function will take the data that was written in the dumpfile in either
// the [average temps] section or the [all temps] section and create a 3d graph of the data
void ThermModel::dump_3d_graph() {
  I(0);
}

void ThermModel::find_unused_model_units() {
  // find the number of number of unused columns in the heat_sink_fins_floorplan_dyn_
  unsigned int itor_x = 0;
  unsigned int itor_y = 0;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++)
    datalibrary_->all_layers_info_[i]->unused_dyn_count_ = 0;

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    if(datalibrary_->all_layers_info_[i]->type_ == HEAT_SINK_FINS_LAYER ||
       (datalibrary_->all_layers_info_[i]->type_ == UCOOL_LAYER && datalibrary_->all_layers_info_[i]->layer_used_)) {
      for(itor_x = 0; itor_x < datalibrary_->all_layers_info_[i]->floorplan_layer_dyn_->ncols(); itor_x++) {
        for(itor_y = 0; itor_y < datalibrary_->all_layers_info_[i]->floorplan_layer_dyn_->nrows(); itor_y++) {
          if((*datalibrary_->all_layers_info_[i]->floorplan_layer_dyn_)[itor_x][itor_y].defined_ == false)
            datalibrary_->all_layers_info_[i]->unused_dyn_count_++;
        }
      }
    }
  }
}

void ThermModel::compute_rc_start() {
  I(0);
}

int ThermModel::get_modelunit_count(int layer) {
  I(layer >= 0 && (unsigned int)layer < datalibrary_->all_layers_info_.size());
  return (get_dyn_array(layer).nrows() * get_dyn_array(layer).ncols());
}

int ThermModel::get_modelunit_rows(int layer) {
  I(layer >= 0 && (unsigned int)layer < datalibrary_->all_layers_info_.size());
  return (get_dyn_array(layer).nrows());
}

int ThermModel::get_modelunit_cols(int layer) {
  I(layer >= 0 && (unsigned int)layer < datalibrary_->all_layers_info_.size());
  return (get_dyn_array(layer).ncols());
}

void ThermModel::set_time(MATRIX_DATA timestep) {
  datalibrary_->timestep_ = timestep;
}

void ThermModel::compute_rc_stop() {
  datalibrary_->accumulate_rc_ = false;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_)
            get_dyn_array(layer)[x_itor][y_itor].compute_rc();
        }
    }
}

void ThermModel::fast_forward(MATRIX_DATA timestep) {
  datalibrary_->use_rc_ = true;
  compute(timestep);
  datalibrary_->use_rc_ = false;
}

void ThermModel::print_unsolved_model() {
  std::cerr << "******** Printing unsolved_matrix_dyn_: ********" << std::endl;

  // Now we create the labels for all of the various units
  std::vector<std::string> tempVector;
  tempVector.push_back("0,");

  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_)
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_ == false)
            continue; // skip if unit is not defined
          tempVector.push_back("L" + Utilities::stringify(layer) + "[" + Utilities::stringify(x_itor) + "][" +
                               Utilities::stringify(y_itor) + "],");
        }

  // for(unsigned int itor=0;itor<tempVector.size();itor++)
  //      std::cerr << tempVector[itor];
  std::cerr << std::endl;
}

// run through all of the floorplan dynamic arrays and calculate the terms for each one
void ThermModel::compute_model_units(bool calc_solution_only, MATRIX_DATA time_increment, bool accumulate_rc, bool use_rc) {
  // swap current temperature and previous temperature pointers
  // 1) Previous Temperatures take on the value of the temperature array
  //      and Temperature array takes on previous temperatues
  // 2) We then overwrite the temperatures array with the new temperatures

  SUElement_t *temp                          = datalibrary_->previous_temperature_matrix_;
  datalibrary_->previous_temperature_matrix_ = datalibrary_->temperature_matrix_;
  datalibrary_->temperature_matrix_          = temp;

  if(!calc_solution_only) {
    recompute_material_properties();
  }

  /*
  //handle the locked temperatures
  for (unsigned int layer=0;layer<datalibrary_->all_layers_info_.size();layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_ && datalibrary_->all_layers_info_[layer]->temp_locking_enabled_){
      for (unsigned int y_itor=0;y_itor<get_dyn_array(layer).nrows();y_itor++)
        for (unsigned int x_itor=0;x_itor<get_dyn_array(layer).ncols();x_itor++){
          if(!get_dyn_array(layer)[x_itor][y_itor].defined_){
            continue;
          }
          //cerr << "temp locking enabled on layer: " << layer << " value is:" << datalibrary_->all_layers_info_[layer]->lock_temp_
  << std::endl;
          *(get_dyn_array(layer)[x_itor][y_itor].get_previous_temperature())=datalibrary_->all_layers_info_[layer]->lock_temp_;
          //else{
            //get_dyn_array(layer)[x_itor][y_itor].get_previous_temperature()=get_dyn_array(layer)[x_itor][y_itor].get_temperature();
          //}
        }
    }


   */

  int y_itor_global = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_)
            continue;

          munit.calculate_terms(time_increment, calc_solution_only, use_rc);
          // if(accumulate_rc)
          //      get_dyn_array(layer)[x_itor][y_itor].accumulate_rc();
          y_itor_global++;
        }
    }

  // t_INF is always equal to ambient! (infinite reservoir of heat)
  // datalibrary_->temperature_matrix_[datalibrary_->unsolved_matrix_dyn_->nrows()-1]=datalibrary_->config_data_->ambient_temp_;

  if(!calc_solution_only) {
    // Also reset the coefficient values for rk4
    if(datalibrary_->_rk4Matrix)
      datalibrary_->_rk4Matrix->initialize_Matrix(_matrix_model_units);
  }
}

void ThermModel::print_unsolved_model_row(int row) {
  // Now we create the labels for all of the various units
  std::vector<std::string> tempVector;
  tempVector.push_back("0,");
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_)
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_ == false)
            continue; // skip if unit is not defined
          tempVector.push_back("L" + Utilities::stringify(layer) + "[" + Utilities::stringify(y_itor) + "][" +
                               Utilities::stringify(x_itor) + "]");
        }

  // for(unsigned int itor=0;itor<tempVector.size();itor++)
  //      std::cerr << tempVector[itor];
  std::cerr << std::endl;
}

void ThermModel::partition_floorplans() {
  MATRIX_DATA  bottomy_datapoint = 0;
  MATRIX_DATA  leftx_datapoint   = 0;
  MATRIX_DATA  rightx_datapoint  = 0;
  MATRIX_DATA  topy_datapoint    = 0;
  MATRIX_DATA  width;
  MATRIX_DATA  height;
  unsigned int y_itor           = 0;
  unsigned int x_itor           = 0;
  unsigned int x_itor_dyn_index = 0;
  unsigned int y_itor_dyn_index = 0;
  // bool got_x_offset = false;
  // bool got_y_offset = false;
  int x_offset = 0;
  int y_offset = 0;

  // clear the leftx datapoints
  leftx_.clear();

  // Step1: accumulate all of the datapoints to leftx and bottomy
  // run through all the layers
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    if(!datalibrary_->all_layers_info_[i]->layer_used_)
      continue;

    if(datalibrary_->all_layers_info_[i]->type_ == UCOOL_LAYER) {
      I(0);
    } else if(datalibrary_->all_layers_info_[i]->type_ == HEAT_SINK_FINS_LAYER) {
      // store all the leftx datapoints from the heat sink fins
      // Add the rightx datapoints that correspond to the heatsink fins (we don't need datapoint 0)
      MATRIX_DATA leftx_datapoint = datalibrary_->all_layers_info_[i]->leftx_;

      for(int j = 0; i < datalibrary_->config_data_->heat_sink_fin_number_; j++) {
        leftx_.push_back(leftx_datapoint);
        leftx_datapoint += datalibrary_->config_data_->heat_sink_width_ / datalibrary_->config_data_->heat_sink_fin_number_;
      }

      // store bottomy and topy datapoints
      bottomy_.push_back(datalibrary_->all_layers_info_[i]->bottomy_);
      bottomy_.push_back(datalibrary_->all_layers_info_[i]->bottomy_ + datalibrary_->all_layers_info_[i]->height_);

    }
    // if the layer has an associated floorplan and is NOT the UCOOL LAYER
    else if(datalibrary_->all_layers_info_[i]->type_ != UCOOL_LAYER && datalibrary_->all_layers_info_[i]->chip_floorplan_) {
      // store all the leftx datapoints from the chip floorplan
      for(unsigned int j = 0; j < datalibrary_->all_layers_info_[i]->chip_floorplan_->leftx_coords_.size(); j++) {
        leftx_.push_back(datalibrary_->all_layers_info_[i]->chip_floorplan_->leftx_coords_[j]);
      }

      // store all the bottomy datapoints from the chip floorplan
      for(unsigned int j = 0; j < datalibrary_->all_layers_info_[i]->chip_floorplan_->bottomy_coords_.size(); j++) {
        bottomy_.push_back(datalibrary_->all_layers_info_[i]->chip_floorplan_->bottomy_coords_[j]);
      }
    } else {
      // DIE_TRANSISTOR_LAYER must have an associated floorplan
      if(datalibrary_->all_layers_info_[i]->type_ == DIE_TRANSISTOR_LAYER) {
        std::cerr << "FATAL: ThermModel::store_model_unit => DIE_TRANSISTOR_LAYER must have an associated floorplan defined "
                     "(currently set to -1)"
                  << std::endl;
        exit(1);
      }
      // store leftx and rightx datapoints
      leftx_.push_back(datalibrary_->all_layers_info_[i]->leftx_);
      leftx_.push_back(datalibrary_->all_layers_info_[i]->rightx_);
      // store bottomy and topy datapoints
      bottomy_.push_back(datalibrary_->all_layers_info_[i]->bottomy_);
      bottomy_.push_back(datalibrary_->all_layers_info_[i]->topy_);
    }
  } // for each layer

  // sort the vectors, remove duplicate entries
  std::sort(leftx_.begin(), leftx_.end());
  std::sort(bottomy_.begin(), bottomy_.end());

  std::vector<MATRIX_DATA>::iterator new_end = std::unique(leftx_.begin(), leftx_.end(), Utilities::myUnique());
  // delete all elements past new_end
  leftx_.erase(new_end, leftx_.end());

  new_end = std::unique(bottomy_.begin(), bottomy_.end(), Utilities::myUnique());
  // delete all elements past new_end
  bottomy_.erase(new_end, bottomy_.end());

  // Step2: Create the 2-d cross-sectional regions for each layer
  // run through all the layers
  std::cerr << std::endl;
  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    if(datalibrary_->all_layers_info_[i]->layer_used_) {
#ifdef _ESESCTHERM_DEBUG
      std::cerr << "Partitioning Layer: " << i << std::endl;
#endif
      rightx_datapoint = datalibrary_->all_layers_info_[i]->leftx_;
      topy_datapoint   = datalibrary_->all_layers_info_[i]->bottomy_;
      x_offset         = 0;
      y_offset         = 0;
      y_itor           = 0;
      x_itor           = 0;
      y_itor_dyn_index = 0;
      x_itor_dyn_index = 0;
      // got_y_offset = false;
      // got_x_offset = false;

      // find the x-offset. This will be the index of leftx_[] that is just beyond the first leftx_ coordinate of the particular
      // layer
      while(LE(leftx_[x_offset], datalibrary_->all_layers_info_[i]->leftx_)) {
        x_offset++;
        if(x_offset == (int)leftx_.size() || GT(leftx_[x_offset], datalibrary_->all_layers_info_[i]->rightx_)) {
          std::cerr << "ThermModel::partitionfloorplans -> Fatal: Could not find datapoint just beyond leftx_ for floorplan [ " << i
                    << " ] " << std::endl;
          exit(1);
        }
      }

      // find the y-offset. This will be the index of bottomy_[] that is just beyond the first bottomy_ coordinate of the particular
      // layer
      while(LE(bottomy_[y_offset], datalibrary_->all_layers_info_[i]->bottomy_)) {
        y_offset++;
        if(y_offset == (int)bottomy_.size() || GT(bottomy_[x_offset], datalibrary_->all_layers_info_[i]->topy_)) {
          std::cerr << "ThermModel::partitionfloorplans -> Fatal: Could not find datapoint just beyond bottomy_ for floorplan [ "
                    << i << " ] " << std::endl;
          exit(1);
        }
      }

      // iterate from bottomy to topy
      while(LT(topy_datapoint, datalibrary_->all_layers_info_[i]->topy_)) {
        // store the bottomy datapoint of the cross-sectional region
        bottomy_datapoint = topy_datapoint;

        // if the bottom of the cross-sectional region occurs in an overlapping region with another layer with higher resolution,
        // select the layer with highest resolution find the finest granularity for the particular cross-sectional region
        MATRIX_DATA min_offset = 0;
        for(int j = 0; j < (int)datalibrary_->all_layers_info_.size(); j++) {
          if(GE(bottomy_datapoint, datalibrary_->all_layers_info_[j]->bottomy_) &&
             LE(bottomy_datapoint, datalibrary_->all_layers_info_[j]->topy_)) {
            // if min_offset not yet assigned, just assign the first encountered resolution (not necessary the highest resolution)
            if(min_offset == 0)
              min_offset = datalibrary_->all_layers_info_[j]->granularity_;
            else
              min_offset = MIN(min_offset, datalibrary_->all_layers_info_[j]->granularity_);
          }
        }
        // if min offset equals zero, then no offset was assigned. This means that an error occurred as every region's topy should
        // occur within SOME layer
        if(min_offset == 0) {
          std::cerr << "ThermModel::partitionfloorplans -> Fatal: Could not determine highest granularity for the particular "
                       "cross-sectional region in layer ["
                    << i << "]" << std::endl;
          exit(1);
        }

        // offset the topy_datapoint
        topy_datapoint += min_offset;

        // find the topy_datapoint of the cross-sectional region
        // if there is very fine granularity, this may be a lower value than the next flp unit's topy coordinate
        topy_datapoint = MIN(bottomy_[y_itor + y_offset], topy_datapoint);
        // topy_datapoint - bottomy_datapoint
        height           = topy_datapoint - bottomy_datapoint; // compute the height of the cross-sectional region
        x_itor           = 0;
        x_itor_dyn_index = 0;

        rightx_datapoint =
            datalibrary_->all_layers_info_[i]->leftx_; // the rightx_datapoint is initially the left point of the particular layer

        // iterate from leftx to rightx
        while(LT(rightx_datapoint, datalibrary_->all_layers_info_[i]->rightx_)) {
          // store the leftx datapoint of the cross-sectional region
          leftx_datapoint = rightx_datapoint;
          // if the left of the cross-sectional region occurs in an overlapping region with another layer with higher resolution,
          // select the layer with highest resolution
          MATRIX_DATA min_offset = 0;
          for(int j = 0; j < (int)datalibrary_->all_layers_info_.size(); j++) {
            if(GE(rightx_datapoint, datalibrary_->all_layers_info_[j]->leftx_) &&
               LE(rightx_datapoint, datalibrary_->all_layers_info_[j]->rightx_)) {
              // if min_offset not yet assigned, just assign the first encountered resolution (not necessary the highest resolution)
              if(min_offset == 0)
                min_offset = datalibrary_->all_layers_info_[j]->granularity_;
              else
                min_offset = MIN(min_offset, datalibrary_->all_layers_info_[j]->granularity_);
            }
          }
          // if min offset equals zero, then no offset was assigned. This means that an error occurred as every region's topy should
          // occur within SOME layer
          if(min_offset == 0) {
            std::cerr << "ThermModel::partitionfloorplans -> Fatal: Could not determine highest granularity for the particular "
                         "cross-sectional region in layer ["
                      << i << "]" << std::endl;
            exit(1);
          }

          // offset the rightx_datapoint
          rightx_datapoint += min_offset;
          // find the rightx_datapoint of the cross-sectional region
          // if there is very fine granularity, this may be a lower value than the next flp unit's leftx coordinate
          rightx_datapoint = MIN(leftx_[x_itor + x_offset], rightx_datapoint);
          width            = rightx_datapoint - leftx_datapoint; // compute the width of the cross-sectional region

          store_model_unit(rightx_datapoint, topy_datapoint, width, height, x_itor_dyn_index, y_itor_dyn_index, i);

          if(GE(rightx_datapoint,
                leftx_[x_itor + x_offset])) // if we haven't reached the data value leftx_[x_itor], don't increment itor
            x_itor++;
          x_itor_dyn_index++;
        }
        if(GE(topy_datapoint,
              bottomy_[y_itor + y_offset])) // if we haven't reached the data value bottomy_[y_itor], don't increment itor
          y_itor++;
        y_itor_dyn_index++;
      }
    }
  }

  // Step 3: Run through all of the layers within the dynamic arrays, fill in all additional information

  for(int i = 0; i < (int)datalibrary_->all_layers_info_.size(); i++) {
    if(datalibrary_->all_layers_info_[i]->layer_used_) {
      ChipLayers &layer_above = *datalibrary_->all_layers_info_[MIN(i + 1, (int)datalibrary_->all_layers_info_.size() - 1)];
      ChipLayers &layer_below = *datalibrary_->all_layers_info_[MAX(i - 1, 0)];
      for(y_itor = 0; y_itor < get_dyn_array(i).nrows(); y_itor++) {
        for(x_itor = 0; x_itor < get_dyn_array(i).ncols(); x_itor++) {
          ModelUnit &cur_model_unit = get_dyn_array(i)[x_itor][y_itor];

          // Initialize first
          for(int dir = 0; dir < MAX_DIR; dir++) {
            cur_model_unit.conduct_[dir]               = 0;
            cur_model_unit.heat_transfer_methods_[dir] = 0;
            cur_model_unit.model_[dir]                 = 0x0;
          }
          cur_model_unit.x1_ = cur_model_unit.x3_ = 0;
          cur_model_unit.y1_ = cur_model_unit.y3_ = 0;
          cur_model_unit.z1_ = cur_model_unit.z3_ = 0;

          // FIXME: add ability for detailed interface resistances between thermal units
          for(int dir = 0; dir < MAX_DIR; dir++)
            cur_model_unit.resist_[dir] = 0;

          if(datalibrary_->all_layers_info_[i]->type_ == UCOOL_LAYER) {
            // values already initialized above
            I(0);
          }

          // DETERMINE VERTICAL PARAMETERS (THICKNESSES and CONDUCTIVITIES)
          if(i - 1 >= 0) {
            // if there is a layer below, and a unit below

            ModelUnit *found_unit = ModelUnit::find_model_unit(cur_model_unit.leftx_, cur_model_unit.bottomy_, i - 1, datalibrary_);
            if(found_unit) {
              // if the layer is the heat_sink and the layer below is a heat_spreader_layer, then add the interface resistance
              if(datalibrary_->all_layers_info_[i]->type_ == HEAT_SINK_LAYER && layer_below.type_ == HEAT_SPREADER_LAYER) {
                cur_model_unit.resist_[dir_bottom] = datalibrary_->config_data_->heat_sink_resistance_;
              }
              cur_model_unit.y1_                                = found_unit->y2_;
              cur_model_unit.conduct_[dir_bottom]               = found_unit->conduct_center_[dir_top];
              cur_model_unit.heat_transfer_methods_[dir_bottom] = determine_heat_transfer_methods(cur_model_unit, *found_unit);
              cur_model_unit.model_[dir_bottom]                 = found_unit;
            }
          } // this is the bottom layer, treat bottom as insulating

          // if there is a layer above, and a unit above
          if(i + 1 < (int)datalibrary_->all_layers_info_.size()) {

            ModelUnit *found_unit = ModelUnit::find_model_unit(cur_model_unit.leftx_, cur_model_unit.bottomy_, i + 1, datalibrary_);
            if(found_unit) {
              // if the layer is the heat_spreader and the layer above is a heat_sink_layer, then add the interface resistance
              if(datalibrary_->all_layers_info_[i]->type_ == HEAT_SPREADER_LAYER && layer_above.type_ == HEAT_SINK_LAYER) {
                cur_model_unit.resist_[dir_top] = datalibrary_->config_data_->heat_sink_resistance_;
              }
              cur_model_unit.y3_                             = found_unit->y2_;
              cur_model_unit.conduct_[dir_top]               = found_unit->conduct_center_[dir_bottom];
              cur_model_unit.heat_transfer_methods_[dir_top] = determine_heat_transfer_methods(cur_model_unit, *found_unit);
              cur_model_unit.model_[dir_top]                 = found_unit;
            }
          } // this is the top layer, treat top as insulating

          // DETERMINE LATERAL PARAMETERS (WIDTH/HEIGHT and CONDUCTIVITIES)

          // if there is a unit downwards
          if((int)(y_itor - 1) >= 0) {
            cur_model_unit.conduct_[dir_down] = get_dyn_array(i)[x_itor][y_itor - 1].conduct_center_[dir_up];
            cur_model_unit.z1_                = get_dyn_array(i)[x_itor][y_itor - 1].z2_;
            cur_model_unit.heat_transfer_methods_[dir_down] =
                determine_heat_transfer_methods(cur_model_unit, get_dyn_array(i)[x_itor][y_itor - 1]);
            cur_model_unit.model_[dir_down] = &get_dyn_array(i)[x_itor][y_itor - 1];
          }

          // if there is a unit upwards
          if(y_itor + 1 < get_dyn_array(i).nrows()) {
            cur_model_unit.conduct_[dir_up] = get_dyn_array(i)[x_itor][y_itor + 1].conduct_center_[dir_down];
            cur_model_unit.z3_              = get_dyn_array(i)[x_itor][y_itor + 1].z2_;
            cur_model_unit.heat_transfer_methods_[dir_up] =
                determine_heat_transfer_methods(cur_model_unit, get_dyn_array(i)[x_itor][y_itor + 1]);
            cur_model_unit.model_[dir_up] = &get_dyn_array(i)[x_itor][y_itor + 1];
          }

          // if there is a unit to the left
          if((int)(x_itor - 1) >= 0) {
            cur_model_unit.conduct_[dir_left] = get_dyn_array(i)[x_itor - 1][y_itor].conduct_center_[dir_right];
            cur_model_unit.x1_                = get_dyn_array(i)[x_itor - 1][y_itor].x2_;
            cur_model_unit.heat_transfer_methods_[dir_left] =
                determine_heat_transfer_methods(cur_model_unit, get_dyn_array(i)[x_itor - 1][y_itor]);
            cur_model_unit.model_[dir_left] = &get_dyn_array(i)[x_itor - 1][y_itor];
          }

          // if there is a unit to the right
          if(x_itor + 1 < get_dyn_array(i).ncols()) {
            cur_model_unit.conduct_[dir_right] = get_dyn_array(i)[x_itor + 1][y_itor].conduct_center_[dir_left];
            cur_model_unit.x3_                 = get_dyn_array(i)[x_itor + 1][y_itor].x2_;
            cur_model_unit.heat_transfer_methods_[dir_right] =
                determine_heat_transfer_methods(cur_model_unit, get_dyn_array(i)[x_itor + 1][y_itor]);
            cur_model_unit.model_[dir_right] = &get_dyn_array(i)[x_itor + 1][y_itor];
          }
        } // for x_itor
      }   // for y-itor
    }     // if layer is used
  }       // for each layer
}

unsigned int ThermModel::determine_heat_transfer_methods(ModelUnit &ModelUnit_source, ModelUnit &ModelUnit_dest) {
  // make sure to set the convection coefficient properly
  if(ModelUnit_dest.heat_transfer_methods_center_ & (1 << HEAT_CONVECTION_TRANSFER)) {
    ModelUnit_source.convection_coefficient_ = ModelUnit_dest.convection_coefficient_;
  }

  // if the source model unit is a fluid/air that uses convective heat transfer, it CANNOT use conduction regardless of what the
  // adjacent unit is
  if(ModelUnit_source.heat_transfer_methods_center_ & (1 << HEAT_CONVECTION_TRANSFER)) {
    return (1 << HEAT_CONVECTION_TRANSFER);
  } else {
    return (ModelUnit_dest.heat_transfer_methods_center_);
  }
}

//(x,y) is the datapoint at the very center of the ModelUnit
MATRIX_DATA ThermModel::get_governing_equation(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height,
                                               int layer) {
  MATRIX_DATA leftx = rightx - width;
  // MATRIX_DATA bottomy = topy - height;
  // set the correct equation accordingly
  if(layer > 0 && datalibrary_->all_layers_info_[layer - 1]->type_ == UCOOL_LAYER)
    I(0);

  MATRIX_DATA itor = 0;
  switch(datalibrary_->all_layers_info_[layer]->type_) {
  case MAINBOARD_LAYER:
  case PINS_LAYER:
  case PWB_LAYER:
  case FCPBGA_LAYER:
  case C4_UNDERFILL_LAYER:
  case INTERCONNECT_LAYER:
  case DIE_TRANSISTOR_LAYER:
  case BULK_SI_LAYER:
  case OIL_LAYER:
  case AIR_LAYER:
  case HEAT_SPREADER_LAYER:
  case HEAT_SINK_LAYER:
    return (BASIC_EQUATION);
  case UCOOL_LAYER:
    I(0);
    break;
  case HEAT_SINK_FINS_LAYER:
    //   bottomy = topy - height;
    leftx = rightx - width;
    itor  = 0;
    while(LT(itor, (leftx + (width / 2)))) {
      itor += (MATRIX_DATA)datalibrary_->config_data_->heat_sink_width_ / datalibrary_->config_data_->heat_sink_fin_number_;
    }
    itor -= (MATRIX_DATA)datalibrary_->config_data_->heat_sink_width_ / datalibrary_->config_data_->heat_sink_fin_number_;
    itor *= (MATRIX_DATA)datalibrary_->config_data_->heat_sink_fin_number_;
    itor /= (MATRIX_DATA)datalibrary_->config_data_->heat_sink_width_;

    if(EVEN((int)itor))
      return (BASIC_EQUATION);
    else
      return (-1.00); // we are in between the heatsink fins (return -1.00)
  default:
    std::cerr << "ThermModel::get_governing_equation => unknown layer type [" << datalibrary_->all_layers_info_[layer]->type_ << "]"
              << std::endl;
    exit(1);
  }
  return 0;
}

void ThermModel::store_model_unit(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height, int x_itor,
                                  int y_itor, int layer) {
  ModelUnit   newunit(datalibrary_);
  MATRIX_DATA leftx   = rightx - width;
  MATRIX_DATA bottomy = topy - height;

  // store a ModelUnit to the heat_sink layer DynamicArray ( no need to lookup anything for chip floorplan):
  newunit.defined_        = true;
  newunit.x_coord_        = x_itor;
  newunit.y_coord_        = y_itor;
  newunit.leftx_          = rightx - width;
  newunit.bottomy_        = topy - height;
  newunit.topy_           = topy;
  newunit.rightx_         = rightx;
  newunit.width_          = width;
  newunit.height_         = height;
  newunit.flp_percentage_ = 1; // 100%

  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];

  newunit.x2_ = width;
  newunit.y2_ = layerInfo->thickness_;
  newunit.z2_ = height;

  // newunit.name_=datalibrary_->all_layers_info_[layer]->name_;
  newunit.governing_equation_ = get_governing_equation(rightx, topy, width, height, layer);

  newunit.specific_heat_              = layerInfo->spec_heat_;
  newunit.row_                        = layerInfo->density_;
  newunit.conduct_center_[dir_left]   = layerInfo->horizontal_conductivity_;
  newunit.conduct_center_[dir_right]  = layerInfo->horizontal_conductivity_;
  newunit.conduct_center_[dir_down]   = layerInfo->horizontal_conductivity_;
  newunit.conduct_center_[dir_up]     = layerInfo->horizontal_conductivity_;
  newunit.conduct_center_[dir_top]    = layerInfo->vertical_conductivity_up_;
  newunit.conduct_center_[dir_bottom] = layerInfo->vertical_conductivity_down_;

  newunit.emissivity_                   = layerInfo->emissivity_;
  newunit.convection_coefficient_       = layerInfo->convection_coefficient_;
  newunit.heat_transfer_methods_center_ = layerInfo->heat_transfer_methods_;
  newunit.temp_locking_enabled_         = layerInfo->temp_locking_enabled_;
  newunit.lock_temp_                    = layerInfo->lock_temp_;

  // store a mapping of [leftx_,bottomy_] to [x_itor][y_itor]
  layerInfo->coord_to_index_x_[newunit.leftx_]   = x_itor;
  layerInfo->coord_to_index_y_[newunit.bottomy_] = y_itor;

  // If we are in the vacant region, ignore
  if(get_governing_equation(rightx, topy, width, height, layer) == -1) {
    newunit.governing_equation_ = -1.0;
    newunit.defined_            = false;
    newunit.flp_percentage_     = 0;
    return;
  }

  switch(datalibrary_->all_layers_info_[layer]->type_) {
  case OIL_LAYER:
    break;
  case AIR_LAYER:
    break;
  case HEAT_SPREADER_LAYER:
    break;
  case HEAT_SINK_FINS_LAYER:
    // this is the percentage of the particular heat sink fin
    // area of cross-sectional region/size of fin
    newunit.flp_percentage_ =
        (width * height) / ((datalibrary_->all_layers_info_[layer]->height_ * datalibrary_->all_layers_info_[layer]->width_) /
                            datalibrary_->config_data_->heat_sink_fin_number_);
    break;
  case MAINBOARD_LAYER:
  case PINS_LAYER:
  case PWB_LAYER:
  case FCPBGA_LAYER:
  case C4_UNDERFILL_LAYER:
  case INTERCONNECT_LAYER:
  case DIE_TRANSISTOR_LAYER:
  case BULK_SI_LAYER:
    // check if there is an associated floorplan defined
    if(datalibrary_->all_layers_info_[layer]->chip_floorplan_ != NULL) {
      if(datalibrary_->all_layers_info_[layer]->chip_floorplan_->flp_unit_exists((leftx + (width) / 2), (bottomy + (height / 2)))) {
        newunit.source_chip_flp_ =
            &datalibrary_->all_layers_info_[layer]->chip_floorplan_->find_flp_unit((leftx + (width) / 2), (bottomy + (height / 2)));
        // newunit.name_=datalibrary_->all_layers_info_[layer]->chip_floorplan_->find_flp_unit((leftx+(width)/2),(bottomy+(height/2))).name_;
        newunit.flp_percentage_ = height * width /
                                  (datalibrary_->all_layers_info_[layer]
                                       ->chip_floorplan_->find_flp_unit((leftx + (width) / 2), (bottomy + (height / 2)))
                                       .height_ *
                                   datalibrary_->all_layers_info_[layer]
                                       ->chip_floorplan_->find_flp_unit((leftx + (width) / 2), (bottomy + (height / 2)))
                                       .width_);
      } else {
        std::cerr << "ThermModel::store_model_unit => Fatal:Chip region was found that does not correspond to a functional unit"
                  << std::endl;
        std::cerr << "ThermModel::store_model_unit => Could not find flp unit that corresponds to the datapoint ["
                  << (leftx + (width / 2)) << "][" << (bottomy + (height / 2)) << "]" << std::endl;
        exit(1);
      }
    }
    break;

  default:
    std::cerr << "Fatal: store_model_unit: invalid layer number[" << layer << std::endl;
    exit(1);
  }

  // To handle the UCOOL_HOT and UCOOL_COLD units, we need to check the adjacent regions
  // The reason why is because it might be possible for a UCOOL_HOT unit to appear in ANY other kind of layer
  // To handle this, we need to be able to convert that unit into a UCOOL_HOT or cold unit

  // If the layer below is a UCOOL_LAYER, then this is a COLD region
  if(layer >= 1 && datalibrary_->all_layers_info_[layer - 1]->type_ == UCOOL_LAYER) {
    I(0);
  }

  if(layer < (int)(datalibrary_->all_layers_info_.size() - 1) && datalibrary_->all_layers_info_[layer + 1]->type_ == UCOOL_LAYER) {
    I(0);
  }

  get_dyn_array(layer)[x_itor][y_itor] = newunit;
  // calculate the material properties for the particular unit
  // depending upon the type, no computation may be required
  Material::calc_material_properties_dyn_density(layer, x_itor, y_itor, datalibrary_);
}

// This will recompute the material conductivities for each
// cross-sectional region based upon the material and temperature at that particular region
// at this point, only the silicon layer is temperature dependent
void ThermModel::recompute_material_properties() {
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_) {
            Material::calc_material_properties_dyn_temp(layer, x_itor, y_itor, datalibrary_);
          }
        }
    }
}

void ThermModel::recompute_material_properties(int layer) {
  if(layer < 0 || layer >= (int)datalibrary_->all_layers_info_.size()) {
    std::cerr << "FATAL: ThermModel::recompute_material_properties =>  layer[ " << layer << " ] does not exist" << std::endl;
    exit(1);
  }
  for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
    for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
      if(get_dyn_array(layer)[x_itor][y_itor].defined_) {
        Material::calc_material_properties_dyn_temp(layer, x_itor, y_itor, datalibrary_);
      }
    }
}

void ThermModel::print_graphics() {
  if(datalibrary_->graphics_->get_enable_graphics())
    datalibrary_->graphics_->print_graphics();
}

void ThermModel::accumulate_sample_data() {
  // accumulate data*timestep values in each of the layers
  for(unsigned int i = 0; i < datalibrary_->all_layers_info_.size(); i++)
    datalibrary_->all_layers_info_[i]->accumulate_data_for_sample();
}

void ThermModel::enable_temp_locking(int layer, MATRIX_DATA temperature) {
  ChipLayers *linfo = datalibrary_->all_layers_info_[layer];

  linfo->temp_locking_enabled_ = true;
  linfo->lock_temp_            = temperature;
}

void ThermModel::disable_temp_locking(int layer) {
  datalibrary_->all_layers_info_[layer]->temp_locking_enabled_ = false;
}

// FIXME: to delete this and associated code (RK4 does not use it)
MATRIX_DATA ThermModel::get_max_timestep() {
  // stability criteria requires that finite-difference form of Fourier number be less than (1/6)
  // Fourier number is defined as alpha_*dt/(dx^2*dy^2*dz^2)
  // Therefore dt< ((1/6)*dx^2*dy^2*dz^2/alpha)
  MATRIX_DATA average_timestep = 0;
  MATRIX_DATA num_elements     = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(munit.defined_ == false)
            continue; // skip if unit is not defined

          // MATRIX_DATA volume=munit.x2_* munit.y2_* munit.z2_;
          MATRIX_DATA ave_side_length = (munit.x2_ + munit.y2_ + munit.z2_) / 3;
          MATRIX_DATA volume          = double_pow(ave_side_length, 3.0);
          MATRIX_DATA surface_area    = 6 * ave_side_length;
          MATRIX_DATA char_length     = volume / surface_area;
          average_timestep += (1.0 / 6.0) * double_pow(char_length, 2.0) / munit.calc_alpha_max(false);
          num_elements++;
        }
    }

  return (average_timestep / num_elements);
}

// FIXME: to delete this and associated code (RK4 does not use it)
MATRIX_DATA ThermModel::get_recommended_timestep() {
  // stability criteria requires that finite-difference form of Fourier number be less than (1/24)
  // Fourier number is defined as alpha_*dt/(dx^2*dy^2*dz^2)
  // Therefore dt< ((1/24)*dx^2*dy^2*dz^2/alpha)
  MATRIX_DATA average_timestep = 0;
  MATRIX_DATA num_elements     = 0;
  for(unsigned int layer = 0; layer < datalibrary_->all_layers_info_.size(); layer++)
    if(datalibrary_->all_layers_info_[layer]->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_)
            continue;

          MATRIX_DATA ave_side_length = (munit.x2_ + munit.y2_ + munit.z2_) / 3;
          MATRIX_DATA volume          = double_pow(ave_side_length, 3.0);
          MATRIX_DATA surface_area    = 6 * ave_side_length;
          MATRIX_DATA char_length     = volume / surface_area;
          average_timestep += (1.0 / 24.0) * double_pow(char_length, 2.0) / munit.calc_alpha_max(false);
          num_elements++;
        }
    }
  return (average_timestep / num_elements);
}
