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

#ifndef TASKHANDER_H
#define TASKHANDER_H

//#define ENABLE_MP 0 
#include <vector>

#include "nanassert.h"
#include "EmulInterface.h"
#include <pthread.h>

class GProcessor;

class TaskHandler {
  private:
    class EmulSimuMapping {
      public:
        FlowID         fid;
        bool           active;
        EmulInterface *emul;
        GProcessor    *simu;
    };

    typedef std::vector<EmulSimuMapping> AllMapsType;
    typedef FlowID* runningType;

    static AllMapsType allmaps;
    static bool terminate_all;

    static runningType running;
    static FlowID running_size;
    static pthread_mutex_t mutex;

    // static std::vector<bool>             active; // Is the flow active?
    static std::vector<EmulInterface *>  emulas; // associated emula
    static std::vector<GProcessor *>     cpus;   // All the CPUs in the system
  public:

    static std::vector<FlowID>           FlowIDEmulMapping;   //Which FlowIDs are associated with CPU and which with the GPUs 

    static void freeze(FlowID fid, Time_t nCycles);

    static FlowID resumeThread(FlowID uid, FlowID last_fid);
    static FlowID resumeThread(FlowID uid);
    static void removeFromRunning(FlowID fid);
    static void pauseThread(FlowID fid);
    static void syncRunning();
    static void terminate();

    static void report(const char *str);

    static void addEmul(EmulInterface *eint, FlowID fid = 0);
    static void addEmulShared(EmulInterface *eint);
    static void addSimu(GProcessor *gproc);

    static bool isTerminated() {
      return terminate_all; 
    }

    static bool isActive(FlowID fid) {
      I(fid<allmaps.size());
      return allmaps[fid].active;
    }

    static FlowID getNumActiveCores(); 

    static FlowID getMaxFlows() {
      I(emulas.size() == cpus.size());
      return emulas.size();
    }

    static EmulInterface *getEmul(FlowID fid) {
      I(fid<emulas.size());
      return emulas[fid];
    };

    static GProcessor *getSimu(FlowID fid) {
      I(fid<cpus.size());
      return cpus[fid];
    };

    static void plugBegin();
    static void plugEnd();
    static void boot();
    static void unboot();
    static void unplug();

    static void syncStats();

};


#endif
