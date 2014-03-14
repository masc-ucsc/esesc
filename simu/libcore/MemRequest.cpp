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

#include <string.h>
#include <set>

#include "SescConf.h"

#include "GMemorySystem.h"

#include "MemRequest.h"
#include "MemObj.h"
#include "Pipeline.h"
#include "Resource.h"
#include "Cluster.h"
#include "MemStruct.h"
/* }}} */

pool<MemRequest>  MemRequest::actPool(256, "MemRequest");

MemRequest::MemRequest()
  /* constructor  */
  :doReqCB(this)
  ,doReqAckCB(this)
  ,doSetStateCB(this)
  ,doSetStateAckCB(this)
  ,doDispCB(this)
{
}
/*  */

MemRequest::~MemRequest() 
	// destructor 
{
  // to avoid warnings
}
// 


void MemRequest::doReq()         { upce(); currMemObj->doReq(this);         }
void MemRequest::doReqAck()      { upce(); currMemObj->doReqAck(this);      }
void MemRequest::doSetState()    { upce(); currMemObj->doSetState(this);    }
void MemRequest::doSetStateAck() { upce(); currMemObj->doSetStateAck(this); }
void MemRequest::doDisp()        { upce(); currMemObj->doDisp(this);        }

void MemRequest::addPendingSetStateAck(MemRequest *mreq) 
{
  I(mreq->id < id);
  I(mreq->mt == mt_setState || mreq->mt == mt_req);
  I(mt == mt_setState);
  mreq->pendingSetStateAck++;
  I(setStateAckOrig == 0);
  setStateAckOrig = mreq;
}

void MemRequest::setStateAckDone()
{  
  MemRequest *orig = setStateAckOrig;
  setStateAckOrig = 0;
  I(orig);
  I(orig->pendingSetStateAck>0);
  orig->pendingSetStateAck--;
  if (orig->pendingSetStateAck<=0) {
    if(orig->mt == mt_req) {
      orig->doReq();
    }else if (orig->mt == mt_setState) {
      I(orig->setStateAckOrig==0);
      orig->ack();
      //orig->setStateAckDone(); No recursive/dep chains for the moment
    }else{
      I(0);
    }
  }
}

void MemRequest::req()         { I(mt == mt_req);         currMemObj->req(this);         }
void MemRequest::reqAck()      { I(mt == mt_reqAck);      currMemObj->reqAck(this);      }
void MemRequest::setState()    { I(mt == mt_setState);    currMemObj->setState(this);    }
void MemRequest::setStateAck() { I(mt == mt_setStateAck); currMemObj->setStateAck(this); }
void MemRequest::disp()        { I(mt == mt_disp);        currMemObj->disp(this);        }

MemRequest *MemRequest::create(MemObj *mobj, AddrType addr, bool doStats, CallbackBase *cb) 
{
  I(mobj);

  MemRequest *r = actPool.out();

  r->addr        = addr;
  r->homeMemObj  = mobj;
  r->creatorObj  = mobj;
  r->currMemObj  = mobj;
#ifdef DEBUG
  static uint64_t current_id = 0;
  r->id          = current_id++;
#endif
#ifdef DEBUG_CALLPATH
  r->prevMemObj  = 0;
  r->calledge.clear();
  r->lastCallTime= globalClock;
#endif
  r->cb          = cb;
  r->startClock  = globalClock;
	r->retrying = false;
	r->doStats  = doStats;
  r->pendingSetStateAck = 0;
  r->setStateAckOrig    = 0;

  return r;
}

#ifdef DEBUG_CALLPATH
void MemRequest::dump_calledge(TimeDelta_t lat, bool interesting)
{
  return; // FIXME

  Time_t total = 0;
  for(size_t i = 0;i<calledge.size();i++) {
    CallEdge ce  = calledge[i];
    total       += ce.tismo;
    if (ce.mt == mt_setState || ce.mt == mt_disp) {
      interesting = true;
      break;
    }
  }
//  if(!interesting && total <200)
//    return;

  rawdump_calledge(lat, total);
}

void MemRequest::rawdump_calledge(TimeDelta_t lat, Time_t total) {
  if(calledge.empty())
    return;

  printf("digraph path{\n");
  printf("  ce [label=\"0x%x addr id %ld delta %lu @%lld\"]\n",(unsigned int)addr,id,total,(long long)globalClock);

  CacheDebugAccess *c = CacheDebugAccess::getInstance();
  c->mapReset();

  char gname[1024];
  for(size_t i=0;i<calledge.size();i++) {
    CallEdge ce = calledge[i];
    // get Type
    const char *t;
    switch(ce.mt) {
      case mt_req         : t = "RQ"; break;
      case mt_reqAck      : t = "RA"; break;
      case mt_setState    : t = "SS"; break;
      case mt_setStateAck : t = "SA"; break;
      case mt_disp        : t = "DI"; break;
      default: I(0);
    }
		const char *a;
    switch(ce.ma) {
      case ma_setInvalid   : a = "I"; break;
      case ma_setValid     : a = "V"; break;
      case ma_setDirty     : a = "D"; break;
      case ma_setShared    : a = "S"; break;
      case ma_setExclusive : a = "E"; break;
      case ma_VPCWU        : a = "VPC"; break;
      case ma_MMU          : a = "MMU"; break;
      default: I(0);
    }
    int k;
    const char *name;
    // get START NAME
    k=0;
    if (ce.s==0) {
      printf("  CPU ");
    }else{
      name = ce.s->getName();
      for(int j=0;name[j]!=0;j++) {
        if(isalnum(name[j])) {
          gname[k++]=name[j];
        }
      }
      gname[k]=0;
      printf("  %s",gname);

      c->setCacheAccess(gname);
    }
    // get END NAME
    k=0;
    name = ce.e->getName();
    for(int j=0;name[j]!=0;j++) {
      if(isalnum(name[j])) {
        gname[k++]=name[j];
      }
    }
    gname[k]=0;
    printf(" -> %s",gname);

    printf(" [label=\"%d%s_%s%d\"]\n", (int)i, t, a, ce.tismo);
  }
  printf("  %s -> CPU [label=\"%dRA%d\"]\n",gname, (int)calledge.size(), lat);

  printf("  {rank=same; P0DL1 P0IL1 P1DL1 P1IL1}\n");
  printf("}\n");

}

void MemRequest::upce()
{
  CallEdge ce;
  ce.s          = prevMemObj;
  ce.e          = currMemObj;
/*
  if (ce.s == 0x0)
    MSG("mreq %d: starts:NULL  ends:%s",id, ce.e->getName());
  else if (ce.s == 0x0)
    MSG("mreq %d: starts:%s ends:NULL",id, ce.s->getName());
  else
    MSG("mreq %d: starts:%s ends:%s",id, ce.s->getName(), ce.e->getName());
*/
  ce.tismo      = globalClock-lastCallTime;
  ce.mt         = mt;
  ce.ma         = ma;
  I(globalClock>=lastCallTime);
  lastCallTime  = globalClock;
  MemRequest *p = this;
  p->calledge.push_back(ce);
}
#endif

void MemRequest::setNextHop(MemObj *newMemObj) 
{
#ifdef DEBUG_CALLPATH
  prevMemObj = currMemObj;
#endif
  currMemObj = newMemObj;
}

void MemRequest::destroy() 
/* destroy/recycle current and parent_req messages  */
{
  actPool.in(this);
}
/*  */

