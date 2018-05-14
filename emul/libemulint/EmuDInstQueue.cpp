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

#include "EmuDInstQueue.h"
#include "Snippets.h"

/*
  The idea behind the head and tail pointer is to support replays eventually.
  The head will point to the instruction that will be executed, and the tail will
  point to the instruction that will be retired. So, the tail will always follow
  the head. The insertpoint is the place where the new instruction will be inserted.

  The head pointer will always lie between the tail pointer and the insertpoint.

*/

EmuDInstQueue::EmuDInstQueue() {
  head        = 0;
  tail        = 0;
  insertpoint = 0; // Starts inserting in the insertpoint + 1 position.
  nFreeElems  = 1024;
  ndrop       = 0;
  trace.resize(1024);

  I(ISPOWER2(trace.size()));
}

void EmuDInstQueue::adjust_trace() {

  // T...H...I
  uint32_t new_tail = trace.size();
  trace.resize(trace.size() * 2);
  I(new_tail < 1024 * 64);

  I(ISPOWER2(trace.size()));
  I(ISPOWER2(new_tail));

  for(uint32_t i = 0; i < new_tail; i++) {
    trace[new_tail + i] = trace[(tail + i) & (new_tail - 1)];
  }

  if(head >= tail)
    head = new_tail + head - tail;
  else
    head = new_tail + new_tail - (tail - head);

  tail        = new_tail;
  insertpoint = 0;
  nFreeElems  = new_tail;
}
