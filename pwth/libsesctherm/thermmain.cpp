//  Contributed by Ehsan K. Ardestani
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
File name:      thermmain.cpp
Description:    This file is for the standalone execution of SescTherm
********************************************************************************/
#include "Report.h"
#include "SescConf.h"
#include "SescTherm.h"
#include "ThermTrace.h"
#include <complex>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>

static char *reportFile;
void         ThermTrace::read_sesc_variable(const char *input_file) {

  std::string tmp_str = "";
  // READ first line of input_file. It has the energies reported by sesc

  input_fd_ = fopen(input_file, "r");
  GMSG(input_fd_ == 0, "ERROR: could not open the input file \"%s\" (ignoring it)", input_file);

  char        buffer = 0;
  bool        eof    = false;
  std::string line;
  while(!eof && buffer != '\n') {
    int32_t s = fread(&buffer, 1, 1, input_fd_);
    if(s == 0)
      eof = true;
    else
      line += buffer;
  }
  if(eof) {
    MSG("Error: Input file %s has invalid format", input_file);
    exit(2);
  }

  TokenVectorType sesc_variable;
  tokenize(line.c_str(), sesc_variable);
  if(sesc_variable.empty()) {
    MSG("Error: Input file %s does not have input variable", input_file);
    exit(2);
  }
  if(!energyCntrNames_)
    free(energyCntrNames_);
  energyCntrNames_ = new std::vector<std::string>(sesc_variable.size());
  mapping.resize(sesc_variable.size());
  for(size_t j = 0; j < sesc_variable.size(); j++) {
    LOG("variable[%d]=[%s] ", (int)j, sesc_variable[j]);
    mapping[j].name = strdup(sesc_variable[j]);
    tmp_str         = strdup(sesc_variable[j]);
    (energyCntrNames_->at(j)).assign(tmp_str.c_str());
  }
}

void ThermTrace::ThermTraceInit(const char *input_file) {
  //: input_file_(strdup(input_file?input_file:"")) {

  input_file_ = strdup(input_file ? input_file : "");
  hpThLeakageCoefs.set(-122.292, 19055.107, 1.216, 0.18);
  lstpThLeakageCoefs.set(19.5294, -5209.1702, 0.9928, -0.49);
  lpThLeakageCoefs.set(8.4025, -2213.1390, 0.9966, -0.23);

  input_fd_ = NULL;
  if(input_file) {
    read_sesc_variable(input_file);
  }

  read_floorplan_mapping();
  lkgCntrValues_       = new std::vector<float>(mapping.size());
  devTypes             = new std::vector<uint32_t>(mapping.size());
  scaledLkgCntrValues_ = new std::vector<float>();
  energyCntrValues_    = new std::vector<std::vector<float>>(1);
  energyCntrValues_->at(0).resize(mapping.size());
  totalPowerSamples = 0;
  loadLkgCoefs();
  nFastForward       = static_cast<int>(SescConf->getDouble(pwrsection, "nFastForward"));
  actualnFastForward = nFastForward;
  // to remain 1, do not change
  set_nFastForwardCoeff(1);
}

void ThermTrace::ThermTraceInit_light(const char *input_file) {
  //: input_file_(strdup(input_file?input_file:"")) {

  input_file_ = strdup(input_file ? input_file : "");
  hpThLeakageCoefs.set(-122.292, 19055.107, 1.216, 0.18);
  lstpThLeakageCoefs.set(19.5294, -5209.1702, 0.9928, -0.49);
  lpThLeakageCoefs.set(8.4025, -2213.1390, 0.9966, -0.23);

  if(input_file) {
    // Ignoring the title
    // read_sesc_variable(input_file);
    input_fd_ = fopen(input_file, "r");
    GMSG(input_fd_ == 0, "ERROR: could not open the input file \"%s\" (ignoring it)", input_file);

    char        buffer = 0;
    bool        eof    = false;
    std::string line;
    while(!eof && buffer != '\n') {
      fread(&buffer, 1, 1, input_fd_);
    }
    if(eof) {
      MSG("Error: Input file %s has invalid format", input_file);
      exit(2);
    }
  }

  totalPowerSamples = 0;
}

void ThermTrace::read_lkg(const char *lkg_file, const char *devt_file) {
  FILE *lkg_fd = fopen(lkg_file, "r");
  GMSG(lkg_fd == 0, "ERROR: could not open leakage file \"%s\" (ignoring it)", lkg_file);
  FILE *devt_fd = fopen(devt_file, "r");
  GMSG(devt_fd == 0, "ERROR: could not open devicetype file \"%s\" (ignoring it)", devt_file);
  for(size_t i = 0; i < mapping.size(); i++)
    if(EOF == fscanf(lkg_fd, "%e", &((*lkgCntrValues_)[i])))
      return;
    else
      fscanf(devt_fd, "%d", &((*devTypes)[i]));
  fclose(lkg_fd);
  fclose(devt_fd);
}

// elnaz:: scaling subth leakage based on temperature
void ThermTrace::leakage(std::vector<double> *Temperatures) {
  static std::vector<float> strTemps(energyCntrNames_->size());
  // strTemps = new std::vector <float> ();
  scaledLkgCntrValues_->clear();
  BlockT2StrcT(Temperatures, &strTemps);
  for(size_t i = 0; i < lkgCntrValues_->size(); i++) {
    scaledLkgCntrValues_->push_back(scaleLeakage(strTemps[i], lkgCntrValues_->at(i), devTypes->at(i)));
  }
  return;
}

void ThermTrace::dumpLeakage2(FILE *logLeakage, FILE *logDensity) {

  if(dumppwth) {
    for(size_t i = 0; i < scaledLkgCntrValues_->size(); i++) {
      fprintf(logLeakage, "%e\t", scaledLkgCntrValues_->at(i));
    }
    /*for (size_t i=0; i< flp.size(); i++){
      fprintf(logLeakage, "%e\t", flp[i]->energy);
      fprintf(logDensity, "%e\t", flp[i]->energy/flp[i]->area/10000);
      printf("%e\t", flp[i]->area/10000);
      } */
    fprintf(logLeakage, "\n");
    // fprintf(logDensity, "\n");
    fflush(logLeakage);
    // fflush(logDensity);
  }
  return;
}

SescTherm::SescTherm(int argc, const char **argv) {
  conf_file   = NULL;
  input_file  = NULL;
  output_file = NULL;
  label_file  = NULL;
  label_match = NULL;
  lkg_file    = NULL;
  label_path  = ".";
#ifdef METRICS_AT_SIMPOINTS
  sp_section = NULL;
#endif
  option_cooldown = false;

  set_parameters(argc, const_cast<char **>(argv));
}

void SescTherm::showUsage() {
  printf("sesctherm:\n");
  printf("\t-i <infile>             - The therm file for which you want temperature data\n");
  printf("\t-o <outfile>(optional)  - Output file name for either steady state or transient temperature\n");
  printf("\t-c <power.conf>         - This is the mappings from floorplan variables to sesc variables\n");
  printf("\t-s <sim_point.labels>   - Simpoint labels file\n");
  printf("\t-t <starting_name>      - Simpoint therm trace starting filename. E.g: crafty_X\n");
  printf("\t-p <label path>         - Path to simpoint therm traces\n");
  printf("\t-l <leakage file>       - the file containing original leakage values\n");
  printf("\t-d <deviceType file>    - the file containing device types for each leakage entry\n");
#ifdef METRICS_AT_SIMPOINTS
  printf("\t-m <config section>     - the section of config file containing simpoints and weights.\n");
#endif
  printf("\t-k                      - Perform chip cooldown iterations\n");
}

void SescTherm::set_parameters(int argc, char **argv) {
  int opt;
#ifdef METRICS_AT_SIMPOINTS
  while((opt = getopt(argc, argv, "kc:f:i:o:s:t:p:m:l:d:")) != -1) {
#else
  while((opt = getopt(argc, argv, "kc:f:i:o:s:t:p:l:d:")) != -1) {
#endif
    switch(opt) {
    case 'c':
      conf_file = optarg;
      break;
    case 'f':
      flp_file = optarg;
      break;
    case 'i':
      input_file = optarg;
      break;
    case 'o':
      output_file = optarg;
      break;
    case 's':
      label_file = optarg;
      break;
    case 't':
      label_match = optarg;
      break;
    case 'p':
      label_path = optarg;
      break;
#ifdef METRICS_AT_SIMPOINTS
    case 'm':
      sp_section = optarg;
      break;
#endif
    case 'k':
      option_cooldown = true;
      break;
    case 'l':
      lkg_file = optarg;
      break;
    case 'd':
      devt_file = optarg;
      break;
    default:
      showUsage();
      exit(1);
      break;
    }
  }
  if((conf_file == NULL || input_file == 0) && label_file == 0) {
    showUsage();
    exit(1);
  }
  //  if ((conf_file == NULL || input_file[0] == 0) && label_file == 0) {
  //    showUsage ();
  //    exit (1);
  //  }

  if(label_file && !label_match) {
    std::cerr << "ERROR: you should specify -t matching name when simpoints are active\n";
    showUsage();
    exit(2);
  }

  // eka, any other parameters to set?
}

int SescTherm::process_trace(const char *input_files, ThermModel &temp_model, MATRIX_DATA power_timestep, int transistor_layer,
                             MATRIX_DATA initialTemp, int power_samples_per_therm_sample, bool rabbit, int cyclesPerSample,
                             FILE *logLeakage, FILE *logDensity) {
  int                num_computations = 0;
  clock_t            start, end;
  static MATRIX_DATA sescThermElapsed = 0;

  // ----------------------------------------------------------------
  // READ trace
  //  ThermTrace trace(input_files);
  trace.ThermTraceInit_light(input_files); // we miss one power sample here at the first call
  if(!trace.is_ready())
    return 0; // trace file did not exist

  trace.read_lkg(lkg_file, devt_file);
  trace.loadLkgCoefs();
  // TODO: scale leakage, add to dyn power in cadena, save in another file,
  // pass it to process_trace
  // scale leackage

  static std::vector<MATRIX_DATA> power(trace.get_energy_size());

  std::cout << std::setiosflags(std::ios::fixed);

  static std::vector<MATRIX_DATA> flp_temperature_map;
  flp_temperature_map.clear();
  flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
  trace.leakage(const_cast<std::vector<double> *>(&flp_temperature_map));
  trace.dumpLeakage2(logLeakage, logDensity);

  // Get filename for cycle info
  char *perf_file = replace(const_cast<char *>(input_files), "power_dyn", "perf");
  printf("perf file:%s\n", perf_file);
  FILE *perff = fopen(perf_file, "r");
  if(perff == NULL) {
    printf("Cannot open perf file: %s", perf_file);
    exit(1);
  }

  char buffer = 0;
  bool eof    = false;
  while(!eof && buffer != '\n') {
    fread(&buffer, 1, 1, perff);
  }

  while(trace.read_energy(true)) {
    start = clock();
    if(temp_model.get_time() > 90.0) {
      std::cerr << "Timeout: Do not model over 90secs of execution (boring)" << std::endl;
      fclose(perff);
      return 0;
    }

    // update power_timestep for the sample
#define PERF_COLS 6
    float    buffer[PERF_COLS];
    uint64_t Cycles = 0;
    uint64_t Cnt    = 0;
    // initialize buffer
    for(size_t i = 0; i < PERF_COLS; i++)
      buffer[i] = 0;
    // read nFastForward number of samples
    size_t j;
    bool   moreSmpls = true;
    for(j = 0; j < trace.get_nFastForward(); j++) {
      for(size_t i = 0; i < PERF_COLS; i++) {
        if(EOF == fscanf(perff, "%e", &buffer[i]))
          moreSmpls = false;

        if(!moreSmpls)
          break;

        // printf("buf[%d] = %e\t", i, buffer[i]);
        if(i == 2) {
          Cycles += buffer[2];
          Cnt++;
        }
      }
      if(!moreSmpls)
        break;
    }
    power_timestep = Cycles / frequency;
    //////////////////////////////////////////////

#if 1 // eka, Check the thermalThrottle and cooldown if necessary
    {

      bool throttle   = true;
      option_cooldown = false;
      while(throttle) {
        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        for(size_t i = 0; i < flp_temperature_map.size(); i++) {
          if(flp_temperature_map[i] > thermalThrottle) {
            option_cooldown = true;
            break;
          }
        }

        if(option_cooldown) {
          cooldown(temp_model, transistor_layer);
          throttleLength++;
        } else
          throttle = false;

        option_cooldown = false;
      }
    }
#endif
    //-------------------------------------
    // Advance Power
    for(size_t j = 0; j < trace.get_energy_size(); j++) {
      power[j] = trace.get_energy(j);
    }
#if 0
    std::cout << "P " << temp_model.get_time () << " ";
    MATRIX_DATA total = 0;
    MATRIX_DATA p;
    for (size_t j = 0; j < power.size (); j++) {
      p = power[j]
      std::cout << p << "\t";
      total += p;
    }
    std::cout << "\t:\t" << total << std::endl;
    std::cout << ": " << samples << std::endl;
#endif

    for(size_t j = 0; j < trace.get_energy_size(); j++) {
      if(max_power[j] < power[j])
        max_power[j] = power[j];
      temp_model.set_power_flp(j, transistor_layer, power[j]);
    }

    //-------------------------------------
    // Advance Thermal

    MATRIX_DATA ts = power_timestep;
    // printf("ts: %f, pts: %f, nFF: %d \n", ts, power_timestep*frequency, trace.get_nFastForward());
    double     timeinterval    = ts * frequency;
    static int each_ten_update = -1;
    if(each_ten_update < 0) {
      temp_model.compute(ts, true);
      each_ten_update = 10;
    } else {
      temp_model.compute(ts, false);
      each_ten_update--;
    }

    num_computations++;
    //-------------
    static MATRIX_DATA print_at = 0;
    if(temp_model.get_time() > print_at) { // eka, TEMPORARY
                                           // print_at = temp_model.get_time () + 0.0025;	// every 250us
                                           // print_at = temp_model.get_time () ;
#ifdef DUMP_SVG
      temp_model.print_graphics();
#endif
#if 0
      static int conta = 0;
      conta++;
      if (conta == 9) {
        temp_model.print_graphics();
        conta = 0;
      }
#endif
      flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);

      for(int i = 0; i < Cnt; i++) { // to sync for rms, if there is FF
        if(rabbit) {
          std::cout << "R " << temp_model.get_time() << " ";
          if(logtemp)
            fprintf(logtemp, "R\t%f\t", temp_model.get_time());

        } else {
          std::cout << std::setprecision(6) << "T " << temp_model.get_time() << " ";
          if(logtemp)
            fprintf(logtemp, "T\t%f\t", temp_model.get_time());
        }

        if(logtemp)
          // fprintf(logtemp, "S\t%d\t", flp_temperature_map.size());
          for(unsigned int i = 0; i < flp_temperature_map.size(); i++) {
            std::cout << std::setprecision(3) << flp_temperature_map[i] << " ";
            if(logtemp)
              fprintf(logtemp, "%f\t", flp_temperature_map[i]);
          }
        std::cout << std::endl;
        if(logtemp) {
          fprintf(logtemp, "\n");
          fflush(logtemp);
        }
      }
    }

    char wait = 0;
    if(wait >= 10.0 / static_cast<float>(trace.get_nFastForward())) {
      trace.leakage(const_cast<std::vector<double> *>(&flp_temperature_map));
      wait = 0;
    } else
      wait++;

    end = clock();
    sescThermElapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;
    trace.dumpLeakage2(logLeakage, logDensity);

#ifdef METRICS_AT_SIMPOINTS
    // printf ("weight: %f", weight);
    if(exact_match) {
      // std::cin.get();
      for(int i = 0; i < Cnt; i++)
        metrics->updateAllMetrics(const_cast<const std::vector<MATRIX_DATA> *>(&flp_temperature_map), weight * (timeinterval / Cnt),
                                  &trace, throttleLength, sescThermElapsed, temp_model.get_time());
    }
#else
    for(int i = 0; i < Cnt; i++)
      metrics->updateAllMetrics(const_cast<const std::vector<MATRIX_DATA> *>(&flp_temperature_map), timeinterval / Cnt, &trace,
                                throttleLength, sescThermElapsed, temp_model.get_time());
#endif
  }

  fclose(perff);
  fclose(trace.input_fd_);
  return num_computations;
}

void SescTherm::computeTemp(int argc, const char **argv) {
  clock_t     start, end;
  MATRIX_DATA elapsed = 0;

  // eka, set_parameter in the class constructor will do that
  // read_parameters (argc, const_cast<char **>(argv));

  SescConf = new SConfig(argc, argv);
  if(SescConf == 0) {
    std::cout << "Cannot open configuration file" << std::endl;
    exit(1);
  }

  const char *tmp;
  if(getenv("REPORTFILE")) {
    tmp = strdup(getenv("REPORTFILE"));
  } else {
    tmp = SescConf->getCharPtr("", "reportFile", 0);
  }
  reportFile = (char *)malloc(30 + strlen(tmp));
  sprintf(reportFile, "esesc_%s.XXXXXX", tmp);
  Report::openFile(reportFile);
  SescConf->getDouble("technology", "frequency"); // Just read it to get it in the dump
  std::cout << "STARTING SESCTHERM" << std::endl;

  const char *model = SescConf->getCharPtr("thermal", "model");

  start       = clock();
  bool useRK4 = SescConf->getBool(model, "useRK4");

  ThermModel temp_model(flp_file, 0, useRK4, output_file, true, false);
  end = clock();

  std::cout << "MODEL INITIALIZED" << std::endl;

  elapsed = ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;
  std::cout << "MODEL INITIALIZATION TIME IS " << elapsed << "SECONDS" << std::endl;

  std::cout << "MAX MODEL TIMESTEP:" << temp_model.get_max_timestep() << std::endl;
  std::cout << "RECOMMENDED MODEL TIMESTEP: " << temp_model.get_recommended_timestep() << std::endl;

  temp_model.datalibrary_->set_timestep(temp_model.get_recommended_timestep());

  // this is a map of maps
  // it stores the time index of the run, and the corresponding temperature map,
  // where flp_temperature_map[1.0][0] returns the temperature for flp unit 0 at timestep 1.0

  MATRIX_DATA initialTemp = SescConf->getDouble(model, "initialTemp"); // sesctherm works in K (not C)
  MATRIX_DATA ambientTemp = SescConf->getDouble(model, "ambientTemp");

  // eka
  FILE *logLeakage;
  FILE *logDensity;

  logLeakage = NULL;
  logDensity = NULL;

#ifdef METRICS_AT_SIMPOINTS
  spfile                = NULL;
  char *spname          = (char *)malloc(1023);
  char *p               = (char *)malloc(1023);
  char *s               = (char *)malloc(1023);
  char *label_name      = (char *)malloc(1023);
  char *label_dir       = (char *)malloc(1023);
  char *bname           = (char *)malloc(1023);
  char *tmp_label_match = (char *)malloc(1023);
  char *tmp_label_file  = (char *)malloc(1023);
  int   spbuffer[10]    = {0};
  int   n               = 0;
  int   i               = 0;
  int   col1            = 0;
  int   col2            = 0;
  weight                = 0.0;
#endif
  trace.dumppwth   = 0;
  trace.pwrsection = (char *)SescConf->getCharPtr("", "pwrmodel", 0);
  // pwschult
  int cyclesPerSample = static_cast<int>(SescConf->getDouble(trace.pwrsection, "updateInterval"));
  trace.dumppwth      = SescConf->getBool(trace.pwrsection, "dumpPower", 0);
  char *fname         = (char *)malloc(1023);

  sprintf(fname, "temp_sa_%s", Report::getNameID());
  logtemp = fopen(fname, "w");
  GMSG(logtemp == 0, "ERROR: could not open logtemp file \"%s\" (ignoring it)", fname);
  if(trace.dumppwth) {

    sprintf(fname, "scaled_lkg_sa_%s", Report::getNameID());
    logLeakage = fopen(fname, "w");
    GMSG(logLeakage == 0, "ERROR: could not open logLeakage file \"%s\" (ignoring it)", fname);

    sprintf(fname, "density_%s", Report::getNameID());
    logDensity = fopen(fname, "w");
    GMSG(logDensity == 0, "ERROR: could not open logDensity file \"%s\" (ignoring it)", fname);
  }

#ifdef METRICS_AT_SIMPOINTS
  strcpy(tmp_label_match, (char *)label_match);
  strcpy(tmp_label_file, (char *)label_file);

  char *save = 0;

  p = strtok_r((char *)tmp_label_match, "_", &save);

  while(p != NULL) {
    p = strtok_r(NULL, "_", &save);
    if(p != NULL)
      bname = p;
  }
  s = strtok_r((char *)tmp_label_file, "/", &save);
  while(s != NULL) {
    s = strtok_r(NULL, "/", &save);
    if(s != NULL)
      label_name = s;
  }

  label_dir = replace((char *)label_file, label_name, "");
  sprintf(spname, "%s%s_simpoints", label_dir, bname);
  spfile = fopen(spname, "r");
  GMSG(spfile == 0, "ERROR: could not open simpoint file \"%s\" (ignoring it)", spname);

  while(n != EOF) {
    n           = fscanf(spfile, "%d", &col1);
    n           = fscanf(spfile, "%d", &col2);
    spbuffer[i] = col1;
    i++;
  }
  fclose(spfile);

  int32_t mini    = SescConf->getRecordMin(sp_section, "spoint");
  int32_t maxi    = SescConf->getRecordMax(sp_section, "spoint");
  int     npoints = SescConf->getRecordSize(sp_section, "spoint");
  printf("spsection: %s", sp_section);
  if(npoints != (maxi - mini + 1)) {
    MSG("ERROR: there can not be holes in the spoints[%d:%d]", mini, maxi);
    exit(-4);
  }
  spweight.resize(npoints);
  for(int i = 0; i < npoints; i++) {
    spweight[i] = SescConf->getDouble(sp_section, "spweight", i);
    // printf("spweight[%d]:%f \n", i,spweight[i]);
  }
#endif

  frequency                          = SescConf->getDouble("technology", "Frequency");
  throttleCycles                     = cyclesPerSample * SescConf->getDouble(trace.pwrsection, "throttleCycleRatio");
  thermalThrottle                    = SescConf->getDouble(trace.pwrsection, "thermalThrottle");
  throttleTime                       = throttleCycles / frequency;
  int power_samples_per_therm_sample = SescConf->getInt(model, "PowerSamplesPerThermSample");
  power_timestep                     = cyclesPerSample / frequency;
  therm_timestep                     = power_timestep * power_samples_per_therm_sample;

  std::cout << "USED THERM TIMESTEP: " << therm_timestep << " POWER TIMESTEP: " << power_timestep << std::endl;
  std::cout << "USED power_samples_per_therm_sample: " << power_samples_per_therm_sample << std::endl;

  const char *thermal          = SescConf->getCharPtr("", "thermal");
  int         n_layers_start   = SescConf->getRecordMin(thermal, "layer");
  int         n_layers_end     = SescConf->getRecordMax(thermal, "layer"); // last layer has ambient
  int         transistor_layer = -1;
  for(int i = n_layers_start; i <= n_layers_end; i++) {
    const char *layer_name = SescConf->getCharPtr(thermal, "layer", i);
    const char *layer_sec  = SescConf->getCharPtr(layer_name, "type");

    std::cerr << "Looking for layers " << layer_name << " type " << layer_sec;
    if(strcmp(layer_sec, "die_transistor") == 0) {
      transistor_layer = i;
      if(SescConf->getInt(layer_name, "lock_temp") == -1) {
        temp_model.set_temperature_layer(i, initialTemp);
        std::cerr << " Setting temperature " << initialTemp << " setting initial" << std::endl;
      }
    } else if(SescConf->getInt(layer_name, "lock_temp") == -1) {
      temp_model.set_temperature_layer(i, ambientTemp);
      std::cerr << " Setting temperature " << ambientTemp << " setting initial" << std::endl;
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

  // read initial leakage into the internal buffer
  // trace.read_lkg(lkg_file, devt_file);
  //
  //
  // init Metric
  const char *thermSec = SescConf->getCharPtr("", "thermal");
  const char *flpSec   = SescConf->getCharPtr(thermSec, "floorplan", 0);
  size_t      min      = SescConf->getRecordMin(flpSec, "blockDescr");
  size_t      max      = SescConf->getRecordMax(flpSec, "blockDescr");
  metrics              = new Metrics(max - min + 1);
  throttleLength       = 0;
  // FIXME SescConf->lock ();		// Done reading all the options
  // GStats::startAll();
  elapsed              = 0;
  int num_computations = 0;

  if(input_file) {
    start = clock();
    trace.ThermTraceInit(input_file);
    num_computations = process_trace(input_file, temp_model, power_timestep, transistor_layer, initialTemp,
                                     power_samples_per_therm_sample, false, cyclesPerSample, logLeakage, logDensity);
    end              = clock();
    elapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;

    cooldown(temp_model, transistor_layer);
  } // if input file
  else if(label_file) {
    std::ifstream file;
    file.open(label_file);
    if(!file.is_open() || file.bad()) {
      std::cerr << "ERROR: label file \"" << label_file << "\" can't be found or open\n";
      exit(-1);
    }

    SescConf->lock(); // Done reading all the options
    elapsed              = 0;
    int num_computations = 0;
    int line_no          = 0;

#ifdef METRICS_AT_SIMPOINTS
    exact_match = false;
#endif
    if(input_file) {
      start = clock();
      trace.ThermTraceInit(input_file);
      num_computations = process_trace(input_file, temp_model, power_timestep, transistor_layer, initialTemp,
                                       power_samples_per_therm_sample, false, cyclesPerSample, logLeakage, logDensity);
      end              = clock();
      elapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;

      cooldown(temp_model, transistor_layer);
    } // if input file
    else if(label_file) {
      std::ifstream file;
      file.open(label_file);
      if(!file.is_open() || file.bad()) {
        std::cerr << "ERROR: label file \"" << label_file << "\" can't be found or open\n";
        exit(-1);
      }

      char line[1024];
      bool enough = false;
      bool first  = true;
      while(!file.getline(line, 1000).eof() && !enough) {
        std::stringstream line_tmp;
        line_tmp.clear();
        line_tmp.str(line);

        int phaseID;
        line_tmp >> phaseID;
#ifdef METRICS_AT_SIMPOINTS
        exact_match = false;

        // FIXME: move me to config file
        // NOTE: To change nFastForwatdCoeff, only change this->
        trace.set_nFastForwardCoeff(10);
        //<-

        if(line_no == spbuffer[phaseID]) {
          exact_match = true;
          weight      = spweight[phaseID];
          // to remain 1, do not change
          trace.set_nFastForwardCoeff(1);
          // printf("weight[%d]:%e\n", phaseID,weight);
          // std::cin.get();
        }
#endif
        line_no++;
#if 0 // if you want only 20B instructions to run
        if(line_no == 200) break;
#endif
        std::cout << "Phase " << phaseID << std::endl;

        DIR *pdir;

        struct dirent *pent;

        pdir = opendir(label_path); //"." refers to the current dir
        if(!pdir) {
          printf("opendir() failure; terminating");
          exit(1);
        }
        errno = 1;
        while((pent = readdir(pdir))) {
          // std::cout <<"now pdir: "<< label_path << "pent: " << pent <<std::endl;
          char cadena[1024];
          sprintf(cadena, "power_dyn_%d_%s.XXXXXX", phaseID + 1, label_match);

          if(strncmp(pent->d_name, cadena, strlen(cadena) - 6) == 0) {
            //-------------------------
            bool rabbit = false;

            if(therm_timestep > (power_timestep * 8)) {
              //                      continue;
              //          rabbit = true;
            }

            sprintf(cadena, "%s/%s", label_path, pent->d_name);
            std::cout << "Power file for PhaseID " << phaseID << " is " << cadena << std::endl;

            errno = 0;

            start = clock();
            if(first) {
              trace.ThermTraceInit(cadena);
              first = false;
            }
            num_computations += process_trace(cadena, temp_model, power_timestep, transistor_layer, initialTemp,
                                              power_samples_per_therm_sample, rabbit, cyclesPerSample, logLeakage, logDensity);
            end = clock();
            elapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;

            //-------------------------
          }
        }

        if(errno) {
          printf("readdir() failure, or cannot find power trace %d; terminating", phaseID);
          exit(1);
        }

        closedir(pdir);
      }
      file.close();
      cooldown(temp_model, transistor_layer);
    } else if(argc > 6) { // process trace
      // sesctherm -c power.conf -o ouput input input input input input ...
      // sesctherm -c power.conf -o ouput -i input
      num_computations = 0;

      for(int i = 5; i < argc; i++) {
        std::cout << "Processing trace " << argv[i] << std::endl;
        start = clock();
        trace.ThermTraceInit(argv[i]);
        num_computations += process_trace(argv[i], temp_model, power_timestep, transistor_layer, initialTemp,
                                          power_samples_per_therm_sample, false, cyclesPerSample, logLeakage, logDensity);
        end = clock();
        elapsed += ((MATRIX_DATA)(end - start)) / CLOCKS_PER_SEC;
      }
      cooldown(temp_model, transistor_layer);
    } else {
      /*-------- SYNTHETIC CHIP SIMULATION --------*/
      // we perform fpga simulation
#if SYNTHETIC_TEST == FPGA
      std::cerr << " RUNNING THERMAL SIMULATION USING SYNTHETIC FPGA DATA " << std::endl;
      /************

        [dt]	TIME INTERVAL	AR05	AR04	AR03	AR02	AR01
        [1.199] 2-3.199			OFF		OFF		OFF		OFF		ON
        [2.419] 3.199-5.618		ON		OFF		OFF		OFF		OFF
        [2.355] 5.618-7.973		OFF		ON		OFF		OFF		OFF
        [2.425] 7.973-10.398	OFF		OFF		ON		OFF		OFF
        [2.36] 	10.398-12.758	OFF		OFF		OFF		ON		OFF
        [2.244] 12.758-15.002	OFF		OFF		OFF		OFF		ON

       ***************/

      std::vector<MATRIX_DATA> flp_temperature_map;
      MATRIX_DATA              timestep, recommended_timestep;

      // MATRIX_DATA leak_power = 0.15192;
      MATRIX_DATA leak_power         = 0.15192 / 6;
      MATRIX_DATA dyn_power          = 0.35421 * 1.4;
      temp_model.datalibrary_->time_ = 0.0;

      // initialize chip temperature
      MATRIX_DATA init_temp     = 26.00 + 273.15;
      MATRIX_DATA starting_temp = 37 + 273.15;
      //              temp_model.set_temperature_layer(0, init_temp);         //air (locked)
      temp_model.set_temperature_layer(1, init_temp);     // mainboard
      temp_model.set_temperature_layer(2, starting_temp); // pins
      temp_model.set_temperature_layer(3, starting_temp); // pcb
      temp_model.set_temperature_layer(4, starting_temp); // upcb-c5
      temp_model.set_temperature_layer(5, starting_temp); // c4 underfill
      temp_model.set_temperature_layer(6, starting_temp); // metal
      temp_model.set_temperature_layer(7, starting_temp); // substrate
      //              temp_model.set_temperature_layer(8, init_temp);         //air (locked)

      /*for(int i=1;i<=7;i++){
        temp_model.set_temperature_flp(7, transistor_layer, i, 38);
        temp_model.set_temperature_flp(6, transistor_layer, i, 38);
        temp_model.set_temperature_flp(5, transistor_layer, i, 38);
        temp_model.set_temperature_flp(4, transistor_layer, i, 38);
        temp_model.set_temperature_flp(3, transistor_layer, i, 38);
      temp_model.set_temperature_flp(2, transistor_layer, i, 38);
      temp_model.set_temperature_flp(1, transistor_layer, i, 38);
      } */

      temp_model.set_power_flp(0, transistor_layer, .00);
      temp_model.set_power_flp(5, transistor_layer, .00);
      temp_model.set_power_flp(7, transistor_layer, .00);
      temp_model.set_power_flp(8, transistor_layer, .00);
      temp_model.set_power_flp(9, transistor_layer, .00);

      recommended_timestep = .025; // temp_model.get_max_timestep();

      // ****** TIME INTERVAL  1 [0-3.199] [AR01 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, dyn_power);  // AR01 generation to 354.21mw
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, leak_power); // AR02 generation to 0w
      temp_model.set_power_flp(3, transistor_layer, leak_power); // AR03 generation to 0w
      temp_model.set_power_flp(2, transistor_layer, leak_power); // AR04 generation to 0w
      temp_model.set_power_flp(1, transistor_layer, leak_power); // AR05 generation to 0w

      timestep = 3.199; // run model to 3.199s

      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        // for every 20% of the computations, recompute the material properties (silicon and air properties)
        if(j % (int)(.2 * num_computations) == 0)
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        else
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }

      //              temp_model.print_graphics();
      // ****** TIME INTERVAL  2 [3.199-5.618] [AR05 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, leak_power); // AR01 generation to 0w
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, leak_power); // AR02 generation to 0w
      temp_model.set_power_flp(3, transistor_layer, leak_power); // AR03 generation to 0w
      temp_model.set_power_flp(2, transistor_layer, leak_power); // AR04 generation to 0w
      temp_model.set_power_flp(1, transistor_layer, dyn_power);  // AR05 generation to 2w
      timestep         = 2.419;
      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        if(j % (int)(.2 * num_computations) == 0) {
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        } else {
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        }

        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }

      // ****** TIME INTERVAL  3 [5.618-7.973] [AR04 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, leak_power); // AR01 generation to 0w
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, leak_power); // AR02 generation to 0w
      temp_model.set_power_flp(3, transistor_layer, leak_power); // AR03 generation to 0w
      temp_model.set_power_flp(2, transistor_layer, dyn_power);  // AR04 generation to 2w
      temp_model.set_power_flp(1, transistor_layer, leak_power); // AR05 generation to 0w
      timestep         = 2.355;
      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        if(j % (int)(.2 * num_computations) == 0) {
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        } else {
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        }

        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }

      // ****** TIME INTERVAL  4 [7.973-10.398] [AR03 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, leak_power); // AR01 generation to 0w
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, leak_power); // AR02 generation to 0w
      temp_model.set_power_flp(3, transistor_layer, dyn_power);  // AR03 generation to 2w
      temp_model.set_power_flp(2, transistor_layer, leak_power); // AR04 generation to 0w
      temp_model.set_power_flp(1, transistor_layer, leak_power); // AR05 generation to 0w
      timestep         = 2.425;
      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        if(j % (int)(.2 * num_computations) == 0) {
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        } else {
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        }

        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }

      // ****** TIME INTERVAL  5 [10.398-12.758] [AR02 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, leak_power); // AR01 generation to 0w
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, dyn_power);  // AR02 generation to 2w
      temp_model.set_power_flp(3, transistor_layer, leak_power); // AR03 generation to 0w
      temp_model.set_power_flp(2, transistor_layer, leak_power); // AR04 generation to 0w
      temp_model.set_power_flp(1, transistor_layer, leak_power); // AR05 generation to 0w
      timestep         = 2.36;
      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        if(j % (int)(.2 * num_computations) == 0) {
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        } else {
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        }

        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }

      // ****** TIME INTERVAL  6 [12.758-15.002] [AR01 ON, rest off] ********
      temp_model.set_power_flp(6, transistor_layer, dyn_power);  // AR01 generation to 2w
      temp_model.set_power_flp(5, transistor_layer, 0);          // ARUNUSED generation to 0w
      temp_model.set_power_flp(4, transistor_layer, leak_power); // AR02 generation to 0w
      temp_model.set_power_flp(3, transistor_layer, leak_power); // AR03 generation to 0w
      temp_model.set_power_flp(2, transistor_layer, leak_power); // AR04 generation to 0w
      temp_model.set_power_flp(1, transistor_layer, leak_power); // AR05 generation to 0w
      timestep         = 2.244;
      num_computations = timestep / min(.1 * timestep, recommended_timestep);
      for(int j = 0; j < num_computations; j++) {
        if(j % (int)(.2 * num_computations) == 0) {
          temp_model.compute(min(.1 * timestep, recommended_timestep), true);
        } else {
          temp_model.compute(min(.1 * timestep, recommended_timestep), false);
        }

        flp_temperature_map = temp_model.get_temperature_layer_map(transistor_layer, transistor_layer);
        std::cout << temp_model.get_time() << " ";
        // floorplan units are numbered from 0 -- following the order in the configuration file
        std::cout << flp_temperature_map[6] << " ";       // print AR01
        std::cout << flp_temperature_map[1] << " ";       // print AR05
        std::cout << flp_temperature_map[2] << " ";       // print AR04
        std::cout << flp_temperature_map[3] << " ";       // print AR03
        std::cout << flp_temperature_map[4] << std::endl; // print AR02
      }
      // otherwise run the AMD test
#elif SYNTHETIC_TEST == AMD

      std::cerr << " RUNNING THERMAL SIMULATION USING AMD DATA " << std::endl;

      std::vector<MATRIX_DATA> flp_temperature_map;
      MATRIX_DATA              timestep, recommended_timestep;

      temp_model.datalibrary_->time_ = 0.0;

      // initialize chip temperature to 25.0 degrees celcius
      MATRIX_DATA init_temp = 25.0 + 273.15;
#if 1
      temp_model.set_temperature_layer(1, init_temp); // mainboard (locked)
      temp_model.set_temperature_layer(2, init_temp); // pins
      temp_model.set_temperature_layer(3, init_temp); // package pwb
      temp_model.set_temperature_layer(4, init_temp); // package substrate c5
      temp_model.set_temperature_layer(5, init_temp); // c4 underfill
      temp_model.set_temperature_layer(6, init_temp); // interconnect
      temp_model.set_temperature_layer(7, init_temp); // bulksilicon
      //      temp_model.set_temperature_layer(8, init_temp);         //air
#endif

      recommended_timestep = temp_model.get_max_timestep();
      timestep             = .25;
      recommended_timestep = timestep * .01;

      MATRIX_DATA runtime = 4.5; // total runtime is 4.5s
      for(int iteration = 0; iteration < runtime / timestep; iteration++) {
        if(iteration % 2 == 0) { // every even run, all the units are off
          for(int i = 0; i < 23; i++)
            temp_model.set_power_flp(i, transistor_layer, 0);
        } else {
          for(int i = 0; i < 23; i++) // every odd run, all the units are on
            temp_model.set_power_flp(i, transistor_layer, .5);
        }

        num_computations = timestep / std::min(.1 * timestep, recommended_timestep);

        for(int j = 0; j < num_computations; j++) {
          // for every 20% of the computations, recompute the material properties (silicon and air properties)
          if(j % (int)(.2 * num_computations) == 0)
            temp_model.compute(min(.1 * timestep, recommended_timestep), true);
          else
            temp_model.compute(min(.1 * timestep, recommended_timestep), false);

          std::cout << temp_model.get_time() << " ";
          // floorplan units are numbered from 0 -- following the order in the configuration file
          MATRIX_DATA average_temp = 0;
          // for(int j=0;j<23;j++)
          //      average_temp+=flp_temperature_map[j];   //print unit b_i
          // cout << average_temp/23 << endl;
          std::cout << temp_model.get_temperature_layer_average(transistor_layer) << std::endl;
        }
      }
#ifdef DUMP_SVG
      temp_model.print_graphics();
#endif

#endif
    }

    // std::cout << " COMPUTATION TIME FOR " << num_computations << " ITERATIONS IS "
    //  << elapsed << "  (" << elapsed / num_computations << "SECONDS PER ITERATION )" << std::endl;

    //      std::cout << "COMPUTATION TIME FOR " << num_computations << " ITERATIONS IS " << elapsed << "  (" <<
    //      elapsed/num_computations << "SECONDS PER ITERATION )" << std::endl;

    //      std::cout << "QUITTING SESCTHERM" << std::endl;
  }
  // GStats::stopAll(1);

  // just to dump latest simulated time
  std::vector<MATRIX_DATA> flp_temperature_map;
  metrics->updateAllMetrics(const_cast<const std::vector<MATRIX_DATA> *>(&flp_temperature_map), 0, &trace, throttleLength, 0,
                            temp_model.get_time());
  SescConf->dump();
  GStats::report("Thermal Metrics");
  Report::close();

  if(trace.dumppwth) {
    fclose(logLeakage);
    fclose(logDensity);
  }
}
char *SescTherm::replace(char *st, char *orig, char *repl) {
  static char buffer[4096];
  char *      ch;
  if(!(ch = strstr(st, orig)))
    return st;
  strncpy(buffer, st, ch - st);
  buffer[ch - st] = 0;
  sprintf(buffer + (ch - st), "%s%s", repl, ch + strlen(orig));
  return buffer;
}
int main(int argc, const char **argv) {
  SescTherm sesctherm(argc, argv);
  sesctherm.computeTemp(argc, argv);
}
