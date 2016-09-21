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

#include "GMemorySystem.h"

#include "MemObj.h"
#include "MemRequest.h"
#include "SescConf.h"
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

uint16_t MemObj::id_counter = 0;

MemObj::MemObj(const char *sSection, const char *sName)
   /* constructor {{{1 */
  :section(sSection)
  ,name(sName)
  ,id(id_counter++)
{
  deviceType = SescConf->getCharPtr(section,"deviceType");

	coreid = -1; // No first Level cache by default
  firstLevelIL1 = false;
  firstLevelDL1 = false;
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


void MemObj::addLowerLevel(MemObj *obj) { 
	router->addDownNode(obj);
	I( obj );
	obj->addUpperLevel(this);
	//printf("%s with lower level %s\n",getName(),obj->getName());
}

void MemObj::addUpperLevel(MemObj *obj) { 
	//printf("%s upper level is %s\n",getName(),obj->getName());
	router->addUpNode(obj);
}

void MemObj::blockFill(MemRequest *mreq)
{
	// Most objects do nothing
}

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

void DummyMemObj::doReq(MemRequest *req)    
  /* req {{{1 */
{ 
  I(0);
  req->ack();
}
/* }}} */

void DummyMemObj::doReqAck(MemRequest *req)    
  /* reqAck {{{1 */
{ 
  I(0);
}
/* }}} */

void DummyMemObj::doSetState(MemRequest *req)    
  /* setState {{{1 */
{ 
  I(0);
  req->ack(); 
}
/* }}} */

void DummyMemObj::doSetStateAck(MemRequest *req)    
  /* setStateAck {{{1 */
{ 
  I(0);
}
/* }}} */

void DummyMemObj::doDisp(MemRequest *req)    
  /* disp {{{1 */
{ 
  I(0);
}
/* }}} */

bool DummyMemObj::isBusy(AddrType addr) const 
// Can it accept more requests {{{1
{
	return false;
}
// }}}

void DummyMemObj::tryPrefetch(AddrType addr, bool doStats)
  /* drop the try prefetch {{{1 */
{ 
}
/* }}} */

TimeDelta_t DummyMemObj::ffread(AddrType addr)
  /* fast forward read {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

TimeDelta_t DummyMemObj::ffwrite(AddrType addr)
  /* fast forward write {{{1 */
{ 
  return 1;   // 1 cycle does everything :)
}
/* }}} */

/* Optional virtual methods {{{1 */
bool MemObj::checkL2TLBHit(MemRequest *req) {
  // If called, it should be redefined by the object
  I(0);
  return false;
}
void MemObj::replayCheckLSQ_removeStore(DInst *) {
	I(0);
}
void MemObj::updateXCoreStores(AddrType addr) {
	I(0);
}
void MemObj::replayflush() {
	I(0);
}
void MemObj::setTurboRatio(float r) { 
	I(0);
}
void MemObj::plug() {
	I(0);
}
void MemObj::setNeedsCoherence() {
	// Only cache uses this
}
void MemObj::clearNeedsCoherence() {
}

bool MemObj::Invalid(AddrType addr) const {
  I(0);
}
/* }}} */
