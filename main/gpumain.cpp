/*
 ESESC: Enhanced Super ESCalar simulator
 Copyright (C) 2009 University of California, Santa Cruz.

 Contributed by  Gabriel Southern
 Jose Renau

 This file is part of ESESC.

 ESESC is free software; you can redistribute it and/or modify it under the terms
 of the GNU General Public License as published by the Free Software Foundation;
 either version 2, or (at your option) any later version.

 ESESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 PARTICULAR PURPOSE.  See the GNU General Public License for more details.

 You should  have received a copy of  the GNU General  Public License along with
 ESESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "SescConf.h"
#include "EmulInterface.h"
#include "SamplerSkipSim.h"
#include "BootLoader.h"
#ifdef DEBUG
#include <mcheck.h>
#endif

#include <pthread.h>
#include <sys/time.h>

int main(int argc, const char **argv) {

  BootLoader::plug(argc, argv);

  // Prepare to boot (Similar to BootLoader::boot)
  if (!SescConf->lock())
    exit(-1);

  SescConf->dump();

  TaskHandler::getEmul(0)->startRabbit(0);

  // Run the main loop (Similar to TaskHanlder::boot but without CPU)
  timeval startTime;
  gettimeofday(&startTime,0);

  uint64_t instcount = 0;
  uint64_t cpuinstcount = 0;
  uint64_t gpuinstcount = 0;
  FlowID maxFlows = TaskHandler::getMaxFlows();
  MSG("MaxFlows = %d",maxFlows);
  bool allCPUsfinished = false;
  MSG("_____________________________________________________________________________________________");


  int count = 0;   
  while (!allCPUsfinished) {
//  while (!TaskHandler::isTerminated()) {
    allCPUsfinished = true; // Normally set to quit the loop
    for (FlowID i = 0; i < maxFlows; i++) {
      // If Flow is inactive, continue.
      if (!TaskHandler::isActive(i))
        continue;
 
      EmulInterface *eint = TaskHandler::getEmul(i);
      DInst *dinst = eint->executeHead(i);
      count = 0;
      if (dinst) {
        eint->reexecuteTail(i);
//        dinst->dump(""); 
//        dinst->scrap(); //FIXME
        instcount++;
        if (i == 0) cpuinstcount++;
        if (i == 1) gpuinstcount++;

      }
      allCPUsfinished = false;
    }
    if (allCPUsfinished && count < 1000 ){
      allCPUsfinished = false; 
      count++;
    }
  }

  timeval endTime;
  gettimeofday(&endTime,0);
  double msecs = ( endTime.tv_sec - startTime.tv_sec ) * 1000
                   + ( endTime.tv_usec - startTime.tv_usec ) / 1000;

  long double res = instcount/1000;
  res /= msecs;
  MSG("------------------");
  MSG("Simulator performance with inst skip");
  MSG ( "qemumain MIPS = %g secs = %g insts = %lld",(double)res, msecs/1000,(long long)instcount);
  MSG ( "cpuinsts = %lld gpuinsts = %lld",(long long)cpuinstcount,(long long)gpuinstcount);
  MSG("------------------");

  BootLoader::unplug();

  return 0;
}
