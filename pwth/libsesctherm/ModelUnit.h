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
File name:      ModelUnit.h
Classes:        ModelUnit
********************************************************************************/

#ifndef MODEL_UNIT_H
#define MODEL_UNIT_H

#include <vector>

#include "sesctherm3Ddefine.h"

class ChipFloorplan_Unit;
class DataLibrary;
template <class T> class DynamicArray;

enum { dir_left = 0, dir_right = 1, dir_bottom = 2, dir_top = 3, dir_down = 4, dir_up = 5, MAX_DIR = 6 };

class ModelUnit {
public:
  ModelUnit() {
  }

  SUElement_t *                   get_temperature();
  SUElement_t *                   get_previous_temperature();
  static std::vector<MATRIX_DATA> compute_average_temps(int flp_layer, DynamicArray<ModelUnit> &dyn_array, int sample_type,
                                                        DataLibrary *datalibrary);
  static std::vector<MATRIX_DATA> compute_average_powers(int flp_layer, DynamicArray<ModelUnit> &dyn_array, int sample_type,
                                                         DataLibrary *datalibrary);

  void initialize(DataLibrary *datalibrary);
  ModelUnit(DataLibrary *datalibrary) {
    initialize(datalibrary);
  }

  MATRIX_DATA       calc_view_factor(int dir);
  static void       print_dyn_layer(int layer, DataLibrary *datalibrary_);
  static void       print_dyn_equations(int layer, DataLibrary *datalibrary_);
  static ModelUnit *find_model_unit(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary_);

  void        print_rk4_equation(MATRIX_DATA power);
  static void locate_model_units(int layer, std::vector<ModelUnit *> &located_units, MATRIX_DATA leftx, MATRIX_DATA bottomy,
                                 MATRIX_DATA rightx, MATRIX_DATA topy, DataLibrary *datalibrary);

  static void locate_model_units(DynamicArray<ModelUnit> &dyn_array, std::vector<ModelUnit *> &located_units, MATRIX_DATA leftx,
                                 MATRIX_DATA bottomy, MATRIX_DATA rightx, MATRIX_DATA topy, DataLibrary *datalibrary);

  MATRIX_DATA calc_alpha_max(bool use_rc);
  void        accumulate_rc();
  void        compute_rc();
  static int  find_model_unit_xitor(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary_);
  static int  find_model_unit_yitor(MATRIX_DATA leftx, MATRIX_DATA bottomy, int layer, DataLibrary *datalibrary_);
  void        set_power(MATRIX_DATA power);

  void operator=(int value) {
  }

  // calulate all the term values given the equation number
  void print_coeffs();
  void calculate_terms(MATRIX_DATA time_increment, bool calc_solution_only, bool use_rc);
  void calc_gamma(MATRIX_DATA time_increment, bool use_urc);

  // void calc_hot_spot_count();
  // void calc_hot_spot_duration();
  // calculate R_th/2

  // data values
  int x_coord_;
  int y_coord_;

  DataLibrary *datalibrary_;

  // this is a reference to the source chip flp that this modelunit is a part of
  ChipFloorplan_Unit *source_chip_flp_;
  bool                defined_; // is this modelunit used?
  //  string name_;            //name of region that unit belongs to

  // percentage of the region's area that the unit belongs to. Note: this does need to be used when calculating energy data
  MATRIX_DATA flp_percentage_;

  // power data
  MATRIX_DATA energy_data_; // this is the energy data for the given call to the solver
  // solution data
  MATRIX_DATA lock_temp_;
  bool        temp_locking_enabled_;

  SUElement_t **temperature_;          // this is the current temperature (points to another array)
  SUElement_t **previous_temperature_; // this was the previously calculated result (initially the steady-temp value)
  MATRIX_DATA   sample_temperature_;   // this is a local storage of the temperature (only used in the graphics subsystem)
  // this is used to make the algorithm simpler
  int temperature_index_;

  // gamma
  MATRIX_DATA gamma_;
  // row
  MATRIX_DATA row_;

  // specific heat (Cp) or (C)
  MATRIX_DATA specific_heat_;
  MATRIX_DATA specific_heat_eff_;

  // material conductivities
  MATRIX_DATA conduct_[MAX_DIR];

  // These are the conductivities for the central region of the model unit
  // However, we have separate conductivities for each direction.
  // The effective conductivity would, of course, be the average of these values
  // However, we can obtain greater accurracy by splitting this up into separate values
  // This makes sense as the lateral resistance of a given layer may be very different than
  // the vertical resistance

  MATRIX_DATA conduct_center_[MAX_DIR];

  MATRIX_DATA resist_[MAX_DIR];

  // dimensions
  MATRIX_DATA x1_;
  MATRIX_DATA x2_;
  MATRIX_DATA x3_;
  MATRIX_DATA y1_;
  MATRIX_DATA y2_;
  MATRIX_DATA y3_;
  MATRIX_DATA z1_;
  MATRIX_DATA z2_;
  MATRIX_DATA z3_;

  MATRIX_DATA leftx_;
  MATRIX_DATA rightx_;
  MATRIX_DATA topy_;
  MATRIX_DATA bottomy_;
  MATRIX_DATA width_;
  MATRIX_DATA height_;

  //	MATRIX_DATA hot_spot_count_;
  //	MATRIX_DATA hot_spot_duration_;
  //	bool not_hot_spot_;

  // the governing equation for the given model unit
  // Equation 1 is basic heat transfer to surrounding solid or air flow
  // Equation 2 is for microcooler cold side
  // Equation 3 is for microcooler inner layer
  // Equation 4 is for microcooler hot side
  // Equation 5 is for chip pins
  // Equation 6 is for conducting heat through thermal grease
  // Equation 7 is for conducting heat through thermal crystals
  // Equation 8 is for conducting heat through saffire
  // Equation 9 is for conducting heat through water flow

  MATRIX_DATA governing_equation_;

  // the value for each of the terms for the governing equation
  // Tm,n,o
  SUElement_t *t_mno; // self coefficient

  // directions are dir_left, dir_right, dir_bottom , dir_top , dir_down , dir_up
  // { t_m_M1_no, t_m_P1_no, t_mn_M1_o, t_mn_P1_o,  t_mno_M1, t_mno_P1 } ;
  SUElement_t *t_ptr[MAX_DIR];
  // MATRIX_DATA dir_coords[MAX_DIR] = { x1_ , x3_ , y1_, y3_ , z1_ , z3_ } ;

  ModelUnit *model_[MAX_DIR];
  // These are binary values, use DEFINES: HEAT_CONVECTION_TRANSFER, HEAT_CONDUCTION_TRANSFER, HEAT_EMMISION_TRANSFER for transfer
  // types
  unsigned int heat_transfer_methods_[MAX_DIR];
  unsigned int heat_transfer_methods_center_;

  SUElement_t power_per_unit_area_;
  SUElement_t total_power();
  SUElement_t volume() const {
    return (x2_ * y2_ * z2_);
  }

  SUElement_t convection_coefficient_;
  SUElement_t emissivity_;

  // this is used for the sampling subsystem
  SUElement_t ave_temperature_;
  SUElement_t max_temperature_;
  SUElement_t min_temperature_;
  SUElement_t ave_energy_data_;
  SUElement_t max_energy_data_;
  SUElement_t min_energy_data_;

  // vector<MATRIX_DATA> datapoints_x_;
  // vector<MATRIX_DATA> datapoints_y_;
};

#endif
