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

#ifndef CLUSTER_H
#define CLUSTER_H

#include "estl.h"

#include <vector>
#include <limits.h>

#include "nanassert.h"

#include "DepWindow.h"
#include "GStats.h"
#include "Instruction.h"

class Resource;
class GMemorySystem;
class GProcessor;

class Cluster {
 private:
  void buildUnit(const char *clusterName
                 ,GMemorySystem *ms
                 ,Cluster *cluster
                 ,InstOpcode type);

 protected:
  DepWindow window;

  const int32_t MaxWinSize;
  int32_t windowSize;

  GProcessor *const gproc;

  GStatsAvg  winNotUsed;
  GStatsCntr rdRegPool;
  GStatsCntr wrRegPool;

  Resource   *res[iMAX];

  int32_t regPool;


 protected:
  void delEntry() {
    windowSize++;
    I(windowSize<=MaxWinSize);
  }
  void newEntry() {
    windowSize--;
    I(windowSize>=0);
  }
  
  virtual ~Cluster();
  Cluster(const char *clusterName, GProcessor *gp);

 public:

  void select(DInst *dinst);

  virtual void executing(DInst *dinst) = 0;
  virtual void executed(DInst *dinst)  = 0;
  virtual bool retire(DInst *dinst, bool replay)    = 0;

  static Cluster *create(const char *clusterName, GMemorySystem *ms, GProcessor *gproc);

  Resource *getResource(InstOpcode type) const {
    I(type < iMAX);
    return res[type];
  }

  StallCause canIssue(DInst *dinst) const;
  void addInst(DInst *dinst);

  GProcessor *getGProcessor() const { return gproc; }

 
};

class ExecutingCluster : public Cluster {
  // This is SCOORE style. The instruction is removed from the queue at dispatch time
 public:
  virtual ~ExecutingCluster() {
  }
    
  ExecutingCluster(const char *clusterName, GProcessor *gp)
    : Cluster(clusterName, gp) { }
    
  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};

class ExecutedCluster : public Cluster {
 public:
  virtual ~ExecutedCluster() {
  }
    
  ExecutedCluster(const char *clusterName, GProcessor *gp)
    : Cluster(clusterName, gp) { }
    
  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};

class RetiredCluster : public Cluster {
 public:
  virtual ~RetiredCluster() {
  }
  RetiredCluster(const char *clusterName, GProcessor *gp)
    : Cluster(clusterName, gp) { }

  void executing(DInst *dinst);
  void executed(DInst *dinst);
  bool retire(DInst *dinst, bool replay);
};


#endif // CLUSTER_H
