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

/*******************************************************************************
File name:      Metrics.h
Classes:        Metrics
********************************************************************************/

#ifndef METRICS_H
#define METRICS_H

#include "GStatsTherm.h"
#include "ThermTrace.h"

class Metrics {
private:
  std::vector<GStatsMax *> *MaxT;
  std::vector<GStatsAvg *> *EM_fit;
  std::vector<GStatsAvg *> *SM_fit;
  std::vector<GStatsAvg *> *TDDB_fit;
  std::vector<GStatsAvg *> *TC_fit;
  std::vector<GStatsAvg *> *NBTI_fit;
  GStatsAvg *               ChipPower;
  GStatsAvg *               ChipLeak;
  GStatsMax *               chipMaxT;
  GStatsMax *               chipGradT;
  GStatsAvg *               chipAvgT;
  GStatsMax *               sescThermTime;
  GStatsMax *               simulatedTime;
  GStatsMax *               Throttling;

  GStatsAvg *               LSQPower;
  GStatsAvg *               DCPower;
  uint32_t                  nCall;
  std::vector<MATRIX_DATA> *Temperature;

  // parameters,FIXME: define as constant
  int   MDT;
  float IMC;
  int   AMBT;
  float CMC;
  float VDE;
  float KLVOFST;
  float EM1;
  float SM1;
  int   TD1;
  float TD2;
  float TD3;
  float TD4;
  float NB1;
  float NB2;
  float NB3;
  float NB4;
  float NB5;

  void updateTiming();
  void updateFITs(double timeinterval, ThermTrace *trace);

public:
  Metrics(size_t flpSize);
  ~Metrics();
  void updateAllMetrics(const std::vector<MATRIX_DATA> *Temp, double interval, ThermTrace *trace, uint64_t throttleTotal,
                        double time, double simTime);
};

#endif
