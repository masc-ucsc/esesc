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

#include "SescConf.h"
#include "MemorySystem.h"
#include "MemRequest.h"

#include "NICECache.h"
/* }}} */

NICECache::NICECache(MemorySystem *gms, const char *section, const char *sName)
  /* dummy constructor {{{1 */
  : MemObj(section, sName)
  ,hitDelay  (SescConf->getInt(section,"hitDelay"))
  ,readHit         ("%s:readHit",         sName)
  ,pushDownHit     ("%s:pushDownHit",     sName)
  ,writeHit        ("%s:writeHit",        sName)
{
}
/* }}} */

Time_t NICECache::nextReadSlot(       const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+hitDelay;
}
/* }}}*/
Time_t NICECache::nextWriteSlot(      const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+hitDelay;
}
/* }}}*/
Time_t NICECache::nextBusReadSlot(    const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+hitDelay;
}
/* }}}*/
Time_t NICECache::nextPushDownSlot(   const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t NICECache::nextPushUpSlot(     const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t NICECache::nextInvalidateSlot( const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/

void NICECache::read(MemRequest *req)    
  /* read (down) {{{1 */
{ 
  readHit.inc(req->getStatsFlag());
  req->ack();
}
/* }}} */

void NICECache::write(MemRequest *req)    
  /* write (down) {{{1 */
{ 
  writeHit.inc(req->getStatsFlag());
  req->ack();
}
/* }}} */

void NICECache::writeAddress(MemRequest *req)    
  /* write (down) {{{1 */
{ 
  req->ack();
}
/* }}} */

void NICECache::busRead(MemRequest *req)    
  /* bus read (down) {{{1 */
{ 
  readHit.inc(req->getStatsFlag());
  router->fwdPushUp(req); // ack the busRead
}
/* }}} */

void NICECache::pushDown(MemRequest *req)    
  /* push (down) {{{1 */
{ 
  pushDownHit.inc(req->getStatsFlag());
  I(req->isWriteback());
  req->destroy();
}
/* }}} */

void NICECache::pushUp(MemRequest *req)    
  /* push (up) {{{1 */
{ 
  I(0);
}
/* }}} */

void NICECache::invalidate(MemRequest *req)    
  /* push (up) {{{1 */
{ 
  router->fwdPushDown(req); // invalidate ack
}
/* }}} */

//DM
//bool NICECache::canAcceptRead(AddrType addr) const
bool NICECache::canAcceptRead(DInst *dinst) const
  /* can accept reads? {{{1 */
{ 
  return true;  
}
/* }}} */

//DM
//bool NICECache::canAcceptWrite(AddrType addr) const
bool NICECache::canAcceptWrite(DInst *dinst) const
  /* can accept writes? {{{1 */
{ 
  return true;  
}
/* }}} */

TimeDelta_t NICECache::ffread(AddrType addr, DataType data)
  /* can accept writes? {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

TimeDelta_t NICECache::ffwrite(AddrType addr, DataType data)
  /* can accept writes? {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

void NICECache::ffinvalidate(AddrType addr, int32_t lineSize)
  /* can accept writes? {{{1 */
{
  // Nothing to do
}
/* }}} */


