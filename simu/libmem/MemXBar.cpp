// copyright and includes {{{1
// Contributed by David Munday
//                Jose Renau
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

#include "MemXBar.h"
#include "MemorySystem.h"
#include "SescConf.h"

/* }}} */

MemXBar::MemXBar(const char *section, const char *name)
    : GXBar(section, name) { /*{{{*/

  setParam(section, name);

} /*}}}*/

MemXBar::MemXBar(MemorySystem *current, const char *section, const char *name)
    /* {{{ constructor */
    : GXBar(section, name) {

  I(current);
  lower_level_banks = NULL;

  setParam(section, name);

  lower_level_banks = new MemObj *[numLowerLevelBanks];
  XBar_rw_req       = new GStatsCntr *[numLowerLevelBanks];

  std::vector<char *> vPars      = SescConf->getSplitCharPtr(section, "lowerLevel");
  const char *        lower_name = "";
  if(vPars.size() > 1)
    lower_name = vPars[1];

  for(size_t i = 0; i < numLowerLevelBanks; i++) {
    char *tmp = (char *)malloc(255);
    if(numLowerLevelBanks > 1)
      sprintf(tmp, "%s%s(%lu)", name, lower_name, i);
    else
      sprintf(tmp, "%s%s", name, lower_name);
    lower_level_banks[i] = current->declareMemoryObj_uniqueName(tmp, vPars[0]);
    addLowerLevel(lower_level_banks[i]);

    XBar_rw_req[i] = new GStatsCntr("%s_to_%s:rw_req", name, lower_level_banks[i]->getName());
  }
}
/* }}} */

void MemXBar::setParam(const char *section, const char *name) { /*{{{*/
  SescConf->isInt(section, "dropBits");

  dropBits = SescConf->getInt(section, "dropBits");

  numLowerLevelBanks = SescConf->getRecordSize(section, "lowerLevel");

} /*}}}*/

uint32_t MemXBar::addrHash(AddrType addr) const
// {{{ drop lower bits in address
{
  addr = addr >> dropBits;
  return (addr % numLowerLevelBanks);
}
// }}}

void MemXBar::doReq(MemRequest *mreq)
/* read if splitter above L1 (down) {{{1 */
{
  if(mreq->getAddr() == 0) {
    mreq->ack();
    return;
  }

  uint32_t pos = addrHash(mreq->getAddr());
  I(pos < numLowerLevelBanks);

  mreq->resetStart(lower_level_banks[pos]);
  XBar_rw_req[pos]->inc(mreq->getStatsFlag());

  router->scheduleReqPos(pos, mreq);
}
/* }}} */

void MemXBar::doReqAck(MemRequest *mreq)
/* req ack (up) {{{1 */
{
  I(0);

  if(mreq->isHomeNode()) {
    mreq->ack();
    return;
  }
  router->scheduleReqAck(mreq);
}
/* }}} */

void MemXBar::doSetState(MemRequest *mreq)
/* setState (up) {{{1 */
{
  // FIXME
  I(0); // You should check the L1 as incoherent, so that it does not send invalidated to higher levels
  router->sendSetStateAll(mreq, mreq->getAction());
}
/* }}} */

void MemXBar::doSetStateAck(MemRequest *mreq)
/* setStateAck (down) {{{1 */
{
  uint32_t pos = addrHash(mreq->getAddr());
  router->scheduleSetStateAckPos(pos, mreq);
  // FIXME: use dinst->getPE() to decide who to send up if GPU mode
  // I(0);
}
/* }}} */

void MemXBar::doDisp(MemRequest *mreq)
/* disp (down) {{{1 */
{
  uint32_t pos = addrHash(mreq->getAddr());
  router->scheduleDispPos(pos, mreq);
  I(0);
  // FIXME: use dinst->getPE() to decide who to send up if GPU mode
}
/* }}} */

bool MemXBar::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  uint32_t pos = addrHash(addr);
  return router->isBusyPos(pos, addr);
}
/* }}} */

void MemXBar::tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* fast forward reads {{{1 */
{
  uint32_t pos = addrHash(addr);
  router->tryPrefetchPos(pos, addr, degree, doStats, pref_sign, pc, cb);
}
/* }}} */

TimeDelta_t MemXBar::ffread(AddrType addr)
/* fast forward reads {{{1 */
{
  uint32_t pos = addrHash(addr);
  return router->ffreadPos(pos, addr);
}
/* }}} */

TimeDelta_t MemXBar::ffwrite(AddrType addr)
/* fast forward writes {{{1 */
{
  uint32_t pos = addrHash(addr);
  return router->ffwritePos(pos, addr);
}
/* }}} */
