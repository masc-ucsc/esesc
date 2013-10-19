/* License & includes {{{1 */
/* 
   ESESC: Super ESCalar simulator
   Copyright (C) 2012 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Max Dunne
                  Shea Ellerson

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
#include "MemController.h"
#include <iostream>
#include "stdlib.h"
#include <queue>
#include <cmath>
#include <vector>
/* }}} */

MemController::MemController(MemorySystem* current ,const char *section ,const char *name)
  /* constructor {{{1 */
  : MemObj(section, name)
  ,delay(SescConf->getInt(section, "delay"))
  ,PreChargeLatency(SescConf->getInt(section, "PreChargeLatency"))
  ,RowAccessLatency(SescConf->getInt(section, "RowAccessLatency"))
  ,ColumnAccessLatency(SescConf->getInt(section, "ColumnAccessLatency"))
  ,nPrecharge("%s:nPrecharge", name)
  ,nColumnAccess("%s:nColumnAccess", name)
  ,nRowAccess("%s:nRowAccess", name)
  ,avgMemLat("%s_avgMemlat", name)
  ,memRequestBufferSize(SescConf->getInt(section, "memRequestBufferSize"))
{
  MemObj *lower_level = NULL;
  SescConf->isInt(section, "numPorts");
  SescConf->isInt(section, "portOccp");
  SescConf->isInt(section, "delay");
  SescConf->isGT(section, "delay",0);

  NumUnits_t  num = SescConf->getInt(section, "numPorts");
  TimeDelta_t occ = SescConf->getInt(section, "portOccp");

  char cadena[100];
  sprintf(cadena,"Cmd%s", name);
  cmdPort  = PortGeneric::create(cadena, num, occ);

  SescConf->isPower2(section, "numRows",0);
  SescConf->isPower2(section, "numColumns",0);
  SescConf->isPower2(section, "numBanks",0);
  SescConf->isGT(section, "ColumnAccessLatency",4); // 1 cycle is not supported

  numBanks = SescConf->getInt(section, "NumBanks");
  unsigned int numRows = SescConf->getInt(section, "NumRows");
  unsigned int ColumnSize = SescConf->getInt(section, "ColumnSize");
  unsigned int numColumns = SescConf->getInt(section, "NumColumns");

  columnOffset = log2(ColumnSize);
  columnMask   = numColumns-1;
  columnMask   = columnMask<<columnOffset;      // FIXME: Use AddrType

  rowOffset    = columnOffset+log2(numColumns);
  rowMask      = numRows - 1;
  rowMask      = rowMask<<rowOffset; // FIXME: use AddrType

  bankOffset   = rowOffset+log2(numRows);
  bankMask     = numBanks -1;
  bankMask     = bankMask<<bankOffset;
  
  bankState = new BankStatus[numBanks];
  for(uint32_t curBank=0; curBank<numBanks;curBank++){
    bankState[curBank].activeRow=0;
    bankState[curBank].state=ACTIVE;
  }
  I(current);
  lower_level = current->declareMemoryObj(section, "lowerLevel");   
  if (lower_level)
    addLowerLevel(lower_level);
}
/* }}} */

Time_t MemController::nextReadSlot(       const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0); // It can not be at the first level 
  return globalClock; 
}
/* }}} */
Time_t MemController::nextWriteSlot(      const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0); // It can not be at the first level
  return globalClock;
}
/* }}} */
Time_t MemController::nextBusReadSlot(    const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t MemController::nextPushDownSlot(   const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  return cmdPort->nextSlot();
}
/* }}} */
Time_t MemController::nextPushUpSlot(     const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0); // Nobody is under MemController
  return globalClock;
}
/* }}} */
Time_t MemController::nextInvalidateSlot( const MemRequest *mreq)
  /* calculate next free time {{{1 */
{
  I(0); // Nobody is under MemController
  return globalClock;
}
/* }}} */

void MemController::read(MemRequest *mreq)
  /* no read in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */
void MemController::write(MemRequest *mreq)
  /* no write in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */
void MemController::writeAddress(MemRequest *mreq)
  /* no writeAddress in bus {{{1 */
{
  I(0); // Bus should not be a first level object
}
/* }}} */

void MemController::busRead(MemRequest *mreq)
  /* forward bus read {{{1 */
{
  addMemRequest(mreq);
}
/* }}} */
void MemController::pushDown(MemRequest *mreq)
  /* push down {{{1 */
{
  addMemRequest(mreq);
}
/* }}} */
void MemController::pushUp(MemRequest *mreq)
  /* push up {{{1 */
{
  I(0);
}
/* }}} */

void MemController::invalidate(MemRequest *mreq)
  /* forward invalidate to the higher levels {{{1 */
{
  I(0);
}
/* }}} */

bool MemController::canAcceptRead(DInst *dinst) const
/* always can accept writes {{{1 */
{
  return true;
}
/* }}} */

bool MemController::canAcceptWrite(DInst *disnt) const
/* always can accept reads {{{1 */
{
  return true;
}
/* }}} */

TimeDelta_t MemController::ffread(AddrType addr, DataType data)
  /* fast forward reads {{{1 */
{ 
  return delay + RowAccessLatency;
}
/* }}} */

TimeDelta_t MemController::ffwrite(AddrType addr, DataType data)
  /* fast forward writes {{{1 */
{ 
  return delay + RowAccessLatency;
}
/* }}} */

void MemController::ffinvalidate(AddrType addr, int32_t ilineSize)
  /* fast forward invalidate {{{1 */
{ 
  I(0);
  // FIXME: router->sendInvalidateAll(mreq->getLineSize(), mreq, mreq->getAddr(), delay);
}
/* }}} */


void MemController::addMemRequest(MemRequest *mreq)
{
  FCFSField *newEntry= new FCFSField;
  newEntry->Bank=getBank(mreq);
  newEntry->Row=getRow(mreq);
  newEntry->Column=getColumn(mreq);
  newEntry->mreq=mreq;
  newEntry->TimeEntered=globalClock;
  OverflowMemoryRequests.push(newEntry);
  manageRam();
}

// This function implements the FR-FCFS memory scheduling algorithm
void MemController::manageRam(void)
{
  
  // First, we need to determine if any actions (precharging, activating, or accessing) have been completed
  for(uint32_t curBank=0; curBank <numBanks;curBank++){
    if((bankState[curBank].state == PRECHARGE) && (globalClock - bankState[curBank].bankTime >= PreChargeLatency)){

      bankState[curBank].state= IDLE;

    }else if ((bankState[curBank].state == ACTIVATING) && (globalClock - bankState[curBank].bankTime >= RowAccessLatency)){

      bankState[curBank].state=ACTIVE;

    }else if ((bankState[curBank].state==ACCESSING)&&(globalClock - bankState[curBank].bankTime >=ColumnAccessLatency)){

      bankState[curBank].state = ACTIVE;

      for (FCFSList::iterator it = curMemRequests.begin();
           it != curMemRequests.end();
           it++) {

        FCFSField *tempMem = *it;

        if((curBank==tempMem->Bank)&&(bankState[curBank].activeRow==tempMem->Row)) {

          I(tempMem->mreq);
          if(tempMem->mreq->isWriteback())
            tempMem->mreq->destroy();
          else {
            router->fwdPushUp(tempMem->mreq,globalClock-tempMem->TimeEntered);
            avgMemLat.sample(globalClock-tempMem->TimeEntered);
          }
          IS(tempMem->mreq = 0);
          
          curMemRequests.erase(it);

          break;
        }
      }
    }
  }
  
  // Call function to replace any deleted address with a new one from queue
  transferOverflowMemory();
  // Call function to determine what the next action should begin
  scheduleNextAction();
}


// This function adds any pending references in the queue to the buffer if there is space available
void MemController::transferOverflowMemory(void){
  while((curMemRequests.size()<=memRequestBufferSize) && (!OverflowMemoryRequests.empty())){
    curMemRequests.push_back(OverflowMemoryRequests.front());
    OverflowMemoryRequests.pop();
  }
}

// This function determines what action can be performed next and schedules a callback for when that action completes
void MemController::scheduleNextAction(void)
{
  uint32_t curBank,curRow;
  uint32_t oldestReadyColsBank=numBanks+1;
  uint32_t oldestReadyRowsBank=numBanks+1;
  uint32_t oldestReadyRow=0;
  uint32_t oldestbank=0;
  
  bool oldestColumnFound=false;
  bool oldestRowFound=false;
  bool oldestBankFound=false;
  
  // Go through memory references in buffer to determine what actions are ready to begin
  for (uint32_t curReference=0; curReference < curMemRequests.size(); curReference++){
    curBank=curMemRequests[curReference]->Bank;
    curRow=curMemRequests[curReference]->Row;
    if(!oldestColumnFound){
      if ((bankState[curBank].state == ACTIVE) &&(bankState[curBank].activeRow==curRow)){
        bankState[curBank].cpend = true;
        oldestColumnFound=true;
        if(bankState[oldestReadyColsBank].bankTime > bankState[curBank].bankTime){
          oldestReadyColsBank=curBank;
        }        
      }
    }
    if((bankState[curBank].state == ACTIVE) &&(bankState[curBank].activeRow!=curRow)){
      bankState[curBank].bpend = true;

    }
    if(!oldestRowFound){
      if(bankState[curBank].state == IDLE){
        oldestRowFound=true;
        if(bankState[oldestReadyRowsBank].bankTime > bankState[curBank].bankTime){
          oldestReadyRow=curMemRequests[curReference]->Row;
          oldestReadyRowsBank=curBank;
        }
      }
    }

  }
  
  //... and determine if a bank has no pending column references
  for (uint32_t curBank=0;curBank<numBanks;curBank++){
    if((bankState[curBank].bpend)&&(!bankState[curBank].cpend)){
      oldestBankFound=true;
      if(bankState[oldestbank].bankTime > bankState[curBank].bankTime){
        oldestbank=curBank;
      }  
    }
    bankState[curBank].bpend=false;
    bankState[curBank].cpend=false;
  }
  
  // Now determine which of the ready actions should be start and when the callback should occur
  if(oldestBankFound){
    bankState[oldestbank].state = PRECHARGE;
    bankState[oldestbank].bankTime=globalClock;

    nPrecharge.inc();

    ManageRamCB::schedule(PreChargeLatency,this);  
  } else if(oldestColumnFound){
    bankState[oldestReadyColsBank].state=ACCESSING;
    bankState[oldestReadyColsBank].bankTime=globalClock;

    nColumnAccess.inc();

    ManageRamCB::schedule(ColumnAccessLatency,this);
  } else if (oldestRowFound){
    bankState[oldestReadyRowsBank].state=ACTIVATING;
    bankState[oldestReadyRowsBank].bankTime=globalClock;
    bankState[oldestReadyRowsBank].activeRow=oldestReadyRow;

    nRowAccess.inc();

    ManageRamCB::schedule(RowAccessLatency,this);
  }
}

uint32_t MemController::getBank(MemRequest *mreq) const
{
  uint32_t bank = (mreq->getAddr()&bankMask) >> bankOffset;
  return bank;
}
uint32_t MemController::getRow(MemRequest *mreq) const
{
  uint32_t row = (mreq->getAddr()&rowMask) >>rowOffset;
  return row;
}

uint32_t MemController::getColumn(MemRequest *mreq) const
{
  uint32_t column = (mreq->getAddr()&columnMask) >>columnOffset;
  return column;
}
