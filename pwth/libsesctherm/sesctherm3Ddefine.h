// Contributed by Ian Lee
//                Joseph Nayfach - Battilana
//                Jose Renau
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
File name:      sesctherm3Ddefine.h
Description:    This is the only place where defines occur throughout the framework.
********************************************************************************/

#include <stdint.h>

#include "sescthermMacro.h"

#ifndef ESESCTHERM3D_DEFINE_H
#define ESESCTHERM3D_DEFINE_H

//#define _ESESCTHERM_USE_MP
//#define _ESESCTHERM_DEBUG //turn on robust debugging of entire model
#define _ESESCTHERM_DEBUG_RUNTIME // enable runtime debugging to catch runtime errors
// this should be left on unless the model is assured to be given accurate input

#define _ESESCTHERM_INLINE __attribute__((always_inline)) __attribute__((nothrow))
#define _ESESCTHERM_CONST_INLINE const __attribute__((const)) __attribute__((always_inline)) __attribute__((nothrow))

#ifdef _ESESCTHERM_DEBUG_RUNTIME
#define ASSERT_ESESCTHERM(cond, error, class, function)                                                                         \
  do {                                                                                                                          \
    if(!(cond)) {                                                                                                               \
      std::cerr << "FATAL: " << class << "::" << function << "() => " << error << " on line " << __LINE__ << " in " << __FILE__ \
                << std::endl                                                                                                    \
                << std::endl;                                                                                                   \
      exit(1);                                                                                                                  \
    }                                                                                                                           \
  } while(0);
#else
#define ASSERT_ESESCTHERM(cond, error, class, function)
#endif

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif
#define ODD(X) ((X) % 2 ? (true) : (false))
#define EVEN(X) ((X) % 2 ? (false) : (true))
#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

// #ifdef USE_FLOAT
// #define DELTA		1.0e-5
// #else
// #define DELTA		1.0e-9
// #endif

#define ROUND(x) (ceil(x * pow(10.0, 9.0)) / pow(10.0, 9.0))
#define EQ(x, y) (fabs(x - y) < DELTA)
#define LT(x, y) (x < y && !(fabs(x - y) < DELTA))
#define GT(x, y) (x > y && !(fabs(x - y) < DELTA))
#define GE(x, y) (x > y || (fabs(x - y) < DELTA))
#define LE(x, y) (x < y || (fabs(x - y) < DELTA))
#define ABS_DIF(x, y) ((GE(x, y)) ? (fabs(x - y)) : (fabs(y - x)))

#define BOLTZMAN_CONST 5.67e-8

typedef MATRIX_DATA SUElement_t;

// define the heat transfer types for the model (aka base) units
enum { HEAT_CONVECTION_TRANSFER, HEAT_CONDUCTION_TRANSFER, HEAT_RADIATION_TRANSFER };

// define the technology
enum { TECH_250, TECH_180, TECH_130, TECH_90, TECH_65 };

// The following defines the various kinds of materials
enum { BULK_SI, SI_O2, POLYSILICON, COPPER, VIRTUAL, SIMOX, DIELECTRIC_PACKAGE, DIELECTRIC_CHIP };

// This defines the graphics file output
//			 [AVE_/DIF_][POWER/TEMP]_CUR_[M/F] -> output power/temperature for each timestep
//			 [AVE_/DIF_][POWER/TEMP]_MAX_[M/F] -> output max power/temperature over each sample duration
//			 [AVE_/DIF_][POWER/TEMP]_MIN_[M/F] -> output min power/temperature over the sample duration
//			 [AVE_/DIF_][POWER/TEMP]_AVE_[M/F] -> output average power/temperature for sample duration
enum {
  GFX_LAYER_FLOORPLAN,
  GFX_LAYER_NORMAL,
  GFX_LAYER_AVE,
  GFX_LAYER_DIF
};                                           // nothing, compute average, compute difference across layers
enum { GFX_MUNIT, GFX_FUNIT };               // using model units/ using functional unit
enum { GFX_POWER, GFX_TEMP };                // use power values / use temperature values
enum { GFX_CUR, GFX_MAX, GFX_MIN, GFX_AVE }; // use cur for each timestep/ [max/min/ave] values for each sample

#define PINS_DYNARRAY_MINSIZE 200
#define PWB_DYNARRAY_MINSIZE 200
#define FCPBGA_DYNARRAY_MINSIZE 200
#define CHIP_DYNARRAY_MINSIZE 200
#define UCOOL_DYNARRAY_MINSIZE 200
#define HEAT_SPREADER_DYNARRAY_MINSIZE 200
#define HEAT_SINK_DYNARRAY_MINSIZE 200
#define HEAT_SINK_FINS_DYNARRAY_MINSIZE 200
#define UNSOLVED_MATRIX_DYNARRAY_MINSIZE 4000
enum {
  MAINBOARD_LAYER,
  PINS_LAYER,
  PWB_LAYER,
  FCPBGA_LAYER,
  C4_UNDERFILL_LAYER,
  INTERCONNECT_LAYER,
  DIE_TRANSISTOR_LAYER,
  BULK_SI_LAYER,
  OIL_LAYER,
  AIR_LAYER,
  UCOOL_LAYER,
  HEAT_SPREADER_LAYER,
  HEAT_SINK_LAYER,
  HEAT_SINK_FINS_LAYER
};
enum { BASIC_EQUATION, UCOOL_COLD_EQUATION, UCOOL_HOT_EQUATION, UCOOL_INNER_LAYER_EQUATION };

// parameters to calculate polysilicon thermal conductivity at 300K
#define PHOTON_GROUP_VELOCITY 6166   // (m/s)
#define PHOTON_SPECIFIC_HEAT 1.654e6 // (J/m^3*K)

#define TECH_PROCESS 0
#define SRAM_CELL TECH_PROCESS + 1
#define TECH_VDD SRAM_CELL + 1
#define TECH_GATE_L_AVE TECH_VDD + 1
#define TECH_GATE_PMOS_W_AVE TECH_GATE_L_AVE + 1
#define TECH_GATE_NMOS_W_AVE TECH_GATE_PMOS_W_AVE + 1
#define TECH_LEFF TECH_GATE_NMOS_W_AVE + 1
#define TECH_TOX TECH_LEFF + 1
#define TECH_LEVELS TECH_TOX + 1
#define TECH_K TECH_LEVELS + 1
#define TECH_POLY 10
#define TECH_METAL1 TECH_POLY + 9
#define TECH_METAL2 TECH_METAL1 + 9
#define TECH_METAL3 TECH_METAL2 + 9
#define TECH_METAL4 TECH_METAL3 + 9
#define TECH_METAL5 TECH_METAL4 + 9
#define TECH_METAL6 TECH_METAL5 + 9
#define TECH_METAL7 TECH_METAL6 + 9
#define TECH_METAL8 TECH_METAL7 + 9
//#define TECH_METAL9		78 FIXME: Metal 9 data is completely wrong here (45nm process)

enum { TECH_H, TECH_W, TECH_SPACE, TECH_RESIST, TECH_CAP, TECH_SOPT, TECH_TINS, TECH_VIADIST, TECH_NUMWIRES };

// These are used for the effective RC computation
enum { RC_CHARGING, RC_DISCHARGING };

// eka elnaz: added the class for scaling subth
// leakage based on temperature
class LeakageCoefs {
public:
  float a;
  float b;
  float c;
  float offset;

  LeakageCoefs(){};
  LeakageCoefs(float ta, float tb, float tc, float toffset)
      : a(ta)
      , b(tb)
      , c(tc)
      , offset(toffset) {
  }
  // Assigning proper coefficient for calculating
  // leakage based on the device type (HP(0), LSTP(1), LP(2))
  void set(const LeakageCoefs &coefs) {
    a      = coefs.a;
    b      = coefs.b;
    c      = coefs.c;
    offset = coefs.offset;
  }
  void set(float ta, float tb, float tc, float toffset) {
    a      = ta;
    b      = tb;
    c      = tc;
    offset = toffset;
  }
};

#endif
