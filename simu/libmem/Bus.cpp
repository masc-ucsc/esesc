// Contributed by Jose Renau
//                Basilio Fraguela
//                Luis Ceze
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

#include "Bus.h"
#include "SescConf.h"
/* }}} */

Bus::Bus(MemorySystem *current, const char *section, const char *name)
    /* constructor {{{1 */
    : MemObj(section, name)
    , delay(SescConf->getInt(section, "delay")) {
  // busyUpto = 0;

  SescConf->isInt(section, "numPorts");
  SescConf->isInt(section, "portOccp");
  SescConf->isInt(section, "delay");

  NumUnits_t  num = SescConf->getInt(section, "numPorts");
  TimeDelta_t occ = SescConf->getInt(section, "portOccp");

  char cadena[100];
  sprintf(cadena, "Data%s", name);
  dataPort = PortGeneric::create(cadena, num, occ);
  sprintf(cadena, "Cmd%s", name);
  cmdPort = PortGeneric::create(cadena, num, 1);

  I(current);
  MemObj *lower_level = current->declareMemoryObj(section, "lowerLevel");
  if(lower_level) {
    addLowerLevel(lower_level);
  }
}
/* }}} */

void Bus::doReq(MemRequest *mreq)
/* forward bus read {{{1 */
{
  // MSG("@%lld bus %s 0x%lx %d",globalClock, mreq->getCurrMem()->getName(), mreq->getAddr(), mreq->getAction());
  TimeDelta_t when = cmdPort->nextSlotDelta(mreq->getStatsFlag()) + delay;
  router->scheduleReq(mreq, when);
}
/* }}} */

void Bus::doDisp(MemRequest *mreq)
/* forward bus read {{{1 */
{
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag()) + delay;
  router->scheduleDisp(mreq, when);
}
/* }}} */

void Bus::doReqAck(MemRequest *mreq)
/* data is coming back {{{1 */
{
  TimeDelta_t when = dataPort->nextSlotDelta(mreq->getStatsFlag()) + delay;

  if(mreq->isHomeNode()) {
    mreq->ack(when);
    return;
  }

  router->scheduleReqAck(mreq, when);
}
/* }}} */

void Bus::doSetState(MemRequest *mreq)
/* forward set state to all the upper nodes {{{1 */
{
  if(router->isTopLevel()) {
    mreq->convert2SetStateAck(ma_setInvalid, false); // same as a miss (not sharing here)
    router->scheduleSetStateAck(mreq, 1);
    return;
  }
  router->sendSetStateAll(mreq, mreq->getAction(), delay);
}
/* }}} */

void Bus::doSetStateAck(MemRequest *mreq)
/* forward set state to all the lower nodes {{{1 */
{
  if(mreq->isHomeNode()) {
    mreq->ack();
    return;
  }
  router->scheduleSetStateAck(mreq, delay);
}
/* }}} */

bool Bus::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  return false;
}
/* }}} */

void Bus::tryPrefetch(AddrType addr, bool doStats, int degree, AddrType pref_sign, AddrType pc, CallbackBase *cb)
/* forward tryPrefetch {{{1 */
{
  router->tryPrefetch(addr, doStats, degree, pref_sign, pc, cb);
}
/* }}} */

TimeDelta_t Bus::ffread(AddrType addr)
/* fast forward reads {{{1 */
{
  return delay;
}
/* }}} */

TimeDelta_t Bus::ffwrite(AddrType addr)
/* fast forward writes {{{1 */
{
  return delay;
}
/* }}} */
