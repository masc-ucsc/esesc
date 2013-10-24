/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela

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

#ifndef MEMOBJ_H
#define MEMOBJ_H

#include "DInst.h"
#include "callback.h"

#include "MRouter.h"
#include "nanassert.h"

#include "Resource.h"

class MemRequest;      // Memory Request (from processor to cache)

class MemObj {
private:
protected:
  MRouter *router;
  const char *section;
  const char *name;

  void addLowerLevel(MemObj *obj) { 
    router->addDownNode(obj);
    I( obj );
    obj->addUpperLevel(this);
    printf("%s with lower level %s\n",getName(),obj->getName());
  }
  void addLowerLevel(MRouter *router, MemObj *obj) { 
    I(0); // Stop using this OLD CODE
    router->addDownNode(obj);
    I( obj );
    obj->addUpperLevel(this);
    printf("%s with lower level %s\n",getName(),obj->getName());
  }

  void addUpperLevel(MemObj *obj) { 
    printf("%s upper level is %s\n",getName(),obj->getName());
    router->addUpNode(obj);
  }
  void addUpperLevel(MRouter *router, MemObj *obj) { 
    I(0); // Stop using this OLD CODE
    printf("%s upper level is %s\n",getName(),obj->getName());
    router->addUpNode(obj);
  }

public:
  MemObj(const char *section, const char *sName);
  MemObj();
  virtual ~MemObj();

  const char *getSection() const { return section; }
  const char *getName() const    { return name;    }

  MRouter *getRouter()           { return router;  }
  
  virtual Time_t nextReadSlot(       const MemRequest *mreq) = 0;
  virtual Time_t nextWriteSlot(      const MemRequest *mreq) = 0;
  virtual Time_t nextBusReadSlot(    const MemRequest *mreq) = 0;
  virtual Time_t nextPushDownSlot(   const MemRequest *mreq) = 0;
  virtual Time_t nextPushUpSlot(     const MemRequest *mreq) = 0;
  virtual Time_t nextInvalidateSlot( const MemRequest *mreq) = 0;

  // Interface for fast-forward (no BW, just warmup caches)
  virtual TimeDelta_t ffread(AddrType addr, DataType data) = 0;
  virtual TimeDelta_t ffwrite(AddrType addr, DataType data) = 0;
  virtual void        ffinvalidate(AddrType addr, int32_t lineSize) = 0;

  // processor direct requests
  virtual void read(MemRequest  *req) = 0;
  virtual void write(MemRequest *req) = 0;
  virtual void writeAddress(MemRequest *req) = 0;

  // DOWN
  virtual void busRead(MemRequest *req) = 0;
  virtual void pushDown(MemRequest *req) = 0;

  // UP
  virtual void pushUp(MemRequest *req) = 0;
  virtual void invalidate(MemRequest *req) = 0;
  
  // When the buffers in the cache are full and it does not accept more requests
  virtual bool canAcceptRead(DInst *dinst) const = 0;
  virtual bool canAcceptWrite(DInst *dinst) const = 0;

  // Print stats
  virtual void dump() const;

  virtual void plug() {
    // nothing by default, but it can be use to link with gprocessor
  };

  // TLB direct requests  
  virtual bool checkL2TLBHit(MemRequest *req);

  virtual void replayCheckLSQ_removeStore(DInst *) {};
  virtual void updateXCoreStores(AddrType addr) {};
  virtual void replayflush() {};

};

class DummyMemObj : public MemObj {
private:
protected:

  Time_t nextReadSlot(       const MemRequest *mreq);
  Time_t nextWriteSlot(      const MemRequest *mreq);
  Time_t nextBusReadSlot(    const MemRequest *mreq);
  Time_t nextPushDownSlot(   const MemRequest *mreq);
  Time_t nextPushUpSlot(     const MemRequest *mreq);
  Time_t nextInvalidateSlot( const MemRequest *mreq);

  // processor direct requests
  void read(MemRequest  *req);      // processor read (not propagated down)
  void write(MemRequest *req);      // processor write (not propagated down)
  void writeAddress(MemRequest *req);   // processor write (not propagated down)

  // DOWN
  void busRead(MemRequest *req);    // cache miss or exclusive request (busRead | busReadX)
  void pushDown(MemRequest *req);   // Writeback or Invalidate Ack (it can be empty)

  // UP
  void pushUp(MemRequest *req);     // Similar to pushDown but bottom->up direction (busRead Ack)
  void invalidate(MemRequest *req); // A busRead (X) would trigger invalidates (which should trigger pushLine)

  bool canAcceptRead(DInst *dinst) const;
  bool canAcceptWrite(DInst *dinst) const;

  // TLB direct requests  
  bool cheatCheckHit(MemRequest *req){
    return false;
  };

public:
  DummyMemObj();
  DummyMemObj(const char *section, const char *sName);

  TimeDelta_t ffread(AddrType addr, DataType data);
  TimeDelta_t ffwrite(AddrType addr, DataType data);
  void        ffinvalidate(AddrType addr, int32_t lineSize);

};

#endif // MEMOBJ_H
