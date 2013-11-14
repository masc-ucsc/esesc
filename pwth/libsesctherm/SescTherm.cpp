/*
    ESESC: Super ESCalar simulator
    Copyright (C) 2008 University of California, Santa Cruz.
    Copyright (C) 2010 University of California, Santa Cruz.

    Contributed by  Ehsan K.Ardestani
                    Ian Lee 
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
File name:      SescTherm.cpp
Description:    Sample driver for SescTherm Library
********************************************************************************/

#include <complex>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <errno.h>
#include <dirent.h>
#include "SescConf.h"
#include "SescTherm.h"
#include "Report.h"
SescTherm::SescTherm ()
{
  conf_file   = NULL;
  input_file  = NULL;
  flp_file    = NULL;
  output_file = NULL;
  label_file  = NULL;
  label_match = NULL;
  label_path  = ".";
  option_cooldown=false;
  lkg_file = NULL;
  devt_file= NULL;
}


uint32_t SescTherm::cooldown(ThermModel & temp_model, int transistor_layer)
{ 
  if (!option_cooldown)
    return 0;

  //std::cout << " Cooldown section, delay: "<<throttleTime<<"\n";
  for (size_t j = 0; j < max_power.size (); j++) {
    temp_model.set_power_flp (j, transistor_layer, max_power[j] / 50);	// 5% of the max power per block
  }

  std::vector < MATRIX_DATA > flp_temperature_map;
  uint32_t d = 0;
  do{
    d++;
    option_cooldown=false;
    // 1 second cooldown
    //for (int j = 0; j < 400; j++) {
    //temp_model.compute (0.005, false);
    temp_model.compute (throttleTime, false);
    flp_temperature_map.clear(); 
    flp_temperature_map = 
    temp_model.get_temperature_layer_map (transistor_layer, transistor_layer);
    for (size_t i = 0; i < flp_temperature_map.size (); i++) {
      if (flp_temperature_map[i] > thermalThrottle-3){ 
        //std::cout << " ThrottleTime: "<<throttleTime/2<< "temp: "<<flp_temperature_map[i]<<"\n";
        option_cooldown=true;
        break;
      }
    }
  }while(option_cooldown);


    std::cout << "C " << temp_model.get_time () << " ";
    for (unsigned int i = 0; i < flp_temperature_map.size (); i++) {
      std::cout << flp_temperature_map[i] << "\t";
    }
    std::cout << std::endl;
    return d;
  //}
}

int SescTherm::process_trace (ThermModel & temp_model,
    MATRIX_DATA power_timestep, int transistor_layer,
    MATRIX_DATA initialTemp, int power_samples_per_therm_sample,
    bool rabbit)
{
  //int num_computations = 0;
  static std::vector <MATRIX_DATA > power (trace.get_energy_size());
  int samples = 0;


			//static uint32_t beshmar = 0;
#if 1 //eka, Check the thermalThrottle and cooldown if necessary
		{
			//std::cout << "T " << std::setprecision(6)<< temp_model.get_time() << " - ";
			std::vector <MATRIX_DATA> flp_temperature_map;

			bool throttle = true;
			option_cooldown = false;
			//throttleLength = 0;
			while (throttle){
		  flp_temperature_map.clear(); 
			flp_temperature_map = temp_model.get_temperature_layer_map (transistor_layer, transistor_layer);
				for (size_t i = 0; i < flp_temperature_map.size (); i++) {
					if (flp_temperature_map[i] > thermalThrottle){ 
						option_cooldown=true;
						break;
					}
				}

				if (option_cooldown){
					throttleLength+=cooldown (temp_model, transistor_layer);
				}else
					throttle = false;

				option_cooldown=false;

			}
		}
		//beshmar = throttleLength;
		//std::cout << "-T " << std::setprecision(6)<< temp_model.get_time()<< "-- "<<throttleLength<< " "<<beshmar<< " " <<beshmar*throttleCycles ;
		//std::cout << std::endl;
#endif

  while (trace.read_energy (false)) {

		for (size_t j = 0; j < trace.get_energy_size (); j++) {
			power[j] = trace.get_energy (j);
		}

		samples = 1;

#if DEBUG
		{
			std::cout << "D " << temp_model.get_time () << " ";
			MATRIX_DATA total = 0;
			MATRIX_DATA p;
			for (size_t j = 0; j < power.size (); j++) {
				p = ((power[j] / samples) / trace.get_area(j))/10000; //W/cm^2
				std::cout << trace.get_name(j) << "=" << p << "\t";
				if (p>1.2e3) {
					std::cout << "\nTOO DENSE!!!\n";
				//	std::cout << "ADJUSTED";
				//	power[j] *= 1/p;
				}
				total += p;
			}
			std::cout << ": " << std::endl;
		}
#endif
#if 0
		{
			std::cout << "P " << temp_model.get_time () << " ";
			MATRIX_DATA total = 0;
			MATRIX_DATA p;
			for (size_t j = 0; j < power.size (); j++) {
				p = power[j] / samples;
				std::cout << j << "=" << p << "\t";
				total += p;
			}
			std::cout << "\t:\t" << total << std::endl;
			std::cout << ": " << samples << std::endl;
		}
#endif

    for (size_t j = 0; j < trace.get_energy_size (); j++) {
      power[j] /= samples;
      if (max_power[j] < power[j])
        max_power[j] = power[j];
      temp_model.set_power_flp (j, transistor_layer, power[j]);
      power[j] = 0.0;
    }

    //-------------------------------------
    // Advance Thermal

		//std::cout << "TC " << std::setprecision(6)<< temp_model.get_time() << " - ";
    MATRIX_DATA ts = power_timestep * samples;
    static int each_ten_update = -1;
    if (each_ten_update < 0) {
      temp_model.compute(ts, true); 
      each_ten_update = 10;
    }else{
      temp_model.compute(ts, false);
      each_ten_update--;
    }

		//std::cout << "-TC " << std::setprecision(6)<< temp_model.get_time() << " -- " << ts;
		//std::cout << std::endl;
    //num_computations++;

    //-------------
		static MATRIX_DATA print_at = 0;
    if (temp_model.get_time () > print_at) { //eka, TEMPORARY
#ifndef DUMP_ALLPT
    print_at = temp_model.get_time () + 0.0000025;	// every 250us
#endif 

//#ifdef DUMP_SVG
      temp_model.print_graphics();
//#endif

      if (rabbit){
				std::cout << "R " << temp_model.get_time () << " ";
			//	if (logtemp)
			//		fprintf(logtemp, "R\t%f\t", temp_model.get_time()); 

			}else{
        std::cout << std::setprecision(6) << "T " << temp_model.get_time () << " ";
			if (logtemp)
					fprintf(logtemp, "T\t%f\t", temp_model.get_time()); 
			}

      std::vector < MATRIX_DATA > flp_temperature_map =
        temp_model.get_temperature_layer_map (transistor_layer,
            transistor_layer);
#ifdef DUMP_ALLPT
      double maxTemp = flp_temperature_map[0];
      for (unsigned int i = 0; i < flp_temperature_map.size (); i++) {
        std::cout << std::setprecision(3) << flp_temperature_map[i] << " ";
        if (flp_temperature_map[i+1] > maxTemp)
          maxTemp = flp_temperature_map[i+1];
			  //if (logtemp)
        //  fprintf(logtemp, "%f\t", flp_temperature_map[i]); 
      }
      //if (logMaxTemp){
      //  fprintf(logMaxTemp, "%f\n",maxTemp);
      //}
      fflush(logMaxTemp);
#else 
      for (unsigned int i = 0; i < flp_temperature_map.size (); i++) {
        std::cout << std::setprecision(3) << flp_temperature_map[i] << " ";
			  if (logtemp)
          fprintf(logtemp, "%f\t", flp_temperature_map[i]); 
      }
#endif 
      std::cout << std::endl;
			if (logtemp){
				fprintf(logtemp,"\n");
        fflush(logtemp);
      }
    }
  }
  return 0;
}


void SescTherm::configure(ChipEnergyBundle * energyBundle){
	clock_t start, end;
	elapsed = 0;
  
	trace.loadLkgPtrs(energyBundle);
  //eka, set_parameter in the class constructor will do that
  //read_parameters (argc, const_cast<char **>(argv));

  //SescConf = new SConfig (argc, argv);
  //if (SescConf == 0) {
   // std::cout << "Cannot open configuration file" << std::endl;
   // exit (1);
  //}

  std::cout << "STARTING SESCTHERM" << std::endl;

  model = SescConf->getCharPtr ("thermal", "model");

  trace.dumppwth    = 0; 
  trace.pwrsection = (char *) SescConf->getCharPtr("","pwrmodel",0);
  trace.dumppwth  = SescConf->getBool(trace.pwrsection,"dumpPower",0);
  //if (trace.dumppwth){
    //dumping temperature
    char *fname = (char *) malloc(1023); 
    sprintf(fname, "temp_%s",Report::getNameID());
    logtemp   = fopen(fname,"w");  
    GMSG(logtemp==0,"ERROR: could not open logtemp file \"%s\" (ignoring it)",fname);
#ifdef DUMP_ALLPT 
  char *tempfname = (char *) malloc(1023);
  sprintf(tempfname, "max_temp_%s",Report::getNameID());
  logMaxTemp   = fopen(tempfname,"w");  
  GMSG(logMaxTemp==0,"ERROR: could not open logtemp file \"%s\" (ignoring it)",tempfname);
#endif 
  //}
  trace.initDumpLeakage();
  start = clock ();
  useRK4 = SescConf->getBool(model,"useRK4");
  //eka, flp_file? what should I do with that?
  temp_model.ThermModelInit(flp_file, 0, useRK4, output_file, 
      true, false);
  end = clock ();

  std::cout << "MODEL INITIALIZED" << std::endl;

  elapsed = ((MATRIX_DATA) (end - start)) / CLOCKS_PER_SEC;
  std::cout << "MODEL INITIALIZATION TIME IS " << elapsed << "SECONDS" << std::endl;

  std::cout << "MAX MODEL TIMESTEP:" << temp_model.get_max_timestep () << std::endl;
  std::cout << "RECOMMENDED MODEL TIMESTEP: " << temp_model.get_recommended_timestep () << std::endl;

  temp_model.datalibrary_->set_timestep(temp_model.get_recommended_timestep());

  //this is a map of maps
  //it stores the time index of the run, and the corresponding temperature map, 
  // where flp_temperature_map[1.0][0] returns the temperature for flp unit 0 at timestep 1.0 

  initialTemp = SescConf->getDouble (model, "initialTemp");	// sesctherm works in K (not C)
  ambientTemp = SescConf->getDouble (model, "ambientTemp");

  cyclesPerSample = static_cast<int>(SescConf->getDouble(trace.pwrsection,"updateInterval"));
  frequency = SescConf->getDouble ("technology", "Frequency");
	throttleCycles = cyclesPerSample * SescConf->getDouble(trace.pwrsection,"throttleCycleRatio"); 
  thermalThrottle = SescConf->getDouble (trace.pwrsection, "thermalThrottle");
  power_samples_per_therm_sample = SescConf->getInt (model, "PowerSamplesPerThermSample");
  power_timestep = cyclesPerSample / frequency;
  therm_timestep = power_timestep * power_samples_per_therm_sample;
  throttleTime = throttleCycles/frequency;
  std::cout << "USED THERM TIMESTEP: " << therm_timestep << " POWER TIMESTEP: " << power_timestep << std::endl;
  std::cout << "USED power_samples_per_therm_sample: " << power_samples_per_therm_sample << std::endl;

  thermal = SescConf->getCharPtr ("", "thermal");
  n_layers_start = SescConf->getRecordMin (thermal, "layer");
  n_layers_end = SescConf->getRecordMax (thermal, "layer");	// last layer has ambient
  transistor_layer = -1;
  for (int i = n_layers_start; i <= n_layers_end; i++) {
    const char * layer_name = SescConf->getCharPtr (thermal, "layer", i);
    const char * layer_sec = SescConf->getCharPtr (layer_name, "type");

    std::cerr << "Looking for layers " << layer_name
      << " type " << layer_sec;
    if (strcmp (layer_sec, "die_transistor") == 0) {
      transistor_layer = i;
      if (SescConf->getInt (layer_name, "lock_temp") == -1) {
        temp_model.set_temperature_layer (i, initialTemp);
        std::cerr << " Setting temperature " << initialTemp
          << " LOCKED" << std::endl;
      }
    }else if (SescConf->getInt(layer_name, "lock_temp") == -1) {
      temp_model.set_temperature_layer (i, ambientTemp);
      std::cerr << " Setting temperature " << ambientTemp
        << " LOCKED" << std::endl;
    }else {
      double t = -SescConf->getDouble(layer_name, "lock_temp");
      if (t < -1)
        t = -t;
      std::cerr << " Setting temperature " << t
        << std::endl;
      temp_model.set_temperature_layer (i, t);
    }
  }
  // temp_model.set_temperature_layer(n_layers_end,ambientTemp); FIXME: this crashes n_layers_end == 8

  if (transistor_layer == -1) {
    for (int i = n_layers_start; i <= n_layers_end; i++) {
      const char * layer_name = SescConf->getCharPtr (thermal, "layer", i);
      const char * layer_sec = SescConf->getCharPtr (layer_name, "type");
      if (strcmp (layer_sec, "bulk_silicon") == 0)
        transistor_layer = i;
    }
    if (transistor_layer == -1) {
      std::cerr << "ERROR: Could not find 'die_transistor' layer" << std::endl;
      exit (-2);
    }
  }

  std::cerr << "USING FLOORPLAN WITHIN LAYER:" << transistor_layer << std::endl;

  int n_blocks = 0;
  for (int i = n_layers_start; i <= n_layers_end; i++)
    n_blocks += temp_model.get_modelunit_count (i);

  std::cout << "Modeling " << n_blocks << " blocks" << std::endl;
  max_power.resize (temp_model.  get_temperature_layer_map (transistor_layer,
        transistor_layer).size ());

  //eka, initialize trace with mapping between energyCntrNames
  // and the blocks in floorplaning
  trace.ThermTraceInit ();
  if (!trace.is_ready())
    exit(0);		// trace file did not exist
  
  trace.BlockT2StrcTMap();
  //init Metrics
	metrics = new Metrics(trace.get_energy_size());
  trace.loadLkgCoefs();

  printf("initializing throttlingLength\n");
	throttleLength = 0;
}


//eka, this is for the esesc integrated version, as opposed to 
//stand alone version
int SescTherm::computeTemp(ChipEnergyBundle *energyBundle, 
		std::vector<float> * temperatures, uint64_t timeinterval){
    clock_t start, end;
    //elapsed = 0;
    int return_signal = 0;

		//adjust for timeinterval
		power_timestep = static_cast<MATRIX_DATA>(timeinterval/1.0e9);
		// throttlingcycles is based on original interval

		start = clock ();
		trace.loadValues(energyBundle, temperatures);
    return_signal = process_trace(temp_model, power_timestep, 
        transistor_layer, initialTemp, power_samples_per_therm_sample, 
        false);
		end = clock ();
    elapsed += ((MATRIX_DATA) (end - start)) / CLOCKS_PER_SEC;


		// eka, dump block temperature
    std::vector < MATRIX_DATA > flp_temperature_map = 
        temp_model.get_temperature_layer_map (transistor_layer, transistor_layer);
    //  eka, map block temperature to power model structure temperature
		trace.BlockT2StrcT(&flp_temperature_map, temperatures);
    
    cooldown (temp_model, transistor_layer);

    //std::cout << " COMPUTATION TIME FOR " << num_computations << " ITERATIONS IS "
    //    << elapsed << "  (" << elapsed / num_computations << "SECONDS PER ITERATION )" << std::endl;
		return return_signal;
}

void SescTherm::dumpTemp(std::vector <MATRIX_DATA> * flp_temperature_map){
			if (logtemp)
					fprintf(logtemp, "T\t%f\t", temp_model.get_time()); 
			

#ifdef DUMP_ALLPT
      double maxTemp = (*flp_temperature_map)[0];
      for (unsigned int i = 0; i < (*flp_temperature_map).size (); i++) {
        if ((*flp_temperature_map)[i+1] > maxTemp)
          maxTemp = (*flp_temperature_map)[i+1];
			  if (logtemp)
          fprintf(logtemp, "%f\t", (*flp_temperature_map)[i]); 
      }
      if (logMaxTemp){
        fprintf(logMaxTemp, "%f\n",maxTemp);
      }
      fflush(logMaxTemp);
#else 
      for (unsigned int i = 0; i < (*flp_temperature_map).size(); i++) {
			  if (logtemp)
          fprintf(logtemp, "%f\t", (*flp_temperature_map)[i]); 
      }
#endif 
			if (logtemp)
				fprintf(logtemp,"\n");
      fflush(logtemp);
}


SescTherm::~SescTherm(){
  //if (trace.dumppwth){
    if (logtemp)
      fclose(logtemp);
 // }
#ifdef DUMP_ALLPT
    if (logMaxTemp)
     fclose(logMaxTemp); 
#endif
}

void SescTherm::updateMetrics(uint64_t timeinterval){
    std::vector < MATRIX_DATA > flp_temperature_map = 
        temp_model.get_temperature_layer_map (transistor_layer, transistor_layer);
    metrics->updateAllMetrics( const_cast<const std::vector < MATRIX_DATA > *>(&flp_temperature_map)
				,timeinterval, &trace, throttleLength, elapsed,temp_model.get_time() );
		dumpTemp(&flp_temperature_map);
}


