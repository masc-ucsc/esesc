/*
   ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ian Lee
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
File name:      ThermModel.h
Classes:        ThermModel
********************************************************************************/

#ifndef THERM_MODEL_H
#define THERM_MODEL_H

#include "sescthermMacro.h"

class ThermModel
{
  private:
    DynamicArray<ModelUnit>& get_floorplan_layer(int layer);
    void print_unsolved_model();
    void check_matrices();
    void print_unsolved_model_row(int row);
    void set_initial_temperatures();
    void create_solution_matrices();
    void find_unused_model_units();

    void compute_model_units(bool calc_solution_only, MATRIX_DATA time_increment, bool accumulate_rc, bool use_rc);
    void partition_floorplans();
    MATRIX_DATA get_governing_equation(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height, int layer);
    unsigned int determine_heat_transfer_methods(ModelUnit& ModelUnit_source,ModelUnit& ModelUnit_dest);
    void store_model_unit(MATRIX_DATA rightx, MATRIX_DATA topy, MATRIX_DATA width, MATRIX_DATA height, int x_itor, int y_itor, int layer);
    void generate_pins_layer(MATRIX_DATA width, MATRIX_DATA height);
    void recompute_material_properties();
    void recompute_material_properties(int layer);
    void run_sampler(MATRIX_DATA timestep, bool use_rc, bool recompute_material_properties);
  public:
    ThermModel(); //eka, to have static declaration
    ThermModel(const char* flp_filename,
        const char* ucool_filename, 
        bool useRK4, 
        const char* output_filename="sesctherm.out",
        bool get_chipflp_from_sescconf=true, 
        bool get_ucoolflp_from_sescconf=false);
    ~ThermModel();
    //eka, to initialize when declared with static constructor
    void ThermModelInit(const char* flp_filename,
        const char* ucool_filename, 
        bool useRK4, 
        const char* output_filename="sesctherm.out",
        bool get_chipflp_from_sescconf=true, 
        bool get_ucoolflp_from_sescconf=false);

    void compute(MATRIX_DATA timestep, bool recompute_material_properties);
    void compute(bool recompute_material_properties);	
    void compute_rc_start();
    void compute_rc_stop();
    void fast_forward(MATRIX_DATA timestep);
    void set_time(MATRIX_DATA timestep);
    void determine_gridsize();
    void determine_timestep();
    void print_graphics();
    void accumulate_sample_data();
    void compute_sample();
    void set_power_flp(int flp_pos, int layer, MATRIX_DATA power);
    void set_power_layer(int layer, std::vector<MATRIX_DATA>& power_map);
    void set_temperature_flp(int flp_pos, int flp_layer, int source_layer, MATRIX_DATA temperature);
    void set_temperature_layer(int flp_layer, int source_layer, std::vector<MATRIX_DATA>& temperature_map);
    void set_temperature_layer(int source_layer, MATRIX_DATA temperature);
    void enable_temp_locking(int layer, MATRIX_DATA temperature);
    void disable_temp_locking(int layer);
    MATRIX_DATA get_temperature_layer_average(int source_layer);
    std::vector<MATRIX_DATA> get_temperature_layer_map(int flp_layer, int source_layer);
    std::vector<MATRIX_DATA> get_temperature_layer_noavg(int flp_layer, int source_layer);
    MATRIX_DATA get_temperature_flp(int flp_pos, int flp_layer, int source_layer);
    // vector<MATRIX_DATA> get_power_layer(int flp_layer);
    // MATRIX_DATA get_power_flp(int flp_layer, int flp_pos);
    void set_ambient_temp(MATRIX_DATA temp);
    void set_initial_temp(MATRIX_DATA temp);
    MATRIX_DATA get_time();
    MATRIX_DATA get_max_timestep();
    MATRIX_DATA get_recommended_timestep();
    unsigned int chip_flp_count(int layer);
    std::string get_chip_flp_name(int flp_pos, int layer);
    void set_max_power_flp(int flp_pos, int layer, MATRIX_DATA power);
    void dump_temp_data(); // FIXME: rename dump
    void run_model(MATRIX_DATA timestep, bool accumulate_rc, bool use_rc, bool recompute_material_properties);
    int get_modelunit_count(int layer);
    int get_modelunit_rows(int layer);
    int get_modelunit_cols(int layer);

    void dump_flp_temp_data(); // TODO
    void print_flp_temp_labels(); // TODO
    void warmup_chip(); // TODO
    void dump_3d_graph(); // TODO
    void print_model_units();

    DynamicArray<ModelUnit> &get_dyn_array(int layer) {
      return datalibrary_->get_dyn_array(layer);
    }

    //this contains all of the data 
    DataLibrary* datalibrary_;

    MATRIX_DATA time_;
    MATRIX_DATA time_step_;
    MATRIX_DATA cur_sample_end_time_;

    std::vector<ModelUnit *>  _matrix_model_units; // one to one link, i-th entry is i-th used unit
    std::vector<MATRIX_DATA>   leftx_;         //holds compiled list of leftx_values
    std::vector<MATRIX_DATA>   bottomy_;        //holds compiled list of bottomy_values
    bool _useRK4;				// whether RK4 is to be used with complete coefficient matrices
};

#endif
