
//                Smruti Sarangi
//                Milos Prvulovic
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


#ifndef BPRED_H
#define BPRED_H

/*
 * The original Branch predictor code was inspired in simplescalar-3.0, but
 * heavily modified to make it more OOO. Now, it supports more branch predictors
 * that the standard simplescalar distribution.
 *
 * Supported branch predictors models:
 *
 * Oracle, NotTaken, Taken, 2bit, 2Level, 2BCgSkew
 *
 */

#include <iostream>

#include "nanassert.h"
#include "estl.h"

#include "GStats.h"
#include "DInst.h"
#include "CacheCore.h"
#include "SCTable.h"
#include "DOLC.h"

#define RAP_T_NT_ONLY   1

enum PredType {
  CorrectPrediction = 0,
  NoPrediction,
  NoBTBPrediction,
  MissPrediction
};

class MemObj;

class BPred {
public:
  typedef int64_t HistoryType;
  class Hash4HistoryType {
  public: 
    size_t operator()(const HistoryType &addr) const {
      return ((addr) ^ (addr>>16));
    }
  };

  typedef boost::dynamic_bitset<> LongHistoryType; 

protected:
  const int32_t id;

  GStatsCntr nHit;  // N.B. predictors should not update these counters directly
  GStatsCntr nMiss; // in their predict() function.

  int32_t addrShift;

  HistoryType calcHist(AddrType pc) const {
    HistoryType cid = pc>>2; // psudo-PC works, no need lower 2 bit

    // Remove used bits (restrict predictions per cycle)
    cid = cid>>addrShift;
    // randomize it
    cid = (cid >> 17) ^ (cid); 

    return cid;
  }
protected:
public:
  BPred(int32_t i, const char *section, const char *sname, const char *name);
  virtual ~BPred();

  virtual PredType predict(DInst *dinst, bool doUpdate, bool doStats) = 0;
  virtual void fetchBoundaryBegin(DInst *dinst); // If the branch predictor support fetch boundary model, do it
  virtual void fetchBoundaryEnd(); // If the branch predictor support fetch boundary model, do it

  PredType doPredict(DInst *dinst, bool doUpdate, bool doStats=true) {
    PredType pred = predict(dinst, doUpdate, doStats);
    if (!doUpdate || pred == NoPrediction)
      return pred;

    if (dinst->getInst()->isJump())
      return pred;

    nHit.inc(pred == CorrectPrediction && dinst->getStatsFlag() && doStats);
    nMiss.inc(pred == MissPrediction && dinst->getStatsFlag() && doStats);

    return pred;
  }

  void update(DInst *dinst) {
    predict(dinst,true, false);
  }
  
};


class BPRas : public BPred {
private:
  const uint16_t RasSize;
  const uint16_t rasPrefetch;

  AddrType *stack;
  int32_t index;
protected:
public:
  BPRas(int32_t i, const char *section, const char *sname);
  ~BPRas();
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

  void tryPrefetch(MemObj *il1, bool doStats);
};

class BPBTB : public BPred {
private:

  GStatsCntr nHitLabel;  // hits to the icache label (ibtb)
  DOLC *dolc;
  bool btbicache;
  uint64_t btbHistorySize;

  class BTBState : public StateGeneric<AddrType> {
  public:
    BTBState(int32_t lineSize) {
      inst = 0;
    }

    AddrType inst;

    bool operator==(BTBState s) const {
      return inst == s.inst;
    }
  };

  typedef CacheGeneric<BTBState, AddrType> BTBCache;
  
  BTBCache *data;
  
protected:
public:
  BPBTB(int32_t i, const char *section, const char *sname, const char *name=0);
  ~BPBTB();

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);
  void updateOnly(DInst *dinst);

};

class BPOracle : public BPred {
private:
  BPBTB btb;

protected:
public:
  BPOracle(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "Oracle")
    ,btb(i, section, sname) {
  }

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPNotTaken : public BPred {
private:
  BPBTB btb;

protected:
public:
  BPNotTaken(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "NotTaken") 
    ,btb(  i, section, sname) {
    // Done
  }

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPMiss : public BPred {
private:
protected:
public:
  BPMiss(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "Miss") {
    // Done
  }

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);
};

class BPNotTakenEnhanced : public BPred {
private:
  BPBTB btb;

protected:
public:
  BPNotTakenEnhanced(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "NotTakenEnhanced")
    ,btb(  i, section, sname) {
    // Done
  }

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPTaken : public BPred {
private:
  BPBTB btb;

protected:
public:
  BPTaken(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "Taken") 
    ,btb(  i, section, sname) {
    // Done
  }

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BP2bit:public BPred {
private:
  BPBTB btb;

  SCTable table;
protected:
public:
  BP2bit(int32_t i, const char *section, const char *sname);

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class IMLIBest;

class BPIMLI:public BPred {
private:
  BPBTB btb;

  IMLIBest *imli;

  const bool FetchPredict;

protected:
public:
  BPIMLI(int32_t i, const char *section, const char *sname);

  void fetchBoundaryBegin(DInst *dinst);
  void fetchBoundaryEnd();
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);
};

class BP2level:public BPred {
private:
  BPBTB btb;

  const uint16_t l1Size;
  const uint32_t  l1SizeMask;

  const uint16_t historySize;
  const HistoryType historyMask;

  SCTable globalTable;

  DOLC dolc;
  HistoryType *historyTable; // LHR
  bool useDolc;

protected:
public:
  BP2level(int32_t i, const char *section, const char *sname);
  ~BP2level();

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPHybrid:public BPred {
private:
  BPBTB btb;

  const uint16_t historySize;
  const HistoryType historyMask;

  SCTable globalTable;

  HistoryType ghr; // Global History Register

  SCTable localTable;
  SCTable metaTable;

protected:
public:
  BPHybrid(int32_t i, const char *section, const char *sname);
  ~BPHybrid();

  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BP2BcgSkew : public BPred {
private:
  BPBTB btb;

  SCTable BIM;

  SCTable G0;
  const uint16_t       G0HistorySize;
  const HistoryType  G0HistoryMask;

  SCTable G1;
  const uint16_t       G1HistorySize;
  const HistoryType  G1HistoryMask;

  SCTable metaTable;
  const uint16_t       MetaHistorySize;
  const HistoryType  MetaHistoryMask;

  HistoryType history;
protected:
public:
  BP2BcgSkew(int32_t i, const char *section, const char *sname);
  ~BP2BcgSkew();
  
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPyags : public BPred {
private:
  BPBTB btb;

  const uint16_t historySize;
  const HistoryType historyMask;

  SCTable table;
  SCTable ctableTaken;
  SCTable ctableNotTaken;

  HistoryType ghr; // Global History Register

  uint8_t    *CacheTaken;
  HistoryType CacheTakenMask;
  HistoryType CacheTakenTagMask;

  uint8_t    *CacheNotTaken;
  HistoryType CacheNotTakenMask;
  HistoryType CacheNotTakenTagMask;

protected:
public:
  BPyags(int32_t i, const char *section, const char *sname);
  ~BPyags();
  
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

class BPOgehl : public BPred {
private:
  BPBTB btb;

  const int32_t mtables;
  const int32_t glength;
  const int32_t nentry;
  const int32_t addwidth;
  int32_t logpred;

  int32_t THETA;
  int32_t MAXTHETA;
  int32_t THETAUP;
  int32_t PREDUP;

  int64_t *ghist;
  int32_t *histLength;
  int32_t *usedHistLength;
  
  int32_t *T;
  int32_t AC;  
  uint8_t miniTag;
  uint8_t *MINITAG;

  uint8_t genMiniTag(const DInst *dinst) const {
    AddrType t = dinst->getPC()>>2;
    return (uint8_t)((t ^ (t>>3)) & 3);
  }
  
  char **pred;
  int32_t TC;
protected:
  int32_t geoidx(uint64_t Add, int64_t *histo, int32_t m, int32_t funct);
public:
  BPOgehl(int32_t i, const char *section, const char *sname);
  ~BPOgehl();
  
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);
};

class LoopPredictor {
private:
  struct LoopEntry {
    uint64_t tag;
    uint32_t iterCounter;
    uint32_t currCounter;
    int32_t confidence;
    uint8_t  dir;

    LoopEntry () {
      tag         = 0;
      confidence  = 0;
      iterCounter = 0;
      currCounter = 0;
    }
  };

  const uint32_t nentries;
  LoopEntry *table; // TODO: add assoc, now just BIG

public:
  LoopPredictor(int size);
  void update(uint64_t key, uint64_t tag, bool taken);

  bool isLoop(uint64_t key, uint64_t tag) const;
  bool isTaken(uint64_t key, uint64_t tag, bool taken);
  uint32_t getLoopIter(uint64_t key, uint64_t tag) const;
};

class BPDGP : public BPred {
private:
  BPBTB btb;

  DOLC dolc;
  DOLC **ldolc;
  LoopPredictor lp;
  uint32_t lp_last_iter; // Based on LoopPredictor
  uint32_t loop_counter; // Based on backward jumps
  AddrType loop_end_pc; 

  SCTable ahead_local;
  SCTable ahead_global;
  SCTable ahead_meta;

  const int32_t ntables;
  const int32_t nlocal;
  const int32_t glength;
  int32_t alength;
  const int32_t nentry;
  const int32_t addwidth;

  int32_t  *TableSizeBits;
  uint32_t *TableSizeMask;

  int32_t CorrSize;

  int32_t MaxVal;
  int32_t TableValBits;
  int32_t TableTagBits;
  uint32_t TableTagMask;

  int64_t *ahist;  // All bit history
  int32_t *histLength;
  
  struct PredEntry {
    int32_t  val;
    uint32_t tag;
  };

  PredEntry **table;

  PredEntry *reverse;

  struct CorrEntry {
    int32_t *val;
  };

  CorrEntry *corr;

protected:
  int32_t geoidx(uint64_t Add, int64_t *histo, int32_t indexSize, int32_t m, int32_t funct, int tableid);
  uint64_t goodHash(uint64_t key) const;
public:
  BPDGP(int32_t i, const char *section, const char *sname);
  ~BPDGP();
  
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);

};

#if 1
class BPSOgehl : public BPred {
private:
  BPBTB btb;
  
  const int32_t mtables;
  const int32_t glength;
  const int32_t AddWidth;
  int32_t logtsize;

  int32_t THETA;
  int32_t MAXTHETA;
  int32_t THETAUP;
  int32_t PREDUP;

  LongHistoryType ghr;
  int32_t *histLength;
  int32_t *usedHistLength;
  
  int32_t *T;
  int32_t AC;  
  uint8_t miniTag;
  uint8_t *MINITAG;

  uint8_t genMiniTag(const DInst *dinst) const {
    AddrType t = dinst->getPC()>>2;
    return (uint8_t)((t ^ (t>>3)) & 3);
  }
  
  char **pred;
  int32_t TC;
protected:
  uint32_t geoidx2(uint64_t Add, int32_t m);
public:
  BPSOgehl(int32_t i, const char *section, const char *sname);
  ~BPSOgehl();
  
  PredType predict(DInst *dinst, bool doUpdate, bool doStats);
};
#endif




class BPTage : public BPred {
  private:
    BPBTB btb;
    HistoryType ghr;

    const int32_t numberOfTaggedPredictors;   
    const int32_t numberOfBimodalEntries;     
    const int32_t taggedTableEntries;     
    const int32_t maxHistLength;              
    const int32_t minHistLength;              
    const int32_t glength;		      
    const int32_t ctrCounterWidth;	      
    unsigned hystBits;				
    unsigned *tagWidth;
    int32_t *histLength;			
    int32_t logpred;           
    int32_t *logOfNumberOfEntriesInTaggedTables; 
    char **tageIndexSupport;			
    int32_t *bimodalTableSize;			
    int32_t *taggedTableSize;			
    uint64_t *ghist;  				

    unsigned hashIndexForTaggedTable (DInst *dinst, unsigned bankID); 
    unsigned F (unsigned pathHistory, unsigned numTables, unsigned currentTagBankID);   
    unsigned tagCalculation (DInst *dinst, unsigned bankID); 
    bool getBimodalPrediction (DInst *dinst, bool saturated);
    void bimodalUpdate (DInst *dinst, bool taken);  
    void updatePredictorDirection (DInst *dinst, bool taken, void *bph);
    inline unsigned bimodalIndexCalculation(AddrType branchAddr)  
    {
      return ((branchAddr >> instShiftAmount) & bimodalIdxMask);  
    }

    void updateGlobalHistory (DInst *dinst, bool taken, void * &bph, bool save); 

 
  public:
    unsigned instShiftAmount;   
				
				
    unsigned baseHystShift;     
    unsigned BPHistoryCount;    
    unsigned historyID;
    unsigned pathHistory;	
    unsigned sinceBimodalMisprediction;
    unsigned randomSeed;
    unsigned *globalHistory, *retireGlobalHistory;
    unsigned BPHistory_count;	
    int useAltOnNA;		
    int shiftAmount;
    unsigned counterResetControl, counterLog; 
    
    AddrType bimodalIdxMask, *taggedTableIdxMask, *taggedTableTagMask; 
    BPTage(int32_t i, const char *section, const char *sname);
    ~BPTage();
  
    inline unsigned randomGenerator()
    {
      ++randomSeed;
      return randomSeed & 3;
    }
    PredType predict(DInst *dinst, bool doUpdate, bool doStats);

    bool lookup (DInst *dinst, void * &bph);                   
    void unconditionalBranch (DInst *dinst, void * &bph);      
    void recoverFromMisprediction (DInst *dinst, bool taken, void *bph); 
    void updateBranchPredictor (DInst *dinst, bool taken, void *bph, bool squash);
    void fixGlobalHistory (DInst *dinst, void *bph, bool actTaken);      
    void squashRecovery (void *bph);			
    unsigned *hyst, *pred;  		
    unsigned **tageTableTagEntry;	
    unsigned **tageTableUsefulCounter;  
    int32_t **ctrCounter, ctrMaxValue, ctrMinValue;	
    bool **hConf;		
    int32_t *numberOfEntriesInTaggedComponent;		
      
    struct historyOverhead		
    {
      unsigned direction, id;		
      uint64_t sequenceNumber; 		

    };

    struct BPHistory			
    {

      BPHistory (const bool init) 
      : highConfidence(false)
      ,mediumConfidence(false) 
      ,lowConfidence(false)
      ,actuallyTaken(false)
      ,bimodalHighConfidence(false)
      ,bimodalMediumConfidence(false)
      ,bimodalLowConfidence(false)
      ,sTag(false)
      ,nsTag(false)
      ,wTag(false)
      ,nwTag(false)
      ,unconditional(false)
      ,usedBimodal(false)
      ,usedTagged(false)
      ,usedStandard(false)
      ,usedAlt(false)
      {

      }

      unsigned pathHistoryBackup;	
      unsigned historyID;		
      historyOverhead* directionPtr;	
      unsigned *indexStore, *tagStore;  
      AddrType *ch_i_comp;
      AddrType *ch_t0_comp;
      AddrType *ch_t1_comp;
      bool highConfidence, mediumConfidence, lowConfidence;
      bool predictTaken, altTaken;	
      bool tagePrediction, hitBank, altBank;		
      bool actuallyTaken;
      bool bimodalHighConfidence, bimodalMediumConfidence, bimodalLowConfidence;
      bool sTag, nsTag, wTag, nwTag;
      bool unconditional;				
      bool usedBimodal, usedTagged, usedStandard, usedAlt;

    };


    class tageGlobalEntry {
      public:
	int32_t counter, maxValue, minValue;
	unsigned tag, u;
	bool hi;

	tageGlobalEntry (unsigned counterWidth)         
	{
	  counter = 0;
	  maxValue = (1 << counterWidth) - 1;
	  minValue = 0;
	  tag = 0;
	  u = 0;
	  hi = false;
	}

	inline void counterUpdate (bool taken, unsigned counterWidth)
        {
	  maxValue = (1 << counterWidth) - 1;
	  minValue = 0;
	  
  	  if (taken && (counter < maxValue))
	    ++counter;
	  else if (!taken && (counter > 0))
	    --counter;
        }


    };

    std::deque<historyOverhead> globHist;
    std::deque<historyOverhead> retire_globHist;  

};


class BPredictor {
private:
  const int32_t id;
  const bool SMTcopy;
  MemObj  *il1; // For prefetch

  BPRas *ras;
  BPred *pred1;
  BPred *pred2;
  
  int32_t BTACDelay;
  int32_t FetchWidth;
  int32_t bpredDelay;

  GStatsCntr nBTAC;

  GStatsCntr nBranches;
  GStatsCntr nTaken;
  GStatsCntr nMiss;           // hits == nBranches - nMiss

  GStatsCntr nBranches2;
  GStatsCntr nTaken2;
  GStatsCntr nMiss2;           // hits == nBranches - nMiss

protected:
  PredType predict1(DInst *dinst, bool doUpdate);
  PredType predict2(DInst *dinst, bool doUpdate);

public:
  BPredictor(int32_t i, MemObj *il1, BPredictor *bpred=0);
  ~BPredictor();

  static BPred *getBPred(int32_t id, const char *sname, const char *sec);

  void fetchBoundaryBegin(DInst *dinst);
  void fetchBoundaryEnd();
  TimeDelta_t predict(DInst *dinst, bool *fastfix);

  void dump(const char *str) const;
};

#endif   // BPRED_H
