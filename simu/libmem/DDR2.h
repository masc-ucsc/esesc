/*
SESC: Super ESCalar simulator
Copyright (C) 2003 University of Illinois.

Contributed by Engin Ipek
  
This file is part of SESC.
  
SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.
  
SESC is distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
  
You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
  
#ifndef DDR2_H
#define DDR2_H
 
#include "nanassert.h"
  
#include <list>
    
#include "GStats.h" 
#include "MemRequest.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "Port.h"
   
#include <vector>
 
/* DDR2 memory subsystem, indluding the memory controller */

//Bank state 
enum STATE {
  IDLE = 0,
  ACTIVE
};

//DDR2 Power state
enum POWER_STATE {
  IDD2P = 0,     //CKE low, all banks precharged
  IDD3PF,        //CKE low, at least one bank active, fast-exit mode 
  IDD3PS,        //CKE low, at least one bank active, slow-exit mode
  IDD2N,         //CKE high, all banks precharged
  IDD3N          //CKE high, at least one bank active
}; 

//Exit mode register
enum EMR {
  SLOW = 0,      //Slow-exit mode
  FAST           //Fast-exit mode
};

//Preference relationships for FR-FCFS family
enum PREFERENCE {
  LOAD_OVER_STORE = 0,
  OLDER_OVER_YOUNGER,
  CAS_OVER_RAS,
  ROB_ORDER,
  HISTORY_BASED
};

//Arbiters for history-based command scheduling
enum ARBITER {
  R1W1 = 0,
  R2W1 = 1,
  R1W2 = 2
};

//History for history-based command scheduling
enum HISTORY {
  RR = 0,
  RW = 1,
  WR = 2,
  WW = 3
};

//Memory reference waiting in the scheduler's queue
class MemRef {
  
  //Arrival time of this reference to the DRAM queue
  Time_t timeStamp;

  //MemRequest that made this reference
  MemRequest *mReq;

  //Rank ID
  int rankID;

  //Bank ID
  int bankID;
  
  //Row ID
  int rowID;

  //True if this reference currently needs a precharge 
  bool prechargePending;

  //True if this reference currently needs an activate 
  bool activatePending;

  //True if this refrence currently needs a read (cas) operation
  bool readPending;

  //True if this reference currently needs a write (cas) operation
  bool writePending;

  //True if this reference is ready 
  bool ready;
  
  //True if this reference is valid 
  bool valid;

  //True if this reference is a read
  bool read;
  
  //True if this reference is a load miss
  bool loadMiss;

  //Sequence number of load miss
  int loadMissSeqNum;
  
  //Core ID of load miss
  int loadMissCoreID;
  
 public:
  
  //Get the arrival time of this reference
  Time_t getTimeStamp() { return timeStamp; }
  
  //Get the memory request that made this reference
  MemRequest *getMReq() { return mReq; };
  
  //Get the rank ID
  int getRankID() { return rankID; }

  //Get the bank ID
  int getBankID() { return bankID; }

  //Get the row ID
  int getRowID() { return rowID; }

  //Does this refrence currently need a precharge ?
  bool needsPrecharge() { return prechargePending; }

  //Does this reference currently need an activate ?
  bool needsActivate() { return activatePending; }
  
  //Does this reference currently need a read operation ?
  bool needsRead() { return readPending; }

  //Does this reference currently needs a write operation ?
  bool needsWrite() { return writePending; }

  //Is this refrence ready ?
  bool isReady() { return ready; }
  
  //Is this reference valid ?
  bool isValid() { return valid; }
  
  //Is this reference a read ?
  bool isRead() { return read; }

  //Is this a load miss ?
  bool isLoadMiss() { return loadMiss; }

  //Get sequence number of load miss
  int getLoadMissSeqNum() { return loadMissSeqNum; }

  //Get core ID of load miss
  int getLoadMissCoreID() { return loadMissCoreID; }

  //Set the timestamp for this memory reference
  void setTimeStamp(Time_t _timeStamp) { timeStamp = _timeStamp; }

  //Set the memory request that made this reference
  void setMReq(MemRequest *_mReq) { mReq = _mReq; }

  //Set the rank ID
  void setRankID(int _rankID) { rankID = _rankID; }

  //Set the bank ID
  void setBankID(int _bankID) { bankID = _bankID; }

  //Set the row ID
  void setRowID(int _rowID) { rowID = _rowID; }
  
  //Set precharge status
  void setPrechargePending(bool _prechargePending) { prechargePending = _prechargePending; }

  //Set activate status
  void setActivatePending(bool _activatePending) { activatePending = _activatePending; }

  //Set read (cas) status
  void setReadPending(bool _readPending) { readPending = _readPending; }
  
  //Set write (cas) status
  void setWritePending(bool _writePending) { writePending = _writePending; }

  //Set ready status
  void setReady(bool _ready) { ready = _ready; }

  //Set valid status
  void setValid(bool _valid) { valid = _valid; }
  
  //Set read/write flag
  void setRead(bool _read) { read = _read; }

  //Set load miss flag
  void setLoadMiss(bool _loadMiss) {loadMiss = _loadMiss; }

  //Set load miss sequence number
  void setLoadMissSeqNum(int _loadMissSeqNum) { loadMissSeqNum = _loadMissSeqNum; }

  //Set load miss coreID
  void setLoadMissCoreID(int _loadMissCoreID) { loadMissCoreID = _loadMissCoreID; }

  //Complete this transaction
  void complete(Time_t when);

  //Constructor
  MemRef();

};

//A single DDR2 bank
class DDR2Bank {
  
  //Bank status
  STATE state;

  //Open rowID
  int openRowID;
  
  //Track command timing for each bank
  Time_t lastPrecharge;
  Time_t lastActivate;
  Time_t lastRead;
  Time_t lastWrite;
 
  //Has a cas cmd hit the open row since activation ?
  bool casHit;

  //Number of cas hits since the row activation
  int numCasHits; 

 public:
  
  //Get bank state
  STATE getState() { return state; }
  
  //Get open row ID
  int getOpenRowID() { return openRowID; }
  
  //Get the time of last activate
  Time_t getLastActivate() { return lastActivate; }
  
  //Get the time of last precharge
  Time_t getLastPrecharge() { return lastPrecharge; }
  
  //Get the time of last read
  Time_t getLastRead() { return lastRead; }
  
  //Get the time of last write
  Time_t getLastWrite() { return lastWrite; }
  
  //Get cas hit status
  bool getCasHit() { return casHit; }

  //Get cas hit count
  int getNumCasHits() { return numCasHits; }
  
  //Set bank state
  void setState(STATE _state) { state = _state; }

  //Set open row ID
  void setOpenRowID(int _openRowID) { openRowID = _openRowID; }

  //Set cas hit status
  void setCasHit(bool _casHit) { casHit = _casHit; }

  //Precharge bank
  void precharge();

  //Activate given row
  void activate(int rowID);
  
  //Read from open row
  void read();

  //Write to open row
  void write();

  //Constructor
  DDR2Bank();
  
};

//A single DDR2 rank
class DDR2Rank {
  
  //Power state
  POWER_STATE powerState;
  
  //Exit mode register
  EMR emr;
  
  //CKE
  bool cke;
  
  //Number of banks per rank
  int numBanks;
  
  //Log2 of bank count
  int numBanksLog2;
    
  //DRAM Banks
  std::vector<DDR2Bank *> banks;
  
  //Track command timing for each rank
  Time_t lastPrecharge;
  Time_t lastActivate;
  Time_t lastRead;
  Time_t lastWrite;
  
  //DDR2 timing parameters
  Time_t tRCD;   //Activate to read
  Time_t tCL;    //Read to data bus valid
  Time_t tWL;    //Write to data bus valid
  Time_t tCCD;   //CAS to CAS 
  Time_t tWTR;   //Write to read, referenced from 1st cycle after last dIn pair
  Time_t tWR;    //Internal write to precharge, referenced from 1st cycle after last dIn pair
  Time_t tRTP;   //Internal read to precharge
  Time_t tRP;    //Precharge to activate
  Time_t tRRD;   //Activate to activate (different banks)
  Time_t tRAS;   //Activate to precharge
  Time_t tRC;    //Activate to activate (same bank)
  Time_t tRTRS;  //Rank-to-rank switching time
  Time_t tOST;   //Output switching time
  int BL;        //Burst length
  
  //DDR2 energy parameters in pico-Joules
  long eIDD2P;      //IDD2P background energy (pJ)
  long eIDD2N;      //IDD2N background energy (pJ)
  long eIDD3N;      //IDD3N background energy (pJ)
  long eIDD3PF;     //IDD3P background energy for fast exit mode (pJ) 
  long eIDD3PS;     //IDD3P background energy for slow exit mode (pJ) 
  long eActivate;   //Activate energy (pJ)
  long ePrecharge;  //Precharge energy (pJ)
  long eRead;       //Read energy (pJ)
  long eWrite;      //Write energy (pJ)  

 public:
  
  //Get DDR2 background energy for current system state
  long getBackgroundEnergy();

  //Update DDR2 power state after issuing a cmd
  void updatePowerState();

  //Get rank power state
  POWER_STATE getPowerState() { return powerState; }
  
  //Get bank state
  STATE getBankState(int bankID) { return banks[bankID]->getState(); }
  //Get open row ID
  int getOpenRowID(int bankID) { return banks[bankID]->getOpenRowID(); }
   
  //Get the time of last activate
  Time_t getLastActivate() { return lastActivate; }
  //Get the time of last precharge
  Time_t getLastPrecharge() { return lastPrecharge; }
  //Get the time of last read
  Time_t getLastRead() { return lastRead; }
  //Get the time of last write
  Time_t getLastWrite() { return lastWrite; }
  
  //Set bank state
  void setPowerState(POWER_STATE _powerState) { powerState = _powerState; }

  //Precharge bank
  void precharge(int bankID);
  //Activate given row
  void activate(int bankID, int rowID);
  //Read from open row
  void read(int bankID, int rowID);
  //Write to open row
  void write(int bankID, int rowID);

  //Can a precharge to given bank issue now ?
  bool canIssuePrecharge(int bankID);
  //Can an activate to given bank issue now ?
  bool canIssueActivate(int bankID);
  //Can a read to given bank and row issue now ?
  bool canIssueRead(int bankID, int rowID);
  //Can a write to given bank and row issue now ?
  bool canIssueWrite(int bankID, int rowID);
  
  //Energy stats
  GStatsCntr *rankEnergy;

  //Rank costructor
  DDR2Rank(const char *section);
  
};

//DDR2 memory system
class DDR2 {
  
  //Epoch length for power-aware controller policies
  Time_t epochLength;
  
  //Energy budget (in pJ) for one interval
  long budget;
  
  //Pointer to next channel
  DDR2 *nextChannel;
  
  //DDR2 channel ID
  int channelID;

  //Clock multiplier
  int multiplier;

  //Number of cores
  int numCores;

  //Number of channels
  int numChannels;

  //Number of ranks
  int numRanks;

  //Number of banks
  int numBanks;

  //Page size
  int pageSize;
  
  //Log2 of page size, channel count, rank count, and bank count
  int numChannelsLog2;
  int numRanksLog2;
  int numBanksLog2;
  int pageSizeLog2;

  //DRAM scheduling queue size
  int queueSize;
  
  //Free-list for allocating scheduling queue entries
  std::list<int> *freeList; 

  //Scheduling queue
  std::vector<MemRef *> dramQ;

  //Priority level for each cmd in dramQ (FR-FCFS family)
  std::vector<int> priorities;
  
  //Arrival queue
  MemRequest *arrivalHead;
  MemRequest *arrivalTail;
  
  //DRAM Ranks
  std::vector<DDR2Rank *> ranks;
  
  //Track command timing across whole system
  Time_t lastPrecharge;
  Time_t lastActivate;
  Time_t lastRead;
  Time_t lastWrite;
  
  //Rank IDs of the least read and write commands issued
  int lastReadRank;
  int lastWriteRank;
  
  //DDR2 timing parameters
  Time_t tRCD;   //Activate to read
  Time_t tCL;    //Read to data bus valid
  Time_t tWL;    //Write to data bus valid
  Time_t tCCD;   //CAS to CAS 
  Time_t tWTR;   //Write to read, referenced from 1st cycle after last dIn pair
  Time_t tWR;    //Internal write to precharge, referenced from 1st cycle after last dIn pair
  Time_t tRTP;   //Internal read to precharge
  Time_t tRP;    //Precharge to activate
  Time_t tRRD;   //Activate to activate (different banks)
  Time_t tRAS;   //Activate to precharge
  Time_t tRC;    //Activate to activate (same bank)
  Time_t tRTRS;  //Rank-to-rank switching time
  Time_t tOST;   //Output switching time
  int BL;        //Burst length
    
  //DRAMQ Occupancy
  int occupancy;
  
  //Arrival queue occupancy
  int arrivals;
    
  //Dummy reference for closing open pages with no pending refs
  MemRef *dummyRef;
  
  //Features selected for tiling
  int *features;
  
  //Arbiter for history-based scheduling
  ARBITER arbiter;
  
  //Command history for history-based scheduling
  HISTORY history;
  
  //Read and write counts for history-based scheduling
  int readCount;
  int writeCount;
  
  //Time of last arbiter selection
  Time_t lastArbiterTime;

  //Energy stats
  GStatsCntr *channelEnergy;

public:
  
  //Get next DDR2 channel
  DDR2 *getNextChannel() { return nextChannel; }
  
  //Set next DDR2 channel
  void setNextChannel(DDR2 *_nextChannel) { nextChannel = _nextChannel; }
  
  //Get the number of channels
  int getNumChannels() { return numChannels; }

  //Get channel ID
  int getChannelID() { return channelID; }

  //Get the channel ID of given request
  int getChannelID(MemRequest *mReq);
  
  //Add a new incoming memory reference to arrival queue
  void arrive(MemRequest *mreq);
  
  //Enqueue a new reference in scheduler's queue
  void enqueue(MemRequest *mreq);
  
  //Get bank state
  STATE getBankState(int rankID, int bankID) { return ranks[rankID]->getBankState(bankID); }
  //Get open row ID
  int getOpenRowID(int rankID, int bankID) { return ranks[rankID]->getOpenRowID(bankID); }

  //Can a precharge to given rank and bank issue now ?
  bool canIssuePrecharge(int rankID, int bankID);
  //Can an activate to given rank and bank issue now ?
  bool canIssueActivate(int rankID, int bankID);
  //Can a read to given rank, bank and row issue now ?
  bool canIssueRead(int rankID, int bankID, int rowID);
  //Can a write to given rank, bank and row issue now ?
  bool canIssueWrite(int rankID, int bankID, int rowID);
 
  //Precharge bank 
  void precharge(int rankID, int bankID);
  //Activate row
  void activate(int rankID, int bankID, int rowID);
  //Read
  void read(int rankID, int bankID, int rowID);
  //Write
  void write(int rankID, int bankID, int rowID);

  //Update queue state after issuing a cmd 
  void updateQueueState();

  //FCFS scheduler
  void scheduleFCFS();

  //FR-FCFS scheduler
  void scheduleFRFCFS();

  //Wrapper scheduler for FR-FCFS family of policies
  void scheduleFamily();
  
  //Preference function for FR-FCFS family of policies
  void prioritize(PREFERENCE p);

  //Clock the DDR2 subsystem
  void clock();

  //Timing stats
  GStatsAvg  *dramQAvgOccupancy;
  GStatsAvg  *arrivalQAvgOccupancy;
  GStatsAvg  *memoryAccesTime;
  GStatsAvg  *completionRate;
  GStatsCntr *numPrecharges;
  GStatsCntr *numActivates;
  GStatsCntr *numReads;
  GStatsCntr *numWrites;
  GStatsCntr *numNops;

  //Constructor
  DDR2(const char *section, int channelID); 

};
 
#endif
