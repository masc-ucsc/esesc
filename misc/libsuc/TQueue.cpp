/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau

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

#include <stdlib.h>
#include <string.h>

#define TQUEUE_CPP

#include "TQueue.h"

exportTemplate template <class Data, class Time>
TQueue<Data, Time>::TQueue(uint32_t MaxTimeDiff)
    : AccessSize(MaxTimeDiff)
    , AccessMask(AccessSize - 1) {
  I(AccessSize > 7);
  I((AccessSize & (AccessSize - 1)) == 0);

  accessTail = (Data *)malloc(AccessSize * sizeof(Data));

  access = (Data *)malloc(AccessSize * sizeof(Data));

  reset();
}

exportTemplate template <class Data, class Time> void TQueue<Data, Time>::reset() {
  bzero(access, AccessSize * sizeof(Data));

  nNodes  = 0;
  minTime = 0;
  minPos  = 0;

  minTooFar = MaxTime; // MaxTime means empty
}

exportTemplate template <class Data, class Time> TQueue<Data, Time>::~TQueue() {
  GMSG(nNodes, "Destroying TQueue %d with pending nodes", nNodes);

  free(accessTail);

  free(access);
}

exportTemplate template <class Data, class Time> void TQueue<Data, Time>::dump() {
  MSG("TQueue dump: size=%d", size());

  int32_t pos   = minPos;
  int32_t conta = nNodes;

  Data node = access[pos];

  while(conta) {
    if(node) {
      conta--;
      printf(" %p @ %d ", node, node->getTQTime());
      node = node->getTQNext();
    } else {
      pos++;
      node = access[pos];
    }
  }
  printf("\n");
}
