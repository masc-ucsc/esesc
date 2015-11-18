// Contributed by Jose Renau
//                Basilio Fraguela
//                James Tuck
//                Milos Prvulovic
//                Luis Ceze
//                Islam Atta (IBM)
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

#include <math.h>

#include "SescConf.h"

#include "AccProcessor.h"

#include "TaskHandler.h"
#include "FetchEngine.h"
#include "GMemorySystem.h"
#include "EmuSampler.h"
#include "MemRequest.h"

/* }}} */

AccProcessor::AccProcessor(GMemorySystem *gm, CPU_t i)
  /* constructor {{{1 */
  :GProcessor(gm,i,1)
{
  setActive();
}
/* }}} */

AccProcessor::~AccProcessor()
 /* destructor {{{1 */
{
  // Nothing to do
}
/* }}} */

void AccProcessor::performed( uint32_t id)
{
  MSG("@%lld: AccProcessor::performed cpu_id=%d reqid=%d\n",(long long int)globalClock,cpu_id, id);
}

bool AccProcessor::advance_clock(FlowID fid)
  /* Full execution: fetch|rename|retire {{{1 */
{
  // MSG("@%lld: AccProcessor::advance_clock(fid=%d) cpu_id=%d\n",(long long int)globalClock,fid,cpu_id);

  if( fid == 2) {
    static AddrType myAddr = 0;
    static int reqid       = 0;

    if( globalClock < 240000 && globalClock > 500 && ((globalClock % 100) == 0) ) {
      MSG("@%lld: OoOProcessor::advance_clock(fid=%d) memRequest write cpu_id=%d myAddr=%016llx\n",(long long int)globalClock,fid,cpu_id,myAddr);
      MemRequest::sendReqWrite(memorySystem->getDL1(), true, myAddr+=8192, performedCB::create(this,reqid++));
      myAddr %= (128*1024);
    }
  }

  if (!active) {
    return false;
  }

  clockTicks.inc(true);
  setWallClock(true);

  return true;
}
/* }}} */

StallCause AccProcessor::addInst(DInst *dinst)
  /* rename (or addInst) a new instruction {{{1 */
{
  I(0);

  return NoStall;
}
/* }}} */

void AccProcessor::retire()
  /* Try to retire instructions {{{1 */
{
  I(0);
}
/* }}} */

void AccProcessor::fetch(FlowID fid) {
  I(0);
}

LSQ *AccProcessor::getLSQ() {
  I(0);
}

bool AccProcessor::isFlushing() {
  I(0);
}

bool AccProcessor::isReplayRecovering() {
  I(0);
}

Time_t AccProcessor::getReplayID() {
  I(0);
}

