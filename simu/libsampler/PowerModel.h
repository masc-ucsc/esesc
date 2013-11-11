#ifndef POWERMODEL_H
#define POWERMODEL_H

/* License & includes {{{1 */
/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Gabriel Southern
                  Jose Renau
                  Ehsan K.Ardestani

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <vector>
#include <map>
#include "PowerStats.h"
#include "PowerGlue.h"
#include "callback.h"
#include "Wrapper.h"
#include "SescThermWrapper.h"
#include "TaskHandler.h"
#include "EmuSampler.h"
#include "Bundle.h"

//#define DUMP_ALLPT  //enables dumping of total power, total scaled leakage, max Temperature, and signature vectors
/* }}} */

const float INITIAL_TEMP = 300.0; // TODO: read this from conf file instead

class PowerModel {
private:

  enum SamplerType {
    Adaptive,
    Periodic,
    SMARTS,
    SkipSim,
    SPoint,
    Others
  }samplerType;
  std::vector<PowerStats *> *stats;

  PowerGlue powerGlue;

  std::vector<GStatsAvg *> powerDynCntr; 
  std::vector<GStatsAvg *> powerLkgCntr; 
  std::vector<GStatsAvg *> energyCntr; 
  std::vector<GStatsAvg *> engDelayCntr; 
  std::vector<GStatsAvg *> currentEngDelayCntr; 
  std::vector<GStatsCntr *> areaCntr; 
  GStatsCntr powerTime;

  std::vector<const char *> instCounters;
  double thermalThrottle;  
  double tCycQuanta; 
  bool   enableTurbo;
  bool   enableNTC;
  int    turboMode;
  float  volNTC;
#ifdef ENABLE_CUDA
  uint32_t kId;
  std::vector <float>  kernelsVolNTC;
#endif

  GStatsCntr *pSig[100];
  uint32_t totalPowerSamples;
  const char* pwrsection;
  FILE *logpwr;
  FILE *logprf;
  FILE *logpwrdyn;
  FILE *logpwrlkg;
  FILE *logDeviceTypes;
  bool dumppwth;
  bool dumppwthSplit;
  uint32_t nPowerCall;
  const char *filename;
  FILE *logfile;
  Wrapper *mcpatWrapper;

  uint32_t updateInterval;
  uint64_t instCountCurrent;
  uint64_t instCountPrev;
  std::vector<Time_t>  clockPrev ;
  std::vector<Time_t>  clockInterval ;
  uint64_t timeInterval;
  double  freq;
  uint64_t bPredHitCurrent;
  uint64_t bPredMissCurrent;
  uint64_t bPredHitPrev;
  uint64_t bPredMissPrev;
  double   avgMemLat;
  double   ipc;
  float    avgTimingIPC;
  uint32_t IWSHisLength; 
  bool     avgPowerValid;
  bool     doPowPred;
  bool     doTherm;
  float    totalPow;
  float    meanPow;
  float    stdevPow;
  short     downSample;
  float    samplingRatio;


  uint32_t coreEIdx;
  uint32_t ncores;
  uint32_t nL2;
  uint32_t nL3;
  std::vector<uint32_t> *coreIndex;
  std::vector<uint32_t> *gpuIndex;

  std::vector<float> totalPowHist;
  std::vector<std::vector<float> > powerHist;
  std::vector<float> predPow;
  size_t PowerHistSize;
  size_t headPtr;
  bool usePrediction;
  void readStatsFromConfFile(const char *section);
  void genIndeces();
  void initPowerModel(const char *section);
  void readPowerConfig(const char *section);
  void readTempConfig(const char *section);
  void initTempModel(const char *section);
  void initPowerGStats();
  void updatePowerGStats();
  void updateSig();
  void updatePowerTime(uint64_t timeinterval);
  std::vector<FlowID> * getFlowIDEmulMapping() { return &(TaskHandler::FlowIDEmulMapping); };
  EmulInterface *getEmul(FlowID fid)           { return TaskHandler::getEmul(fid); };
  void syncStats()                             { return TaskHandler::syncStats();  };
  void freeze(FlowID fid, Time_t nCycles)      { return TaskHandler::freeze(fid, nCycles); };
  FlowID getNumActiveCores()                   { return TaskHandler::getNumActiveCores(); }; 
  void setTurboRatio(float freqCoef)           { return EmuSampler::setTurboRatio(freqCoef); };
  float getTurboRatio()                        { return EmuSampler::getTurboRatio(); };

  float getDyn(uint32_t i)       { return energyBundle->cntrs[i].getDyn();       };
  float getLkg(uint32_t i)       { return energyBundle->cntrs[i].getLkg();       };
  float getScaledLkg(uint32_t i) { return energyBundle->cntrs[i].getScaledLkg(); };
  uint32_t getCyc(uint32_t i)    { return energyBundle->cntrs[i].getCyc();       };

  FILE *dpowerf;

public:


  ChipEnergyBundle *energyBundle;

  std::vector<std::string> *sysConn;
  std::vector<std::string> *coreConn;
  std::map<float, float > *ipcRepo;
  std::map<float, std::vector<float> > *pwrCntrRepo;
  std::vector<float> *avgEnergyCntrValues;
  uint32_t nSamples;
  std::vector<float> *temperatures;
  std::vector<uint32_t> *activity;
  PowerModel();
  ~PowerModel();

  SescThermWrapper *sescThermWrapper;

  void plug(const char *section);
  void unplug();
  int  calcStats(uint64_t timeinterval, bool reusePower, FlowID fid); 
  void updateActivity(uint64_t timeinterval, FlowID fid = 0);
  void printStatus();
  void testWrapper();
  void getPowerDumpFile();
  void closePowerDumpFile();
  void dumpPerf(bool pwrReuse); 
  void dumpDynamic();
  void dumpLeakage();
  void dumpDeviceTypes();
  void dumpActivity();
  void startDump();
  void stopDump();
  double getIPC(){
    return avgTimingIPC;
  };
  double getIPC(float key){
    std::map<float, float>::iterator it = ipcRepo->find(key);
    return it->second;
  };
  float getLastTotalPower();
  float getLastDynPower();
  void updatePowerHist();
  void loadPredPower();
  bool isDeviationHigh();
  void setUsePrediction()  { usePrediction = true;  };
  void clearUsePrediction(){ usePrediction = false; };
  bool predictionStatus()  { return usePrediction;  };
  short getDownSample()    { return downSample;     }
  void adjustSamplingRate();
  void stdevOffst(bool subtract = true, float c = 0.5);
  void dumpTotalPower(const char * str);
  void energy2power();
  bool throttle(FlowID fid, uint32_t throttleLength);

  float getPowerTime()   const  { return powerTime.getDouble()/freq; };

  void updateTurboState();
  void initTurboState();
  void updatePowerTurbo(int freqState);
  float getVolRatio(int state);
  int updateFreqTurbo();
  int updateFreqDVFS_T();
  float getMaxT();
  double getFreq()                             { return freq; };
#ifdef ENABLE_CUDA
  void  setTurboRatioGPU(float freqCoef)       { return EmuSampler::setTurboRatioGPU(freqCoef); };
  void  updatePowerNTC();
  int   updateFreqNTC();
  float freqTrendNTC(float volNTC);
  float freqTrendNTC_var(float volNTC);
  float getCurrentED();
  void  setKernelId(uint32_t kId_) { kId = kId_; };
#endif

  void plugStackingValidator();
  void stackingValidator();

  void setSamplingRatio(float r) {samplingRatio = r;};

};

#endif //POWERMODEL_H
