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
    : avgTime(name) {
  nUsers = 0;
}

PortGeneric::~PortGeneric() {
  GMSG(nUsers != 0, "Not enough destroys for FUGenUnit");
}

PortGeneric *PortGeneric::create(const char *unitName, NumUnits_t nUnits, TimeDelta_t occ) {
  // Search for the configuration with the best performance. In theory
  // everything can be solved with PortNPipe, but it is the slowest.
  // Sorry, but I'm a performance freak!

  char *name = (char *)malloc(strlen(unitName) + 10);
  sprintf(name, "%s_occ", unitName);

  PortGeneric *gen;

  if(occ == 0 || nUnits == 0) {
    gen = new PortUnlimited(name);
  } else if(occ == 1) {
    if(nUnits == 1) {
      gen = new PortFullyPipe(name);
    } else {
      gen = new PortFullyNPipe(name, nUnits);
    }
  } else {
    if(nUnits == 1) {
      gen = new PortPipe(name, occ);
    } else {
      gen = new PortNPipe(name, nUnits, occ);
    }
  }

  free(name);

  return gen;
}

void PortGeneric::destroy() {
  delete this;
}

void PortGeneric::occupyUntil(Time_t u) {
  Time_t t = globalClock;

  while(t < u) {
    t = nextSlot(false);
  }
}

PortUnlimited::PortUnlimited(const char *name)
    : PortGeneric(name) {
}

Time_t PortUnlimited::nextSlot(bool en) {

  avgTime.sample(0, en); // Just to keep usage statistics
  return globalClock;
}

void PortUnlimited::occupyUntil(Time_t u) {
}

Time_t PortUnlimited::calcNextSlot() const {
  return globalClock;
}

PortFullyPipe::PortFullyPipe(const char *name)
    : PortGeneric(name) {
  lTime = globalClock;
}

Time_t PortFullyPipe::nextSlot(bool en) {
  ID(Time_t cns = calcNextSlot());

  if(lTime < globalClock)
    lTime = globalClock;

  I(cns == lTime);
  avgTime.sample(lTime - globalClock, en);
  return lTime++;
}

Time_t PortFullyPipe::calcNextSlot() const {
  return ((lTime < globalClock) ? globalClock : lTime);
}

PortFullyNPipe::PortFullyNPipe(const char *name, NumUnits_t nFU)
    : PortGeneric(name)
    , nUnitsMinusOne(nFU - 1) {
  I(nFU > 0); // For unlimited resources use the FUUnlimited

  lTime = globalClock;

  freeUnits = nFU;
}

Time_t PortFullyNPipe::nextSlot(bool en) {
  ID(Time_t cns = calcNextSlot());

  if(lTime < globalClock) {
    lTime     = globalClock;
    freeUnits = nUnitsMinusOne;
  } else if(freeUnits > 0) {
    freeUnits--;
  } else {
    lTime++;
    freeUnits = nUnitsMinusOne;
  }

  I(cns == lTime);
  avgTime.sample(lTime - globalClock, en);
  return lTime;
}

Time_t PortFullyNPipe::calcNextSlot() const {
  return ((lTime < globalClock) ? globalClock : (lTime + (freeUnits == 0)));
}

PortPipe::PortPipe(const char *name, TimeDelta_t occ)
    : PortGeneric(name)
    , ocp(occ) {
  lTime = (globalClock > ocp) ? globalClock - ocp : 0;
}

Time_t PortPipe::nextSlot(bool en) {
  ID(Time_t cns = calcNextSlot());

  if(lTime < globalClock)
    lTime = globalClock;

  Time_t st = lTime;
  lTime += ocp;

  I(cns == st);
  avgTime.sample(st - globalClock, en);
  return st;
}

Time_t PortPipe::calcNextSlot() const {
  return ((lTime < globalClock) ? globalClock : lTime);
}

PortNPipe::PortNPipe(const char *name, NumUnits_t nFU, TimeDelta_t occ)
    : PortGeneric(name)
    , ocp(occ)
    , nUnits(nFU) {
  portBusyUntil = (Time_t *)malloc(sizeof(Time_t) * nFU);

  for(int32_t i = 0; i < nFU; i++)
    portBusyUntil[i] = globalClock;
}

PortNPipe::~PortNPipe() {
  free(portBusyUntil);
}

Time_t PortNPipe::nextSlot(bool en) {
  return nextSlot(ocp, en);
}

Time_t PortNPipe::nextSlot(int32_t occupancy, bool en) {
  ID(Time_t cns = calcNextSlot());
  Time_t bufTime = portBusyUntil[0];

  if(bufTime < globalClock) {
    portBusyUntil[0] = globalClock + occupancy;
    I(cns == globalClock);
    avgTime.sample(0, en);
    return globalClock;
  }

  NumUnits_t bufPort = 0;

  for(NumUnits_t i = 1; i < nUnits; i++) {
    if(portBusyUntil[i] < globalClock) {
      portBusyUntil[i] = globalClock + occupancy;
      I(cns == globalClock);
      avgTime.sample(0, en);
      return globalClock;
    }
    if(portBusyUntil[i] < bufTime) {
      bufPort = i;
      bufTime = portBusyUntil[i];
    }
  }

  portBusyUntil[bufPort] += occupancy;

  I(cns == bufTime);
  avgTime.sample(bufTime - globalClock, en);
  return bufTime;
}

Time_t PortNPipe::calcNextSlot() const {
  Time_t firsttime = portBusyUntil[0];

  for(NumUnits_t i = 1; i < nUnits; i++)
    if(portBusyUntil[i] < firsttime)
      firsttime = portBusyUntil[i];
  return (firsttime < globalClock) ? globalClock : firsttime;
}
