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
File name:      ThermGraphics.h
Classes:        ThermGraphics
                ThermGraphics_FileType
                ThermGraphics_Color
********************************************************************************/

#ifndef ESESCTHERM3D_GRAPHICS_H
#define ESESCTHERM3D_GRAPHICS_H

#include <vector>

class ThermGraphics;
class ThermGraphics_Color;
class ThermGraphics_FileType;

class ThermGraphics {
public:
  ThermGraphics(DataLibrary *datalibrary);
  void print_floorplans();
  void print_graphics_helper(ThermGraphics_FileType &graphics_file_type, std::vector<MATRIX_DATA> &temperature_map,
                             std::vector<MATRIX_DATA> &power_map, DynamicArray<ModelUnit> &layer_dyn, int layer);
  void print_graphics();
  bool get_enable_graphics();
  void data_flp2svg(std::vector<ChipFloorplan_Unit> &flp_units, std::vector<MATRIX_DATA> &data_map, MATRIX_DATA min_dval,
                    MATRIX_DATA max_dval, std::ofstream &of_graphics_outfile);
  void data_model2svg(std::vector<ChipFloorplan_Unit> &flp_units, DynamicArray<ModelUnit> &layer_dyn, int data_type,
                      int sample_type, MATRIX_DATA min_dval, MATRIX_DATA max_dval, std::ofstream &of_graphics_outfile);
  void flp2svg(std::vector<ChipFloorplan_Unit> &flp_units, std::vector<MATRIX_DATA> data_map, std::ofstream &of_graphics_outfile,
               MATRIX_DATA &min_dataval, MATRIX_DATA &max_dataval);
  void flp2svg_simple(std::vector<ChipFloorplan_Unit> &flp_units, std::ofstream &of_graphics_outfile);
  void model2svg_helper(std::vector<ChipFloorplan_Unit> &flp_units, DynamicArray<ModelUnit> &layer_dyn, int data_type,
                        int sample_type, std::ofstream &of_graphics_outfile, MATRIX_DATA &min_dataval, MATRIX_DATA &max_dataval);
  void calc_x_bound(std::vector<ChipFloorplan_Unit> &ChipFloorplan_Units, MATRIX_DATA &min_x, MATRIX_DATA &max_x,
                    MATRIX_DATA &min_y, MATRIX_DATA &max_y);
  void calc_x_bound(DynamicArray<ModelUnit> &ChipFloorplan_Units, MATRIX_DATA &min_x, MATRIX_DATA &max_x, MATRIX_DATA &min_y,
                    MATRIX_DATA &max_y);
  void draw_color_scale_bar(MATRIX_DATA max_dataval, MATRIX_DATA min_dataval, MATRIX_DATA max_x, MATRIX_DATA max_y,
                            MATRIX_DATA min_x, MATRIX_DATA min_y, std::ofstream &of_graphics_outfile);
  void create_open_graphics_file(int data_type, int unit_type, int sample_type, int layer_computation, std::vector<int> &layers,
                                 int layer, std::ofstream &of_graphics_outfile);

  DataLibrary *datalibrary_;
  MATRIX_DATA  num_levels_;     // number of colors used
  MATRIX_DATA  max_rotate_;     // maximum hue rotation
  MATRIX_DATA  stroke_coeff_;   // used to tune the stroke-width
  MATRIX_DATA  stroke_opacity_; // used to control the opacity of the floor plan
  MATRIX_DATA  smallest_shown_; // fraction of the entire chip necessary to see macro
  MATRIX_DATA  zoom_;
  MATRIX_DATA  txt_offset_;
  // static int palette_[21][3];

  std::vector<ThermGraphics_Color> colors_;  // these are the colors to be used in the palette
  std::vector<ThermGraphics_Color> palette_; // this is the actual palette
  bool                             floorplans_printed_;
};

class ThermGraphics_FileType {
public:
  ThermGraphics_FileType(std::string config_string);
  int                  layer_computation_; // GFX_NORMAL/GFX_AVE/GFX_DIF
  int                  unit_type_;         // GFX_MUNIT/GFX_FUNIT
  int                  data_type_;         // GFX_POWER/GFX_TEMP
  int                  sample_type_;       // GFX_CUR/GFX_MAX/GFX_MIN/GFX_AVE
  MATRIX_DATA          min_val_;
  MATRIX_DATA          max_val_;
  std::vector<int>     layers_; // layers used
  friend std::ostream &operator<<(std::ostream &out, const ThermGraphics_FileType &type) {
    out << "printing ThermGraphics_FileType object" << std::endl;
    out << "Layer_computation_ :" << type.layer_computation_ << std::endl;
    out << "Unit_type_: " << type.unit_type_ << std::endl;
    out << "Data_type_: " << type.data_type_ << std::endl;
    out << "Sample_type_: " << type.sample_type_ << std::endl;
    out << "Layers_:[";
    for(unsigned int i = 0; i < type.layers_.size(); i++)
      out << type.layers_[i] << " ";
    out << "]" << std::endl;
    return out;
  }
};

class ThermGraphics_Color {
public:
  ThermGraphics_Color(int r, int g, int b, int steps);
  static void create_palette(std::vector<ThermGraphics_Color> &palette, std::vector<ThermGraphics_Color> &colors);
  static void gradient(std::vector<ThermGraphics_Color> &palette, ThermGraphics_Color &color1, ThermGraphics_Color &color2,
                       int steps);
  int         r_;
  int         g_;
  int         b_;
  int         steps_;
};

#endif
