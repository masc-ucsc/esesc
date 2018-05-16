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
File name:      ThermTrace.h
Classes:        ThermTrace
********************************************************************************/

#ifndef THERM_TRACE_H_
#define THERM_TRACE_H_

#include <string>
#include <vector>

#include "Bundle.h"
#include "nanassert.h"
#include "sesctherm3Ddefine.h"
#include "sescthermMacro.h"

#include "Report.h"

class ThermTrace {
public:
  typedef std::vector<const char *> TokenVectorType;
  // elnaz, added for dumping pwr numbers
  const char *filename;
  const char *pwrsection;
  FILE *      loglkg;
  FILE *      logdyn;
  FILE *      input_fd_;
  bool        dumppwth;
  // elnaz, added for scaling leakage based on temperature
  LeakageCoefs hpThLeakageCoefs;
  LeakageCoefs lstpThLeakageCoefs;
  LeakageCoefs lpThLeakageCoefs;

  // Floorplan
  class FLPUnit {
  public:
    FLPUnit(const char *na)
        : name(na) {
      area    = 0;
      units   = 0;
      x       = 0;
      y       = 0;
      delta_x = 0;
      delta_y = 0;
    }
    void dump() const {
      MSG("Block %20s area %4.2g #units %d (%6.5g,%6.5g) -> (%6.5g,%6.5g)", name, area * 1e6, units, x, y, x + delta_x,
          y + delta_y);
    }

    const char *         name;
    int32_t              id;
    TokenVectorType      match;
    std::vector<uint8_t> blk2strMap;
    float                area;
    float                x;
    float                y;
    float                delta_x;
    float                delta_y;
    int32_t              units;
    float                energy; // used by trace only
    LeakageCoefs         devType;
  };
  std::vector<FLPUnit *> flp;
  int                    get_nFastForward() {
    return actualnFastForward;
  };
  void set_nFastForwardCoeff(int f) {
    I(f != 0);
    nFastForwardCoeff = f;
  };

private:
  const char *input_file_;
  // FILE * input_fd_;
  // eka, pointer to a vector that keeps names of counters
  std::vector<std::string> *energyCntrNames_;
  // eka, pointer to a vector that keeps values of counters
  std::vector<float> *   lkgCntrValues_;
  std::vector<uint32_t> *devTypes;
  ChipEnergyBundle *     energyBundle;
  uint32_t               energyCntrSample_;
  uint32_t               totalPowerSamples;
  int                    nFastForward;
  int                    actualnFastForward;
  int                    nFastForwardCoeff;

  class Mapping {
  public:
    Mapping() {
      area = 0;
      name = 0;
    }
    void dump() const {
      fprintf(stderr, "Mapping %45s area %4.2g map:", name, area * 1e6);
      for(size_t j = 0; j < map.size(); j++) {
        fprintf(stderr, " %3d (%3.0f%%)", map[j], 100 * ratio[j]);
      }
      fprintf(stderr, "\n");
    }

    const char *       name;
    float              area;
    std::vector<int>   map;
    std::vector<float> ratio;
  };
  std::vector<Mapping> mapping;

  static void tokenize(const char *str, TokenVectorType &tokens);
  static bool grep(const char *line, const char *pattern);

  void read_sesc_variable(const char *input_file);
  void read_sesc_variable();
  void read_floorplan_mapping();
  // elnaz, dump the floorplan block power numbers
  void dump_flpblk_pwrs();

protected:
public:
  std::vector<float> *             scaledLkgCntrValues_;
  std::vector<std::vector<float>> *energyCntrValues_;

  ThermTrace(const char *input_file);
  ThermTrace();
  ~ThermTrace();
  // eka, read from the input vector
  void ThermTraceInit();
  void ThermTraceInit(const char *input_file);
  void ThermTraceInit_light(const char *input_file);

  bool is_ready() const {
    return input_fd_ >= 0;
  }
  void loadValues(ChipEnergyBundle *energyBundle, std::vector<float> *temperatures);
  void loadLkgPtrs(ChipEnergyBundle *eBundle) {
    energyBundle = eBundle;
  }

  void loadLkgCoefs();
  // eka, change it to also read from input vector,
  // pass TRUE for file, FALSE for vector.
  bool read_energy(bool File_nVector);

  float get_energy(size_t pos) const {
    I(pos < flp.size());
    return flp[pos]->energy;
  }
  const char *get_name(size_t pos) const {
    I(pos < flp.size());
    return flp[pos]->name;
  }
  float get_area(size_t pos) const {
    return flp[pos]->area;
  }
  size_t get_energy_size() const {
    return flp.size();
  }

  const FLPUnit *findBlock(const char *name) const;

  void dump() const;
  // eka, map temperature in transistor layer and
  // back to structures in power model
  void         BlockT2StrcTMap();
  void         BlockT2StrcT(const std::vector<MATRIX_DATA> *blkTemp, std::vector<float> *strTemp);
  unsigned int getCounterSize() {
    return energyBundle->cntrs.size();
  }
  // elnaz, scaling the leakage
  void  leakage(std::vector<double> *Temperatures);
  float scaleLeakage(float temperature, float leakage, uint32_t device_type);

  void read_lkg(const char *lkg_file, const char *devt_file);
  // elnaz, dumping the scaled lkg
  void initDumpLeakage();
  void dumpLeakage();
  void dumpLeakage2(FILE *logLeakage, FILE *logDensity);
  void initLoglkg(const char *tfilename);
};

#endif
