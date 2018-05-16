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
File name:      ModelUnit.cpp
Description:    Handles the equations for each temperature node
********************************************************************************/

#include "sescthermMacro.h"
#include <algorithm>
#include <complex>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>

#include "ClassDeclare.h"
#include "sesctherm3Ddefine.h"

#include "ChipFloorplan.h"
#include "ChipLayers.h"
#include "DataLibrary.h"
#include "ModelUnit.h"
#include "Utilities.h"
#include "nanassert.h"

void ModelUnit::initialize(DataLibrary *datalibrary) {
  if(datalibrary == NULL) {
    std::cerr << "FATAL: ModelUnit::ModelUnit::zero called, but datalibrary pointer was NULL!" << std::endl;
  }
  datalibrary_     = datalibrary;
  defined_         = false;
  source_chip_flp_ = NULL;  // this is a reference to the source chip flp that this modelunit is a part of
  defined_         = false; // is this modelunit used?
  // ModelUnit.name_="";            //name of region that unit belongs to
  flp_percentage_ =
      0; // percentage of the region's area that the unit belongs to. Note: this does need to be used when calculating energy data
  // solution data
  temperature_          = NULL;
  previous_temperature_ = NULL; // this was the previously calculated result (initially the steady-temp value)
  // gamma
  temperature_index_ = -1;
  gamma_             = 0;
  row_               = 0;
  specific_heat_     = 0;

  // material conductances
  for(int dir = 0; dir < MAX_DIR; dir++) {
    conduct_[dir]        = 0;
    conduct_center_[dir] = 0;
    resist_[dir]         = 0;
  }

  // dimensions
  x1_ = 0;
  x2_ = 0;
  x3_ = 0;
  y1_ = 0;
  y2_ = 0;
  y3_ = 0;
  z1_ = 0;
  z2_ = 0;
  z3_ = 0;

  // the governing equation for the given model unit
  governing_equation_ = 0;

  t_mno = NULL;
  for(int dir = 0; dir < MAX_DIR; dir++)
    t_ptr[dir] = NULL;

  power_per_unit_area_ = -1.0;
}

// set power per unit area (W/m^3)
void ModelUnit::set_power(MATRIX_DATA power) {
  power_per_unit_area_ = power / (y2_ * source_chip_flp_->width_ * source_chip_flp_->height_);
}

// return: total power in this unit
SUElement_t ModelUnit::total_power() {
  // if no source chip floorplan, it means that we are in a layer that does
  // not have internal power generation (layers 2,3 or 4)
  if(!source_chip_flp_)
    return 0.0;
  if(EQ(power_per_unit_area_, -1.0))
    return (source_chip_flp_->get_power_per_unit_area() * x2_ * y2_ * z2_);
  else
    return (power_per_unit_area_ * x2_ * y2_ * z2_);
}

SUElement_t *ModelUnit::get_temperature() {
  return (*temperature_ + temperature_index_);
}

SUElement_t *ModelUnit::get_previous_temperature() {
  return (*previous_temperature_ + temperature_index_);
}

void ModelUnit::accumulate_rc() {
  I(0);
  // datapoints_x_.push_back(datalibrary_->time_);
  // datapoints_y_.push_back(*get_temperature());
}

void ModelUnit::compute_rc() {
  I(0);
}

void ModelUnit::print_coeffs() {
  printf("\n");
  if(t_mno)
    printf(" t_mno %g", *t_mno);

  for(int dir = 0; dir < MAX_DIR; dir++)
    if(t_ptr[dir])
      printf("t_ptr[%d] %g", dir, *(t_ptr[dir]));
}

// calulate all the term values given the equation number
// time_increment is the time step
// calc_solution_only is used if we don't need to recompute the matrix
//(recomputation is necessary when timestep changes, material properties change, or model geometry changes)
// accumulate_rc is used when we need to accumuate the [time,temperature data] for each timestep
// use_rc is used when we want to use an effective RC time constant to fast-forward the model

void ModelUnit::calculate_terms(MATRIX_DATA time_increment, bool calc_solution_only, bool use_rc) {
  if(!defined_) {
    // if the model unit is unused, set everything to zero
    t_mno = NULL;
    for(int dir = 0; dir < MAX_DIR; dir++)
      t_ptr[dir] = NULL;

    // initialize solution data
    temperature_          = NULL;
    previous_temperature_ = NULL;
    temperature_index_    = -1;
    sample_temperature_   = 0;
    return;
  }

  calc_gamma(time_increment, use_rc);

  // directions are dir_left, dir_right, dir_bottom , dir_top , dir_down , dir_up
  /* SUElement_t *t_ptr[MAX_DIR] =
     { t_m_M1_no, t_m_P1_no, t_mn_M1_o, t_mn_P1_o,  t_mno_M1, t_mno_P1 } ; */
  MATRIX_DATA dir_coords[MAX_DIR] = {x1_, x3_, y1_, y3_, z1_, z3_};

  // if this is heat transfer to another model unit though HEAT_CONVECTION_TRANSFER and/or HEAT_CONDUCTION_TRANSFER and/or
  // HEAT_RADIATION_TRANSFER
  if(governing_equation_ == BASIC_EQUATION) {
    // if temperature locking is enabled, then the equation is T_mno=locktemp
    if(temp_locking_enabled_) {
      if(!calc_solution_only) {
        I(t_mno);
        if(t_mno)
          *t_mno = 0.0;

        for(int dir = 0; dir < MAX_DIR; dir++)
          if(t_ptr[dir])
            *(t_ptr[dir]) = 0.0;
      }
      *get_temperature()          = lock_temp_;
      *get_previous_temperature() = lock_temp_;

      return;
    }

    if(!calc_solution_only) {
      I(t_mno);
      if(t_mno)
        *t_mno = 0.0;

      MATRIX_DATA ro_Cp_V = row_ * specific_heat_ * volume();

      MATRIX_DATA conduct_convection[MAX_DIR];
      MATRIX_DATA conduct_conduction[MAX_DIR];
      MATRIX_DATA conduct_radiation[MAX_DIR];
      MATRIX_DATA radiation_heat_transfer_coefficient[MAX_DIR];
      MATRIX_DATA convection_coefficient[MAX_DIR];
      MATRIX_DATA temp[MAX_DIR];

      // initialize to 0 for all 6 directions
      for(int dir = 0; dir < MAX_DIR; dir++) {
        conduct_convection[dir]                  = 0;
        conduct_conduction[dir]                  = 0;
        conduct_radiation[dir]                   = 0;
        radiation_heat_transfer_coefficient[dir] = 0;
        convection_coefficient[dir]              = 0;
        temp[dir]                                = 0;
        if(t_ptr[dir])
          *(t_ptr[dir]) = 0.0;
      }

      MATRIX_DATA area[MAX_DIR];
      area[dir_left] = area[dir_right] = z2_ * y2_;
      area[dir_down] = area[dir_up] = x2_ * y2_;
      area[dir_bottom] = area[dir_top] = x2_ * z2_;

      // These numbers are used in computing parallel conduction between adjacent nodes.
      // These are specific to direction
      MATRIX_DATA num1[MAX_DIR], num2[MAX_DIR];
      num1[dir_left]   = x2_;
      num2[dir_left]   = x1_;
      num1[dir_right]  = x2_;
      num2[dir_right]  = x3_;
      num1[dir_down]   = z2_;
      num2[dir_down]   = z1_;
      num1[dir_up]     = z2_;
      num2[dir_up]     = z3_;
      num1[dir_bottom] = y2_;
      num2[dir_bottom] = y1_;
      num1[dir_top]    = y2_;
      num2[dir_top]    = y3_;

      for(int dir = 0; dir < MAX_DIR; dir++) {
        if(model_[dir]) {
          temp[dir]                   = *model_[dir]->get_previous_temperature();
          convection_coefficient[dir] = std::max(model_[dir]->convection_coefficient_, convection_coefficient_);
        } // if model
        if(heat_transfer_methods_[dir] & (1 << HEAT_CONVECTION_TRANSFER))
          conduct_convection[dir] = (convection_coefficient[dir] * area[dir]);

        if(heat_transfer_methods_[dir] & (1 << HEAT_CONDUCTION_TRANSFER)) {
          conduct_conduction[dir] = 1 / ((num1[dir] / 2) / (conduct_center_[dir] * area[dir]) + resist_[dir] +
                                         (num2[dir] / 2) / (conduct_[dir] * area[dir]));
        }
        if(heat_transfer_methods_[dir] & (1 << HEAT_RADIATION_TRANSFER)) {
          radiation_heat_transfer_coefficient[dir] =
              4 * emissivity_ * BOLTZMAN_CONST * calc_view_factor(dir) * pow((*get_previous_temperature()) * (temp[dir]), 3 / 2);
          conduct_radiation[dir] = (radiation_heat_transfer_coefficient[dir] * area[dir]);
        }

        // set the coefficient in matrix for this direction
        if(!use_rc && t_ptr[dir] && (!EQ(dir_coords[dir], -1.0))) { // model and not ambient air
          MATRIX_DATA tot = conduct_conduction[dir] + conduct_convection[dir] + conduct_radiation[dir];
          *t_mno += tot;
          *(t_ptr[dir]) = -tot;
        }
      } // for each of 6 directions

      *t_mno = (*t_mno) / ro_Cp_V;
      for(int d = 0; d < MAX_DIR; d++)
        if(t_ptr[d])
          *t_ptr[d] = (*t_ptr[d]) / ro_Cp_V;
    } // if we need to calculate model parameters also

    *get_temperature() = *get_previous_temperature();
  } else {
    std::cerr << "Model-unit error: Invalid governing equation: " << governing_equation_ << std::endl;
    exit(1);
  }
  // print_coeffs();
}

inline void ModelUnit::calc_gamma(MATRIX_DATA time_increment, bool use_rc) {
  gamma_ = ((row_ * x2_ * y2_ * z2_ * specific_heat_) / time_increment);
}

// return the maximum value of alpha (thermal diffusivity m^2/s)
// this will be used to determine the maximum/recommended timestep
MATRIX_DATA ModelUnit::calc_alpha_max(bool use_rc) {

  MATRIX_DATA max_conductivity = -1.0;
  for(int dir = 0; dir < MAX_DIR; dir++)
    max_conductivity = MAX(max_conductivity, conduct_center_[dir]);

  if(!use_rc)
    return (max_conductivity / (row_ * specific_heat_));
  else
    return (max_conductivity / (row_ * specific_heat_eff_));
}

MATRIX_DATA ModelUnit::calc_view_factor(int dir) {
  MATRIX_DATA width, height, distance;
  switch(dir) {
  case dir_left:
    width    = z2_;
    height   = y2_;
    distance = (x1_ / 2 + x2_ / 2);
    break;
  case dir_right:
    width    = z2_;
    height   = y2_;
    distance = (x3_ / 2 + x2_ / 2);
    break;
  case dir_top:
    width    = x2_;
    height   = z2_;
    distance = (y3_ / 2 + y2_ / 2);
    break;
  case dir_bottom:
    width    = x2_;
    height   = z2_;
    distance = (y1_ / 2 + y2_ / 2);
    break;
  case dir_up:
    width    = x2_;
    height   = y2_;
    distance = (z3_ / 2 + z2_ / 2);
    break;
  case dir_down:
    width    = x2_;
    height   = y2_;
    distance = (z1_ / 2 + z2_ / 2);
    break;
  default:
    std::cerr << "Invalid direction in calculaye view factor!!" << std::endl;
    return 0;
  }

  MATRIX_DATA x_bar = width / distance;  // x_bar=x/l
  MATRIX_DATA y_bar = height / distance; // y_bar=y/l
  return ((2 / (M_PI * x_bar * y_bar)) *
          (logf(sqrtf((1 + powf(x_bar, 2.0)) * (1 + powf(y_bar, 2.0)))) // note: log=natural log
           + (x_bar * sqrt(1 + powf(y_bar, 2.0)) * atan(x_bar / sqrt(1 + powf(y_bar, 2.0)))) +
           (y_bar * sqrt(1 + powf(x_bar, 2.0)) * atan(y_bar / sqrt(1 + powf(x_bar, 2.0)))) - (x_bar * atan(x_bar)) -
           (y_bar * atan(y_bar)))); // compute view factor of aligned parallel rectangles (see Incropera/Dewitt pg 723
}

void ModelUnit::print_dyn_equations(int layer, DataLibrary *datalibrary_) {
  DynamicArray<ModelUnit> *dyn_layer = datalibrary_->all_layers_info_[layer]->floorplan_layer_dyn_;

  for(unsigned int y_itor = 0; y_itor < dyn_layer->nrows(); y_itor++) {
    for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++) {
      ModelUnit &temp_unit = (*dyn_layer)[x_itor][y_itor];
      I(temp_unit.t_mno);
      std::cerr << "L" << layer << "[" << x_itor << "][" << y_itor << "] (" << *temp_unit.t_mno << ")T_mno ";

      // print neighbors
      for(int dir = 0; dir < MAX_DIR; dir++) {
        SUElement_t *tptr = temp_unit.t_ptr[dir];
        if(tptr && !EQ(*tptr, 0.0))
          std::cerr << "+ (" << dir << " " << *tptr << ") ";
      }

      MATRIX_DATA power = 0;
      if(temp_unit.source_chip_flp_ != NULL)
        power = temp_unit.source_chip_flp_->get_power_per_unit_area();
      if(power != 0)
        std::cerr << "= " << temp_unit.gamma_ << "*" << *temp_unit.get_previous_temperature() << " + " << power << "*"
                  << temp_unit.x2_ << "*" << temp_unit.y2_ << "*" << temp_unit.z2_
                  << "  [temperature = " << *temp_unit.get_temperature()
                  << "][previous_temperature = " << *temp_unit.get_previous_temperature() << "]" << std::endl;
      else
        std::cerr << "= " << temp_unit.gamma_ << "*" << *temp_unit.get_previous_temperature()
                  << "  [temperature = " << *temp_unit.get_temperature()
                  << "][previous_temperature = " << *temp_unit.get_previous_temperature() << "]" << std::endl;
    }
  }
}

// Print the equation ( dT/dt + CT = p) at this node
void ModelUnit::print_rk4_equation(MATRIX_DATA power) {
  int  idx       = temperature_index_;
  bool has_coeff = (!EQ(*t_mno, 0.0));
  for(int dir = 0; dir < MAX_DIR; dir++)
    if(t_ptr[dir] && !EQ(*t_ptr[dir], 0.0))
      has_coeff = true;

  if(!has_coeff)
    return;

  printf("\ndT(%d) + %.3f * %.3f ", idx, *t_mno, *get_temperature());
  for(int dir = 0; dir < MAX_DIR; dir++) {
    if(t_ptr[dir] && !EQ(*t_ptr[dir], 0.0))
      printf(" %.3f * %.3f (%d) ", *t_ptr[dir], *(model_[dir]->get_temperature()), model_[dir]->temperature_index_);
  }
  printf(" = %.3f", power);

  MATRIX_DATA ro_Cp_V = row_ * specific_heat_ * volume();
  printf(" ro_Cp_V = %.3g", ro_Cp_V);
}

// This macro is used to print a field of the dynamic layer.
#define PRINT_FIELD(FIELD_STR, FIELD)                                                         \
  fprintf(stderr, " ***** PRINTING %s for layer [%d]*****\n", FIELD_STR, layer);              \
  std::cerr << "(L" << layer << ").ncols()=" << dyn_layer->ncols() << std::endl;              \
  std::cerr << "(L" << layer << ").nrows()=" << dyn_layer->nrows() << std::endl << std::endl; \
  for(int y_itor = (int)dyn_layer->nrows() - 1; y_itor >= 0; y_itor--) {                      \
    for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++)                       \
      std::cerr << "[" << std::setw(6) << (*dyn_layer)[x_itor][y_itor].FIELD << "]   ";       \
    std::cerr << std::endl;                                                                   \
  }                                                                                           \
  std::cerr << std::endl << std::endl;

#define PRINT_FIELD_WITH_NULL_CHECK(FIELD_STR, FIELD)                                         \
  fprintf(stderr, " ***** PRINTING %s for layer [%d]*****\n", FIELD_STR, layer);              \
  std::cerr << "(L" << layer << ").ncols()=" << dyn_layer->ncols() << std::endl;              \
  std::cerr << "(L" << layer << ").nrows()=" << dyn_layer->nrows() << std::endl << std::endl; \
  for(int y_itor = (int)dyn_layer->nrows() - 1; y_itor >= 0; y_itor--) {                      \
    for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++) {                     \
      if((*dyn_layer)[x_itor][y_itor].FIELD)                                                  \
        std::cerr << "[" << std::setw(6) << *(*dyn_layer)[x_itor][y_itor].FIELD << "]   ";    \
      else                                                                                    \
        std::cerr << "[" << std::setw(6) << 0 << "]   ";                                      \
    }                                                                                         \
    std::cerr << std::endl;                                                                   \
  }                                                                                           \
  std::cerr << std::endl << std::endl;

// Use macro PRINT_FIELD and PRINT_FIELD_WITH_NULL_CHECK to print many fields
void ModelUnit::print_dyn_layer(int layer, DataLibrary *datalibrary_) {
  DynamicArray<ModelUnit> *dyn_layer = datalibrary_->all_layers_info_[layer]->floorplan_layer_dyn_;

  PRINT_FIELD_WITH_NULL_CHECK("t_mno", t_mno);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_left]", t_ptr[dir_left]);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_right]", t_ptr[dir_right]);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_bottom]", t_ptr[dir_bottom]);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_top]", t_ptr[dir_top]);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_down]", t_ptr[dir_down]);
  PRINT_FIELD_WITH_NULL_CHECK("t_ptr[dir_up]", t_ptr[dir_up]);

  PRINT_FIELD("specific_heat_", specific_heat_);
  PRINT_FIELD("row_", row_);
  PRINT_FIELD("heat_transfer_methods_center_", heat_transfer_methods_center_);

  // printing heat capacitance
  std::cerr << " ***** PRINTING lumped thermal capacitance for layer [" << layer << "]*****" << std::endl;
  std::cerr << "(L" << layer << ").ncols()=" << dyn_layer->ncols() << std::endl;
  std::cerr << "(L" << layer << ").nrows()=" << dyn_layer->nrows() << std::endl << std::endl;

  for(int y_itor = (int)dyn_layer->nrows() - 1; y_itor >= 0; y_itor--) {
    for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++) {
      std::cerr << "[" << std::setw(6)
                << (*dyn_layer)[x_itor][y_itor].row_ * (*dyn_layer)[x_itor][y_itor].x2_ * (*dyn_layer)[x_itor][y_itor].y2_ *
                       (*dyn_layer)[x_itor][y_itor].z2_ * (*dyn_layer)[x_itor][y_itor].specific_heat_
                << "]   ";
    }
    std::cerr << std::endl;
  }
  std::cerr << std::endl << std::endl;

  PRINT_FIELD("conduct_center_[dir_left]", conduct_center_[dir_left]);
  PRINT_FIELD("conduct_center_[dir_right]", conduct_center_[dir_right]);
  PRINT_FIELD("conduct_center_[dir_down]", conduct_center_[dir_down]);
  PRINT_FIELD("conduct_center_[dir_up]", conduct_center_[dir_up]);
  PRINT_FIELD("conduct_center_[dir_bottom]", conduct_center_[dir_bottom]);
  PRINT_FIELD("conduct_center_[dir_top]", conduct_center_[dir_top]);

  PRINT_FIELD("x1_", x1_);
  PRINT_FIELD("x2_", x2_);
  PRINT_FIELD("x3_", x3_);

  PRINT_FIELD("y1_", y1_);
  PRINT_FIELD("y2_", y2_);
  PRINT_FIELD("y3_", y3_);

  PRINT_FIELD("z1_", z1_);
  PRINT_FIELD("z2_", z2_);
  PRINT_FIELD("z3_", z3_);

  PRINT_FIELD("width_", width_);
  PRINT_FIELD("height_", height_);
  PRINT_FIELD("leftx_", leftx_);
  PRINT_FIELD("bottomy_", bottomy_);

  PRINT_FIELD("conduct_center_[dir_left]", conduct_center_[dir_left]);
  PRINT_FIELD("conduct_center_[dir_right]", conduct_center_[dir_right]);
  PRINT_FIELD("conduct_center_[dir_down]", conduct_center_[dir_down]);
  PRINT_FIELD("conduct_center_[dir_up]", conduct_center_[dir_up]);
  PRINT_FIELD("conduct_center_[dir_bottom]", conduct_center_[dir_bottom]);
  PRINT_FIELD("conduct_center_[dir_top]", conduct_center_[dir_top]);

  PRINT_FIELD("conduct_[dir_left]", conduct_[dir_left]);
  PRINT_FIELD("conduct_[dir_right]", conduct_[dir_right]);
  PRINT_FIELD("conduct_[dir_down]", conduct_[dir_down]);
  PRINT_FIELD("conduct_[dir_up]", conduct_[dir_up]);
  PRINT_FIELD("conduct_[dir_bottom]", conduct_[dir_bottom]);
  PRINT_FIELD("conduct_[dir_top]", conduct_[dir_top]);

  PRINT_FIELD("defined_", defined_);
  PRINT_FIELD("flp_percentage_", flp_percentage_);

  PRINT_FIELD("resist_[dir_bottom]", resist_[dir_bottom]);
  PRINT_FIELD("resist_[dir_down]", resist_[dir_down]);
  PRINT_FIELD("resist_[dir_left]", resist_[dir_left]);
  PRINT_FIELD("resist_[dir_right]", resist_[dir_right]);
  PRINT_FIELD("resist_[dir_top]", resist_[dir_top]);
  PRINT_FIELD("resist_[dir_up]", resist_[dir_up]);

  // PRINT source chip flp
  if(datalibrary_->all_layers_info_[layer]->chip_floorplan_ != NULL) {
    std::cerr << " ***** PRINTING source_chip_flp_->name_ for layer [" << layer << "]*****" << std::endl;
    std::cerr << "(L" << layer << ").ncols()=" << dyn_layer->ncols() << std::endl;
    std::cerr << "(L" << layer << ").nrows()=" << dyn_layer->nrows() << std::endl << std::endl;

    for(int y_itor = (int)dyn_layer->nrows() - 1; y_itor >= 0; y_itor--) {
      for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++) {
        if((*dyn_layer)[x_itor][y_itor].source_chip_flp_ != NULL)
          std::cerr << "[" << std::setw(6) << ((*dyn_layer)[x_itor][y_itor].source_chip_flp_)->name_ << "]   ";
      }
      std::cerr << std::endl;
    }
    std::cerr << std::endl << std::endl;
  }

  // PRINT solution data
  PRINT_FIELD("get_temperature()", get_temperature())

  // PRINT energy dat
  if(datalibrary_->all_layers_info_[layer]->chip_floorplan_ != NULL) {
    std::cerr << " ***** PRINTING energy_data_ for layer [" << layer << "]*****" << std::endl;
    std::cerr << "(L" << layer << ").ncols()=" << dyn_layer->ncols() << std::endl;
    std::cerr << "(L" << layer << ").nrows()=" << dyn_layer->nrows() << std::endl << std::endl;
    for(int y_itor = (int)dyn_layer->nrows() - 1; y_itor >= 0; y_itor--) {
      for(unsigned int x_itor = 0; x_itor < dyn_layer->ncols(); x_itor++) {
        if((*dyn_layer)[x_itor][y_itor].source_chip_flp_ != NULL) {
          if((*dyn_layer)[x_itor][y_itor].power_per_unit_area_ == -1)
            std::cerr << "[" << std::setw(6)
                      << (*dyn_layer)[x_itor][y_itor].source_chip_flp_->get_power_per_unit_area() *
                             (*dyn_layer)[x_itor][y_itor].x2_ * (*dyn_layer)[x_itor][y_itor].y2_ * (*dyn_layer)[x_itor][y_itor].z2_
                      << "]   ";
          else
            std::cerr << "[" << std::setw(6)
                      << (*dyn_layer)[x_itor][y_itor].power_per_unit_area_ * (*dyn_layer)[x_itor][y_itor].x2_ *
                             (*dyn_layer)[x_itor][y_itor].y2_ * (*dyn_layer)[x_itor][y_itor].z2_
                      << "]   ";
        }
      }
      std::cerr << std::endl;
    }
    std::cerr << std::endl << std::endl;
  }

  PRINT_FIELD("gamma_", gamma_);
}

// FIXME: pass vector<SUElement_t> as parameter, not as return value
std::vector<MATRIX_DATA> ModelUnit::compute_average_temps(int flp_layer, DynamicArray<ModelUnit> &dyn_array, int sample_type,
                                                          DataLibrary *datalibrary) {
  std::vector<MATRIX_DATA> temperature_map;
  std::vector<ModelUnit *> located_units;
  for(int i = 0; i < (int)datalibrary->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(); i++) {
    ChipFloorplan_Unit &flp_unit = datalibrary->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[i];
    ModelUnit::locate_model_units(dyn_array, located_units, flp_unit.leftx_, flp_unit.bottomy_, flp_unit.leftx_ + flp_unit.width_,
                                  flp_unit.bottomy_ + flp_unit.height_, datalibrary);

    MATRIX_DATA average_temp = 0;
    // compute average temperature for the units
    for(int j = 0; j < (int)located_units.size(); j++) {
      switch(sample_type) {
      case GFX_CUR:
        if(located_units[j]->get_temperature() != NULL) {
          average_temp += *located_units[j]->get_temperature();
        }
        break;
      case GFX_MIN:
        average_temp += located_units[j]->min_temperature_;
        break;
      case GFX_MAX:
        average_temp += located_units[j]->max_temperature_;
        break;
      case GFX_AVE:
        average_temp += located_units[j]->ave_temperature_;
        break;
      default:
        break;
      }
    }
    average_temp /= located_units.size();
    temperature_map.push_back(average_temp);
  }

  return (temperature_map);
}

std::vector<MATRIX_DATA> ModelUnit::compute_average_powers(int flp_layer, DynamicArray<ModelUnit> &dyn_array, int sample_type,
                                                           DataLibrary *datalibrary) {
  std::vector<MATRIX_DATA> power_map;
  std::vector<ModelUnit *> located_units;

  for(int i = 0; i < (int)datalibrary->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_.size(); i++) {
    ChipFloorplan_Unit &flp_unit = datalibrary->all_layers_info_[flp_layer]->chip_floorplan_->flp_units_[i];
    ModelUnit::locate_model_units(dyn_array, located_units, flp_unit.leftx_, flp_unit.bottomy_, flp_unit.leftx_ + flp_unit.width_,
                                  flp_unit.bottomy_ + flp_unit.height_, datalibrary);

    MATRIX_DATA average_energy = 0;
    // compute average power for the units
    for(int j = 0; j < (int)located_units.size(); j++) {
      switch(sample_type) {
      case GFX_CUR:
        average_energy += located_units[j]->energy_data_;
        break;
      case GFX_MIN:
        average_energy += located_units[j]->min_energy_data_;
        break;
      case GFX_MAX:
        average_energy += located_units[j]->max_energy_data_;
        break;
      case GFX_AVE:
        average_energy += located_units[j]->ave_energy_data_;
        break;
      default:
        break;
      }
    }
    average_energy /= located_units.size();
    power_map.push_back(average_energy);
  }

  return (power_map);
}

// searches for a base unit within a given layer with leftx/bottomy coordinates specified
// returns NULL is no element was found

ModelUnit *ModelUnit::find_model_unit(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary) {
  std::map<MATRIX_DATA, int>::iterator itr;
  std::map<MATRIX_DATA, int> &         coord_to_index_x = datalibrary->all_layers_info_[layer]->coord_to_index_x_;
  std::map<MATRIX_DATA, int> &         coord_to_index_y = datalibrary->all_layers_info_[layer]->coord_to_index_y_;

  int index_x = 0;
  int index_y = 0;

  if(LT(leftx, datalibrary->all_layers_info_[layer]->leftx_) ||
     GT(leftx, datalibrary->all_layers_info_[layer]->leftx_ + datalibrary->all_layers_info_[layer]->width_) ||
     LT(bottomy, datalibrary->all_layers_info_[layer]->bottomy_) ||
     GT(bottomy, datalibrary->all_layers_info_[layer]->bottomy_ + datalibrary->all_layers_info_[layer]->height_))
    return (NULL);

  if(!datalibrary->all_layers_info_[layer]->layer_used_)
    return (NULL);

  if((itr = find_if(coord_to_index_x.begin(), coord_to_index_x.end(), ValueEquals<MATRIX_DATA, int>(leftx))) ==
     coord_to_index_x.end()) {
    // for(itr = coord_to_index_x.begin(); itr!=coord_to_index_x.end(); itr++)
    //      cerr << "[" << itr->first << "][" << itr->second << "]" << endl;

    return (NULL);
  } else
    index_x = itr->second;

  if((itr = find_if(coord_to_index_y.begin(), coord_to_index_y.end(), ValueEquals<MATRIX_DATA, int>(bottomy))) ==
     coord_to_index_y.end())
    return (NULL);
  else
    index_y = itr->second;

  //      I(index_x <     (int)datalibrary->all_layers_info_[layer]->floorplan_layer_dyn_->nrows() && index_x>0 &&
  //              index_y < (int)datalibrary->all_layers_info_[layer]->floorplan_layer_dyn_->ncols() && index_y>0);

  return (&(*datalibrary->all_layers_info_[layer]->floorplan_layer_dyn_)[index_x][index_y]);
}

int ModelUnit::find_model_unit_xitor(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary) {
  std::map<MATRIX_DATA, int>::iterator itr;
  std::map<MATRIX_DATA, int> &         coord_to_index_x = datalibrary->all_layers_info_[layer]->coord_to_index_x_;

  int index_x = 0;
  if((itr = find_if(coord_to_index_x.begin(), coord_to_index_x.end(), ValueEquals<MATRIX_DATA, int>(leftx))) ==
     coord_to_index_x.end())
    return (-1);

  index_x = itr->second;
  return index_x;
}

int ModelUnit::find_model_unit_yitor(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary) {

  std::map<MATRIX_DATA, int>::iterator itr;
  std::map<MATRIX_DATA, int> &         coord_to_index_y = datalibrary->all_layers_info_[layer]->coord_to_index_y_;

  int index_y = 0;

  if((itr = find_if(coord_to_index_y.begin(), coord_to_index_y.end(), ValueEquals<MATRIX_DATA, int>(bottomy))) ==
     coord_to_index_y.end())
    return (-1);

  index_y = itr->second;
  return (index_y);
}

void ModelUnit::locate_model_units(int layer, std::vector<ModelUnit *> &located_units, MATRIX_DATA leftx, MATRIX_DATA bottomy,
                                   MATRIX_DATA rightx, MATRIX_DATA topy, DataLibrary *datalibrary) {
  if(!located_units.empty())
    located_units.clear();

  DynamicArray<ModelUnit> &layer_dyn = (*datalibrary->all_layers_info_[layer]->floorplan_layer_dyn_);

  for(unsigned int y_itor = 0; y_itor < layer_dyn.nrows(); y_itor++) {
    for(unsigned int x_itor = 0; x_itor < layer_dyn.ncols(); x_itor++) {
      if(!layer_dyn[x_itor][y_itor].defined_)
        continue;

      if(GE(layer_dyn[x_itor][y_itor].leftx_, leftx) && LE(layer_dyn[x_itor][y_itor].rightx_, rightx) &&
         GE(layer_dyn[x_itor][y_itor].bottomy_, bottomy) && LE(layer_dyn[x_itor][y_itor].topy_, topy))
        located_units.push_back(&(layer_dyn[x_itor][y_itor]));
    }
  }
}

void ModelUnit::locate_model_units(DynamicArray<ModelUnit> &dyn_array, std::vector<ModelUnit *> &located_units, MATRIX_DATA leftx,
                                   MATRIX_DATA bottomy, MATRIX_DATA rightx, MATRIX_DATA topy, DataLibrary *datalibrary) {
  if(!located_units.empty()) {
    located_units.clear(); // clear the vector
  }
  for(unsigned int y_itor = 0; y_itor < dyn_array.nrows(); y_itor++) {
    for(unsigned int x_itor = 0; x_itor < dyn_array.ncols(); x_itor++) {
      if(!dyn_array[x_itor][y_itor].defined_)
        continue;
      if(GE(dyn_array[x_itor][y_itor].leftx_, leftx) && LE(dyn_array[x_itor][y_itor].rightx_, rightx) &&
         GE(dyn_array[x_itor][y_itor].bottomy_, bottomy) && LE(dyn_array[x_itor][y_itor].topy_, topy))
        located_units.push_back(&dyn_array[x_itor][y_itor]);
    }
  }
}
