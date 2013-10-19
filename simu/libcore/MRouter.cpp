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
#include "MRouter.h"
#include "MemRequest.h"

#include "DrawArch.h"
extern DrawArch arch;

/* }}} */

MRouter::MRouter(MemObj *obj)
  /* constructor {{{1 */
  :self_mobj(obj)
{
  down_node.clear();
  up_node.clear();
  //down_mobj = 0;
}
/* }}} */

MRouter::~MRouter()
  /* destructor {{{1 */
{
}
/* }}} */

void MRouter::fillRouteTables() {
  I(up_node.size()==0); // First level cache only
  I(up_map.size() == 0);

  I(down_node.size()>=1);
  for (size_t i = 0; i < down_node.size(); i++){
    down_node[i]->getRouter()->updateRouteTables(self_mobj, self_mobj);
  }
}

void MRouter::updateRouteTables(MemObj *upmobj, MemObj * const top_node)
  /* regenerate the routing tables {{{1 */
{  

  up_map[top_node] = upmobj;
  for(size_t i=0;i<down_node.size();i++) {
    down_node[i]->getRouter()->updateRouteTables(self_mobj, top_node);
    down_node[i]->getRouter()->updateRouteTables(self_mobj, self_mobj);
  }

#ifdef DEBUG
  bool found=false;
  for(size_t i=0;i<up_node.size();i++) {
    if (up_node[i] == upmobj) {
      found = true;
      break;
    }
  }
  I(found);
#endif
}
/* }}} */

void MRouter::addUpNode(MemObj *upm)
  /* add up node {{{1 */
{
  up_node.push_back(upm);
}
/*  }}} */

void MRouter::addDownNode(MemObj *dpm)
  /* add down node {{{1 */
{
  down_node.push_back(dpm);

#if GOOGLE_GRAPH_API
//Generating memory_arch.html.  
  std::string mystr("[{v:'");
  mystr += self_mobj->getName();
  mystr += "',f: '";
  mystr += self_mobj->getName();
  mystr += "<div style=\"font-size:9; color:red; font-style:italic\">";
  mystr += self_mobj->getSection();
  mystr  += "</div>'}, '";
  mystr  += dpm->getName();
  mystr += "', '";
  mystr += self_mobj->getSection(); 
  mystr += "']";
  //MSG("xxx>%s",mystr.c_str());
  arch.addObj(mystr);
#else
  std::string mystr("");
  mystr += "\"";
  mystr += self_mobj->getName();
  mystr += "\"";
  //mystr += self_mobj->getSection();
  //mystr += "\"";
  mystr += " -> ";
  mystr += "\"";
  mystr  += dpm->getName();
  mystr += "\"";
  arch.addObj(mystr);
#endif

  //down_mobj = upm;
}
/* }}} */

void MRouter::fwdPushDownPos(uint32_t pos, MemRequest *mreq)
  /* fwd push down {{{1 */
{
  I(down_node.size()>pos);
  mreq->setNextHop(down_node[pos]);
  mreq->fwdPushDown();
}
/* }}} */
void MRouter::fwdPushDownPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
  /* fwd push down {{{1 */
{
  I(down_node.size()>pos);
  mreq->setNextHop(down_node[pos]);
  mreq->fwdPushDown(lat);
}
/* }}} */

void MRouter::fwdPushUp(MemRequest *mreq)
  /* fwd push up {{{1 */
{
  I(!up_node.empty());
  MemObj *obj = up_node[0];
  if (up_node.size()>1) {
    UPMapType::const_iterator it = up_map.find(mreq->getHomeNode());
    I(it != up_map.end());
    obj = it->second;
  }
  mreq->setNextHop(obj);
  mreq->fwdPushUp();
}
/* }}} */

void MRouter::fwdPushUpPos(uint32_t pos, MemRequest *mreq)
  /* fwd push up {{{1 */
{
  I(!up_node.empty());
  I(pos<up_node.size());

  mreq->setNextHop(up_node[pos]);
  mreq->fwdPushUp();
}
/* }}} */
void MRouter::fwdPushUp(MemRequest *mreq, TimeDelta_t lat)
  /* fwd push up {{{1 */
{
  I(!up_node.empty());
  MemObj *obj = up_node[0];
  UPMapType::const_iterator it;

  if (up_node.size()>1) {
    it = up_map.find(mreq->getHomeNode());
    I(it != up_map.end());
    obj = it->second;
  }

  I(obj);

  mreq->setNextHop(obj);
  mreq->fwdPushUp(lat);
}
/* }}} */
void MRouter::fwdPushUpPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
  /* fwd push up {{{1 */
{
  I(!up_node.empty());
  I(pos<up_node.size());

  mreq->setNextHop(up_node[pos]);
  mreq->fwdPushUp(lat);
}
/* }}} */

void MRouter::fwdBusReadPos(uint32_t pos, MemRequest *mreq)
  /* fwd bus read (down) {{{1 */
{
  I(down_node.size()>pos);
  mreq->setNextHop(down_node[pos]);
  mreq->fwdBusRead();
}
/* }}} */
void MRouter::fwdBusReadPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
  /* fwd bus read (down) {{{1 */
{
  I(down_node.size()>pos);
  mreq->setNextHop(down_node[pos]);
  mreq->fwdBusRead(lat);
}
/* }}} */

bool MRouter::sendInvalidateOthers(int32_t lsize, const MemRequest *mreq, AddrType naddr, TimeDelta_t lat)
  /* send invalidate {{{1 */
{
  if (up_node.size() <= 1)
    return false; // if single node, for sure it does not get one

  MemObj *skip_mobj            = 0;
  UPMapType::const_iterator it = up_map.find(mreq->getHomeNode());
  I(it != up_map.end());
  skip_mobj                    = it->second;

  MemRequest *ireq = 0;
  for(size_t i=0;i<up_node.size();i++) {
    if (up_node[i] == skip_mobj) 
      continue;

    MemRequest *breq;
    if(ireq==0) {
      ireq = MemRequest::createInvalidate(self_mobj, naddr, lsize, false /*not a capacity Invalidate */);
      breq = ireq;
    }else{
      breq = ireq->createInvalidate();
    }
    breq->setNextHop(up_node[i]);
    breq->fwdInvalidate(lat);
  }

  return true;
}
/* }}} */

bool MRouter::sendInvalidateAll(int32_t lsize, const MemRequest *mreq, AddrType naddr, TimeDelta_t lat)
  /* send invalidate to all the up nodes {{{1 */
{
  if(up_node.empty())
    return false;

  MemRequest *ireq = 0;
  for(size_t i=0;i<up_node.size();i++) {
    MemRequest *breq;
    if(ireq==0) {
      ireq = MemRequest::createInvalidate(self_mobj, naddr, lsize, true /*capacity Invalidate*/);
      breq = ireq;
    }else{
      breq = ireq->createInvalidate();
    }
    breq->setNextHop(up_node[i]);
    breq->fwdInvalidate(lat);
  }

  return true;
}
/* }}} */
bool MRouter::sendInvalidatePos(uint32_t pos, int32_t lsize, const MemRequest *mreq, AddrType naddr, TimeDelta_t lat)
  /* send invalidate to all the up nodes {{{1 */
{
  if(up_node.empty())
    return false;

  I(up_node.size()>pos);
  MemRequest *breq= MemRequest::createInvalidate(self_mobj, naddr, lsize, true /*capacity Invalidate*/);

  breq->setNextHop(up_node[pos]);
  breq->fwdInvalidate(lat);

  return true;
}
/* }}} */

bool MRouter::canAcceptReadPos(uint32_t pos, DInst *dinst) const 
/* Check if the lower level can accept a new addr request {{{1 */
{
  //I(down_node.size()==1);
  return down_node[pos]->canAcceptRead(dinst);
}
/* }}} */
bool MRouter::canAcceptWritePos(uint32_t pos, DInst *dinst) const 
/* Check if the lower level can accept a new addr request {{{1 */
{
  //I(down_node.size()==1);
  return down_node[pos]->canAcceptWrite(dinst);
}
/* }}} */

TimeDelta_t MRouter::ffreadPos(uint32_t pos, AddrType addr, DataType data)
  /* propagate the read to the lower level {{{1 */
{
  return down_node[pos]->ffread(addr,data);
}
/* }}} */

TimeDelta_t MRouter::ffwritePos(uint32_t pos, AddrType addr, DataType data)
  /* propagate the read to the lower level {{{1 */
{
  return down_node[pos]->ffwrite(addr,data);
}
/* }}} */


