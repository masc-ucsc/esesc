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

#include "TaskHandler.h"

#include "EmuSampler.h"
#include "EmulInterface.h"
#include "GProcessor.h"
#include "Report.h"
#include "SescConf.h"
#include <iostream>
#include <string.h>
/* }}} */

std::vector<TaskHandler::EmulSimuMapping> TaskHandler::allmaps;
volatile bool                             TaskHandler::terminate_all;
pthread_mutex_t                           TaskHandler::mutex;
pthread_mutex_t                           TaskHandler::mutex_terminate;

FlowID *TaskHandler::running;
FlowID  TaskHandler::running_size;

std::vector<EmulInterface *> TaskHandler::emulas; // associated emula
std::vector<GProcessor *>    TaskHandler::cpus;   // All the CPUs in the system

void TaskHandler::report(const char *str) {
  /* dump statistics to report file {{{1 */

  Report::field("OSSim:nCPUs=%d", cpus.size());
  size_t cpuid        = 0;
  size_t cpuid_sub    = 0;
  FlowID samplercount = 0;

  for(size_t i = 0; i < emulas.size(); i++) {
    Report::field("OSSim:P(%d)_Sampler=%d", cpuid, (emulas[i]->getSampler())->getsFid());
    if((emulas[i]->getSampler())->getsFid() > samplercount) {
      samplercount = (emulas[i]->getSampler())->getsFid();
    }
    Report::field("OSSim:P(%d)_Type=%d", cpuid, emulas[i]->cputype);

    cpuid_sub++;
    // if (cpuid_sub>=cpus[cpuid]->getMaxFlows()) {
    cpuid_sub = 0;
    GI((cpus.size() > 1), (cpuid < cpus.size()));
    cpuid = cpuid + 1;
    //}
  }
  Report::field("OSSim:nSampler=%d", samplercount + 1);
  Report::field("OSSim:globalClock=%lld", globalClock);

  /*
   *
     for(size_t i=0;i<cpus.size();i++) {
     cpus[i]->report(str);
     }*/
}
/* }}} */

void TaskHandler::addEmul(EmulInterface *eint, FlowID fid) {
  /* add a new emulator to the system {{{1 */

  FlowID nemul = SescConf->getRecordSize("", "cpuemul");

  if(emulas.empty()) {
    for(FlowID i = 0; i < nemul; i++) {
      emulas.push_back(0x0);
    }
  }

  I(!emulas.empty());

  emulas.erase(emulas.begin() + fid);
  emulas.insert(emulas.begin() + fid, eint);
}
/* }}} */

void TaskHandler::addEmulShared(EmulInterface *eint) {
  /* add a new emulator to the system {{{1 */

  FlowID nemul = SescConf->getRecordSize("", "cpuemul");

  if(emulas.empty()) {
    for(FlowID i = 0; i < nemul; i++) {
      emulas.push_back(0x0);
    }
  }

  I(!emulas.empty());

  for(FlowID i = 0; i < nemul; i++) {
    const char *section     = SescConf->getCharPtr("", "cpuemul", i);
    const char *eintsection = eint->getSection();
    if(strcasecmp(section, eintsection) == 0) {
      emulas.erase(emulas.begin() + i);
      emulas.insert(emulas.begin() + i, eint);
    }
  }

  /*
  // IF DEBUG
  for(FlowID i=0;i<nemul;i++)
  {
    if (emulas.at(i) != 0x0)
      MSG("Emul[%d] = %s", i, (emulas.at(i))->getSection());
  }
  */
}
/* }}} */

void TaskHandler::addSimu(GProcessor *gproc) {
  /* add a new simulator to the system {{{1 */
  I(cpus.size() == static_cast<size_t>(gproc->getID()));
  cpus.push_back(gproc);

  EmulSimuMapping map;

  map.fid          = gproc->getID();
  map.emul         = 0;
  map.simu         = gproc;
  map.active       = gproc->isActive();
  map.deactivating = false;

  allmaps.push_back(map);
}
/* }}} */

FlowID TaskHandler::resumeThread(FlowID uid, FlowID fid) {
  /* activate fid {{{1 */

  // I(uid<65535); // No more than 65K threads for the moment
  pthread_mutex_lock(&mutex);

  I(allmaps[fid].fid == fid);
  I(!allmaps[fid].active);

  // IS(MSG("TaskHandler::CPU resumeThread(%d)",fid));

  if(allmaps[fid].active || terminate_all) {
    if(terminate_all)
      MSG("TaskHandler::terminate_all flag is set, cannot start Thread (%d)", fid);
    /*
        fprintf(stderr,"CPUResume(%d): running_size = %d : running->",fid,running_size);
        for (int i = 0; i < running_size; i++)
          fprintf(stderr,"%d->",running[i]);
        fprintf(stderr,"\n");

    */
    pthread_mutex_unlock(&mutex);
    return (0);
  }
  allmaps[fid].active       = true;
  allmaps[fid].deactivating = false;
  allmaps[fid].simu->setActive();
  bool found = false;
  for(int i = 0; i < running_size; i++) {
    if(running[i] == fid) {
      found = true;
      break;
    }
  }

  if(!found) {
    allmaps[fid].active = true;
    I(running_size < allmaps.size());
    running[running_size] = fid;
    running_size++;
  }

#ifdef DEBUG2
  fprintf(stderr, "CPUResume: fid=%d running_size=%d running=", fid, running_size);
  for(int i = 0; i < running_size; i++)
    fprintf(stderr, "%d:", running[i]);
  fprintf(stderr, "\n");
#endif

  pthread_mutex_unlock(&mutex);
  return (fid);
}

/* }}} */

FlowID TaskHandler::resumeThread(FlowID fid) {
  /* activate fid, for GPU flow {{{1 */

  I(fid < 65535); // No more than 65K flows for the moment

  pthread_mutex_lock(&mutex);
  for(size_t i = 0; i < running_size; i++) {
    if(running[i] == fid) {
      // MSG("Was in queue");
      allmaps[fid].active       = true;
      allmaps[fid].deactivating = false;
      allmaps[fid].simu->setActive();
      /*
      fprintf(stderr,"GPUResume: running_size = %d : running->",running_size);
      for (int i = 0; i < running_size; i++)
        fprintf(stderr,"%d->",running[i]);
      fprintf(stderr,"\n");
      */
      pthread_mutex_unlock(&mutex);
      return (fid);
    }
  }

  pthread_mutex_unlock(&mutex);

  FlowID GPU_fid = (fid);
  I(allmaps[GPU_fid].fid == GPU_fid);

  pthread_mutex_lock(&mutex);
  //  if (allmaps[GPU_fid].active || terminate_all){
  if(terminate_all) {
    MSG("TaskHandler::terminate_all flag is set, cannot start Thread (%d)", fid);
    pthread_mutex_unlock(&mutex);
    return (999);
  } else if(allmaps[GPU_fid].active) {
    MSG("TaskHandler::fid(%d) is already active", GPU_fid);
    pthread_mutex_unlock(&mutex);
    return (GPU_fid);
  }

  allmaps[GPU_fid].active       = true;
  allmaps[GPU_fid].deactivating = false;
  allmaps[GPU_fid].simu->setActive();

  // Make sure that the fid is not in the running queue
  // This might happen if the execution is very slow.

  running[running_size] = GPU_fid;
  running_size++;

  fprintf(stderr, "CPUResume: fid=%d running_size=%d running=", fid, running_size);
  for(int i = 0; i < running_size; i++)
    fprintf(stderr, "%d:", running[i]);
  fprintf(stderr, "\n");

  //  allmaps[GPU_fid].emul->getSampler()-> startMode(GPU_fid);
  pthread_mutex_unlock(&mutex);
  return (GPU_fid);
}
/* }}} */

void TaskHandler::removeFromRunning(FlowID fid) {
  /* remove fid from the running queue {{{1 */

  I(allmaps[fid].simu->isROBEmpty());

  for(size_t i = 0; i < running_size; i++) {
    if(running[i] != fid)
      continue;

    if(i < running_size - 1) {
      for(size_t j = i; j < running_size - 1; j++) {
        running[j] = running[j + 1];
      }
    }
    running_size--;

#ifdef DEBUG2
    fprintf(stderr, "removeFromRunning: fid=%d running_size=%d : running=", fid, running_size);
    for(int j = 0; j < running_size; j++)
      fprintf(stderr, "%d:", running[j]);
    fprintf(stderr, "\n");
#endif

    break;
  }
}
/* }}} */

void TaskHandler::pauseThread(FlowID fid) {
  /* deactivae an fid {{{1 */

  fprintf(stderr, "P");
  I(allmaps[fid].fid == fid);
  I(fid < 65535);
  if(terminate_all)
    return;

  pthread_mutex_lock(&mutex);

  if(!allmaps[fid].active) {
    // MSG("TaskHandler::pauseThread(%d) not needed, since it is not active",fid);
    if(allmaps[fid].deactivating) {
      if(allmaps[fid].simu->isROBEmpty()) {
        removeFromRunning(fid);
        allmaps[fid].active       = false;
        allmaps[fid].deactivating = false; // already deactivated
        allmaps[fid].simu->clearActive();
      }
    }
    pthread_mutex_unlock(&mutex);
    return;
  }

  allmaps[fid].active       = false; // So that no more populate is called
  allmaps[fid].deactivating = true;

  pthread_mutex_unlock(&mutex);
}
/* }}} */

void TaskHandler::terminate()
/* deactivae an fid {{{1 */
{
  terminate_all = true;

  MSG("TaskHandler::terminate");

  for(size_t i = 0; i < allmaps.size(); i++) {
    if(!allmaps[i].active)
      continue;
    if(allmaps[i].emul)
      allmaps[i].emul->getSampler()->stop();
    allmaps[i].active       = false;
    allmaps[i].deactivating = false;
  }

  // GStats::stopAll(1);

  running_size = 0;

  // LOCK thread until TaskHandler::unplug is called (otherwise, there could be a race)
  fflush(stdout);
  fflush(stderr);
  pthread_mutex_lock(&mutex_terminate);
}
/* }}} */

void TaskHandler::freeze(FlowID fid, Time_t nCycles) {
  /* put a core to sleep (thermal emergency?) fid {{{1 */
  I(allmaps[fid].active);
  I(fid < 65535); // No more than 65K threads for the moment
  allmaps[fid].simu->freeze(nCycles);
}
/* }}} */

extern "C" void helper_esesc_dump();
void            TaskHandler::boot()
/* main simulation loop {{{1 */
{
  while(!terminate_all) {
    if(unlikely(running_size == 0)) {
      bool needIncreaseClock = false;
      for(AllMapsType::iterator it = allmaps.begin(); it != allmaps.end(); it++) {
        if(it->emul == 0)
          continue;
        EmuSampler::EmuMode m = (*it).emul->getSampler()->getMode();
        if(m == EmuSampler::EmuDetail || m == EmuSampler::EmuTiming) {
          needIncreaseClock = true;
          break;
        }
      }
      if(needIncreaseClock)
        EventScheduler::advanceClock();
    } else {
      // 1st Make sure that they have enough instructions
      bool one_failed;
      bool all_failed;
      bool retry = false;
      do {
#ifdef DEBUG
        static int conta = 0;
        conta++;
        if(conta > 1000000) {
          fprintf(stderr, ".");
          conta = 0;
        }
#endif
        do {
          one_failed = false;
          all_failed = true;
          for(size_t i = 0; i < running_size; i++) {
            FlowID fid = running[i];
            if(allmaps[fid].emul == 0) {
              all_failed = false;
              continue;
            }
            bool p = allmaps[fid].emul->populate(fid);
            if(!p)
              one_failed = true;
            else
              all_failed = false;
          }
        } while(all_failed && running_size);
        if(!one_failed)
          break;
        if(retry) {
          for(size_t i = 0; i < running_size; i++) {
            FlowID fid = running[i];
            if(!allmaps[fid].active) {
              if(!allmaps[fid].deactivating)
                removeFromRunning(fid);
              continue;
            }

            if(allmaps[fid].emul) {
              bool p = allmaps[fid].emul->populate(fid);
              if(!p)
                pauseThread(fid);
            }
          }
          break;
        }
        retry = true;
      } while(running_size);
      // 2nd: advance cores
      for(size_t i = 0; i < running_size; i++) {
        FlowID fid = running[i];
        if(allmaps[fid].deactivating) {
#ifdef DEBUG
          if(!allmaps[fid].simu->isROBEmpty()) {
            MSG("@%lld drain fid:%d rob:%d", globalClock, fid, allmaps[fid].simu->getROBsize());
          }
#endif
          allmaps[fid].simu->drain();
          if(allmaps[fid].simu->isROBEmpty()) {
            pauseThread(fid);
            I(!allmaps[fid].deactivating || terminate_all);
          }
        } else {
          allmaps[fid].simu->advance_clock(fid);
        }
      }
#ifdef DEBUG
      for(size_t i = 0; i < allmaps.size(); i++) {
        if(allmaps[i].active)
          continue;
        if(allmaps[i].deactivating)
          continue;
        I(allmaps[i].simu->isROBEmpty());
      }
#endif
      EventScheduler::advanceClock();
    }
  }
}
/* }}} */

void TaskHandler::unboot()
/* nothing to do {{{1 */
{
}
/* }}} */

void TaskHandler::plugBegin()
/* allocate objects {{{1 */
{
  I(emulas.empty());
  I(cpus.empty());
  terminate_all = false;

  running      = NULL;
  running_size = 0;

  pthread_mutex_lock(&mutex_terminate);
}
/* }}} */

void TaskHandler::plugEnd()
/* setup running and allmaps before starting the main loop {{{1 */
{
  size_t nCPUThreads = 0;
  for(size_t i = 0; i < cpus.size(); i++) {
    nCPUThreads += 1; // cpus[i]->getMaxFlows();
  }
  if(emulas.size() > nCPUThreads) {
    MSG("ERROR: There are more emul (%zu) than cpu flows (%zu) available. Increase the number of cores or emulas can starve",
        emulas.size(), nCPUThreads);
    SescConf->notCorrect();
  } else if(emulas.size() < nCPUThreads) {
    if(emulas.size() != 0)
      MSG("Warning: There are more cores than threads (%zu vs %zu). Powering down unusable cores", emulas.size(), nCPUThreads);
  }

  // Tie the emulas to the all maps
  size_t cpuid     = 0;
  size_t cpuid_sub = 0;

  for(size_t i = 0; i < emulas.size(); i++) {
    allmaps[i].fid  = static_cast<FlowID>(i);
    allmaps[i].emul = emulas[i];
    I(allmaps[i].simu == cpus[cpuid]);

    if(i == 0) {
      I(allmaps[i].active == true); // active by default
    } else {
      I(allmaps[i].active == false);
    }

    allmaps[i].simu->setEmulInterface(emulas[i]);

    cpuid_sub++;
    // if (cpuid_sub>=cpus[cpuid]->getMaxFlows()) {
    cpuid_sub = 0;
    I(cpuid < cpus.size());
    cpuid = cpuid + 1;
    // }
  }
  for(size_t i = 0; i < allmaps.size(); i++) {
    if(allmaps[i].active)
      running_size++;
  }
  I(running_size > 0);
  /*************************************************/

  running         = new FlowID[allmaps.size()];
  int running_pos = 0;
  for(size_t i = 0; i < allmaps.size(); i++) {
    if(allmaps[i].emul)
      allmaps[i].emul->start();
    if(allmaps[i].active)
      running[running_pos++] = i;
  }
  I(running_pos == running_size);
}
/* }}} */

void TaskHandler::unplug()
/* delete objects {{{1 */
{
#ifdef WAVESNAP_EN
  for(size_t i = 0; i < cpus.size(); i++) {
    if(i == 0) {
      std::cout << "Done! Getting wavesnap info." << std::endl;
      if(SINGLE_WINDOW) {
        cpus[i]->snap->calculate_single_window_ipc();
      } else {
        cpus[i]->snap->calculate_ipc();
        cpus[i]->snap->window_frequency();
      }
    }
  }
#endif

#if 0
  for(size_t i=0; i<cpus.size() ; i++) {
    delete cpus[i];
  }

  for(size_t i=0; i<emulas.size() ; i++) {
    delete emulas[i];
  }
#endif

  pthread_mutex_unlock(&mutex_terminate);
}
/* }}} */

FlowID TaskHandler::getNumActiveCores() {
  FlowID numActives = 0;
  for(size_t i = 0; i < allmaps.size(); i++) {
    if(allmaps[i].active)
      numActives++;
  }
  return numActives;
}
