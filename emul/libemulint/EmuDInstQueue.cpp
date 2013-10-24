/* 
   ESESC: Enhanced Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

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

#include "Snippets.h"
#include "EmuDInstQueue.h"

/*
  The idea behind the head and tail pointer is to support replays eventually. 
  The head will point to the instruction that will be executed, and the tail will 
  point to the instruction that will be retired. So, the tail will always follow 
  the head. The insertpoint is the place where the new instruction will be inserted. 

  The head pointer will always lie between the tail pointer and the insertpoint.
  
*/

EmuDInstQueue::EmuDInstQueue() {
  head = 0;
  tail = 0;
  insertpoint = 0; // Starts inserting in the insertpoint + 1 position.
  nFreeElems = 1024;
  ndrop = 0;
  trace.resize(1024);

  I(ISPOWER2(trace.size()));
}

void EmuDInstQueue::adjust_trace() {

  // T...H...I
  uint32_t new_tail = trace.size();
  trace.resize(trace.size()*2);
  I(new_tail<1024*64);

  I(ISPOWER2(trace.size()));
  I(ISPOWER2(new_tail));

  for(uint32_t i=0;i<new_tail;i++) {
    trace[new_tail+i] = trace[(tail + i) & (new_tail-1)];
  }

  if (head>=tail)
    head = new_tail + head-tail;
  else
    head = new_tail + new_tail - (tail-head);

  tail = new_tail;
  insertpoint = 0;
  nFreeElems = new_tail;
}

