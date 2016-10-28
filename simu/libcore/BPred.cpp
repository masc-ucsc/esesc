// Contributed by Jose Renau
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

#include <string.h>
#include <strings.h>
#include <math.h>
#include <alloca.h>
#include <stdint.h>

#include "Report.h"
#include "BPred.h"
#include "MemObj.h"
#include "SescConf.h"
#include "IMLIBest.h"

#define CLOSE_TARGET_OPTIMIZATION 1

/*****************************************
 * BPred
 */

BPred::BPred(int32_t i, const char *sec, const char *sname, const char *name)
  :id(i)
  ,nHit("P(%d)_BPred%s_%s:nHit",i,sname, name)
  ,nMiss("P(%d)_BPred%s_%s:nMiss",i,sname, name)
{
  if (SescConf->checkInt(sec, "addrShift")) {
    SescConf->isBetween(sec, "addrShift", 0, 8);
    addrShift = SescConf->getInt(sec, "addrShift");
  }else{
    addrShift = 0;
  }
}

BPred::~BPred() {
}

void BPred::fetchBoundaryBegin(DInst *dinst) {
  // No fetch boundary implemented (must be specialized per predictor if supported)

}

void BPred::fetchBoundaryEnd() {
}

/*****************************************
 * RAS
 */
BPRas::BPRas(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname, "RAS")
   ,RasSize(SescConf->getInt(section,"rasSize"))
   ,rasPrefetch(SescConf->getInt(section,"rasPrefetch"))
{
  // Constraints
  SescConf->isInt(section, "rasSize");
  SescConf->isBetween(section, "rasSize", 0, 128);    // More than 128???

  if(RasSize == 0) {
    stack = 0;
    return;
  }

  stack = new AddrType[RasSize];
  I(stack);

  index = 0;
}

BPRas::~BPRas()
{
  delete stack;
}

void BPRas::tryPrefetch(MemObj *il1, bool doStats) {

  if (rasPrefetch == 0)
    return;

  for(int j=0;j<rasPrefetch;j++) {
    int i = index-j-1;
    while (i<0)
      i = RasSize - 1;

    il1->tryPrefetch(stack[i], doStats);
  }
}

PredType BPRas::predict(DInst *dinst, bool doUpdate, bool doStats) {

  // RAS is a little bit different than other predictors because it can update
  // the state without knowing the oracleNextPC. All the other predictors update the
  // statistics when the branch is resolved. RAS automatically updates the
  // tables when predict is called. The update only actualizes the statistics.

  if(dinst->getInst()->isFuncRet()) {

    if(stack == 0)
      return CorrectPrediction;

    if (doUpdate) {
      index--;
      if( index < 0 )
        index = RasSize-1;
    }

    if( stack[index] == dinst->getAddr() || (stack[index]+4) == dinst->getAddr() ) {

      //MSG("RET  %llx -> %llx  (stack=%llx) good",dinst->getPC(),dinst->getAddr(), stack[index]);
      return CorrectPrediction;
    }
    //MSG("RET  %llx -> %llx  (stack=%llx) miss",dinst->getPC(),dinst->getAddr(), stack[index]);

    return MissPrediction;
  } else if(dinst->getInst()->isFuncCall() && stack) {

    //MSG("CALL %llx -> %llx  (stack=%llx)",dinst->getPC(),dinst->getAddr(), stack[index]);

    if (doUpdate) {
      stack[index] = dinst->getPC()+4; 
      index++;

      if( index >= RasSize )
        index = 0;
    }
  }

  return NoPrediction;
}

/*****************************************
 * BTB
 */
BPBTB::BPBTB(int32_t i, const char *section, const char *sname, const char *name)
  :BPred(i, section, sname, name ? name : "BTB")
  ,nHitLabel("P(%d)_BPred%s_%s:nHitLabel",i,sname, name?name:"BTB")
{
  if (SescConf->checkInt(section,"btbHistorySize"))
    btbHistorySize=SescConf->getInt(section,"btbHistorySize");
  else
    btbHistorySize=0;

  if (btbHistorySize)
    dolc = new DOLC(btbHistorySize+1,5,2,2);
  else
    dolc = 0;

  if (SescConf->checkBool(section,"btbicache"))
    btbicache = SescConf->getBool(section,"btbicache");
  else
    btbicache = false;

  if( SescConf->getInt(section,"btbSize") == 0 ) {
    // Oracle
    data = 0;
    return;
  }

  data = BTBCache::create(section,"btb","P(%d)_BPred%s_BTB:",i, sname);
  I(data);
}

BPBTB::~BPBTB()
{
  if( data )
    data->destroy();
}

void BPBTB::updateOnly(DInst *dinst)
{
  if( data == 0 || !dinst->isTaken() )
    return;

  uint32_t key   = calcHist(dinst->getPC());

  BTBCache::CacheLine *cl = data->fillLine(key);
  I( cl );
  
  cl->inst = dinst->getAddr();
}

PredType BPBTB::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  bool ntaken = !dinst->isTaken();

  if (data == 0) {
    // required when BPOracle
    if (dinst->getInst()->doesCtrl2Label()) 
      nHitLabel.inc(doUpdate && dinst->getStatsFlag() && doStats);
    else
      nHit.inc(doUpdate && dinst->getStatsFlag() && doStats);

    if (ntaken) {
      // Trash result because it shouldn't have reach BTB. Otherwise, the
      // worse the predictor, the better the results.
      return NoBTBPrediction;
    }
    return CorrectPrediction;
  }

  uint32_t key = calcHist(dinst->getPC());

  if (dolc) {
    if (doUpdate)
      dolc->update(dinst->getPC());

    key ^= dolc->getSign(btbHistorySize,btbHistorySize);
  }

  if ( ntaken || !doUpdate ) {
    // The branch is not taken. Do not update the cache
    if (dinst->getInst()->doesCtrl2Label())  {
      nHitLabel.inc(doStats && doUpdate && dinst->getStatsFlag());
      return NoBTBPrediction;
    }

    BTBCache::CacheLine *cl = data->readLine(key);

    if( cl == 0 ) {
      nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
      return NoBTBPrediction; // NoBTBPrediction because BTAC would hide the prediction
    }

    if( cl->inst == dinst->getAddr() ) {
      nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
      return CorrectPrediction;
    }

    nMiss.inc(doStats && doUpdate && dinst->getStatsFlag());
    return MissPrediction;
  }

  I(doUpdate);

  // The branch is taken. Update the cache

  if (btbicache) {
    // i-cache results visible, possible to fix all the branch and jumps to label BTB misses
    if (dinst->getInst()->doesCtrl2Label())  {
      nHitLabel.inc(doStats && doUpdate && dinst->getStatsFlag());
      return CorrectPrediction;
    }
  }

  BTBCache::CacheLine *cl = data->fillLine(key);
  I( cl );
  
  AddrType predictID = cl->inst;
  cl->inst = dinst->getAddr();

  if( predictID == dinst->getAddr() ) {
    nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
    //MSG("hit :%llx -> %llx",dinst->getPC(), dinst->getAddr());
    return CorrectPrediction;
  }

  //MSG("miss:%llx -> %llx (%llx)",dinst->getPC(), dinst->getAddr(), predictID);
  nMiss.inc(doStats && doUpdate && dinst->getStatsFlag());
  return NoBTBPrediction;
}

/*****************************************
 * BPOracle
 */

PredType BPOracle::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if( !dinst->isTaken() )
    return CorrectPrediction; //NT
  
  return btb.predict(dinst, doUpdate, doStats);
}

/*****************************************
 * BPTaken
 */

PredType BPTaken::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if( dinst->getInst()->isJump() || dinst->isTaken())
    return btb.predict(dinst, doUpdate, doStats);

  PredType p = btb.predict(dinst,false, doStats);
 
 if (p == CorrectPrediction)
    return CorrectPrediction; // NotTaken and BTB empty

  return MissPrediction;
}

/*****************************************
 * BPNotTaken
 */

PredType  BPNotTaken::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  return dinst->isTaken() ? MissPrediction : CorrectPrediction;
}

/*****************************************
 * BPMiss
 */

PredType  BPMiss::predict(DInst *dinst, bool doUpdate, bool doStats) {
  return MissPrediction;
}

/*****************************************
 * BPNotTakenEnhaced
 */

PredType  BPNotTakenEnhanced::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats); // backward branches predicted as taken (loops)

  if (dinst->isTaken()) {
    AddrType dest = dinst->getAddr();
    if (dest < dinst->getPC())
      return btb.predict(dinst, doUpdate, doStats); // backward branches predicted as taken (loops)

    return MissPrediction; // Forward branches predicted as not-taken, but it was taken 
  }

  return CorrectPrediction;
}

/*****************************************
 * BP2bit
 */

BP2bit::BP2bit(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname, "2bit")
  ,btb(  i, section, sname)
  ,table(section
         ,SescConf->getInt(section,"size")
         ,SescConf->getInt(section,"bits"))
{
  // Constraints
  SescConf->isInt(section, "size");
  SescConf->isPower2(section, "size");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "bits", 1, 7);

  // Done
}

PredType BP2bit::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  bool taken = dinst->isTaken();

  bool ptaken;
  if (doUpdate)
    ptaken = table.predict(calcHist(dinst->getPC()), taken);
  else
    ptaken = table.predict(calcHist(dinst->getPC()));

  if( taken != ptaken ) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPIMLI: SC-TAGE-L with IMLI from Seznec Micro paper
 */

BPIMLI::BPIMLI(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname, "imli")
  ,btb(  i, section, sname)
  ,FetchPredict(SescConf->getBool(section,"FetchPredict"))
{
  imli = new IMLIBest;
}

void BPIMLI::fetchBoundaryBegin(DInst *dinst) {
  if (FetchPredict)
    imli->fetchBoundaryBegin(dinst->getPC());
}

void BPIMLI::fetchBoundaryEnd() {
  if (FetchPredict)
    imli->fetchBoundaryEnd();
}

PredType BPIMLI::predict(DInst *dinst, bool doUpdate, bool doStats) {

  if( dinst->getInst()->isJump() ) {
    imli->TrackOtherInst(dinst->getPC(), dinst->getInst()->getOpcode(), dinst->getAddr());
    dinst->setBiasBranch(true);
    return btb.predict(dinst, doUpdate, doStats);
  }

  bool taken = dinst->isTaken();

  if (!FetchPredict)
    imli->fetchBoundaryBegin(dinst->getPC());

  bool bias;
  bool ptaken = imli->getPrediction(dinst->getPC(), bias);
  dinst->setBiasBranch(bias);

  if (doUpdate) {
    imli->updatePredictor(dinst->getPC(), taken, ptaken, dinst->getAddr());
  }

  if (!FetchPredict)
    imli->fetchBoundaryEnd();

  if( taken != ptaken ) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BP2level
 */

BP2level::BP2level(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname,"2level")
   ,btb( i, section, sname)
   ,l1Size(SescConf->getInt(section,"l1Size"))
   ,l1SizeMask(l1Size - 1)
   ,historySize(SescConf->getInt(section,"historySize"))
   ,historyMask((1 << historySize) - 1)
   ,globalTable(section
                ,SescConf->getInt(section,"l2Size")
                ,SescConf->getInt(section,"l2Bits"))
   ,dolc(SescConf->getInt(section,"historySize"),5,2,2)
{
  // Constraints
  SescConf->isInt(section, "l1Size");
  SescConf->isPower2(section, "l1Size");
  SescConf->isBetween(section, "l1Size", 0, 32768);

  SescConf->isInt(section, "historySize");
  SescConf->isBetween(section, "historySize", 1, 63);

  SescConf->isInt(section, "l2Size");
  SescConf->isPower2(section, "l2Size");
  SescConf->isBetween(section, "l2Bits", 1, 7);

  if (SescConf->checkBool(section,"useDolc")) {
    useDolc = SescConf->checkBool(section,"useDolc");
  }else{
    useDolc = false;
  }

  I((l1Size & (l1Size - 1)) == 0); 

  historyTable = new HistoryType[l1Size];
  I(historyTable);
}

BP2level::~BP2level()
{
  delete historyTable;
}

PredType BP2level::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() ) {
    if (useDolc)
      dolc.update(dinst->getPC());
    return btb.predict(dinst, doUpdate, doStats);
  }

  bool taken = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  uint16_t    l1Index = iID & l1SizeMask;
  HistoryType l2Index = historyTable[l1Index];

  if (useDolc)
    dolc.update(dinst->getPC());

  // update historyTable statistics
  if (doUpdate) {
    HistoryType nhist = 0;
    if (useDolc)
      nhist = dolc.getSign(historySize,historySize);
    else
      nhist = ((l2Index << 1) | ((iID>>2 & 1)^(taken?1:0))) & historyMask;
    historyTable[l1Index] = nhist;
  }
  
  // calculate Table possition
  l2Index = ((l2Index ^ iID) & historyMask ) | (iID<<historySize);

  if (useDolc && taken)
    dolc.update(dinst->getAddr());

  bool ptaken;
  if (doUpdate)
    ptaken = globalTable.predict(l2Index, taken);
  else
    ptaken = globalTable.predict(l2Index);

  if( taken != ptaken) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPHybid
 */

BPHybrid::BPHybrid(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname,"Hybrid")
  ,btb(  i, section, sname)
   ,historySize(SescConf->getInt(section,"historySize"))
   ,historyMask((1 << historySize) - 1)
   ,globalTable(section
                ,SescConf->getInt(section,"l2Size")
                ,SescConf->getInt(section,"l2Bits"))
   ,ghr(0)
   ,localTable(section
               ,SescConf->getInt(section,"localSize")
               ,SescConf->getInt(section,"localBits"))
   ,metaTable(section
              ,SescConf->getInt(section,"MetaSize")
              ,SescConf->getInt(section,"MetaBits"))

{
  // Constraints
  SescConf->isInt(section,    "localSize");
  SescConf->isPower2(section,  "localSize");
  SescConf->isBetween(section, "localBits", 1, 7);

  SescConf->isInt(section    , "MetaSize");
  SescConf->isPower2(section  , "MetaSize");
  SescConf->isBetween(section , "MetaBits", 1, 7);

  SescConf->isInt(section,    "historySize");
  SescConf->isBetween(section, "historySize", 1, 63);

  SescConf->isInt(section,   "l2Size");
  SescConf->isPower2(section, "l2Size");
  SescConf->isBetween(section,"l2Bits", 1, 7);
}

BPHybrid::~BPHybrid()
{
}

PredType BPHybrid::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  bool taken = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  HistoryType l2Index = ghr;

  // update historyTable statistics
  if (doUpdate) {
    ghr = ((ghr << 1) | ((iID>>2 & 1)^(taken?1:0))) & historyMask;
  }

  // calculate Table possition
  l2Index = ((l2Index ^ iID) & historyMask ) | (iID<<historySize);

  bool globalTaken;
  bool localTaken;
  if (doUpdate) {
    globalTaken = globalTable.predict(l2Index, taken);
    localTaken  = localTable.predict(iID, taken);
  }else{
    globalTaken = globalTable.predict(l2Index);
    localTaken  = localTable.predict(iID);
  }

  bool metaOut;
  if (!doUpdate) {
    metaOut = metaTable.predict(l2Index); // do not update meta
  }else if( globalTaken == taken && localTaken != taken) {
    // global is correct, local incorrect
    metaOut = metaTable.predict(l2Index, false);
  }else if( globalTaken != taken && localTaken == taken) {
    // global is incorrect, local correct
    metaOut = metaTable.predict(l2Index, true);
  }else{
    metaOut = metaTable.predict(l2Index); // do not update meta
  }

  bool ptaken = metaOut ? localTaken : globalTaken;

  if (taken != ptaken) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * 2BcgSkew 
 *
 * Based on:
 *
 * "De-aliased Hybird Branch Predictors" from A. Seznec and P. Michaud
 *
 * "Design Tradeoffs for the Alpha EV8 Conditional Branch Predictor"
 * A. Seznec, S. Felix, V. Krishnan, Y. Sazeides
 */

BP2BcgSkew::BP2BcgSkew(int32_t i, const char *section, const char *sname)
  : BPred(i, section, sname,"2BcgSkew")
  ,btb(   i, section, sname)
  ,BIM(section,SescConf->getInt(section,"BIMSize"))
  ,G0(section,SescConf->getInt(section,"G0Size"))
  ,G0HistorySize(SescConf->getInt(section,"G0HistorySize"))
  ,G0HistoryMask((1 << G0HistorySize) - 1)
  ,G1(section,SescConf->getInt(section,"G1Size"))
  ,G1HistorySize(SescConf->getInt(section,"G1HistorySize"))
  ,G1HistoryMask((1 << G1HistorySize) - 1)
  ,metaTable(section,SescConf->getInt(section,"MetaSize"))
  ,MetaHistorySize(SescConf->getInt(section,"MetaHistorySize"))
  ,MetaHistoryMask((1 << MetaHistorySize) - 1)
{
  // Constraints
  SescConf->isInt(section    , "BIMSize");
  SescConf->isPower2(section  , "BIMSize");
  SescConf->isGT(section      , "BIMSize", 1);

  SescConf->isInt(section    , "G0Size");
  SescConf->isPower2(section  , "G0Size");
  SescConf->isGT(section      , "G0Size", 1);

  SescConf->isInt(section    , "G0HistorySize");
  SescConf->isBetween(section , "G0HistorySize", 1, 63);

  SescConf->isInt(section    , "G1Size");
  SescConf->isPower2(section  , "G1Size");
  SescConf->isGT(section      , "G1Size", 1);

  SescConf->isInt(section    , "G1HistorySize");
  SescConf->isBetween(section , "G1HistorySize", 1, 63);

  SescConf->isInt(section    , "MetaSize");
  SescConf->isPower2(section  , "MetaSize");
  SescConf->isGT(section      , "MetaSize", 1);

  SescConf->isInt(section    , "MetaHistorySize");
  SescConf->isBetween(section , "MetaHistorySize", 1, 63);

  history = 0x55555555;
}

BP2BcgSkew::~BP2BcgSkew()
{
  // Nothing?
}


PredType BP2BcgSkew::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if (dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  HistoryType iID = calcHist(dinst->getPC());
  
  bool taken = dinst->isTaken();

  HistoryType xorKey1    = history^iID;
  HistoryType xorKey2    = history^(iID>>2);
  HistoryType xorKey3    = history^(iID>>4);

  HistoryType metaIndex = (xorKey1 & MetaHistoryMask) | iID<<MetaHistorySize;
  HistoryType G0Index   = (xorKey2 & G0HistoryMask)   | iID<<G0HistorySize;
  HistoryType G1Index   = (xorKey3 & G1HistoryMask)   | iID<<G1HistorySize;

  bool metaOut = metaTable.predict(metaIndex);

  bool BIMOut   = BIM.predict(iID);
  bool G0Out    = G0.predict(G0Index);
  bool G1Out    = G1.predict(G1Index);

  bool gskewOut = (G0Out?1:0) + (G1Out?1:0) + (BIMOut?1:0) >= 2;

  bool ptaken = metaOut ? BIMOut : gskewOut;

  if (ptaken != taken) {
    if (!doUpdate)
      return MissPrediction;
    
    BIM.predict(iID,taken);
    G0.predict(G0Index,taken);
    G1.predict(G1Index,taken);

    BIMOut   = BIM.predict(iID);
    G0Out    = G0.predict(G0Index);
    G1Out    = G1.predict(G1Index);
      
    gskewOut = (G0Out?1:0) + (G1Out?1:0) + (BIMOut?1:0) >= 2;
    if (BIMOut != gskewOut)
      metaTable.predict(metaIndex,(BIMOut == taken));
    else
      metaTable.reset(metaIndex,(BIMOut == taken));
    
    I(doUpdate);
    btb.updateOnly(dinst);
    return MissPrediction;
  }

  if (doUpdate) {
    if (metaOut) {
      BIM.predict(iID,taken);
    }else{
      if (BIMOut == taken)
        BIM.predict(iID,taken);
      if (G0Out == taken)
        G0.predict(G0Index,taken);
      if (G1Out == taken)
        G1.predict(G1Index,taken);
    }
    
    if (BIMOut != gskewOut)
      metaTable.predict(metaIndex,(BIMOut == taken));
    
    history = history<<1 | ((iID>>2 & 1)^(taken?1:0));
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * YAGS
 * 
 * Based on:
 *
 * "The YAGS Brnach Prediction Scheme" by A. N. Eden and T. Mudge
 *
 * Arguments to the predictor:
 *     type    = "yags"
 *     l1size  = (in power of 2) Taken Cache Size.
 *     l2size  = (in power of 2) Not-Taken Cache Size.
 *     l1bits  = Number of bits for Cache Taken Table counter (2).
 *     l2bits  = Number of bits for Cache NotTaken Table counter (2).
 *     size    = (in power of 2) Size of the Choice predictor.
 *     bits    = Number of bits for Choice predictor Table counter (2).
 *     tagbits = Number of bits used for storing the address in
 *               direction cache.
 *
 * Description:
 *
 * This predictor tries to address the conflict aliasing in the choice
 * predictor by having two direction caches. Depending on the
 * prediction, the address is looked up in the opposite direction and if
 * there is a cache hit then that predictor is used otherwise the choice
 * predictor is used. The choice predictor and the direction predictor
 * used are updated based on the outcome.
 *
 */

BPyags::BPyags(int32_t i, const char *section, const char *sname)
  :BPred(i, section, sname, "yags")
  ,btb(  i, section, sname)
  ,historySize(24)
  ,historyMask((1 << 24) - 1)
  ,table(section
           ,SescConf->getInt(section,"size")
           ,SescConf->getInt(section,"bits"))
  ,ctableTaken(section
                ,SescConf->getInt(section,"l1size")
                ,SescConf->getInt(section,"l1bits"))
  ,ctableNotTaken(section
                   ,SescConf->getInt(section,"l2size")
                   ,SescConf->getInt(section,"l2bits"))
{
  // Constraints
  SescConf->isInt(section, "size");
  SescConf->isPower2(section, "size");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "bits", 1, 7);

  SescConf->isInt(section, "l1size");
  SescConf->isPower2(section, "l1bits");
  SescConf->isGT(section, "l1size", 1);

  SescConf->isBetween(section, "l1bits", 1, 7);

  SescConf->isInt(section, "l2size");
  SescConf->isPower2(section, "l2bits");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "l2bits", 1, 7);

  SescConf->isBetween(section, "tagbits", 1, 7);

  CacheTaken = new uint8_t[SescConf->getInt(section,"l1size")];
  CacheTakenMask = SescConf->getInt(section,"l1size") - 1;
  CacheTakenTagMask = (1 << SescConf->getInt(section,"tagbits")) - 1;

  CacheNotTaken = new uint8_t[SescConf->getInt(section,"l2size")];
  CacheNotTakenMask = SescConf->getInt(section,"l2size") - 1;
  CacheNotTakenTagMask = (1 << SescConf->getInt(section,"tagbits")) - 1;

  // Done
}

BPyags::~BPyags()
{

}

PredType BPyags::predict(DInst *dinst,bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  bool taken = dinst->isTaken();
  HistoryType iID      = calcHist(dinst->getPC());
  HistoryType iIDHist  = ghr;
  bool choice;
  if (doUpdate) {
    ghr = ((ghr << 1) | ((iID>>2 & 1)^(taken?1:0))) & historyMask;
    choice = table.predict(iID, taken);
  }else
    choice = table.predict(iID);

  iIDHist = ((iIDHist ^ iID) & historyMask ) | (iID<<historySize);

  bool ptaken;
  if (choice) {
    ptaken = true;

    // Search the not taken cache. If we find an entry there, the
    // prediction from the cache table will override the choice table.

    HistoryType cacheIndex = iIDHist & CacheNotTakenMask;
    HistoryType tag        = iID & CacheNotTakenTagMask;
    bool cacheHit          = (CacheNotTaken[cacheIndex] == tag);

    if (cacheHit) {
      if (doUpdate) {
        CacheNotTaken[cacheIndex] = tag;
        ptaken = ctableNotTaken.predict(iIDHist, taken);
      } else {
        ptaken = ctableNotTaken.predict(iIDHist);
      }
    } else if ((doUpdate) && (taken == false)) {
      CacheNotTaken[cacheIndex] = tag;
      (void)ctableNotTaken.predict(iID, taken);
    }
  } else {
    ptaken = false;
    // Search the taken cache. If we find an entry there, the prediction
    // from the cache table will override the choice table.

    HistoryType cacheIndex = iIDHist & CacheTakenMask;
    HistoryType tag        = iID & CacheTakenTagMask;
    bool cacheHit          = (CacheTaken[cacheIndex] == tag);

    if (cacheHit) {
      if (doUpdate) {
        CacheTaken[cacheIndex] = tag;
        ptaken = ctableTaken.predict(iIDHist, taken);
      } else {
        ptaken = ctableTaken.predict(iIDHist);
      }
    } else if ((doUpdate) && (taken == true)) {
        CacheTaken[cacheIndex] = tag;
        (void)ctableTaken.predict(iIDHist, taken);
        ptaken = false;
    }
  }

  if( taken != ptaken ) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }
  
  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPOgehl
 *
 * Based on "The O-GEHL Branch Predictor" by Andre Seznec
 * Code ported from Andre's CBP 2004 entry by Jay Boice (boice@soe.ucsc.edu)
 * 
 */
 
BPOgehl::BPOgehl(int32_t i,const char *section, const char *sname)
  :BPred(i, section, sname, "ogehl")
  ,btb(  i, section, sname)
  ,mtables(SescConf->getInt(section,"mtables"))
  ,glength(SescConf->getInt(section,"glength"))
  ,nentry(3)
  ,addwidth(8)
  ,logpred(log2i(SescConf->getInt(section,"tsize")))
  ,THETA(SescConf->getInt(section,"mtables"))
  ,MAXTHETA(31)
  ,THETAUP(1 << (SescConf->getInt(section,"tcbits") - 1))
  ,PREDUP(1 << (SescConf->getInt(section,"tbits") - 1))
  ,TC(0)
{
  SescConf->isInt(section, "tsize");
  SescConf->isPower2(section, "tsize");
  SescConf->isGT(section, "tsize", 1);
  SescConf->isBetween(section, "tbits", 1, 15);
  SescConf->isBetween(section, "tcbits", 1, 15);
  SescConf->isBetween(section, "mtables", 3, 32);

  pred = new char*[mtables];
  for (int32_t i = 0; i < mtables; i++) {
    pred[i] = new char[1 << logpred];
    for (int32_t j = 0; j < (1 << logpred); j++)
      pred[i][j] = 0;
  }

  T = new int[nentry * logpred + 1];
  ghist = new int64_t[(glength >> 6) + 1];
  MINITAG = new uint8_t[(1 << (logpred - 1))];
  
  for (int32_t i = 0; i < (glength >> 6) + 1; i++) {
    ghist[i] = 0;
  }

  for (int32_t j = 0; j < (1 << (logpred - 1)); j++)
    MINITAG[j] = 0;
  AC=0;

  double initset = 3;
  double tt = ((double)glength) / initset;
  double Pow = pow(tt, 1.0/(mtables + 1));
  
  histLength = new int[mtables + 3];
  usedHistLength = new int[mtables];
  histLength[0] = 0;
  histLength[1] = 3;
  for (int32_t i = 2; i < mtables + 3; i++)
    histLength[i] = (int) ((initset * pow (Pow, (double) (i - 1))) + 0.5);
  for (int32_t i = 0; i < mtables; i++) {
    usedHistLength[i] = histLength[i];
  }
}

BPOgehl::~BPOgehl()
{
}

PredType BPOgehl::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  bool taken = dinst->isTaken();
  bool ptaken = false;

  int32_t S = 0; // mtables/2
  HistoryType *iID = (HistoryType *)alloca(mtables*sizeof(HistoryType));

  // Prediction is sum of entries in M tables (table 1 is half-size to fit in 64k)
  for (int32_t i = 0; i < mtables; i++) {
    if (i == 1)
      logpred--;
    iID[i] = geoidx(dinst->getPC()>>2, ghist, usedHistLength[i], (i & 3) + 1);    
    if (i == 1)
      logpred++;
    S += pred[i][iID[i]];
  }
  ptaken = (S >= 0);
#if 0
  for (int32_t i = 0; i < mtables; i++) {
    MSG("0x%llx %s (%d) idx=%d pred=%d total=%d",dinst->getPC(), ptaken==taken?"C":"M", i, iID[i], (int)pred[i][iID[i]],S);
  }
#endif

  if( doUpdate ) {
 
    // Update theta (threshold)
    if (taken != ptaken) {
      TC++;
      if (TC > THETAUP - 1) {
        TC = THETAUP - 1;
        if (THETA < MAXTHETA) {
          TC = 0;
          THETA++;
        }
      }
    } else if (S < THETA && S >= -THETA) {
      TC--;
      if (TC < -THETAUP) {
        TC = -THETAUP;
        if (THETA > 0) {
          TC = 0;
          THETA--;
        }
      }
    }
  
    if( taken != ptaken || (S < THETA && S >= -THETA)) {

      // Update M tables
      for (int32_t i = 0; i < mtables; i++) {
        if (taken) {
          if (pred[i][iID[i]] < PREDUP - 1)
            pred[i][iID[i]]++;
        } else {
          if (pred[i][iID[i]] > -PREDUP)
            pred[i][iID[i]]--;
        }
      }

      // Update history lengths
      if (taken != ptaken) {
        miniTag = MINITAG[iID[mtables - 1] >> 1];
        if (miniTag != genMiniTag(dinst)) {
          AC -= 4;
          if (AC < -256) {
            AC = -256;
            usedHistLength[6] = histLength[6];
            usedHistLength[4] = histLength[4];
            usedHistLength[2] = histLength[2];
          }
        }else{
          AC++;
          if (AC > 256 - 1) {
            AC = 256 - 1;
            usedHistLength[6] = histLength[mtables + 2];
            usedHistLength[4] = histLength[mtables + 1];
            usedHistLength[2] = histLength[mtables];
          }
        }
      }
      MINITAG[iID[mtables - 1] >> 1] = genMiniTag(dinst);
    }
  
    // Update branch/path histories
    for (int32_t i = (glength >> 6)+1; i > 0; i--)
      ghist[i] = (ghist[i] << 1) + (ghist[i - 1] < 0);
    ghist[0] = ghist[0] << 1;
    if (taken) {
      ghist[0] = 1;
    }
#if 0
    static int conta = 0;
    conta++;
    if (conta > glength) {
      conta = 0;
      printf("@%lld O:",globalClock);
      uint64_t start_mask = glength&63;
      start_mask          = 1<<start_mask;
      for (int32_t i = (glength >> 6)+1; i > 0; i--) {
        for (uint64_t j=start_mask;j!=0;j=j>>1) {
          if (ghist[i] & j) {
            printf("1");
          }else{
            printf("0");
          }
        }
        start_mask=((uint64_t) 1)<<63;
        //printf(":");
      }
      printf("\n");
    }
#endif
  }

  if (taken != ptaken) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }
  
  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}


int32_t BPOgehl::geoidx(uint64_t Add, int64_t *histo, int32_t m, int32_t funct)
{
  uint64_t inter, Hh, Res;
  int32_t x, i, shift;
  int32_t PT;
  int32_t MinAdd;
  int32_t FUNCT;

  MinAdd = nentry * logpred - m;
  if (MinAdd > 20)
    MinAdd = 20;

  if (MinAdd >= 8) {
    inter =
      ((histo[0] & ((1 << m) - 1)) << (MinAdd)) +
      ((Add & ((1 << MinAdd) - 1)));
  }else{
    for (x = 0; x < nentry * logpred; x++) {
      T[x] = ((x * (addwidth + m - 1)) / (nentry * logpred - 1));
    }

    T[nentry * logpred] = addwidth + m;
    inter = 0;

    Hh = histo[0];
    Hh >>= T[0];
    inter = (Hh & 1);
    PT = 1;

    for (i = 1; T[i] < m; i++) {
      if ((T[i] & 0xffc0) == (T[i - 1] & 0xffc0)) {
        shift = T[i] - T[i - 1];
      }else{
        Hh = histo[PT];
        PT++;
        shift = T[i] & 63;
      }
      
      inter = (inter << 1);
      Hh = Hh >> shift;
      inter ^= (Hh & 1);
    }

    Hh = Add;
    for (; T[i] < m + addwidth; i++) {
      shift = T[i] - m;
      inter = (inter << 1);
      inter ^= ((Hh >> shift) & 1);
    }
  }

  FUNCT = funct;
  Res = inter & ((1 << logpred) - 1);
  for (i = 1; i < nentry; i++) {
    inter = inter >> logpred;
    Res ^=
      ((inter & ((1 << logpred) - 1)) >> FUNCT) ^
      ((inter & ((1 << FUNCT) - 1)) << ((logpred - FUNCT)));
    FUNCT = (FUNCT + 1) % logpred;
  }

  return ((int) Res);
}

/*****************************************
 * LGW: Local Global Wavelet
 *
 * Extending OGEHL with TAGE ideas and LGW 
 *
 */

LoopPredictor::LoopPredictor(int n)
 :nentries(n) {

   table = new LoopEntry[nentries];
}

void LoopPredictor::update(uint64_t key, uint64_t tag, bool taken) {

  // FIXME: add a small learning fully assoc (12 entry?) to learn loops. Backup with a 2-way loop entry
  //
  // FIXME: add some resilience to have loops that alternate between different loop sizes.
  //
  // FIXME: systematically check all the loops in CBP, and see how to capture them all

  LoopEntry *ent= &table[key % nentries];

  if (ent->tag != tag) {
    ent->tag         = tag;
    ent->confidence  = 0;
    ent->currCounter = 0;
    ent->dir         = taken;
  }

  ent->currCounter++;
  if (ent->dir != taken) {

    if (ent->iterCounter == ent->currCounter) {
    //MSG("1updt: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir, ent->confidence);
      ent->confidence++;
    }else{
    //MSG("2updt: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir, ent->confidence);
      ent->tag        = 0;
      ent->confidence = 0;
    }

    if (ent->confidence==0)
      ent->dir = taken;

    ent->iterCounter = ent->currCounter;
    ent->currCounter = 0;
  }
}

bool LoopPredictor::isLoop(uint64_t key, uint64_t tag) const {
  const LoopEntry *ent= &table[key % nentries];

  if (ent->tag != tag)
    return false;

  return (ent->confidence * ent->iterCounter) > 800 && ent->confidence > 7;
}

bool LoopPredictor::isTaken(uint64_t key, uint64_t tag, bool taken) {

  I(isLoop(key,tag));

  LoopEntry *ent= &table[key % nentries];

  bool dir = ent->dir;
  if ((ent->currCounter+1) == ent->iterCounter)
    dir = !ent->dir;

  if (dir!=taken) {
    //MSG("baad: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir, ent->confidence);

    ent->confidence /= 2;
  }else{
    //MSG("good: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir, ent->confidence);
  }

  return dir;
}

uint32_t LoopPredictor::getLoopIter(uint64_t key, uint64_t tag) const {
  const LoopEntry *ent= &table[key % nentries];

  if (ent->tag != tag)
    return 0;

  return ent->iterCounter;
}
 
// TODO:
//
// -The path works well most of the time, but things like a loop with a fix
// number of iterations tends not to be captured. In a ghr, as long as the
// history is shorter, it would be captured. Create some test cases, maybe use
// a GHR like thingy for cases when finalS_confidence is set
//
// FIXME:
//
// finalS_confident is pretty good, but it should be much better if we have a table indexed by PC and S to 
// detect the finalS_confident (or a way to estimate the missprediction rate), but something like putting over 50%
// of the miss predictions in the !finalS_confident looks reasonable (or better to have Confident with less than 1% missrate)
//
// The finalS_confident should be improved so that it can be done as a "need
// help". If the DGP main predictor needs help, we can ask a secondary.
// Similarly, the secondary is updated/inserted only for PCs that need help
// (PCs, not finalS_confident only).
//
//
// FIXME: (Something to try to help improve replacement)
//
// Maybe read 2 predictions per entry, and apply LRU (entry 0 is MRU), entry promotes to MRU if used (max_tag)

BPDGP::BPDGP(int32_t i,const char *section, const char *sname)
  :BPred(i, section, sname, "dgp")
  ,btb(  i, section, sname)
  ,dolc(SescConf->getInt(section,"glength"),3,9,16)
  ,lp(82833) // Prime number, way too large until the 2way + learn is built
  ,ahead_local(section,SescConf->getInt(section,"tableSize"),4)
  ,ahead_global(section,SescConf->getInt(section,"tableSize"),2)
  ,ahead_meta(section,SescConf->getInt(section,"tableSize"),2)
  ,ntables(SescConf->getInt(section,"ntables"))
  ,nlocal(SescConf->getInt(section,"nlocal"))
  ,glength(2*SescConf->getInt(section,"glength"))
  ,nentry(3)
  ,addwidth(8)
  ,CorrSize(SescConf->getInt(section,"corrSize"))
  ,MaxVal((1 << (SescConf->getInt(section,"tableValBits") - 1))-1)
  ,TableValBits(SescConf->getInt(section,"tableValBits"))
  ,TableTagBits(SescConf->getInt(section,"tableTagBits"))
  ,TableTagMask((1<<SescConf->getInt(section,"tableTagBits"))-1)
{
  alength = glength;
  if (alength>48)
    alength = 48;

  SescConf->isInt(section, "tableSize");
  SescConf->isPower2(section, "tableSize");
  SescConf->isBetween(section, "tableSize", 1,((uint64_t)1)<<(64/nentry-1));
  SescConf->isBetween(section, "tableValBits", 1, 30);
  SescConf->isBetween(section, "tableTagBits", 0, 30);
  SescConf->isBetween(section, "ntables", 3, 32);
  SescConf->isInt(section, "corrSize");
  //SescConf->isPower2(section, "corrSize");

  TableSizeBits = new int32_t[ntables];
  TableSizeMask = new uint32_t[ntables];
  int nbits     = log2i(SescConf->getInt(section,"tableSize"));

  TableSizeBits[0] = nbits+2;
  for(int i=1;i<ntables;i++) {
    TableSizeBits[i] = nbits;
  }
  for(int i=0;i<ntables;i++) {
    TableSizeMask[i] = (1<<TableSizeBits[i])-1;
  }

  ldolc = new DOLC*[nlocal];
  for (int32_t i = 0; i < nlocal; i++) {
    ldolc[i] = new DOLC(32,3,9,18);
  }

  table = new PredEntry*[ntables];
  for (int32_t i = 0; i < ntables; i++) {
    table[i] = new PredEntry[1 << TableSizeBits[i]];
    for (int32_t j = 0; j < (1 << TableSizeBits[i]); j++) {
      table[i][j].val = 0;
      table[i][j].tag = 0;
    }
  }

  int reverseSize = SescConf->getInt(section,"tableSize");
  reverse = new PredEntry[reverseSize];
  for (int32_t j = 0; j < reverseSize; j++) {
    reverse[j].val = 0;
    reverse[j].tag = 0;
  }

  corr = new CorrEntry[CorrSize];
  for (int32_t i = 0; i < CorrSize; i++) {
    corr[i].val = new int32_t[ntables];
    for (int32_t j = 0; j < ntables; j++) {
      corr[i].val[j] = 0 ;
    }
  }

  ahist = new int64_t[(alength >> 6) + 1];
  
  for (int32_t i = 0; i < (alength >> 6) + 1; i++) {
    ahist[i] = 0;
  }

  double tt = ((double)glength*3/2);
  double Pow = pow(tt, 1.0/(ntables));
  
  histLength = new int[ntables];
  histLength[0] = 0;
  histLength[1] = 3; // better? 1;
  for (int32_t i = 2; i < ntables; i++) {
    histLength[i] = histLength[1] + i + (int) ((pow(Pow, (double) (i))) + 0.5);
  }

  for (int32_t i = 0; i < ntables; i++) {
    MSG("table[%d] size=%d histlength=%d taglength=%d",i,TableSizeBits[i], histLength[i],(i+2));
  }

  lp_last_iter = 0;
}

BPDGP::~BPDGP() {
}

// Sorted by impact
#define DGP_LOOP

// TODO: Add a SC like in S-TAGE to detect branches with bias towards T/NT and
// that the predictor has a worse prediction rate that just the bias (maybe
// tune the correlator to detect the best history, hist=0 is same as S-TAGE
// correlator)


// #define DGP_CORR
// FIXME:
//
// Create a Statistical Correlator like in poTAGE+SC, not the HUGE but the SCg

// FIXME:
// Convert the DGP_CORR to a weighted vote (COLT predictor like) to select between the tables.
//
// The COLT/CORR table it is PC indexed. In one side, it can have DGP, in another a small TAGE
// and a small predictor that uses local history (neural?). The small TAGE is
// trained only for difficult DGP branches.
//
// None of the following seem to do much (at least in my traces)
//#define DGP_LOCAL
//#define DGP_AHIST

PredType BPDGP::predict(DInst *dinst, bool doUpdate, bool doStats) {

  bool dolc_updated = false;
  if (dinst->getPC()>loop_end_pc && loop_end_pc!=0) {
    //printf("clr  jmp %llx to %llx\n",dinst->getPC(),dinst->getAddr());
    dolc.update(calcHist(loop_end_pc));
    dolc_updated = true;
    loop_end_pc  = 0;
    loop_counter = 0;
    for(int i=0;i<nlocal;i++)
      ldolc[i]->reset(dinst->getPC());
  }

  if (dinst->getAddr() < dinst->getPC()) {
    //printf("back jmp %llx to %llx\n",dinst->getPC(),dinst->getAddr());
    if (loop_end_pc == dinst->getPC()) {
      loop_counter++;
    }else{
      loop_counter=0;
      for(int i=0;i<nlocal;i++)
        ldolc[i]->reset(dinst->getPC());
    }

    loop_end_pc = dinst->getPC();
  }

  if( dinst->getInst()->isJump() ) {
    if (!dolc_updated)
      dolc.update(calcHist(dinst->getPC()));

#ifdef DGP_AHIST
    for (int32_t i = (alength >> 6)+1; i > 0; i--)
      ahist[i] = (ahist[i] << 1) + (ahist[i - 1] < 0);
    ahist[0] = ahist[0] << 1;
    ahist[0]++;
#endif

    return btb.predict(dinst, doUpdate, doStats);
  }
  if (!dolc_updated)
    dolc.update(calcHist(dinst->getPC()));

  bool taken = dinst->isTaken();
#ifdef DGP_LOCAL
  if (loop_counter>0)
    ldolc[loop_counter % nlocal]->update(calcHist(dinst->getPC()));
#endif

  int32_t S    = 0; // ntables/2
  int32_t preS = 0; // ntables/2
  HistoryType *iID = (HistoryType *)alloca(ntables*sizeof(HistoryType));
  HistoryType *tID = (HistoryType *)alloca(ntables*sizeof(HistoryType));

  // Prediction is sum of entries in M tables (table 1 is half-size to fit in 64k)

  int32_t max_val = 0;
  int32_t min_val = 0;
  int max_tag     = 0;
  int pre_tag     = 0;

  for (int32_t i  = 0; i < ntables; i++) {
    int32_t tableid = i;

    HistoryType idx = geoidx(calcHist(dinst->getPC()), ahist, TableSizeBits[tableid], histLength[tableid], (tableid & 3) + 1, tableid);
    iID[tableid]          = idx & TableSizeMask[tableid];

    idx          = dolc.getSign(TableTagBits,  histLength[tableid]);
    tID[tableid] = idx & TableTagMask; 
#if 1
    // max tag bits per table: 2, 3, 4, 5...
    tID[tableid]          = tID[tableid] & (((uint64_t)1<<(1*tableid+2))-1);
#endif
  }

#ifdef DGP_CORR
  int corr_idx = iID[0] % CorrSize;
#endif
  float nadd=0;
  int64_t corr_best  = -100*MaxVal;
  int64_t corr_sum   = 1;
  int64_t corr_n     = 0;
#ifdef DGP_CORR
  for (int32_t i  = 0; i < ntables; i++) {
    int32_t tableid = i;

    if (table[tableid][iID[tableid]].tag != tID[tableid])
      continue;

    corr_n++;
    corr_sum += corr[corr_idx].val[tableid];

    if (corr[corr_idx].val[tableid] < corr_best)
      continue;
    corr_best = corr[corr_idx].val[tableid];
  }
  float corr_avg   = 0;
  if (corr_n)
    corr_avg = ((float)corr_sum)/corr_n;
  else
    corr_avg = 1;
#endif

  for (int32_t i  = 0; i < ntables; i++) {
    int32_t tableid = i;

    if (table[tableid][iID[tableid]].tag != tID[tableid])
      continue;

    int32_t val = table[tableid][iID[tableid]].val;
    if (val==0)
      continue;

    S = val;

#ifdef DGP_CORR
    val  = val  * (1+corr[corr_idx].val[tableid])/corr_avg;
    nadd = nadd + (1+corr[corr_idx].val[tableid])/corr_avg;
#else
    nadd++;
#endif

    // FIXME: tune the finalS in this case to be aware of the history lenght (longer -> more accurate)
    preS = (preS) + val;
    pre_tag = max_tag;
    max_tag = tableid;
  }
  if (nadd) {
    preS = preS/nadd;
    if (preS>MaxVal)
      preS = MaxVal;
    else if (preS<-MaxVal)
      preS = -MaxVal;
  }

  int finalS;

  bool finalS_confident = false;
  // Approximately, 20% of the branches are !finalS_confident, but they account around 40% of the miss predictions

  if (nadd < 0.1) {
    finalS = 0; // NT default
  }else if (abs(S)<3) {
    finalS=(S+preS)+1; // difficult, bias for taken
  }else{
    finalS_confident = true;
    finalS = (S+preS/(1+abs(S)/2)); // good finalS quality prediction
  }
  bool ptaken = finalS>0;

  uint64_t lpindex = dinst->getPC();
  uint64_t lptag   = dinst->getPC();

  bool isLoop = false;
  if (lp.isLoop(lpindex,lptag)) {
    bool ltaken = lp.isTaken(lpindex,lptag,taken);

#ifdef DGP_LOOP
    lp_last_iter = lp.getLoopIter(lpindex,lptag);

    isLoop = true;
    ptaken = ltaken;
#ifdef DGP_AHEAD2
    ataken = ltaken;
#endif
#endif
  }
  if (abs(table[0][iID[0]].val)>3 || isLoop) // Learn only bias branches
    lp.update(lpindex,lptag,taken);

#ifdef DGP_AHEAD2
  bool pmainpred = ptaken;
  ptaken     = pmainpred?!ataken:ataken;
#else
  bool pmainpred = ptaken;
#endif

#ifdef DGP_AHEAD2
  bool mainpred = ataken!=taken;
#else
  bool mainpred = taken;
#endif

  static int ngood = 0;
  static int nbaad = 0;
  static int ngoodc = 0;
  static int nbaadc = 0;
  static int t_ngood = 0;
  static int t_nbaad = 0;
  static int t_ngoodc = 0;
  static int t_nbaadc = 0;
  if (finalS_confident) {
    if (mainpred == pmainpred)
      ngoodc++;
    else
      nbaadc++;
  }else{
    if (mainpred == pmainpred)
      ngood++;
    else
      nbaad++;
  }

  if (ngood>10064 || nbaad>10064 || ngoodc>10064 || nbaadc>10064) {
    t_ngood  += ngood;
    t_nbaad  += nbaad;
    t_ngoodc += ngoodc;
    t_nbaadc += nbaadc;

#if 0
    MSG("NC mp=%5.1f C mp=%5.1f all mp=%5.1f %%C=%5.1f: NC mp=%5.1f C mp=%5.1f all mp=%5.1f %%C=%5.1f"
        ,(double)100*nbaad/(ngood+nbaad+1)
        ,(double)100*nbaadc/(ngoodc+nbaadc+1)
        ,(double)100*(nbaad+nbaadc)/(ngoodc+nbaadc+ngood+nbaad)
        ,(double)100*(nbaad+ngoodc)/(ngoodc+nbaadc+ngood+nbaad)
        ,(double)100*t_nbaad/(t_ngood+t_nbaad+1)
        ,(double)100*t_nbaadc/(t_ngoodc+t_nbaadc+1)
        ,(double)100*(t_nbaad+t_nbaadc)/(t_ngoodc+t_nbaadc+t_ngood+t_nbaad)
        ,(double)100*(t_nbaad+t_ngoodc)/(t_ngoodc+t_nbaadc+t_ngood+t_nbaad)
        );
#endif
    ngood = 0;
    nbaad = 0;
    ngoodc = 0;
    nbaadc = 0;
  }
#if 0
    printf("%llx %s %s pS=%5d S=%5d m=%5d l=%d ll=%4d lc=%4d ",(int)dinst->getPC(), ptaken!=taken?"b":"g", mainpred?"t":"n", preS, S, max_tag, isLoop?1:0, lp_last_iter, loop_counter);
    for (int32_t i = 0; i < ntables; i++) {
      printf("%+5d%s ",table[i][iID[i]].val,table[i][iID[i]].tag == tID[i]?"h":"m");
    }
    printf("\n");
#endif

    

  if( doUpdate ) {

#if 1
    // Better but difficult to do timing wise in a real system (1 cycle update latency)

    int saturated     = 0;
    bool updated      = false;
    bool corrected    = false;

    bool low          = false;
    bool allsaturated = true; // Like poTAGE during ramup, no need to 
    for (int32_t i  = 0; i < ntables; i++) {
      if (table[i][iID[i]].tag != tID[i])
        continue;
      if (abs(table[i][iID[i]].val)>(MaxVal-1))
        continue;

      allsaturated = false;

      if (abs(table[i][iID[i]].val)>4)
        continue;

      low = true;
      break;
    }

    if( mainpred != pmainpred) { 
#if 1
      if (!low && !allsaturated) {
        for (int32_t i  = 0; i < ntables; i++) {
          if (table[i][iID[i]].tag != tID[i])
            continue;
          table[i][iID[i]].val /= 2;
        }
      }
#endif

      bool reverse_updated = false; // to make sure that a single write per access happens, and to avoid unnecessary updates

      // Update M tables
      for (int32_t i = 0; i < ntables; i++) {

        if (i>max_tag && ((i&1) != (max_tag&1))) {
          if ( table[i][iID[i]].tag != tID[i] && !isLoop ) {
            int32_t val = table[i][iID[i]].val;
            if (val==0) {
              table[i][iID[i]].val = 0;
              table[i][iID[i]].tag = tID[i];
            }else{
              if (!allsaturated) // Like poTAGE, do not steal new entry if all saturated
                table[i][iID[i]].val /= 2;
            }
          }
        }
        if ( table[i][iID[i]].tag == tID[i] ) {
          if (mainpred) {
            if (table[i][iID[i]].val<0)
              corrected = true;

            table[i][iID[i]].val++;
            if (table[i][iID[i]].val >= MaxVal) {
              saturated++;
              table[i][iID[i]].val = MaxVal;
            }

          } else {
            if (table[i][iID[i]].val>0)
              corrected = true;

            table[i][iID[i]].val--;
            if (table[i][iID[i]].val <= -MaxVal) {
              saturated++;
              table[i][iID[i]].val = -MaxVal;
            }
          }
        }
      }
      updated = true;

    }else if (abs(preS + S) < MaxVal && !isLoop && !allsaturated) {
      // Update M tables
      // No need to do table 0 (local history)
      for (int32_t i = 0; i < ntables; i++) {
        if ( table[i][iID[i]].tag == tID[i]) {
          if (mainpred) {
            if (table[i][iID[i]].val<0)
              corrected = true;

            table[i][iID[i]].val++;
            if (table[i][iID[i]].val >= MaxVal) {
              saturated++;
              table[i][iID[i]].val = MaxVal;
            }
          } else {
            if (table[i][iID[i]].val>0)
              corrected = true;

            table[i][iID[i]].val--;
            if (table[i][iID[i]].val <= -MaxVal) {
              saturated++;
              table[i][iID[i]].val = -MaxVal;
            }
          }

        }
      }
      updated = true;
    }
#if 1
    if (!updated && !allsaturated) {
      for (int32_t i = pre_tag; i < ntables; i++) {
        if ( table[i][iID[i]].tag == tID[i]) {
          if (mainpred) {
            if (table[i][iID[i]].val<0)
              corrected = true;

            table[i][iID[i]].val++;
            if (table[i][iID[i]].val >= MaxVal) {
              saturated++;
              table[i][iID[i]].val = MaxVal;
            }
          } else {
            if (table[i][iID[i]].val>0)
              corrected = true;

            table[i][iID[i]].val--;
            if (table[i][iID[i]].val <= -MaxVal) {
              saturated++;
              table[i][iID[i]].val = -MaxVal;
            }
          }
        }
      }
    }
#endif

#ifdef DGP_CORR
    if (corrected || saturated) {
      static int conta = 0;
      conta++;
      if (conta>63300000)
        conta = 0;

      if (!conta)
        printf("pc=%llx ",dinst->getPC());

      saturated = false;
      bool low_corr = false;
      for (int32_t i  = 0; i < ntables; i++) {

        // Check tag for all but table 0
        int32_t c_val = corr[corr_idx].val[i];
        if (table[i][iID[i]].tag == tID[i]) {
          int32_t val = table[i][iID[i]].val;
          if (!conta)
            printf("%3dh ",c_val);
          if ((val>0) == taken)
            corr[corr_idx].val[i]++; // Good
          else
            corr[corr_idx].val[i]--;

          //S = val;
        }else{
          if (!conta)
            printf("%3dm ",c_val);
          corr[corr_idx].val[i]++; // Assume good for not hit
        }

        if (corr[corr_idx].val[i] > 4*MaxVal) {
          saturated = true;
          corr[corr_idx].val[i] = 4*MaxVal;
        } else if (corr[corr_idx].val[i] < 0) {
          low_corr = true;
          corr[corr_idx].val[i] = 0;
        } else if (corr[corr_idx].val[i] < MaxVal/2) {
          low_corr = true;
        }
      }
      if (!conta)
        printf("\n");
      if (!low_corr) {
        for (int32_t i  = 0; i < ntables; i++) {
          int32_t t             = corr[corr_idx].val[i];
          corr[corr_idx].val[i] = t/2;
        }
      }
    }
#endif

#ifdef DGP_AHIST
    for (int32_t i = (alength >> 6)+1; i > 0; i--)
      ahist[i] = (ahist[i] << 1) + (ahist[i - 1] < 0);
    ahist[0] = ahist[0] << 1;
    if (mainpred) {
      ahist[0]++;
    }
#endif
    if (mainpred!=pmainpred)
      for(int i=0;i<nlocal;i++)
        ldolc[i]->reset(dinst->getPC());
#ifdef DGP_LOCAL
    if (taken && loop_counter>0) {
      ldolc[loop_counter % nlocal]->update(calcHist(dinst->getAddr()));
    }
#endif
    if (taken) {
      dolc.update(calcHist(dinst->getAddr()));
    }
#endif
  }

  if (taken != ptaken) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }
  
  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

uint64_t BPDGP::goodHash(uint64_t key) const {

#if 1
  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);
#else
  key = key % 32839;
#endif

  return key;
}

int32_t BPDGP::geoidx(uint64_t Add, int64_t *histo, int32_t sizeBits, int32_t m, int32_t funct, int tableid) {
  uint64_t Res=0;

  int m_orig = m; // For DOLC

  // Not bad for not having branch hostpr
#ifdef DGP_LOCAL
  if ((tableid & 3)==(loop_counter&3) && m_orig < 64 && loop_counter>0)
    Res = ldolc[loop_counter % nlocal]->getSign(sizeBits,m_orig/2);
  else
#endif
    Res = Res ^ dolc.getSign(sizeBits,m_orig);

  return ((int) Res);
}

/*****************************************
 * BPSOgehl  ; SCOORE Adaptation of the OGELH predictor
 *
 * Based on "The O-GEHL Branch Predictor" by Andre Seznec
 * 
 */

#if 1
BPSOgehl::BPSOgehl(int32_t i,const char *section, const char *sname)
  :BPred(i, section, sname, "sogehl")
  ,btb(  i, section, sname)
  ,mtables(SescConf->getInt(section,"mtables"))
  ,glength(SescConf->getInt(section,"glength"))
  ,AddWidth(log2i(SescConf->getInt(section,"tsize")))
  ,logtsize(log2i(SescConf->getInt(section,"tsize")))
  ,THETA(SescConf->getInt(section,"mtables"))
  ,MAXTHETA(31)
  ,THETAUP(1 << (SescConf->getInt(section,"tcbits") - 1))
  ,PREDUP(1 << (SescConf->getInt(section,"tbits") - 1))
  ,TC(0)
{
  SescConf->isInt(section    , "tsize");
  SescConf->isPower2(section , "tsize");
  SescConf->isGT(section     , "tsize"  , 1);
  SescConf->isBetween(section, "tbits"  , 1  , 15);
  SescConf->isBetween(section, "tcbits" , 1  , 15);
  SescConf->isBetween(section, "mtables", 3  , 32);

  pred = new char*[mtables];
  for (int32_t i = 0; i < mtables; i++) {
    pred[i] = new char[1 << logtsize];
    for (int32_t j = 0; j < (1 << logtsize); j++)
      pred[i][j] = 0;
  }

  ghr.resize(glength);

  T       = new int[logtsize + 1];
  MINITAG = new uint8_t[(1 << (logtsize - 1))];
  
  for (int32_t j = 0; j < (1 << (logtsize - 1)); j++)
    MINITAG[j] = 0;
  AC=0;

  double initset = 3;
  double tt      = ((double)glength) / initset;
  double Pow     = pow(tt, 1.0/(mtables + 1));

  histLength     = new int[mtables + 3];
  usedHistLength = new int[mtables];
  histLength[0]  = 0;
  histLength[1]  = 3;
  for (int32_t i = 2; i < mtables + 3; i++)
    histLength[i] = (int) ((initset * pow (Pow, (double) (i - 1))) + 0.5);
  for (int32_t i = 0; i < mtables; i++) {
    usedHistLength[i] = histLength[i];
  }
}

BPSOgehl::~BPSOgehl()
{
}

PredType BPSOgehl::predict(DInst *dinst, bool doUpdate, bool doStats)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate, doStats);

  bool taken  = dinst->isTaken();
  bool ptaken = false;

  int32_t S = (mtables/2);
  HistoryType *iID = (HistoryType *)alloca(mtables*sizeof(HistoryType));

  // Prediction is sum of entries in M tables
  for (int32_t i = 0; i < mtables; i++) {
    if (i == 1)
      logtsize--;
    iID[i] = geoidx2(dinst->getPC()>>3, usedHistLength[i]);
    if (i == 1)
      logtsize++;
    S += pred[i][iID[i]];
  }
  ptaken = (S >= 0);

  if( doUpdate ) {
 
    // Update theta (threshold)
    if (taken != ptaken) {
      TC++;
      if (TC > THETAUP - 1) {
        TC = THETAUP - 1;
        if (THETA < MAXTHETA) {
          TC = 0;
          THETA++;
        }
      }
    } else if (S < THETA && S >= -THETA) {
      TC--;
      if (TC < -THETAUP) {
        TC = -THETAUP;
        if (THETA > 0) {
          TC = 0;
          THETA--;
        }
      }
    }
  
    if( taken != ptaken || (S < THETA && S >= -THETA)) {

      // Update M tables
      for (int32_t i = 0; i < mtables; i++) {
        if (taken) {
          if (pred[i][iID[i]] < PREDUP - 1)
            pred[i][iID[i]]++;
        } else {
          if (pred[i][iID[i]] > -PREDUP)
            pred[i][iID[i]]--;
        }
      }
      btb.updateOnly(dinst);

      // Update history lengths
      if ((iID[mtables - 1] & 1) == 0) {
        if (taken != ptaken) {
          miniTag = MINITAG[iID[mtables - 1] >> 1];
          if (miniTag != genMiniTag(dinst)) {
            AC -= 4;
            if (AC < -256) {
              AC = -256;
              usedHistLength[6] = histLength[6];
              usedHistLength[4] = histLength[4];
              usedHistLength[2] = histLength[2];
            }
          }else{
            AC++;
            if (AC > 256 - 1) {
              AC = 256 - 1;
                usedHistLength[6] = histLength[mtables + 2];
                usedHistLength[4] = histLength[mtables + 1];
                usedHistLength[2] = histLength[mtables];
            }
          }
        }
        MINITAG[iID[mtables - 1] >> 1] = genMiniTag(dinst);
      }
    }
  
    // Update branch/path histories
    ghr <<=1;
    ghr[0] = taken;
  }

  if (taken != ptaken)
    return MissPrediction;
  
  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

uint32_t BPSOgehl::geoidx2(uint64_t Add, int32_t m)
{
  uint32_t inter   = Add & ((1<< AddWidth)-1);                                           // start with the PC

  for(int32_t i = 0;i<m;i++) {
    int32_t bit = ghr[i] ^ (inter&1);
    int32_t pos = i %logtsize;

    if (bit)
      inter = inter | (1<<pos);  // set bit
    else
      inter = inter & ~(1<<pos); // clear bit
  }

  return inter;
}
#endif





/***** TAGE Predictor
 * ****
 *
 *
 *
 */



BPTage::BPTage(int32_t i, const char *section, const char *sname)
    :BPred(i, section, sname, "TAGE")
    ,btb(i, section, sname)
    ,numberOfTaggedPredictors(SescConf->getInt(section,"taggedPredictors"))
    ,numberOfBimodalEntries(SescConf->getInt(section,"bimodalEntries"))
    ,taggedTableEntries(SescConf->getInt(section,"taggedEntries"))
    ,maxHistLength(SescConf->getInt(section,"maxHistLength"))
    ,minHistLength(SescConf->getInt(section,"minHistLength")) 
    ,glength(SescConf->getInt(section,"glength"))
    ,hystBits(SescConf->getInt(section,"hystBits"))
    ,ctrCounterWidth(SescConf->getInt(section,"ctrCounterWidth"))
    ,instShiftAmount(log2i(SescConf->getInt(section,"cyclicShift")/2))	//define shiftAmount value in conf file
    ,randomSeed(0)
    ,BPHistory_count(0)
    ,useAltOnNA(0)
    ,pathHistory(0)
    ,sinceBimodalMisprediction(0)
    ,counterLog(19)
    ,counterResetControl((1 << (counterLog - 1)))

{ 
  SescConf->isInt(section, "tsize");
  SescConf->isPower2(section, "tsize");
  SescConf->isGT(section, "tsize", 1);
  SescConf->isBetween(section, "tbits", 1, 15);
  SescConf->isBetween(section, "tcbits", 1, 15);
  SescConf->isBetween(section, "taggedPredictors", 3, 32);

  tagWidth = new unsigned[numberOfTaggedPredictors + 1];
  tagWidth[0] = 9;
  tagWidth[1] = 9;
  tagWidth[2] = 9;
  tagWidth[3] = 10;
  tagWidth[4] = 10;
  tagWidth[5] = 11;
  tagWidth[6] = 11;
  tagWidth[7] = 12;
  tagWidth[8] = 12;

  numberOfEntriesInTaggedComponent = new int[numberOfTaggedPredictors];
  numberOfEntriesInTaggedComponent[0] = -1;

  for (int ii = 1; ii < numberOfTaggedPredictors + 1; ii++) 
  {
    numberOfEntriesInTaggedComponent[ii] = taggedTableEntries;             //ADD IT TO CONF FILE
    //numberOfEntriesInTaggedComponent[ii] = ii + 8;
    //if (numberOfEntriesInTaggedComponent[ii] > 11)
      //numberOfEntriesInTaggedComponent[ii] = 11;
  }


  tageTableTagEntry = new unsigned*[numberOfTaggedPredictors+1]; 
  for (int ii = 0; ii < numberOfTaggedPredictors + 1; ii++) 
  {
    tageTableTagEntry[ii] = new unsigned[taggedTableEntries]; 
  }

  for (int ii = 0; ii < numberOfTaggedPredictors + 1; ii++)
    for (int jj = 0; jj < taggedTableEntries; jj++)
      tageTableTagEntry[ii][jj] = 0;
    

  ctrCounter = new int*[numberOfTaggedPredictors+1];
  for (int ii = 0; ii < numberOfTaggedPredictors + 1; ii++)
  {
    ctrCounter[ii] = new int[taggedTableEntries];
    /*for (int jj = 0; jj < taggedTableEntries; jj++)
    {
      ctrCounter[ii][jj] = 0;
    }*/
  } 

  for (int ii = 0; ii < numberOfTaggedPredictors + 1; ii++) 
    for (int jj = 0; jj < taggedTableEntries; jj++)
      ctrCounter[ii][jj] = 0;

  histLength = new int[numberOfTaggedPredictors + 1]; 
  histLength[0] = 0;
  histLength[1] = minHistLength;    
  for (int32_t i = 2; i < numberOfTaggedPredictors + 1; i++)  // define numHistComponents 
    histLength[i] = (int)((histLength[1] * pow((maxHistLength / minHistLength), (i-1))) + 0.5);
  
  bimodalTableSize = new int[1 << numberOfBimodalEntries]; 
  pred = new unsigned[1 << numberOfBimodalEntries];   
  hyst = new unsigned[1 << numberOfBimodalEntries]; 

  for (int32_t i = 0; i < (1 << numberOfBimodalEntries); i++)
  {
    pred[i] = 0;
    hyst[i] = 0; 
  }

  for (int32_t i = 1; i < numberOfTaggedPredictors + 1; i++)
    taggedTableSize = new int[1 << numberOfEntriesInTaggedComponent[i]];

  taggedTableTagMask = new AddrType[numberOfTaggedPredictors+1];
  taggedTableIdxMask = new AddrType[numberOfTaggedPredictors+1]; 

  //mask calculation
  bimodalIdxMask = ((1 << numberOfBimodalEntries) - 1);  
  
  for (int32_t i = 1; i < numberOfTaggedPredictors + 1; i++)
    taggedTableTagMask[i] = ((1 << tagWidth[i-1]) - 1);
  
  for (int32_t i = 1; i < numberOfTaggedPredictors + 1; i++) 
    taggedTableIdxMask[i] = ((1 << numberOfEntriesInTaggedComponent[i]) - 1);

  ghist = new uint64_t[(glength >> 6) + 1]; 
                
  for (int32_t i = 0; i < (glength >> 6) + 1; i++)
    ghist[i] = 0;
  
  globalHistory = new unsigned[numberOfTaggedPredictors+1];
  retireGlobalHistory = new unsigned[numberOfTaggedPredictors+1];

  for (int32_t i = 0; i < numberOfTaggedPredictors; i++)
  {  
    globalHistory[i] = 0;  
    retireGlobalHistory[i] = 0;
  }

}


BPTage::~BPTage()
{
}


unsigned BPTage :: hashIndexForTaggedTable (DInst *dinst, unsigned bankID)
{
  AddrType branchAddr = dinst->getPC();
  AddrType branch_addr = (branchAddr >> instShiftAmount);
  AddrType index;
  unsigned M = (histLength[bankID] > 16) ? 16 : histLength[bankID];
  index = (branch_addr ^ (branch_addr >> static_cast<unsigned>(abs(numberOfEntriesInTaggedComponent[bankID] - bankID) + 1)) ^ F(pathHistory, M, bankID));
  return (unsigned)(index & taggedTableIdxMask[bankID]);

}



unsigned BPTage :: F (unsigned history, unsigned numTables, unsigned bankID)
{
  AddrType res, h1, h2;
  res = (AddrType)history;
  res = res & ((1 << numTables) - 1);
  h1 = (res & taggedTableIdxMask[bankID]);
  h2 = (res >> numberOfEntriesInTaggedComponent[bankID]); 
  h2 = ((h2 << bankID) & taggedTableIdxMask[bankID]) + (h2 >> (numberOfEntriesInTaggedComponent[bankID] - bankID));
  res = h1 ^ h2;
  res = ((res << bankID) & taggedTableIdxMask[bankID]) + (res >> (numberOfEntriesInTaggedComponent[bankID] - bankID));
  return (unsigned)res;
}


unsigned BPTage :: tagCalculation (DInst *dinst, unsigned bankID) 
{
  AddrType addr = dinst->getPC();
  AddrType branch_addr = (addr >> instShiftAmount);
  AddrType tag = branch_addr;
  return (unsigned)(tag & taggedTableTagMask[bankID]);

}


bool BPTage :: getBimodalPrediction (DInst *dinst, bool saturated)
{
  AddrType branchAddr = dinst->getPC();	
  unsigned index = bimodalIndexCalculation(branchAddr);
  unsigned inter = (pred[index] << 1) + hyst[index >> baseHystShift]; 
  saturated = (inter == 0 || inter == 3);
  return (pred[index] > 0);

}

void BPTage :: bimodalUpdate (DInst *dinst, bool taken)
{
  AddrType branchAddr = dinst->getPC();		
  unsigned index = bimodalIndexCalculation(branchAddr);
  unsigned inter = (pred[index] << 1) + hyst[index >> baseHystShift];
  unsigned old = inter;

  if (taken) 
  {
    if (old < 3)
      ++inter;
  }
  else if (old > 0)
    --inter;
  
  pred[index] = inter >> 1;
  hyst[index >> baseHystShift] = (inter & 1);

}


void BPTage :: updateGlobalHistory (DInst *dinst, bool taken, void * &bph, bool save)
{
  AddrType branch_addr = dinst->getPC();
  unsigned size = numberOfTaggedPredictors + 1;
  unsigned i, direction, id;
  historyOverhead h;
  h.direction = taken ? 1 : 0;
  h.id = BPHistory_count;				

  BPHistory *his = new BPHistory(true);
  his->historyID = BPHistory_count++;
  
  globHist.push_front(h);
  his->directionPtr = &(globHist.front());  

  pathHistory = (pathHistory << 1) | ((branch_addr >> instShiftAmount) & 1);
  pathHistory = (pathHistory & ((1 << 16) - 1));
  
}

bool BPTage :: lookup (DInst *dinst, void * &bph)
{
  int i;
  AddrType branch_addr = dinst->getPC();
  unsigned size = numberOfTaggedPredictors + 1;
  unsigned hitBank = 0;
  unsigned altBank = 0;
  bool tagePrediction;			
  bool altTaken;
  bool predictionTaken;
  bool bimodalPrediction = false;	
  bool bimodalSaturated = false;	
  unsigned *hitBankList, *altBankList;

  BPHistory *his = new BPHistory(true);

  his->tagStore = new unsigned [size];
  his->indexStore = new unsigned [size];
  hitBankList = new unsigned[size];
  altBankList = new unsigned[size];

  for (i = 1; i < size; i++)
  {
    his->indexStore[i] = hashIndexForTaggedTable (dinst, i);
    his->tagStore [i] = tagCalculation (dinst, i);
  }

  for (i = size - 1; i > 0; --i)
  {
    if (tageTableTagEntry[i][his->indexStore[i]] = his->tagStore[i])
    {
      hitBank = i;
      break;
    }
  }

  for (i = size-1; i > 0; --i)
  {
    if (tageTableTagEntry[i][his->indexStore[i]] == his->tagStore[i])
      if ((useAltOnNA < 0) || (abs(2 * ctrCounter[i][his->indexStore[i]] + 1 > 1)))
      {
    	altBank = i;	    
        break;  
      }
  }

  if (hitBank > 0)
  {
    if (altBank > 0)
      altTaken = (ctrCounter[altBank][his->indexStore[altBank]] >= 0);
    else
    {
      altTaken = getBimodalPrediction (dinst, bimodalSaturated);
      bimodalPrediction = true; 
    }
    if ((useAltOnNA < 0) || (abs (2 * ctrCounter[hitBank][his->indexStore[hitBank]] + 1) > 1))
    {
      tagePrediction = (ctrCounter[hitBank][his->indexStore[hitBank]] >= 0);
      bimodalPrediction = false;
      int ctr = ctrCounter[hitBank][his->indexStore[hitBank]];
      if (ctr == 0 || ctr == -1)
	his->wTag = true;
      if (ctr == 1 || ctr == -2)	
	his->nwTag = true;
      if (ctr == 2 || ctr == -3)
	his->nsTag = true;

      his->usedTagged = true;
      his->usedStandard = true;
    }
    else
    {
      tagePrediction = altTaken;
      if (bimodalPrediction)
      {
	if (bimodalSaturated)
	  his->bimodalHighConfidence = true;
	else
	  his->bimodalLowConfidence = true;

	his->usedBimodal = true;
      } 
      else
      {
        int ctr = ctrCounter[altBank][his->indexStore[altBank]];
	if (ctr == 0 || ctr == -1)
	  his->wTag = true;
	if (ctr == 1 || ctr == -2)
	  his->nwTag = true;
	if (ctr == 2 || ctr == -3)
	  his->nsTag = true;
	if (hConf[hitBank][his->indexStore[hitBank]])
  	{
	  his->sTag = true;
	  his->nsTag = false;
	}
	
	his->usedTagged = true;
      }
      his->usedAlt = true;
    }
  }
  else
  {
    altTaken = getBimodalPrediction(dinst, bimodalSaturated);
    tagePrediction = altTaken;
    bimodalPrediction = true;
    if (bimodalSaturated)
      his->bimodalHighConfidence = true;
    else
      his->bimodalLowConfidence = true;

    his->usedBimodal = true;
    his->usedAlt = true;  
  }
  
  predictionTaken = tagePrediction; 
  if (bimodalPrediction && sinceBimodalMisprediction < 8 && his-> bimodalHighConfidence)
  {
    his->bimodalMediumConfidence = true;
    his->bimodalHighConfidence = false;
    his->bimodalLowConfidence = false;
  }
  if (his->bimodalHighConfidence || his->sTag)
    his->highConfidence = true;
  if (his->bimodalMediumConfidence || his->nsTag)
    his->mediumConfidence = true;
  if (his->bimodalLowConfidence || his->wTag || his->nwTag)
    his->lowConfidence = true;

  sinceBimodalMisprediction++;
 
  updateGlobalHistory(dinst, predictionTaken, bph, true); 
  return predictionTaken; 

}


void BPTage :: unconditionalBranch(DInst *dinst, void * &bph)
{
  AddrType branch_addr = dinst->getPC();
  BPHistory *his = new BPHistory(true);
  his->predictTaken = true;
  his->tagePrediction = true;
  his->unconditional = true;
  his->actuallyTaken = true;
  his->highConfidence = true;

  bph = static_cast<void *>(his);
  updateGlobalHistory(dinst, true, bph, true);

}


void BPTage :: updatePredictorDirection(DInst *dinst, bool taken, void *bph)
{
  AddrType branch_addr = dinst->getPC();
  unsigned hitBank, altBank, i, j;
  bool tagePrediction, altTaken;
  unsigned size = numberOfTaggedPredictors + 1; 
  unsigned randomTemp = randomGenerator();
  BPHistory *his = static_cast<BPHistory *>(bph);
  
  tagePrediction = his->tagePrediction;
  altTaken = his->altTaken;
  hitBank = his->hitBank;
  altBank = his->altBank;

  bool allocate = ((tagePrediction != taken) && (hitBank < numberOfTaggedPredictors));
  if(hitBank > 0)
  {
    bool longestMatch = (ctrCounter[hitBank][his->indexStore[hitBank]] >= 0);
    bool pseudoLongest = ((abs(2 * ctrCounter[hitBank][his->indexStore[hitBank]] + 1) <= 1)); 
    
    if(pseudoLongest)
    {
      if(longestMatch == taken)
        allocate = false;
      if(longestMatch != altTaken)
      {
	if(altTaken == taken)
	{
	  if(useAltOnNA < 7)
	    useAltOnNA++;
	}
	else if(useAltOnNA > -8)
	  useAltOnNA--;
      }
      if(useAltOnNA >= 0)
	tagePrediction = longestMatch;
    }
  }

  if(allocate)
  {
    unsigned min = 1;
    for(i = numberOfTaggedPredictors; i > hitBank; --i)
      if(tageTableUsefulCounter[i][his->indexStore[i]] < min)
        min = tageTableUsefulCounter[i][his->indexStore[i]];
    unsigned x = hitBank + 1;
    unsigned y = randomTemp & ((1 << (numberOfTaggedPredictors - hitBank - 1)) - 1);
    if(y & 1)
    {
      x++;
      if(y & 2)
        x++;
    }
    
 
    for(i = x; i < size; ++i)
      if(tageTableUsefulCounter[i][his->indexStore[i]] == 0)
      {
	tageTableTagEntry[i][his->indexStore[i]] = his->tagStore[i];
 	ctrCounter[i][his->indexStore[i]] = (taken ? 0 : -1);
        tageTableUsefulCounter[i][his->indexStore[i]] = 0;
	hConf[i][his->indexStore[i]] = false;
	break; 
      }
    
  }

  counterResetControl++;
  if ((counterResetControl & ((1 << counterLog) - 1)) == 0)
    for (i = 1; i < size; ++i)
      for (j = 0; j < (1 << numberOfEntriesInTaggedComponent[i]); i++)
	tageTableUsefulCounter[i][j] = tageTableUsefulCounter[i][j] >> 1;

  if (hitBank > 0)
  {
    if (taken)
      ctrCounter[hitBank][his->indexStore[altBank]]++;
    else
      ctrCounter[hitBank][his->indexStore[altBank]]--;

    if (tageTableUsefulCounter[hitBank][his->indexStore[hitBank]] == 0)
    {
      if (altBank > 0)
        if (taken)
   	  ctrCounter[hitBank][his->indexStore[altBank]]++;
	else
	  ctrCounter[hitBank][his->indexStore[altBank]]--;

      if (altBank == 0)
	bimodalUpdate(dinst, taken); 
    }
  }
  else
    bimodalUpdate(dinst, taken); 

  if (tagePrediction != altTaken)
  {
    if (tagePrediction == taken)
    {
      if (tageTableUsefulCounter[hitBank][his->indexStore[hitBank]] < 3)
	tageTableUsefulCounter[hitBank][his->indexStore[hitBank]]++;	
    } 
    else
    {
      if (useAltOnNA < 0)
	if (tageTableUsefulCounter[hitBank][his->indexStore[hitBank]] > 0)
	  tageTableUsefulCounter[hitBank][his->indexStore[hitBank]]--;
    }
  }

}


void BPTage :: recoverFromMisprediction (DInst *dinst, bool taken, void *bph) 
{
  unsigned i, size = numberOfTaggedPredictors + 1;
  BPHistory *his = static_cast<BPHistory *>(bph);
  updateGlobalHistory(dinst, taken, bph, false); 

}


void BPTage::updateBranchPredictor(DInst *dinst, bool taken, void *bph, bool squash) {

  if (bph==0)
    return;

  void *abcd;
  AddrType branch_addr = dinst->getPC();
  BPHistory *his = static_cast<BPHistory *>(bph);
  if (!squash) {

    assert(his->directionPtr->id == his->historyID);
    assert(his->directionPtr->direction == his->actuallyTaken);
    historyOverhead ho;
    ho.direction = his->actuallyTaken;
    ho.id = BPHistory_count;
    assert(bph);

    retire_globHist.push_front(ho);
    retire_globHist.pop_back();

    if (!his->unconditional)
      updatePredictorDirection(dinst, his->actuallyTaken, bph); 

    globHist.pop_back();
  } else {
    globHist.pop_front();
    recoverFromMisprediction(dinst, taken, bph);
  }

}


void BPTage::fixGlobalHistory(DInst *dinst, void *bph, bool actTaken) {

  BPHistory *his = static_cast<BPHistory *>(bph);
  globHist.pop_front();
  recoverFromMisprediction(dinst, actTaken, bph);
  his->actuallyTaken = actTaken;
} 


void BPTage::squashRecovery(void *bph) {

  unsigned i, size = numberOfTaggedPredictors + 1;
  BPHistory *his = static_cast<BPHistory *>(bph);
  assert(his); 
  assert(globHist.front().id == his->historyID);
  globHist.pop_front();
  pathHistory = his->pathHistoryBackup;

  delete his;
}

PredType BPTage::predict(DInst *dinst, bool doUpdate, bool doStats) {

  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  void *bph=0;  
  bool taken  = dinst->isTaken();
  bool squash = false;

  bool ptaken = lookup(dinst, bph);

  if (taken != ptaken) {
    if (doUpdate) {
      updateBranchPredictor(dinst, taken, bph, true);
      btb.updateOnly(dinst); 
    }
    return MissPrediction;
  }

  updateBranchPredictor(dinst, taken, bph, false);

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;

}


/*****************************************
 * BPredictor
 */


BPred *BPredictor::getBPred(int32_t id, const char *sec, const char *sname)
{
  BPred *pred=0;
  
  const char *type = SescConf->getCharPtr(sec, "type");

  // Normal Predictor
  if (strcasecmp(type, "oracle") == 0) {
    pred = new BPOracle(id, sec, sname);
  } else if (strcasecmp(type, "Miss") == 0) {
    pred = new BPMiss(id, sec, sname);
  } else if (strcasecmp(type, "NotTaken") == 0) {
    pred = new BPNotTaken(id, sec, sname);
  } else if (strcasecmp(type, "NotTakenEnhanced") == 0) {
    pred = new BPNotTakenEnhanced(id, sec, sname);
  } else if (strcasecmp(type, "Taken") == 0) {
    pred = new BPTaken(id, sec, sname);
  } else if (strcasecmp(type, "2bit") == 0) {
    pred = new BP2bit(id, sec, sname);
  } else if (strcasecmp(type, "2level") == 0) {
    pred = new BP2level(id, sec, sname);
  } else if (strcasecmp(type, "2BcgSkew") == 0) {
    pred = new BP2BcgSkew(id, sec, sname);
  } else if (strcasecmp(type, "Hybrid") == 0) {
    pred = new BPHybrid(id, sec, sname);
  } else if (strcasecmp(type, "yags") == 0) {
    pred = new BPyags(id, sec, sname);
  } else if (strcasecmp(type, "ogehl") == 0) {
    pred = new BPOgehl(id, sec, sname);
  } else if (strcasecmp(type, "dgp") == 0) {
    pred = new BPDGP(id, sec, sname);
  } else if (strcasecmp(type, "TAGE") == 0) {
    pred = new BPTage(id, sec, sname);
  } else if (strcasecmp(type, "imli") == 0) {
    pred = new BPIMLI(id, sec, sname);
  } else if (strcasecmp(type, "sogehl") == 0) {
    pred = new BPSOgehl(id, sec, sname);
  } else {
    MSG("BPredictor::BPredictor Invalid branch predictor type [%s] in section [%s]", type,sec);
    SescConf->notCorrect();
    return 0;
  }
  I(pred);

  return pred;
}

BPredictor::BPredictor(int32_t i, MemObj *iobj, BPredictor *bpred)
  :id(i)
  ,SMTcopy(bpred != 0)
  ,il1(iobj)
  ,nBTAC("P(%d)_BPred:nBTAC", id)
  ,nBranches("P(%d)_BPred:nBranches", id)
  ,nTaken("P(%d)_BPred:nTaken", id)
  ,nMiss("P(%d)_BPred:nMiss", id)
  ,nBranches2("P(%d)_BPred:nBranches2", id)
  ,nTaken2("P(%d)_BPred:nTaken2", id)
  ,nMiss2("P(%d)_BPred:nMiss2", id)
{
  const char *bpredSection = SescConf->getCharPtr("cpusimu","bpred",id);
  const char *bpredSection2 = 0;
  if (SescConf->checkCharPtr("cpusimu","bpred2",id)) 
    bpredSection2 = SescConf->getCharPtr("cpusimu","bpred2",id);

  BTACDelay  = SescConf->getInt(bpredSection, "BTACDelay");
  FetchWidth = SescConf->getInt("cpusimu", "fetchWidth", id);

  if (BTACDelay)
    SescConf->isBetween("cpusimu", "bpredDelay",1,BTACDelay,id);
  else
    SescConf->isBetween("cpusimu", "bpredDelay",1,1024,id);

  bpredDelay = SescConf->getInt("cpusimu", "bpredDelay",id);

  SescConf->isInt(bpredSection, "BTACDelay");
  SescConf->isBetween(bpredSection, "BTACDelay", 0, 1024);

  ras = new BPRas(id, bpredSection, "");

  // Threads in SMT system share the predictor. Only the Ras is duplicated
  if (bpred) {
    pred1 = bpred->pred1;
    pred2 = bpred->pred2;
  }else{
    pred1 = getBPred(id, bpredSection, "");
    pred2 = 0;
    if (BTACDelay!=0 && bpredSection2) {
      pred2 = getBPred(id, bpredSection2, "2");
    }
  }
}

BPredictor::~BPredictor()
{
  if (!SMTcopy) {
    delete pred1;
    if (pred2)
      delete pred2;
  }
}

void BPredictor::fetchBoundaryBegin(DInst *dinst) {
  pred1->fetchBoundaryBegin(dinst);
  if (pred2)
    pred2->fetchBoundaryBegin(dinst);
}

void BPredictor::fetchBoundaryEnd() {
  pred1->fetchBoundaryEnd();
  if (pred2)
    pred2->fetchBoundaryEnd();
}

PredType BPredictor::predict1(DInst *dinst, bool doUpdate) {
  I(dinst->getInst()->isControl());

#if 0
  printf("BPRED: pc: %x ", dinst->getPC() );
  printf(" fun call: %d, ", dinst->getInst()->isFuncCall()); printf("fun ret: %d, ", dinst->getInst()->isFuncRet());printf("taken: %d, ", dinst->isTaken());  printf("target addr: 0x%x\n", dinst->getAddr());
#endif

  nBranches.inc(doUpdate && dinst->getStatsFlag());
  nTaken.inc(dinst->isTaken() && doUpdate && dinst->getStatsFlag());

  PredType p = pred1->doPredict(dinst, doUpdate);

  nMiss.inc(p != CorrectPrediction && doUpdate && dinst->getStatsFlag());

  return p;
}

PredType BPredictor::predict2(DInst *dinst, bool doUpdate) {
  I(dinst->getInst()->isControl());

  nBranches2.inc(doUpdate && dinst->getStatsFlag());
  nTaken2.inc(dinst->isTaken() && doUpdate && dinst->getStatsFlag());
  // No RAS in L2

  PredType p = pred2->doPredict(dinst, doUpdate);

  nMiss2.inc(p != CorrectPrediction && doUpdate && dinst->getStatsFlag());

  return p;
}

TimeDelta_t BPredictor::predict(DInst *dinst, bool *fastfix) {

  *fastfix = true;

  PredType outcome1;
  PredType outcome2;
  dinst->setBiasBranch(false);

  outcome1 = ras->doPredict(dinst, true);
  if( outcome1 == NoPrediction ) {
    outcome1 = predict1(dinst,true);
    outcome2 = outcome1;
    if (pred2)
      outcome2 = predict2(dinst,true);
  }else{
    outcome2 = outcome1;
  }
  if(dinst->getInst()->isFuncRet() || dinst->getInst()->isFuncCall()) {
    dinst->setBiasBranch(true);
    ras->tryPrefetch(il1,dinst->getStatsFlag());
  }

  if (outcome1 == CorrectPrediction && outcome2 == CorrectPrediction) {
    // Both agree, and they are right
    
    if (dinst->isTaken()) {
#ifdef CLOSE_TARGET_OPTIMIZATION
      int distance = dinst->getAddr()-dinst->getPC();

      uint32_t delta= distance/FetchWidth+1;
      if (delta<bpredDelay && distance>0)
        return delta-1;
#endif
      
      return bpredDelay-1;
    }
    return 0;
  }
  if (outcome1 != CorrectPrediction && outcome2 == CorrectPrediction) {
    // Fast wrong, slow right
#ifdef CLOSE_TARGET_OPTIMIZATION
    if (dinst->isTaken()) { // outcome1 was not taken

      int distance = dinst->getAddr()-dinst->getPC();

      uint32_t delta= distance/FetchWidth+1;
      if (delta<BTACDelay && distance>0)
        return delta-1;
    }
#endif
    return BTACDelay-1;
  }

  if( dinst->getInst()->doesJump2Label() ) {
    nBTAC.inc(dinst->getStatsFlag());
    return BTACDelay-1;
  }

  *fastfix = false;

  return 1; // Anything but zero
}

void BPredictor::dump(const char *str) const
{
  // nothing?
}


