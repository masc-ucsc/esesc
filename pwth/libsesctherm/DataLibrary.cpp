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
File name:      DataLibrary.cpp
Description:    This handles the storage of all the model parameters that are shared
                throughout the modeling framework
********************************************************************************/

#include <complex>
#include <fstream>
#include <iostream>
#include <map>

#include "sescthermMacro.h"

#include "ClassDeclare.h"
#include "sesctherm3Ddefine.h"

#include "ChipLayers.h"
#include "ChipMaterial.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ModelUnit.h"
#include "RK4Matrix.h"
#include "ThermGraphics.h"
#include "Utilities.h"

#include "nanassert.h"

// const int DataLibrary::gate_pitches[5]={1932,2760,3900,5571,7959};

int DataLibrary::tech_metal_index(int layer) {
  return (TECH_POLY + 9 * layer);
}

/* FIXME: this should be put in a separate file (and integrated with ESESC, not possible with sesctherm)
   const MATRIX_DATA datalibrary::leakage_technology_parameters_[25][4] = {
   {180,	130,	100,	70},						//Process lithography (nm)
   {3.5E-9,	3.3E-9,	2.5E-9,	1.0E-9},				//oxide thickness
   {2.0,		1.5,	1.0,	0.9},					//ireg_voltage
   { 0.3979, 0.3353, 0.2607, 0.3979},					//ireg_threshold_Cell_N
   {0.4659, 0.3499,0.3030, 0.2130},					//ireg_threshold_Access_N
   {2.0,1.5,1.0,2.0},									//il1_voltage
   {2.0,1.5,1.0,2.0},									//il2_voltage
   {2.0,1.5,1.0,2.0},									//dl1_voltage
   {2.0,1.5,1.0,2.0},									//dl2_voltage
   {0.3979,0.3353,0.2607,0.1902},						//il1_threshold_Cell_N
   {0.4659,0.3499,0.3030,0.2130},						//il1_threshold_Cell_P
   {0.3979,0.3353,0.2607,0.1902},						//il1_threshold_Access_N
   {0.3979,0.3353,0.2607,0.1902},						//il2_threshold_Cell_N
   {0.4659,0.3499,0.3030,0.2130},						//il2_threshold_Cell_P
   {0.3979,0.3353,0.2607,0.1902},						//il2_threshold_Access_N

   {0.4659,0.3499,0.3030,0.2130},						//il1_threshold_P
   {0.4659,0.3499,0.3030,0.2130},						//il2_threshold_P
   {0.3979,0.3353,0.2607,0.1902},						//dl1_threshold_Cell_N
   {0.4659,0.3499,0.3030,0.2130},						//dl1_threshold_Cell_P
   {0.3979,0.3353,0.2607,0.1902},						//dl1_threshold_Access_N
   {0.4659,0.3499,0.3030,0.2130},						//dl2_threshold_P
   {0.4659,0.3499,0.3030,0.2130},						//dl1_threshold_P
   {0.3979,0.3353,0.2607,0.1902},						//dl2_threshold_Cell_N
   {0.4659,0.3499,0.3030,0.2130},						//dl2_threshold_Cell_P
   {0.3979,0.3353,0.2607,0.1902}						//dl2_threshold_Access_N
   }

 */

DataLibrary::DataLibrary(bool useRK4) {
  _rk4Matrix   = new sescthermRK4Matrix(1);
  config_data_ = new ConfigData(this);
  graphics_    = new ThermGraphics(this);

  previous_temperature_matrix_ = NULL;
  temperature_matrix_          = NULL;
  accumulate_rc_               = false;
  timestep_                    = 0.0;
  time_                        = 0.0;
  use_rc_                      = false;
}

DataLibrary::~DataLibrary() {
  I(_rk4Matrix);
  delete _rk4Matrix;
  delete config_data_;
}

// Top level functions to call/initialize appropriate solver
void DataLibrary::solve_matrix(MATRIX_DATA timestep, std::vector<ModelUnit *> &matrix_models) {
  I(_rk4Matrix);
  _rk4Matrix->solve_matrix(config_data_, temperature_matrix_, timestep, matrix_models);
}

/*
   TechnologyNode	hp250	hp180	hp130	hp90	hp65
   MPU/ASICPhysicalGateLength(nm)	199.5	140	100	38	25
   Starting silicon layer thickness PD (nm)	170.4082	132.7298	101.2898	76	50.5
   Starting silicon layer thickness FD (nm)	36.9997	20.1853	18.0853	14.3998	10.5
   Buried Oxide thickness (BOX) FD (nm)	206.0767	159.2903	120.2183	75	51
   Buried Oxide thickness (BOX) PD (nm)	150	150	150	150	150
   STI depth (SOI) (nm)	84.0954	64.7142	48.5714	30	20
   Trench width at top (nm)	179.4803	149.6467	123.1467	90	65
 */
// pmos=2*nmos

// FIXME: add 45nm parameters
// FIXME: add #wires for 250,180nm and poly layers
// FIXME: add #wires for INTEL/TSMC (only IBM current used, see technology parameters reference)
// FIXME: #wires should be scaled using sesctherm3Dinterconnect.cpp
const MATRIX_DATA DataLibrary::technology_parameters_[91][5] = {
    {250e-9, 180e-9, 130e-9, 90e-9, 65e-9},       // Process lithography (nm)
    {4643e-9, 3083e-9, 2000e-9, 1000e-9, 570e-9}, // 6-T SRAM Cell Size (nm)
    {2, 1.8, 1.5, 1.057, 0.884},                  // Vdd (V)
    {199.5e-9, 140e-9, 100e-9, 38e-9, 25e-9},     // Gate length of average-sized nmos/pmos (nm) (2xminimum)
    {11440e-9, 824e-9, 596e-9, 414e-9, 300e-9},   // Gate width of average-sized pmos (2xminimum)
    {5720e-9, 412e-9, 298e-9, 207e-9, 150e-9},    // Gate width of average-sized nmos (2xminimum)
    {160, 100, 70, 46.571, 29.844},               // Leff (nm)
    {60, 45, 35, 24.472, 18.7260},                // tox(Angstroms = 1.0x10-10 meters)
    {6, 6, 7, 8, 9},                              // levels (metal layers, includes M0 (POLY) )

    // interlayer-dielectric constant k_ild
    // Taken from (1) Thermal Scaling analysis of multilevel cu/low-k interconnect structures
    // (2) A 90nm Logic technology featuring 50nm strained silicon channel transistors, ...
    // (3) An enhanced 130nm generation logic technology featuring 60nm transistors optimied for high performance and...

    {5.81, 4.53, 3.6, 2.9, 2.4}, // k_ild

    // Poly
    {289e-9, 217e-9, 160e-9, 140e-9, 90e-9},                           // H
    {502e-9, 395e-9, 319e-9, 260e-9, 220e-9},                          // W
    {502e-9, 395e-9, 319e-9, 260e-9, 220e-9},                          // Space
    {1589.688171, 1547.624512, 393.9827542, 688.7825641, 839.7005831}, // wire resistance
    {0.140922039, 0.275407057, 0.317581512, 0.323601485, 0.327612261}, // wire capacitance
    {4.564777016, 2.922899361, 6.107996587, 4.34055825, 3.722697363},  // sopt
    {322e-9, 235e-9, 97e-9, 72e-9, 30e-9},                             // tins
    {400e-9, 260e-9, 140e-9, 100e-9, 70e-9},                           // inter-via distances
    {0, 0, 0, 0, 0},                                                   // number of wires

    // Metal 1
    {389e-9, 304e-9, 280e-9, 150e-9, 170e-9},                          // H
    {445e-9, 353e-9, 293e-9, 220e-9, 210e-9},                          // W
    {445e-9, 353e-9, 293e-9, 220e-9, 210e-9},                          // Space
    {1540.715952, 1500.414608, 384.6548897, 669.1671877, 682.068109},  // wire resistance
    {0.14493629, 0.279517033, 0.319996725, 0.325661893, 0.329137069},  // wire capacitance
    {4.572093864, 2.946621908, 6.158239884, 4.389763505, 4.120953955}, // sopt
    {434e-9, 329e-9, 169e-9, 77e-9, 57e-9},                            // tins
    {299e-9, 186e-9, 64e-9, 63e-9, 63e-9},                             // inter-via distances
    {0, 0, 26250594, 51954488, 0},                                     // number of wires

    // Metal 2
    {467e-9, 378e-9, 360e-9, 256e-9, 190e-9},                          // H
    {760e-9, 560e-9, 425e-9, 320e-9, 210e-9},                          // W
    {760e-9, 560e-9, 425e-9, 320e-9, 210e-9},                          // Space
    {1540.715952, 1500.414608, 384.6548897, 669.1671877, 682.068109},  // wire resistance
    {0.14493629, 0.279517033, 0.319996725, 0.325661893, 0.329137069},  // wire capacitance
    {4.572093864, 2.946621908, 6.158239884, 4.389763505, 4.120953955}, // sopt
    {521e-9, 409e-9, 217e-9, 131e-9, 64e-9},                           // tins
    {1498e-9, 968e-9, 442e-9, 196e-9, 63e-9},                          // inter-via distances
    {0, 0, 1995879, 4629072, 0},                                       // number of wires

    // Metal 3
    {460e-9, 375e-9, 360e-9, 256e-9, 200e-9},                         // H
    {743e-9, 552e-9, 425e-9, 320e-9, 220e-9},                         // W
    {743e-9, 552e-9, 425e-9, 320e-9, 220e-9},                         // Space
    {107, 107, 188, 414.5524582, 627.9467305},                        // wire resistance
    {0.202, 0.333, 0.336, 0.335053232, 0.335761407},                  // wire capacitance
    {14.69594182, 10.10927913, 8.596396413, 5.49851306, 4.252293599}, // sopt
    {513e-9, 406e-9, 217e-9, 131e-9, 67e-9},                          // tins
    {1373e-9, 908e-9, 442e-9, 196e-9, 70e-9},                         // inter-via distances
    {0, 0, 572524, 1270748, 0},                                       // number of wires

    // Metal 4
    {766e-9, 592e-9, 570e-9, 320e-9, 250e-9},                         // H
    {1368e-9, 961e-9, 718e-9, 400e-9, 280e-9},                        // W
    {1368e-9, 961e-9, 718e-9, 400e-9, 280e-9},                        // Space
    {107, 107, 188, 414.5524582, 627.9467305},                        // wire resistance
    {0.202, 0.333, 0.336, 0.335053232, 0.335761407},                  // wire capacitance
    {14.69594182, 10.10927913, 8.596396413, 5.49851306, 4.252293599}, // sopt
    {854e-9, 640e-9, 344e-9, 164e-9, 84e-9},                          // tins
    {33628e-9, 23606e-9, 10573e-9, 482e-9, 136e-9},                   // inter-via distance
    {0, 0, 208671, 456388, 0},                                        // number of wires

    // Metal 5
    {1271e-9, 935e-9, 900e-9, 384e-9, 300e-9},                         // H
    {2167e-9, 1470e-9, 1064e-9, 480e-9, 330e-9},                       // W
    {2167e-9, 1470e-9, 1064e-9, 480e-9, 330e-9},                       // space
    {161.2217667, 160.5662844, 68.29904873, 83.2515535, 187.3168749},  // wire resistance
    {0.193670237, 0.325685993, 0.346020375, 0.347706972, 0.348226762}, // wire capacitance
    {12.22705083, 8.344637493, 14.0542228, 12.04450737, 7.645053143},  // sopt
    {1416e-9, 1012e-9, 543e-9, 197e-9, 101e-9},                        // tins
    {2006481e-9, 1361111e-9, 449548e-9, 1184e-9, 238e-9},              // inter-via distance
    {0, 0, 85245, 188980, 0},                                          // number of wires

    // Metal 6
    {0e-9, 0e-9, 1200e-9, 576e-9, 430e-9},         // H
    {0e-9, 0e-9, 1143e-9, 720e-9, 480e-9},         // W
    {0e-9, 0e-9, 1143e-9, 720e-9, 480e-9},         // space
    {0, 0, 68.29904873, 83.2515535, 187.3168749},  // wire resistance
    {0, 0, 0.346020375, 0.347706972, 0.348226762}, // wire capacitance
    {0, 0, 14.0542228, 12.04450737, 7.645053143},  // sopt
    {0e-9, 0e-9, 724e-9, 295e-9, 145e-9},          // tins
    {0e-9, 0e-9, 1058333e-9, 17535e-9, 1262e-9},   // inter-via distance
    {0, 0, 36318, 49918, 0},                       // number of wires

    // Metal 7
    {0e-9, 0e-9, 0e-9, 972e-9, 650e-9},       // H
    {0e-9, 0e-9, 0e-9, 1080e-9, 720e-9},      // W
    {0e-9, 0e-9, 0e-9, 1080e-9, 720e-9},      // space
    {0, 0, 0, 4.687560857, 18.20896974},      // wire resistance
    {0, 0, 0, 0.357893674, 0.358988515},      // wire capacitance
    {0, 0, 0, 50.03126038, 24.14999628},      // sopt
    {0e-9, 0e-9, 0e-9, 498e-9, 219e-9},       // tins
    {0e-9, 0e-9, 0e-9, 1000000e-9, 18228e-9}, // inter-via distance
    {0, 0, 0, 60162, 0},                      // number of wires

    // Metal 8
    {0e-9, 0e-9, 0e-9, 0e-9, 975e-9},         // H
    {0e-9, 0e-9, 0e-9, 0e-9, 1080e-9},        // W
    {0e-9, 0e-9, 0e-9, 0e-9, 1080e-9},        // space
    {0, 0, 0, 0, 18.20896974},                // wire resistance
    {0, 0, 0, 0, 0.358988515},                // wire capacitance
    {0e-9, 0e-9, 0e-9, 0e-9, 24.14999628e-9}, // sopt
    {0e-9, 0e-9, 0e-9, 0e-9, 329e-9},         // tins
    {0, 0, 0, 0, 1000000},                    // inter-via distance
    {0, 0, 0, 0, 0}                           // number of wires
};

/* Most of the technology parameters were taken from:
   "Investigation of Performance Metrics For DSM Interconnect"
http://www.eecs.umich.edu/~kimyz/interconnect.htm

65nm gate pitch = 1932nm
90nm gate pitch = 2760nm
130nm gate pitch = 3900nm
180nm gate pitch = 5571nm
250nm gate pitch = 7959nm
This represents the average gate-to-gate distance (not to be confused with transitor-to-transistor distance)

Krishnan et al and Vikas et al were used


Further data values were taken from:'Accurate Energy Dissipation and Thermal Modeling for Nanometer-Scale Buses', Krishnan
Sundaresan and Nihar R. Mahapatra Further data values were taken from: The Effect of Technology Scaling on Microarchitectural
Structures, Vikas Agarwal, Stephen W. Keckler, Doug Burger


 * lcrit and sopt values for intermediate layer
 * are derived from the values for global layer.
 * lcrit * sqrt(wire_r * wire_c) is a constant
 * for a given layer. So is the expression
 * sopt * sqrt(wire_r / wire_c) (Equation 2 and
 * Theorem 2 from Brayton et. al). Using this
 * for the global layer, one can find the constants
 * for the intermediate layer and combining this
 * with the wire_r and wire_c values for the
 * intermediate layer, its lcrit and sopt values
 * can be calculated.



 *	We interpolate the data values for the wire resistances and the capacitances for each layer.
 *	This is done assuming that the wire ressitances are proportional to the cross-sectional area of the wire.
 *	Further, we assume that the wire capacitance is a function of the height of the wire
 *
 */

DynamicArray<ModelUnit> &DataLibrary::get_dyn_array(int layer) {
  if(layer < 0 || layer >= (int)all_layers_info_.size()) {
    std::cerr << "FATAL: ThermModel::get_dyn_array =>  layer[ " << layer << " ] does not exist" << std::endl;
    exit(1);
  }
  return (*(all_layers_info_[layer]->floorplan_layer_dyn_));
}

// this will set the pointers of the model unit's coefficient pointer to point to the location in the unsolved matrix
void DataLibrary::initialize_unsolved_matrix_row(int y_itor_global, int layer, int x_itor, int y_itor, bool set_pointer) {
  DynamicArray<ModelUnit> &temp_floorplan_dyn = get_dyn_array(layer);
  ModelUnit &              mu                 = temp_floorplan_dyn[x_itor][y_itor];

  // self = Tm,n,o
  store_pointer(find_unsolved_matrix_row_index(layer, x_itor, y_itor), y_itor_global, mu.t_mno, set_pointer);

  // dir_left = Tm-1,n,o
  if(!EQ(mu.x1_, -1.0) && !EQ(mu.x1_, 0.0))
    store_pointer(find_unsolved_matrix_row_index(layer, x_itor - 1, y_itor), y_itor_global, mu.t_ptr[dir_left], set_pointer);
  // dir_right = Tm+1,n,o
  if(!EQ(mu.x3_, -1.0) && !EQ(mu.x3_, 0.0))
    store_pointer(find_unsolved_matrix_row_index(layer, x_itor + 1, y_itor), y_itor_global, mu.t_ptr[dir_right], set_pointer);
  // dir_top = Tm,n+1,o
  if(!EQ(mu.y3_, -1.0) && !EQ(mu.y3_, 0.0)) {
    // skip the UCOOL layer
    if(all_layers_info_[layer + 1]->type_ == UCOOL_LAYER) {
      I(0);
    } else {
      int x_itor_tmp = ModelUnit::find_model_unit_xitor(mu.leftx_, mu.bottomy_, layer + 1, this);
      int y_itor_tmp = ModelUnit::find_model_unit_yitor(mu.leftx_, mu.bottomy_, layer + 1, this);

      store_pointer(find_unsolved_matrix_row_index(layer + 1, x_itor_tmp, y_itor_tmp), y_itor_global, mu.t_ptr[dir_top],
                    set_pointer);
    } // not UCOOL layer
  }
  // dir_bottom = Tm,n-1,o
  if(!EQ(mu.y1_, -1.0) && !EQ(mu.y1_, 0.0)) {
    if(layer == 0) {
      std::cerr << "FATAL: ThermModel::create_unsolved_matrix_row: Tm,n-1,o MUST be 0 on Layer" << layer << "(and it's not!)"
                << std::endl;
      exit(1);
    }
    // skip the UCOOL layer
    if(all_layers_info_[layer - 1]->type_ == UCOOL_LAYER) {
      I(0);
    } else {
      int x_itor_tmp = ModelUnit::find_model_unit_xitor(mu.leftx_, mu.bottomy_, layer - 1, this);
      int y_itor_tmp = ModelUnit::find_model_unit_yitor(mu.leftx_, mu.bottomy_, layer - 1, this);

      store_pointer(find_unsolved_matrix_row_index(layer - 1, x_itor_tmp, y_itor_tmp), y_itor_global, mu.t_ptr[dir_bottom],
                    set_pointer);
    }
  }

  // dir_down = Tm,n,o-1
  if(!EQ(mu.z1_, -1.0) && !EQ(mu.z1_, 0.0))
    store_pointer(find_unsolved_matrix_row_index(layer, x_itor, y_itor - 1), y_itor_global, mu.t_ptr[dir_down], set_pointer);
  // dir_up = Tm,n,o+1
  if(!EQ(mu.z3_, -1.0) && !EQ(mu.z3_, 0.0))
    store_pointer(find_unsolved_matrix_row_index(layer, x_itor, y_itor + 1), y_itor_global, mu.t_ptr[dir_up], set_pointer);
}

// This will initialize the unsolved matrix (allocate it)
// Further, this will set the t_mn_o-type coefficient pointers to point to the correct location in the unsolved matrix
void DataLibrary::initialize_unsolved_matrix() {
  int                        y_itor_global   = 0;
  std::vector<ChipLayers *> &all_layer_infos = all_layers_info_;

  // Find total number of nodes from the no. of units in each layer.
  int global_size = 0;
  for(int layer = 0; layer < (int)all_layer_infos.size(); layer++) {
    ChipLayers *layer_info = all_layer_infos[layer];
    if(layer_info->layer_used_) {
      global_size +=
          (layer_info->floorplan_layer_dyn_->ncols() * layer_info->floorplan_layer_dyn_->nrows()) - layer_info->unused_dyn_count_;
    }
  }

  num_locktemp_rows_ = 0;
  // increase the global size by the number of nodes with temp locking enabled
  for(unsigned int layer = 0; layer < all_layer_infos.size(); layer++) {
    ChipLayers *layer_info = all_layer_infos[layer];
    if(layer_info->layer_used_ && layer_info->temp_locking_enabled_) {
      num_locktemp_rows_ +=
          (layer_info->floorplan_layer_dyn_->ncols() * layer_info->floorplan_layer_dyn_->nrows()) - layer_info->unused_dyn_count_;
    }
  }

  // resize the unsolved_model_dyn_ to have the correct number of rows and columns
  // delete unsolved_matrix_dyn_->clear(); //clear out of the model

  if(_rk4Matrix)
    _rk4Matrix->realloc_matrices(global_size);

  for(unsigned int layer = 0; layer < all_layer_infos.size(); layer++) {
    ChipLayers *layer_info = all_layer_infos[layer];
    if(layer_info->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          ModelUnit &munit = get_dyn_array(layer)[x_itor][y_itor];
          if(!munit.defined_)
            continue; // skip if unit is not defined

          // allocate space within the unsolved matrix
          initialize_unsolved_matrix_row(y_itor_global, layer, x_itor, y_itor, false);
          y_itor_global++;
        }
    } // if layer is used
  }   // for each layer

  y_itor_global = 0;
  for(unsigned int layer = 0; layer < all_layer_infos.size(); layer++) {
    ChipLayers *layer_info = all_layer_infos[layer];
    if(layer_info->layer_used_) {
      for(unsigned int y_itor = 0; y_itor < get_dyn_array(layer).nrows(); y_itor++)
        for(unsigned int x_itor = 0; x_itor < get_dyn_array(layer).ncols(); x_itor++) {
          if(get_dyn_array(layer)[x_itor][y_itor].defined_ == false)
            continue; // skip if unit is not defined

          // set the model pointers to the various elements
          initialize_unsolved_matrix_row(y_itor_global, layer, x_itor, y_itor, true);
          y_itor_global++;
        } // x_itor
    }     // if layer is used
  }       // for each layer
}

void DataLibrary::store_pointer(int x_coord, int y_coord, SUElement_t *&pointer_location, bool set_pointer) {
  if(_rk4Matrix) {
    if(set_pointer)
      pointer_location = &(_rk4Matrix->unsolved_matrix_dyn_[y_coord][x_coord]);
    else
      _rk4Matrix->unsolved_matrix_dyn_[y_coord][x_coord] = -1.0;
  }
}

// FIXME:  I'm currently designating unused nodes to the vacant regions in between the fins!
// These nodes should be removed (save memory and calculation time)
// return the index in the unsolved_matrix_dyn row given the layer, x_coord and y_coord of the model unit index
int DataLibrary::find_unsolved_matrix_row_index(int layer, int x_coord, int y_coord) {
  return (get_dyn_array(layer)[x_coord][y_coord]).temperature_index_;
}

// Put here to avoid declaring RK4 in header
void DataLibrary::set_timestep(double step) {
  if(_rk4Matrix)
    _rk4Matrix->set_timestep(step);
}
