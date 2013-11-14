/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2008 University of California, Santa Cruz.
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Ian Lee
                  Joseph Nayfach - Battilana
                  Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.

********************************************************************************
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
    ThermGraphics(DataLibrary* datalibrary);
    void print_floorplans();
    void print_graphics_helper(ThermGraphics_FileType& graphics_file_type,
        std::vector<MATRIX_DATA>& temperature_map,
        std::vector<MATRIX_DATA>& power_map,
        DynamicArray<ModelUnit>& layer_dyn,
        int layer);
    void print_graphics();
    bool get_enable_graphics(); 
    void data_flp2svg(std::vector<ChipFloorplan_Unit>& flp_units, std::vector<MATRIX_DATA>& data_map, MATRIX_DATA min_dval, MATRIX_DATA max_dval, std::ofstream& of_graphics_outfile);
    void data_model2svg(std::vector<ChipFloorplan_Unit>& flp_units, DynamicArray<ModelUnit>& layer_dyn, int data_type, int sample_type, MATRIX_DATA min_dval, MATRIX_DATA max_dval, std::ofstream& of_graphics_outfile);
    void flp2svg(std::vector<ChipFloorplan_Unit>& flp_units,std::vector<MATRIX_DATA> data_map, std::ofstream& of_graphics_outfile, MATRIX_DATA& min_dataval, MATRIX_DATA& max_dataval);
    void flp2svg_simple(std::vector<ChipFloorplan_Unit>& flp_units, std::ofstream& of_graphics_outfile);
    void model2svg_helper(std::vector<ChipFloorplan_Unit>& flp_units, 
        DynamicArray<ModelUnit>& layer_dyn, 
        int data_type,
        int sample_type,
        std::ofstream& of_graphics_outfile, MATRIX_DATA& min_dataval, MATRIX_DATA& max_dataval);
    void calc_x_bound(std::vector<ChipFloorplan_Unit>& ChipFloorplan_Units, MATRIX_DATA& min_x, MATRIX_DATA& max_x, MATRIX_DATA& min_y, MATRIX_DATA& max_y);
    void calc_x_bound(DynamicArray<ModelUnit>& ChipFloorplan_Units, MATRIX_DATA& min_x, MATRIX_DATA& max_x, MATRIX_DATA& min_y, MATRIX_DATA& max_y);
    void draw_color_scale_bar(MATRIX_DATA max_dataval, MATRIX_DATA min_dataval, MATRIX_DATA max_x, MATRIX_DATA max_y, MATRIX_DATA min_x, MATRIX_DATA min_y, std::ofstream& of_graphics_outfile);
    void create_open_graphics_file(int data_type, int unit_type, int sample_type, int layer_computation, std::vector<int>& layers, int layer, std::ofstream& of_graphics_outfile);

    DataLibrary* datalibrary_;
    MATRIX_DATA num_levels_;			    //number of colors used
    MATRIX_DATA max_rotate_;				//maximum hue rotation  								 
    MATRIX_DATA stroke_coeff_;			//used to tune the stroke-width
    MATRIX_DATA stroke_opacity_;			//used to control the opacity of the floor plan
    MATRIX_DATA smallest_shown_;			//fraction of the entire chip necessary to see macro
    MATRIX_DATA zoom_;
    MATRIX_DATA txt_offset_;
    //static int palette_[21][3];		

    std::vector<ThermGraphics_Color> colors_;	//these are the colors to be used in the palette
    std::vector<ThermGraphics_Color> palette_;	//this is the actual palette
    bool floorplans_printed_;
};

class ThermGraphics_FileType{
  public:
    ThermGraphics_FileType(std::string config_string);
    int layer_computation_; //GFX_NORMAL/GFX_AVE/GFX_DIF
    int unit_type_;			//GFX_MUNIT/GFX_FUNIT
    int data_type_;			//GFX_POWER/GFX_TEMP
    int sample_type_;		//GFX_CUR/GFX_MAX/GFX_MIN/GFX_AVE
    MATRIX_DATA min_val_;
    MATRIX_DATA max_val_;
    std::vector<int> layers_;	//layers used
    friend std::ostream& operator<<(std::ostream& out, const ThermGraphics_FileType& type)
    {
      out << "printing ThermGraphics_FileType object" << std::endl;
      out << "Layer_computation_ :" << type.layer_computation_ << std::endl;
      out << "Unit_type_: " << type.unit_type_ << std::endl;
      out << "Data_type_: " << type.data_type_ << std::endl;
      out << "Sample_type_: " << type.sample_type_ << std::endl;
      out << "Layers_:[";
      for(unsigned int i=0;i< type.layers_.size();i++)
        out << type.layers_[i] << " ";
      out << "]" << std::endl;
      return out;
    }

};

class ThermGraphics_Color{
  public:
    ThermGraphics_Color(int r, int g, int b, int steps);
    static void create_palette(std::vector<ThermGraphics_Color>& palette, std::vector<ThermGraphics_Color>& colors);
    static void gradient(std::vector<ThermGraphics_Color>& palette, ThermGraphics_Color& color1, ThermGraphics_Color& color2, int steps);
    int r_;
    int g_;
    int b_;
    int steps_;
};

#endif

