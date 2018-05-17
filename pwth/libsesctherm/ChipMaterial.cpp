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
File name:      ChipMaterial.cpp
Description:    This is the materials library that synthesizes lumped thermal
                parameters for a variety of materials, which can then be
                assigned to given layers
********************************************************************************/

// THermal response is too fast compared with reality. We add a capacitance
// factor to the die_transistor, metal, and bulk
#if 1
#define CAPACITANCE_FACTOR 50
#define CAPACITANCE_FACTOR_INTERCONNECT 5
#define CAPACITANCE_FACTOR_BULK 5
#else
#define CAPACITANCE_FACTOR 1
#define CAPACITANCE_FACTOR_INTERCONNECT 1
#define CAPACITANCE_FACTOR_BULK 1
#endif

#include <complex>
#include <fstream>
#include <iostream>
#include <map>

#include "sescthermMacro.h"

#include "ClassDeclare.h"
#include "sesctherm3Ddefine.h"

#include "ChipFloorplan.h"
#include "ChipLayers.h"
#include "ChipMaterial.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ModelUnit.h"
#include "RegressionLine.h"
#include "Utilities.h"

static inline double double_pow(double a, double b) {
  return pow(a, b);
}

// MATERIAL_LIST CLASS
void Material_List::create(std::string &name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity,
                           MATRIX_DATA emissivity) {
  Material *mat = new Material(name, density, spec_heat, conductivity, emissivity);

  ASSERT_ESESCTHERM(storage.find(name) == storage.end(),
                    "Error: MATRIX_DATA insertion of same material"
                        << "name",
                    "Material_List", "create");

  storage[name] = mat;
}

Material &Material_List::find(std::string name) {
  std::map<std::string, Material *>::iterator it = storage.find(name);

  ASSERT_ESESCTHERM(it != storage.end(), "Error: could not find material " << name << " within config file", "Material_List",
                    "find");
  return (*it->second);
}

// MATERIAL CLASS
Material::Material(std::string &name, MATRIX_DATA density, MATRIX_DATA spec_heat, MATRIX_DATA conductivity,
                   MATRIX_DATA emissivity) {
  name_         = name;
  density_      = density;
  spec_heat_    = spec_heat;
  conductivity_ = conductivity;
  emissivity_   = emissivity;
}

// Note: material properties change with temperature
// FIXME: we should add the transistor layer too!, this will change with temperature as well
// However, for this we need to find out (1) how BOX is effected by temperature (2) how silicon island is effected by temperature
// and
// (3) effective capacitance is changed with temperature
void Material::calc_material_properties_dyn_temp(int layer, int x, int y, DataLibrary *datalibrary_) {
  switch(datalibrary_->all_layers_info_[layer]->type_) {
  case BULK_SI_LAYER:
    Material::calc_silicon_layer_properties_dyn(layer, x, y, datalibrary_);
    break;
  case AIR_LAYER:
    Material::calc_air_layer_properties_dyn(layer, x, y, datalibrary_);
    break;
  default:
    break;
  }
}

// this updates the material properties of the particular dynarray element
// this is done to reflect the varying densities of the floorplan units
void Material::calc_material_properties_dyn_density(int layer, int x, int y, DataLibrary *datalibrary_) {
  // there are two layers that are effected by density
  // these are the die_transistor_layer,  interconnect_layer
  switch(datalibrary_->all_layers_info_[layer]->type_) {
  case DIE_TRANSISTOR_LAYER:
    Material::calc_die_transistor_layer_properties_dyn(layer, x, y, datalibrary_);
    break;
  case INTERCONNECT_LAYER:
    Material::calc_interconnect_layer_properties_dyn(layer, x, y, datalibrary_);
    break;
  default:
    break; // otherwise don't recompute
  }
}

void Material::calc_silicon_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_) {
  // TODO: This layer assumes a constant Silicon (Bulk). A depleted silicon has
  // a few um??, in a fully depleted system, it should be inmmediately under
  // the transistor layer.

  ModelUnit & munit       = (*datalibrary_->all_layers_info_[layer]->floorplan_layer_dyn_)[x][y];
  MATRIX_DATA temperature = *(munit.get_temperature());

  // Silicon conductivity is temperature-dependent
  // 117.5 - 0.42*(T-100) is what is typically used (FLOTHERM)

  MATRIX_DATA new_v = 117.5 - 0.42 * (temperature - 273.15 - 100);

  for(int dir = 0; dir < MAX_DIR; dir++)
    munit.conduct_center_[dir] = new_v;
}

// To compensate for the varying densities in the base units, we scale the total number of transistors in the design.
// Note: we enforce that the max transistor density NEVER exceeds
void Material::calc_die_transistor_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_) {
  ConfigData *conf      = datalibrary_->config_data_;
  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];
  ModelUnit & munit     = (*layerInfo->floorplan_layer_dyn_)[x][y];

  ChipFloorplan_Unit *source_chip_flp = munit.source_chip_flp_;
  MATRIX_DATA         unit_width      = munit.x2_;

  MATRIX_DATA unit_height    = munit.z2_;
  MATRIX_DATA unit_thickness = munit.y2_;

  MATRIX_DATA chip_thickness = layerInfo->thickness_;

  MATRIX_DATA transistor_width    = conf->sample_gate_width_;
  MATRIX_DATA silicon_film_length = conf->sample_silicon_film_length_;

  MATRIX_DATA t_box           = conf->sample_box_thickness_;
  MATRIX_DATA k_si            = conf->sample_silicon_island_conductivity_;
  MATRIX_DATA p_ox_c_ox       = conf->sample_thermal_capacity_oxide_;
  MATRIX_DATA p_si_c_si       = conf->sample_thermal_capacity_silicon_;
  MATRIX_DATA num_transistors = conf->transistor_count_;
  int         technology      = conf->technology_; // get the technology used

  MATRIX_DATA pmos_transistor_size = DataLibrary::technology_parameters_[TECH_GATE_L_AVE][technology] *
                                     DataLibrary::technology_parameters_[TECH_GATE_PMOS_W_AVE][technology]; // p_mos area
  MATRIX_DATA nmos_transistor_size = DataLibrary::technology_parameters_[TECH_GATE_L_AVE][technology] *
                                     DataLibrary::technology_parameters_[TECH_GATE_NMOS_W_AVE][technology]; // n_mos area

  MATRIX_DATA t_sub    = conf->sample_silicon_substrate_thickness_;
  MATRIX_DATA t_si     = conf->sample_silicon_film_thickness_;
  MATRIX_DATA k_ox_eff = conf->sample_oxide_conductivity_transistor_;
  MATRIX_DATA k_box    = conf->sample_oxide_conductivity_;

  MATRIX_DATA t_interconnect = 0;
  int         num_layers     = (int)DataLibrary::technology_parameters_[TECH_LEVELS][technology];

  for(int i = 0; i < num_layers; i++) {
    t_interconnect = DataLibrary::technology_parameters_[10 + (i * 9) + TECH_H]
                                                        [technology]; // get the thickness of the layer for the given technology
  }

  MATRIX_DATA percentage_box          = t_box / (t_sub + t_box + t_si + t_interconnect);
  MATRIX_DATA box_thickness_actual    = chip_thickness * percentage_box;
  MATRIX_DATA percentage_silicon_film = t_si / (t_sub + t_box + t_si + t_interconnect);
  MATRIX_DATA silicon_film_thickness  = chip_thickness * percentage_silicon_film;

  // we have to compute the density value used as the scaling factor
  // We cannot have more than N transistors on the chip, the density numbers determine how the
  // available transistors are distributed.
  // this is done as follows
  // SUM(i, 0, N)(flp_density_i*area_i*base_density)=Total number of transistors
  // then model unit density equals source_chip_flp->density*base_density (transistors/area^2)

  std::vector<ChipFloorplan_Unit> &flp_units = (*layerInfo->chip_floorplan_).flp_units_;

  MATRIX_DATA total = 0;
  for(int i = 0; i < (int)flp_units.size(); i++) {
    total += flp_units[i].area_ * flp_units[i].chip_density_;
  }

  MATRIX_DATA base_density = num_transistors / total; // this is the density used as a baseline, all the values are scaled from this

  // compute effective density (transistors/m^2)
  MATRIX_DATA transistor_density   = source_chip_flp->chip_density_ * base_density;
  MATRIX_DATA interconnect_density = source_chip_flp->interconnect_density_;

  num_transistors =
      transistor_density * unit_width * unit_height; // this is the number of transistors for the cross-sectional region

  // compute the conductivity from the chip to interconnect
  // R_top=interlayer_resistance (compute)
  MATRIX_DATA conduct_bottom = Material::calc_effective_interconnect_layer_vertical_conductivity(
      0, interconnect_density, datalibrary_); // Layer 0, default density

  // STORE THE VERTICAL CONDUCTIVITY DOWNWARDS
  munit.conduct_center_[dir_bottom] = conduct_bottom;

  // compute the resistance from the transistor through the BOX and FOX to the silicon substrate
  // R_bottom=R_box=[t_box/(k_ox_eff*width)](L_si)
  MATRIX_DATA transistor_vertical_resistance_down = (box_thickness_actual / (k_ox_eff * transistor_width * silicon_film_length));

  // k_vertical=l/R_bottom*A=(t_si+t_box)/R_bottom*transistor_width*L_si;
  MATRIX_DATA transistor_conductivity_vertical = (silicon_film_thickness + box_thickness_actual) /
                                                 (transistor_vertical_resistance_down * transistor_width * silicon_film_length);

  // R_lateral=L_si/[width*(k_ox_eff(t_box/3) + k_si*t_si)]
  MATRIX_DATA transistor_lateral_resistance =
      silicon_film_length / (transistor_width * (k_ox_eff * (box_thickness_actual / 3) + k_si * silicon_film_thickness));

  // k_lateral=L_si/(R_lateral*transistor_width*(t_si+t_box))
  MATRIX_DATA transistor_conductivity_lateral =
      silicon_film_length / (transistor_lateral_resistance * transistor_width * (silicon_film_thickness + box_thickness_actual));

  // C_eff=(L_si)(width)[p_ox*c_ox*t_box/3+ p_si*c_si*t_si]=density*spec_heat*volume
  MATRIX_DATA effective_capacitance =
      silicon_film_length * transistor_width * (p_ox_c_ox * (box_thickness_actual / 3) + p_si_c_si * silicon_film_thickness);

  effective_capacitance *= CAPACITANCE_FACTOR;

  // transistor density*transistor_specific_heat=C_eff/[(t_si + t_box/3)(L_si)(width)]

  MATRIX_DATA transistor_p_eff_c_eff =
      effective_capacitance / ((silicon_film_thickness + box_thickness_actual / 3) * (silicon_film_length) * (transistor_width));

  // COMPUTE EFFECTIVE LATERAL  conductivity given the transistor effective lateral/vertical conductivities

  // compute %silicon based upon the average size of the transistor and the number of transistors for the design
  // assume average sized transistors and 50% pullup/pulldown overall

  MATRIX_DATA transistor_area = .5 * num_transistors * pmos_transistor_size + .5 * num_transistors * nmos_transistor_size;

  // COMPUTE THE PERCENTAGE OF THE DIE THAT IS COVERED BY TRANSISTORS (should be roughly 70% for 130nm technology), the rest is poly
  MATRIX_DATA percent_transistor = transistor_area / (unit_height * unit_width);
  MATRIX_DATA percent_silicon    = 1 - percent_transistor;

  // if we EVER use more than 100% of the total die area for transistors, we did something wrong
  if(percent_silicon < 0) {
    std::cerr << "Material::calc_die_transistor_layer_properties_dyn() => We are using more than 100% of the transistors with "
                 "percent_transistor=["
              << percent_transistor * 100 << " % ]" << std::endl;
    std::cerr << "Note: this model does NOT allow the granularity to be less than that of a single transistor" << std::endl;
    exit(1);
  }

  // given the percentage of the die that is silicon (and not transistors), compute the silicon lateral resistance
  // R_silicon=unit_width/(k_si*unit_height*si_thickness*%silicon)
  MATRIX_DATA silicon_effective_laterial_resistance = unit_width / (datalibrary_->layer_materials_.find("BULK_SI").conductivity_ *
                                                                    unit_height * silicon_film_thickness * percent_silicon);

  // given the percentage of the die that is transistors (and not silicon), compute the silicon lateral resistance
  // R_transistor=unit_width/(k_transistor_lateral*unit_height*si_thickness*%transistor)
  MATRIX_DATA transistor_effective_lateral_resistance =
      unit_width / (transistor_conductivity_lateral * unit_height * silicon_film_thickness * percent_transistor);

  // R_box=unit_width/(k_box*unit_height*box_thickness)
  MATRIX_DATA box_resistance = unit_width / (k_box * unit_height * box_thickness_actual);

  // R_eq_lateral=1/ (1/R_silicon+1/R_transistor+1/R_box)
  MATRIX_DATA r_eq_lateral =
      1 / ((1 / silicon_effective_laterial_resistance) + (1 / transistor_effective_lateral_resistance) + (1 / box_resistance));

  // k_eq_lateral=unit_width/(R_eq_lateral*layer_thickness*unit_height)
  MATRIX_DATA k_eq_lateral = unit_width / (r_eq_lateral * (silicon_film_thickness + box_thickness_actual) * unit_height);

  // STORE THE LATERAL CONDUCTIVITY
  munit.conduct_center_[dir_left]  = k_eq_lateral;
  munit.conduct_center_[dir_right] = k_eq_lateral;
  munit.conduct_center_[dir_up]    = k_eq_lateral;
  munit.conduct_center_[dir_down]  = k_eq_lateral;

  // COMPUTE EFFECTIVE VERTICAL DOWN conductivity given the transistor effective lateral/vertical conductivities
  // R_transistor=l/(k_transistor_vertical*A)=layer_thickness/(k_transistor_vertical*width_unit*height_unit*%transistors)

  MATRIX_DATA r_transistor = unit_thickness / (transistor_conductivity_vertical * unit_width * unit_height * percent_transistor);

  // R_fox=l/(k_fox_box*A)=layer_thickness/(k_fox_box*height_unit*width_unit*%silicon)
  MATRIX_DATA r_fox = unit_thickness / (k_box * unit_height * unit_width * percent_silicon);

  // R_eq_vertical_down=1/( (1/R_transistor) + (1/R_fox) )
  MATRIX_DATA r_eq_vertical_down = 1 / ((1 / r_transistor) + (1 / r_fox));

  // k_eq_vertical_down=l/R_eq_vertical_down*A=layer_thickness/(R_eq_vertical_down*unit_height*unit_width)
  MATRIX_DATA k_eq_vertical_down = unit_thickness / (r_eq_vertical_down * unit_height * unit_width);

  // STORE THE VERTICAL CONDUCTIVITY UPWARDS
  munit.conduct_center_[dir_top] = k_eq_vertical_down;

  // COMPUTE EFFECTIVE density*spec_heat value
  // FIXME: this is a crude method of computing the density/specific heat
  // Density and Specific heat is scaled by density as follows:
  // We don't know the exact density and specific heat but we know the product of the two.
  // If we assume that the density and specific heat scales as a function of the area, then we can compute
  // a weighted average of the density*specific heat of the fox/box and the transistors/box
  //%transistor/box*(density_transistor*specific_heat_transistor)+%fox/box*(density_fox*specific_heat_fox)
  MATRIX_DATA p_eff_c_eff = (percent_transistor * transistor_p_eff_c_eff + percent_silicon * p_ox_c_ox);

  // we don't know the exact density and specific heat, so what we store is p_eff_c_eff
  // this means that we set the specific heat to 1 and set the density to be equal to p_eff_c_eff, assuming average density

  // STORE THE LAYER p_eff_c_eff (or set spec_heat to 1 and density to density_eff * spec_heat_eff)

  munit.specific_heat_ = 1;
  munit.row_           = p_eff_c_eff;

  // store the emissivity using a simple weighting rule
  MATRIX_DATA emissivity =
      (percent_transistor) * (percentage_box) * (datalibrary_->layer_materials_.find("SIMOX").emissivity_) +
      (percent_silicon) * (datalibrary_->layer_materials_.find("SIMOX").emissivity_) +
      (percent_transistor) * (percentage_silicon_film) * (datalibrary_->layer_materials_.find("DOPED_POLYSILICON").emissivity_);
  munit.emissivity_                   = emissivity;
  munit.heat_transfer_methods_center_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
}

// FIXME: this is currently implemented using the following parameters
// This is for a simplified SOI Mosfet based a 250nm lithography process
// This should be scaled for ANY technology
//"Efficient Thermal Modeling of SOI MOSFETs for Fast Dynamic Operation"
// FIXME: Strained silicon should be added (SOI is only modeled right now)
// FIXME: Silicon conductivity should be a function of temperature
/*
   Oxide Thickness				tox			3nm
   Gate Length					L_g			0.22um
   Fox Length					L_fox		2um
   Eff Channel Length			L_ef		0.18um
   Source/Drain Contact Length	L_cd,L_cd	0.25um
   Silicon Substrate Thickness	t_sub		1.5um
   Silicon Film Length			L_si		1.18um
   Silicon Film Thickness		t_si		0.04um
   Box thickness				t_BOX		0.15um
   Vthreshold					Vth			0.22V
   Oxide thermal conductivity	K_ox		0.82W/mK
   Silicon Island thermal Conductivity	K_si	63W/mK
   Effective Oxide thermal conductivity K_ox_eff 1.79W/mK (FIXME: this should be computed, currently extrapolated from above paper)
   Thermal Capacity Density	p_si*c_si	1.63x10^6J/M^3*K
   width=2.86um	(FIXME: this value should be updated)


   R_top=interlayer_resistance (compute)
   R_bottom=R_box=[t_box/(k_ox_eff*width)](L_si)
   k_vertical=l/R_bottom*A=(t_si+t_box)/R_bottom*transistor_width*L_si;

   R_lateral=1/[width*(k_ox_eff(t_box/3) + k_si*t_si)]
   k_lateral=L_si/(R_lateral*transistor_width*(t_si+t_box))

   C_eff=(L_si)(width)[p_ox*c_ox*t_box/3+ p_si*c_si*t_si]=density*spec_heat*volume

   We want the volume to be that of the cross-sectional region, hence we divide by the volume
   used here to obtain density*spec_heeat

   spec_heat=1 //we don't know the exact specific heat, only the densith*spec_heat
   transistor density=C_eff/[(t_si + t_box/3)(L_si)(width)]

   We want the conductivities, not the resistances: obtain as follows


Note: this means that we WON'T have the exact specific heat and density for the die_transistor layer

We are assuming that the time periods here are significantly longer than the thermal time constant

Once we obtain these value, we scale them based upon the density of the transistors.

For the lateral resistances, we obtain a parallel resistance between the transistor resistance, silicon resistance and box/fox
resistance: R_silicon=chip_width/(k_si*chip_height*si_thickness*%silicon)
R_transistor=chip_width/(k_transistor_lateral*chip_height*si_thickness*%transistor)
R_box=chip_width/(k_box*chip_height*box_thickness)
R_eq_lateral=1/ (1/R_silicon+1/R_transistor+1/R_box)
k_eq_lateral=chip_width/(R_eq_lateral*layer_thickness*chip_height)

Note: we don't know the exact silicon film thickness and box thickness. All we know if the entire thickness of the box/fox layer.

For the top resistance/conductivity, this is computed via an external function which compensates for density

The botom resistance is scaled by computing the parallel resistance between the transistor resistance and box/fox resistance:
R_transistor=l/(k_transistor_vertical*A)=layer_thickness/(k_transistor_vertical*width_chip*height_chip*%transistors)
R_fox=l/(k_fox_box*A)=layer_thickness/(k_fox_box*height_chip*width_chip*%silicon)
R_eq_vertical_down=1/( (1/R_transistor) + (1/R_fox) )
k_eq_vertical_down=l/R_eq_vertical_down*A=layer_thickness/(R_eq_vertical_down*chip_height*chip_width)
FIXME: this is a crude method of computing the density/specific heat
Density and Specific heat is scaled by density as follows:
We don't know the exact density and specific heat but we know the product of the two.
If we assume that the density and specific heat scales as a function of the area, then we can compute
a weighted average of the density*specific heat of the fox/box and the transistors/box
%transistor*(density_transistor*specific_heat_transistor)+%fox*(density_fox*specific_heat_fox)
 */
void Material::calc_die_transistor_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  ConfigData *conf                 = datalibrary_->config_data_;
  MATRIX_DATA chip_thickness       = (layer.thickness_ == 0) ? conf->chip_thickness_ : layer.thickness_;
  MATRIX_DATA chip_width           = (layer.width_ == 0) ? conf->chip_width_ : layer.width_;
  MATRIX_DATA chip_height          = (layer.height_ == 0) ? conf->chip_height_ : layer.height_;
  MATRIX_DATA transistor_width     = conf->sample_gate_width_;
  MATRIX_DATA silicon_film_length  = conf->sample_silicon_film_length_;
  MATRIX_DATA t_box                = conf->sample_box_thickness_;
  MATRIX_DATA k_si                 = conf->sample_silicon_island_conductivity_;
  MATRIX_DATA p_ox_c_ox            = conf->sample_thermal_capacity_oxide_;
  MATRIX_DATA p_si_c_si            = conf->sample_thermal_capacity_silicon_;
  MATRIX_DATA num_transistors      = conf->transistor_count_;
  int         technology           = conf->technology_; // get the technology used
  MATRIX_DATA pmos_transistor_size = DataLibrary::technology_parameters_[TECH_GATE_L_AVE][technology] *
                                     DataLibrary::technology_parameters_[TECH_GATE_PMOS_W_AVE][technology]; // p_mos area
  MATRIX_DATA nmos_transistor_size = DataLibrary::technology_parameters_[TECH_GATE_L_AVE][technology] *
                                     DataLibrary::technology_parameters_[TECH_GATE_NMOS_W_AVE][technology]; // n_mos area

  // first we need to find the actual thickness of the buried oxide and silicon film thickness
  // we assume that the box/silicon film thickness is in the same proportions for the sample transistor
  // as it is in the target technology

  // FIXME: we should actually be obtaining approximate values that vary with process technology and foundry (IBM/TSMC/INTEL)

  MATRIX_DATA t_sub    = conf->sample_silicon_substrate_thickness_;
  MATRIX_DATA t_si     = conf->sample_silicon_film_thickness_;
  MATRIX_DATA k_ox_eff = conf->sample_oxide_conductivity_transistor_;
  MATRIX_DATA k_box    = conf->sample_oxide_conductivity_;

  MATRIX_DATA t_interconnect = 0;
  int         num_layers     = (int)DataLibrary::technology_parameters_[TECH_LEVELS][technology];
  for(int i = 0; i < num_layers; i++) {
    t_interconnect = DataLibrary::technology_parameters_[10 + (i * 9) + TECH_H]
                                                        [technology]; // get the thickness of the layer for the given technology
  }

  MATRIX_DATA percentage_box          = t_box / (t_sub + t_box + t_si + t_interconnect);
  MATRIX_DATA box_thickness_actual    = chip_thickness * percentage_box;
  MATRIX_DATA percentage_silicon_film = t_si / (t_sub + t_box + t_si + t_interconnect);
  MATRIX_DATA silicon_film_thickness  = chip_thickness * percentage_silicon_film;

  if(layer.width_ == 0)
    layer.width_ = chip_width;
  if(layer.height_ == 0)
    layer.height_ = chip_height;
  if(layer.thickness_ == 0)
    layer.thickness_ = box_thickness_actual + silicon_film_thickness;

  // compute the conductivity from the chip to interconnect
  // R_bottom=interlayer_resistance (compute)
  MATRIX_DATA conduct_bottom =
      Material::calc_effective_interconnect_layer_vertical_conductivity(0, 1.0, datalibrary_); // Layer 0, default density

  // STORE THE VERTICAL CONDUCTIVITY DOWNWARDS
  layer.vertical_conductivity_down_ = conduct_bottom;

  // compute the resistance from the transistor through the BOX and FOX to the silicon substrate
  // R_bottom=R_box=[t_box/(k_ox_eff*width)](L_si)
  MATRIX_DATA transistor_vertical_resistance_down = (box_thickness_actual / (k_ox_eff * transistor_width * silicon_film_length));

  // k_vertical=l/R_bottom*A=(t_si+t_box)/R_bottom*transistor_width*L_si;
  MATRIX_DATA transistor_conductivity_vertical = (silicon_film_thickness + box_thickness_actual) /
                                                 (transistor_vertical_resistance_down * transistor_width * silicon_film_length);

  // R_lateral=L_si/[width*(k_ox_eff(t_box/3) + k_si*t_si)]
  MATRIX_DATA transistor_lateral_resistance =
      silicon_film_length / (transistor_width * (k_ox_eff * (box_thickness_actual / 3) + k_si * silicon_film_thickness));

  // k_lateral=L_si/(R_lateral*transistor_width*(t_si+t_box))
  MATRIX_DATA transistor_conductivity_lateral =
      silicon_film_length / (transistor_lateral_resistance * transistor_width * (silicon_film_thickness + box_thickness_actual));

  // C_eff=(L_si)(width)[p_ox*c_ox*t_box/3+ p_si*c_si*t_si]=density*spec_heat*volume
  MATRIX_DATA effective_capacitance =
      silicon_film_length * transistor_width * (p_ox_c_ox * (box_thickness_actual / 3) + p_si_c_si * silicon_film_thickness);

  // transistor density*transistor_specific_heat=C_eff/[(t_si + t_box/3)(L_si)(width)]=C_eff/volume
  MATRIX_DATA transistor_p_eff_c_eff =
      effective_capacitance / ((silicon_film_thickness + box_thickness_actual / 3) * (silicon_film_length) * (transistor_width));

  // COMPUTE EFFECTIVE LATERAL  conductivity given the transistor effective lateral/vertical conductivities

  // compute %silicon based upon the average size of the transistor and the
  // number of transistors for the design assume average sized transistors and
  // 50% pullup/pulldown overall

  MATRIX_DATA transistor_area = .5 * num_transistors * pmos_transistor_size + .5 * num_transistors * nmos_transistor_size;

  // COMPUTE THE PERCENTAGE OF THE DIE THAT IS COVERED BY TRANSISTORS (should be
  // roughly 70% for 130nm technology), the rest is poly
  MATRIX_DATA percent_transistor = transistor_area / (chip_height * chip_width);
  MATRIX_DATA percent_silicon    = 1 - percent_transistor;

  // if we EVER use more than 100% of the total die area for transistors, we did
  // something wrong
  if(percent_silicon < 0) {
    std::cerr << "Material::calc_die_transistor_layer_properties() => We are using more than 100% of the transistors with "
                 "percent_transistor=["
              << percent_transistor * 100 << "% ]" << std::endl;
    exit(1);
  }

  // given the percentage of the die that is silicon (and not transistors), compute the silicon lateral resistance
  // R_silicon=chip_width/(k_si*chip_height*si_thickness*%silicon)

  Material &bulk_si = datalibrary_->layer_materials_.find("BULK_SI");

  MATRIX_DATA silicon_effective_laterial_resistance =
      chip_width / (bulk_si.conductivity_ * chip_height * silicon_film_thickness * percent_silicon);

  // given the percentage of the die that is transistors (and not silicon), compute the silicon lateral resistance
  // R_transistor=chip_width/(k_transistor_lateral*chip_height*si_thickness*%transistor)
  MATRIX_DATA transistor_effective_lateral_resistance =
      chip_width / (transistor_conductivity_lateral * chip_height * silicon_film_thickness * percent_transistor);

  // R_box=chip_width/(k_box*chip_height*box_thickness)
  MATRIX_DATA box_resistance = chip_width / (k_box * chip_height * box_thickness_actual);

  // R_eq_lateral=1/ (1/R_silicon+1/R_transistor+1/R_box)
  MATRIX_DATA r_eq_lateral =
      1 / ((1 / silicon_effective_laterial_resistance) + (1 / transistor_effective_lateral_resistance) + (1 / box_resistance));

  // k_eq_lateral=chip_width/(R_eq_lateral*layer_thickness*chip_height)
  MATRIX_DATA k_eq_lateral = chip_width / (r_eq_lateral * (silicon_film_thickness + box_thickness_actual) * chip_height);

  // STORE THE LATERAL CONDUCTIVITY
  layer.horizontal_conductivity_ = k_eq_lateral;

  // COMPUTE EFFECTIVE VERTICAL DOWN conductivity given the transistor effective lateral/vertical conductivities
  // R_transistor=l/(k_transistor_vertical*A)=layer_thickness/(k_transistor_vertical*width_chip*height_chip*%transistors)

  MATRIX_DATA r_transistor = layer.thickness_ / (transistor_conductivity_vertical * chip_width * chip_height * percent_transistor);

  // R_fox=l/(k_fox_box*A)=layer_thickness/(k_fox_box*height_chip*width_chip*%silicon)
  MATRIX_DATA r_fox = layer.thickness_ / (k_box * chip_height * chip_width * percent_silicon);

  // R_eq_vertical_down=1/( (1/R_transistor) + (1/R_fox) )
  MATRIX_DATA r_eq_vertical_down = 1 / ((1 / r_transistor) + (1 / r_fox));

  // k_eq_vertical_down=l/R_eq_vertical_down*A=layer_thickness/(R_eq_vertical_down*chip_height*chip_width)
  MATRIX_DATA k_eq_vertical_down = layer.thickness_ / (r_eq_vertical_down * chip_height * chip_width);

  // STORE THE VERTICAL CONDUCTIVITY UPWARDS
  layer.vertical_conductivity_up_ = k_eq_vertical_down;

  // COMPUTE EFFECTIVE density*spec_heat value
  // FIXME: this is a crude method of computing the density/specific heat
  // Density and Specific heat is scaled by density as follows:
  // We don't know the exact density and specific heat but we know the product of the two.
  // If we assume that the density and specific heat scales as a function of the area, then we can compute
  // a weighted average of the density*specific heat of the fox/box and the transistors/box
  //%transistor/box*(density_transistor*specific_heat_transistor)+%fox/box*(density_fox*specific_heat_fox)

  MATRIX_DATA p_eff_c_eff = (percent_transistor * transistor_p_eff_c_eff + percent_silicon * p_ox_c_ox);

  // we don't know the exact density and specific heat, so what we store is p_eff_c_eff
  // this means that we set the specific heat to 1 and set the density to be equal to p_eff_c_eff, assuming average density

  // STORE THE LAYER p_eff_c_eff (or set spec_heat to 1 and density to density_eff * spec_heat_eff)
  layer.spec_heat_ = 1;
  layer.density_   = p_eff_c_eff;

  // store the emissivity using a simple weighting rule between BOX/FOX/and POLY
  MATRIX_DATA emissivity =
      (percent_transistor) * (percentage_box) * (datalibrary_->layer_materials_.find("SIMOX").emissivity_) +
      (percent_silicon) * (datalibrary_->layer_materials_.find("SIMOX").emissivity_) +
      (percent_transistor) * (percentage_silicon_film) * (datalibrary_->layer_materials_.find("DOPED_POLYSILICON").emissivity_);

  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
}

// this is used to compute the relationship between k_ild and K_ild
// where k_ild is the dielectric contact (process specific) and K_ild is the inter-layer conductivity

//
//
// FIXME: material 0 is the only one currently implemented (get the other parameters)
//
//
// parameters (defaults to xerogel)
//      Material 0: xerogel (porous silicate)

//                       R. M. Costescu, A. J. Bullen, G. Matamis, K. E. O’Hara, and D. G.
//                      Cahill, “Thermal conductivity and sound velocities of hydrogen-
//                      silsesquioxane low-k dielectrics,” Phys. Rev. B, vol. 65, no. 9, pp.
//                      094205/1-6, 2002.

//                      A. Jain, S. Rogojevic, S. Ponoth, W. N. Gill, J. L. Plawsky, E.
//                      Simonyi, S. T. Chen, and P. S. Ho, “Processing dependent thermal
//                      conductivity of nanoporous silica xerogel films,” J. Appl. Phys., vol.
//                      91, no. 5, pp. 3275-3281, 2002.

//                      B. Y. Tsui, C. C. Yang, and K. L. Fang, “Anisotropic thermal
//                      conductivity of nanoporous silica film,” IEEE Trans. Electron
//                      Devices, vol. 51, no. 1, pp. 20-27, Jan. 2004.

//      Material 1: flourinated silicate glass (FSG)

//                      B. C. Daly, H. J. Maris, W. K. Ford, G. A. Antonelli, L. Wong, and
//                      E. Andideh, “Optical pump and probe measurement of the thermal
//                      conductivity of low-k dielectno. 10, pp. 6005-6009, 2002.

//      Material 2: hydrogen-sisesquioxane (HSQ)

//                      R. M. Costescu, A. J. Bullen, G. Matamis, K. E. O’Hara, and D. G.
//                      Cahill, “Thermal conductivity and sound velocities of hydrogen-
//                      silsesquioxane low-k dielectrics,” Phys. Rev. B, vol. 65, no. 9, pp.
//                      094205/1-6, 2002.

//                      P. Zhou, Ph. D. Dissertation, Stanford University, 2001.

//      Material 3: carbon-doped oxide (CDO)

//                      B. C. Daly, H. J. Maris, W. K. Ford, G. A. Antonelli, L. Wong, and
//                      E. Andideh, “Optical pump and probe measurement of the thermal
//                      conductivity of low-k dielectric thin films,”  J. Appl. Phys., vol. 92,
//                      no. 10, pp. 6005-6009, 2002.

//                      P. Zhou, Ph. D. Dissertation, Stanford University, 2001.

//      Material 4: organic polymers

//                      P. Zhou, Ph. D. Dissertation, Stanford University, 2001.

//      Material 5: methylsilsesquioxane (MSQ)

//                      J. Liu, D. Gan, C. Hu, M. Kiene, P. S. Ho, W. Volksen, and R. D.
//                      Miller, “Porosity effect on the dielectric constant and
//                      thermomechanical properties of organosilicate films,”  Appl. Phys.
//                      Lett., vol. 81, no. 22, pp. 4180-4182, 2002.

//              Material 6: phosphosilicate glass (PSG)
//                      this is assumed to be the pre-metal inter-layer dielectric
//                      FIXME: we assume a thermal conductivity of 0.94W/mk ALWAYS!
//

//      k_p=dielectric constant of pores
//      k_m=dielectric constant of matrix material
//      K_p=cond_p=thermal conductivity of pores
//      K_m=cond_m=thermal conductivity of matrix material
//      x is a fitting parameter

MATRIX_DATA Material::calc_interlayer_conductivity(MATRIX_DATA dielectric_constant, int material, DataLibrary *datalibrary_) {
  MATRIX_DATA k_p, k_m, cond_p, cond_m, x;
  switch(material) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    k_p    = 1.0;
    k_m    = 4.1;
    cond_p = 0.0255;
    cond_m = 1.4;
    x      = 0.49;
    break;
  case 6:
    return (0.94); // FIXME: PSG is assumed to have thermal conductivity of 0.94W/mK always!
  default:
    std::cerr << "Material::calc_interlayer_conductivity -> invalid material (assuming material 0, xerogel)" << std::endl;
    k_p    = 1.0;
    k_m    = 4.1;
    cond_p = 0.0255;
    cond_m = 1.4;
    x      = 0.49;
  }

  std::map<MATRIX_DATA, std::map<int, MATRIX_DATA>>::iterator iter =
      datalibrary_->config_data_->inter_layer_conductivities_.find(dielectric_constant);
  if(iter != datalibrary_->config_data_->inter_layer_conductivities_.end()) {
    std::map<int, MATRIX_DATA>::iterator iter2 = iter->second.find(material);
    if(iter2 != iter->second.end()) {
      return iter2->second; // return the cached value if it exists
    }
  }
  // otherwise compute the value

  RegressionLine::Points k_ild_to_P;
  RegressionLine::Points P_to_cond_ild;

  for(MATRIX_DATA p = 0; LT(p, 1.0); p += .001) {
    // k_ild_to_P[(2*k_m + 3*p*k_p - 3*p*k_m)/4]=p;  //ignore imaginary portion
    MATRIX_DATA tmp = (p * cond_p + (1 - p) * cond_m) * (1 - double_pow(p, x)) +
                      ((cond_p * cond_m * powf(p, x)) / (p * cond_m + (1 - p) * cond_p));
    P_to_cond_ild[p] = tmp;
  }

  //   RegressionLine k_ild_to_P_line(k_ild_to_P);

  // normalize inter_layer_dielectric to three digits
  // perform polynomial interpolation of k_ild_to_P and P_to_cond_ild
  // find P based upon k_ild (ignore imaginary portion)
  // then find cond_ild based upon P
  MATRIX_DATA tmp0     = floor(dielectric_constant * powf(10.0, 3.0) + .5);
  MATRIX_DATA tmp1     = powf(10.0, 3.0);
  MATRIX_DATA k_ild    = tmp0 / tmp1;
  MATRIX_DATA p_lookup = (k_ild - k_m) * (k_p + 2 * k_ild) / (-4 * powf(k_ild, 2.0) + 3 * k_ild * (k_p - k_m));
  MATRIX_DATA value    = RegressionLine::quad_interpolate(P_to_cond_ild, p_lookup);
  datalibrary_->config_data_->inter_layer_conductivities_[dielectric_constant][material] = value;
  return (RegressionLine::quad_interpolate(P_to_cond_ild, p_lookup));
}

// density is the percentage density of the interconnect relative to the average density (50% of SRAM)
// this is used to scale the effective conductivity as a function of the density of the corresponding functional unit
MATRIX_DATA Material::calc_effective_interconnect_layer_vertical_conductivity(int layer, MATRIX_DATA density,
                                                                              DataLibrary *datalibrary_) {
  int         technology  = datalibrary_->config_data_->technology_; // get the technology used
  int         metal_index = DataLibrary::tech_metal_index(layer); // get the metal layer offset for the technology_parameters table
  MATRIX_DATA dielectric_constant = DataLibrary::technology_parameters_[TECH_K][technology];
  // compute the bulk thermal conductivity of the inter-layer dielectric using the dielectric constant

  MATRIX_DATA bulk_thermal_conductivity;

  if(layer == 0)
    bulk_thermal_conductivity =
        calc_interlayer_conductivity(dielectric_constant, 5, datalibrary_); // PMD (pre-metal dielectric is PSG)
  else
    bulk_thermal_conductivity =
        calc_interlayer_conductivity(dielectric_constant, 0, datalibrary_); // PMD (pre-metal dielectric is PSG)

  MATRIX_DATA metal_width        = DataLibrary::technology_parameters_[metal_index + TECH_W][technology];
  MATRIX_DATA metal_spacing      = DataLibrary::technology_parameters_[metal_index + TECH_SPACE][technology];
  MATRIX_DATA via_size           = metal_width; // this assumes that the via size is the same as the metal width;
  MATRIX_DATA inter_via_distance = DataLibrary::technology_parameters_[metal_index + TECH_VIADIST][technology];
  MATRIX_DATA via_thermal_conductivity;
  if(layer == 0)
    via_thermal_conductivity = 165; // tungsten has thermal conductivity of 165W/mK
  else
    via_thermal_conductivity = 396.36; // copper thermal conductivityis 396.36W/mK at 85 degrees celcius.

  MATRIX_DATA via_density = (via_size * via_size) / ((metal_width + metal_spacing) * inter_via_distance) * density;
  MATRIX_DATA cond_eff =
      MIN(via_thermal_conductivity, via_density * via_thermal_conductivity + (1 - via_density) * bulk_thermal_conductivity);
  return (cond_eff);
}

// this computes the horizontal conductivity across the layer
MATRIX_DATA Material::calc_effective_interconnect_layer_horizontal_conductivity(int material_layer, int interconnect_layer,
                                                                                MATRIX_DATA density, DataLibrary *datalibrary_) {
  MATRIX_DATA height;
  if(datalibrary_->all_layers_info_[material_layer]->height_ == 0)
    height = datalibrary_->config_data_->chip_height_;
  else
    height = datalibrary_->all_layers_info_[material_layer]->height_;
  MATRIX_DATA width;
  if(datalibrary_->all_layers_info_[material_layer]->width_ == 0)
    width = datalibrary_->config_data_->chip_width_;
  else
    width = datalibrary_->all_layers_info_[material_layer]->width_;

  int         technology          = datalibrary_->config_data_->technology_; // get the technology used
  MATRIX_DATA dielectric_constant = DataLibrary::technology_parameters_[TECH_K][technology];
  // compute inter-layer dielectric bulk thermal conductivity
  MATRIX_DATA bulk_thermal_conductivity;
  if(interconnect_layer == 0)
    bulk_thermal_conductivity =
        calc_interlayer_conductivity(dielectric_constant, 6, datalibrary_); // PMD (pre-metal dielectric is PSG)
  else
    bulk_thermal_conductivity =
        calc_interlayer_conductivity(dielectric_constant, 0, datalibrary_); // PMD (pre-metal dielectric is PSG)

  MATRIX_DATA layer_area = 0;
  MATRIX_DATA metal_area = 0;
  // compute metal layer volume
  layer_area = height * width;
  if(interconnect_layer == 0)
    metal_area = layer_area * MIN(.71 * density, 1.0);
  else
    metal_area = layer_area * MIN(.60 * density, 1.0);

  MATRIX_DATA copper_conductivity = datalibrary_->layer_materials_.find("COPPER").conductivity_;
  MATRIX_DATA total_wire_crosssectional_area =
      (metal_area / width) * DataLibrary::technology_parameters_[10 + interconnect_layer * 9 + TECH_H][technology];
  MATRIX_DATA total_dielectric_crossectional_area =
      ((layer_area - metal_area) / width) * DataLibrary::technology_parameters_[10 + interconnect_layer * 9 + TECH_H][technology];
  MATRIX_DATA total_resistance = 1 / (1 / (width / (copper_conductivity * total_wire_crosssectional_area)) +
                                      1 / (width / (bulk_thermal_conductivity * total_dielectric_crossectional_area)));
  MATRIX_DATA total_conductivity =
      width / (total_resistance * height * DataLibrary::technology_parameters_[10 + interconnect_layer * 9 + TECH_H][technology]);
  return (total_conductivity);
}

void Material::calc_interconnect_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_) {
  ConfigData *conf      = datalibrary_->config_data_;
  ChipLayers *layerInfo = datalibrary_->all_layers_info_[layer];

  ChipFloorplan_Unit *source_chip_flp = (*layerInfo->floorplan_layer_dyn_)[x][y].source_chip_flp_;
  int                 technology      = conf->technology_; // get the technology used
  MATRIX_DATA         height;
  if(layerInfo->height_ == 0)
    height = conf->chip_height_;
  else
    height = layerInfo->height_;
  MATRIX_DATA width;
  if(layerInfo->width_ == 0)
    width = conf->chip_width_;
  else
    width = layerInfo->width_;
  MATRIX_DATA thickness;
  if(layerInfo->thickness_ == 0)
    thickness = conf->chip_thickness_;
  else
    thickness = layerInfo->thickness_;

  int num_layers = (int)DataLibrary::technology_parameters_[TECH_LEVELS][technology];

  MATRIX_DATA interconnect_density = source_chip_flp->interconnect_density_;

  // compute the effective vertical thermal conductivity for all of the metal layers

  MATRIX_DATA area         = 0;
  MATRIX_DATA conductivity = 0;
  MATRIX_DATA resistance   = 0;

  MATRIX_DATA temp_thickness = 0;

  // here we compute the layer thickness for the sample technology parameters
  MATRIX_DATA total_thickness = 0;
  for(int i = 0; i < num_layers; i++) {
    // get the thickness of the layer for the given technology
    total_thickness += DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology];
  }

  // here we assume the same propotions for the layers
  area = width * height;
  for(int i = 0; i < num_layers; i++) {
    temp_thickness = (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) *
                     thickness; // get the thickness of the layer for the given technology
    conductivity = Material::calc_effective_interconnect_layer_vertical_conductivity(
        i, interconnect_density, datalibrary_); // assume average conductivity initial computation
    resistance += temp_thickness / (conductivity * area);
  }

  // k=l/AR
  MATRIX_DATA effective_vertical_conductivity = thickness / (area * resistance);

  // store the vertical conductivity
  ModelUnit &munit = (*layerInfo->floorplan_layer_dyn_)[x][y];
  // The conductivity to the transistor layer should be higher because the metal is in contact
  munit.conduct_center_[dir_top]    = effective_vertical_conductivity * 4;
  munit.conduct_center_[dir_bottom] = effective_vertical_conductivity;

  MATRIX_DATA conductance = 0;

  // compute the effective horizontal thermal conductivity for all of the metal layers
  for(int i = 0; i < num_layers; i++) {
    temp_thickness = (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) *
                     thickness; // get the thickness of the layer for the given technology
    conductance +=
        Material::calc_effective_interconnect_layer_horizontal_conductivity(layer, i, interconnect_density, datalibrary_) *
        temp_thickness * height / width; // assume average conductivity initial computation
  }
  resistance = 1 / conductance; // compute effective resistance

  // k=l/AR
  MATRIX_DATA effective_horizontal_conductivity = width / (thickness * height * resistance);

  // store the horizontal conductivity
  munit.conduct_center_[dir_left]  = effective_horizontal_conductivity;
  munit.conduct_center_[dir_right] = effective_horizontal_conductivity;
  munit.conduct_center_[dir_up]    = effective_horizontal_conductivity;
  munit.conduct_center_[dir_down]  = effective_horizontal_conductivity;

  // calculate the effective specific heat for all of the metal layers
  // based upon data analysis, 71% wiring density of layer 1 and 60% on the remaining layers
  MATRIX_DATA layer_volume       = 0;
  MATRIX_DATA total_layer_volume = 0;
  MATRIX_DATA total_metal_volume = 0;
  // compute total metal layer volume
  for(int i = 0; i < num_layers; i++) {
    layer_volume =
        height * width * (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) * thickness;
    total_layer_volume += layer_volume;
    if(i == 0)
      total_metal_volume += layer_volume * MIN(.71 * interconnect_density, 1.0);
    else
      total_metal_volume += layer_volume * MIN(.60 * interconnect_density, 1.0);
  }

  Material &  cu            = datalibrary_->layer_materials_.find("COPPER");
  Material &  xerogel       = datalibrary_->layer_materials_.find("XEROGEL");
  MATRIX_DATA specific_heat = (total_metal_volume / total_layer_volume) * (cu.spec_heat_) +
                              (1 - total_metal_volume / total_layer_volume) * (xerogel.spec_heat_);
  // store the specific heat
  munit.specific_heat_ = specific_heat;

  munit.specific_heat_ *= CAPACITANCE_FACTOR_INTERCONNECT;

  // compute effective density
  MATRIX_DATA density = (total_metal_volume / total_layer_volume) * (cu.density_) +
                        (1 - total_metal_volume / total_layer_volume) * (xerogel.density_);
  // store the density
  munit.row_ = density;

  // store the emissivity
  MATRIX_DATA emissivity = (total_metal_volume / total_layer_volume) * (cu.emissivity_) +
                           (1 - total_metal_volume / total_layer_volume) * (xerogel.emissivity_);
  munit.emissivity_                   = emissivity;
  munit.heat_transfer_methods_center_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
}

void Material::calc_interconnect_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  ConfigData *conf       = datalibrary_->config_data_;
  int         technology = conf->technology_;
  int         num_layers = (int)DataLibrary::technology_parameters_[TECH_LEVELS][technology];

  MATRIX_DATA height               = (layer.height_ == 0) ? conf->chip_height_ : layer.height_;
  MATRIX_DATA width                = (layer.width_ == 0) ? conf->chip_width_ : layer.width_;
  MATRIX_DATA thickness            = (layer.thickness_ == 0) ? conf->chip_thickness_ : layer.thickness_;
  MATRIX_DATA interconnect_density = 1;

  // store the height
  if(layer.height_ == 0)
    layer.height_ = height;

  // store the width
  if(layer.width_ == 0)
    layer.width_ = width;

  // here we compute the layer thickness for the sample technology parameters
  MATRIX_DATA total_thickness = 0;
  for(int i = 0; i < num_layers; i++) {
    // get the thickness of the layer for the given technology
    total_thickness += DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology];
  }

  // here we assume the same propotions for the layers
  MATRIX_DATA area           = width * height;
  MATRIX_DATA conductivity   = 0;
  MATRIX_DATA resistance     = 0;
  MATRIX_DATA temp_thickness = 0;
  for(int i = 0; i < num_layers; i++) {
    // get the thickness of the layer for the given technology
    temp_thickness = (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) * thickness;

    // assume average conductivity initial computation
    conductivity = Material::calc_effective_interconnect_layer_vertical_conductivity(i, interconnect_density, datalibrary_);
    resistance += temp_thickness / (conductivity * area);
  }

  // store the layer thickness
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;

  // k=l/AR
  MATRIX_DATA effective_vertical_conductivity = thickness / (area * resistance);

  // store the vertical conductivity
  layer.vertical_conductivity_down_ = effective_vertical_conductivity;
  layer.vertical_conductivity_up_   = effective_vertical_conductivity;

  conductivity            = 0;
  MATRIX_DATA conductance = 0;
  // compute the effective horizontal thermal conductivity for all of the metal layers
  for(int i = 0; i < num_layers; i++) {
    // get the thickness of the layer for the given technology
    temp_thickness = (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) * thickness;

    // assume average conductivity initial computation
    conductance += Material::calc_effective_interconnect_layer_horizontal_conductivity(layer.layer_number_, i, interconnect_density,
                                                                                       datalibrary_) *
                   temp_thickness * height / width;
  }
  resistance = 1 / conductance;
  // k=l/AR
  MATRIX_DATA effective_horizontal_conductivity = width / (resistance * thickness * height);

  // store the horizontal conductivbity
  layer.horizontal_conductivity_ = effective_horizontal_conductivity;

  // calculate the effective specific heat for all of the metal layers
  // based upon data analysis, 71% wiring density of layer 1 and 60% on the remaining layers
  MATRIX_DATA layer_volume       = 0;
  MATRIX_DATA total_layer_volume = 0;
  MATRIX_DATA total_metal_volume = 0;
  // compute total metal layer volume
  for(int i = 0; i < num_layers; i++) {
    layer_volume =
        height * width * (DataLibrary::technology_parameters_[10 + i * 9 + TECH_H][technology] / total_thickness) * thickness;
    total_layer_volume += layer_volume;
    if(i == 0)
      total_metal_volume += layer_volume * MIN(.71 * interconnect_density, 1.0);
    else
      total_metal_volume += layer_volume * MIN(.60 * interconnect_density, 1.0);
  }

  MATRIX_DATA specific_heat =
      (total_metal_volume / total_layer_volume) * (datalibrary_->layer_materials_.find("COPPER").spec_heat_) +
      (1 - total_metal_volume / total_layer_volume) * (datalibrary_->layer_materials_.find("XEROGEL").spec_heat_);

  // store the specific heat
  layer.spec_heat_ = specific_heat;

  // compute effective density
  Material &  cu      = datalibrary_->layer_materials_.find("COPPER");
  Material &  xerogel = datalibrary_->layer_materials_.find("XEROGEL");
  MATRIX_DATA density = (total_metal_volume / total_layer_volume) * (cu.density_) +
                        (1 - total_metal_volume / total_layer_volume) * (xerogel.density_);
  // store the density
  layer.density_ = density;

  // store the emissivity
  MATRIX_DATA emissivity = (total_metal_volume / total_layer_volume) * (cu.emissivity_) +
                           (1 - total_metal_volume / total_layer_volume) * (xerogel.emissivity_);
  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
}

// CALCULATE HEAT SINK LAYER PARAMETERS
void Material::calc_heat_sink_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA width            = datalibrary_->config_data_->heat_sink_width_;
  MATRIX_DATA height           = datalibrary_->config_data_->heat_sink_height_;
  MATRIX_DATA thickness        = datalibrary_->config_data_->heat_sink_thickness_;
  MATRIX_DATA emissivity       = datalibrary_->layer_materials_.find("COPPER").emissivity_;
  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;

  Material &cu                      = datalibrary_->layer_materials_.find("COPPER");
  layer.spec_heat_                  = cu.spec_heat_;
  layer.density_                    = cu.density_;
  layer.horizontal_conductivity_    = cu.conductivity_;
  layer.vertical_conductivity_up_   = cu.conductivity_;
  layer.vertical_conductivity_down_ = cu.conductivity_;
}

// CALCULATE HEAT SINK FINS LAYER PARAMETERS
void Material::calc_heat_sink_fins_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA width            = datalibrary_->config_data_->heat_sink_width_;
  MATRIX_DATA height           = datalibrary_->config_data_->heat_sink_height_;
  MATRIX_DATA thickness        = datalibrary_->config_data_->heat_sink_fins_thickness_;
  MATRIX_DATA emissivity       = datalibrary_->layer_materials_.find("COPPER").emissivity_;
  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;
  layer.spec_heat_                  = datalibrary_->layer_materials_.find("COPPER").spec_heat_;
  layer.density_                    = datalibrary_->layer_materials_.find("COPPER").density_;
  layer.horizontal_conductivity_    = datalibrary_->layer_materials_.find("COPPER").conductivity_;
  layer.vertical_conductivity_up_   = datalibrary_->layer_materials_.find("COPPER").conductivity_;
  layer.vertical_conductivity_down_ = datalibrary_->layer_materials_.find("COPPER").conductivity_;
}

// CALCULATE HEAT SPREADER LAYER PARAMETERS
void Material::calc_heat_spreader_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA width      = datalibrary_->config_data_->heat_spreader_width_;
  MATRIX_DATA height     = datalibrary_->config_data_->heat_spreader_height_;
  MATRIX_DATA thickness  = datalibrary_->config_data_->heat_spreader_thickness_;
  MATRIX_DATA emissivity = datalibrary_->layer_materials_.find("COPPER").emissivity_;

  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);

  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;

  layer.spec_heat_ = datalibrary_->layer_materials_.find("COPPER").spec_heat_;
  layer.density_   = datalibrary_->layer_materials_.find("COPPER").density_;

  Material &copper                  = datalibrary_->layer_materials_.find("COPPER");
  layer.horizontal_conductivity_    = copper.conductivity_;
  layer.vertical_conductivity_down_ = copper.conductivity_;
  layer.vertical_conductivity_up_   = copper.conductivity_;
}

// CALCULATE C4_UNDERFILL LAYER PARAMETERS
void Material::calc_c4underfill_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA width            = datalibrary_->config_data_->chip_width_;
  MATRIX_DATA height           = datalibrary_->config_data_->chip_height_;
  MATRIX_DATA thickness        = 90.55e-6; // 90.55um (Takem from "Prediction of Thermal Performance...")
  MATRIX_DATA pin_count        = datalibrary_->config_data_->pin_count_;
  MATRIX_DATA emissivity       = datalibrary_->layer_materials_.find("COPPER").emissivity_;
  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;

  MATRIX_DATA volume             = width * height * thickness;
  layer.horizontal_conductivity_ = 0.8; // 0.8W/mK (Takem from "Prediction of Thermal Performance...")

  layer.vertical_conductivity_down_ = 1.66; // 1.66W/mK (Takem from "Prediction of Thermal Performance...")
  layer.vertical_conductivity_up_   = 1.66; // 1.66W/mK (Takem from "Prediction of Thermal Performance...")

  MATRIX_DATA volume_bump = (4 / 3) * 3.14159265 * (powf((250e-6) / 2.0, 3.0));
  layer.spec_heat_        = datalibrary_->layer_materials_.find("COPPER").spec_heat_ * ((volume_bump * pin_count) / volume);
  layer.density_          = datalibrary_->layer_materials_.find("COPPER").density_ * ((volume_bump * pin_count) / volume);
}

// CALCULATE PWB LAYER PARAMETERS
void Material::calc_pwb_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  ConfigData *conf       = datalibrary_->config_data_;
  MATRIX_DATA width      = (layer.width_ == 0) ? conf->package_width_ : layer.width_;
  MATRIX_DATA height     = (layer.height_ == 0) ? conf->package_height_ : layer.height_;
  MATRIX_DATA thickness  = (layer.thickness_ == 0) ? conf->pwb_layer_total_thickness_ : layer.thickness_;
  MATRIX_DATA emissivity = 0.8; // PCB resin material

  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
  // store the layer height and width
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;

  // First we need to determine the thickness of just the pwb itself.
  // We only know the ENTIRE package thickness (not the individual portions)
  MATRIX_DATA total_thickness = 0;
  // compute the total thickness of the sample stackup
  for(int i = 0; i < (int)conf->pwb_layer_thickness_.size(); i++)
    total_thickness += conf->pwb_layer_thickness_[i];

  // store the thickness of the pwb from the sample stackup
  MATRIX_DATA pwb_layer_total_thickness = total_thickness;

  // finish accumulating the total thickness of the substrate+PCB regions
  for(int i = 0; i < (int)conf->fcpbga_layer_thickness_.size(); i++)
    total_thickness += conf->fcpbga_layer_thickness_[i];

  MATRIX_DATA percent = pwb_layer_total_thickness / total_thickness; // this is the percent of the stackup that's the substrate
                                                                     // layer

  // Calculate the actual thickness of the substrate layer by assuming that is the same proportions as the sample stackup

  // store the layer thickness
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness * percent; // this is the actual thickness of the substrate

  thickness = layer.thickness_;

  // scale the layer thicknesses based upon the relative thickness of the pwb actually used
  std::vector<MATRIX_DATA> layer_thickness;

  for(int i = 0; i < (int)conf->pwb_layer_thickness_.size(); i++) {
    // this is the percentage of the substrate that this layer occupies
    MATRIX_DATA percentage = conf->pwb_layer_thickness_[i] / pwb_layer_total_thickness;
    layer_thickness.push_back(layer.thickness_ * percentage); // determine the actual layer thickness
  }

  // calculates the equivalent specific heat for the entire layer
  // this is SUM(i=1 to n, %volume_i * specific_heat_i)
  MATRIX_DATA specific_heat = 0;
  for(int i = 0; i < (int)layer_thickness.size(); i++) {
    specific_heat +=
        ((width * height * layer_thickness[i]) / (width * height * layer.thickness_) * conf->pwb_layer_specific_heat_[i]);
  }
  layer.spec_heat_ = specific_heat;

  // calculates the equivalent  density for the entire layer
  // this is SUM(i=1 to n, %volume_i * density_i)
  MATRIX_DATA density = 0;
  for(int i = 0; i < (int)layer_thickness.size(); i++) {
    density += ((width * height * layer_thickness[i]) / (width * height * layer.thickness_) * conf->pwb_layer_density_[i]);
  }
  layer.density_ = density;

  // this calculates the equivalent resistance for the entire layer (vertical)
  MATRIX_DATA resistance = 0;
  for(int i = 0; i < (int)layer_thickness.size(); i++) {
    resistance += (layer_thickness[i] / (conf->pwb_layer_conductivity_vert_[i] * width * height));
  }

  MATRIX_DATA conductivity_vertical = (layer.thickness_ / (width * height * resistance));

  //~8.58X thu-plane conductivity difference, accounting for 25%
  // vias(from "wiring statistics and printed wiring board thermal conductivity")
  layer.vertical_conductivity_down_ = conductivity_vertical * 8.58;

  //~8.58X thu-plane conductivity difference, accounting for 25%
  // vias(from "wiring statistics and printed wiring board thermal conductivity")
  layer.vertical_conductivity_up_ = conductivity_vertical * 8.58;

  MATRIX_DATA conductivity = 0;
  // this calculates the equivalent resistance for the entire layer (horizontal)
  for(int i = 0; i < (int)layer_thickness.size(); i++) {
    conductivity += conf->pwb_layer_conductivity_horiz_[i];
  }
  conductivity /= ((int)layer_thickness.size()); // find the average lateral resistance
  MATRIX_DATA conductivity_horizontal = conductivity;

  //~.53X horizontal in-plane conductivity when accounting for 25% vias
  // (from "wiring statistics and printed wiring board thermal conductivity")
  layer.horizontal_conductivity_ = conductivity_horizontal / .53;
}

// CALCULATE PACKAGE SUBTRATE PROPERTIES
void Material::calc_package_substrate_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA width  = datalibrary_->config_data_->chip_width_;
  MATRIX_DATA height = datalibrary_->config_data_->chip_height_;
  MATRIX_DATA thickness =
      datalibrary_->config_data_->package_thickness_; // this is NOT the actual thickness (it includes a number of layers)
  MATRIX_DATA specific_heat   = 0;
  MATRIX_DATA total_thickness = 0;
  MATRIX_DATA emissivity      = 0.8; // PCB resin material

  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;

  // First we need to determine the thickness of just the substrate itself.
  // We only know the ENTIRE package thickness (not the individual portions)

  // compute the total thickness of the sample stackup
  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++)
    total_thickness += datalibrary_->config_data_->fcpbga_layer_thickness_[i];

  // store the thickness of the substrate from the sample stackup
  MATRIX_DATA substrate_layer_total_thickness = total_thickness;

  // finish accumulating the total thickness of the substrate+PCB regions
  for(int i = 0; i < (int)datalibrary_->config_data_->pwb_layer_thickness_.size(); i++)
    total_thickness += datalibrary_->config_data_->pwb_layer_thickness_[i];

  // this is the percent of the stackup that's the substrate layer
  MATRIX_DATA percent = substrate_layer_total_thickness / total_thickness;

  // Calculate the actual thickness of the substrate layer by assuming that is the same proportions as the sample stackup
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness * percent; // this is the actual thickness of the substrate

  // scale the layer thicknesses based upon the relative thickness of the package substrate actually used
  std::vector<MATRIX_DATA> layer_thickness;

  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++) {
    // this is the percentage of the substrate that this layer occupies
    MATRIX_DATA percentage = datalibrary_->config_data_->fcpbga_layer_thickness_[i] / substrate_layer_total_thickness;
    layer_thickness.push_back(layer.thickness_ * percentage); // determine the actual layer thickness
  }

  // calculates the equivalent specific heat for the entire layer
  // this is SUM(i=1 to n, %volume_i * specific_heat_i)
  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++) {
    specific_heat += ((width * height * layer_thickness[i]) / (width * height * layer.thickness_) *
                      datalibrary_->config_data_->fcpbga_layer_specific_heat_[i]);
  }

  layer.spec_heat_ = specific_heat;

  MATRIX_DATA density = 0;
  // calculates the equivalent density for the entire layer
  // this is SUM(i=1 to n, %volume_i * specific_heat_i)
  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++) {
    density += ((width * height * layer_thickness[i]) / (width * height * layer.thickness_) *
                datalibrary_->config_data_->fcpbga_layer_density_[i]);
  }

  layer.density_ = density;

  // compute the equivalent vertical resistance across the package substrate layer
  MATRIX_DATA resistance = 0;
  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++) {
    resistance += (layer_thickness[i] / (datalibrary_->config_data_->fcpbga_layer_conductivity_vert_[i] * width * height));
  }

  MATRIX_DATA conductivity = (layer.thickness_ / (width * height * resistance));

  layer.vertical_conductivity_down_ =
      conductivity * 8.58; //~8.58X thu-plane conductivity difference when accounting for 25% vias (from "wiring statistics and
                           // printed wiring board thermal conductivity")
  layer.vertical_conductivity_up_ = conductivity * 8.58; //~8.58X thu-plane conductivity difference when accounting for 25% vias
                                                         //(from "wiring statistics and printed wiring board thermal conductivity")

  // this calculates the equivalent resistance for the entire layer (horizontal)

  conductivity = 0;
  for(int i = 0; i < (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size(); i++) {
    conductivity += datalibrary_->config_data_->fcpbga_layer_conductivity_horiz_[i];
  }
  conductivity /= (int)datalibrary_->config_data_->fcpbga_layer_thickness_.size();
  layer.horizontal_conductivity_ = conductivity * .53; //~.53X horizontal in-plane conductivity when accounting for 25% vias  (from
                                                       //"wiring statistics and printed wiring board thermal conductivity")

  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER) | (1 << HEAT_RADIATION_TRANSFER);
}

// CALCULATE PINS LAYER PROPERTIES
void Material::calc_pins_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  int         num_pins       = (int)datalibrary_->config_data_->pin_count_;
  MATRIX_DATA package_area   = datalibrary_->config_data_->package_width_ * datalibrary_->config_data_->package_height_;
  MATRIX_DATA pin_tip_area   = 3.14159265 * powf(datalibrary_->config_data_->pin_pitch_ / 4, 2);
  MATRIX_DATA pin_height     = datalibrary_->config_data_->pins_height_;
  MATRIX_DATA conductivity   = datalibrary_->layer_materials_.find("COPPER").conductivity_;
  MATRIX_DATA spec_heat      = datalibrary_->layer_materials_.find("COPPER").spec_heat_;
  MATRIX_DATA density_copper = datalibrary_->layer_materials_.find("COPPER").density_;
  MATRIX_DATA emissivity     = datalibrary_->layer_materials_.find("COPPER").emissivity_;

  // store conductivity = conductance * layer_thickness/layer_area
  MATRIX_DATA cond_eff = (conductivity * pin_tip_area * num_pins) / (pin_height) * ((pin_height) / package_area);

  // store cond_eff to the pins layer
  layer.vertical_conductivity_up_   = cond_eff;
  layer.vertical_conductivity_down_ = cond_eff;
  layer.horizontal_conductivity_    = 0; // we assume that there is negligible heat transfer from one pin to another

  // calculate the effective specific heat
  // effective specific heat = specific heat of copper * (% area copper versus air)
  // store effective specific heat to pins layer
  MATRIX_DATA percent_copper = (num_pins * pin_tip_area) / (package_area);

  layer.density_   = density_copper * percent_copper;
  layer.spec_heat_ = spec_heat * percent_copper;

  // store height
  if(layer.height_ == 0)
    layer.height_ = datalibrary_->config_data_->package_height_;

  // store width
  if(layer.width_ == 0)
    layer.width_ = datalibrary_->config_data_->package_width_;

  // store thickness
  if(layer.thickness_ == 0)
    layer.thickness_ = pin_height;

  layer.emissivity_            = emissivity;
  layer.heat_transfer_methods_ = (1 << HEAT_CONDUCTION_TRANSFER);
}

// CALCULATE OIL LAYER PROPERTIES
// NOTE: we assume oil flow is along WIDTH of chip (NOT HEIGHT)
void Material::calc_oil_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA density          = datalibrary_->config_data_->coolant_density_;
  MATRIX_DATA viscosity        = datalibrary_->config_data_->coolant_viscosity_;
  MATRIX_DATA conductivity     = datalibrary_->config_data_->coolant_thermal_conductivity_;
  MATRIX_DATA spec_heat        = datalibrary_->config_data_->coolant_specific_heat_;
  MATRIX_DATA prandtl_number   = datalibrary_->config_data_->coolant_prandtl_number_;
  MATRIX_DATA flow_rate        = datalibrary_->config_data_->coolant_flow_rate_;
  MATRIX_DATA angle            = datalibrary_->config_data_->coolant_angle_;
  MATRIX_DATA nozzle_diameter  = datalibrary_->config_data_->coolant_nozzle_diameter_;
  MATRIX_DATA coverage_percent = datalibrary_->config_data_->coolant_coverage_percent_;
  MATRIX_DATA height           = datalibrary_->config_data_->chip_height_;
  MATRIX_DATA width =
      datalibrary_->config_data_->chip_width_; // NOTE: we assume oil flow is along WIDTH of chip (the longer part) (NOT HEIGHT)

  // compute thickness of oil across chip as a function of the angle and the nozzle diameter
  // Note: this is not the actual thickness of the oil on the chip, but instead an "effective thermal capacitance" computation for
  // the oil volume
  MATRIX_DATA thickness = (nozzle_diameter * powf(cos(angle), 2)) / 2;

  // compute the volume of oil flowing over the chip
  MATRIX_DATA eff_oil_volume = thickness * height * width;

  // based upon the volume flow rate of the nozzle and that of the oil flowing over the chip, compute the effective velocity
  // this is augmented by coverage_percent because we assume that a certain amount of the flow is lost
  // this is flow_rate_nozzle/eff_oil_volume
  // this is the speed at which the oil is flowing over the chip
  MATRIX_DATA eff_velocity = coverage_percent * flow_rate / eff_oil_volume;

  // compute the reynold's number
  // if the flow becomes non-laminar, it is NOT modeled (quit in this case)

  MATRIX_DATA r_el = (eff_velocity * width) / viscosity;

  if(r_el > 20000) {
    std::cerr << "Fatal: Material::calc_oil_layer_properties -> Reynold's Number is [" << r_el
              << "]. This indicates turbulent flow. This is not modeled!"
              << " eff_velocity " << eff_velocity << " flow_rate " << flow_rate << " oil volume " << eff_oil_volume << std::endl;
    exit(1);
  }

  MATRIX_DATA nusselt_average         = (conductivity / width) * 0.664 * sqrtf(r_el) * powf(prandtl_number, .3333333333);
  MATRIX_DATA average_convection_coef = (nusselt_average * conductivity) / width;

  // local convection coeff is 1/2 the average one in this case
  MATRIX_DATA local_convection_coef = average_convection_coef / 2;

  datalibrary_->config_data_->oil_layer_convection_coefficient_ = local_convection_coef;
  datalibrary_->config_data_->oil_used_                         = true;
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  layer.spec_heat_ = spec_heat;
  layer.density_   = density;
  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;
  layer.horizontal_conductivity_    = conductivity;
  layer.vertical_conductivity_down_ = conductivity;
  layer.vertical_conductivity_up_   = conductivity;
  layer.heat_transfer_methods_      = (1 << HEAT_CONVECTION_TRANSFER);
  layer.convection_coefficient_     = average_convection_coef;
#ifdef _ESESCTHERM_DEBUG
  MATRIX_DATA heat_transfer_rate_local = (width) * (30) * (local_convection_coef);
  std::cerr << "[DEBUG INFO] Material::calc_oil_layer_properties() => Local Heat Transfer Rate (for 30K drop) is ["
            << local_convection_coef << "]" << std::endl;
#endif
}

// CALCULATE SILICON LAYER PROPERTIES
void Material::calc_silicon_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA height         = datalibrary_->config_data_->chip_height_;
  MATRIX_DATA width          = datalibrary_->config_data_->chip_width_;
  MATRIX_DATA chip_thickness = datalibrary_->config_data_->chip_thickness_;
  if(layer.width_ == 0)
    layer.width_ = width;
  if(layer.height_ == 0)
    layer.height_ = height;
  layer.density_   = datalibrary_->layer_materials_.find("BULK_SI").density_;
  layer.spec_heat_ = datalibrary_->layer_materials_.find("BULK_SI").spec_heat_;

  layer.spec_heat_ *= CAPACITANCE_FACTOR_BULK;

  // Silicon conductivity is temperature-dependent
  // 117.5 - 0.42*(T-100) is what is typically used (FLOTHERM)
  MATRIX_DATA tmp                   = 117.5 - 0.42 * (datalibrary_->config_data_->init_temp_ - 273.15 - 100);
  layer.horizontal_conductivity_    = tmp;
  layer.vertical_conductivity_down_ = tmp;
  layer.vertical_conductivity_up_   = tmp;

  // what is the %of the sample stackup that the substrate layer occupies?
  // this percentage is used to find the actual thickness of the silicon substrate for the chip_thickness
  MATRIX_DATA t_sub          = datalibrary_->config_data_->sample_silicon_substrate_thickness_;
  MATRIX_DATA t_box          = datalibrary_->config_data_->sample_box_thickness_;
  MATRIX_DATA t_si           = datalibrary_->config_data_->sample_silicon_film_thickness_;
  int         technology     = datalibrary_->config_data_->technology_;
  MATRIX_DATA t_interconnect = 0;
  int         num_layers     = (int)DataLibrary::technology_parameters_[TECH_LEVELS][technology];
  for(int i = 0; i < num_layers; i++) {
    t_interconnect = DataLibrary::technology_parameters_[10 + (i * 9) + TECH_H]
                                                        [technology]; // get the thickness of the layer for the given technology
  }

  MATRIX_DATA percentage_silicon_substrate = t_sub / (t_sub + t_box + t_si + t_interconnect); // t_sub/(t_sub+t_box+t_si)
  if(layer.thickness_ == 0)
    layer.thickness_ = percentage_silicon_substrate * chip_thickness;
  layer.heat_transfer_methods_ = (1 << HEAT_RADIATION_TRANSFER) | (1 << HEAT_CONDUCTION_TRANSFER);
  layer.emissivity_            = datalibrary_->layer_materials_.find("BULK_SI").emissivity_;
}

// This is based upon "Wiring Statistics and Printed Wiring Board Thermal Conductivity" Richard D Nelson, Teravicta Technologies,
// IEEE SEMI-THERM 2001 Data Taken from "summary of thermal conductivity results for full PWB models" Wire Width: 8mills, Wire
// thickness: 1mill Dielectric thickness: 6mills Lateral space between wires 16mills Copper Via Drill Diameter: 8mills Copper Vias
// Plating Thickness: 1mill Minimum Wire Length/Spacing: 3.6mm Maximum Wire Length/Spacing: 7.2mm Using PWB model result summary,
// Un-wired P/g and 25% vias W3S1 used Percent Copper in entire PWB: 7.34% Kz=1.58W/mk Kx,y=14.8W/mk 12"x9.6" (inches)
void Material::calc_pcb_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  layer.horizontal_conductivity_    = 14.8;
  layer.vertical_conductivity_up_   = 1.58;
  layer.vertical_conductivity_down_ = 1.58;
  layer.heat_transfer_methods_      = (1 << HEAT_RADIATION_TRANSFER) | (1 << HEAT_CONDUCTION_TRANSFER);
  if(layer.thickness_ == 0)
    layer.thickness_ = .0016; // 1.6mm thickness
  if(layer.height_ == 0)
    layer.height_ = .24384; // 243.84cm height
  if(layer.width_ == 0)
    layer.width_ = .3048; // 304.8cm width
  layer.emissivity_ = datalibrary_->layer_materials_.find("COPPER").emissivity_ * .0734 + 0.8 * .9266;
  layer.spec_heat_  = datalibrary_->layer_materials_.find("COPPER").spec_heat_ * .0734 + 1200 * .9266;
  layer.density_    = datalibrary_->layer_materials_.find("COPPER").density_ * .0734 + 2080 * .9266;
}

// CALCULATE AIR LAYER PROPERTIES
// NOTE: we assume flow is along WIDTH of chip (NOT HEIGHT)

// Methodology for Convection Calculation
// 1. Assume flow over flat plate with transition reynolds number of 5x10^5
// 2. Assume moderate boundary-layer temperture differences. Use film temperature relation based upon equation 7.2 in Dewitt
// 3. Assume dilute, binary mixtures
// 4. Determine wether the flow is laminar or turbulent using the reynolds number
// 5. Determine the local convection coefficient

// This approximation is taken from the "Heat Transfer Handbook"
// Compute the average convection coefficient
// We assume that the Critical reynolds  number is 5x10^5
// 0.6 < Pr < 60
// 5x10^5 < Re_l <= 10^8
// Re_x,c=5x10^5
// Nu_L_ave = (0.037Re_L^(4/5) - 871) Pr^(1/3)
// These were computed using the following data values (taken from Incropera and Dewitt):
// All values are at a pressure of 1 atm
// T(c)          v*10^6(m^2/s)   Pr              k*10^3 (W/mK)
//-172.15       2                       0.786   9.34
//-122.15       4.426           0.758   13.8
//-72.15        7.59            0.737   18.1
//-22.15        11.44           0.72    22.3
// 27.85         15.89           0.707   26.3
// 77.85         20.92           0.7             30
// 127.85        26.41           0.69    33.8
// 177.85        32.39           0.686   37.3
// 227.85        38.79           0.684   40.7
// 277.85        45.57           0.683   43.9
// 327.85        52.69           0.685   46.9

// y = -3E-05x^2 + 0.0801x + 24.056                      k(T), for domain [-172.15,327.85], T(C)
// y = 0.0001x^2 + 0.0869x + 13.587                      V(T), for domain[-172.15,327.85], T(C)
// y = 6E-07x^2 - 0.0003x + 0.7159                       Pr(T), for domain [-172.15,327.85], T(C)

void Material::calc_air_layer_properties_dyn(int layer, int x, int y, DataLibrary *datalibrary_) {
  MATRIX_DATA nusselt_ave  = 0;
  MATRIX_DATA fan_distance = abs((int)datalibrary_->config_data_->fan_distance_);
  ModelUnit & temp_unit    = (*datalibrary_->all_layers_info_[layer]->floorplan_layer_dyn_)[x][y];
  MATRIX_DATA ambient_temp = datalibrary_->config_data_->ambient_temp_;
  MATRIX_DATA air_pressure = datalibrary_->config_data_->air_pressure_;
  // film temperature is used to account for non-uniformity in the fluid properties due to changing temperature across boundary
  // layer
  double film_temperature = ((ambient_temp - 273.15 + *(temp_unit.get_temperature()) - 273.15) / 2);
  double fan_velocity     = datalibrary_->config_data_->fan_velocity_; // air flow rate
  // determine kinematic viscosity @ 1atm
  MATRIX_DATA kinematic_viscosity = (0.0001 * double_pow(film_temperature, 2) + 0.0869 * film_temperature + 13.587) * 1.0e-6;
  // k, Pr, and u may be assumed to be independant of pressure, but kinematic viscosity (v=u/p) will vary with pressure
  // through its dependance upon density.
  // From the ideal gas law PV=nRT. Thus, the ratio of the kinematic viscosities will be the inverse of the pressures.
  // v1/v2=p2/p1
  kinematic_viscosity = kinematic_viscosity * (1.0133E5 / air_pressure); // compensate for pressure difference
  // determine the location of the transition
  double      temp_unit_leftx  = temp_unit.leftx_ + fan_distance; // offset axis by the distance of the fan to the chip
  double      temp_unit_rightx = temp_unit.rightx_ + fan_distance;
  MATRIX_DATA transition_point = MIN(kinematic_viscosity / fan_velocity * 5.0e5, 1e8);

  double reynolds_number_right      = fan_velocity * (temp_unit_rightx) / kinematic_viscosity;
  double reynolds_number_left       = fan_velocity * (temp_unit_leftx) / kinematic_viscosity;
  double reynolds_number_transition = fan_velocity * (transition_point) / kinematic_viscosity;
  double prandtl_number             = 6E-07 * double_pow(film_temperature, 2) - 0.0003 * film_temperature + 0.7159;
  if(prandtl_number < 0.6) {
    std::cerr << "Material::calc_air_layer_properties_dyn => prandtl_number cannot be less than 0.6 for Dewitt equations to be used"
              << std::endl;
    exit(1);
  }
  double air_conductivity = (-3E-05 * double_pow(film_temperature, 2) + 0.0801 * film_temperature + 24.056) * 1.0e-3;

  for(int dir = 0; dir < MAX_DIR; dir++)
    temp_unit.conduct_center_[dir] = air_conductivity;

  // in this case it is completely laminar
  // we use equation 7.31 in Dewitt

  if(temp_unit_rightx < transition_point) {
    nusselt_ave = 0.664 * (double_pow(reynolds_number_right, .5) - double_pow(reynolds_number_left, .5)) *
                  double_pow(prandtl_number, .33333333333);
  } else {
    if(LT(reynolds_number_right, 5.0e5) || GT(reynolds_number_right, 1.0e8)) {
      std::cerr << "Material::calc_air_layer_properties_dyn => reynolds number outside domain [5x10^5,1.0e8]" << std::endl;
      exit(1);
    }
    if(prandtl_number > 60) {
      std::cerr
          << "Material::calc_air_layer_properties_dyn => prandtl_number cannot be greater than 60 for Dewitt equations to be used"
          << std::endl;
      exit(1);
    }

    // mixed laminar and turbulent flow conditions
    if(temp_unit_leftx < transition_point && temp_unit_rightx > transition_point)
      nusselt_ave = 0.664 * (double_pow(reynolds_number_transition, .5) - double_pow(reynolds_number_left, 0.5)) +
                    0.037 * (double_pow(reynolds_number_right, .8) - double_pow(reynolds_number_transition, .8)) *
                        double_pow(prandtl_number, .333333333);
    else // only turbulent flow conditions
      nusselt_ave = 0.037 * (double_pow(reynolds_number_right, .8) - double_pow(reynolds_number_left, .8)) *
                    double_pow(prandtl_number, .333333);
  }
  MATRIX_DATA convection_coefficient = 0;
  if(datalibrary_->config_data_->fan_used_)
    convection_coefficient = temp_unit.convection_coefficient_ =
        nusselt_ave * air_conductivity / (temp_unit_rightx - temp_unit_leftx);
  else
    convection_coefficient = temp_unit.convection_coefficient_ = 0;

  datalibrary_->all_layers_info_[layer]->convection_coefficient_ = convection_coefficient;
}

// CALCULATE AIR LAYER PROPERTIES
// NOTE: we assume flow is along WIDTH of chip (NOT HEIGHT)

void Material::calc_air_layer_properties(ChipLayers &layer, DataLibrary *datalibrary_) {
  MATRIX_DATA density      = 1.205; // 1.205kg/m^3
  MATRIX_DATA spec_heat    = 1.005; // 1.005kg/m^3
  MATRIX_DATA ambient_temp = datalibrary_->config_data_->ambient_temp_;
  MATRIX_DATA air_pressure = datalibrary_->config_data_->air_pressure_;
  int         layer_above  = MIN(layer.layer_number_ + 1, (int)(datalibrary_->all_layers_info_.size() - 1));
  int         layer_below  = MAX(layer.layer_number_ - 1, 0);
  MATRIX_DATA height       = std::max(datalibrary_->all_layers_info_[layer_above]->height_,
                                datalibrary_->all_layers_info_[layer_below]
                                    ->height_); // make the height the same as the maximum height of the adjacent layers
  MATRIX_DATA width        = std::max(
      datalibrary_->all_layers_info_[layer_above]->width_,
      datalibrary_->all_layers_info_[layer_below]->width_); // make the width the same as the maximum width of the adjacent layers
  MATRIX_DATA fan_velocity = datalibrary_->config_data_->fan_velocity_;
  // assume that there is 1m thickness of air above the chip unless specified
  MATRIX_DATA thickness = 1;

  if(layer.width_ == 0) {
    if(width != 0)
      layer.width_ = width;
    else
      layer.width_ = datalibrary_->config_data_->chip_width_;
  }

  if(layer.height_ == 0) {
    if(height != 0)
      layer.height_ = height;
    else
      layer.height_ = datalibrary_->config_data_->chip_height_;
  }

  layer.spec_heat_ = spec_heat;
  layer.density_   = density;

  if(layer.thickness_ == 0)
    layer.thickness_ = thickness;

  MATRIX_DATA nusselt_ave  = 0;
  MATRIX_DATA fan_distance = abs((int)datalibrary_->config_data_->fan_distance_);
  // film temperature is used to account for non-uniformity in the fluid properties due to changing temperature across boundary
  // layer
  double film_temperature = ((ambient_temp - 273.15 + 60) / 2);
  // determine kinematic viscosity @ 1atm
  double kinematic_viscosity = (0.0001 * double_pow(film_temperature, 2) + 0.0869 * film_temperature + 13.587) * 1.0e-6;
  // k, Pr, and u may be assumed to be independant of pressure, but kinematic viscosity (v=u/p) will vary with pressure
  // through its dependance upon density.
  // From the ideal gas law PV=nRT. Thus, the ratio of the kinematic viscosities will be the inverse of the pressures.
  // v1/v2=p2/p1
  kinematic_viscosity = kinematic_viscosity * (1.0133E5 / air_pressure); // compensate for pressure difference
  // determine the location of the transition
  MATRIX_DATA leftx            = fan_distance; // offset axis by the distance of the fan to the chip
  MATRIX_DATA rightx           = width + fan_distance;
  MATRIX_DATA transition_point = MIN(kinematic_viscosity / fan_velocity * 5.0e5, 1e8);

  MATRIX_DATA reynolds_number_right      = round(fan_velocity * (rightx) / kinematic_viscosity);
  MATRIX_DATA reynolds_number_left       = round(fan_velocity * (leftx) / kinematic_viscosity);
  MATRIX_DATA reynolds_number_transition = round(fan_velocity * (transition_point) / kinematic_viscosity);
  double      prandtl_number             = 6E-07 * double_pow(film_temperature, 2) - 0.0003 * film_temperature + 0.7159;
  if(prandtl_number < 0.6) {
    std::cerr << "Material::calc_air_layer_properties => prandtl_number cannot be less than 0.6 for Dewitt equations to be used"
              << std::endl;
    exit(1);
  }
  double air_conductivity = (-3E-05 * double_pow(film_temperature, 2) + 0.0801 * film_temperature + 24.056) * 1.0e-3;
  // in this case it is completely laminar
  // we use equation 7.31 in Dewitt

  // the thermal conductivity of the air is a function of the surface temperature
  // this is computed for each model-unit at runtime
  layer.horizontal_conductivity_    = air_conductivity;
  layer.vertical_conductivity_down_ = air_conductivity;
  layer.vertical_conductivity_up_   = air_conductivity;
  layer.heat_transfer_methods_      = (1 << HEAT_CONVECTION_TRANSFER);

  if(rightx < transition_point) {
    nusselt_ave = 0.664 * (double_pow(reynolds_number_right, .5) - double_pow(reynolds_number_left, .5)) *
                  double_pow(prandtl_number, .33333333333);
  } else {
    if(LT(reynolds_number_right, 5.0e5) || GT(reynolds_number_right, 1.0e8)) {
      std::cerr << "Material::calc_air_layer_properties => reynolds number outside domain [5x10^5,1.0e8]" << std::endl;
      exit(1);
    }
    if(prandtl_number > 60) {
      std::cerr << "Material::calc_air_layer_properties => prandtl_number cannot be greater than 60 for Dewitt equations to be used"
                << std::endl;
      exit(1);
    }
    // mixed laminar and turbulent flow conditions
    if(leftx < transition_point && rightx > transition_point)
      nusselt_ave = 0.664 * (double_pow(reynolds_number_transition, .5) - double_pow(reynolds_number_left, 0.5)) *
                        double_pow(prandtl_number, .333333333) +
                    0.037 * (double_pow(reynolds_number_right, .8) - double_pow(reynolds_number_transition, .8)) *
                        double_pow(prandtl_number, .333333333);
    else // only turbulent flow conditions
      nusselt_ave = 0.037 * (double_pow(reynolds_number_right, .8) - double_pow(reynolds_number_left, .8)) *
                    double_pow(prandtl_number, .333333);
  }
  MATRIX_DATA convection_coefficient = nusselt_ave * air_conductivity / (rightx - leftx);
  // the convection coefficient is a function of the surface temperature. Therefore, we need to know the temperature first.
  // this is computed for each model-unit at runtime
  if(datalibrary_->config_data_->fan_used_)
    layer.convection_coefficient_ = convection_coefficient;
  else
    layer.convection_coefficient_ = 0;
}
