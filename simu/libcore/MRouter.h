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
