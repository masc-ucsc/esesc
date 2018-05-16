//  Contributed by Ehsan K.Ardestani
//                 Ian Lee
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
File name:      SescTherm.h
Classes:        SescTherm
********************************************************************************/

#ifndef SESC_THERM_H
#define SESC_THERM_H

#define SYNTHETIC_TEST 0
#define FPGA 1
#define AMD 2
//#define METRICS_AT_SIMPOINTS 1

#include "ThermTrace.h"
#include "sesctherm3Ddefine.h"
#include "sescthermMacro.h"

#include "ChipMaterial.h"
#include "ClassDeclare.h"
#include "DataLibrary.h"
#include "Metrics.h"
#include "ThermModel.h"

#include "Bundle.h"

// eka TODO:
// 1- make the class definition cleaner in terms of
// public and private variables and methods
// 3- Clean for stand alone version vs. integrated one
class SescTherm {
public:
  const char *conf_file;
  const char *input_file;
  const char *flp_file;
  const char *lkg_file;
  const char *devt_file;
  const char *output_file;
  const char *label_file;
  const char *label_match;
  const char *label_path;
  bool        option_cooldown;

  FILE *logtemp;
#ifdef METRICS_AT_SIMPOINTS
  FILE *              spfile; // simpoints
  const char *        sp_section;
  bool                exact_match;
  std::vector<double> spweight;
  double              weight;
#endif

  const char *             tfilename;
  std::vector<MATRIX_DATA> max_power;
  const char *             model;
  bool                     useRK4;
  ThermModel               temp_model;
  MATRIX_DATA              initialTemp;
  MATRIX_DATA              ambientTemp;
  int                      cyclesPerSample;
  MATRIX_DATA              frequency;
  int                      power_samples_per_therm_sample;
  MATRIX_DATA              power_timestep;
  MATRIX_DATA              therm_timestep;
  const char *             thermal;
  int                      n_layers_start;
  int                      n_layers_end;
  int                      transistor_layer;
  ThermTrace               trace;
  Metrics *                metrics;
  MATRIX_DATA              throttleCycles;
  MATRIX_DATA              throttleTime;
  uint64_t                 throttleLength; // how many times called in a row
  MATRIX_DATA              thermalThrottle;
  MATRIX_DATA              elapsed;
  // eka, I have to figure out what else should be defined here

  // elnaz, added scaling leakage based on temperature
  // std::vector <float> *lkgCntrValues; //eka: FIXME: move to ThermTrace
  // std::vector <uint32_t> *devTypes; //device types specificed in OOO.xml in mcpat

  SescTherm(int argc, const char **argv);
  SescTherm();
  ~SescTherm();
  void     showUsage();
  void     set_parameters(int argc, char **argv);
  uint32_t cooldown(ThermModel &temp_model, int transistor_layer);

  int process_trace(ThermModel &temp_model, MATRIX_DATA power_timestep, int transistor_layer, MATRIX_DATA initialTemp,
                    int power_samples_per_therm_sample, bool rabbit);

  int      process_trace(const char *input_file, ThermModel &temp_model, MATRIX_DATA power_timestep, int transistor_layer,
                         MATRIX_DATA initialTemp, int power_samples_per_therm_sample, bool rabbit, int cyclesPerSample, FILE *logLeakage,
                         FILE *logDensity);
  void     configure(ChipEnergyBundle *energyCntrNames);
  void     configure(std::vector<std::string> *energyCntrNames, std::vector<float> *leakageCntrValues,
                     std::vector<uint32_t> *deviceTypes);
  int      computeTemp(ChipEnergyBundle *energyBundle, std::vector<float> *temperatures, uint64_t timeinterval);
  void     dumpBlockT2StrT(std::vector<float> *strTemp);
  void     computeTemp(int argc, const char **argv);
  void     dumpTemp(std::vector<MATRIX_DATA> *flp_temperature_map);
  uint32_t get_throttleLength() {
    return throttleLength;
  }
  void  updateMetrics(uint64_t timeinterval = 100000);
  char *replace(char *st, char *orig, char *repl);
};
#endif
