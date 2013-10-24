/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2008 University of California, Santa Cruz.
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by 
	 								Ehsan K.Ardestani
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
File name:      ThermTrace.h
Classes:        ThermTrace
********************************************************************************/

#ifndef THERM_TRACE_H_
#define THERM_TRACE_H_

#include <vector>
#include <string>

#include "sescthermMacro.h"
#include "nanassert.h"
#include "sesctherm3Ddefine.h"
#include "Bundle.h"

#include "Report.h"


class ThermTrace {
  public:
    typedef std::vector<const char *> TokenVectorType;
    // elnaz, added for dumping pwr numbers
    const char* filename;
    const char* pwrsection;
    FILE *loglkg;
    FILE *logdyn;
#ifdef DUMP_ALLPT
    FILE *loglkgTotal;
#endif
    FILE * input_fd_;
    bool dumppwth;
    //elnaz, added for scaling leakage based on temperature
    LeakageCoefs hpThLeakageCoefs;
    LeakageCoefs lstpThLeakageCoefs;
    LeakageCoefs lpThLeakageCoefs;

    // Floorplan
    class FLPUnit {
      public:
        FLPUnit(const char *na) : name(na) {
          area  = 0;
          units = 0;
          x = 0;
          y = 0;
          delta_x = 0;
          delta_y = 0;
        }
        void dump() const {
          MSG("Block %20s area %4.2g #units %d (%6.5g,%6.5g) -> (%6.5g,%6.5g)",name, area*1e6, units,x,y,x+delta_x,y+delta_y);
        }

        const char *name;
        int32_t id;
        TokenVectorType match;
        std::vector<uint8_t> blk2strMap;
        float area;
        float x;
        float y;
        float delta_x;
        float delta_y;
        int32_t units;
        float energy; // used by trace only
        LeakageCoefs devType;
		};
		std::vector<FLPUnit *> flp;
		int get_nFastForward(){
			return actualnFastForward;
		};
 		void set_nFastForwardCoeff(int f){
      I(f!=0);
			nFastForwardCoeff = f;
		};
 private:
    const char *input_file_;
    //FILE * input_fd_;
    //eka, pointer to a vector that keeps names of counters
    std::vector<std::string> *  energyCntrNames_;
    //eka, pointer to a vector that keeps values of counters
    std::vector<float> * lkgCntrValues_;
    std::vector <uint32_t> *devTypes; 
		ChipEnergyBundle *energyBundle;
    uint32_t energyCntrSample_;
    uint32_t totalPowerSamples;
 		int nFastForward;
		int actualnFastForward;
		int nFastForwardCoeff;
   
    class Mapping {
      public:
        Mapping() {
          area = 0;
          name = 0;
        }
        void dump() const {
          fprintf(stderr,"Mapping %45s area %4.2g map:", name, area*1e6);
          for(size_t j=0; j<map.size(); j++) {
            fprintf(stderr," %3d (%3.0f%%)", map[j], 100*ratio[j]);
          }
          fprintf(stderr,"\n");
        }

        const char *name;
        float area;
        std::vector<int>   map;
        std::vector<float> ratio;
    };
    std::vector<Mapping> mapping;

    static void tokenize(const char *str, TokenVectorType &tokens);
    static bool grep(const char *line, const char *pattern);

    void read_sesc_variable(const char *input_file);
    void read_sesc_variable();
    void read_floorplan_mapping();
    //elnaz, dump the floorplan block power numbers 
    void dump_flpblk_pwrs();

  protected:
  public:

    std::vector<float> * scaledLkgCntrValues_;
    std::vector<std::vector <float> > * energyCntrValues_;

    ThermTrace(const char *input_file);
    ThermTrace();
    ~ThermTrace();
    //eka, read from the input vector
    void ThermTraceInit();
    void ThermTraceInit(const char *input_file);
    void ThermTraceInit_light(const char *input_file);

    bool is_ready() const { return input_fd_>=0; }
    void loadValues(ChipEnergyBundle *energyBundle,
                    std::vector <float> *temperatures);
		void loadLkgPtrs(ChipEnergyBundle *eBundle) {
			energyBundle = eBundle;
    }

    void loadLkgCoefs();
    //eka, change it to also read from input vector, 
    //pass TRUE for file, FALSE for vector.
    bool read_energy(bool File_nVector);

    float get_energy(size_t pos) const {
      I(pos < flp.size());
      return flp[pos]->energy;
    }
    const char *get_name(size_t pos) const {
      I(pos < flp.size());
      return flp[pos]->name;
    }
    float get_area(size_t pos) const { return flp[pos]->area; }
    size_t get_energy_size() const { return flp.size(); }

    const FLPUnit *findBlock(const char *name) const;

    void dump() const;
    //eka, map temperature in transistor layer and
    //back to structures in power model
    void BlockT2StrcTMap();
    void BlockT2StrcT(const std::vector<MATRIX_DATA> *blkTemp,
    std::vector<float> * strTemp);
		unsigned int getCounterSize(){
			return energyBundle->cntrs.size();
		}
    //elnaz, scaling the leakage
    void leakage(std::vector<double> *Temperatures);
    float scaleLeakage(float temperature, float leakage, uint32_t device_type);

    void read_lkg(const char *lkg_file, const char *devt_file);
    //elnaz, dumping the scaled lkg
    void initDumpLeakage();
    void dumpLeakage();
    void dumpLeakage2(FILE* logLeakage, FILE* logDensity);
    void initLoglkg(const char* tfilename);


};

#endif
