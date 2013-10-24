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


