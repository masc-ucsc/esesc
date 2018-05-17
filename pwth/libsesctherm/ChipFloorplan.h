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
File name:      ChipFloorplan.h
Classes:        ChipFloorplan_Unit
                ChipFloorplan
********************************************************************************/

#ifndef CHIP_FLOORPLAN_H
#define CHIP_FLOORPLAN_H

#include <complex>
#include <vector>

#include "sesctherm3Ddefine.h"

class ChipFloorplan;
class ChipFloorplan_Unit;
class DataLibrary;
class ModelUnit;

/* functional unit placement definition	*/
class ChipFloorplan_Unit {
  MATRIX_DATA power_;
  MATRIX_DATA power_per_unit_area_; // this is the current power dissipated by the flp (W/m^3)
public:
  ChipFloorplan_Unit() {
  }
  ChipFloorplan_Unit(DataLibrary *datalibrary);
  ChipFloorplan_Unit(DataLibrary *datalibrary, int id, std::string &name, MATRIX_DATA width, MATRIX_DATA height, MATRIX_DATA leftx,
                     MATRIX_DATA bottomy, MATRIX_DATA chipdensity, MATRIX_DATA interconnectdensity);
  friend std::ostream &operator<<(std::ostream &os, ChipFloorplan_Unit &tmp_ChipFloorplan_Unit);

  struct cmpWidth {
    bool operator()(const ChipFloorplan_Unit &a, const ChipFloorplan_Unit &b) const {
      return (LT(a.width_, b.width_));
    }
  };

  struct cmpHeight {
    bool operator()(const ChipFloorplan_Unit &a, const ChipFloorplan_Unit &b) const {
      return (LT(a.height_, b.height_));
    }
  };

  struct cmpLeftx {
    bool operator()(const ChipFloorplan_Unit &a, const ChipFloorplan_Unit &b) const {
      return (LT(a.leftx_, b.leftx_));
    }
  };

  struct cmpBottomy {
    bool operator()(const ChipFloorplan_Unit &a, const ChipFloorplan_Unit &b) const {
      return (LT(a.bottomy_, b.bottomy_));
    }
  };

  struct cmpArea {
    bool operator()(const ChipFloorplan_Unit &a, const ChipFloorplan_Unit &b) const {
      return (LT(a.area_, b.area_));
    }
  };

  // stores the line number/block description number where the declaraction occurs
  int line_number;

  // NOTE: power!=power_per_unit_area
  void        set_power(MATRIX_DATA power, MATRIX_DATA thickness);
  MATRIX_DATA get_power() {
    return power_;
  }

  MATRIX_DATA get_power_per_unit_area() {
    return power_per_unit_area_;
  }
  void set_power_per_unit_area(MATRIX_DATA v) {
    power_per_unit_area_ = v;
  }

  void                     set_max_power(MATRIX_DATA power);
  int                      id_; // number of the floorplan unit
  std::string              name_;
  MATRIX_DATA              width_;
  MATRIX_DATA              height_;
  MATRIX_DATA              leftx_;
  MATRIX_DATA              bottomy_;
  MATRIX_DATA              area_;
  MATRIX_DATA              chip_density_;
  MATRIX_DATA              interconnect_density_;
  std::vector<MATRIX_DATA> ModelUnit_percentages_;   // this is a list of the percentages of each of the dependent model units
  MATRIX_DATA              max_power_per_unit_area_; // this is the max power density
  MATRIX_DATA              max_dynamic_power_;
  MATRIX_DATA              leakage_power_;
  MATRIX_DATA              dynamic_power_;
  MATRIX_DATA              hot_spot_count_;
  MATRIX_DATA              hot_spot_duration_;
  MATRIX_DATA              temperature_;
  DataLibrary *            datalibrary_;
  std::vector<std::vector<ModelUnit *>> located_units_;
};

/* This encapsulates the entire chip floorplan, this includes all three layers including the inner virtual layer */
class ChipFloorplan {
public:
  ChipFloorplan(DataLibrary *datalibrary);
  friend std::ostream &operator<<(std::ostream &os, ChipFloorplan &tmp_floorplan);
  void                 get_floorplan_ucool(std::ifstream &if_infile, MATRIX_DATA width, MATRIX_DATA height);
  // read the .flp file, store information to ChipFloorplan_Units
  // store information to leftx_coords (sorted list of x-values)
  // store information to bottom_corrds (corted list of y-values)
  void get_floorplan(bool get_from_sescconf, int flp_num);
  void get_floorplan_sescconf(int flp_num);
  void get_floorplan_flpfile();
  // This offsets the data values
  void offset_floorplan(MATRIX_DATA x_amount, MATRIX_DATA y_amount);
  /* total chip width	*/
  MATRIX_DATA get_total_width(void);

  /* total chip height */
  MATRIX_DATA get_total_height(void);
  MATRIX_DATA get_total_area(void);

  std::vector<ChipFloorplan_Unit> &get_flp_units(void);

  std::vector<ChipFloorplan_Unit> &get_leftx_coords(void);

  std::vector<ChipFloorplan_Unit> &get_bottomy_coords(void);

  bool is_floorplan_defined();

  std::string get_floorplan_name();

  bool                            flp_unit_exists(MATRIX_DATA x, MATRIX_DATA y);
  ChipFloorplan_Unit &            find_flp_unit(MATRIX_DATA x, MATRIX_DATA y);
  std::vector<ChipFloorplan_Unit> flp_units_;
  std::vector<MATRIX_DATA>        leftx_coords_;
  std::vector<MATRIX_DATA>        bottomy_coords_;
  MATRIX_DATA                     max_x_;
  MATRIX_DATA                     max_y_;
  MATRIX_DATA                     area_;
  MATRIX_DATA                     width_;
  MATRIX_DATA                     height_;
  MATRIX_DATA                     hot_spot_count_;
  MATRIX_DATA                     hot_spot_duration_;
  std::string                     floorplan_name_;
  bool                            floorplan_defined_;
  DataLibrary *                   datalibrary_;
};

#endif
