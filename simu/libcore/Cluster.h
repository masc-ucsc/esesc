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

#ifndef CLUSTER_H
#define CLUSTER_H

#include "estl.h"

#include <limits.h>
#include <vector>

#include "nanassert.h"

#include "DepWindow.h"
#include "GStats.h"
#include "Instruction.h"

class Resource;
class GMemorySystem;

class Cluster {
private:
  void buildUnit(const char *clusterName, uint32_t pos, GMemorySystem *ms, Cluster *cluster, uint32_t cpuid, InstOpcode type,
                 GProcessor *gproc);

protected:
  DepWindow window;

  const int32_t MaxWinSize;
  int32_t       windowSize;

  int32_t nready;

  GStatsAvg  winNotUsed;
  GStatsCntr rdRegPool;
  GStatsCntr wrRegPool;

  Resource *res[iMAX];

  int32_t nRegs;
  int32_t regPool;
  bool    lateAlloc;

  uint32_t cpuid;

  char *name;

protected:
  void delEntry() {
    windowSize++;
    I(windowSize <= MaxWinSize);
  }
  void newEntry() {
    windowSize--;
    I(windowSize >= 0);
  }

  virtual ~Cluster();
  Cluster(const char *clusterName, uint32_t pos, uint32_t cpuid);

public:
  void select(DInst *dinst);

  virtual void executing(DInst *dinst)           = 0;
  virtual void executed(DInst *dinst)            = 0;
  virtual bool retire(DInst *dinst, bool replay) = 0;

  static Cluster *create(const char *clusterName, uint32_t pos, GMemorySystem *ms, uint32_t cpuid, GProcessor *gproc);

  Resource *getResource(InstOpcode type) const {
    I(type < iMAX);
    return res[type];
  }

  const char *getName() const {
    return name;
  }

  StallCause canIssue(DInst *dinst) const;
  void       addInst(DInst *dinst);

  int32_t getAvailSpace() const {
    if(regPool < windowSize)
      return regPool;
    return windowSize;
  }
  int32_t getNReady() const {
    return nready;
  }
};

class ExecutingCluster : public Cluster {
  // This is SCOORE style. The instruction is removed from the queue at dispatch time
public:
  virtual ~ExecutingCluster() {
  }

  ExecutingCluster(const char *clusterName, uint32_t pos, uint32_t cpuid)
      : Cluster(clusterName, pos, cpuid) {
  }

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};

class ExecutedCluster : public Cluster {
public:
  virtual ~ExecutedCluster() {
  }

  ExecutedCluster(const char *clusterName, uint32_t pos, uint32_t cpuid)
      : Cluster(clusterName, pos, cpuid) {
  }

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};

class RetiredCluster : public Cluster {
public:
  virtual ~RetiredCluster() {
  }
  RetiredCluster(const char *clusterName, uint32_t pos, uint32_t cpuid)
      : Cluster(clusterName, pos, cpuid) {
  }

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};

#endif // CLUSTER_H
