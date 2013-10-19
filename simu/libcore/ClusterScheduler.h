/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
   Updated by     Milos Prvulovic

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

#ifndef CLUSTERSCHEDULER_H
#define CLUSTERSCHEDULER_H

#include <vector>
#include <stdio.h>

#include "GStats.h"
#include "DInst.h"
#include "Resource.h"

typedef std::vector< std::vector<Resource *> > ResourcesPoolType;

class ClusterScheduler {
 private:
 protected:
  ResourcesPoolType res;

 public:
  ClusterScheduler(const ResourcesPoolType ores);
  virtual ~ClusterScheduler();

  virtual Resource *getResource(DInst *dinst) =0;
};

class RoundRobinClusterScheduler : public ClusterScheduler {
 private:
  std::vector<unsigned int> nres;
  std::vector<unsigned int> pos;
  
 public:
  RoundRobinClusterScheduler(const ResourcesPoolType res);
  ~RoundRobinClusterScheduler();

  Resource *getResource(DInst *dinst);
};

class LRUClusterScheduler : public ClusterScheduler {
 private:
 public:
  LRUClusterScheduler(const ResourcesPoolType res);
  ~LRUClusterScheduler();

  Resource *getResource(DInst *dinst);
};

#endif

