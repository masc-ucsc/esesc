/* License & includes {{{1 */
/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2010 University of California, Santa Cruz.

   Contributed by Jose Renau

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

#ifndef MROUTER_H
#define MROUTER_H

#include "estl.h"
#include <vector>

#include "Snippets.h"
#include "RAWDInst.h"

#include "nanassert.h"

//DM
#include "Resource.h"
/* }}} */

class MemObj;
class MemRequest;

class MRouter {
private:
protected:
  class MemObjHashFunc {
    public:
      size_t operator()(const MemObj *mobj) const {
        HASH<const char *> H;
        return H((const char *)mobj);
      }
  };
  
  MemObj *self_mobj;

  typedef HASH_MAP<const MemObj *, MemObj *, MemObjHashFunc> UPMapType;
  UPMapType up_map;
  std::vector<MemObj *> up_node;
  std::vector<MemObj *> down_node;

  void updateRouteTables(MemObj *upmobj, MemObj * const top_node);

public:
  MRouter(MemObj *obj);
  virtual ~MRouter();

  void fillRouteTables();
  void addUpNode(MemObj *upm);
  void addDownNode(MemObj *upm);

  void fwdReadPos(uint32_t pos, MemRequest              *mreq);
  void fwdReadPos(uint32_t pos, MemRequest              *mreq,  TimeDelta_t lat);
  void fwdRead(MemRequest              *mreq) { 
    fwdReadPos(0,mreq); 
  }
  void fwdRead(MemRequest              *mreq,  TimeDelta_t lat) {
    fwdReadPos(0,mreq,lat); 
  }

  void fwdWritePos(uint32_t pos, MemRequest             *mreq);
  void fwdWritePos(uint32_t pos, MemRequest             *mreq,  TimeDelta_t lat);
  void fwdWrite(MemRequest             *mreq) {
    fwdWritePos(0,mreq);
  };
  void fwdWrite(MemRequest             *mreq,  TimeDelta_t lat) {
    fwdWritePos(0,mreq,lat);
  };

  void fwdPushDownPos(uint32_t pos, MemRequest          *mreq);
  void fwdPushDownPos(uint32_t pos, MemRequest          *mreq,  TimeDelta_t lat);
  void fwdPushDown(MemRequest          *mreq) {
    fwdPushDownPos(0,mreq);
  }
  void fwdPushDown(MemRequest          *mreq,  TimeDelta_t lat) {
    fwdPushDownPos(0,mreq,lat);
  }

  void fwdPushUpPos(uint32_t pos, MemRequest            *mreq);
  void fwdPushUpPos(uint32_t pos, MemRequest            *mreq,  TimeDelta_t lat);
  void fwdPushUp(MemRequest            *mreq);
  void fwdPushUp(MemRequest            *mreq,  TimeDelta_t lat);

  void fwdBusReadPos(uint32_t pos, MemRequest           *mreq);
  void fwdBusReadPos(uint32_t pos, MemRequest           *mreq,  TimeDelta_t lat);
  void fwdBusRead(MemRequest           *mreq) {
    fwdBusReadPos(0,mreq);
  }
  void fwdBusRead(MemRequest           *mreq,  TimeDelta_t lat) {
    fwdBusReadPos(0,mreq,lat);
  }

  void sendBusReadPos(uint32_t pos, MemRequest          *mreq);
  void sendBusReadPos(uint32_t pos, MemRequest          *mreq,  TimeDelta_t lat);
  void sendBusRead(MemRequest          *mreq) {
    sendBusReadPos(0,mreq);
  }
  void sendBusRead(MemRequest          *mreq,  TimeDelta_t lat) {
    sendBusReadPos(0,mreq,lat);
  }

  bool sendInvalidateOthers(int32_t    lsize, const MemRequest *mreq, AddrType naddr,  TimeDelta_t lat);
  bool sendInvalidateAll(int32_t       lsize, const MemRequest *mreq, AddrType naddr,  TimeDelta_t lat);
  bool sendInvalidatePos(uint32_t pos, int32_t       lsize, const MemRequest *mreq, AddrType naddr,  TimeDelta_t lat);

  bool canAcceptReadPos(uint32_t pos, DInst *dinst) const;
  bool canAcceptWritePos(uint32_t pos, DInst *dinst) const;
  bool canAcceptRead(DInst *dinst) const {
    return canAcceptReadPos(0,dinst);
  }
  bool canAcceptWrite(DInst *dinst) const {
    return canAcceptWritePos(0,dinst);
  }

  TimeDelta_t ffreadPos(uint32_t pos, AddrType addr, DataType data);
  TimeDelta_t ffwritePos(uint32_t pos, AddrType addr, DataType data);
  TimeDelta_t ffread(AddrType addr, DataType data) {
    return ffreadPos(0,addr,data);
  }
  TimeDelta_t ffwrite(AddrType addr, DataType data) {
    return ffwritePos(0,addr,data);
  }
};

#endif
