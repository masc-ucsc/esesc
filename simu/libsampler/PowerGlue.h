#ifndef POWERGLUE_H
#define POWERGLUE_H

// Contributed by Ehsan K.Ardestani
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
#include <map>
#include <string>
#include <vector>

class ChipEnergyBundle;
class SescThermWrapper;

class PowerGlue {

private:
  const char *      pwr_sec;
  ChipEnergyBundle *energyBundle;
  bool              reFloorplan;
  uint32_t          nStats;

  GProcessor *getSimu(FlowID fid) {
    return TaskHandler::getSimu(fid);
  };
  void     initStatCounters();
  void     closeStatCounters();
  void     createCoreStats();
  void     createCoreLayoutDescr();
  void     createMemLayoutDescr();
  void     createCoreStats(const char *section, uint32_t i);
  void     createCoreDescr(const char *section, uint32_t i);
  void     autoCreateCoreDescr(const char *section);
  void     createMemStats();
  void     createMemStats(const char *section, uint32_t i);
  void     createMemObjStats(const char *temp_sec, const char *pname, const char *name);
  char *   getText(const char *format, ...);
  char *   replicateVar(const char *format, const char *s1, const char *s);
  uint32_t size(const char *str);
  char *   getStr(const char *str, uint32_t i);
  char *   privateName(const std::string *name);
  void     checkStatCounters(const char *section);
  void     addVRecord(const char *sec, const char *str);

public:
  PowerGlue();
  void createStatCounters();
  void dumpFlpDescr(uint32_t coreEIdx);
  void plug(const char *section, ChipEnergyBundle *eBundle);
  void unplug();
};

#endif // POWERGLUE_H
