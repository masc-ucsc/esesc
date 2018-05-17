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
File name:      ThermGraphics.cpp
Description:    This is a graphics subsystem to output svg files with either a
                temperature map or power map overlayed against the chip floorplan
                for the particular layer specified in the input configuration file.
********************************************************************************/

#include <complex>
#include <fstream>
#include <iostream>
#include <map>
#include <strings.h>

#include "sescthermMacro.h"

#include "sesctherm3Ddefine.h"

#include "ChipFloorplan.h"
#include "ChipLayers.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "ThermGraphics.h"
#include "Utilities.h"

#include "Report.h"

#include "nanassert.h"

ThermGraphics_Color::ThermGraphics_Color(int r, int g, int b, int steps) {
  r_     = r;
  g_     = g;
  b_     = b;
  steps_ = steps;
}

void ThermGraphics_Color::create_palette(std::vector<ThermGraphics_Color> &palette, std::vector<ThermGraphics_Color> &colors) {
  if(!palette.empty())
    palette.clear();
  for(unsigned int i = 0; i < colors.size() - 1; i++) {
    ThermGraphics_Color::gradient(palette, colors[i], colors[i + 1], colors[i].steps_);
  }
}

void ThermGraphics_Color::gradient(std::vector<ThermGraphics_Color> &palette, ThermGraphics_Color &color1,
                                   ThermGraphics_Color &color2, int steps) {
  int step_r = (int)((color1.r_ - color2.r_) / (steps - 1));
  int step_g = (int)((color1.g_ - color2.g_) / (steps - 1));
  int step_b = (int)((color1.b_ - color2.b_) / (steps - 1));
  for(int i = 0; i < steps; i++) {
    ThermGraphics_Color newcolor((int)round(color1.r_ - (step_r * i)), (int)round(color1.g_ - (step_g * i)),
                                 (int)round(color1.b_ - (step_b * i)), 0);
    palette.push_back(newcolor);
  }
}

ThermGraphics_FileType::ThermGraphics_FileType(std::string config_string) {
  std::vector<std::string> tokens, tokens2;

  layer_computation_ = -1;
  unit_type_         = -1;
  sample_type_       = -1;
  data_type_         = -1;

  Utilities::Tokenize(config_string, tokens, "_, "); // tokenize by underscore and space and comma

  /*cerr <<  "printing tokens:" << std::endl;
    for (unsigned int i=0;i<tokens.size();i++){
    std::cerr <<  tokens[i] << ",";
    }
    std::cerr << std::endl;
   */

  Utilities::Tokenize(tokens[4], tokens2, "[]-");
  min_val_ = Utilities::convertToDouble(tokens2[0]);
  max_val_ = Utilities::convertToDouble(tokens2[1]);

  for(unsigned int i = 5; i < tokens.size(); i++)
    layers_.push_back(Utilities::convertToInt(tokens[i]));

  if(!strcasecmp(tokens[0].c_str(), "FLOORPLAN")) {
    layer_computation_ = GFX_LAYER_FLOORPLAN;
    return;
  }

  const char *layer_computation = tokens[0].c_str();
  const char *data_type         = tokens[1].c_str();
  const char *sample_type       = tokens[2].c_str();
  const char *unit_type         = tokens[3].c_str();

  // GFX_LAYER_NORMAL/GFX_LAYER_AVE/GFX_LAYER_DIF
  if(!strcasecmp(layer_computation, "NORM"))
    layer_computation_ = GFX_LAYER_NORMAL;
  else if(!strcasecmp(layer_computation, "AVE"))
    layer_computation_ = GFX_LAYER_AVE;
  else if(!strcasecmp(layer_computation, "DIF"))
    layer_computation_ = GFX_LAYER_DIF;
  else {
    std::cerr << "FATAL: graphics file type [" << config_string << "] specified in config file is invalid" << std::endl;
    exit(1);
  }

  // GFX_POWER/GFX_TEMP
  if(!strcasecmp(data_type, "POWER"))
    data_type_ = GFX_POWER;
  else if(!strcasecmp(data_type, "TEMP"))
    data_type_ = GFX_TEMP;
  else {
    std::cerr << "FATAL: graphics file type [" << config_string << "] specified in config file is invalid" << std::endl;
    exit(1);
  }

  if(!strcasecmp(sample_type, "CUR"))
    sample_type_ = GFX_CUR;
  else if(!strcasecmp(sample_type, "MAX"))
    sample_type_ = GFX_MAX;
  else if(!strcasecmp(sample_type, "MIN"))
    sample_type_ = GFX_MIN;
  else if(!strcasecmp(sample_type, "AVE"))
    sample_type_ = GFX_AVE;
  else {
    std::cerr << "FATAL: graphics file type [" << config_string << "] specified in config file is invalid" << std::endl;
    exit(1);
  }

  if(!strcasecmp(unit_type, "M"))
    unit_type_ = GFX_MUNIT;
  else if(!strcasecmp(unit_type, "F"))
    unit_type_ = GFX_FUNIT;
  else {
    std::cerr << "FATAL: graphics file type [" << config_string << "] specified in config file is invalid" << std::endl;
    exit(1);
  }
}

// output a svg file based on floorplan
ThermGraphics::ThermGraphics(DataLibrary *datalibrary) {
  datalibrary_        = datalibrary;
  num_levels_         = 260;           // number of colors used
  max_rotate_         = 200;           // maximum hue rotation
  stroke_coeff_       = pow(10.0, -7); // used to tune the stroke-width
  stroke_opacity_     = 0;             // used to control the opacity of the floor plan
  smallest_shown_     = 10000;         // fraction of the entire chip necessary to see macro
  zoom_               = pow(10.0, 6);
  txt_offset_         = 100;
  floorplans_printed_ = false;
  ThermGraphics_Color color1(255, 0, 0, 120);  // blue-green, 12 steps
  ThermGraphics_Color color2(255, 255, 0, 50); // green-yellow, 5 steps
  ThermGraphics_Color color3(0, 255, 255, 90); // yellow-red, 9 steps
  ThermGraphics_Color color4(0, 0, 255, 0);    //
  colors_.push_back(color1);
  colors_.push_back(color2);
  colors_.push_back(color3);
  colors_.push_back(color4);
  ThermGraphics_Color::create_palette(palette_, colors_); // store the palette based upon the colors
}

void ThermGraphics::print_floorplans() {
  if(floorplans_printed_ == true)
    return;

  for(unsigned int i = 0; i < datalibrary_->config_data_->graphics_file_types_.size(); i++) {
    // if we haven't printed out the floorplan files yet, then print them
    if(datalibrary_->config_data_->graphics_file_types_[i]->layer_computation_ == GFX_LAYER_FLOORPLAN) {
#ifdef _ESESCTHERM_DEBUG
      //                              std::cerr << datalibrary_->config_data_->graphics_file_types_[i] << std::endl;
#endif
      for(unsigned int j = 0; j < datalibrary_->config_data_->graphics_file_types_[i]->layers_.size(); j++) {
        int           layer_num = datalibrary_->config_data_->graphics_file_types_[i]->layers_[j];
        std::ofstream of_graphics_outfile;
        create_open_graphics_file(0, 0, 0, GFX_LAYER_FLOORPLAN, datalibrary_->config_data_->graphics_file_types_[i]->layers_,
                                  layer_num, of_graphics_outfile);

        // print a floorplan file for layer i
        flp2svg_simple(datalibrary_->all_layers_info_[layer_num]->chip_floorplan_->flp_units_, of_graphics_outfile);
        of_graphics_outfile.close();
      }
    }
  }
  floorplans_printed_ = true;
}

void ThermGraphics::print_graphics_helper(ThermGraphics_FileType &graphics_file_type, std::vector<MATRIX_DATA> &temperature_map,
                                          std::vector<MATRIX_DATA> &power_map, DynamicArray<ModelUnit> &layer_dyn, int layer) {
#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << graphics_file_type << std::endl;
#endif
  int         unit_type                = graphics_file_type.unit_type_;
  int         data_type                = graphics_file_type.data_type_;
  int         sample_type              = graphics_file_type.sample_type_;
  int         layer_computation        = graphics_file_type.layer_computation_;
  MATRIX_DATA min_data_val             = graphics_file_type.min_val_;
  MATRIX_DATA max_data_val             = graphics_file_type.max_val_;
  int         graphics_floorplan_layer = datalibrary_->config_data_->graphics_floorplan_layer_;
  if(graphics_floorplan_layer < 0 || (unsigned int)graphics_floorplan_layer >= datalibrary_->all_layers_info_.size() ||
     datalibrary_->all_layers_info_[graphics_floorplan_layer]->chip_floorplan_ == NULL) {
    std::cerr << "ThermGraphics::print_graphics_helper => graphics_floorplan_layer is invalid in configuration file (no chip "
                 "floorplan has been defined for layer "
              << graphics_floorplan_layer << ")" << std::endl;
    exit(1);
  }

  std::vector<int> &               layers = graphics_file_type.layers_;
  std::vector<ChipFloorplan_Unit> &flp_units =
      datalibrary_->all_layers_info_[graphics_floorplan_layer]->chip_floorplan_->flp_units_;

  // create and open graphics file
  std::ofstream of_graphics_outfile;
  create_open_graphics_file(data_type, unit_type, sample_type, layer_computation, layers, layer, of_graphics_outfile);

  if(unit_type == GFX_FUNIT) {
    // print the colorized floorplan
    if(data_type == GFX_TEMP) {
      temperature_map = ModelUnit::compute_average_temps(graphics_floorplan_layer, layer_dyn, sample_type, datalibrary_);
      data_flp2svg(flp_units, temperature_map, min_data_val, max_data_val, of_graphics_outfile);
    } else {
      power_map = ModelUnit::compute_average_powers(graphics_floorplan_layer, layer_dyn, sample_type, datalibrary_);
      data_flp2svg(flp_units, power_map, min_data_val, max_data_val, of_graphics_outfile);
    }
  }
  // otherwise we print out the layer by model-unit
  else {
    // we print out the data specified by data_type that is stored in the layer_dyn
    data_model2svg(flp_units, layer_dyn, data_type, sample_type, min_data_val, max_data_val, of_graphics_outfile);
  }
  of_graphics_outfile.close();
}

// it is assumed that print_graphics will be called at the end of a given sampling period if
// the average/max/min values are desired. Otherwise, it will get old values.
void ThermGraphics::print_graphics() {
  std::vector<ThermGraphics_FileType *> &graphics_file_types      = datalibrary_->config_data_->graphics_file_types_;
  int                                    graphics_floorplan_layer = datalibrary_->config_data_->graphics_floorplan_layer_;

  if(graphics_floorplan_layer < 0 || (unsigned int)graphics_floorplan_layer >= datalibrary_->all_layers_info_.size() ||
     datalibrary_->all_layers_info_[graphics_floorplan_layer]->chip_floorplan_ == NULL) {
    std::cerr << "ThermGraphics::print_graphics => graphics_floorplan_layer is invalid in configuration file (no chip floorplan "
                 "has been defined for layer "
              << graphics_floorplan_layer << ")" << std::endl;
    exit(1);
  }

  DynamicArray<ModelUnit>  graphics_layer_dyn(datalibrary_); // this will hold all of the relevent data for output
  std::vector<MATRIX_DATA> power_map;
  std::vector<MATRIX_DATA> temperature_map;
  int                      layer_num = 0;

  print_floorplans();

  for(unsigned int i = 0; i < graphics_file_types.size(); i++) {
    if(graphics_file_types[i]->layer_computation_ == GFX_LAYER_FLOORPLAN)
      continue; // we have already printed the floorplan-type files, no need to do it again

    graphics_layer_dyn.clear();
    if(!temperature_map.empty())
      temperature_map.clear();
    if(!power_map.empty())
      power_map.clear();

    switch(graphics_file_types[i]->layer_computation_) {
    case GFX_LAYER_NORMAL:
      // this means that we print out one graphics file per layer specified
      for(unsigned int j = 0; j < graphics_file_types[i]->layers_.size(); j++) {
        int layer_num = graphics_file_types[i]->layers_[j];
        if(layer_num < 0 || (unsigned int)layer_num >= datalibrary_->all_layers_info_.size()) {
          std::cerr << "Attempting to print graphics for invalid layer: " << layer_num << std::endl;
          exit(1);
        }
        for(unsigned int itor_x = 0; itor_x < datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_->ncols(); itor_x++) {
          for(unsigned int itor_y = 0; itor_y < datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_->nrows();
              itor_y++) {
            if((*datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_)[itor_x][itor_y].defined_ == false)
              (*datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_)[itor_x][itor_y].sample_temperature_ = 0;
            else
              (*datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_)[itor_x][itor_y].sample_temperature_ =
                  *(*datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_)[itor_x][itor_y].get_temperature() - 273.15;
          }
        }
        // pass the floorplan_layer_dyn to the graphics helper
        print_graphics_helper(*graphics_file_types[i], temperature_map, power_map,
                              *datalibrary_->all_layers_info_[layer_num]->floorplan_layer_dyn_, layer_num);
      }
      break;
    case GFX_LAYER_AVE:
      // this means that we average the data across all the layers for the particular data_type
      graphics_layer_dyn = ChipLayers::compute_layer_averages(graphics_file_types[i]->layers_, datalibrary_);
      layer_num          = 0;
      print_graphics_helper(*graphics_file_types[i], temperature_map, power_map, graphics_layer_dyn, layer_num);
      break;
    case GFX_LAYER_DIF:
      layer_num = 0;
      graphics_layer_dyn =
          ChipLayers::compute_layer_diff(graphics_file_types[i]->layers_[0], graphics_file_types[i]->layers_[1], datalibrary_);

      print_graphics_helper(*graphics_file_types[i], temperature_map, power_map, graphics_layer_dyn, layer_num);

      break;
    default:
      std::cerr << "ThermGraphics::printGraphics => graphics_file_types[i]->layer_computation_ is invalid" << std::endl;
      exit(1);
    }
  }
}

// print out a colorized version of the floorplan for a given dataset (temperature/power)
void ThermGraphics::data_flp2svg(std::vector<ChipFloorplan_Unit> &flp_units, std::vector<MATRIX_DATA> &data_map,
                                 MATRIX_DATA min_dval, MATRIX_DATA max_dval, std::ofstream &of_graphics_outfile) {
  MATRIX_DATA min_x = 0;
  MATRIX_DATA max_x = 0;
  MATRIX_DATA min_y = 0;
  MATRIX_DATA max_y = 0;
  // MATRIX_DATA x_bound = 0;

  calc_x_bound(flp_units, min_x, max_x, min_y, max_y);

#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << "DATA_FLP2SVG: minx:" <<  min_x << "maxx:" << max_x << "miny:" << min_y << "maxy:" << max_y << std::endl;
#endif
  // x_bound = max_x * 1.2;
  (of_graphics_outfile) << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20001102//EN\" " << std::endl;
  (of_graphics_outfile) << "\"http://www.w3.org/TR/2000/CR-SVG-20001102/DTD/svg-20001102.dtd\"> " << std::endl;
  (of_graphics_outfile) << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\""
                        << datalibrary_->config_data_->resolution_x_ << "\" height=\"" << datalibrary_->config_data_->resolution_y_
                        << "\" " << std::endl;

  if(datalibrary_->config_data_->enable_perspective_view_) {
    //      (of_graphics_outfile) << "\t viewBox=\"" << min_x*1.3 << " " << min_y*1.1 << " " <<
    //      .5*sqrtf(powf(max_x-minx_x,2.0)+powf(max_y-min_y,2.0)) << " " << (max_y-min_y)*1.5 << "\"> " << std::endl;
    (of_graphics_outfile) << "\t viewBox=\"" << min_x * .93 << " " << min_y * .98 << " "
                          << sqrtf(powf(max_x - min_x, 2.0) + powf(max_y - min_y, 2.0)) * 1.8 << " "
                          << sqrtf(powf(max_x - min_x, 2.0) + powf(max_y - min_y, 2.0)) * 1.6 << "\"> " << std::endl;
  } else {
    (of_graphics_outfile) << "\t viewBox=\"" << min_x * .98 << " " << min_y * .98 << " " << (max_x - min_x) * 1.5 << " "
                          << (max_y - min_y) * 1.5 << "\"> " << std::endl;
  }

  (of_graphics_outfile) << "<title>Sesctherm</title>" << std::endl;
  (of_graphics_outfile) << "<defs>" << std::endl;
  (of_graphics_outfile) << "<g id=\"ChipFloorplan_Unit_names\" style=\"stroke: none\">" << std::endl;

  // print the floorplan shared based upon the data_map
  flp2svg_simple(flp_units, of_graphics_outfile);
  (of_graphics_outfile) << "</g>";

  (of_graphics_outfile) << "<g id=\"floorplan\" style=\"stroke: none; fill: red\">" << std::endl;

  flp2svg(flp_units, data_map, of_graphics_outfile, min_dval, max_dval);
  (of_graphics_outfile) << "</g>";

  (of_graphics_outfile) << "<g id=\"thermal_scale\" style=\"stroke: none; fill: red\">" << std::endl;
  draw_color_scale_bar(max_dval, min_dval, max_x, max_y, min_x, max_y, of_graphics_outfile);

  (of_graphics_outfile) << "</g>";

  (of_graphics_outfile) << "</defs>";
  if(datalibrary_->config_data_->enable_perspective_view_) {
    (of_graphics_outfile) << "<use id=\"flp3\" xlink:href=\"#floorplan\" transform=\" translate(" << min_x << "," << min_y
                          << ") scale(1.5,0.9) skewX(35) rotate(-35) translate(" << -1.1 * min_x << "," << -1.000 * min_y << ")\"/>"
                          << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#ChipFloorplan_Unit_names\" transform=\" translate(" << min_x << "," << min_y
                          << ") scale(1.5,0.9) skewX(35) rotate(-35) translate(" << -1.1 * min_x << "," << -1.000 * min_y << ")\"/>"
                          << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#thermal_scale\" transform=\" translate(" << .11 * min_x << "," << 0 << ")\"/>"
                          << std::endl;
    (of_graphics_outfile) << "</svg>" << std::endl;
  } else {
    (of_graphics_outfile) << "<use id=\"flp3\" xlink:href=\"#floorplan\" />" << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#ChipFloorplan_Unit_names\" />" << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#thermal_scale\"/>" << std::endl;
    (of_graphics_outfile) << "</svg>" << std::endl;
  }
}

// print out a colorized version of the model /w floorplan overlay for a given dataset (temperature/power)
void ThermGraphics::data_model2svg(std::vector<ChipFloorplan_Unit> &flp_units, DynamicArray<ModelUnit> &layer_dyn, int data_type,
                                   int sample_type, MATRIX_DATA min_dval, MATRIX_DATA max_dval,
                                   std::ofstream &of_graphics_outfile) {
  MATRIX_DATA min_x = 0;
  MATRIX_DATA max_x = 0;
  MATRIX_DATA min_y = 0;
  MATRIX_DATA max_y = 0;
  // MATRIX_DATA x_bound = 0;

  calc_x_bound(layer_dyn, min_x, max_x, min_y, max_y);

#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << "DATA_MODEL2SVG: minx:" <<  min_x << "maxx:" << max_x << "miny:" << min_y << "maxy:" << max_y << std::endl;
#endif
  // x_bound = max_x * 1.2;

  (of_graphics_outfile) << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20001102//EN\" " << std::endl;
  (of_graphics_outfile) << "\"http://www.w3.org/TR/2000/CR-SVG-20001102/DTD/svg-20001102.dtd\"> " << std::endl;
  (of_graphics_outfile) << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\""
                        << datalibrary_->config_data_->resolution_x_ << "\" height=\"" << datalibrary_->config_data_->resolution_y_
                        << "\" " << std::endl;
  if(datalibrary_->config_data_->enable_perspective_view_) {
    //              (of_graphics_outfile) << "\t viewBox=\"" << min_x*1.3 << " " << min_y*1.1 << " " <<
    //              .5*sqrtf(powf(x_bound,2.0)+powf(max_y,2.0)) << " " << max_y << "\"> " << std::endl;
    (of_graphics_outfile) << "\t viewBox=\"" << min_x * .93 << " " << min_y * .98 << " "
                          << sqrtf(powf(max_x - min_x, 2.0) + powf(max_y - min_y, 2.0)) * 1.8 << " "
                          << sqrtf(powf(max_x - min_x, 2.0) + powf(max_y - min_y, 2.0)) * 1.6 << "\"> " << std::endl;
  } else {
    (of_graphics_outfile) << "\t viewBox=\"" << min_x * .98 << " " << min_y * .98 << " " << (max_x - min_x) * 1.5 << " "
                          << (max_y - min_y) * 1.5 << "\"> " << std::endl;
  }

  (of_graphics_outfile) << "<title>Sesctherm</title>" << std::endl;

  (of_graphics_outfile) << "<defs>" << std::endl;

  (of_graphics_outfile) << "<g id=\"ChipFloorplan_Unit_names\" style=\"stroke: none\">" << std::endl;

  flp2svg_simple(flp_units, of_graphics_outfile);

  (of_graphics_outfile) << "</g>";

  (of_graphics_outfile) << "<g id=\"floorplan\" style=\"stroke: none; fill: red\">" << std::endl;

  model2svg_helper(flp_units, layer_dyn, data_type, sample_type, of_graphics_outfile, min_dval, max_dval);

  (of_graphics_outfile) << "</g>";
  (of_graphics_outfile) << "<g id=\"thermal_scale\" style=\"stroke: none; fill: red\">" << std::endl;

  draw_color_scale_bar(max_dval, min_dval, max_x, max_y, min_x, min_y, of_graphics_outfile);
  (of_graphics_outfile) << "</g>";

  (of_graphics_outfile) << "</defs>";

  if(datalibrary_->config_data_->enable_perspective_view_) {
    (of_graphics_outfile) << "<use id=\"flp3\" xlink:href=\"#floorplan\" transform=\" translate(" << min_x << "," << min_y
                          << ") scale(1.5,0.9) skewX(35) rotate(-35) translate(" << -1.1 * min_x << "," << -1.000 * min_y << ")\"/>"
                          << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#ChipFloorplan_Unit_names\" transform=\" translate(" << min_x << "," << min_y
                          << ") scale(1.5,0.9) skewX(35) rotate(-35) translate(" << -1.1 * min_x << "," << -1.000 * min_y << ")\"/>"
                          << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#thermal_scale\" transform=\" translate(" << .11 * min_x << "," << 0 << ") \"/>"
                          << std::endl;
    (of_graphics_outfile) << "</svg>" << std::endl;
  } else {
    (of_graphics_outfile) << "<use id=\"flp3\" xlink:href=\"#floorplan\" />" << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#ChipFloorplan_Unit_names\" />" << std::endl;
    (of_graphics_outfile) << "<use xlink:href=\"#thermal_scale\"/>" << std::endl;
    (of_graphics_outfile) << "</svg>" << std::endl;
  }
}

void ThermGraphics::flp2svg(std::vector<ChipFloorplan_Unit> &flp_units, std::vector<MATRIX_DATA> data_map,
                            std::ofstream &of_graphics_outfile, MATRIX_DATA &max_dataval, MATRIX_DATA &min_dataval) {
  int         level           = 0;
  MATRIX_DATA current_dataval = 0;
  if(min_dataval == -1)
    min_dataval = pow(10.0, 10);
  // no need for this (it will be -1 anyway) max_dataval=-1;
  I(data_map.size() == flp_units.size()); // ensure that the data_map array is the same size as the flp_units array
  // get max data value, min data value
  for(unsigned int i = 0; i < data_map.size(); i++) {
    if(min_dataval == -1)
      min_dataval = MIN(min_dataval, data_map[i]);
    if(max_dataval == -1)
      max_dataval = MAX(max_dataval, data_map[i]);
  }

  for(unsigned int i = 0; i < data_map.size(); i++) {
    current_dataval = data_map[i];
    level           = (int)((max_dataval - current_dataval) / ((max_dataval - min_dataval) * (num_levels_ - 1)));

    (of_graphics_outfile) << "\t"
                          << "<rect x=\"" << flp_units[i].leftx_ * zoom_ << "\" y=\"" << flp_units[i].bottomy_ * zoom_ << "\"";
    (of_graphics_outfile) << " width=\"" << flp_units[i].width_ * zoom_ << "\"";
    (of_graphics_outfile) << " height=\"" << flp_units[i].height_ * zoom_ << "\"";
    (of_graphics_outfile) << " style=\"fill:rgb(" << Utilities::stringify(palette_[level].r_) << ","
                          << Utilities::stringify(palette_[level].g_) << "," << Utilities::stringify(palette_[level].b_) << ")\" />"
                          << std::endl;
  }
}

void ThermGraphics::flp2svg_simple(std::vector<ChipFloorplan_Unit> &flp_units, std::ofstream &of_graphics_outfile) {
  MATRIX_DATA min_x = pow(10.0, 10);
  MATRIX_DATA min_y = pow(10.0, 10);
  MATRIX_DATA max_x = 0;
  MATRIX_DATA max_y = 0;
  // MATRIX_DATA tot_x = 0;
  // MATRIX_DATA tot_y = 0;
  // MATRIX_DATA x_bound;

  calc_x_bound(flp_units, min_x, max_x, min_y, max_y);
#ifdef _ESESCTHERM_DEBUG
  //      std::cerr << "FLP2SVG: minx:" <<  min_x << "maxx:" << max_x << "miny:" << min_y << "maxy:" << max_y << std::endl;
#endif
  // tot_x = max_x - min_x;
  // tot_y = max_y - min_y;
  // x_bound = max_x * 1.2;

  for(unsigned int i = 0; i < flp_units.size(); i++) {
    MATRIX_DATA txt_start_x = flp_units[i].leftx_;
    MATRIX_DATA txt_start_y = flp_units[i].bottomy_ + flp_units[i].height_;

    // draw leftx,bottomy to rightx,bottomy
    (of_graphics_outfile) << "\t"
                          << "<line x1=\"" << flp_units[i].leftx_ * zoom_ << "\" y1=\"" << flp_units[i].bottomy_ * zoom_ << "\"";

    (of_graphics_outfile) << " x2=\"" << (flp_units[i].leftx_ + flp_units[i].width_) * zoom_ << "\" y2=\""
                          << flp_units[i].bottomy_ * zoom_ << "\"";
    (of_graphics_outfile) << " style=\"stroke:black;stroke-width:10\" />" << std::endl;

    // draw rightx,bottomy to rightx,topy
    (of_graphics_outfile) << " \t"
                          << "<line x1=\"" << (flp_units[i].leftx_ + flp_units[i].width_) * zoom_ << "\" y1=\""
                          << flp_units[i].bottomy_ * zoom_ << "\"";
    (of_graphics_outfile) << " x2=\"" << (flp_units[i].leftx_ + flp_units[i].width_) * zoom_ << "\" y2=\""
                          << (flp_units[i].bottomy_ + flp_units[i].height_) * zoom_ << "\"";
    (of_graphics_outfile) << " style=\"stroke:black;stroke-width:10\" />" << std::endl;

    // draw rightx,topy to leftx,topy
    (of_graphics_outfile) << "\t"
                          << "<line x1=\"" << (flp_units[i].leftx_ + flp_units[i].width_) * zoom_ << "\" y1=\""
                          << (flp_units[i].bottomy_ + flp_units[i].height_) * zoom_ << "\"";
    (of_graphics_outfile) << " x2=\"" << flp_units[i].leftx_ * zoom_ << "\" y2=\""
                          << (flp_units[i].bottomy_ + flp_units[i].height_) * zoom_ << "\"";
    (of_graphics_outfile) << " style=\"stroke:black;stroke-width:10\" />" << std::endl;

    // draw leftx,topy to leftx,bottomy
    (of_graphics_outfile) << "\t"
                          << "<line x1=\"" << flp_units[i].leftx_ * zoom_ << "\" y1=\""
                          << (flp_units[i].bottomy_ + flp_units[i].height_) * zoom_ << "\"";
    (of_graphics_outfile) << " x2=\"" << flp_units[i].leftx_ * zoom_ << "\" y2=\"" << flp_units[i].bottomy_ * zoom_ << "\"";
    (of_graphics_outfile) << " style=\"stroke:black;stroke-width:10\" />" << std::endl;

    (of_graphics_outfile) << "\t"
                          << "<text x=\"" << txt_start_x * zoom_ << "\" y=\"" << txt_start_y * zoom_ << "\"";
    (of_graphics_outfile) << " fill=\"black\" text_anchor=\"start\" font-size=\"" << (max_y - min_y) / 50
                          << "\" style=\"font-size:" << (max_y - min_y) / 50 << "\">";
    (of_graphics_outfile) << flp_units[i].name_ << "</text>" << std::endl;
  }
}

void ThermGraphics::model2svg_helper(std::vector<ChipFloorplan_Unit> &flp_units, DynamicArray<ModelUnit> &layer_dyn, int data_type,
                                     int sample_type, std::ofstream &of_graphics_outfile, MATRIX_DATA &min_dataval,
                                     MATRIX_DATA &max_dataval) {
  int         level           = 0;
  MATRIX_DATA current_dataval = 0;
  if(min_dataval == -1)
    min_dataval = pow(10.0, 20);
  // max_dataval=-1; //no need for this (it will be -1 anyway)

  // get max data value, min data value
  for(unsigned int y_itor = 0; y_itor < layer_dyn.nrows(); y_itor++) {
    for(unsigned int x_itor = 0; x_itor < layer_dyn.ncols(); x_itor++) {
      if(data_type == GFX_POWER) {
        switch(sample_type) {
        case GFX_CUR:
          current_dataval = layer_dyn[x_itor][y_itor].energy_data_;
          break;
        case GFX_MIN:
          current_dataval = layer_dyn[x_itor][y_itor].min_energy_data_;
          break;
        case GFX_MAX:
          current_dataval = layer_dyn[x_itor][y_itor].max_energy_data_;
          break;
        case GFX_AVE:
          current_dataval = layer_dyn[x_itor][y_itor].ave_energy_data_;
          break;
        default:
          break;
        }
      } else if(data_type == GFX_TEMP) {
        switch(sample_type) {
        case GFX_CUR:
          current_dataval = layer_dyn[x_itor][y_itor].sample_temperature_;
          break;
        case GFX_MIN:
          current_dataval = layer_dyn[x_itor][y_itor].min_temperature_;
          break;
        case GFX_MAX:
          current_dataval = layer_dyn[x_itor][y_itor].max_temperature_;
          break;
        case GFX_AVE:
          current_dataval = layer_dyn[x_itor][y_itor].ave_temperature_;
          break;
        default:
          break;
        }
      } else {
        std::cerr << "ThermGraphics::model2svg_helper() => invalid data type" << std::endl;
        exit(1);
      }
      if(min_dataval == -1)
        min_dataval = MIN(min_dataval, current_dataval);
      if(max_dataval == -1)
        max_dataval = MAX(max_dataval, current_dataval);
    }
  }

  unsigned int x_itor = 0;
  unsigned int y_itor = 0;

  for(y_itor = 0; y_itor < layer_dyn.nrows(); y_itor++) {
    for(x_itor = 0; x_itor < layer_dyn.ncols(); x_itor++) {
      if(data_type == GFX_POWER) {
        switch(sample_type) {
        case GFX_CUR:
          current_dataval = layer_dyn[x_itor][y_itor].energy_data_;
          break;
        case GFX_MIN:
          current_dataval = layer_dyn[x_itor][y_itor].min_energy_data_;
          break;
        case GFX_MAX:
          current_dataval = layer_dyn[x_itor][y_itor].max_energy_data_;
          break;
        case GFX_AVE:
          current_dataval = layer_dyn[x_itor][y_itor].ave_energy_data_;
          break;
        default:
          break;
        } // switch
      } else if(data_type == GFX_TEMP) {
        switch(sample_type) {
        case GFX_CUR:
          current_dataval = layer_dyn[x_itor][y_itor].sample_temperature_;
          break;
        case GFX_MIN:
          current_dataval = layer_dyn[x_itor][y_itor].min_temperature_;
          break;
        case GFX_MAX:
          current_dataval = layer_dyn[x_itor][y_itor].max_temperature_;
          break;
        case GFX_AVE:
          current_dataval = layer_dyn[x_itor][y_itor].ave_temperature_;
          break;
        default:
          break;
        }
      } else {
        std::cerr << "ThermGraphics::model2svg_helper() => invalid data type" << std::endl;
        exit(1);
      }
#ifdef _ESESCTHERM_DEBUG
      //                      std::cerr << current_dataval << "[" << x_itor << "][" << y_itor << "]" << ",";
#endif
      if(max_dataval < current_dataval || min_dataval > current_dataval) {
        if(current_dataval > max_dataval)
          current_dataval = max_dataval;
        else
          current_dataval = min_dataval;
      }
      level = (int)((max_dataval - current_dataval) / (max_dataval - min_dataval) * (num_levels_ - 1));
      (of_graphics_outfile) << "\t"
                            << "<rect x=\"" << layer_dyn[x_itor][y_itor].leftx_ * zoom_ << "\" y=\""
                            << layer_dyn[x_itor][y_itor].bottomy_ * zoom_ << "\"";
      (of_graphics_outfile) << " width=\"" << layer_dyn[x_itor][y_itor].width_ * zoom_ << "\"";
      (of_graphics_outfile) << " height=\"" << layer_dyn[x_itor][y_itor].height_ * zoom_ << "\"";
      (of_graphics_outfile) << " style=\"fill:rgb(" << Utilities::stringify(palette_[level].r_) << ","
                            << Utilities::stringify(palette_[level].g_) << "," << Utilities::stringify(palette_[level].b_)
                            << ")\" />" << std::endl;
    }
#ifdef _ESESCTHERM_DEBUG
    //              std::cerr << std::endl;
#endif
  }
}

void ThermGraphics::calc_x_bound(std::vector<ChipFloorplan_Unit> &ChipFloorplan_Units, MATRIX_DATA &min_x, MATRIX_DATA &max_x,
                                 MATRIX_DATA &min_y, MATRIX_DATA &max_y) {
  min_x = pow(10.0, 10);
  min_y = pow(10.0, 10);
  max_x = 0;
  max_y = 0;
  MATRIX_DATA in_minx, in_miny, in_maxx, in_maxy;

  // get in_minx, in_miny, in_maxx, in_maxy, tot_x, tot_y
  for(unsigned int i = 0; i < ChipFloorplan_Units.size(); i++) {
    in_minx = ChipFloorplan_Units[i].leftx_ * zoom_;
    in_miny = ChipFloorplan_Units[i].bottomy_ * zoom_;
    in_maxx = (ChipFloorplan_Units[i].leftx_ + ChipFloorplan_Units[i].width_) * zoom_;
    in_maxy = (ChipFloorplan_Units[i].bottomy_ + ChipFloorplan_Units[i].height_) * zoom_;
    min_x   = MIN(min_x, in_minx);
    min_y   = MIN(min_y, in_miny);
    max_x   = MAX(max_x, in_maxx);
    max_y   = MAX(max_y, in_maxy);
  }
}

void ThermGraphics::calc_x_bound(DynamicArray<ModelUnit> &dyn_layer, MATRIX_DATA &min_x, MATRIX_DATA &max_x, MATRIX_DATA &min_y,
                                 MATRIX_DATA &max_y) {
  min_x = pow(10.0, 10);
  min_y = pow(10.0, 10);
  max_x = 0;
  max_y = 0;
  MATRIX_DATA in_minx, in_miny, in_maxx, in_maxy;

  // get in_minx, in_miny, in_maxx, in_maxy, tot_x, tot_y
  for(unsigned int y_itor = 0; y_itor < dyn_layer.nrows(); y_itor++) {
    for(unsigned int x_itor = 0; x_itor < dyn_layer.ncols(); x_itor++) {
      if(!dyn_layer[x_itor][y_itor].defined_)
        continue;
      in_minx = dyn_layer[x_itor][y_itor].leftx_ * zoom_;
      in_miny = dyn_layer[x_itor][y_itor].bottomy_ * zoom_;
      in_maxx = (dyn_layer[x_itor][y_itor].leftx_ + dyn_layer[x_itor][y_itor].width_) * zoom_;
      in_maxy = (dyn_layer[x_itor][y_itor].bottomy_ + dyn_layer[x_itor][y_itor].height_) * zoom_;
      min_x   = MIN(min_x, in_minx);
      min_y   = MIN(min_y, in_miny);
      max_x   = MAX(max_x, in_maxx);
      max_y   = MAX(max_y, in_maxy);
    }
  }
}

void ThermGraphics::draw_color_scale_bar(MATRIX_DATA max_dataval, MATRIX_DATA min_dataval, MATRIX_DATA max_x, MATRIX_DATA max_y,
                                         MATRIX_DATA min_x, MATRIX_DATA min_y, std::ofstream &of_graphics_outfile) {
  MATRIX_DATA txt_ymin   = 0;
  MATRIX_DATA w2         = (max_x - min_x) * 0.05;
  MATRIX_DATA h2         = (max_y - min_y) / num_levels_;
  MATRIX_DATA clr_xmin   = max_x + (max_x - min_x) * .03;
  MATRIX_DATA clr_ymin   = min_y;
  MATRIX_DATA scale_xmin = max_x + (max_x - min_x) * .02;
  MATRIX_DATA scale_value;

  // we want 15 data points
  for(int i = 0; i < num_levels_; i++) {
    (of_graphics_outfile) << "\t"
                          << "<rect x=\"" << clr_xmin << "\" y=\"" << clr_ymin << "\" width=\"" << w2 << "\" height=\"" << h2
                          << "\"";
    (of_graphics_outfile) << " style=\"fill:rgb(" << Utilities::stringify(palette_[i].r_) << ","
                          << Utilities::stringify(palette_[i].g_) << "," << Utilities::stringify(palette_[i].b_)
                          << "); stroke:none\" />" << std::endl;
    if(i % int(num_levels_ / 10) == 0) {
      txt_ymin    = clr_ymin + h2 * 0.5;
      scale_value = (max_dataval - min_dataval) * (1 - i / (num_levels_ - 1)) + min_dataval;
      (of_graphics_outfile) << "\t"
                            << "<text x=\"" << scale_xmin << "\" y=\"" << txt_ymin
                            << "\" fill=\"black\" text_anchor=\"start\" font-size=\"" << (max_y - min_y) / 50
                            << "\" style=\"font-size:" << (max_y - min_y) / 50 << "\">";
      (of_graphics_outfile) << scale_value << "</text>" << std::endl;
    }
    clr_ymin += h2;
  }

  (of_graphics_outfile) << "\t"
                        << "<text x=\"" << scale_xmin << "\" y=\"" << clr_ymin
                        << "\" fill=\"black\" text_anchor=\"start\" font-size=\"" << (max_y - min_y) / 50
                        << "\" style=\"font-size:" << (max_y - min_y) / 50 << "\">";
  (of_graphics_outfile) << min_dataval << "</text>" << std::endl;
}

// prefix is a prefix to place before the timestamp
// file format: prefix.timestamp.svg
// example layer0.001s.svg
// info_type: CUR_TEMP
//                       MAX_TEMP
//                       AVERAGE_TEMP
//
// graphics file
void ThermGraphics::create_open_graphics_file(int data_type, int unit_type, int sample_type, int layer_computation,
                                              std::vector<int> &layers, int layer, std::ofstream &of_graphics_outfile) {
  std::string filename = "";

  switch(layer_computation) {
  case GFX_LAYER_AVE:
    filename += "lcomp-AVE_layers-";
    // print layers used in average
    filename += Utilities::stringify(layers[0]);
    for(unsigned int i = 1; i < layers.size(); i++)
      filename += "," + Utilities::stringify(layers[i]);

    filename += "_";
    break;
  case GFX_LAYER_NORMAL:
    filename += "lcomp-NORM_layer-" + Utilities::stringify(layer) + "_";
    break;
  case GFX_LAYER_DIF:
    filename += "lcomp-DIF_layers-";
    for(unsigned int i = 1; i < layers.size(); i++)
      filename += "," + Utilities::stringify(layers[i]);

    filename += "_";
    break;
  case GFX_LAYER_FLOORPLAN:
    filename += "lcomp-FLOORPLAN_layer-" + Utilities::stringify(layer) + ".svg";
    (of_graphics_outfile).open(filename.c_str(), std::ifstream::out);
    if(!(of_graphics_outfile))
      Utilities::fatal("Cannot create graphics output (.svg) file" + filename + "\n");
    return;

  default:
    std::cerr << "ThermGraphics::create_open_graphics_file => invalid layer computation type" << std::endl;
    exit(1);
  }

  switch(sample_type) {
  case GFX_CUR:
    filename += "smpltype-CUR_" + Utilities::stringify(datalibrary_->time_) + "s_";
    break;
  default:
    std::cerr << "ThermGraphics::create_open_graphics_file => invalid sample type" << std::endl;
    exit(1);
  }

  switch(unit_type) {
  case GFX_FUNIT:
    filename += "utype-FUNIT_";
    break;
  case GFX_MUNIT:
    filename += "utype-MUNIT_";
    break;
  default:
    std::cerr << "ThermGraphics::create_open_graphics_file => invalid unit type" << std::endl;
    exit(1);
  }

  switch(data_type) {
  case GFX_POWER:
    filename += "dtype-PWR";
    break;
  case GFX_TEMP:
    filename += "dtype-TEMP";
    break;
  default:
    std::cerr << "ThermGraphics::create_open_graphics_file => invalid unit type" << std::endl;
    exit(1);
  }

  // filename += ".svg";
  std::string reportFileName = Report::getNameID();
  filename                   = filename + reportFileName.erase(0, 5) + ".svg";
#ifdef _ESESCTHERM_DEBUG
  std::cerr << "Attempting to Create Graphics Output File:" << filename << " ... ";
#endif

  (of_graphics_outfile).open(filename.c_str(), std::ifstream::out);
  if(!(of_graphics_outfile))
    Utilities::fatal("Cannot create graphics output (.svg) file" + filename + "\n");
#ifdef _ESESCTHERM_DEBUG
  else
    std::cerr << "Output File Generated!" << std::endl;
#endif
}

bool ThermGraphics::get_enable_graphics() {
  return (datalibrary_->config_data_->enable_graphics_);
}
