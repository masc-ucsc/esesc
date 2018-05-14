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

#include "EmuSampler.h"
#include "GMemorySystem.h"
#include "MemRequest.h"
#include "TaskHandler.h"
/* }}} */

AccProcessor::AccProcessor(GMemorySystem *gm, CPU_t i)
    /* constructor {{{1 */
    : GProcessor(gm, i)
    , myAddr((i + 1) * 128 * 1024)
    , addrIncr(SescConf->getInt("cpusimu", "addrIncr", i))
    , reqid(0)
    , accReads("P(%d)_acc_reads", i)
    , accWrites("P(%d)_acc_writes", i)
    , accReadLatency("P(%d)_acc_ave_read_latency", i)
    , accWriteLatency("P(%d)_acc_ave_write_latency", i) {
  setActive();
}
/* }}} */

AccProcessor::~AccProcessor()
/* destructor {{{1 */
{
  // Nothing to do
}
/* }}} */

void AccProcessor::read_performed(uint32_t id, Time_t startTime)
// {{{1 callback for completed reads
{
  // MSG("@%lld: AccProcessor::read_performed cpu_id=%d reqid=%d lat=%d\n",(long long int)globalClock,cpu_id, id,
  // (int)(globalClock-startTime));
  accReads.inc(true);
  accReadLatency.sample((int)(globalClock - startTime), true);
}
/* }}} */

void AccProcessor::write_performed(uint32_t id, Time_t startTime)
// {{{1 callback for completed writes
{
  // MSG("@%lld: AccProcessor::write_performed cpu_id=%d reqid=%d lat=%d\n",(long long int)globalClock,cpu_id, id,
  // (int)(globalClock-startTime));
  accWrites.inc(true);
  accWriteLatency.sample((int)(globalClock - startTime), true);
}
/* }}} */

bool AccProcessor::advance_clock(FlowID fid)
/* Full execution: fetch|rename|retire {{{1 */
{
  // MSG("@%lld: AccProcessor::advance_clock(fid=%d) cpu_id=%d\n",(long long int)globalClock,fid,cpu_id);
  if(globalClock > 500 && ((globalClock % 10) == (fid))) {
    if(reqid & 1) {
      // MSG("@%lld: AccProcessor::advance_clock(fid=%d) memRequest write cpu_id=%d myAddr=%016llx\n",(long long
      // int)globalClock,fid,cpu_id,(long long int)myAddr);
      MemRequest::sendReqWrite(memorySystem->getDL1(), true, myAddr += addrIncr, 0,
                               write_performedCB::create(this, reqid++, globalClock));
    } else {
      // MSG("@%lld: AccProcessor::advance_clock(fid=%d) memRequest read cpu_id=%d myAddr=%016llx\n",(long long
      // int)globalClock,fid,cpu_id,(long long int)myAddr);
      MemRequest::sendReqRead(memorySystem->getDL1(), true, myAddr, 0, read_performedCB::create(this, reqid++, globalClock));
    }
  }

  if(!active) {
    return false;
  }

  clockTicks.inc(true);
  setWallClock(true);

  return true;
}
/* }}} */

void AccProcessor::executing(DInst *dinst) {
}

void AccProcessor::executed(DInst *dinst) {
}

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
}
/* }}} */

void AccProcessor::fetch(FlowID fid) {
  I(0);
}

LSQ *AccProcessor::getLSQ() {
  I(0);
  return 0;
}

bool AccProcessor::isFlushing() {
  I(0);
  return false;
}

bool AccProcessor::isReplayRecovering() {
  I(0);
  return false;
}

Time_t AccProcessor::getReplayID() {
  I(0);
  return 0;
}
