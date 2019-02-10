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
#ifndef QEMUEMULINTERFACE_H
#define QEMUEMULINTERFACE_H

#include <map>

#include "EmulInterface.h"
#include "QEMUReader.h"
#include "nanassert.h"

#define FID_NOT_AVAILABLE 0
#define FID_FREE 1
#define FID_FREED 3
#define FID_TAKEN 2
#define FID_NULL std::numeric_limits<uint32_t>::max()

class QEMUEmulInterface : public EmulInterface {
private:
  FlowID      nFlows;
  FlowID      nEmuls;
  QEMUReader *reader;
  FlowID      firstassign;

protected:
  static std::vector<FlowID>      fidFreePool;
  static std::map<FlowID, FlowID> fidMap;

public:
  QEMUEmulInterface(const char *section);
  ~QEMUEmulInterface();

  bool   populate(FlowID fid);
  DInst *peekHead(FlowID fid);
  void   executeHead(FlowID fid);
  void   reexecuteTail(FlowID fid);
  void   syncHeadTail(FlowID fid);
#ifdef FETCH_TRACE
  void markTrace(..);
#endif

  FlowID getNumFlows(void) const;
  FlowID getNumEmuls(void) const;

  FlowID mapGlobalID(FlowID gid) const {
    return gid;
  }

  void start() {
    reader->start();
  }

  void queueInstruction(AddrType pc, AddrType addr, DataType data, FlowID fid, int op, int src1, int src2, int dest, int dest2,
                        bool inEmuTiming, DataType data2) {
    reader->queueInstruction(pc, addr, data, fid, op, src1, src2, dest, dest2, inEmuTiming, data2);
  }

  void syscall(uint32_t num, Time_t time, FlowID fid) {
    reader->syscall(num, time, fid);
  }

  void startRabbit(FlowID fid);
  void startWarmup(FlowID fid);
  void startDetail(FlowID fid);
  void startTiming(FlowID fid);

  FlowID getFirstFlow() const;
  void   setFid(FlowID cpuid);
  FlowID getFid(FlowID last_fid);
  void   freeFid(FlowID fid);
  void   setFirstFlow(FlowID fid);

  void   setSampler(EmuSampler *a_sampler, FlowID fid = 0);
  void   drainFIFO();
  FlowID mapLid(FlowID fid);
};

#endif // QEMUEMULINTERFACE_H
