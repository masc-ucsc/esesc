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

#include "UnMemXBar.h"
#include "MemorySystem.h"
#include "SescConf.h"
/* }}} */

UnMemXBar::UnMemXBar(MemorySystem *current, const char *section, const char *name)
    /* constructor {{{1 */
    : GXBar(section, name) {
  MSG("building an UnMemXbar named:%s\n", name);
  Xbar_unXbar_balance--; // decrement balance of XBars
  lower_level = NULL;

  SescConf->isInt(section, "lowerLevelBanks");
  SescConf->isPower2(section, "lowerLevelBanks");

  SescConf->isInt(section, "LineSize");
  SescConf->isInt(section, "Modfactor");

  numLowerLevelBanks = SescConf->getInt(section, "lowerLevelBanks");
  LineSize           = SescConf->getInt(section, "LineSize");
  Modfactor          = SescConf->getInt(section, "Modfactor");

  if(Modfactor < numLowerLevelBanks) {
    printf("ERROR: UNXBAR: %s does not have a Modfactor(%d) bigger than the number of structures(%d) below it!\n", name, Modfactor,
           numLowerLevelBanks);
    exit(1);
  }

#ifdef OLD_CODE_TO_BE_PHASED_OUT
  std::vector<char *> vPars = SescConf->getSplitCharPtr(section, "lowerLevel");
  size_t              size  = strlen(vPars[0]);
  char *              tmp   = (char *)malloc(size + 6);
  sprintf(tmp, "%s", vPars[1]);
  lower_level = current->declareMemoryObj_uniqueName(tmp, vPars[0]);
#else
  lower_level = current->declareMemoryObj(section, "lowerLevel");
  // Must have only one lower level!
#endif

  addLowerLevel(lower_level);
  I(current);
}
/* }}} */

void UnMemXBar::doReq(MemRequest *mreq)
/* read if splitter above L1 (down) {{{1 */
{
  router->scheduleReq(mreq);
}
/* }}} */

void UnMemXBar::doReqAck(MemRequest *mreq)
/* req ack (up) {{{1 */
{
  I(!mreq->isHomeNode());

  uint32_t pos = addrHash(mreq->getAddr(), LineSize, Modfactor, numLowerLevelBanks);
  router->scheduleReqAckPos(pos, mreq);
}
/* }}} */

void UnMemXBar::doSetState(MemRequest *mreq)
/* setState (up) {{{1 */
{
  uint32_t pos = addrHash(mreq->getAddr(), LineSize, Modfactor, numLowerLevelBanks);
  router->scheduleSetStatePos(pos, mreq);
}
/* }}} */

void UnMemXBar::doSetStateAck(MemRequest *mreq)
/* setStateAck (down) {{{1 */
{
  router->scheduleSetStateAck(mreq);
}
/* }}} */

void UnMemXBar::doDisp(MemRequest *mreq)
/* disp (down) {{{1 */
{
  router->scheduleDisp(mreq);
}
/* }}} */

bool UnMemXBar::isBusy(AddrType addr) const
/* always can accept writes {{{1 */
{
  return router->isBusyPos(0, addr);
}
/* }}} */

TimeDelta_t UnMemXBar::ffread(AddrType addr)
/* fast forward reads {{{1 */
{
  return router->ffread(addr);
}
/* }}} */

TimeDelta_t UnMemXBar::ffwrite(AddrType addr)
/* fast forward writes {{{1 */
{
  return router->ffwrite(addr);
}
/* }}} */
