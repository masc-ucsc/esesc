/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

ESESC is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with ESESC; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include "Port.h"

PortGeneric::PortGeneric(const char *name)
  : avgTime(name) 
{
  nUsers = 0;
}

PortGeneric::~PortGeneric()
{
  GMSG(nUsers != 0, "Not enough destroys for FUGenUnit");
}

PortGeneric *PortGeneric::create(const char *unitName, 
                                 NumUnits_t nUnits, 
                                 TimeDelta_t occ)
{
  // Search for the configuration with the best performance. In theory
  // everything can be solved with PortNPipe, but it is the slowest.
  // Sorry, but I'm a performance freak!

  char *name = (char*)malloc(strlen(unitName)+10);
  sprintf(name,"%s_occ",unitName);

  PortGeneric *gen;
  
  if(occ == 0 || nUnits == 0) {
    gen = new PortUnlimited(name);
  } else if (occ == 1) {
    if(nUnits == 1) {
      gen = new PortFullyPipe(name);
    } else {
      gen = new PortFullyNPipe(name,nUnits);
    }
  } else {
    if(nUnits == 1) {
      gen = new PortPipe(name,occ);
    } else {
      gen = new PortNPipe(name,nUnits,occ);
    }
  }

  free(name);
  
  return gen;
}

void PortGeneric::destroy()
{
  delete this;
}


PortUnlimited::PortUnlimited(const char *name)
  : PortGeneric(name) 
{
}

Time_t PortUnlimited::nextSlot(void* mreq) {
  avgTime.sample(0); // Just to keep usage statistics
  return globalClock;
}

Time_t PortUnlimited::occupySlots(int32_t nSlots) 
{
  return globalClock;
}

Time_t PortUnlimited::calcNextSlot(void* mreq) const
{
  return globalClock;
}

void PortUnlimited::lock4nCycles(TimeDelta_t clks)
{
  if( lTime < globalClock )
    lTime = globalClock;
  
  lTime += clks;
}

PortFullyPipe::PortFullyPipe(const char *name) 
  : PortGeneric(name) 
{
  lTime = globalClock;
}

Time_t PortFullyPipe::nextSlot(void* mreq) 
{
  ID(Time_t cns = calcNextSlot());
    
  if(lTime < globalClock)
    lTime = globalClock;

  I(cns  == lTime);
  avgTime.sample(lTime-globalClock);
  return lTime++;
}

Time_t PortFullyPipe::occupySlots(int32_t nSlots) 
{
  if(lTime < globalClock)
    lTime = globalClock;
  
  avgTime.sample(lTime-globalClock);

  Time_t t = lTime;
  lTime+=nSlots;
  return t;
}

Time_t PortFullyPipe::calcNextSlot(void* mreq) const
{
  return ((lTime < globalClock) ? globalClock : lTime);
}

void PortFullyPipe::lock4nCycles(TimeDelta_t clks)
{
  if( lTime < globalClock )
    lTime = globalClock;
  
  lTime += clks;
}


PortFullyNPipe::PortFullyNPipe(const char *name, NumUnits_t nFU)
  : PortGeneric(name), nUnitsMinusOne(nFU-1) 
{
  I(nFU>0); // For unlimited resources use the FUUnlimited
  
  lTime = globalClock;

  freeUnits = nFU;
}

Time_t PortFullyNPipe::nextSlot(void* mreq) 
{
  ID(Time_t cns = calcNextSlot());

  if(lTime < globalClock) {
    lTime     = globalClock;
    freeUnits = nUnitsMinusOne;
  }else if(freeUnits > 0) {
    freeUnits--;
  }else{
    lTime++;
    freeUnits = nUnitsMinusOne;
  }
    
  I( cns == lTime);
  avgTime.sample(lTime-globalClock);
  return lTime;
}

Time_t PortFullyNPipe::occupySlots(int32_t nSlots)
{
  if(lTime < globalClock)
    lTime = globalClock;
  
  avgTime.sample(lTime-globalClock);

  Time_t t = lTime;
  lTime += nSlots/(nUnitsMinusOne+1);
  return t;
}

Time_t PortFullyNPipe::calcNextSlot(void* mreq) const
{
  return ((lTime < globalClock) ? globalClock : (lTime + (freeUnits == 0)));
}

void PortFullyNPipe::lock4nCycles(TimeDelta_t clks)
{
  if( lTime < globalClock )
    lTime = globalClock;

  lTime +=clks;
  freeUnits = nUnitsMinusOne+1;
}

PortPipe::PortPipe(const char *name, TimeDelta_t occ)
  : PortGeneric(name), ocp(occ) 
{
  lTime = (globalClock > ocp) ? globalClock - ocp : 0;
}

Time_t PortPipe::nextSlot(void* mreq) 
{
  ID(Time_t cns = calcNextSlot());

  if(lTime < globalClock )
    lTime = globalClock;

  Time_t st=lTime;
  lTime+=ocp;

  I(cns == st);
  avgTime.sample(st-globalClock);
  return st;
}

Time_t PortPipe::occupySlots(int32_t nSlots)
{
  if(lTime < globalClock)
    lTime = globalClock;

  I(ocp>1); // If ocp where <1 other PortPipe should be called
  
  avgTime.sample(lTime-globalClock);

  Time_t t = lTime;
  lTime+=ocp*nSlots;
  return t;
}

Time_t PortPipe::calcNextSlot(void* mreq) const
{
  return ((lTime < globalClock) ? globalClock : lTime);
}

void PortPipe::lock4nCycles(TimeDelta_t clks)
{
  if( lTime < globalClock )
    lTime = globalClock;
  
  lTime += clks;
}

PortNPipe::PortNPipe(const char *name, NumUnits_t nFU, TimeDelta_t occ) 
  : PortGeneric(name) 
  ,ocp(occ)
  ,nUnits(nFU) 
{
  portBusyUntil = (Time_t *)malloc(sizeof(Time_t)*nFU);

  for(int32_t i=0;i<nFU;i++)
    portBusyUntil[i]= globalClock;

  doStats = true;
}

PortNPipe::~PortNPipe()
{
  free(portBusyUntil);
}


Time_t PortNPipe::nextSlot(void* mreq) 
{
  return nextSlot(ocp);
}

Time_t PortNPipe::nextSlot(int32_t occupancy) 
{
  ID(Time_t cns      = calcNextSlot());
  Time_t bufTime     = portBusyUntil[0];

  if (bufTime < globalClock) {
    portBusyUntil[0] = globalClock + occupancy;
    I( cns == globalClock );
    if (doStats)
      avgTime.sample(0);
    return globalClock;
  }

  NumUnits_t bufPort = 0; 

  for(NumUnits_t i=1; i < nUnits; i++) {
    if (portBusyUntil[i] < globalClock) {
      portBusyUntil[i] = globalClock + occupancy;
      I( cns == globalClock );
      if (doStats)
        avgTime.sample(0);
      return globalClock;
    }
    if (portBusyUntil[i] < bufTime) {
      bufPort = i;
      bufTime = portBusyUntil[i];
    }
  }

  portBusyUntil[bufPort] += occupancy;

  I(cns == bufTime);
  if (doStats)
    avgTime.sample(bufTime-globalClock);
  return bufTime;
}

Time_t PortNPipe::occupySlots(int32_t nSlots)
{
  // How to optimize this function (ala PortFullyPipe::occupySlots)?
  // Can someone do it?

  avgTime.sample(portBusyUntil[0]-globalClock);

  doStats = false;
  
  Time_t t = nextSlot();
  for(int32_t i=1;i<nSlots;i++)
    nextSlot();

  doStats = true;

  return t;
}

Time_t PortNPipe::calcNextSlot(void* mreq) const
{
  Time_t firsttime = portBusyUntil[0];

  for(NumUnits_t i = 1; i < nUnits; i++)
    if (portBusyUntil[i] < firsttime)
      firsttime = portBusyUntil[i];
  return (firsttime < globalClock) ? globalClock : firsttime;
}

void PortNPipe::lock4nCycles(TimeDelta_t clks)
{
  Time_t cns = calcNextSlot();
  cns+=clks;
  for(NumUnits_t i = 0; i < nUnits; i++)
    if( portBusyUntil[i] < cns )
      portBusyUntil[i] = cns;
}
