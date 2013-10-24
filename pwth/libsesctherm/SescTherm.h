/*
    ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ehsan K.Ardestani
                    Ian Lee
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
File name:      SescTherm.h
Classes:        SescTherm
********************************************************************************/

#ifndef SESC_THERM_H
#define SESC_THERM_H

#define SYNTHETIC_TEST 0
#define FPGA 1
#define AMD 2
//#define METRICS_AT_SIMPOINTS 1
//#define DUMP_SVG 1

#include "sescthermMacro.h"
#include "ThermTrace.h"
#include "sesctherm3Ddefine.h"

#include "ClassDeclare.h"
#include "ChipMaterial.h"
#include "DataLibrary.h"
#include "ThermModel.h"
#include "Metrics.h"

#include "Bundle.h"

//eka TODO:
//1- make the class definition cleaner in terms of
//public and private variables and methods
//3- Clean for stand alone version vs. integrated one
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
    bool option_cooldown;
   
    FILE *logtemp;
#ifdef METRICS_AT_SIMPOINTS
    FILE *spfile; //simpoints
    const char* sp_section; 
    bool exact_match;
    std::vector<double>   spweight;
    double weight; 
#endif 
#ifdef DUMP_ALLPT
    FILE *logMaxTemp;
#endif 
    const char* tfilename;
    std::vector<MATRIX_DATA> max_power;
    const char * model;
    bool useRK4;
    ThermModel temp_model;
    MATRIX_DATA initialTemp;
    MATRIX_DATA ambientTemp;
    int cyclesPerSample;
    MATRIX_DATA frequency;
    int power_samples_per_therm_sample;
    MATRIX_DATA power_timestep;
    MATRIX_DATA therm_timestep;
    const char *thermal;
    int n_layers_start;
    int n_layers_end;
    int transistor_layer;
    ThermTrace trace;
		Metrics *metrics;
    MATRIX_DATA throttleCycles;
    MATRIX_DATA throttleTime;
    uint64_t throttleLength; // how many times called in a row
    MATRIX_DATA thermalThrottle;
    MATRIX_DATA elapsed;
	 	// eka, I have to figure out what else should be defined here
    
    //elnaz, added scaling leakage based on temperature
    //std::vector <float> *lkgCntrValues; //eka: FIXME: move to ThermTrace
    //std::vector <uint32_t> *devTypes; //device types specificed in OOO.xml in mcpat

    
    SescTherm(int argc, const char **argv);
    SescTherm();
    ~SescTherm();
    void showUsage();
    void set_parameters(int argc, char **argv);
    uint32_t cooldown(ThermModel & temp_model, int transistor_layer);

    int process_trace(ThermModel & temp_model,
    MATRIX_DATA power_timestep, int transistor_layer,
    MATRIX_DATA initialTemp, int power_samples_per_therm_sample,
    bool rabbit);

    int process_trace (const char *input_file, ThermModel & temp_model,
    MATRIX_DATA power_timestep, int transistor_layer,
    MATRIX_DATA initialTemp, int power_samples_per_therm_sample,
    bool rabbit, int cyclesPerSample, FILE *logLeakage, FILE *logDensity);
		void configure(ChipEnergyBundle * energyCntrNames);
		void configure(std::vector<std::string> * energyCntrNames,
				std::vector<float> *leakageCntrValues,
				std::vector<uint32_t> *deviceTypes);
		int computeTemp(ChipEnergyBundle *energyBundle,
				std::vector<float> * temperatures, uint64_t timeinterval);
    void dumpBlockT2StrT(std::vector <float> *strTemp);
    void computeTemp(int argc, const char **argv);
    void dumpTemp(std::vector <MATRIX_DATA> * flp_temperature_map);
		uint32_t get_throttleLength(){
			return throttleLength;
		}
		void updateMetrics(uint64_t timeinterval=100000);
		char *replace(char *st, char *orig, char *repl);
};
#endif
