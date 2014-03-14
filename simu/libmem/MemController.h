// Contributed by Jose Renau
//                Max Dunne
//                Shea Ellerson
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

#ifndef MEMCONTROLLER_H
#define MEMCONTROLLER_H

#include "GStats.h"
#include "CacheCore.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"

#include "SescConf.h"
#include "callback.h"

#include "Snippets.h"

class MemorySystem;

#include <queue>
/* }}} */

class MemController: public MemObj {
protected:

   class FCFSField {
  public:
    uint32_t Bank;
    uint32_t Row;
    uint32_t Column;
    Time_t TimeEntered;
    MemRequest *mreq;
  };
  TimeDelta_t delay;
  TimeDelta_t PreChargeLatency;
  TimeDelta_t RowAccessLatency;
  TimeDelta_t ColumnAccessLatency;

  GStatsCntr nPrecharge;
  GStatsCntr nColumnAccess;
  GStatsCntr nRowAccess;
  GStatsAvg avgMemLat;

  enum STATE {
  IDLE = 0,
  ACTIVATING,
  PRECHARGE,
  ACTIVE,
  ACCESSING
};
  PortGeneric *cmdPort;
  
  uint32_t rowMask;
  uint32_t columnMask;
  uint32_t bankMask;
  uint32_t rowOffset;
  uint32_t columnOffset;
  uint32_t bankOffset;
  uint32_t numBanks;
  uint32_t memRequestBufferSize;
  

  class BankStatus {
  public:
    int state;
    uint32_t activeRow;
    bool bpend;
    bool cpend;
    Time_t bankTime;
  };
  
  BankStatus *bankState;
  
  typedef std::vector<FCFSField*> FCFSList;
  FCFSList curMemRequests;
  typedef std::queue<FCFSField*> FCFSQueue;
  FCFSQueue OverflowMemoryRequests;

public:
  MemController(MemorySystem* current, const char *device_descr_section, const char *device_name = NULL);
  ~MemController() {}

	// Entry points to schedule that may schedule a do?? if needed
	void req(MemRequest *req)         { doReq(req); };
	void reqAck(MemRequest *req)      { doReqAck(req); };
	void setState(MemRequest *req)    { doSetState(req); };
	void setStateAck(MemRequest *req) { doSetStateAck(req); };
	void disp(MemRequest *req)        { doDisp(req); }

	// This do the real work
	void doReq(MemRequest *req);
	void doReqAck(MemRequest *req);
	void doSetState(MemRequest *req);
	void doSetStateAck(MemRequest *req);
	void doDisp(MemRequest *req);

  TimeDelta_t ffread(AddrType addr);
  TimeDelta_t ffwrite(AddrType addr);

	bool isBusy(AddrType addr) const;

  uint16_t getLineSize() const;

  void manageRam(void);

  typedef CallbackMember0<MemController, &MemController::manageRam>   ManageRamCB;

  TimeDelta_t ffread(AddrType addr, DataType data);
  TimeDelta_t ffwrite(AddrType addr, DataType data);
  void        ffinvalidate(AddrType addr, int32_t lineSize);
  private:
  uint32_t getBank(MemRequest *mreq) const;
  uint32_t getRow(MemRequest *mreq) const;
  uint32_t getColumn(MemRequest *mreq) const;
  void addMemRequest(MemRequest *mreq);
  
  void transferOverflowMemory(void);
  void scheduleNextAction(void);
  
};




#endif
