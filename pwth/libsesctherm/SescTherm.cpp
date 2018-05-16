//  Contributed by Ehsan K.Ardestani
//                 Ian Lee
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
File name:      SescTherm.cpp
Description:    Sample driver for SescTherm Library
********************************************************************************/

#include "SescTherm.h"
#include "Report.h"
#include "SescConf.h"
#include "nanassert.h"
#include <complex>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
SescTherm::SescTherm() {
  conf_file       = NULL;
  input_file      = NULL;
  flp_file        = NULL;
  output_file     = NULL;
  label_file      = NULL;
  label_match     = NULL;
  label_path      = ".";
  option_cooldown = false;
  lkg_file        = NULL;
  devt_file       = NULL;
}

uint32_t SescTherm::cooldown(ThermModel &temp_model, int transistor_layer) {
  if(!option_cooldown)
    return 0;

  for(size_t j = 0; j < max_power.size(); j++) {
    temp_model.set_power_flp(j, transistor_layer, max_power[j] / 50); // 5% of the max power per block
  }

  std::vector<MATRIX_DATA> flp_temperature_map;
  uint32_t                 d = 0;
  do {
    d++;
    option_cooldown = false;
    // 1 second cooldown
    // for (int j = 0; j < 400; j++) {
    // temp_model.compute (0.005, false);
    temp_model.compute(throttleTime, false);
    flp_temperature_map.clear();
    flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
    for(size_t i = 0; i < flp_temperature_map.size(); i++) {
      if(flp_temperature_map[i] > thermalThrottle - 3) {
        // std::cout << " ThrottleTime: "<<throttleTime/2<< "temp: "<<flp_temperature_map[i]<<"\n";
        option_cooldown = true;
        break;
      }
    }
  } while(option_cooldown);

  // std::cout << "C " << temp_model.get_time () << " ";
  for(unsigned int i = 0; i < flp_temperature_map.size(); i++) {
    // std::cout << flp_temperature_map[i] << "\t";
  }
  // std::cout << std::endl;
  return d;
  //}
}

int SescTherm::process_trace(ThermModel &temp_model, MATRIX_DATA power_timestep, int transistor_layer, MATRIX_DATA initialTemp,
                             int power_samples_per_therm_sample, bool rabbit) {
  static std::vector<MATRIX_DATA> power(trace.get_energy_size());
  std::vector<MATRIX_DATA>        flp_temperature_map;
  bool                            throttle = true;
  option_cooldown                          = false;

  while(throttle) {
    flp_temperature_map.clear();
    flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
    for(size_t i = 0; i < flp_temperature_map.size(); i++) {
      if(flp_temperature_map[i] > thermalThrottle) {
        option_cooldown = true;
        break;
      }
    }

    if(option_cooldown) {
      throttleLength += cooldown(temp_model, transistor_layer);
    } else {
      throttle = false;
    }

    option_cooldown = false;
  }

  while(trace.read_energy(false)) {
    for(size_t j = 0; j < trace.get_energy_size(); j++) {
      power[j] = trace.get_energy(j);
    }

    for(size_t j = 0; j < trace.get_energy_size(); j++) {
      if(max_power[j] < power[j])
        max_power[j] = power[j];
      temp_model.set_power_flp(j, transistor_layer, power[j]);
      power[j] = 0.0;
    }

    MATRIX_DATA ts              = power_timestep;
    static int  each_ten_update = -1;
    if(each_ten_update < 0) {
      temp_model.compute(ts, true);
      each_ten_update = 10;
    } else {
      temp_model.compute(ts, false);
      each_ten_update--;
    }
  }

  // DUMP_SVG
  temp_model.print_graphics();
  return 0;
}

void SescTherm::configure(ChipEnergyBundle *energyBundle) {
  clock_t start, end;
  elapsed = 0;

  trace.loadLkgPtrs(energyBundle);

  std::cout << "STARTING SESCTHERM" << std::endl;

  model = SescConf->getCharPtr("thermal", "model");

  trace.dumppwth   = 0;
  trace.pwrsection = (char *)SescConf->getCharPtr("", "pwrmodel", 0);
  trace.dumppwth   = SescConf->getBool(trace.pwrsection, "dumpPower", 0);
  char *fname      = (char *)malloc(1023);
  sprintf(fname, "temp_%s", Report::getNameID());
  logtemp = fopen(fname, "w");
  GMSG(logtemp == 0, "ERROR: could not open logtemp file \"%s\" (ignoring it)", fname);
  trace.initDumpLeakage();
  start  = clock();
  useRK4 = SescConf->getBool(model, "useRK4");
  temp_model.ThermModelInit(flp_file, 0, useRK4, output_file, true, false);
  end = clock();

  std::cout << "MODEL INITIALIZED" << std::endl;

  elapsed = ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;
  std::cout << "MODEL INITIALIZATION TIME IS " << elapsed << "SECONDS" << std::endl;

  std::cout << "MAX MODEL TIMESTEP:" << temp_model.get_max_timestep() << std::endl;
  std::cout << "RECOMMENDED MODEL TIMESTEP: " << temp_model.get_recommended_timestep() << std::endl;

  temp_model.datalibrary_->set_timestep(temp_model.get_recommended_timestep());

  initialTemp = SescConf->getDouble(model, "initialTemp"); // sesctherm works in K (not C)
  ambientTemp = SescConf->getDouble(model, "ambientTemp");

  cyclesPerSample                = static_cast<int>(SescConf->getDouble(trace.pwrsection, "updateInterval"));
  frequency                      = SescConf->getDouble("technology", "Frequency");
  throttleCycles                 = cyclesPerSample * SescConf->getDouble(trace.pwrsection, "throttleCycleRatio");
  thermalThrottle                = SescConf->getDouble(trace.pwrsection, "thermalThrottle");
  power_samples_per_therm_sample = SescConf->getInt(model, "PowerSamplesPerThermSample");
  power_timestep                 = cyclesPerSample / frequency;
  therm_timestep                 = power_timestep * power_samples_per_therm_sample;
  throttleTime                   = throttleCycles / frequency;
  std::cout << "USED THERM TIMESTEP: " << therm_timestep << " POWER TIMESTEP: " << power_timestep << std::endl;
  std::cout << "USED power_samples_per_therm_sample: " << power_samples_per_therm_sample << std::endl;

  thermal          = SescConf->getCharPtr("", "thermal");
  n_layers_start   = SescConf->getRecordMin(thermal, "layer");
  n_layers_end     = SescConf->getRecordMax(thermal, "layer"); // last layer has ambient
  transistor_layer = -1;
  for(int i = n_layers_start; i <= n_layers_end; i++) {
    const char *layer_name = SescConf->getCharPtr(thermal, "layer", i);
    const char *layer_sec  = SescConf->getCharPtr(layer_name, "type");

    std::cerr << "Looking for layers " << layer_name << " type " << layer_sec;
    if(strcmp(layer_sec, "die_transistor") == 0) {
      transistor_layer = i;
      if(SescConf->getInt(layer_name, "lock_temp") == -1) {
        temp_model.set_temperature_layer(i, initialTemp);
        std::cerr << " Setting temperature " << initialTemp << " LOCKED" << std::endl;
      }
    } else if(SescConf->getInt(layer_name, "lock_temp") == -1) {
      temp_model.set_temperature_layer(i, ambientTemp);
      std::cerr << " Setting temperature " << ambientTemp << " LOCKED" << std::endl;
    } else {
      double t = -SescConf->getDouble(layer_name, "lock_temp");
      if(t < -1)
        t = -t;
      std::cerr << " Setting temperature " << t << std::endl;
      temp_model.set_temperature_layer(i, t);
    }
  }
  // temp_model.set_temperature_layer(n_layers_end,ambientTemp); FIXME: this crashes n_layers_end == 8

  if(transistor_layer == -1) {
    for(int i = n_layers_start; i <= n_layers_end; i++) {
      const char *layer_name = SescConf->getCharPtr(thermal, "layer", i);
      const char *layer_sec  = SescConf->getCharPtr(layer_name, "type");
      if(strcmp(layer_sec, "bulk_silicon") == 0)
        transistor_layer = i;
    }
    if(transistor_layer == -1) {
      std::cerr << "ERROR: Could not find 'die_transistor' layer" << std::endl;
      exit(-2);
    }
  }

  std::cerr << "USING FLOORPLAN WITHIN LAYER:" << transistor_layer << std::endl;

  int n_blocks = 0;
  for(int i = n_layers_start; i <= n_layers_end; i++)
    n_blocks += temp_model.get_modelunit_count(i);

  std::cout << "Modeling " << n_blocks << " blocks" << std::endl;
  max_power.resize(temp_model.get_temperature_layer_map(transistor_layer, transistor_layer).size());

  // eka, initialize trace with mapping between energyCntrNames
  // and the blocks in floorplaning
  trace.ThermTraceInit();
  if(!trace.is_ready())
    exit(0); // trace file did not exist

  trace.BlockT2StrcTMap();
  // init Metrics
  metrics = new Metrics(trace.get_energy_size());
  trace.loadLkgCoefs();

  printf("initializing throttlingLength\n");
  throttleLength = 0;
}

// eka, this is for the esesc integrated version, as opposed to
// stand alone version
int SescTherm::computeTemp(ChipEnergyBundle *energyBundle, std::vector<float> *temperatures, uint64_t timeinterval) {
  clock_t start, end;
  // elapsed = 0;
  int return_signal = 0;

  // adjust for timeinterval
  power_timestep = static_cast<MATRIX_DATA>(timeinterval / 1.0e9);
  // throttlingcycles is based on original interval

  start = clock();
  trace.loadValues(energyBundle, temperatures);
  return_signal = process_trace(temp_model, power_timestep, transistor_layer, initialTemp, power_samples_per_therm_sample, false);
  end           = clock();
  elapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;

  // eka, dump block temperature
  std::vector<MATRIX_DATA> flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
  trace.BlockT2StrcT(&flp_temperature_map, temperatures);

  cooldown(temp_model, transistor_layer);

  return return_signal;
}

void SescTherm::dumpTemp(std::vector<MATRIX_DATA> *flp_temperature_map) {
  if(logtemp)
    fprintf(logtemp, "T\t%2.7lf\t", temp_model.get_time());

  for(unsigned int i = 0; i < (*flp_temperature_map).size(); i++) {
    if(logtemp)
      fprintf(logtemp, "%f\t", (*flp_temperature_map)[i]);
  }

  if(logtemp)
    fprintf(logtemp, "\n");
  fflush(logtemp);
}

SescTherm::~SescTherm() {
  if(logtemp)
    fclose(logtemp);
}

void SescTherm::updateMetrics(uint64_t timeinterval) {
  std::vector<MATRIX_DATA> flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
  metrics->updateAllMetrics(const_cast<const std::vector<MATRIX_DATA> *>(&flp_temperature_map), timeinterval, &trace,
                            throttleLength, elapsed, temp_model.get_time());
  dumpTemp(&flp_temperature_map);
}
