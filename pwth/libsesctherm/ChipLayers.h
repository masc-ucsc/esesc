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
File name:      ChipLayers.h
Classes:        ChipLayers
********************************************************************************/

#ifndef CHIP_LAYERS_H
#define CHIP_LAYERS_H

#include <vector>
#include <map>

class ModelUnit;

class ChipLayers {
  public:
    ChipLayers(DataLibrary* datalibrary, int num_layers);
    ~ChipLayers();
    void determine_layer_properties();
    void offset_layer();
    MATRIX_DATA compute_average_temp();
    std::vector<MATRIX_DATA> compute_average_temps(int flp_layer);
    std::vector<MATRIX_DATA> compute_temps(int flp_layer);
    std::vector<MATRIX_DATA> compute_average_powers(int flp_layer);		
    std::vector<MATRIX_DATA> compute_energy();

    static void allocate_layers(DataLibrary* datalibrary_);

    //this computes the average of both temperature and power across all of the layers
    static DynamicArray<ModelUnit> compute_layer_averages(std::vector<int>& layers, 
        DataLibrary* datalibrary_);
    static DynamicArray<ModelUnit> compute_layer_diff(int layer1, int layer2, 
        DataLibrary* datalibrary_);

    void print(bool detailed);
    void print_lumped_metrics();
    //static void ChipLayers::compute_offsets(int layer_num, DataLibrary* datalibrary_);

    void accumulate_data_for_sample();
    void compute_sample();
    int flp_num_;
    int layer_number_;
    std::string name_;
    int type_;		//the layer types are defined in the sesctherm3Ddefine.h
    MATRIX_DATA thickness_;

    MATRIX_DATA height_;
    MATRIX_DATA width_;
    MATRIX_DATA leftx_;	//calculated 
    MATRIX_DATA bottomy_;	//calculated
    MATRIX_DATA rightx_;
    MATRIX_DATA topy_;

    //these are the effective material properties for the layer (averaged)
    MATRIX_DATA horizontal_conductivity_;
    MATRIX_DATA vertical_conductivity_down_;
    MATRIX_DATA vertical_conductivity_up_;
    MATRIX_DATA spec_heat_;
    MATRIX_DATA density_;
    MATRIX_DATA convection_coefficient_;		//used for fluid/air layers
    MATRIX_DATA emissivity_;

    //These are binary values, use DEFINES: HEAT_CONVECTION_TRANSFER, HEAT_CONDUCTION_TRANSFER, HEAT_EMMISION_TRANSFER for transfer types
    unsigned int heat_transfer_methods_;

    MATRIX_DATA granularity_;

    ChipFloorplan* chip_floorplan_;	//if this is a chip layer, this will be defined, NULL otherwise

    DataLibrary* datalibrary_;			//link to the datalibrary

    std::vector< std::vector< ModelUnit * > > located_units_;
    DynamicArray<ModelUnit>* floorplan_layer_dyn_;  //this is the dynamic array which corresponds to this layer
    int unused_dyn_count_;				  //this stores the number of unused dynamic array entries

    std::map<MATRIX_DATA,int> coord_to_index_x_;
    std::map<MATRIX_DATA,int> coord_to_index_y_;

    //this is the temperature map for the layer (related to chip floorplan)
    //this is only currently used for the transistor layer these are temperatures averaged by flp unit
    std::vector<MATRIX_DATA> temperature_map_;	

    std::vector<MATRIX_DATA> power_map_;

    DynamicArray<MATRIX_DATA> running_average_temperature_map_;		//this is used to create the running average temperatures
    DynamicArray<MATRIX_DATA> running_average_energy_map_;
    DynamicArray<MATRIX_DATA> running_max_temperature_map_;
    DynamicArray<MATRIX_DATA> running_min_temperature_map_;
    DynamicArray<MATRIX_DATA> running_max_energy_map_;
    DynamicArray<MATRIX_DATA> running_min_energy_map_;
    DynamicArray<MATRIX_DATA> sample_temperature_map_;				//this is the average temps over the sample period

    bool layer_used_;					//indicates if the layer is used in the stackup
    int num_layers_;

    bool temp_locking_enabled_;					//lock the temperature to lock_temp_
    MATRIX_DATA lock_temp_;

  private:
    class temporary_layer_info {
      public:
        struct cmpgranularitythensize {
          bool operator()(const temporary_layer_info& a, const temporary_layer_info& b) const {
            if(EQ(a.granularity_,b.granularity_)){
              return(LT(a.size_,b.size_));
            }
            else 
              return(LT(a.granularity_,b.granularity_));
          }
        };				
        temporary_layer_info(int layer_num, MATRIX_DATA granularity, MATRIX_DATA height, MATRIX_DATA width, MATRIX_DATA size, int nregions, MATRIX_DATA areas, MATRIX_DATA areas_removed){
          layer_num_=layer_num;
          granularity_=granularity;
          height_=height;
          width_=width;
          size_=size;
          nregions_=nregions;
          areas_=areas;
          areas_removed_=areas_removed;
        }
        int layer_num_;
        MATRIX_DATA granularity_;
        MATRIX_DATA height_;
        MATRIX_DATA width_;
        MATRIX_DATA size_;
        int nregions_;
        MATRIX_DATA areas_;
        MATRIX_DATA areas_removed_;	
    };
};

#endif
