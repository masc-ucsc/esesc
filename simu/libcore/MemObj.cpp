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

#include "GMemorySystem.h"

#include "MemObj.h"
#include "MemRequest.h"
/* }}} */

#ifdef DEBUG
/* Debug class to search for duplicate names {{{1 */
class Setltstr {
public:
  bool operator()(const char* s1, const char* s2) const {
    return strcasecmp(s1, s2) < 0;
  }
};
/* }}} */
#endif

MemObj::MemObj(const char *sSection, const char *sName)
   /* constructor {{{1 */
  :section(sSection)
  ,name(sName)
{
  
  // Create router (different objects may override the default router)
  router = new MRouter(this);

#ifdef DEBUG
  static std::set<const char *, Setltstr> usedNames;
  if( sName ) {
    // Verify that one else uses the same name
    if( usedNames.find(sName) != usedNames.end() ) {
      MSG("Creating multiple memory objects with the same name '%s' (rename one of them)",sName);
    }else{
      usedNames.insert(sName);
    }
  }
#endif
}
/* }}} */

MemObj::~MemObj() 
  /* destructor {{{1 */
{
  if(section != 0) 
    delete [] section;
  if(name != 0) 
    delete [] name;
}
/* }}} */

void MemObj::dump() const
/* dump statistics {{{1 */
{
  LOG("MemObj name [%s]",name);
}
/* }}} */

DummyMemObj::DummyMemObj() 
  /* dummy constructor {{{1 */
  : MemObj("dummySection", "dummyMem")
{ 
}
/* }}} */

DummyMemObj::DummyMemObj(const char *section, const char *sName)
  /* dummy constructor {{{1 */
  : MemObj(section, sName)
{
}
/* }}} */

Time_t DummyMemObj::nextReadSlot(       const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t DummyMemObj::nextWriteSlot(      const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t DummyMemObj::nextBusReadSlot(    const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t DummyMemObj::nextPushDownSlot(   const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t DummyMemObj::nextPushUpSlot(     const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/
Time_t DummyMemObj::nextInvalidateSlot( const MemRequest *mreq)
  /* get next free slot {{{1 */
{
  return globalClock+1;
}
/* }}}*/

void DummyMemObj::read(MemRequest *req)    
  /* read (down) {{{1 */
{ 
  req->ack();
}
/* }}} */

void DummyMemObj::write(MemRequest *req)    
  /* write (down) {{{1 */
{ 
  req->ack();
}
/* }}} */

void DummyMemObj::writeAddress(MemRequest *req)    
  /* write (down) {{{1 */
{ 
  req->ack();
}
/* }}} */

void DummyMemObj::busRead(MemRequest *req)    
  /* bus read (down) {{{1 */
{ 
  router->fwdPushUp(req); // ack the busRead
}
/* }}} */

void DummyMemObj::pushDown(MemRequest *req)    
  /* push (down) {{{1 */
{ 
  I(0);
}
/* }}} */

void DummyMemObj::pushUp(MemRequest *req)    
  /* push (up) {{{1 */
{ 
  I(0);
}
/* }}} */

void DummyMemObj::invalidate(MemRequest *req)    
  /* push (up) {{{1 */
{ 
  router->fwdPushDown(req); // invalidate ack
}
/* }}} */

bool DummyMemObj::canAcceptRead(DInst *dinst) const
  /* can accept reads? {{{1 */
{ 
  return true;  
}
/* }}} */

bool DummyMemObj::canAcceptWrite(DInst *dinst) const
  /* can accept writes? {{{1 */
{ 
  return true;  
}
/* }}} */

TimeDelta_t DummyMemObj::ffread(AddrType addr, DataType data)
  /* fast forward read {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

TimeDelta_t DummyMemObj::ffwrite(AddrType addr, DataType data)
  /* fast forward write {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

void DummyMemObj::ffinvalidate(AddrType addr, int32_t lineSize)
  /* fast forward invalidate {{{1 */

{
  // Nothing to do
}
/* }}} */

bool MemObj::checkL2TLBHit(MemRequest *req) {
  // If called, it should be redefined by the object
  I(0);
  return false;
}

