// Copyright and includes {{{1
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

#include "MRouter.h"
#include "MemRequest.h"

#include "DrawArch.h"
extern DrawArch arch;

/* }}} */

MRouter::MRouter(MemObj *obj)
    /* constructor {{{1 */
    : self_mobj(obj) {
  down_node.clear();
  up_node.clear();
  // down_mobj = 0;
}
/* }}} */

MRouter::~MRouter()
/* destructor {{{1 */
{
}
/* }}} */

int16_t MRouter::getCreatorPort(const MemRequest *mreq) const {
  if(up_node.empty())
    return -1;

  if(up_node.size() == 1)
    return 0; // Most common case

  UPMapType::const_iterator it = up_map.find(mreq->getCreator());
  if(it == up_map.end()) {
    return -1; // This happens when a mreq is created by the middle node
  }

  MemObj *obj = it->second;

  for(size_t i = 0; i < up_node.size(); i++) {
    if(up_node[i] == obj)
      return i;
  }

  return -1; // Not Found
}

void MRouter::fillRouteTables()
// populate router tables with the up/down nodes {{{1
{

  I(up_node.size() == 0); // First level cache only
  I(up_map.size() == 0);
  MSG("Fill router is %s", self_mobj->getName());

  // it can be a nicecache I(down_node.size()>=1);
  for(size_t i = 0; i < down_node.size(); i++) {
    down_node[i]->getRouter()->updateRouteTables(self_mobj, self_mobj);
  }

  // Look for nicecache or memory, and propagate up to the nodes that do not need coherence (llc)
  MemObj * bottom = self_mobj;
  MRouter *rb     = this;
  while(rb) {
    bottom = rb->self_mobj;
    if(rb->down_node.empty())
      rb = 0;
    else
      rb = rb->down_node[0]->getRouter();
  }
  MSG("Bottom:clear router is %s", bottom->getName());
  rb = bottom->getRouter();
  rb->self_mobj->clearNeedsCoherence();
  while(rb->up_node.size() == 1) {
    MemObj *uobj = rb->up_node[0];
    MSG("single:clear router is %s", uobj->getName());
    uobj->clearNeedsCoherence();
    rb = uobj->router;
  }
  if(!rb->up_node.empty()) {
    for(size_t i = 0; i < rb->up_node.size(); i++) {
      MemObj *uobj = rb->up_node[0];
      //      MSG("Use:set router is %s",uobj->getName());
      uobj->setNeedsCoherence();
    }
  }
}
// }}}

void MRouter::updateRouteTables(MemObj *upmobj, MemObj *const top_node)
/* regenerate the routing tables {{{1 */
{
  //  MSG("Use:set router is %s",upmobj->getName());
  upmobj->setNeedsCoherence();

  up_map[top_node] = upmobj;
  // MSG("Router %s: Setting upmap[%s (Addr = %p)] = %s (Addr =
  // %p)",self_mobj->getName(),top_node->getName(),top_node,upmobj->getName(),upmobj);
  for(size_t i = 0; i < down_node.size(); i++) {
    down_node[i]->getRouter()->updateRouteTables(self_mobj, top_node);
    down_node[i]->getRouter()->updateRouteTables(self_mobj, self_mobj);
  }

#ifdef DEBUG
  bool found = false;
  for(size_t i = 0; i < up_node.size(); i++) {
    if(up_node[i] == upmobj) {
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
  // Generating memory_arch.html.
  std::string mystr("[{v:'");
  mystr += self_mobj->getName();
  mystr += "',f: '";
  mystr += self_mobj->getName();
  mystr += "<div style=\"font-size:9; color:red; font-style:italic\">";
  mystr += self_mobj->getSection();
  mystr += "</div>'}, '";
  mystr += dpm->getName();
  mystr += "', '";
  mystr += self_mobj->getSection();
  mystr += "']";
  // MSG("xxx>%s",mystr.c_str());
  arch.addObj(mystr);
#else
  std::string mystr("");
  mystr += "\"";
  mystr += self_mobj->getName();
  mystr += "\"";
  // mystr += self_mobj->getSection();
  // mystr += "\"";
  mystr += " -> ";
  mystr += "\"";
  mystr += dpm->getName();
  mystr += "\"";
  arch.addObj(mystr);
#endif

  // down_mobj = upm;
}
/* }}} */

void MRouter::scheduleReqPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
/* schedule req down {{{1 */
{
  I(down_node.size() > pos);
  mreq->startReq(down_node[pos], lat);
}
/* }}} */

void MRouter::scheduleReq(MemRequest *mreq, TimeDelta_t lat)
/* schedule req down {{{1 */
{
  I(down_node.size() == 1);
  mreq->startReq(down_node[0], lat);
}
/* }}} */

void MRouter::scheduleReqAck(MemRequest *mreq, TimeDelta_t lat)
/* schedule reqAck up {{{1 */
{
  I(!up_node.empty());
  MemObj *obj = up_node[0];

  if(up_node.size() > 1) {
    UPMapType::const_iterator it = up_map.find(mreq->getHomeNode());
    I(it != up_map.end());
    obj = it->second;
  }

  mreq->startReqAck(obj, lat);
}
/* }}} */

void MRouter::scheduleReqAckAbs(MemRequest *mreq, Time_t w)
/* schedule reqAck up {{{1 */
{
  I(!up_node.empty());
  MemObj *obj = up_node[0];
  if(up_node.size() > 1) {
    UPMapType::const_iterator it = up_map.find(mreq->getHomeNode());
    I(it != up_map.end());
    obj = it->second;
  }
  obj->blockFill(mreq);
  mreq->startReqAckAbs(obj, w);
}
/* }}} */

void MRouter::scheduleReqAckPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
/* schedule push up {{{1 */
{
  I(!up_node.empty());
  I(pos < up_node.size());

  mreq->startReqAck(up_node[pos], lat);
}
/* }}} */

void MRouter::scheduleSetStatePos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
/* schedule setState up {{{1 */
{
  I(!up_node.empty());
  I(pos < up_node.size());

  mreq->startSetState(up_node[pos], lat);
}
/* }}} */

void MRouter::scheduleSetStateAck(MemRequest *mreq, TimeDelta_t lat)
/* schedule setStateAck down {{{1 */
{
  I(down_node.size() == 1);

  mreq->startSetStateAck(down_node[0], lat);
}
/* }}} */

void MRouter::scheduleSetStateAckPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
/* schedule setStateAck down {{{1 */
{
  I(down_node.size() > pos);

  mreq->startSetStateAck(down_node[pos], lat);
}
/* }}} */

void MRouter::scheduleDispPos(uint32_t pos, MemRequest *mreq, TimeDelta_t lat)
/* schedule Displace (down) {{{1 */
{
  I(down_node.size() > pos);
  mreq->startDisp(down_node[pos], lat);
}
/* }}} */

void MRouter::scheduleDisp(MemRequest *mreq, TimeDelta_t lat)
/* schedule Displace (down) {{{1 */
{
  I(down_node.size() == 1);
  mreq->startDisp(down_node[0], lat);
}
/* }}} */

void MRouter::sendDirtyDisp(AddrType addr, bool doStats, TimeDelta_t lat)
/* schedule Displace (down) {{{1 */
{
  I(down_node.size() == 1);
  MemRequest::sendDirtyDisp(down_node[0], self_mobj, addr, doStats);
}
/* }}} */
void MRouter::sendCleanDisp(AddrType addr, bool prefetch, bool doStats, TimeDelta_t lat)
/* schedule Displace (down) {{{1 */
{
  I(down_node.size() == 1);
  MemRequest::sendCleanDisp(down_node[0], self_mobj, addr, prefetch, doStats);
}
/* }}} */

int32_t MRouter::sendSetStateOthers(MemRequest *mreq, MsgAction ma, TimeDelta_t lat)
/* send setState to others, return how many {{{1 */
{
  if(up_node.size() <= 1)
    return 0; // if single node, for sure it does not get one

  bool     doStats = mreq->getStatsFlag();
  AddrType addr    = mreq->getAddr();

  MemObj *                  skip_mobj = 0;
  UPMapType::const_iterator it        = up_map.find(mreq->getHomeNode());
  I(it != up_map.end());
  skip_mobj = it->second;

  int32_t conta = 0;
  I(mreq->isReq() || mreq->isReqAck());
  for(size_t i = 0; i < up_node.size(); i++) {
    if(up_node[i] == skip_mobj)
      continue;

    if(addr == 0xa000000005601213) {
      I(0);
    }

    MemRequest *breq = MemRequest::createSetState(self_mobj, mreq->getCreator(), ma, addr, doStats);
    breq->addPendingSetStateAck(mreq);

    breq->startSetState(up_node[i], lat);
    conta++;
  }

  return conta;
}
/* }}} */

int32_t MRouter::sendSetStateOthersPos(uint32_t pos, MemRequest *mreq, MsgAction ma, TimeDelta_t lat)
/* send setState to specific pos, return how many {{{1 */
{
  if(up_node.size() <= 1)
    return 0; // if single node, for sure it does not get one

  bool     doStats = mreq->getStatsFlag();
  AddrType addr    = mreq->getAddr();

  if(addr == 0xa000000005601213) {
    I(0);
  }

  MemRequest *breq = MemRequest::createSetState(self_mobj, mreq->getCreator(), ma, addr, doStats);
  breq->addPendingSetStateAck(mreq);

  breq->startSetState(up_node[pos], lat);

  return 1;
}
/* }}} */

int32_t MRouter::sendSetStateAll(MemRequest *mreq, MsgAction ma, TimeDelta_t lat)
/* send setState to others, return how many {{{1 */
{
  if(up_node.empty())
    return 0; // top node?

  bool     doStats = mreq->getStatsFlag();
  AddrType addr    = mreq->getAddr();
  if(addr == 0xa000000005601213) {
    I(0);
  }

  I(mreq->isSetState());
  int32_t conta = 0;
  for(size_t i = 0; i < up_node.size(); i++) {
    MemRequest *breq = MemRequest::createSetState(self_mobj, mreq->getCreator(), ma, addr, doStats);
    breq->addPendingSetStateAck(mreq);

    breq->startSetState(up_node[i], lat);
    conta++;
  }

  return conta;
}
/* }}} */

void MRouter::tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* propagate the prefetch to the lower level {{{1 */
{
  down_node[0]->tryPrefetch(addr, doStats, degree, pref_sign, pc, cb);
}
/* }}} */

void MRouter::tryPrefetchPos(uint32_t pos, AddrType addr, int degree, bool doStats, AddrType pref_sign, AddrType pc,
                             CallbackBase *cb)
/* propagate the prefetch to the lower level {{{1 */
{
  I(pos < down_node.size());
  down_node[pos]->tryPrefetch(addr, doStats, degree, pref_sign, pc, cb);
}
/* }}} */

TimeDelta_t MRouter::ffread(AddrType addr)
/* propagate the read to the lower level {{{1 */
{
  return down_node[0]->ffread(addr);
}
/* }}} */

TimeDelta_t MRouter::ffwrite(AddrType addr)
/* propagate the read to the lower level {{{1 */
{
  return down_node[0]->ffwrite(addr);
}
/* }}} */

TimeDelta_t MRouter::ffreadPos(uint32_t pos, AddrType addr)
/* propagate the read to the lower level {{{1 */
{
  I(pos < down_node.size());
  return down_node[pos]->ffread(addr);
}
/* }}} */

TimeDelta_t MRouter::ffwritePos(uint32_t pos, AddrType addr)
/* propagate the read to the lower level {{{1 */
{
  I(pos < down_node.size());
  return down_node[pos]->ffwrite(addr);
}
/* }}} */

bool MRouter::isBusyPos(uint32_t pos, AddrType addr) const
/* propagate the isBusy {{{1 */
{
  I(pos < down_node.size());
  return down_node[pos]->isBusy(addr);
}
/* }}} */
