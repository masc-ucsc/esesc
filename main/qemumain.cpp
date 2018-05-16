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

#include "BootLoader.h"
#include "EmulInterface.h"
#include "QEMUEmulInterface.h"
#include "SamplerSkipSim.h"
#include "SescConf.h"
#ifdef DEBUG
#include <mcheck.h>
#endif

#include <pthread.h>
#include <sys/time.h>

int main(int argc, const char **argv) {

  BootLoader::plug(argc, argv);

  // Prepare to boot (Similar to BootLoader::boot)
  if(!SescConf->lock())
    exit(-1);

  SescConf->dump();

  TaskHandler::getEmul(0)->startRabbit(0);

  // Run the main loop (Similar to TaskHanlder::boot but without CPU)
  timeval startTime;
  gettimeofday(&startTime, 0);

  uint64_t instcount = 0;

  bool allCPUsfinished = false;

  while(!allCPUsfinished) {
    allCPUsfinished = true; // Normally set to quit the loop
    FlowID maxFlows = TaskHandler::getMaxFlows();

    for(FlowID i = 0; i < maxFlows; i++) {
      if(!TaskHandler::isActive(i))
        continue;

      EmulInterface *eint = TaskHandler::getEmul(i);
      allCPUsfinished     = false;
      // If Flow is inactive, continue. else
      DInst *         dinst  = eint->executeHead(i);
      static int      conta  = 0;
      static AddrType lastPC = 0;
      if(dinst) {
        // dinst->dump("");
        conta++;
        if(dinst->getPC() != lastPC) {
          if(conta > 10)
            // MSG("oop=%x",lastPC);
            conta = 0;
        }
        lastPC = dinst->getPC();
        eint->reexecuteTail(i);
        // dinst->dump("");
        dinst->scrap(eint);
        instcount++;
      } else {
        conta = 0;
      }
    }
  }

  timeval endTime;
  gettimeofday(&endTime, 0);
  double msecs = (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec) / 1000;

  long double res = instcount / 1000;
  res /= msecs;
  MSG("------------------");
  MSG("Simulator performance with inst skip");
  MSG("qemumain MIPS = %g secs = %g insts = %lld", (double)res, msecs / 1000, (long long)instcount);
  MSG("------------------");

  BootLoader::report("");
  BootLoader::unplug();

  return 0;
}
