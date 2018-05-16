// Contributed by  Ehsan K.Ardestani
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

/********************************************************************************
File name:      Metrics.cpp
Classes:        Metrics
********************************************************************************/

#include "Metrics.h"
#include "Report.h"
#include "math.h"
#include <float.h>
#include <iostream>
//#include <iomanip>
template <typename T> T tabs(const T &num) {
  return num < static_cast<T>(0) ? -num : num;
}

Metrics::Metrics(size_t flpSize) {
  // FIXME: define as constant
  nCall   = 0;
  MDT     = 500;
  IMC     = 2.5;
  AMBT    = 303;
  CMC     = 2.35;
  VDE     = 1.1;
  KLVOFST = 273.15;
  EM1     = 10444.0545073;
  SM1     = 29011.262502;
  TD1     = 78;
  TD2     = 0.081;
  TD3     = 8807.81930115;
  TD4     = -775180.934541;
  NB1     = 1.6328;
  NB2     = 856.064334447;
  NB3     = 795.140683155;
  NB4     = 3.33;
  NB5     = 0.01;

  MaxT = new std::vector<GStatsMax *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*MaxT)[i] = new GStatsMax("flpBlock[%d]_MaxT", i);
  }
  chipMaxT  = new GStatsMax("ChipMaxT");
  chipAvgT  = new GStatsAvg("ChipAvgT");
  chipGradT = new GStatsMax("ChipGradT");

  EM_fit = new std::vector<GStatsAvg *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*EM_fit)[i] = new GStatsAvg("flpBlock[%d]_EM_fit", i);
  }

  SM_fit = new std::vector<GStatsAvg *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*SM_fit)[i] = new GStatsAvg("flpBlock[%d]_SM_fit", i);
  }

  TDDB_fit = new std::vector<GStatsAvg *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*TDDB_fit)[i] = new GStatsAvg("flpBlock[%d]_TDDB_fit", i);
  }

  TC_fit = new std::vector<GStatsAvg *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*TC_fit)[i] = new GStatsAvg("flpBlock[%d]_TC_fit", i);
  }

  NBTI_fit = new std::vector<GStatsAvg *>(flpSize);
  for(size_t i = 0; i < flpSize; i++) {
    (*NBTI_fit)[i] = new GStatsAvg("flpBlock[%d]_NBTI_fit", i);
  }

  ChipPower     = new GStatsAvg("ChipPower");
  ChipLeak      = new GStatsAvg("ChipLeak");
  LSQPower      = new GStatsAvg("LSQPower");
  DCPower       = new GStatsAvg("DCPower");
  Throttling    = new GStatsMax("ThrottlingCycles");
  sescThermTime = new GStatsMax("sescThermTime");
  simulatedTime = new GStatsMax("simulatedTime");
}

void Metrics::updateTiming() {
  double maxV = DBL_MIN;
  double minV = DBL_MAX;
  for(size_t i = 0; i < (*Temperature).size(); i++) {
    (*MaxT)[i]->sample((*Temperature)[i], true);
    chipMaxT->sample((*Temperature)[i], true);

    maxV = (*Temperature)[i] > maxV ? (*Temperature)[i] : maxV;
    minV = (*Temperature)[i] < minV ? (*Temperature)[i] : minV;
  }
  chipGradT->sample(maxV - minV, true); // Gradients at each time snapshot
  chipAvgT->sample(maxV, true);
  // chipMaxTHist->sample(maxV);
}

// Computes the FIT value for each reliability metric
// TODO: figure out how to get timeinterval (Pass from Sampler?)
void Metrics::updateFITs(double timeinterval, ThermTrace *trace) {

  double scaleFactor   = 1e-10; // to avoid very big numbers
  double scaleFactorSM = 1e-30; // to avoid very big numbers

  double power = 0;
  double leak  = 0;

  for(size_t i = 0; i < (*Temperature).size(); i++) {
    double em = exp(EM1 / (*Temperature)[i]) * scaleFactor;
    (*EM_fit)[i]->sample(timeinterval / em, true);

    double sm = exp(SM1 / (*Temperature)[i]) * pow((MDT - (*Temperature)[i]), -IMC) * scaleFactorSM;
    (*SM_fit)[i]->sample((timeinterval / sm), true);

    // printf("sM:%ld, sM2:%lf, interv:%lf, sm:%lf temp:%lf \n", static_cast<uint64_t>((1/sm) * timeinterval), ((1/sm) *
    // timeinterval), timeinterval, sm, (*Temperature)[i]);
    double tddb = exp(TD3 / (*Temperature)[i]) * pow(pow((1 / VDE), (*Temperature)[i]), TD2) * pow(1 / VDE, TD1) *
                  exp(TD4 * pow(1 / (*Temperature)[i], 2)) * scaleFactor;
    (*TDDB_fit)[i]->sample((timeinterval / tddb), true);

    double tc = pow(tabs(1 / ((*Temperature)[i] - AMBT)), CMC);
    (*TC_fit)[i]->sample((timeinterval / tc), true);

    double nbti =
        pow((log(NB1 / (1 + 2 * exp(NB2 / (*Temperature)[i]))) - log(NB1 / (1 + 2 * exp(NB2 / (*Temperature)[i])) - NB5)) *
                (*Temperature)[i] / exp(1 / (*Temperature)[i]) * NB3,
            NB4) *
        scaleFactor;
    (*NBTI_fit)[i]->sample((timeinterval / nbti), true);

    // double energy =
    // exp((*flp)[i]->devType.a)*exp((*flp)[i]->devType.b/(*Temperature)[i])*pow((*flp)[i]->devType.c,(*Temperature)[i]) *
    // timeinterval;
  }
  for(size_t i = 0; i < trace->scaledLkgCntrValues_->size(); i++) {
    leak += trace->scaledLkgCntrValues_->at(i);
  }
  if(trace->energyCntrValues_->size() > 0) {
    for(size_t i = 0; i < trace->energyCntrValues_->at(0).size(); i++) {
      if(i == 6) {
        LSQPower->sample(trace->energyCntrValues_->at(0)[i], true);
        // printf("LSQP:%e\t",(trace->energyCntrValues_->at(0)[i]));
      }
      if(i == 1) {
        DCPower->sample(trace->energyCntrValues_->at(0)[i], true);
        // printf("DCP:%e\n",(trace->energyCntrValues_->at(0)[i]));
      }
      power += trace->energyCntrValues_->at(0)[i];
    }
  }

  ChipLeak->sample(leak, true);
  ChipPower->sample(power, true);
}

void Metrics::updateAllMetrics(const std::vector<MATRIX_DATA> *Temp, double timeinterval, ThermTrace *trace, uint64_t throttleTotal,
                               double time, double simTime) {
  Temperature = const_cast<std::vector<MATRIX_DATA> *>(Temp);
  updateTiming();
  updateFITs(timeinterval, trace);
  Throttling->sample(static_cast<double>(throttleTotal), true);
  sescThermTime->sample(time, true);
  simulatedTime->sample(simTime, true);
}

Metrics::~Metrics() {
}
