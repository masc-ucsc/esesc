/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2012 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Max Dunne
                  Shea Ellerson

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

#ifndef MEMCONTROLLER_H
#define MEMCONTROLLER_H

#include "GStats.h"
#include "CacheCore.h"
#include "Port.h"
#include "MemRequest.h"
#include "MemObj.h"
#include "DDR2.h"
#include "MSHR.h"

#include "SescConf.h"
#include "MemorySystem.h"
#include "Cache.h"
#include "MSHR.h"
#include "GProcessor.h"
#include "callback.h"

#include "estl.h"
#include "CacheCore.h"
#include "Port.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MSHR.h"
#include "Snippets.h"
#include "MemController.h"


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

  Time_t nextReadSlot(       const MemRequest *mreq);
  Time_t nextWriteSlot(      const MemRequest *mreq);
  Time_t nextBusReadSlot(    const MemRequest *mreq);
  Time_t nextPushDownSlot(   const MemRequest *mreq);
  Time_t nextPushUpSlot(     const MemRequest *mreq);
  Time_t nextInvalidateSlot( const MemRequest *mreq);

  // processor direct requests
  void read(MemRequest  *req);
  void write(MemRequest *req);
  void writeAddress(MemRequest *req);

  // DOWN
  void busRead(MemRequest *req);
  void pushDown(MemRequest *req);
  
  // UP
  void pushUp(MemRequest *req);
  void invalidate(MemRequest *req);
  // Status/state
  uint16_t getLineSize() const;

  bool canAcceptRead(DInst *dinst) const;
  bool canAcceptWrite(DInst *dinst) const;
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
