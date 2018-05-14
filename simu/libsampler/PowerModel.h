#ifndef POWERMODEL_H
#define POWERMODEL_H

// Contributed by Gabriel Southern
//                Jose Renau
//                Ehsan K.Ardestani
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

#include "EmuSampler.h"
#include "PowerGlue.h"
#include "TaskHandler.h"
#include "callback.h"
#include <map>
#include <vector>
/* }}} */

class PowerStats;
class Wrapper;
class MemObj;

const float INITIAL_TEMP = 300.0; // TODO: read this from conf file instead

class PowerModel {
private:
  enum SamplerType { Adaptive, Periodic, SMARTS, SkipSim, SPoint, Others } samplerType;
  std::vector<PowerStats *> *stats;

  PowerGlue powerGlue;

  std::vector<GStatsAvg *>  powerDynCntr;
  std::vector<GStatsAvg *>  powerLkgCntr;
  std::vector<GStatsAvg *>  energyCntr;
  std::vector<GStatsAvg *>  engDelayCntr;
  std::vector<GStatsAvg *>  currentEngDelayCntr;
  std::vector<GStatsCntr *> areaCntr;
  GStatsCntr                powerTime;

  std::vector<const char *> instCounters;
  double                    thermalThrottle;
  double                    tCycQuanta;
  bool                      enableTurbo;
  bool                      enableNTC;
  int                       turboMode;
  float                     volNTC;

  GStatsCntr *pSig[100];
  uint32_t    totalPowerSamples;
  const char *pwrsection;
  FILE *      logpwr;
  FILE *      logprf;
  FILE *      logpwrdyn;
  FILE *      logpwrlkg;
  FILE *      logDeviceTypes;
  bool        dumppwth;
  bool        dumppwthSplit;
  uint32_t    nPowerCall;
  const char *filename;
  FILE *      logfile;
  Wrapper *   mcpatWrapper;

  uint32_t            updateInterval;
  uint64_t            instCountCurrent;
  uint64_t            instCountPrev;
  std::vector<Time_t> clockPrev;
  std::vector<Time_t> clockInterval;
  uint64_t            timeInterval;
  double              freq;
  uint64_t            bPredHitCurrent;
  uint64_t            bPredMissCurrent;
  uint64_t            bPredHitPrev;
  uint64_t            bPredMissPrev;
  double              avgMemLat;
  double              ipc;
  float               avgTimingIPC;
  uint32_t            IWSHisLength;
  bool                avgPowerValid;
  bool                doPowPred;
  bool                doTherm;
  float               totalPow;
  float               meanPow;
  float               stdevPow;
  short               downSample;
  float               samplingRatio;

  uint32_t                      coreEIdx;
  uint32_t                      ncores;
  uint32_t                      nL2;
  uint32_t                      nL3;
  std::vector<uint32_t> *       coreIndex;
  std::vector<uint32_t> *       gpuIndex;
  typedef std::vector<MemObj *> CoupledObjectsType;
  CoupledObjectsType            coupledObjects;

  std::vector<float>              totalPowHist;
  std::vector<std::vector<float>> powerHist;
  std::vector<float>              predPow;
  size_t                          PowerHistSize;
  size_t                          headPtr;
  bool                            usePrediction;
  void                            readStatsFromConfFile(const char *section);
  void                            genIndeces();
  void                            initPowerModel(const char *section);
  void                            readPowerConfig(const char *section);
  void                            readTempConfig(const char *section);
  void                            initTempModel(const char *section);
  void                            initPowerGStats();
  void                            updatePowerGStats();
  void                            updateSig();
  void                            updatePowerTime(uint64_t timeinterval);
  EmulInterface *                 getEmul(FlowID fid) {
    return TaskHandler::getEmul(fid);
  };
  void syncStats() {
    return TaskHandler::syncStats();
  };
  void freeze(FlowID fid, Time_t nCycles) {
    return TaskHandler::freeze(fid, nCycles);
  };
  FlowID getNumActiveCores() {
    return TaskHandler::getNumActiveCores();
  };
  float getTurboRatio() {
    return EmuSampler::getTurboRatio();
  };

  float    getDyn(uint32_t i);
  float    getLkg(uint32_t i);
  float    getScaledLkg(uint32_t i);
  uint32_t getCyc(uint32_t i);

  FILE *dpowerf;

  SescThermWrapper *sescThermWrapper;

public:
  void addTurboCoupledMemory(MemObj *mobj);
  void setTurboRatio(float freqCoef);

  ChipEnergyBundle *energyBundle;

  std::vector<std::string> *           sysConn;
  std::vector<std::string> *           coreConn;
  std::map<float, float> *             ipcRepo;
  std::map<float, std::vector<float>> *pwrCntrRepo;
  std::vector<float> *                 avgEnergyCntrValues;
  uint32_t                             nSamples;
  std::vector<float> *                 temperatures;
  std::vector<uint32_t> *              activity;
  PowerModel();
  ~PowerModel();

  void updateSescTherm(int64_t ti);

  void   plug(const char *section);
  void   unplug();
  void   calcStats(uint64_t timeinterval, bool reusePower, FlowID fid);
  void   updateActivity(uint64_t timeinterval, FlowID fid = 0);
  void   printStatus();
  void   testWrapper();
  void   getPowerDumpFile();
  void   closePowerDumpFile();
  void   dumpPerf(bool pwrReuse);
  void   dumpDynamic();
  void   dumpLeakage();
  void   dumpDeviceTypes();
  void   dumpActivity();
  void   startDump();
  void   stopDump();
  double getIPC() {
    return avgTimingIPC;
  };
  double getIPC(float key) {
    std::map<float, float>::iterator it = ipcRepo->find(key);
    return it->second;
  };
  float getLastTotalPower();
  float getLastDynPower();
  void  updatePowerHist();
  void  loadPredPower();
  bool  isDeviationHigh();
  void  setUsePrediction() {
    usePrediction = true;
  };
  void clearUsePrediction() {
    usePrediction = false;
  };
  bool predictionStatus() {
    return usePrediction;
  };
  short getDownSample() {
    return downSample;
  }
  void adjustSamplingRate();
  void stdevOffst(bool subtract = true, float c = 0.5);
  void dumpTotalPower(const char *str);
  void energy2power();
  bool throttle(FlowID fid, uint32_t throttleLength);

  float getPowerTime() const {
    return powerTime.getDouble() / freq;
  };

  void   updateTurboState();
  void   initTurboState();
  void   updatePowerTurbo(int freqState);
  float  getVolRatio(int state);
  int    updateFreqTurbo();
  int    updateFreqDVFS_T();
  float  getMaxT();
  double getFreq() {
    return freq;
  };

  void plugStackingValidator();
  void stackingValidator();

  void setSamplingRatio(float r) {
    samplingRatio = r;
  };
};

#endif // POWERMODEL_H
