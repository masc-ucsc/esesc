
/* License & includes {{{1 */
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
#include "TaskHandler.h"

#include "SescConf.h"
#include "GProcessor.h"
#include "Report.h"
#include "EmuSampler.h"
#include "EmulInterface.h"
#include <string.h>
/* }}} */

std::vector<TaskHandler::EmulSimuMapping >   TaskHandler::allmaps;
bool TaskHandler::terminate_all;
pthread_mutex_t TaskHandler::mutex;

FlowID* TaskHandler::running;
FlowID TaskHandler::running_size;

std::vector<EmulInterface *>  TaskHandler::emulas; // associated emula
std::vector<GProcessor *>     TaskHandler::cpus;   // All the CPUs in the system
std::vector<FlowID>           TaskHandler::FlowIDEmulMapping;  


void TaskHandler::report(const char *str) {
  /* dump statistics to report file {{{1 */

  Report::field("OSSim:nCPUs=%d",cpus.size());
  size_t cpuid     = 0;
  size_t cpuid_sub = 0;
  FlowID samplercount = 0;

  for(size_t i = 0;i<emulas.size();i++) {
    Report::field("OSSim:P(%d)_Sampler=%d",cpuid,(emulas[i]->getSampler())->getsFid());
    if ((emulas[i]->getSampler())->getsFid() > samplercount){
      samplercount = (emulas[i]->getSampler())->getsFid();
    }
    Report::field("OSSim:P(%d)_Type=%d",cpuid,emulas[i]->cputype);

    cpuid_sub++;
    if (cpuid_sub>=cpus[cpuid]->getMaxFlows()) {
      cpuid_sub = 0;
      cpuid     = cpuid+1;
      I(cpuid < cpus.size());
    }
  }
  Report::field("OSSim:nSampler=%d",samplercount+1);
  Report::field("OSSim:globalClock=%lld",globalClock);



  /*
   *
     for(size_t i=0;i<cpus.size();i++) {
     cpus[i]->report(str);
     }*/

}
/* }}} */

void TaskHandler::addEmul(EmulInterface *eint, FlowID fid) {
  /* add a new emulator to the system {{{1 */


  /* Commented out by Alamelu, who thinks this does not support 
     interleaving


  // By default, all the threads are active. If there are no
  // instructions, blui.poweroff would be called when execute is
  // called the first time
  int num_flows = eint->getNumFlows();

  // Set the starting offset for the EmulInterface
  //eint->setFirstFlow(emulas.size()); //FIXME : Will not work when we have a GPU (A GPU has 1 interface, many flows);

  for(int i=0;i<num_flows;i++) {// FIXME: Do we need to keep eint per flow? For CPU we only have one eint.
  emulas.push_back(eint);
  }

   */

  FlowID nemul = SescConf->getRecordSize("","cpuemul");

  if (emulas.empty()){
    for(FlowID i=0;i<nemul;i++) {
      emulas.push_back(0x0);
    }
  }

  I(!emulas.empty());

  emulas.erase (emulas.begin()+fid);
  emulas.insert(emulas.begin()+fid, eint);
    
  

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

void TaskHandler::addEmulShared(EmulInterface *eint) {
  /* add a new emulator to the system {{{1 */


  /* Commented out by Alamelu, who thinks this does not support 
     interleaving


  // By default, all the threads are active. If there are no
  // instructions, blui.poweroff would be called when execute is
  // called the first time
  int num_flows = eint->getNumFlows();

  // Set the starting offset for the EmulInterface
  //eint->setFirstFlow(emulas.size()); //FIXME : Will not work when we have a GPU (A GPU has 1 interface, many flows);

  for(int i=0;i<num_flows;i++) {// FIXME: Do we need to keep eint per flow? For CPU we only have one eint.
  emulas.push_back(eint);
  }

   */

  FlowID nemul = SescConf->getRecordSize("","cpuemul");

  if (emulas.empty()){
    for(FlowID i=0;i<nemul;i++) {
      emulas.push_back(0x0);
    }
  }

  I(!emulas.empty());

  for(FlowID i=0;i<nemul;i++) {
    const char *section = SescConf->getCharPtr("","cpuemul",i);
    const char *eintsection   = eint->getSection(); 
    if(strcasecmp(section,eintsection) == 0 ) {
      emulas.erase (emulas.begin()+i);
      emulas.insert(emulas.begin()+i, eint);
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
  I(cpus.size() == static_cast<size_t>(gproc->getId()));
  cpus.push_back(gproc);
}
/* }}} */

FlowID TaskHandler::resumeThread(FlowID uid, FlowID fid) {
  /* activate fid {{{1 */

  //I(uid<65535); // No more than 65K threads for the moment
  pthread_mutex_lock (&mutex);

  //FlowID fid = emulas[0]->getFid(last_fid); 

  I(allmaps[fid].fid == fid);
  I(!allmaps[fid].active);

  //IS(MSG("TaskHandler::CPU resumeThread(%d)",fid));

  if (allmaps[fid].active || terminate_all){
    if (terminate_all) 
      MSG("TaskHandler::terminate_all flag is set, cannot start Thread (%d)",fid);
/*
    fprintf(stderr,"CPUResume(%d): running_size = %d : running->",fid,running_size);
    for (int i = 0; i < running_size; i++)
      fprintf(stderr,"%d->",running[i]);
    fprintf(stderr,"\n");

*/
    pthread_mutex_unlock (&mutex);
    return (0);
  }
  allmaps[fid].active = true;
  allmaps[fid].simu->setActive();
  running[running_size] = fid;
  running_size++;
/*
  fprintf(stderr,"CPUResume: running_size = %d : running->",running_size);
  for (int i = 0; i < running_size; i++)
    fprintf(stderr,"%d->",running[i]);
  fprintf(stderr,"\n");
*/

  pthread_mutex_unlock (&mutex);
  return (fid);
}

/* }}} */

FlowID TaskHandler::resumeThread(FlowID fid) {
  /* activate fid, for GPU flow {{{1 */

  I(fid<65535); // No more than 65K flows for the moment

  pthread_mutex_lock (&mutex);
  for (size_t i = 0; i < running_size; i++){
    if (running[i] == fid){
      //MSG("Was in queue");
      allmaps[fid].active = true;
      allmaps[fid].simu->setActive();
      /*
      fprintf(stderr,"GPUResume: running_size = %d : running->",running_size);
      for (int i = 0; i < running_size; i++)
        fprintf(stderr,"%d->",running[i]);
      fprintf(stderr,"\n");
      */
      pthread_mutex_unlock (&mutex);
      return (fid);
    }
  }

  pthread_mutex_unlock (&mutex);

  FlowID GPU_fid = (fid);
  I(allmaps[GPU_fid].fid == GPU_fid);

  //IS(MSG("TaskHandler::GPU resumeThread(%d)",GPU_fid));

  pthread_mutex_lock (&mutex);
  //  if (allmaps[GPU_fid].active || terminate_all){
  if (terminate_all){
    MSG("TaskHandler::terminate_all flag is set, cannot start Thread (%d)",fid);
    pthread_mutex_unlock (&mutex);
/*
    fprintf(stderr,"GPUResume: running_size = %d : running->",running_size);
    for (int i = 0; i < running_size; i++)
      fprintf(stderr,"%d->",running[i]);
    fprintf(stderr,"\n");
*/
    return (999);
  } else if(allmaps[GPU_fid].active){
    MSG("TaskHandler::fid(%d) is already active",GPU_fid);
    pthread_mutex_unlock (&mutex);
/*
    fprintf(stderr,"GPUResume: running_size = %d : running->",running_size);
    for (int i = 0; i < running_size; i++)
      fprintf(stderr,"%d->",running[i]);
    fprintf(stderr,"\n");
*/
    return (GPU_fid);
  }  



  allmaps[GPU_fid].active = true;
  allmaps[GPU_fid].simu->setActive();

  // Make sure that the fid is not in the running queue
  // This might happen if the execution is very slow. 
  
   running[running_size] = GPU_fid;
   running_size++;

/*
   fprintf(stderr,"GPUResume: running_size = %d : running->",running_size);
   for (int i = 0; i < running_size; i++)
     fprintf(stderr,"%d->",running[i]);
   fprintf(stderr,"\n");
*/

//  allmaps[GPU_fid].emul->getSampler()-> startMode(GPU_fid);
  pthread_mutex_unlock (&mutex);
  return (GPU_fid);
}
/* }}} */

void TaskHandler::syncRunning(){
  /* syncs the running queue with emul  {{{1 */
  pthread_mutex_lock (&mutex);

  for(size_t i=0; i<allmaps.size(); i++)
  {
    if (!allmaps[i].active)
    {
      I(0);
      removeFromRunning(i);
    }
  }
  pthread_mutex_unlock (&mutex);
}
/* }}} */


void TaskHandler::removeFromRunning(FlowID fid){
  /* remove fid from the running queue {{{1 */
  //pthread_mutex_lock (&mutex);
  for (size_t i=0;i<running_size;i++){
    if (running[i] == fid){
      if (i < running_size-1)
        //(memmove((void *)running[i],(const void *)running[i+1],(running_size-(i+1))*sizeof(FlowID)));
        for (size_t j = i; j<running_size-1; j++){
          running[j] = running[j+1];
        }
      running_size--;
      i = running_size+10;
    }
  }

  if (allmaps[fid].active){
    //emulas[FlowIDEmulMapping.at(fid)]->freeFid(fid); // FIXME: no vector for emulas?
    emulas[fid]->freeFid(fid); // FIXME: no vector for emulas?
  }
}
/* }}} */

void TaskHandler::pauseThread(FlowID fid){
  /* deactivae an fid {{{1 */

  I(allmaps[fid].fid == fid);
  I(fid<65535);

  pthread_mutex_lock (&mutex);

  if (!allmaps[fid].active){
    //MSG("TaskHandler::pauseThread(%d) not needed, since it is not active",fid);
    pthread_mutex_unlock (&mutex);
    return;
  }

  //IS(MSG("TaskHandler::pauseThread(%d)",fid));
  //running_size--;
  removeFromRunning(fid);
  allmaps[fid].active = false;
  allmaps[fid].simu->clearActive();
  pthread_mutex_unlock (&mutex);
}
/* }}} */

void TaskHandler::terminate() 
  /* deactivae an fid {{{1 */
{
  terminate_all = true;

  //MSG("TaskHandler::terminate");

  for(size_t i = 0; i<allmaps.size();i++) {
    if (!allmaps[i].active)
      continue;
    allmaps[i].active = false;
  }

  
  //GStats::stopAll(1);

  pthread_mutex_lock (&mutex);
  running_size = 0;
  pthread_mutex_unlock (&mutex);

}
/* }}} */

void TaskHandler::freeze(FlowID fid, Time_t nCycles) {
  /* put a core to sleep (thermal emergency?) fid {{{1 */
  I(allmaps[fid].active);
  I(fid<65535); // No more than 65K threads for the moment
  allmaps[fid].simu->freeze(nCycles);
}
/* }}} */

void TaskHandler::boot()
  /* main simulation loop {{{1 */
{
  while(!terminate_all) {
    for(size_t i =0;i<running_size;i++) {
      FlowID fid = running[i];
      GProcessor *simu = allmaps[fid].simu;
      simu->fetch(fid);
      simu->execute();
    }
    if (running_size == 0) {
      bool needIncreaseClock = false;
      for(AllMapsType::iterator it=allmaps.begin();it!=allmaps.end();it++) {
        EmuSampler::EmuMode m = (*it).emul->getSampler()->getMode();
        if (m == EmuSampler::EmuDetail || m == EmuSampler::EmuTiming) {
          needIncreaseClock = true;
          break;
        }
      }
      if (needIncreaseClock)
        EventScheduler::advanceClock();
    }else{
      EventScheduler::advanceClock();
    }
  }
  syncStats();
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

  running = NULL;
  running_size = 0;
}
/* }}} */

void TaskHandler::plugEnd()
  /* setup running and allmaps before starting the main loop {{{1 */
{
  size_t nCPUThreads = 0;
  for(size_t i = 0;i<cpus.size();i++) {
    nCPUThreads += cpus[i]->getMaxFlows();
  }
  if( emulas.size() > nCPUThreads ) {
    MSG("ERROR: There are more emul (%zu) than cpu flows (%zu) available. Increase the number of cores or emulas can starve", emulas.size(), nCPUThreads);
    SescConf->notCorrect();
  }else if(emulas.size() < nCPUThreads) {
    MSG("Warning: There are more cores than threads (%zu vs %zu). Powering down unusable cores", emulas.size(), nCPUThreads);
    //FIXME: Where are they actually powered down?
  }

  allmaps.resize(emulas.size());

  size_t cpuid     = 0;
  size_t cpuid_sub = 0;

  /*************************************************/
  /* 
  FIXME: This loop will be buggy in the case of a cpu 
  with maxFlows > 1 and interleaved interfaces
  */

  for(size_t i = 0;i<emulas.size();i++) {
    allmaps[i].fid    = static_cast<FlowID>(i);
    allmaps[i].emul   = emulas[i];
    allmaps[i].simu   = cpus[cpuid];
    if (i == 0){
      allmaps[i].active = true;      // active by default
    } else {
      allmaps[i].active = false;
    }

    running_size = 1; //Only Thread 0 i started initially
    allmaps[i].simu->setEmulInterface(emulas[i]);
    
    cpuid_sub++;
    if (cpuid_sub>=cpus[cpuid]->getMaxFlows()) {
      cpuid_sub = 0;
      I(cpuid < cpus.size());
      cpuid     = cpuid+1;
    }
  }
  /*************************************************/

  //running = new FlowID[allmaps.size()];
  running = new FlowID[100];
  for(size_t i = 0;i<emulas.size();i++) {
    allmaps[i].emul->start();
    running[i] = i;
  }


}
/* }}} */

void TaskHandler::unplug() 
  /* delete objects {{{1 */
{
#if 0
  for(size_t i=0; i<cpus.size() ; i++) {
    delete cpus[i];
  }

  for(size_t i=0; i<emulas.size() ; i++) {
    delete emulas[i];
  }
#endif
}
/* }}} */

void TaskHandler::syncStats()
  /* call all the emuSampler to sync stats {{{1 */
{
  if (terminate_all)
    return;
  for(AllMapsType::iterator it=allmaps.begin();it!=allmaps.end();it++) {
    (*it).emul->getSampler()->syncStats();
  }
}
/* }}} */

FlowID TaskHandler::getNumActiveCores() {
  FlowID numActives = 0;
  for(size_t i = 0; i<allmaps.size();i++) {
    if (allmaps[i].active)
      numActives ++;
  }
  return numActives;

}
