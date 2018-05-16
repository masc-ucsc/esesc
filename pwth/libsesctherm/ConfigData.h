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
File name:      ConfigData.h
Classes:        ConfigData
********************************************************************************/

#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <vector>

#include "ChipMaterial.h"

class ConfigData;
class ThermGraphics_FileType;

class ConfigData {
public:
  ConfigData(DataLibrary *datalibrary_);
  void                 get_config_data();
  bool                 config_data_defined_;
  friend std::ostream &operator<<(std::ostream &os, ConfigData &configuration);
  // materials
  Material_List material_list;
  // std::vector <Material*> layer_material_;
  //   Material* layer1_material_;
  //   Material* layer2_material_;
  //   Material* layer3_material_;
  //   Material* layer4_material_;
  //   Material* layer5_material_;

  //   layer thicknesses
  // std::vector <MATRIX_DATA> layer_thickness_;
  //   MATRIX_DATA layer1_thickness_;
  //   MATRIX_DATA layer2_thickness_;
  //	 MATRIX_DATA layer3_thickness_;
  //   MATRIX_DATA layer4_thickness_;
  //   MATRIX_DATA layer5_thickness_;

  // GET BASIC PROPERTIES
  MATRIX_DATA init_temp_;
  MATRIX_DATA ambient_temp_;
  MATRIX_DATA default_time_increment_;

  // GET COOLING PROPERTIES
  bool        fan_used_;
  MATRIX_DATA fan_velocity_;
  MATRIX_DATA air_pressure_;
  MATRIX_DATA fan_distance_;

  bool        oil_used_;
  MATRIX_DATA oil_layer_convection_coefficient_;
  MATRIX_DATA coolant_density_;
  MATRIX_DATA coolant_viscosity_;
  MATRIX_DATA coolant_thermal_conductivity_;
  MATRIX_DATA coolant_specific_heat_;
  MATRIX_DATA coolant_prandtl_number_;
  MATRIX_DATA coolant_flow_rate_;
  MATRIX_DATA coolant_angle_;
  MATRIX_DATA coolant_nozzle_diameter_;
  MATRIX_DATA coolant_coverage_percent_;

  // GET THE CHIP PARAMETERS

  MATRIX_DATA transistor_count_;
  MATRIX_DATA pin_count_;
  MATRIX_DATA pins_height_;
  MATRIX_DATA pin_pitch_;
  MATRIX_DATA chip_width_;
  MATRIX_DATA chip_height_;
  MATRIX_DATA chip_thickness_;
  MATRIX_DATA package_height_;
  MATRIX_DATA package_width_;
  MATRIX_DATA package_thickness_;

  // GET THE HEAT SINK/SPREADER PROPERTIES

  MATRIX_DATA heat_spreader_height_;
  MATRIX_DATA heat_spreader_width_;
  MATRIX_DATA heat_spreader_thickness_;

  MATRIX_DATA heat_sink_height_;
  MATRIX_DATA heat_sink_width_;
  MATRIX_DATA heat_sink_thickness_;

  MATRIX_DATA heat_sink_fins_thickness_;

  MATRIX_DATA heat_spreader_microhardness_;
  MATRIX_DATA heat_spreader_surfaceroughness_;

  int         heat_sink_fin_number_;
  MATRIX_DATA heat_sink_fins_thicknsss_;

  MATRIX_DATA heat_sink_resistance_;

  MATRIX_DATA heat_sink_microhardness_;
  MATRIX_DATA heat_sink_surfaceroughness_;
  MATRIX_DATA heat_sink_contactpressure_;

  // GET THE UCOOL PARAMETERS

  MATRIX_DATA ucool_width_;
  MATRIX_DATA ucool_height_;

  MATRIX_DATA ucool_current_;
  int         ucool_coupled_devices_;

  MATRIX_DATA ucool_resistivity_;
  MATRIX_DATA ucool_conductivity_; // FIXME: not initialized
  MATRIX_DATA ucool_seebeck_;
  MATRIX_DATA ucool_g_;

  // interface material properties
  MATRIX_DATA interface_material_conductivity_;
  MATRIX_DATA interface_material_gasparameter_;

  MATRIX_DATA hot_spot_temp_;
  // FIXME: ucooler on temperature
  // FIXME: ucooler off temperatre

  bool get_chipflp_from_sescconf_;
  bool get_ucoolflp_from_sescconf_;
  bool generate_ucool_floorplan_;

  MATRIX_DATA  gen_flp_ucool_pwr_percentage_;
  MATRIX_DATA  max_total_power_;
  MATRIX_DATA  gen_flp_pwr_per_cooler_;
  bool         gen_flp_calc_num_coolers_;
  int          n_coolers_;
  int          ucool_generation_policy_;
  MATRIX_DATA  ucool_weighted_region_size_;
  MATRIX_DATA  ucool_weighted_region_max_temp_;
  MATRIX_DATA  ucool_weighted_region_min_temp_;
  MATRIX_DATA  ucool_on_off_region_size_;
  int          num_pins_;
  int          pin_width_;
  int          pin_height_;
  DataLibrary *datalibrary_;

  // regression parameters

  // these are technology parameters
  int         technology_; // TECH_65, TECH_90, TECH_180, TECH_250
  MATRIX_DATA num_gates_;  // number of gates in design (defaults to 250M)

  std::map<MATRIX_DATA, std::map<int, MATRIX_DATA>> inter_layer_conductivities_;
  // interconnect parameters (for metal layers)
  // stochastic wire model parameters:
  //	MATRIX_DATA wire_stochastic_p_;
  //	MATRIX_DATA wire_stochastic_k_;
  //	MATRIX_DATA wire_stochastic_fanout_;
  //	int num_metal_layers_;		//number of metal layers (default 12)

  // parameters for calculating polysilicon thermal conductivity
  //	MATRIX_DATA polysilicon_grain_size_;
  //	MATRIX_DATA polysilicon_impurity_concentration_;

  // These parameters were taken from the following paper:
  // "Prediction of Thermal Performance of Flip Chip -- Plastic Ball Grid Array (FC_PBGA) Packages:
  //		Effect of Substrate Physical Design"
  //	K. Ramakrishna, T-Y. Tom Lee

  // FC-PBGA package substrate with C5 layer

  // FC-PBGA layer thicknesses
  // TopSM,M1, BT1, M2, BT2, M3, BT Core, M4, BT4, M5, BT5, M6, BottomSM, EQ C5 Layer
  MATRIX_DATA              fcpbga_layer_total_thickness_;
  std::vector<MATRIX_DATA> fcpbga_layer_thickness_;
  std::vector<MATRIX_DATA> fcpbga_layer_conductivity_vert_;
  std::vector<MATRIX_DATA> fcpbga_layer_conductivity_horiz_;
  std::vector<MATRIX_DATA> fcpbga_layer_specific_heat_;
  std::vector<MATRIX_DATA> fcpbga_layer_density_;

  // PWB parameters
  // TopSM, M1, FR4 dielectric, M2, FR4, M3, FR4, M3, FR4, BottomSM
  MATRIX_DATA              pwb_layer_total_thickness_;
  std::vector<MATRIX_DATA> pwb_layer_thickness_;
  std::vector<MATRIX_DATA> pwb_layer_conductivity_vert_;
  std::vector<MATRIX_DATA> pwb_layer_conductivity_horiz_;
  std::vector<MATRIX_DATA> pwb_layer_specific_heat_;
  std::vector<MATRIX_DATA> pwb_layer_density_;

  // Sample Transistor Parameters
  MATRIX_DATA sample_oxide_thickness_;
  MATRIX_DATA sample_gate_length_;
  MATRIX_DATA sample_fox_length_;
  MATRIX_DATA sample_eff_channel_length_;
  MATRIX_DATA sample_contact_length_;
  MATRIX_DATA sample_silicon_substrate_thickness_;
  MATRIX_DATA sample_silicon_film_length_;
  MATRIX_DATA sample_silicon_film_thickness_;
  MATRIX_DATA sample_box_thickness_;
  MATRIX_DATA sample_vthreshold_;
  MATRIX_DATA sample_oxide_conductivity_;
  MATRIX_DATA sample_oxide_conductivity_transistor_;
  MATRIX_DATA sample_silicon_island_conductivity_;
  MATRIX_DATA sample_gate_width_;
  MATRIX_DATA sample_thermal_capacity_silicon_;
  MATRIX_DATA sample_thermal_capacity_oxide_;

  // Graphics parameters
  bool enable_graphics_;
  int  resolution_x_;
  int  resolution_y_;
  bool enable_perspective_view_;
  int  graphics_floorplan_layer_;
  // this holds all the parameters for file types:
  std::vector<ThermGraphics_FileType *> graphics_file_types_;

  friend std::ostream &operator<<(std::ostream &os, const ConfigData &configuration) {
    os << "************ CONFIGURATION_DATA ************" << std::endl;
    os << "Configuration Data Defined?:" << ((configuration.config_data_defined_ == true) ? "yes!" : "no") << std::endl;

    // materials
    //    for (unsigned int i=0; i<configuration.materials_.size(); i++)
    //        os << configuration.materials_[i] << std::endl;

    // Heat Spreader Dimensions
    os << "heat_spreader_height_ =" << configuration.heat_spreader_height_ << std::endl;
    os << "heat_spreader_width_ =" << configuration.heat_spreader_width_ << std::endl;
    // Heat Sink Dimensions
    os << "heat_sink_height_ =" << configuration.heat_sink_height_ << std::endl;
    os << "heat_sink_width_ =" << configuration.heat_sink_width_ << std::endl;
    // Number of Heat Sink Fins
    os << "heat_sink_fin_number_ =" << configuration.heat_sink_fin_number_ << std::endl;
    // Heat Sink Interface Resistance;
    os << "heat_sink_resistance_ =" << configuration.heat_sink_resistance_ << std::endl;
    // initial temp
    os << "init_temp_ =" << configuration.init_temp_ << std::endl;
    // time increment
    os << "default_time_increment_ =" << configuration.default_time_increment_ << std::endl;
    // fan settings
    os << "fan_velocity_ =" << configuration.fan_velocity_ << std::endl;
    // ambient temp
    os << "ambient_temp_ =" << configuration.ambient_temp_ << std::endl;
    // micro-cooler settings
    os << "ucool_width_ =" << configuration.ucool_width_ << std::endl;
    os << "ucool_height_ =" << configuration.ucool_height_ << std::endl;
    os << "ucool_current_ =" << configuration.ucool_current_ << std::endl;
    os << "ucool_coupled_devices_ =" << configuration.ucool_coupled_devices_ << std::endl;
    os << "ucool_conductivity_ =" << configuration.ucool_conductivity_ << std::endl;
    os << "ucool_resistivity_ =" << configuration.ucool_resistivity_ << std::endl;
    os << "ucool_seebeck_ =" << configuration.ucool_seebeck_ << std::endl;
    os << "ucool_g_ =" << configuration.ucool_g_ << std::endl;
    return (os);
  }
};
#endif
