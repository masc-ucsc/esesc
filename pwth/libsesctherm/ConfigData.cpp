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
File name:      ConfigData.cpp
Description:    This handles the reading of the configuration file and
                setting initial model parameters
********************************************************************************/

#include <complex>
#include <fstream>
#include <iostream>
#include <map>

#include "sescthermMacro.h"

#include "SescConf.h"

#include "ClassDeclare.h"
#include "sesctherm3Ddefine.h"

#include "ChipLayers.h"
#include "ChipMaterial.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ModelUnit.h"
#include "ThermGraphics.h"
#include "Utilities.h"
#include "nanassert.h"

ConfigData::ConfigData(DataLibrary *datalibrary) {
  datalibrary_ = datalibrary;

  generate_ucool_floorplan_ = false;
  //	num_pins_=478; //number of pins for standard pentium M PPGA package
  //	num_gates_=250000000;				//250M gates by default

  // default stochastic parameters:
  //	wire_stochastic_p_=0.8;
  //	wire_stochastic_k_=5.0;
  //	wire_stochastic_fanout_=3.0;
  //	num_metal_layers_=12;
  //	polysilicon_impurity_concentration_=2.0e24;	//boron-doped silicon
  //	polysilicon_grain_size_=400e-9;

  fcpbga_layer_thickness_.push_back(0.022e-3); //(m)		//TOP_SM
  fcpbga_layer_thickness_.push_back(0.017e-3); // M1
  fcpbga_layer_thickness_.push_back(0.033e-3); // BT1
  fcpbga_layer_thickness_.push_back(0.017e-3); // M2
  fcpbga_layer_thickness_.push_back(0.033e-3); // BT3
  fcpbga_layer_thickness_.push_back(0.027e-3); // M3
  fcpbga_layer_thickness_.push_back(0.800e-3); // BT_CORE
  fcpbga_layer_thickness_.push_back(0.027e-3); // M4
  fcpbga_layer_thickness_.push_back(0.033e-3); // BT4
  fcpbga_layer_thickness_.push_back(0.017e-3); // M5
  fcpbga_layer_thickness_.push_back(0.033e-3); // BT5
  fcpbga_layer_thickness_.push_back(0.017e-3); // M6
  fcpbga_layer_thickness_.push_back(0.022e-3); // BOTTOM_SM
  fcpbga_layer_thickness_.push_back(0.460e-3); // EQ_C5_LAYER

  fcpbga_layer_total_thickness_ = 0;

  for(int i = 0; i < (int)fcpbga_layer_thickness_.size(); i++) {
    fcpbga_layer_total_thickness_ += fcpbga_layer_thickness_[i];
  }

  fcpbga_layer_conductivity_vert_.push_back(0.2);  // TOP_SM
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M1
  fcpbga_layer_conductivity_vert_.push_back(0.17); // BT1
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M2
  fcpbga_layer_conductivity_vert_.push_back(0.17); // BT3
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M3
  fcpbga_layer_conductivity_vert_.push_back(0.17); // BT_CORE
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M4
  fcpbga_layer_conductivity_vert_.push_back(0.17); // BT4
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M5
  fcpbga_layer_conductivity_vert_.push_back(0.17); // BT5
  fcpbga_layer_conductivity_vert_.push_back(3.5);  // M6
  fcpbga_layer_conductivity_vert_.push_back(0.2);  // BOTTOM_SM
  fcpbga_layer_conductivity_vert_.push_back(16.6); // EQ_C5_LAYER

  fcpbga_layer_conductivity_horiz_.push_back(0.2);  // TOP_SM
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M1
  fcpbga_layer_conductivity_horiz_.push_back(0.17); // BT1
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M2
  fcpbga_layer_conductivity_horiz_.push_back(0.17); // BT2
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M3
  fcpbga_layer_conductivity_horiz_.push_back(0.17); // BT_CORE
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M4
  fcpbga_layer_conductivity_horiz_.push_back(0.17); // BT4
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M5
  fcpbga_layer_conductivity_horiz_.push_back(0.17); // BT5
  fcpbga_layer_conductivity_horiz_.push_back(90);   // M6
  fcpbga_layer_conductivity_horiz_.push_back(0.2);  // BOTTOM_SM
  fcpbga_layer_conductivity_horiz_.push_back(16.6); // EQ_C5_LAYER

  fcpbga_layer_specific_heat_.push_back(180);                    // TOP_SM
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M1 (25% copper, 75% BT)
  fcpbga_layer_specific_heat_.push_back(1520);                   // BT1
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M2
  fcpbga_layer_specific_heat_.push_back(1520);                   // BT2
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M3
  fcpbga_layer_specific_heat_.push_back(1520);                   // BT_CORE
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M4
  fcpbga_layer_specific_heat_.push_back(1520);                   // BT4
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M5
  fcpbga_layer_specific_heat_.push_back(1520);                   // BT5
  fcpbga_layer_specific_heat_.push_back(.25 * 380 + .75 * 1520); // M6
  fcpbga_layer_specific_heat_.push_back(180);                    // BOTTOM_SM
  fcpbga_layer_specific_heat_.push_back(180); // EQ_C5_LAYER FIXME: this value is off, should be determined using submodel

  fcpbga_layer_density_.push_back(1400);                    // TOP_SM
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M1 (25% copper, 75% BT)
  fcpbga_layer_density_.push_back(1140);                    // BT1
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M2
  fcpbga_layer_density_.push_back(1140);                    // BT2
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M3
  fcpbga_layer_density_.push_back(1140);                    // BT_CORE
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M4
  fcpbga_layer_density_.push_back(1140);                    // BT4
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M5
  fcpbga_layer_density_.push_back(1140);                    // BT5
  fcpbga_layer_density_.push_back(.25 * 8933 + .75 * 1140); // M6
  fcpbga_layer_density_.push_back(1400);                    // BOTTOM_SM
  fcpbga_layer_density_.push_back(1400); // EQ_C5_LAYER FIXME: this value is off, should be determined using submodel

  pwb_layer_thickness_.push_back(0.036e-3); // SM
  pwb_layer_thickness_.push_back(0.036e-3); // M1
  pwb_layer_thickness_.push_back(0.392e-3); // FR4	//NELCO N4000-2 multifunctional epoxy laminate & prepreg
  pwb_layer_thickness_.push_back(0.036e-3); // M2
  pwb_layer_thickness_.push_back(0.613e-3); // FR4
  pwb_layer_thickness_.push_back(0.036e-3); // M3
  pwb_layer_thickness_.push_back(0.392e-3); // FR4
  pwb_layer_thickness_.push_back(0.033e-3); // SM

  pwb_layer_total_thickness_ = 0;

  for(int i = 0; i < (int)pwb_layer_thickness_.size(); i++) {
    pwb_layer_total_thickness_ += pwb_layer_thickness_[i];
  }

  pwb_layer_conductivity_vert_.push_back(1.4); // SM
  pwb_layer_conductivity_vert_.push_back(3.5); // M1
  pwb_layer_conductivity_vert_.push_back(1.6); // FR4
  pwb_layer_conductivity_vert_.push_back(3.5); // M2
  pwb_layer_conductivity_vert_.push_back(1.6); // FR4
  pwb_layer_conductivity_vert_.push_back(3.5); // M3
  pwb_layer_conductivity_vert_.push_back(1.6); // FR4
  pwb_layer_conductivity_vert_.push_back(1.4); // SM

  pwb_layer_conductivity_horiz_.push_back(.245); // SM
  pwb_layer_conductivity_horiz_.push_back(90);   // M1
  pwb_layer_conductivity_horiz_.push_back(.35);  // FR4
  pwb_layer_conductivity_horiz_.push_back(90);   // M2
  pwb_layer_conductivity_horiz_.push_back(.35);  // FR4
  pwb_layer_conductivity_horiz_.push_back(90);   // M3
  pwb_layer_conductivity_horiz_.push_back(.35);  // FR4
  pwb_layer_conductivity_horiz_.push_back(.245); // SM

  pwb_layer_specific_heat_.push_back(180);                    // SM
  pwb_layer_specific_heat_.push_back(.25 * 380 + .75 * 1200); // M1 (25% copper, 75% FR4)
  pwb_layer_specific_heat_.push_back(1200);                   // FR4
  pwb_layer_specific_heat_.push_back(.25 * 380 + .75 * 1200); // M2
  pwb_layer_specific_heat_.push_back(1200);                   // FR4
  pwb_layer_specific_heat_.push_back(.25 * 380 + .75 * 1200); // M3
  pwb_layer_specific_heat_.push_back(1200);                   // FR4
  pwb_layer_specific_heat_.push_back(180);                    // SM

  pwb_layer_density_.push_back(1400);                    // SM
  pwb_layer_density_.push_back(.25 * 8933 + .75 * 2080); // M1 (25% copper, 75% FR4)
  pwb_layer_density_.push_back(2500);                    // FR4
  pwb_layer_density_.push_back(.25 * 8933 + .75 * 2080); // M2
  pwb_layer_density_.push_back(2500);                    // FR4
  pwb_layer_density_.push_back(.25 * 8933 + .75 * 2080); // M3
  pwb_layer_density_.push_back(2500);                    // FR4
  pwb_layer_density_.push_back(1400);                    // SM

  /*The following are the properties for several PCB samples. Values are accurate to within 10% of actual
    [WmÐ1KÐ1] = 0.8 + 350(ZCu/Z) and

    N [WmÐ1KÐ1] = [1.69(1 Ð ZCu/Z)) + 0.0026(ZCu/Z)]Ð1
    1. K. Azar and J E Graebner, Experimental Determination of Thermal Conductivity of Printed Wiring Boards, to appear in the
    Journal of Electronic Packaging, 1995
   */

  // const MATRIX_DATA sesctherm3Dconfig::pcb_parameters_[8][2] = {
  //{2,},
  // Sample 	N 	Nc 	Z		ZCu 	Topical circuitry 	KN		KP
  // PC1		4 	2 	0.166 	2 x 66 	none				0.64	-
  // PC2		6 	4 	0.168 	4 x 66 	none				0.71 	-
  // PC5		2 	0 	0.156 	0		none				-		0.81
  // PC6		8 	2 	0.147 	2 x 34 	none				-		15.9
  // PC7		8 	2 	0.143 	2 x 34 	many vias			-		14.6
  // PC8		8 	2 	0.146 	2 x 34 	surface mounts		-		14.5
  // PC11		1 	1 	0.150 	1 x 32 	none				-		9.0
  // PC12		6	1 	0.149 	1 x 34 	little				-		8.1

  // Sample Transistor Parameters
  // this is scaled based upon the technology
  // This provides sample parameters for the dimensions of a silicon-on-insulator transistor,
  // and box thickness (500nm process)
  // This is scaled based upon chip_thickness_
  // Many of these taken from "Efficient Thermal Modeling of SOI MOSFETS for Fast Dynamic Operation"
  sample_oxide_thickness_             = 3e-9;    // 3nm
  sample_gate_length_                 = 0.22e-6; // 0.22um
  sample_fox_length_                  = 2e-6;    // 2um
  sample_eff_channel_length_          = 0.18e-6; //.18um
  sample_contact_length_              = 0.25e-6; //.25um
  sample_silicon_substrate_thickness_ = 1.5e-6;  // 1.5um
  sample_silicon_film_length_         = 1.18e-6; // 1.18um
  sample_silicon_film_thickness_      = 0.04e-6; // 0.04um
  sample_box_thickness_               = 0.15e-6; // 0.15um
  sample_vthreshold_                  = 0.22;    // 0.22V
  sample_oxide_conductivity_ = 0.82; // 0.82W/m*K (based upon value from "Measurements of Buried Oxide Thermal Conductivity for..."
  sample_oxide_conductivity_transistor_ = 1.79; //(2W/m*K) this corresponds to the BOX/Silicon effective conductivity
  // this is different from the sample oxide conductivity
  // which is the bulk conductivity of the BOX/FOX
  sample_silicon_island_conductivity_ = 63;      // 63W/mK
  sample_gate_width_                  = 2860e-9; // 2860nm gate width
  sample_thermal_capacity_silicon_    = 1.63e6;  // 1.63MJ/(m^3*K)
  sample_thermal_capacity_oxide_      = 3.066e6; // 3.06MJ/(m^3*K)
}

void ConfigData::get_config_data() {
  int         min, max;
  const char *thermSec = SescConf->getCharPtr("", "thermal");
  std::string name_str;
  // GET BASIC PROPERTIES
  const char *modelconfigSec = SescConf->getCharPtr(thermSec, "model");

  init_temp_    = SescConf->getDouble(modelconfigSec, "initialTemp");
  ambient_temp_ = SescConf->getDouble(modelconfigSec, "ambientTemp");

  {
    // Adjust default_time_increment_ so that it is a multiple of the sampling frequency
    int         cyclesPerSample  = SescConf->getInt(modelconfigSec, "CyclesPerSample");
    MATRIX_DATA frequency        = SescConf->getDouble("technology", "Frequency");
    MATRIX_DATA seconds_per_step = cyclesPerSample / frequency;
    int         nsamples         = SescConf->getInt(modelconfigSec, "PowerSamplesPerThermSample");

    default_time_increment_ = seconds_per_step * nsamples;
  }

  // GET GRAPHICS PROPERTIES
  const char *graphicsconfigSec = SescConf->getCharPtr(thermSec, "graphics");
  enable_graphics_              = SescConf->getBool(graphicsconfigSec, "enableGraphics");
  resolution_x_                 = SescConf->getInt(graphicsconfigSec, "resolution_x");
  resolution_y_                 = SescConf->getInt(graphicsconfigSec, "resolution_y");
  enable_perspective_view_      = SescConf->getBool(graphicsconfigSec, "perspective_view");
  graphics_floorplan_layer_     = SescConf->getInt(graphicsconfigSec, "graphics_floorplan_layer");

  min = SescConf->getRecordMin(graphicsconfigSec, "graphics_file_type");
  max = SescConf->getRecordMax(graphicsconfigSec, "graphics_file_type");

  for(int id = min; id <= max; id++) {
    std::string             temp_str(SescConf->getCharPtr(graphicsconfigSec, "graphics_file_type", id));
    ThermGraphics_FileType *file_type = new ThermGraphics_FileType(temp_str);
    graphics_file_types_.push_back(file_type);
  }

  // GET COOLING PROPERTIES
  const char *coolingconfigSec = SescConf->getCharPtr(thermSec, "cooling");
  fan_used_                    = SescConf->getInt(coolingconfigSec, "Fan_Used");
  fan_velocity_                = SescConf->getDouble(coolingconfigSec, "Fan_Velocity");
  fan_distance_                = SescConf->getDouble(coolingconfigSec, "Fan_Distance");
  air_pressure_                = SescConf->getDouble(coolingconfigSec, "Air_Pressure");

  coolant_density_              = SescConf->getDouble(coolingconfigSec, "Coolant_density");
  coolant_viscosity_            = SescConf->getDouble(coolingconfigSec, "Coolant_viscosity");
  coolant_thermal_conductivity_ = SescConf->getDouble(coolingconfigSec, "Coolant_thermal_conductivity");
  coolant_specific_heat_        = SescConf->getDouble(coolingconfigSec, "Coolant_specific_heat");
  coolant_prandtl_number_       = SescConf->getDouble(coolingconfigSec, "Coolant_prandtl_number");
  coolant_flow_rate_            = SescConf->getDouble(coolingconfigSec, "Coolant_flow_rate");
  coolant_angle_                = SescConf->getDouble(coolingconfigSec, "Coolant_angle");
  coolant_nozzle_diameter_      = SescConf->getDouble(coolingconfigSec, "Coolant_nozzle_diameter");
  coolant_coverage_percent_     = SescConf->getDouble(coolingconfigSec, "Coolant_coverage_percent");

  // GET THE CHIP PARAMETERS
  const char *chipconfigSec = SescConf->getCharPtr(thermSec, "chip");
  transistor_count_         = SescConf->getDouble(chipconfigSec, "transistor_count");
  pin_count_                = SescConf->getDouble(chipconfigSec, "pin_count");
  pins_height_              = SescConf->getDouble(chipconfigSec, "pins_height");
  pin_pitch_                = SescConf->getDouble(chipconfigSec, "pin_pitch");
  chip_width_               = SescConf->getDouble(chipconfigSec, "chip_width");
  chip_height_              = SescConf->getDouble(chipconfigSec, "chip_height");
  chip_thickness_           = SescConf->getDouble(chipconfigSec, "chip_thickness");
  package_height_           = SescConf->getDouble(chipconfigSec, "package_height");
  package_width_            = SescConf->getDouble(chipconfigSec, "package_width");
  package_thickness_        = SescConf->getDouble(chipconfigSec, "package_thickness");
  technology_               = SescConf->getInt(chipconfigSec, "technology");
  switch(technology_) {
  case 250:
    technology_ = TECH_250;
    break;
  case 180:
    technology_ = TECH_180;
    break;
  case 130:
    technology_ = TECH_130;
    break;
  case 90:
    technology_ = TECH_90;
    break;
  case 65:
    technology_ = TECH_65;
    break;
  default:
    std::cerr << "sesctherm3Dconfig::get_config_data() => Invalid process technology [" << technology_ << "] defined in config file"
              << std::endl;
    exit(1);
  }

  // GET THE HEAT SINK/SPREADER PROPERTIES
  const char *spreadersinkconfigSec = SescConf->getCharPtr(thermSec, "spreader_sink");
  heat_sink_height_                 = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_height");    // height
  heat_sink_width_                  = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_width");     // width
  heat_sink_thickness_              = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_thickness"); // thickness

  heat_spreader_height_    = SescConf->getDouble(spreadersinkconfigSec, "heat_spreader_height");    // height
  heat_spreader_width_     = SescConf->getDouble(spreadersinkconfigSec, "heat_spreader_width");     // width
  heat_spreader_thickness_ = SescConf->getDouble(spreadersinkconfigSec, "heat_spreader_thickness"); // thickness

  heat_sink_fins_thicknsss_ = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_fins_thickness"); // thickness

  heat_spreader_microhardness_ =
      SescConf->getDouble(spreadersinkconfigSec, "heat_spreader_microhardness") * pow(10.0, 6.0); // microhardness
  heat_spreader_surfaceroughness_ =
      SescConf->getDouble(spreadersinkconfigSec, "heat_spreader_surfaceroughness") * pow(10.0, -6.0); // surface roughness

  heat_sink_fin_number_ = SescConf->getInt(spreadersinkconfigSec, "heat_sink_fins");
  heat_sink_fin_number_ += heat_sink_fin_number_ - 1; // if we specify 5 fins, we have 9 fins regions
  // note: specify the value as -1 to have the model determine the interface resistance based upon material properties
  heat_sink_resistance_ = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_resistance") *
                          pow(10.0, -4.0); // total interface resistance (for .02mm epoxy, 0.2-0.9 (x10^4*m^2*K/W)

  heat_sink_microhardness_ =
      SescConf->getDouble(spreadersinkconfigSec, "heat_sink_microhardness") * pow(10.0, 6.0); // microhardness
  heat_sink_surfaceroughness_ =
      SescConf->getDouble(spreadersinkconfigSec, "heat_sink_surfaceroughness") * pow(10.0, -6.0); // surface roughness
  heat_sink_contactpressure_ = SescConf->getDouble(spreadersinkconfigSec, "heat_sink_contactpressure") *
                               pow(10.0, 6.0); // contact pressure (range of 0.07-0.17MPa --megapascals)

  // this would be a thermal compound placed between the heat spreader and the heatsink
  // Note: if the heatsink resistance is defined, these parameters are ignored
  // thermal conductivity (W/mK)
  interface_material_conductivity_ = SescConf->getDouble(spreadersinkconfigSec, "interface_material_conductivity");
  // gas parameter (M0 x 10^6,m)
  interface_material_gasparameter_ = SescConf->getDouble(spreadersinkconfigSec, "interface_material_gasparameter");

  // GET THE MATERIAL PROPERTIES
  min = SescConf->getRecordMin(thermSec, "materials_bulk");
  max = SescConf->getRecordMax(thermSec, "materials_bulk");

  for(int id = min; id <= max; id++) {
    const char *name = SescConf->getCharPtr(thermSec, "materials_bulk", id);
    name_str         = name;
    datalibrary_->layer_materials_.create(name_str, SescConf->getDouble(name, "density"), SescConf->getDouble(name, "specHeat"),
                                          SescConf->getDouble(name, "conductivity"), SescConf->getDouble(name, "emissivity"));
  }

  // GET THE MODEL LAYERS
  min = SescConf->getRecordMin(thermSec, "layer");
  max = SescConf->getRecordMax(thermSec, "layer");

  for(int id = min; id <= max; id++) {
    const char *name        = SescConf->getCharPtr(thermSec, "layer", id);
    const char *layername   = SescConf->getCharPtr(name, "name");
    const char *type        = SescConf->getCharPtr(name, "type");
    MATRIX_DATA thickness   = SescConf->getDouble(name, "thickness");
    MATRIX_DATA width       = SescConf->getDouble(name, "width");
    MATRIX_DATA height      = SescConf->getDouble(name, "height");
    MATRIX_DATA granularity = SescConf->getDouble(name, "granularity");
    MATRIX_DATA lock_temp   = SescConf->getDouble(name, "lock_temp");
    int         flp_num     = SescConf->getInt(name, "floorplan");

    ChipLayers *info = new ChipLayers(datalibrary_, max - min);
    datalibrary_->all_layers_info_.push_back(info);

    info->layer_number_ = id - min;
    info->name_         = layername;
    info->thickness_    = thickness;
    info->width_        = width;
    info->height_       = height;
    info->granularity_  = granularity;
    info->flp_num_      = flp_num;
    if(lock_temp < 0) {
      info->temp_locking_enabled_ = false;
    } else {
      info->temp_locking_enabled_ = true;
      info->lock_temp_            = lock_temp;
    }

    // need to determine lumped spec_heat/conductivity and density for each layer

    info->spec_heat_                  = -1.0;
    info->vertical_conductivity_up_   = -1.0;
    info->vertical_conductivity_down_ = -1.0;
    info->horizontal_conductivity_    = -1.0;
    info->density_                    = -1.0;
    if(!strcmp(type, "mainboard")) {
      info->type_ = MAINBOARD_LAYER;
    } else if(!strcmp(type, "pins")) {
      info->type_ = PINS_LAYER;
    } else if(!strcmp(type, "package_pwb")) {
      info->type_ = PWB_LAYER;
    } else if(!strcmp(type, "package_substrate_c5")) {
      info->type_ = FCPBGA_LAYER;
    } else if(!strcmp(type, "c4_underfill")) {
      info->type_ = C4_UNDERFILL_LAYER;
    } else if(!strcmp(type, "interconnect")) {
      info->type_ = INTERCONNECT_LAYER;
    } else if(!strcmp(type, "die_transistor")) {
      info->type_ = DIE_TRANSISTOR_LAYER;
    } else if(!strcmp(type, "bulk_silicon")) {
      info->type_ = BULK_SI_LAYER;
    } else if(!strcmp(type, "oil")) {
      info->type_ = OIL_LAYER;
    } else if(!strcmp(type, "air")) {
      info->type_ = AIR_LAYER;
    } else if(!strcmp(type, "ucool")) {
      I(0);
      // info->type_=UCOOL_LAYER;
    } else if(!strcmp(type, "heat_spreader")) {
      info->type_ = HEAT_SPREADER_LAYER;
    } else if(!strcmp(type, "heat_sink_bottom")) {
      info->type_ = HEAT_SINK_LAYER;
    } else if(!strcmp(type, "heat_sink_fins")) {
      info->type_ = HEAT_SINK_FINS_LAYER;
    } else {
      std::cerr << "FATAL: ConfigData::get_config_data() -- undefined layer type [" << type << "] defined in configuration file"
                << std::endl;
      exit(1);
    }
  }

  config_data_defined_ = true;

  /****************
  //if the heat_sink_resistance is a negative number, this means that we are to determine the value
  if (LT(heat_sink_resistance_,0)) {

  MATRIX_DATA heat_sink_conductivity=layer_material_[3]->conductivity_;
  MATRIX_DATA heat_spreader_conductivity=layer_material_[2]->conductivity_;
  MATRIX_DATA heat_sink_surface_roughness=heat_sink_surfaceroughness_;
  MATRIX_DATA heat_spreader_surface_roughness=heat_spreader_surfaceroughness_;
  MATRIX_DATA
overall_conductivity=(2*heat_spreader_conductivity*heat_sink_conductivity)/(heat_spreader_conductivity+heat_sink_conductivity);
//ks=2k1k2/(k1+k2) MATRIX_DATA RMS_surface_roughness=sqrt(pow(heat_sink_surface_roughness,2) +
pow(heat_spreader_surface_roughness,2)); //sqrt(roughness1^2+roughness2^2) MATRIX_DATA
asperity_slope_1=0.125*pow(heat_sink_surface_roughness*pow(10.0,6.0),0.402); //m=0.125(roughnessx16^6)^0.402 MATRIX_DATA
asperity_slope_2=0.125*pow(heat_spreader_surface_roughness*pow(10.0,6.0),0.402); //m=0.125(roughnessx16^6)^0.402 MATRIX_DATA
eff_mean_abs_asp_slope=sqrt(pow(asperity_slope_1,2)+pow(asperity_slope_2,2)); //m=sqrt(m1^2+m2^2) MATRIX_DATA
min_microhardness=MIN(heat_sink_microhardness_,heat_spreader_microhardness_); MATRIX_DATA
contact_pressure=heat_sink_contactpressure_;

  MATRIX_DATA
contact_conductivity=1.25*overall_conductivity*(eff_mean_abs_asp_slope/RMS_surface_roughness)*pow(contact_pressure/min_microhardness,0.95);
//hc=1.25Ks(m/RMSroughness)(P/Hc)^0.95

  MATRIX_DATA eff_gap_thickness=1.53*RMS_surface_roughness*pow(contact_pressure/min_microhardness,-0.097);
//Y=1.536*RMS_roughness(pressure/microhardness)^-0.097 MATRIX_DATA
gap_conductivity=interface_material_conductivity_/(eff_gap_thickness+0); //Hg=Kg/(Y+M)
  //FIXME: currently does not support gas parameter (M=0)
  //this depends upon temperature which involves a rewrite of the model unit calculation methods
  MATRIX_DATA joint_conductivity=contact_conductivity+gap_conductivity;
  heat_sink_resistance_=1/joint_conductance;
#ifdef _ESESCTHERM_DEBUG

  cerr << "heat_sink_conductivity:" << heat_sink_conductivity << endl;
  cerr << "heat_spreader_conductivity:" << heat_spreader_conductivity << endl;
  cerr << "heat_sink_surface_roughness:" << heat_sink_surface_roughness << endl;
  cerr << "overall_conductance:" << overall_conductance << endl;
  cerr << "RMS_surface_roughness:" << RMS_surface_roughness << endl;
  cerr << "asperity_slope_1:" << asperity_slope_1 << endl;
  cerr << "asperity_slope_2:" << asperity_slope_2 << endl;
  cerr << "eff_mean_abs_asp_slope:" << eff_mean_abs_asp_slope << endl;
  cerr << "min_microhardness:" << min_microhardness << endl;
  cerr << "contact_pressure:" << contact_pressure << endl;
  cerr << "contact_conductance:" << contact_conductance << endl;
  cerr << "eff_gap_thickness:" << eff_gap_thickness << endl;
  cerr << "gap_conductance:" << gap_conductance << endl;
  cerr << "joint_conductance:" << joint_conductance << endl;

  cerr << "heat_sink_resistance:" << heat_sink_resistance_ << endl;
  }
#endif
   ***************/
}
