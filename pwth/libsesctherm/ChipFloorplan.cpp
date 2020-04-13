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
File name:      ChipFloorplan.cpp
Description:    Handles the reading and modeling of the chip floorplan
********************************************************************************/

#include <iomanip>
#include <algorithm>

#include "sescthermMacro.h"

#include "SescConf.h"

#include "ChipFloorplan.h"
#include "ConfigData.h"
#include "DataLibrary.h"
#include "nanassert.h"

/* functional unit placement definition	*/
// FLP_UNIT
ChipFloorplan_Unit::ChipFloorplan_Unit(DataLibrary *datalibrary) {
  datalibrary_ = datalibrary;

  for(unsigned int i = 0; i < datalibrary_->all_layers_info_.size(); i++) {
    std::vector<ModelUnit *> located_units_layer;
    located_units_layer.clear();
    located_units_.push_back(located_units_layer);
  }
  name_                = "";
  width_               = 0;
  height_              = 0;
  leftx_               = 0;
  bottomy_             = 0;
  power_per_unit_area_ = 0;
  area_                = 0;
  dynamic_power_       = 0;
  leakage_power_       = 0;
  hot_spot_count_      = 0;
  hot_spot_duration_   = 0;
}

ChipFloorplan_Unit::ChipFloorplan_Unit(DataLibrary *datalibrary, int id, std::string &name, MATRIX_DATA width, MATRIX_DATA height,
                                       MATRIX_DATA leftx, MATRIX_DATA bottomy, MATRIX_DATA chipdensity,
                                       MATRIX_DATA interconnectdensity) {
  datalibrary_          = datalibrary;
  id_                   = id;
  name_                 = name;
  height_               = height;
  width_                = width;
  leftx_                = leftx;
  bottomy_              = bottomy;
  area_                 = width * height;
  chip_density_         = chipdensity;
  interconnect_density_ = interconnectdensity;
  power_per_unit_area_  = 0;
  dynamic_power_        = 0;
  leakage_power_        = 0;
  hot_spot_count_       = 0;
  hot_spot_duration_    = 0;
  located_units_.clear();
  for(unsigned int i = 0; i < 20; i++) {
    std::vector<ModelUnit *> located_units_layer;
    located_units_layer.clear();
    located_units_.push_back(located_units_layer);
  }
}

// NOTE: power!=power_per_unit_area
void ChipFloorplan_Unit::set_power(MATRIX_DATA power, MATRIX_DATA thickness) {
  power_               = power;
  power_per_unit_area_ = power / (width_ * height_ * thickness);
}

// NOTE: power!=power_per_unit_area
void ChipFloorplan_Unit::set_max_power(MATRIX_DATA max_dynamic_power) {
  max_dynamic_power_ = max_dynamic_power;
}

/* This encapsulates the entire chip ChipFloorplan, this includes all three layers including the inner virtual layer */
// FLOORPLAN
ChipFloorplan::ChipFloorplan(DataLibrary *datalibrary) {
  datalibrary_ = datalibrary;
}

std::ostream &operator<<(std::ostream &os, ChipFloorplan &tmp_floorplan) {

  os << std::setprecision(11);
  os << "************ chip_floorplan_DATA ************" << std::endl;
  os << "ChipFloorplan Defined?:" << ((tmp_floorplan.floorplan_defined_ == true) ? "yes!" : "no") << std::endl;
  os << "flp_units_:[itor][Name        ] [Width       ] [Height      ] [Leftx       ] [Bottomy     ] [Area            ]"
     << std::endl;
  for(unsigned int i = 0; i < tmp_floorplan.flp_units_.size(); i++) {
    os << "flp_units_:[" << i << "]\t [";
    os << std::setw(12) << tmp_floorplan.flp_units_[i].name_;
    os << "] [" << std::setw(12) << tmp_floorplan.flp_units_[i].width_;
    os << "] [" << std::setw(12) << tmp_floorplan.flp_units_[i].height_;
    os << "] [" << std::setw(12) << tmp_floorplan.flp_units_[i].leftx_;
    os << "] [" << std::setw(12) << tmp_floorplan.flp_units_[i].bottomy_;
    os << "] [" << std::setw(16) << tmp_floorplan.flp_units_[i].area_ << "]" << std::endl;
  }

  for(unsigned int i = 0; i < tmp_floorplan.leftx_coords_.size(); i++)
    os << "leftx_coords:[" << i << "] =" << tmp_floorplan.leftx_coords_[i] << std::endl;

  for(unsigned int i = 0; i < tmp_floorplan.bottomy_coords_.size(); i++)
    os << "bottomy_coords_:[" << i << "] =" << tmp_floorplan.bottomy_coords_[i] << std::endl;

  os << "max_x_=" << tmp_floorplan.max_x_ << std::endl;
  os << "max_y_=" << tmp_floorplan.max_y_ << std::endl;
  os << "chip_area_=" << tmp_floorplan.area_ << std::endl;
  os << "chip_floorplan_name_=" << tmp_floorplan.floorplan_name_ << std::endl;

  return (os);
}

// FIXME: this needs to handle densities (not currently implemented)
void ChipFloorplan::get_floorplan_flpfile() {
  I(0);

  /*
     std::string line;
     std::vector<string> tokens;
     floorplan_defined_=false;
     int line_number=0;
     ifstream& if_infile=datalibrary_->if_flpfile_;
     if (if_infile == NULL)
     Utilities::fatal("Unable to read ChipFloorplan File");
     int id=0;
     while (getline(if_infile,line)) {
     line_number++;
     if (line.empty())
     continue; //ignore blank lines
     Utilities::Tokenize(line, tokens, " \t"); //tokensize the line by space and tabs
     if (tokens[0] == "#")
     continue;//skip comments
     if (tokens.size()!=5) {
     std::cerr << "Error Reading Chip FLP File: Invalid line-length [Line" << line_number << "]" << std::endl;
     std::cerr << "Number of tokens found" << tokens.size() << std::endl;
     exit(1);
     }
     ChipFloorplan_Unit newunit(id,
     tokens[0],
     Utilities::convertToDouble(tokens[1]),
     Utilities::convertToDouble(tokens[2]),
     Utilities::convertToDouble(tokens[3]),
     Utilities::convertToDouble(tokens[4]));
     flp_units_.push_back(newunit);
     leftx_coords_.push_back(Utilities::convertToDouble(tokens[3]));
     bottomy_coords_.push_back(Utilities::convertToDouble(tokens[4]));
     id++;
     }

     std::vector<ChipFloorplan_Unit> tmp_flp_units=flp_units_;
  //Sort the FLP units by leftx
  std::sort(tmp_flp_units.begin(),tmp_flp_units.end(),ChipFloorplan_Unit::cmpLeftx());

  ChipFloorplan_Unit* tmpunit=NULL;
  //Add data point for right-most x-point
  tmpunit=&(tmp_flp_units[tmp_flp_units.size()-1]);
  leftx_coords_.push_back(tmpunit->leftx_+tmpunit->width_);

  //sort the FLP units by bottomy
  std::sort(tmp_flp_units.begin(),tmp_flp_units.end(),ChipFloorplan_Unit::cmpBottomy());

  //Add data point for top-most y-point
  tmpunit=&(tmp_flp_units[tmp_flp_units.size()-1]);
  bottomy_coords_.push_back(tmpunit->bottomy_+tmpunit->height_);

  //sort the leftx std::vector by leftx datapoints
  std::sort(leftx_coords_.begin(),leftx_coords_.end());
  std::vector<MATRIX_DATA>::iterator new_end= std::unique(leftx_coords_.begin(),leftx_coords_.end());
  // delete all elements past new_end
  leftx_coords_.erase(new_end, leftx_coords_.end());

  //store the maximum leftx datapoint (chip width)
  max_x_=leftx_coords_[leftx_coords_.size()-1];

  //sort the bottom std::vector by bottomy datapoints
  std::sort(bottomy_coords_.begin(),bottomy_coords_.end());
  new_end=std::unique(bottomy_coords_.begin(),bottomy_coords_.end());
  // delete all elements past new_end
  bottomy_coords_.erase(new_end, bottomy_coords_.end());

  //store the maximum bottomy datapoint (chip height)
  max_y_=bottomy_coords_[bottomy_coords_.size()-1];

  //now calculate the chip area
  area_=max_x_*max_y_;
  //successfully read the ChipFloorplan
  floorplan_defined_=true;
  floorplan_name_="chip floorplan";
  width_=max_x_;
  height_=max_y_;
  */
}

void ChipFloorplan::get_floorplan_sescconf(int flp_num) {

  std::vector<std::string> tokens;
  std::string              line;
  int                      min, max;
  // Determine which floorplan we are reading from
  const char *thermSec     = SescConf->getCharPtr("", "thermal");
  const char *floorplanSec = SescConf->getCharPtr(thermSec, "floorplan", flp_num);

  // READ the floorplan parameters
  min = SescConf->getRecordMin(floorplanSec, "blockDescr");
  max = SescConf->getRecordMax(floorplanSec, "blockDescr");

  for(int id = min; id <= max; id++) {
    const char *blockdescr = SescConf->getCharPtr(floorplanSec, "blockDescr", id);
    line                   = blockdescr;
    Utilities::Tokenize(line, tokens, " "); // tokensize the line by spaces

    ASSERT_ESESCTHERM(tokens.size() == 5, "Error Reading Chip FLP Description in Sescconf!", "ChipFloorplan",
                      "get_floorplan_sescconf")

    line                                    = tokens[0];
    MATRIX_DATA        chip_density         = SescConf->getDouble(floorplanSec, "blockchipDensity", id);
    MATRIX_DATA        interconnect_density = SescConf->getDouble(floorplanSec, "blockinterconnectDensity", id);
    ChipFloorplan_Unit newunit(&(*datalibrary_), id - min, line, Utilities::convertToDouble(tokens[1]),
                               Utilities::convertToDouble(tokens[2]), Utilities::convertToDouble(tokens[3]),
                               Utilities::convertToDouble(tokens[4]), chip_density, interconnect_density);
    flp_units_.push_back(newunit);

    leftx_coords_.push_back(Utilities::convertToDouble(tokens[3]));
    bottomy_coords_.push_back(Utilities::convertToDouble(tokens[4]));
  }
  std::vector<ChipFloorplan_Unit> tmp_flp_units = flp_units_;
  // Sort the FLP units by leftx
  std::sort(tmp_flp_units.begin(), tmp_flp_units.end(), ChipFloorplan_Unit::cmpLeftx());

  ChipFloorplan_Unit *tmpunit = NULL;
  // Add data point for right-most x-point
  tmpunit = &(tmp_flp_units[tmp_flp_units.size() - 1]);
  leftx_coords_.push_back(tmpunit->leftx_ + tmpunit->width_);

  // sort the FLP units by bottomy
  std::sort(tmp_flp_units.begin(), tmp_flp_units.end(), ChipFloorplan_Unit::cmpBottomy());

  // Add data point for top-most y-point
  tmpunit = &(tmp_flp_units[tmp_flp_units.size() - 1]);
  bottomy_coords_.push_back(tmpunit->bottomy_ + tmpunit->height_);

  // sort the leftx std::vector by leftx datapoints
  std::sort(leftx_coords_.begin(), leftx_coords_.end());
  std::vector<MATRIX_DATA>::iterator new_end = std::unique(leftx_coords_.begin(), leftx_coords_.end());
  // delete all elements past new_end
  leftx_coords_.erase(new_end, leftx_coords_.end());

  // store the maximum leftx datapoint (chip width)
  max_x_ = leftx_coords_[leftx_coords_.size() - 1];

  // sort the bottom std::vector by bottomy datapoints
  std::sort(bottomy_coords_.begin(), bottomy_coords_.end());
  new_end = std::unique(bottomy_coords_.begin(), bottomy_coords_.end());
  // delete all elements past new_end
  bottomy_coords_.erase(new_end, bottomy_coords_.end());

  // store the maximum bottomy datapoint (chip height)
  max_y_ = bottomy_coords_[bottomy_coords_.size() - 1];

  // now calculate the chip area
  area_ = max_x_ * max_y_;
  // successfully read the ChipFloorplan
  floorplan_defined_ = true;
  floorplan_name_    = "chip floorplan";
  width_             = max_x_;
  height_            = max_y_;

  MATRIX_DATA chip_width  = datalibrary_->config_data_->chip_width_;
  MATRIX_DATA chip_height = datalibrary_->config_data_->chip_height_;

  // Scale the leftx/topy datapoints to fit the size of the chip

  MATRIX_DATA rangex = leftx_coords_[leftx_coords_.size() - 1] - leftx_coords_[0];
  MATRIX_DATA rangey = bottomy_coords_[bottomy_coords_.size() - 1] - bottomy_coords_[0];

  // scale each of the leftx/bottomy coordinates to match the actual size of the chip
  // x_coord/rangex*chip_width=new_x_coord
  // y_coord/rangey*chip_height=new_y_coord
  for(int i = 0; i < (int)leftx_coords_.size(); i++)
    leftx_coords_[i] = ((leftx_coords_[i] - leftx_coords_[0]) / rangex) * chip_width + leftx_coords_[0];

  for(int i = 0; i < (int)bottomy_coords_.size(); i++)
    bottomy_coords_[i] = ((bottomy_coords_[i] - bottomy_coords_[0]) / rangey) * chip_height + bottomy_coords_[0];

  for(int i = 0; i < (int)flp_units_.size(); i++) {
    flp_units_[i].leftx_   = ((flp_units_[i].leftx_ - leftx_coords_[0]) / rangex) * chip_width + leftx_coords_[0];
    flp_units_[i].bottomy_ = ((flp_units_[i].bottomy_ - bottomy_coords_[0]) / rangey) * chip_height + bottomy_coords_[0];
    flp_units_[i].width_   = (flp_units_[i].width_ / rangex) * chip_width;
    flp_units_[i].height_  = (flp_units_[i].height_ / rangey) * chip_height;
    flp_units_[i].area_    = flp_units_[i].width_ * flp_units_[i].height_;
  }

  max_x_  = leftx_coords_[leftx_coords_.size() - 1];
  max_y_  = bottomy_coords_[bottomy_coords_.size() - 1];
  area_   = max_x_ * max_y_;
  width_  = chip_width;
  height_ = chip_height;
}

// read the .flp file, store information to flp_units
// store information to leftx_coords (sorted list of x-values)
// store information to bottom_corrds (corted list of y-values)
void ChipFloorplan::get_floorplan(bool get_from_sescconf, int flp_num) {
  if(get_from_sescconf)
    get_floorplan_sescconf(flp_num);
  else
    get_floorplan_flpfile();
}

// This offsets the data values
void ChipFloorplan::offset_floorplan(MATRIX_DATA x_amount, MATRIX_DATA y_amount) {
  for(unsigned int i = 0; i < leftx_coords_.size(); i++) {
    leftx_coords_[i] += x_amount;
  }
  for(unsigned int i = 0; i < bottomy_coords_.size(); i++) {
    bottomy_coords_[i] += y_amount;
  }
  for(unsigned int i = 0; i < flp_units_.size(); i++) {
    flp_units_[i].bottomy_ += y_amount;
    flp_units_[i].leftx_ += x_amount;
  }
  // sort the leftx std::vector by leftx datapoints
  std::sort(leftx_coords_.begin(), leftx_coords_.end());
  // store the maximum leftx datapoint (chip width)
  max_x_ = leftx_coords_[leftx_coords_.size() - 1];
  // sort the bottom std::vector by bottomy datapoints
  std::sort(bottomy_coords_.begin(), bottomy_coords_.end());
  // store the maximum bottomy datapoint (chip height)
  max_y_ = bottomy_coords_[bottomy_coords_.size() - 1];
  // now calculate the chip area (will be the same)
  MATRIX_DATA tmp_area = (max_x_ - leftx_coords_[0]) * (max_y_ - bottomy_coords_[0]);
  if(!EQ(tmp_area, area_) && area_ != 0) { // if the new chip area is different (and we are not using the ucooler floorplan)
    std::cerr << "FATAL: offset_floorplan: new chip area (after offset) different (should be the same ALWAYS)" << std::endl;
    std::cerr << "new chip area:[" << tmp_area << "] old chip area:[" << area_ << "]" << std::endl;
    std::cerr << "error:" << area_ - tmp_area << std::endl;
    exit(1);
  }
}
/* total chip width	*/
MATRIX_DATA ChipFloorplan::get_total_width(void) {
  return (max_x_);
}

/* total chip height */
MATRIX_DATA ChipFloorplan::get_total_height(void) {
  return (max_y_);
}

MATRIX_DATA ChipFloorplan::get_total_area(void) {
  return (area_);
}

std::vector<ChipFloorplan_Unit> &ChipFloorplan::get_flp_units(void) {
  return (flp_units_);
}

std::vector<ChipFloorplan_Unit> &ChipFloorplan::get_leftx_coords(void) {
  return (flp_units_);
}

std::vector<ChipFloorplan_Unit> &ChipFloorplan::get_bottomy_coords(void) {
  return (flp_units_);
}

bool ChipFloorplan::is_floorplan_defined() {
  return (floorplan_defined_);
}

std::string ChipFloorplan::get_floorplan_name() {
  return (floorplan_name_);
}
bool ChipFloorplan::flp_unit_exists(MATRIX_DATA x, MATRIX_DATA y) {

  for(unsigned int i = 0; i < flp_units_.size(); i++) {
    if(LE(flp_units_[i].bottomy_, y) && GE(flp_units_[i].bottomy_ + flp_units_[i].height_, y) && LE(flp_units_[i].leftx_, x) &&
       GE(flp_units_[i].leftx_ + flp_units_[i].width_, x)) {
#ifdef _ESESCTHERM_DEBUG
      //          std::cerr << "Found FLP unit at:[" << x << "][" << y << "]" << std::endl;
#endif
      return (true);
    }
  }
  return (false);
}
ChipFloorplan_Unit &ChipFloorplan::find_flp_unit(MATRIX_DATA x, MATRIX_DATA y) {
  // MATRIX_DATA bottomy_;
  // MATRIX_DATA leftx_;
  // MATRIX_DATA height_;
  // MATRIX_DATA width_;
  for(unsigned int i = 0; i < flp_units_.size(); i++) {
    // bottomy_=flp_units_[i].bottomy_;
    // leftx_=flp_units_[i].leftx_;
    // height_=flp_units_[i].height_;
    // width_=flp_units_[i].width_;
    if(LE(flp_units_[i].bottomy_, y) && GE(flp_units_[i].bottomy_ + flp_units_[i].height_, y) && LE(flp_units_[i].leftx_, x) &&
       GE(flp_units_[i].leftx_ + flp_units_[i].width_, x))
      return (flp_units_[i]);
  }
  std::cerr << "find_flp_unit: [" << x << "][" << y << "] Does not exist. Use flp_unit_exists to check first" << std::endl;
  exit(1);
}
