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

MemRequest::MemRequest()
  /* constructor {{{1 */
  :readCB(this)
  ,writeCB(this)
  ,writeAddressCB(this)
  ,busReadCB(this)
  ,pushDownCB(this)
  ,pushUpCB(this)
  ,invalidateCB(this)
{
  pending    = 0;
}
/* }}} */

MemRequest::~MemRequest() 
{
  // to avoid warnings
}

pool<MemRequest>  MemRequest::actPool(256, "MemRequest");

MemRequest *MemRequest::create(MemObj *mobj, AddrType addr, CallbackBase *cb) 
{
  I(mobj);

  MemRequest *r = actPool.out();

  r->addr        = addr;
  r->homeMemObj  = mobj;
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
  r->prefetch    = false;
  r->mmu         = false;
  r->pending     = 0;

  return r;
}

#ifdef DEBUG_CALLPATH
void MemRequest::dump_calledge(TimeDelta_t lat)
{
  bool interesting = false;
  Time_t total = 0;
  for(size_t i = 0;i<calledge.size();i++) {
    CallEdge ce  = calledge[i];
    total       += ce.tismo;
    if (ce.type == msg_invalidate || ce.type == msg_writeback) {
      interesting = true;
      break;
    }
  }
  if(!interesting || total >200)
    return;

  rawdump_calledge(lat, total);
}

void MemRequest::rawdump_calledge(TimeDelta_t lat, Time_t total) {
  if(calledge.empty())
    return;

  printf("digraph path{\n");
  static int32_t conta=0;
  printf("  ce [label=\"0x%x addr %d# @%lld delta %lu\"]\n",(unsigned int)addr,(int)conta++,(long long)globalClock,total);

  CacheDebugAccess *c = CacheDebugAccess::getInstance();

  c->setAddrsAccessed((int32_t)conta);
  c->mapReset();

  char gname[1024];
  for(size_t i=0;i<calledge.size();i++) {
    CallEdge ce = calledge[i];
    // get Type
    char t;
    switch(ce.type) {
      case msg_read:       t = 'R'; break;
      case msg_write:      t = 'W'; break;
      case msg_writeback:  t = 'B'; break;
      case msg_invalidate: t = 'I'; break;
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

    printf(" [label=\"%d%c%d\"]\n", (int)i, t, ce.tismo);
  }
  printf("  %s -> CPU [label=\"%dA%d\"]\n",gname, (int)calledge.size(), lat);

  printf("  {rank=same; P0DL1 P0IL1 P1DL1 P1IL1}\n");
  printf("}\n");

}

void MemRequest::upce(TimeDelta_t lat)
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
  ce.tismo      = globalClock-lastCallTime+lat;
  ce.type       = type;
  I(globalClock>=lastCallTime);
  lastCallTime  = globalClock+lat;
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
/* destroy/recycle current and parent_req messages {{{1 */
{
  if(pending>0)
    return;

  if (parent_req)
    parent_req->destroy(); // Try to destroy the parent again

  actPool.in(this);
}
/* }}} */

void MemRequest::read()
/* perform read (called from a CB) {{{1 */
{
  currMemObj->read(this);
}
/* }}} */

void MemRequest::write()
/* perform write (called from a CB) {{{1 */
{
  currMemObj->write(this);
}
/* }}} */

void MemRequest::writeAddress()
/* perform write (called from a CB) {{{1 */
{
  currMemObj->writeAddress(this);
}
/* }}} */

void MemRequest::busRead()
/* perform busRead (called from a CB) {{{1 */
{
  currMemObj->busRead(this);
}
/* }}} */

void MemRequest::pushDown()
/* perform pushDown (called from a CB) {{{1 */
{
  currMemObj->pushDown(this);
}
/* }}} */

void MemRequest::pushUp()
/* perform pushUp (called from a CB) {{{1 */
{
  currMemObj->pushUp(this);
}
/* }}} */

void MemRequest::invalidate()
/* perform invalidate (called from a CB) {{{1 */
{
  currMemObj->invalidate(this);
}
/* }}} */


