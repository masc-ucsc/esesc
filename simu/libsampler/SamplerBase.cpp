/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Ehsan K.Ardestani


This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


#include "SamplerBase.h"
#include "EmulInterface.h"
#include "SescConf.h"
#include "BootLoader.h"
#include "MemObj.h"
#include "GProcessor.h"
#include "GMemorySystem.h"

uint64_t SamplerBase::lastTime = 0;
uint64_t SamplerBase::prednsTicks = 0;
uint64_t SamplerBase::nsTicks[128] = {0};
bool     SamplerBase::justResumed[128] = {false};
uint64_t SamplerBase::local_nsticks;
bool     SamplerBase::finished[128] = {true};
uint64_t SamplerBase::pastGlobalTicks = 0;
uint64_t SamplerBase::gpuEstimatedCycles = 0;

SamplerBase::SamplerBase(const char *iname, const char *section, EmulInterface *emu, FlowID fid)
  : EmuSampler(iname, emu, fid)
  /* SamplerBase constructor {{{1 */
{
  nInstSkip   = static_cast<uint64_t>(SescConf->getDouble(section,"nInstSkip"));

  const char *pwrsection = SescConf->getCharPtr("","pwrmodel",0);

  freq      = SescConf->getDouble("technology","frequency");
  doPower = SescConf->getBool(pwrsection,"doPower",0);
  if(doPower) {
    doTherm = SescConf->getBool(pwrsection,"doTherm",0);
    interval  = static_cast<uint64_t>(SescConf->getDouble("pwrmodel","updateInterval"));
    nextCheck = nInstSkip;
  }

  local_nsticks = 0; 
  estCPI           = 1.0;

  cpiHistSize = 1;
  first = true;

  
  for (unsigned int i=0; i<cpiHistSize;i++)
    cpiHist.push_back(1.0);

  localTicksUptodate = true;
  endSimSiged = false;
  pthread_mutex_init(&mode_lock, NULL);
  lastGlobalClock = 0;
}
/* }}} */


bool SamplerBase::callPowerModel(uint64_t &ti, FlowID fid)
  /* Check if it's time to call Power/Thermal Model*/
{

  // Update metrics at each power interval, now the same for all threads
  static uint32_t nextCheck = interval;

  static std::vector<uint64_t> totalnInstPrev(emul->getNumFlows());

  uint64_t nPassedInst = totalnInst - totalnInstPrev[fid];
  if (nPassedInst>= nextCheck){ 
    uint64_t mytime  = getLocalTime();
    ti = static_cast<uint64_t> (mytime - lastTime);
    //ti = static_cast<uint64_t> (getFreq()*(mytime - lastTime)/1e9);
    I(ti>0);
    totalnInstPrev[fid] = totalnInst;
    lastTime = mytime;
    if (first) {
      first = false;
      return false;
    }
    return true;
  }
  return false;
}

SamplerBase::~SamplerBase() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

uint64_t SamplerBase::getTime() 
/* time in ns since the beginning {{{1 */
{
  double addtime = phasenInst;

  addtime *= 1.0e9; // ns
  addtime *= estCPI;
  addtime /= getFreq();
  addtime += getLocalTime();

  return static_cast<uint64_t>(addtime);
}
/* }}} */


uint64_t SamplerBase::getLocalTime(FlowID fid)  
/* time in ns since the beginning {{{1 */
{
  // predicted cpi for Rabbit and Warmup, globalClock for Detail and Timing
  I(localTicksUptodate == true);
  uint64_t nsticks; 

  FlowID lfid = fid;
  if (fid == 999)
    lfid = sFid;
  //nsticks so far (from globalClock_Timing)
  nsticks                = pastGlobalTicks;

  char str[255];
  sprintf(str, "S(%d):globalClock_Timing", lfid);
  GStats *gref = 0;
  gref = GStats::getRef(str);
  I(gref);
  uint64_t gClock = gref->getSamples();

  uint64_t clkTicks = gClock - lastGlobalClock;
  lastGlobalClock   = gClock;
  // nsticks advanced in the last sample
  nsticks          += static_cast<uint64_t>(((double)clkTicks/getFreq())*1.0e9);
  pastGlobalTicks   = nsticks;



  nsticks += nsTicks[lfid];

  return nsticks;
}
/* }}} */


uint64_t SamplerBase::getLocalnsTicks() {
  for(size_t i=0; i<emul->getNumFlows(); i++) {
    if (isActive(mapLid(i))) {
      return nsTicks[mapLid(i)];
    }
  }
  return 0;
}

void SamplerBase::addLocalTime(FlowID fid, uint64_t nsticks) 
{
  nsTicks[fid] +=  static_cast<uint64_t>(nsticks);
  local_nsticks = nsTicks[fid];
}

void SamplerBase::updateLocalTime() 
/* update local time (non timing/detail modes) in ns since the beginning {{{1 */
{
  if (lastMode == EmuTiming || lastMode == EmuDetail || lastMode == EmuInit) 
    return;

  I(stopJustCalled == false);

  double addtime = lastPhasenInst;
  addtime *= 1.0e9; // ns
  addtime *= estCPI;
  addtime /= getFreq();
  
  nsTicks[sFid] +=  static_cast<uint64_t>(addtime);
  local_nsticks = nsTicks[sFid];
  localTicksUptodate = true;
}

/* }}} */
void SamplerBase::syncStats() 
  /* make sure that all the stats are uptodate {{{1 */
{
  if (phasenInst==0 || mode == EmuInit) {
    return;
  }
  stop();
  beginTiming(mode);
}
/* }}} */

void SamplerBase::pauseThread(FlowID fid)
{
  TaskHandler::pauseThread(fid);
}

FlowID SamplerBase::getFid(FlowID last_fid)
{
  return(emul->getFid(last_fid));
}

FlowID SamplerBase::resumeThread(FlowID uid, FlowID last_fid)
{
  FlowID fid = TaskHandler::resumeThread(uid, last_fid);
  syncnSamples(fid);
  justResumed[fid] = true;
  finished[fid] = false;
  return fid;
}

FlowID SamplerBase::resumeThread(FlowID uid)
{
  return(TaskHandler::resumeThread(uid));
}

void SamplerBase::syncRunning()
{
  //TaskHandler::syncRunning();
}

void SamplerBase::terminate()
{
  TaskHandler::terminate();
}

bool SamplerBase::isActive(FlowID fid)
{
  return TaskHandler::isActive(fid);
}

int SamplerBase::isSamplerDone()
{
  if (nInstMax <= totalnInst)
    return 1;
  else
    return 0;
}

void SamplerBase::syncTimes(FlowID fid) {
  uint64_t totalnsTicks = 0;
  uint32_t cnt          = 0;

  // syncMode:
  //          's': Single, the winning Fid will decide the time
  //          'm': Max, the maximum time of all active threads decide the time
  //          'a': Avg, the average time of all the active threads decide the time
  char syncMode = 's'; 

  if (syncMode == 's'){
    totalnsTicks = nsTicks[fid];
    for(size_t i=0; i<emul->getNumFlows();i++) {
      FlowID ifid = mapLid(i);
      nsTicks[ifid] = totalnsTicks;
    }
    local_nsticks = totalnsTicks;
  }else if (syncMode == 'm'){
    for(size_t i=0; i<emul->getNumFlows();i++) {
      FlowID ifid = mapLid(i);
      if (isActive(ifid)) {
        if (nsTicks[ifid] > totalnsTicks)
          totalnsTicks = nsTicks[ifid];
      }
    }
    for(size_t i=0; i<emul->getNumFlows();i++) {
      FlowID ifid = mapLid(i);
      nsTicks[ifid] = totalnsTicks;
    }
    local_nsticks = totalnsTicks;
  }else{
    for(size_t i=0; i<emul->getNumFlows();i++) {
      FlowID ifid = mapLid(i);
      if (isActive(ifid)) {
        totalnsTicks += nsTicks[ifid];
        cnt++;
      }
    }

    uint64_t avgnsTicks = totalnsTicks/cnt;

    for(size_t i=0; i<emul->getNumFlows();i++) {
      FlowID ifid = mapLid(i);
      nsTicks[ifid] = avgnsTicks;
    }

    local_nsticks = avgnsTicks;
  }

  //MSG("synced to(%d): ticks:%lu time:%lu ", fid, local_nsticks, getLocalTime(fid));
}

void SamplerBase::syncnsTicks(FlowID fid) {
    nsTicks[fid] = local_nsticks;
}

void SamplerBase::syncnSamples(FlowID fid) { 
  nSamples[fid] = totalnSamples;
  syncnsTicks(fid);
}

void SamplerBase::getGPUCycles(FlowID fid, float ratio) {
  
    char str[255];
    sprintf(str,"P(%u):clockTicks", getsFid());
    GStats *gref = 0;
    gref = GStats::getRef(str); 
    I(gref);
    uint64_t cticks = gref->getSamples(); // clockTicks
    if (doPower)
      gpuEstimatedCycles = cticks*(ratio);
    else
      gpuEstimatedCycles = cticks;

    gpuSampledRatio = ratio;

}

FILE *SamplerBase::genReportFileNameAndOpen(const char *str) {
  FILE *fp;
  char *fname = (char *) alloca(1023);
  strcpy(fname, str);
  strcat(fname, Report::getNameID());
  fp = fopen(fname,"w");
  if(fp==0) {
    GMSG(fp==0,"ERROR: could not open file \"%s\"",fname);
    exit(-1);
  }

  return fp;
}


void SamplerBase::fetchNextMode() {
  sequence_pos++;
  if (sequence_pos >= sequence_mode.size())
    sequence_pos = 0;
  next_mode = sequence_mode[sequence_pos];
}
