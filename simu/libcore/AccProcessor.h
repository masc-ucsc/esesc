// Contributed by Jose Renau
//                Basilio Fraguela
//                Milos Prvulovic
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

#ifndef _ACCPROCESSOR_H_
#define _ACCPROCESSOR_H_

#include "nanassert.h"

#include "FastQueue.h"
#include "FetchEngine.h"
#include "GOoOProcessor.h"
#include "GStats.h"
#include "Pipeline.h"
#include "callback.h"

class AccProcessor : public GProcessor {
private:
  bool busy;

protected:
  // BEGIN VIRTUAL FUNCTIONS of GProcessor
  bool advance_clock(FlowID fid);

  // Not needed for Acc
  StallCause addInst(DInst *dinst);
  void       retire();
  void       fetch(FlowID fid);
  LSQ *      getLSQ();
  bool       isFlushing();
  bool       isReplayRecovering();
  Time_t     getReplayID();
  void       executing(DInst *dinst);
  void       executed(DInst *dinst);

  virtual void replay(DInst *target){}; // = 0;

  // END VIRTUAL FUNCTIONS of GProcessor

  AddrType myAddr;
  AddrType addrIncr;
  int      reqid;
  int      total_accesses;
  int      outstanding_accesses;

  GStatsCntr accReads;
  GStatsCntr accWrites;

  GStatsAvg accReadLatency;
  GStatsAvg accWriteLatency;

  void read_performed(uint32_t id, Time_t startTime);
  void write_performed(uint32_t id, Time_t startTime);
  typedef CallbackMember2<AccProcessor, uint32_t, Time_t, &AccProcessor::read_performed>  read_performedCB;
  typedef CallbackMember2<AccProcessor, uint32_t, Time_t, &AccProcessor::write_performed> write_performedCB;

public:
  AccProcessor(GMemorySystem *gm, CPU_t i);
  virtual ~AccProcessor();
};

#endif
