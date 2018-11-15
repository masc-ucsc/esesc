// Contributed by Jose Renau
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

#ifndef TASKHANDER_H
#define TASKHANDER_H

//#define ENABLE_MP 0
#include <vector>

#include "EmulInterface.h"
#include "nanassert.h"
#include <pthread.h>

class GProcessor;

class TaskHandler {
private:
  class EmulSimuMapping {
  public:
    FlowID         fid;
    bool           active;
    bool           deactivating;
    EmulInterface *emul;
    GProcessor *   simu;
  };

  typedef std::vector<EmulSimuMapping> AllMapsType;
  typedef FlowID *                     runningType;

  static AllMapsType   allmaps;
  static volatile bool terminate_all;

  static runningType     running;
  static FlowID          running_size;
  static pthread_mutex_t mutex;
  static pthread_mutex_t mutex_terminate;

  static std::vector<EmulInterface *> emulas; // associated emula
  static std::vector<GProcessor *>    cpus;   // All the CPUs in the system

  static void removeFromRunning(FlowID fid);

public:
  static void freeze(FlowID fid, Time_t nCycles);

  static FlowID resumeThread(FlowID uid, FlowID last_fid);
  static FlowID resumeThread(FlowID uid);
  static void   pauseThread(FlowID fid);
  static void   terminate();

  static void report(const char *str);

  static void addEmul(EmulInterface *eint, FlowID fid = 0);
  static void addEmulShared(EmulInterface *eint);
  static void addSimu(GProcessor *gproc);

  static bool isTerminated() {
    return terminate_all;
  }

  static bool isActive(FlowID fid) {
    I(fid < allmaps.size());
    return allmaps[fid].active;
  }

  static FlowID getNumActiveCores();
  static FlowID getNumCores() {
    return allmaps.size();
  }

  static FlowID getNumCPUS() {
    I(cpus.size() > 0);
    return cpus.size();
  }

  static EmulInterface *getEmul(FlowID fid) {
    I(fid < emulas.size());
    return emulas[fid];
  };

  static GProcessor *getSimu(FlowID fid) {
    I(fid < cpus.size());
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
