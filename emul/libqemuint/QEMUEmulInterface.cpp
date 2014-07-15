/*
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by Jose Renau
Gabriel Southern

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

#include <string.h>

#include "QEMUEmulInterface.h"
#include "QEMUInterface.h"
#include "SescConf.h"

  std::vector<FlowID> QEMUEmulInterface::fidFreePool; // 0 not available for this core type, 
                                                      // 1 free
                                                      // 2 taken
  std::map<FlowID, FlowID> QEMUEmulInterface::fidMap;
  QEMUEmulInterface::QEMUEmulInterface(const char *section) 
: EmulInterface(section) 
{
  
  
  nEmuls = SescConf->getRecordSize("","cpuemul");
  if (fidFreePool.size() == 0)
  {
    nFlows = 0; 
    const char* emultype = SescConf->getCharPtr(section,"type");
    for(FlowID i=0;i<nEmuls;i++) { 
      const char *section = SescConf->getCharPtr("","cpuemul",i);
      const char *type    = SescConf->getCharPtr(section,"type");
      if(strcasecmp(type,emultype) == 0 ) {        
        fidFreePool.push_back((i==0)? FID_TAKEN : FID_FREE); // insert as available, fid zero is already taken by qemu
        fidMap[nFlows++] = i;

      }else{
        fidFreePool.push_back(FID_NOT_AVAILABLE); // insert as "not for QEMU"
      }
    }
  }else{
    nFlows = 0; 
    const char* emultype = SescConf->getCharPtr(section,"type");
    for(FlowID i=0;i<nEmuls;i++) { 
      const char *section = SescConf->getCharPtr("","cpuemul",i);
      const char *type    = SescConf->getCharPtr(section,"type");
      if(strcasecmp(type,emultype) == 0 ) {        
        nFlows++;
      }
    }
  }
  
  QEMUArgs *qargs = (QEMUArgs *) malloc(sizeof(QEMUArgs));
  int qargpos = 0;
  int qargc = 1;
  bool dorun = SescConf->getBool(section,"dorun");
  if (!dorun) {
    nFlows = 0;
    return;
  }

  int paramcount = SescConf->getRecordSize(section,"params");

  for(int j = 0; j < paramcount; j++) {
    char *s = strdup(SescConf->getCharPtr(section,"params",j));
    qargc++;
    while(*s) {
      if(isspace(*s))
        qargc++;
      while(isspace(*s))
        s++;
      s++;
    }
  }

  qargs->qargc = qargc;
  qargs->qargv = (char **)malloc(qargs->qargc*sizeof(char*));
  qargs->qargv[qargpos++] = (char *) "qemu";
  for(int j = 0; j < paramcount; j++) {
    char *param = strdup(SescConf->getCharPtr(section,"params",j));
    char *splitparam = strtok(param, " ");
    while(splitparam != NULL) {
      qargs->qargv[qargpos++] = strdup(splitparam);
      splitparam = strtok(NULL," ");
    }
  }

  firstassign = 1; // zero is already assigned as FID
  if (firstassign >= fidFreePool.size())
    firstassign = 0;

  //MSG("QEMUEmulInterface.cpp : nFlows = %d",nFlows);
  reader = new QEMUReader(qargs, section, this);
}

QEMUEmulInterface::~QEMUEmulInterface() 
{
  delete reader;
  reader = 0;
}

FlowID QEMUEmulInterface::getFid(FlowID last_fid)
/* Get a new fid, tries to use the same last_fid if possible/applicable */
{
  //pthread_mutex_lock(&mutex);
  I(fidFreePool.size() == nEmuls); 

  if (last_fid == FID_NULL) {
    // If it is the first time, try to assign in round robin
    if (fidFreePool[firstassign] != FID_TAKEN) {
      fidFreePool[firstassign] = FID_TAKEN;
      int i= firstassign;
      firstassign++;
      if (firstassign >= fidFreePool.size())
        firstassign = 0;

      return i;
    }
  }

  FlowID fid2 = FID_NULL;
  for(size_t i=0; i<nEmuls ; i++){ // Search for fids that are used and freed recently
    if (fidFreePool[i] == FID_FREED) {
      if (fid2 == FID_NULL) fid2 = i;
      if (last_fid != i) continue;
      fidFreePool[i] = FID_TAKEN;
      //pthread_mutex_unlock (&mutex);
      return i;
    }
  }
  FlowID fid1 = FID_NULL;
  for(size_t i=0; i<nEmuls ; i++){ 
    if (fidFreePool[i] == FID_FREE) {
      if (fid1 == FID_NULL) fid1 = i;
      if (last_fid != i) continue;
      fidFreePool[i] = FID_TAKEN;
      //pthread_mutex_unlock (&mutex);
      return i;
    }
  }
  
  if (fid2 != FID_NULL) {
    GI(last_fid != FID_NULL, 0); /* we should get the same FID except for the first call */
    fidFreePool[fid2] = FID_TAKEN;
    //pthread_mutex_unlock (&mutex);
    return fid2;
  }else if (fid1 != FID_NULL) {
    GI(last_fid != FID_NULL, 0); /* we should get the same FID except for the first call */
    fidFreePool[fid1] = FID_TAKEN;
    //pthread_mutex_unlock (&mutex);
    return fid1;
  }else {
    MSG("Error! demanding more flowID than what's available");
    exit(-11);
  }
  
}

void QEMUEmulInterface::freeFid(FlowID fid)
{
  //pthread_mutex_lock (&mutex);

  I(fidFreePool[fid] == FID_TAKEN);
  fidFreePool[fid] = FID_FREED;

  //pthread_mutex_unlock(&mutex);
}

DInst *QEMUEmulInterface::executeHead(FlowID fid) 
{
  I(nFlows);

  return reader->executeHead(fid);
}

void QEMUEmulInterface::reexecuteTail(FlowID fid) 
{
  I(nFlows);

  reader->reexecuteTail(fid);
}

void QEMUEmulInterface::syncHeadTail(FlowID fid) 
{
  I(nFlows);
  reader->syncHeadTail(fid);
}

FlowID QEMUEmulInterface::getNumFlows(void) const 
{
  return nFlows;
}

FlowID QEMUEmulInterface::getNumEmuls(void) const 
{
  return nEmuls;
}

void QEMUEmulInterface::startRabbit(FlowID fid) 
{
  esesc_set_rabbit(fid);
}

void QEMUEmulInterface::startWarmup(FlowID fid)
{
  esesc_set_warmup(fid);
}

void QEMUEmulInterface::startDetail(FlowID fid)
{
  //reader->drainFIFO(fid);
  esesc_set_timing(fid); // No Detail model in qemu Detail == Timing
}

void QEMUEmulInterface::startTiming(FlowID fid)
{
  //reader->drainFIFO(fid);
  esesc_set_timing(fid);
}

FlowID QEMUEmulInterface::getFirstFlow() const {return 0;} 

void QEMUEmulInterface::setFirstFlow(FlowID fid) {
  //This function does not have any meaning 
  //for the QEMUEmulInterface(at least for now)
  //FirstFlow will always be 0 for QEMU. 
  //
  //QEMUReader does not have any variable called FirstFlow
}

//Added by Hamid R. Khaleghzadeh///////////
short int QEMUEmulInterface::fIDStatus(FlowID fid)							//0:not usable by qemu, 1:is free or freed, 2: is taken
{
	if(fidFreePool[fid] == FID_FREED || fidFreePool[fid] == FID_FREE)
		return FID_FREE;
	else if(fidFreePool[fid] == FID_TAKEN)
		return FID_TAKEN;
	else
		return FID_NOT_AVAILABLE;
}

void QEMUEmulInterface::drainFIFO(FlowID fid)
{
	reader->drainFIFO(fid);
}
//end/////////////////////////////////////

void QEMUEmulInterface::setSampler(EmuSampler *a_sampler, FlowID fid) {
  EmulInterface::setSampler(a_sampler);

  //I(qsampler==0);
  I(fid<128);
  qsamplerlist[fid] = a_sampler;
}

void QEMUEmulInterface::drainFIFO()
{
}

FlowID QEMUEmulInterface::mapLid(FlowID fid) {
  // Maps the flow index to emul index
  std::map<FlowID, FlowID>::iterator it = fidMap.find(fid);
  if (it != fidMap.end())
    return it->second;
  else{
    MSG("Error! Cannot find the lid for the required fid:%u", fid);
    exit(-1);
  }
}
