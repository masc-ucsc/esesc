/*  
    SESC: Super ESCalar simulator
    Copyright (C) 2003 University of Illinois.
    
    Contributed by Engin Ipek
    
    This file is part of SESC.
    
    SESC is free software; you can redistribute it and/or modify it under the terms
    of the GNU General Public License as published by the Free Software Foundation;
    either version 2, or (at your option) any later version.
    
    SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
    PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    
    You should  have received a copy of  the GNU General  Public License along with
    SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
    Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "SescConf.h"
#include "Snippets.h"
#include "DDR2.h"
#include "Cache.h"

#include <iostream>
#include <algorithm>

//DRAM clock, scaled wrt globalClock
Time_t DRAMClock; 

//DRAM energy
long DRAMEnergy;

//Duration of one epoch
Time_t DRAMEpoch;

//Memory reference constructor
MemRef::MemRef()
  : timeStamp(0), 
    mReq(NULL),
    rankID(-1),
    bankID(-1),
    rowID(-1),
    prechargePending(true), 
    activatePending(true),
    readPending(true),
    writePending(true),
    ready(false),
    valid(false),
    read(true),
    loadMiss(false),
    loadMissSeqNum(-1),
    loadMissCoreID(-1)
{
  //Nothing to do
} 

//Complete this transaction
void MemRef::complete(Time_t when)
{
  
  //Reset all state
  rankID = -1;
  bankID = -1;
  rowID = -1;
  prechargePending = true;
  activatePending = true;
  readPending = true;
  writePending = true;
  ready = false;
  valid = false;
  read = true;
  loadMiss = false;
  loadMissSeqNum = -1;
  loadMissCoreID = -1;
  
  //Return the memory request to higher levels of the memory hierarchy
  mReq->ackAbs(when); 
  
  //Reset request
  mReq = NULL;
  
} 
 
//Rank constructor
DDR2Rank::DDR2Rank(const char *section)
  : powerState(IDD2N), 
    emr(FAST),
    cke(true), 
    lastPrecharge(0),
    lastActivate(0),
    lastRead(0),
    lastWrite(0)
{
  
  //Check energy parameters
  SescConf->isInt(section, "eIDD2P");
  SescConf->isInt(section, "eIDD2N");
  SescConf->isInt(section, "eIDD3N");
  SescConf->isInt(section, "eIDD3PF");
  SescConf->isInt(section, "eIDD3PS");
  SescConf->isInt(section, "eActivate");
  SescConf->isInt(section, "ePrecharge");
  SescConf->isInt(section, "eRead");
  SescConf->isInt(section, "eWrite");
  
  //Check timing parameters
  SescConf->isInt(section, "tRCD");
  SescConf->isInt(section, "tCL");
  SescConf->isInt(section, "tWL");
  SescConf->isInt(section, "tCCD");
  SescConf->isInt(section, "tWTR");
  SescConf->isInt(section, "tWR");
  SescConf->isInt(section, "tRTP");
  SescConf->isInt(section, "tRP");
  SescConf->isInt(section, "tRRD");
  SescConf->isInt(section, "tRAS");
  SescConf->isInt(section, "tRC");
  SescConf->isInt(section, "tRTRS");
  SescConf->isInt(section, "tOST");
  SescConf->isInt(section, "BL");
  
  //Check bank count
  SescConf->isInt(section, "numBanks");
  
  //Read timing parameters
  tRCD            = SescConf->getInt(section, "tRCD");
  tCL             = SescConf->getInt(section, "tCL");
  tWL             = SescConf->getInt(section, "tWL");
  tCCD            = SescConf->getInt(section, "tCCD");
  tWTR            = SescConf->getInt(section, "tWTR");
  tWR             = SescConf->getInt(section, "tWR");
  tRTP            = SescConf->getInt(section, "tRTP");
  tRP             = SescConf->getInt(section, "tRP");
  tRRD            = SescConf->getInt(section, "tRRD");
  tRAS            = SescConf->getInt(section, "tRAS");
  tRC             = SescConf->getInt(section, "tRC");
  tRTRS           = SescConf->getInt(section, "tRTRS");
  tOST            = SescConf->getInt(section, "tOST"); 
  BL              = SescConf->getInt(section, "BL");
  
  //Read energy parameters
  eActivate       = SescConf->getInt(section, "eActivate");
  ePrecharge      = SescConf->getInt(section, "ePrecharge");
  eRead           = SescConf->getInt(section, "eRead");
  eWrite          = SescConf->getInt(section, "eWrite");  
  eIDD2P          = SescConf->getInt(section, "eIDD2P");
  eIDD2N          = SescConf->getInt(section, "eIDD2N"); 
  eIDD3N          = SescConf->getInt(section, "eIDD3N"); 
  eIDD3PF         = SescConf->getInt(section, "eIDD3PF");
  eIDD3PS         = SescConf->getInt(section, "eIDD3PS"); 
 
  //Read bank count
  numBanks        = SescConf->getInt(section, "numBanks");
    
  //Rank energy
  rankEnergy = new GStatsCntr("ddr_rank_energy");
  
  //Calculate log2 of bank count
  numBanksLog2 = log2i(numBanks);
  
  //Allocate banks
  banks = *(new std::vector<DDR2Bank *>(numBanks));
  for(int i=0; i<numBanks; i++){
    banks[i] = new DDR2Bank();
  } 
  
}

//Bank constructor
DDR2Bank::DDR2Bank()
  :state(IDLE),
   openRowID(-1),
   lastPrecharge(0),
   lastActivate(0),
   lastRead(0),
   lastWrite(0),
   casHit(false),
   numCasHits(0)
{
  //Nothing to do
}

//Precharge bank
void DDR2Bank::precharge()
{
  state = IDLE;
  lastPrecharge = DRAMClock;
  numCasHits = 0;
  casHit = false;
}

//Activate row
void DDR2Bank::activate(int rowID)
{
  state = ACTIVE;
  openRowID = rowID;
  lastActivate = DRAMClock;
  casHit = false;
  numCasHits = 0;
}

//Read from open row
void DDR2Bank::read()
{
  lastRead = DRAMClock;
  numCasHits++;
}

//Write to open row
void DDR2Bank::write()
{
  lastWrite = DRAMClock;
  numCasHits++;
}

//Can a precharge issue now to the given bank ?
bool DDR2Rank::canIssuePrecharge(int bankID)
{
  //Enforce tWR
  if(DRAMClock - (banks[bankID]->getLastWrite() + tWL + (BL/2)) < tWR){
    return false;
  }
  
  //Enforce tRTP
  if(DRAMClock - banks[bankID]->getLastRead() < tRTP){
    return false;
  }
  
  //Enforce tRAS
  if(DRAMClock - banks[bankID]->getLastActivate() < tRAS){
    return false;
  }

  return true;
}

//Can an activate issue now to the given bank ?
bool DDR2Rank::canIssueActivate(int bankID)
{
  //Enforce tRP
  if(DRAMClock - banks[bankID]->getLastPrecharge() < tRP){
    return false;
  }

  //Enforce tRRD
  for(int i=0; i<numBanks; i++){
    if(i!=bankID){
      if(DRAMClock - banks[i]->getLastActivate() < tRRD){
  return false;
      }
    }
  }

  //Enforce tRC
  if(DRAMClock - banks[bankID]->getLastActivate() < tRC){
    return false;
  } 
  
  return true;
}

//Can a read issue to the given bank and row ?
bool DDR2Rank::canIssueRead(int bankID, int rowID)
{
  //Enforce page misses
  if(banks[bankID]->getOpenRowID() != rowID){
    return false;
  }
  
  //Enforce tRCD
  if(DRAMClock - banks[bankID]->getLastActivate() < tRCD){
    return false;
  } 
  
  //Enforce tCCD
  if(DRAMClock - lastRead < tCCD || DRAMClock - lastWrite < tCCD){
    return false;
  }
  
  //Enforce tWTR
  if(DRAMClock - (lastWrite + tWL + (BL/2)) < tWTR){
    return false;
  }
  
  return true;
}

//Can a write issue to the given bank and row ?
bool DDR2Rank::canIssueWrite(int bankID, int rowID)
{
  //Enforce page misses
  if(banks[bankID]->getOpenRowID() != rowID){
    return false;
  }
  
  //Enforce tRCD
  if(DRAMClock - banks[bankID]->getLastActivate() < tRCD){
    return false;
  }
  
  //Enforce tCCD
  if(DRAMClock - lastRead < tCCD || DRAMClock - lastWrite < tCCD){
    return false;
  }

  return true;
}

//Precharge given bank
void DDR2Rank::precharge(int bankID) 
{
  //Timing
  lastPrecharge = DRAMClock;
  banks[bankID]->precharge();
  
  //Energy
  DRAMEnergy += ePrecharge;
  rankEnergy->add(ePrecharge);

} 

//Activate given row in given bank
void DDR2Rank::activate(int bankID, int rowID)
{

  //Timing 
  lastActivate = DRAMClock;
  banks[bankID]->activate(rowID);

  //Energy
  DRAMEnergy += eActivate;
  rankEnergy->add(eActivate);

}

//Read from given row in given bank
void DDR2Rank::read(int bankID, int rowID)
{

  //Timing
  lastRead = DRAMClock;
  banks[bankID]->setCasHit(true);
  banks[bankID]->read();

  //Energy
  DRAMEnergy += eRead;
  rankEnergy->add(eRead);

}

//Write to given row in given bank
void DDR2Rank::write(int bankID, int rowID)
{

  //Timing 
  lastWrite = DRAMClock;
  banks[bankID]->setCasHit(true);
  banks[bankID]->write();

  //Energy
  DRAMEnergy += eWrite;
  rankEnergy->add(eWrite);
}

//Get DDR2 rank's background energy
long DDR2Rank::getBackgroundEnergy()
{
  //Return energy for current power state
  switch(powerState)
    {

    case IDD2P:
      return eIDD2P;
      break;

    case IDD3PF:
      return eIDD3PF;
      break;

    case IDD3PS:
      return eIDD3PS;
      break;
      
    case IDD2N:
      return eIDD2N;
      break;
      
    case IDD3N:
      return eIDD3N;
      break;
      
    default:
      printf("ILLEGAL POWER STATE\n");
      fflush(stdout);
      exit(1);
      break;
    }
}

//Update DDR2 power state
void DDR2Rank::updatePowerState()
{
  
  //Flag indicating all banks are precharged
  bool allBanksPrecharged = true;
  for(int i=0; i<numBanks; i++){
    if(banks[i]->getState() == ACTIVE){
      allBanksPrecharged = false;
    }
  }

  //If CKE is low
  if(cke == false){
    //If all banks are precharged
    if(allBanksPrecharged){
      powerState = IDD2P;
    }
    //Otherwise
    else{
      //If using fast-exit mode
      if(emr == FAST){
  powerState = IDD3PF;
      }
      //Otherwise
      else{
  powerState = IDD3PS;
      }
    }
  }
  //Otherwise
  else{
    //If all banks are precharged
    if(allBanksPrecharged){
      powerState = IDD2N;
    }
    //Otherwise
    else{
      powerState = IDD3N;
    }
  }
}

//DDR2 Constructor
DDR2::DDR2(const char *section, int _channelID)
  : channelID(_channelID)
{
  //Check all parameters
  SescConf->isInt(section, "multiplier");
  SescConf->isInt(section, "numCores");
  SescConf->isInt(section, "numChannels");
  SescConf->isInt(section, "numRanks");
  SescConf->isInt(section, "numBanks");
  SescConf->isInt(section, "pageSize");
  SescConf->isInt(section, "queueSize");
  SescConf->isInt(section, "tRCD");
  SescConf->isInt(section, "tCL");
  SescConf->isInt(section, "tWL");
  SescConf->isInt(section, "tCCD");
  SescConf->isInt(section, "tWTR");
  SescConf->isInt(section, "tWR");
  SescConf->isInt(section, "tRTP");
  SescConf->isInt(section, "tRP");
  SescConf->isInt(section, "tRRD");
  SescConf->isInt(section, "tRAS");
  SescConf->isInt(section, "tRC");  
  SescConf->isInt(section, "tRTRS");
  SescConf->isInt(section, "tOST");
  SescConf->isInt(section, "BL");
  SescConf->isInt(section, "offPolicyCycles");
  SescConf->isInt(section, "epochLength");
  SescConf->isInt(section, "budget");
  
  //Read all parameters
  multiplier      = SescConf->getInt(section, "multiplier");
  numCores        = SescConf->getInt(section, "numCores");
  numChannels     = SescConf->getInt(section, "numChannels");
  numRanks        = SescConf->getInt(section, "numRanks");
  numBanks        = SescConf->getInt(section, "numBanks");
  pageSize        = SescConf->getInt(section, "pageSize");
  queueSize       = SescConf->getInt(section, "queueSize");
  tRCD            = SescConf->getInt(section, "tRCD");
  tCL             = SescConf->getInt(section, "tCL");
  tWL             = SescConf->getInt(section, "tWL");
  tCCD            = SescConf->getInt(section, "tCCD");
  tWTR            = SescConf->getInt(section, "tWTR");
  tWR             = SescConf->getInt(section, "tWR");
  tRTP            = SescConf->getInt(section, "tRTP");
  tRP             = SescConf->getInt(section, "tRP");
  tRRD            = SescConf->getInt(section, "tRRD");
  tRAS            = SescConf->getInt(section, "tRAS");
  tRC             = SescConf->getInt(section, "tRC");
  tRTRS           = SescConf->getInt(section, "tRTRS");
  tOST            = SescConf->getInt(section, "tOST");
  BL              = SescConf->getInt(section, "BL");
  epochLength     = SescConf->getInt(section, "epochLength");
  budget          = SescConf->getInt(section, "budget");
  
  //Calculate log2
  numChannelsLog2 = log2i(numChannels);
  numRanksLog2    = log2i(numRanks);
  numBanksLog2    = log2i(numBanks);
  pageSizeLog2    = log2i(pageSize);

  //Initialize DRAM clock
  DRAMClock = 0;
  
  //Initialize DRAM energy
  DRAMEnergy = 0;
  
  //Initialize scheduling queue occupancy
  occupancy=0;

  //Initialize arrival queue occupancy
  arrivals=0;

  //Initialize arrival queue
  arrivalHead = NULL;
  arrivalTail = NULL;

  //Allocate ranks
  ranks = *(new std::vector<DDR2Rank *>(numRanks));
  for(int i=0; i<numRanks; i++){
    ranks[i] = new DDR2Rank(section);
  } 

  //Allocate scheduling queue
  dramQ = *(new std::vector<MemRef *>(queueSize));
  for(int i=0; i<queueSize; i++){
    dramQ[i] = new MemRef();
  }

  //Allocate free-list
  freeList = new std::list<int>();
  for(int i=0; i<queueSize;i++){
    freeList->push_back(i);
  }

  //Allocate timing stats
  dramQAvgOccupancy    = new GStatsAvg("dramQ_avg_occupancy");
  arrivalQAvgOccupancy = new GStatsAvg("arrivalQ_avg_occupancy");
  completionRate       = new GStatsAvg("avg_completion_rate");
  numPrecharges        = new GStatsCntr("ddr_precharge_count");
  numActivates         = new GStatsCntr("ddr_activate_count");
  numReads             = new GStatsCntr("ddr_read_count");
  numWrites            = new GStatsCntr("ddr_write_count");
  numNops              = new GStatsCntr("ddr_nop_count");
  
  //Allocate energy stats
  channelEnergy        = new GStatsCntr("ddr_channel_energy");

  //Allocate dummy reference for closing open pages with no pending references
  dummyRef = new MemRef();

}

//A new memory request arrives in arrival queue
void DDR2::arrive(MemRequest *mreq)
{
  //If tail pointer exists
  if(arrivalTail != NULL){
    //Mark the tail's back pointer
    arrivalTail->back = mreq;
    //Mark the arrival's front pointer
    mreq->front = arrivalTail;
  }

  //Make new request the tail
  arrivalTail = mreq;

  //If head is NULL, make new request the head 
  if(arrivalHead == NULL){
    arrivalHead = mreq;
  }

  //Update arrival queue occupancy
  arrivals++;

}

//Enqueue memory request in DRAM queue
void DDR2::enqueue(MemRequest *mreq)
{

  //Update arrival queue
  arrivalHead = mreq->back;
  if(arrivalHead == NULL){
    arrivalTail = NULL;
  }

  occupancy++;
  arrivals--;

  //Get index into scheduling queue
  int index = freeList->back();
  freeList->pop_back();
  
  //Set the memory request
  dramQ[index]->setMReq(mreq);
  
  //Set the rank ID
  int rankID = (mreq->getAddr() >> (pageSizeLog2 + numChannelsLog2)) & (numRanks - 1);
  dramQ[index]->setRankID(rankID);

  //Set the bank ID
  int bankID = (mreq->getAddr() >> (pageSizeLog2 + numChannelsLog2 + numRanksLog2)) & (numBanks - 1);
  dramQ[index]->setBankID(bankID);

  //Set the row ID
  int rowID = mreq->getAddr() >> (pageSizeLog2 + numChannelsLog2 + numRanksLog2 + numBanksLog2);
  dramQ[index]->setRowID(rowID);

  //Get bank state and open row ID
  STATE bankState = getBankState(rankID, bankID);
  int openRowID = getOpenRowID(rankID, bankID);

  //Set precharge status
  dramQ[index]->setPrechargePending(bankState == ACTIVE && rowID != openRowID);

  //Set activate status
  dramQ[index]->setActivatePending(bankState == IDLE || rowID != openRowID);

  //Set read/write flag
  dramQ[index]->setRead(!mreq->isWriteback());

  //Set load miss flag 
  dramQ[index]->setLoadMiss(mreq->isLoadMiss());

  //Set load miss sequence number and coreID
  if(dramQ[index]->isLoadMiss()){
    dramQ[index]->setLoadMissSeqNum(mreq->sequenceNum);
    dramQ[index]->setLoadMissCoreID(mreq->coreID);
  }

  //Set cas status
  dramQ[index]->setReadPending(dramQ[index]->isRead());
  dramQ[index]->setWritePending(!dramQ[index]->isRead());
 
  //Set ready status
  dramQ[index]->setReady(false);

  //Set valid status
  dramQ[index]->setValid(true);

  //Set timestamp
  dramQ[index]->setTimeStamp(DRAMClock);

}

//Update queue state after a precharge or activate
void DDR2::updateQueueState()
{
  //Go through all references
  for(int i=0; i<queueSize; i++){
    
    //If the reference is valid
    if(dramQ[i]->isValid()){
      
      //Extract rank, bank and row information
      int bankID = dramQ[i]->getBankID();
      int rowID = dramQ[i]->getRowID();
      int rankID = dramQ[i]->getRankID();
      STATE bankState = getBankState(rankID, bankID);
      int openRowID = getOpenRowID(rankID, bankID);

      //Does the reference currently need a precharge ?
      dramQ[i]->setPrechargePending(bankState == ACTIVE && rowID != openRowID);
    
      //Does the reference currently need an activate ?
      dramQ[i]->setActivatePending(bankState == IDLE || rowID != openRowID);

    }

  }
}

//Can a precharge to given rank and bank issue now ?
bool DDR2::canIssuePrecharge(int rankID, int bankID)
{
  //Check intra-rank constraints
  return ranks[rankID]->canIssuePrecharge(bankID);
}

//Can an activate to given rank and bank issue now ?
bool DDR2::canIssueActivate(int rankID, int bankID)
{
  //Check intra-rank constraints
  return ranks[rankID]->canIssueActivate(bankID);
}

//Can a read to given rank, bank, and row issue now
bool DDR2::canIssueRead(int rankID, int bankID, int rowID)
{
  //Check intra-rank constraints
  if(!ranks[rankID]->canIssueRead(bankID, rowID)){  
    return false;
  }

  //Consecutive reads to different ranks
  if(lastReadRank != rankID){
    if(DRAMClock - (lastRead + (BL/2)) < tRTRS){
      
    } 
  }
  
  //Enforce tRTRS
  for(int i=0; i<numRanks; i++){
    //Go through all other ranks
    if(i!=rankID){
      
      //      printf("\nReference to rank %d\n",i);
      //      printf("Last read was at %lld\n",ranks[i]->getLastRead());
      //      printf("BL is %d\n",BL);
      //      printf("DRAMClock is %lld\n",DRAMClock);
      //      printf("tRTRS is %lld\n",tRTRS);
      //      fflush(stdout);
      
      //Consecutive reads
      if(DRAMClock - (ranks[i]->getLastRead() + (BL/2)) < tRTRS){    
  return false;
      }  
      //Write followed by read
      if(DRAMClock - (ranks[i]->getLastWrite() + tWL + (BL/2) - tCL) < tRTRS){
  return false;
      }  
    }
  }
  
  return true;
  
}

//Can a write to given rank, bank, and row issue now
bool DDR2::canIssueWrite(int rankID, int bankID, int rowID)
{
  //Check intra-rank constraints
  if(!ranks[rankID]->canIssueWrite(bankID, rowID)){
    return false;
  }

  //Enforce tRTRS
  for(int i=0; i<numRanks; i++){
    //Read followed by write, any two ranks
    if(DRAMClock - (lastRead + tCL + (BL/2) - tWL) < tRTRS){
      return false;
    }
    //Write followed by write, different ranks
    if(i!=rankID){
      //Enforce tOST
      if(DRAMClock - (lastWrite + (BL/2)) < tOST){
  return false;
      }
    }
  }
  
  return true;

}

//Precharge bank
void DDR2::precharge(int rankID, int bankID)
{
  //Issue precharge
  ranks[rankID]->precharge(bankID);
  lastPrecharge = DRAMClock;
}

//Activate row
void DDR2::activate(int rankID, int bankID, int rowID)
{ 
  //Issue activate
  ranks[rankID]->activate(bankID, rowID); 
  lastActivate = DRAMClock;
}
 
//Read
void DDR2::read(int rankID, int bankID, int rowID)
{
  //Issue read
  ranks[rankID]->read(bankID, rowID);
  lastRead = DRAMClock;
  lastReadRank = DRAMClock;
}
 
//Write
void DDR2::write(int rankID, int bankID, int rowID)
{
  //Issue write
  ranks[rankID]->write(bankID, rowID);
  lastWrite = DRAMClock;
  lastWriteRank = DRAMClock;
}
 
//Schedule a command using fcfs scheduling
void DDR2::scheduleFCFS()
{  
  //Reference to schedule commands for
  MemRef *mRef = NULL;
 
  //Arrival time of oldest reference
  Time_t oldestTime = DRAMClock;
 
  //Index of oldest reference in dramQ
  int index = -1;
 
  //Find the oldest reference in the scheduler's queue
  for(int i=0; i<queueSize; i++){
    if(dramQ[i]->isValid()){
      if(dramQ[i]->getTimeStamp() < oldestTime){
  mRef = dramQ[i];
  oldestTime = dramQ[i]->getTimeStamp();
  index = i;
      }
    }
  }

  //If an outstanding reference exists in the queue
  if(mRef != NULL){
    
    //If the reference needs a precharge
    if(mRef->needsPrecharge()){
      //If the precharge can be issued now
      if(canIssuePrecharge(mRef->getRankID(), mRef->getBankID())){
  //Precharge bank
  precharge(mRef->getRankID(), mRef->getBankID());
  mRef->setPrechargePending(false);
  //Update queue state
  updateQueueState();
  //Update stats
  numPrecharges->inc();
      }
      //Update stats
      completionRate->sample(0);
      return;
    }
    
    //If the reference needs an activate
    if(mRef->needsActivate()){
      //If the activate can be issued now
      if(canIssueActivate(mRef->getRankID(), mRef->getBankID())){
  //Activate bank
  activate(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
  mRef->setActivatePending(false);
  //Update queue state
  updateQueueState();
  //Upddate stats
  numActivates->inc();
      }
      //Update stats
      completionRate->sample(0);
      return;
    }

    //If the reference needs a read
    if(mRef->needsRead()){
      //If the read can be issued now
      if(canIssueRead(mRef->getRankID(), mRef->getBankID(), mRef->getRowID())){
  //Issue read
  read(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
  mRef->setReadPending(false);
  //This reference is now complete
  mRef->complete(globalClock + multiplier * (tCL + (BL/2) + 1) );
  freeList->push_back(index);
  occupancy--;
  //Update stats
  completionRate->sample(1);
  numReads->inc();
      }
      else{
  //Update stats
  completionRate->sample(0);
      }
      return;
    }

    //If the reference needs a write
    if(mRef->needsWrite()){
      //If the write can be issued now
      if(canIssueWrite(mRef->getRankID(), mRef->getBankID(), mRef->getRowID())){
  //Issue write
  write(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
  mRef->setWritePending(false);
  //This reference is now complete
  mRef->complete(globalClock + multiplier * (tWL + (BL/2) + 1) );
  freeList->push_back(index);
  occupancy--;
  //Update stats
  completionRate->sample(1);
  numWrites->inc();
      }
      else{
  //Update stats
  completionRate->sample(0);
      }
      return;
    } 
  }
  //Update stats
  completionRate->sample(0);
}

//Schedule a command using fr-fcfs scheduling
void DDR2::scheduleFRFCFS()
{  
  //Reference to schedule commands for
  MemRef *mRef = NULL;
  
  //Arrival time of oldest reference
  Time_t oldestTime = DRAMClock;
  
  //Index of oldest reference in dramQ
  int index = -1;

  //Find oldest ready CAS cmd
  for(int i=0; i<queueSize; i++){
    
    //If the request is valid
    if(dramQ[i]->isValid()){  

      //If the request does not need a precharge or activate
      if((!dramQ[i]->needsPrecharge()) && (!dramQ[i]->needsActivate())){
  //If the request needs a read
  if(dramQ[i]->needsRead()){
    //If the read can issue
    if(canIssueRead(dramQ[i]->getRankID(), dramQ[i]->getBankID(), dramQ[i]->getRowID())){
      //If the read is older than the oldest cas cmd found so far
      if(dramQ[i]->getTimeStamp() < oldestTime){
        mRef = dramQ[i];
        oldestTime = dramQ[i]->getTimeStamp();
        index = i;
      }
    }
  }
  //Otherwise, if the request needs a write
  else if(dramQ[i]->needsWrite()){
    //If the write can issue
    if(canIssueWrite(dramQ[i]->getRankID(), dramQ[i]->getBankID(), dramQ[i]->getRowID())){
      //If the write is older than the oldest cas cmd found so far
      if(dramQ[i]->getTimeStamp() < oldestTime){
        mRef = dramQ[i];
        oldestTime = dramQ[i]->getTimeStamp();
        index = i;
      }
    }
  }
  
      }
    }
  }

  //If no CAS cmd was ready, find oldest ready precharge/activate
  if(mRef == NULL){

    //Go through scheduling queue
    for(int i=0; i<queueSize; i++){
      
      //If the request is valid
      if(dramQ[i]->isValid()){
    
  //If the request needs a precharge
  if(dramQ[i]->needsPrecharge()){
  
    //If the precharge can issue
    if(canIssuePrecharge(dramQ[i]->getRankID(), dramQ[i]->getBankID())){
      //If the precharge is older than the oldest pre/act cmd found so far
      if(dramQ[i]->getTimeStamp() < oldestTime){
        mRef = dramQ[i];
        oldestTime = dramQ[i]->getTimeStamp();
        index = i;
      }
    }    
  }
  //Otherwise, if the request needs an activate
  else if(dramQ[i]->needsActivate()){
    
    //If the activate can issue
    if(canIssueActivate(dramQ[i]->getRankID(), dramQ[i]->getBankID())){ 
      
      //If the activate is older than the oldest pre/act cmd found so far
      if(dramQ[i]->getTimeStamp() < oldestTime){
        mRef = dramQ[i];
        oldestTime = dramQ[i]->getTimeStamp();
        index = i;
      }
    }

  }
      }

    }

  }

  //If an outstanding ready cmd exists in the queue
  if(mRef != NULL){

    //If the reference needs a precharge
    if(mRef->needsPrecharge()){
      //Precharge bank
      precharge(mRef->getRankID(), mRef->getBankID());
      mRef->setPrechargePending(false);
      //Update queue state
      updateQueueState();
      //Update stats
      completionRate->sample(0);
      numPrecharges->inc();
      return;
    }
    
    //If the reference needs an activate
    if(mRef->needsActivate()){
      //Activate row
      activate(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setActivatePending(false);
      
      //Update queue state
      updateQueueState();
      
      //Update stats
      completionRate->sample(0);
      numActivates->inc();
      return;
    }
    
    //If the reference needs a read
    if(mRef->needsRead()){
      //Issue read
      read(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setReadPending(false);
      //This reference is now complete
      mRef->complete(globalClock + multiplier * (tCL + (BL/2) + 1) );
      freeList->push_back(index);
      occupancy--;
      //Update stats
      completionRate->sample(1);
      numReads->inc();
      return;
    }
    
    //If the reference needs a write
    if(mRef->needsWrite()){
      //Issue write
      write(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setWritePending(false);
      //This reference is now complete
      mRef->complete(globalClock + multiplier * (tWL + (BL/2) + 1) );
      freeList->push_back(index);
      occupancy--;
      //Update stats
      completionRate->sample(1);
      numWrites->inc();
      return;
    } 
  }
    
  //Update stats
  completionRate->sample(0);

}
 

//Clock the DDR2 system
void DDR2::clock()
{

  //Power-related stall condition
  bool stall = false;

  //If one DRAM clock has passed since last call
  if(globalClock % multiplier == 0){

    //Channel 0 Updates DRAM clock 
    if(channelID == 0){
      DRAMClock++;
      DRAMEpoch++;
    }

    //Go through all ranks
    for(int i=0; i<numRanks; i++){
      //Each rank updates its power state
      ranks[i]->updatePowerState();
      
      //Each channel updates DRAM energy with its background power
      DRAMEnergy += ranks[i]->getBackgroundEnergy();
      channelEnergy->add(ranks[i]->getBackgroundEnergy());
    }
    
    //Check for stall condition
    if(DRAMEnergy >= budget){
      //If epoch not over yet
      if(DRAMEpoch <= epochLength){
  stall = true;
      }
      //Otherwise
      else{
  DRAMEnergy = 0;
  DRAMEpoch = 0;
  stall = false;  
      }
    } 
    else{
      stall = false;
    }
    
    //Update stats
    dramQAvgOccupancy->sample(occupancy);
    arrivalQAvgOccupancy->sample(arrivals);
    
    //Enqueue new arrivals
    while( (!freeList->empty()) && (arrivalHead != NULL)){
      enqueue(arrivalHead);
    }
       
      //Schedule only if not stalling
      if(!stall){
  //Schedule next DDR2 command
  scheduleFRFCFS();
  //scheduleFamily();
      }
    
  }
  
}


 
//Get DDR2 channel ID
int DDR2::getChannelID(MemRequest *mreq)
{
  return (mreq->getAddr() >> pageSizeLog2) & (numChannels - 1); 
}























































