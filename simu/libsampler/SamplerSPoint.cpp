/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau


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


#include "SamplerSPoint.h"
#include "EmulInterface.h"
#include "BootLoader.h"
#include "SescConf.h"

SamplerSPoint::SamplerSPoint(const char *iname, const char *section, EmulInterface *emu)
  : SamplerBase(iname,section, emu)
  /* SamplerSPoint constructor {{{1 */
{

  spointSize = static_cast<uint64_t>(SescConf->getDouble(section,"spointSize"));
  
  int32_t min = SescConf->getRecordMin(section,"spoint");
  int32_t max = SescConf->getRecordMax(section,"spoint");

  int32_t npoints = SescConf->getRecordSize(section,"spoint");
  if (npoints != (max-min+1)) {
    MSG("ERROR: there can not be holes in the spoints[%d:%d]",min,max);
    exit(-4);
  }

  spoint.resize(npoints);
  spweight.resize(npoints);
  LOG("NUMBER OF SIMPOINTS: %d",npoints);
  for(int32_t i=0;i<npoints;i++) {
    double start = SescConf->getDouble(section,"spoint",i);
    spoint[i] = static_cast<uint64_t>(start);
    //FIXME: figure out bug with SimPoint weights, using 1.0 for now
    spweight[i] = SescConf->getDouble(section,"SPweight",i);
    //spweight[i] = 1.0; 
    if (i>0) {
      if (spoint[i-1]>spoint[i]) {
        MSG("ERROR: non ordered simulation points spoint %d is bigger than %d",i-1,i);
        exit(-3);
      }
      if ((spoint[i-1]+spointSize)>spoint[i]) {
        MSG("ERROR: overlapping simulation points spoint %d vs %d",i-1,i);
        exit(-3);
      }
    }
  }
  const char *pwrsection = SescConf->getCharPtr("","pwrmodel",0);
  doPower = SescConf->getBool(pwrsection,"doPower",0);
  if(doPower) {
    interval = static_cast<uint64_t>(SescConf->getDouble("pwrmodel","updateInterval"));
    freq = SescConf->getDouble("technology","frequency"); 
  }

// Adjust the weights. The total should be equal to # Sim Points
  double total = 0;
  for(int32_t i = 0;i<npoints;i++) {
    total += spweight[i];
  }
  double ratio = npoints/total;
  for(int32_t i = 0;i<npoints;i++) {
    spweight[i] = ratio*spweight[i];
  }

  if (spoint.empty()) {
    MSG("ERROR: SamplerSPoint needs at least one valid interval");
    exit(-2);
  }

  spoint_pos = 0;

  nextSwitch = spoint[spoint_pos];
  if (nextSwitch==0) {
    // First instruction is the first simpoint
    nextSwitch = spointSize;
    //spoint_pos++; will be incremented later in queue
    startTiming(0);
  }else{
    startRabbit(0);
  }
}
/* }}} */

SamplerSPoint::~SamplerSPoint() 
  /* DestructorRabbit {{{1 */
{
  // Free name, but who cares 
}
/* }}} */

void SamplerSPoint::queue(uint32_t insn, uint64_t pc, uint64_t addr, uint64_t data, uint32_t fid, char op, uint64_t icount, void *env)
  /* main qemu/gpu/tracer/... entry point {{{1 */
{
  if(!execute(fid, icount))
    return; // QEMU can still send a few additional instructions (emul should stop soon)

  I(mode!=EmuInit);

  I(insn);

  // process the current sample mode
  if (nextSwitch>totalnInst) {
    if (mode == EmuRabbit)
      return;

    I(mode == EmuTiming);
    emul->queueInstruction(insn, pc, addr, data, (op&0xc0) /* thumb */, fid, env, getStatsFlag());

    static uint32_t nextCheck = interval;
    static uint64_t totalnInstPrev = 0;
    if (doPower){ // To dump power

      static uint64_t lastTime = 0;
      uint64_t nPassedInst = totalnInst - totalnInstPrev;

      if (nPassedInst>= nextCheck){ 
        totalnInstPrev = totalnInst;

        uint64_t mytime = getTime(); 
        I((mytime - lastTime) > 0);
        //printf("totalnInst:%ld, nPassedInst:%ld, interval:%ld\n", totalnInst, nPassedInst, interval);


        uint64_t timeinterval = static_cast<uint64_t> (freq*(mytime - lastTime)/1e9);
        //std::cout<<"mode "<<mode<<" Timeinterval "<<timeinterval<<" mytime "<<mytime<<" last time "<<lastTime<<"\n";  
        if (timeinterval > 0)
          BootLoader::getPowerModelPtr()->calcStats(timeinterval, 
              !(mode == EmuTiming), fid);
        lastTime = mytime;
      }
    }// doPower
    return;
  }

  // Look for the new mode
  I(nextSwitch <= totalnInst);

  if (mode == EmuRabbit) {
    nextSwitch += spointSize;
    stop();
    startTiming(fid);
    return;
  }

  I(mode == EmuTiming);

  //stop(spweight[spoint_pos]); 
  stop(); 
  if (doPower)
    BootLoader::startReportOnTheFly();

  spoint_pos++;
  // We did enough
  if (spoint_pos >= spoint.size()) {
    markDone();
    return;
  }
  nextSwitch       = spoint[spoint_pos];
  I(totalnInst< nextSwitch);

  startRabbit(fid);
}
/* }}} */

void SamplerSPoint::syncStats() 
  /* make sure that all the stats are uptodate {{{1 */
{
  if (phasenInst==0 || mode == EmuInit) {
    return;
  }
  stop(); // Add the simpoint weight if mode == EmuTiming, 

  beginTiming(mode);
}
/* }}} */


