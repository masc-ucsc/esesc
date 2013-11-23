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

#include "Report.h"
#include "BPred.h"
#include "SescConf.h"

/*****************************************
 * BPred
 */

BPred::BPred(int32_t i, int32_t fetchWidth, const char *sec, const char *name)
  :id(i)
  ,nHit("P(%d)_BPRED_%s:nHit",i,name)
  ,nMiss("P(%d)_BPRED_%s:nMiss",i,name)
{
  // bpred4CycleAddrShift
  if (SescConf->checkInt(sec, "bpred4Cycle")) {
    SescConf->isPower2(sec, "bpred4Cycle");
    SescConf->isBetween(sec, "bpred4Cycle", 1, fetchWidth);
    bpred4Cycle = SescConf->getInt(sec, "bpred4Cycle");
  }else{
    bpred4Cycle = fetchWidth;
  }
  bpred4CycleAddrShift = log2i(fetchWidth/bpred4Cycle);
  I(bpred4CycleAddrShift>=0 && 
      (unsigned)bpred4CycleAddrShift<=roundUpPower2((unsigned)fetchWidth));
}

BPred::~BPred() {
}

/*****************************************
 * RAS
 */
BPRas::BPRas(int32_t i, int32_t fetchWidth, const char *section)
  :BPred(i, fetchWidth, section,"RAS")
   ,RasSize(SescConf->getInt(section,"rasSize"))
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

PredType BPRas::predict(DInst *dinst, bool doUpdate)
{
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

    if( (stack[index]&~1) == (dinst->getAddr()&~1) ) {
      return CorrectPrediction;
    }

    return MissPrediction;
  } else if(dinst->getInst()->isFuncCall() && stack) {

    if (doUpdate) {
      stack[index] = dinst->getPC()+4; // Assume always delay slot for function calls
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
BPBTB::BPBTB(int32_t i, int32_t fetchWidth,const char *section, const char *name)
  :BPred(i, fetchWidth, section, name ? name : "BTB")
{
  if( SescConf->getInt(section,"btbSize") == 0 ) {
    // Oracle
    data = 0;
    return;
  }

  data = BTBCache::create(section,"btb","P(%d)_BPRED_BTB:",i);
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

PredType BPBTB::predict(DInst *dinst, bool doUpdate)
{
  bool ntaken = !dinst->isTaken();

  if (data == 0) {
    // required when BPOracle
    nHit.inc(doUpdate && dinst->getStatsFlag());

    if (ntaken) {
      // Trash result because it shouldn't have reach BTB. Otherwise, the
      // worse the predictor, the better the results.
      return NoBTBPrediction;
    }
    return CorrectPrediction;
  }

  uint32_t key = calcHist(dinst->getPC());

  if ( ntaken || !doUpdate ) {
    // The branch is not taken. Do not update the cache
    BTBCache::CacheLine *cl = data->readLine(key);

    if( cl == 0 ) {
      nMiss.inc(doUpdate && dinst->getStatsFlag());
      return NoBTBPrediction; // NoBTBPrediction because BTAC would hide the prediction
    }

    if( cl->inst == dinst->getAddr() ) {
      nHit.inc(doUpdate && dinst->getStatsFlag());
      return CorrectPrediction;
    }

    nMiss.inc(doUpdate && dinst->getStatsFlag());
    return NoBTBPrediction;
  }

  I(doUpdate);

  // The branch is taken. Update the cache

  BTBCache::CacheLine *cl = data->fillLine(key);
  I( cl );
  
  AddrType predictID = cl->inst;
  cl->inst = dinst->getAddr();

  if( predictID == dinst->getAddr() ) {
    nHit.inc(dinst->getStatsFlag());
    return CorrectPrediction;
  }

  nMiss.inc(dinst->getStatsFlag());
  return NoBTBPrediction;
}

/*****************************************
 * BPOracle
 */

PredType BPOracle::predict(DInst *dinst, bool doUpdate) {
  if( !dinst->isTaken() )
    return CorrectPrediction; //NT
  
  return btb.predict(dinst, doUpdate);
}

/*****************************************
 * BPTaken
 */

PredType BPTaken::predict(DInst *dinst, bool doUpdate) {
  if( dinst->getInst()->isJump() || dinst->isTaken())
    return btb.predict(dinst, doUpdate);

  return MissPrediction;
}

/*****************************************
 * BPNotTaken
 */

PredType  BPNotTaken::predict(DInst *dinst, bool doUpdate) {
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

  return dinst->isTaken() ? MissPrediction : CorrectPrediction;
}

/*****************************************
 * BPNotTakenEnhaced
 */

PredType  BPNotTakenEnhanced::predict(DInst *dinst, bool doUpdate) {
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate); // backward branches predicted as taken (loops)

  if (dinst->isTaken()) {
    AddrType dest = dinst->getAddr();
    if (dest < dinst->getPC())
      return btb.predict(dinst, doUpdate); // backward branches predicted as taken (loops)

    return MissPrediction; // Forward branches predicted as not-taken, but it was taken 
  }

  return CorrectPrediction;
}

/*****************************************
 * BP2bit
 */

BP2bit::BP2bit(int32_t i, int32_t fetchWidth, const char *section)
  :BPred(i, fetchWidth, section, "2bit")
  ,btb(  i, fetchWidth, section)
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

PredType BP2bit::predict(DInst *dinst, bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

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
  
  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
}

/*****************************************
 * BP2level
 */

BP2level::BP2level(int32_t i, int32_t fetchWidth, const char *section)
  :BPred(i, fetchWidth, section,"2level")
   ,btb( i, fetchWidth, section)
   ,l1Size(SescConf->getInt(section,"l1Size"))
   ,l1SizeMask(l1Size - 1)
   ,historySize(SescConf->getInt(section,"historySize"))
   ,historyMask((1 << historySize) - 1)
   ,globalTable(section
                ,SescConf->getInt(section,"l2Size")
                ,SescConf->getInt(section,"l2Bits"))
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

  I((l1Size & (l1Size - 1)) == 0); 

  historyTable = new HistoryType[l1Size];
  I(historyTable);
}

BP2level::~BP2level()
{
  delete historyTable;
}

PredType BP2level::predict(DInst *dinst, bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

  bool taken = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  uint16_t    l1Index = iID & l1SizeMask;
  HistoryType l2Index = historyTable[l1Index];

  // update historyTable statistics
  if (doUpdate)
    historyTable[l1Index] = ((l2Index << 1) | ((iID>>2 & 1)^(taken?1:0))) & historyMask;
  
  // calculate Table possition
  l2Index = ((l2Index ^ iID) & historyMask ) | (iID<<historySize);

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

  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
}

/*****************************************
 * BPHybid
 */

BPHybrid::BPHybrid(int32_t i, int32_t fetchWidth, const char *section)
  :BPred(i, fetchWidth, section,"Hybrid")
  ,btb(  i, fetchWidth, section)
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

PredType BPHybrid::predict(DInst *dinst, bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

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

  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
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

BP2BcgSkew::BP2BcgSkew(int32_t i, int32_t fetchWidth, const char *section)
  : BPred(i, fetchWidth, section,"2BcgSkew")
  ,btb(   i, fetchWidth, section)
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


PredType BP2BcgSkew::predict(DInst *dinst, bool doUpdate)
{
  if (dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate);

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

  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
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

BPyags::BPyags(int32_t i, int32_t fetchWidth, const char *section)
  :BPred(i, fetchWidth, section, "yags")
  ,btb(  i, fetchWidth, section)
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

PredType BPyags::predict(DInst *dinst,bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

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
  
  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
}

/*****************************************
 * BPOgehl
 *
 * Based on "The O-GEHL Branch Predictor" by Andre Seznec
 * Code ported from Andre's CBP 2004 entry by Jay Boice (boice@soe.ucsc.edu)
 * 
 */
 
BPOgehl::BPOgehl(int32_t i, int32_t fetchWidth,const char *section)
  :BPred(i, fetchWidth, section, "ogehl")
  ,btb(  i, fetchWidth, section)
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
  ghist = new long long[(glength >> 6) + 1];
  MINITAG = new char[(1 << (logpred - 1))];
  
  for (int32_t i = 0; i < (glength >> 6) + 1; i++)
    ghist[i] = 0;

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

PredType BPOgehl::predict(DInst *dinst, bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

  bool taken = dinst->isTaken();
  bool ptaken = false;

  int32_t S = (mtables/2);
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
      if ((iID[mtables - 1] & 1) == 0) {
        if (taken != ptaken) {
          miniTag = MINITAG[iID[mtables - 1] >> 1];
          if (miniTag != ((int)(dinst->getPC() & 1))) {
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
        MINITAG[iID[mtables - 1] >> 1] = (char) (dinst->getPC() & 1);
      }
    }
  
    // Update branch/path histories
    for (int32_t i = (glength >> 6); i > 0; i--)
      ghist[i] = (ghist[i] << 1) + (ghist[i - 1] < 0);
    ghist[0] = ghist[0] << 1;
    if (taken)
      ghist[0]++;
  }

  if (taken != ptaken) {
    if (doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }
  
  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
}


int32_t BPOgehl::geoidx(long long Add, long long *histo, int32_t m, int32_t funct)
{
  long long inter, Hh, Res;
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
 * BPSOgehl  ; SCOORE Adaptation of the OGELH predictor
 *
 * Based on "The O-GEHL Branch Predictor" by Andre Seznec
 * 
 */

#if 1
BPSOgehl::BPSOgehl(int32_t i, int32_t fetchWidth,const char *section)
  :BPred(i, fetchWidth, section, "sogehl")
  ,btb(  i, fetchWidth, section)
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
  MINITAG = new char[(1 << (logtsize - 1))];
  
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

PredType BPSOgehl::predict(DInst *dinst, bool doUpdate)
{
  if( dinst->getInst()->isJump() )
    return btb.predict(dinst, doUpdate);

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
          if (miniTag != ((int)(dinst->getPC() & 1))) {
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
        MINITAG[iID[mtables - 1] >> 1] = (char) (dinst->getPC() & 1);
      }
    }
  
    // Update branch/path histories
    ghr <<=1;
    ghr[0] = taken;
  }

  if (taken != ptaken)
    return MissPrediction;
  
  return ptaken ? btb.predict(dinst, doUpdate) : CorrectPrediction;
}

uint32_t BPSOgehl::geoidx2(long long Add, int32_t m)
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
/*****************************************
 * BPredictor
 */


BPred *BPredictor::getBPred(int32_t id, int32_t fetchWidth, const char *sec)
{
  BPred *pred=0;
  
  const char *type = SescConf->getCharPtr(sec, "type");

  // Normal Predictor
  if (strcasecmp(type, "oracle") == 0) {
    pred = new BPOracle(id, fetchWidth, sec);
  } else if (strcasecmp(type, "NotTaken") == 0) {
    pred = new BPNotTaken(id, fetchWidth, sec);
  } else if (strcasecmp(type, "NotTakenEnhanced") == 0) {
    pred = new BPNotTakenEnhanced(id, fetchWidth, sec);
  } else if (strcasecmp(type, "Taken") == 0) {
    pred = new BPTaken(id, fetchWidth, sec);
  } else if (strcasecmp(type, "2bit") == 0) {
    pred = new BP2bit(id, fetchWidth, sec);
  } else if (strcasecmp(type, "2level") == 0) {
    pred = new BP2level(id, fetchWidth, sec);
  } else if (strcasecmp(type, "2BcgSkew") == 0) {
    pred = new BP2BcgSkew(id, fetchWidth, sec);
  } else if (strcasecmp(type, "Hybrid") == 0) {
    pred = new BPHybrid(id, fetchWidth, sec);
  } else if (strcasecmp(type, "yags") == 0) {
    pred = new BPyags(id, fetchWidth, sec);
  } else if (strcasecmp(type, "ogehl") == 0) {
    pred = new BPOgehl(id, fetchWidth, sec);
  } else if (strcasecmp(type, "sogehl") == 0) {
    pred = new BPSOgehl(id, fetchWidth, sec);
  } else {
    MSG("BPredictor::BPredictor Invalid branch predictor type [%s] in section [%s]", type,sec);
    SescConf->notCorrect();
    return 0;
  }
  I(pred);

  return pred;
}

BPredictor::BPredictor(int32_t i, int32_t fetchWidth, const char *sec, BPredictor *bpred)
  :id(i)
  ,SMTcopy(bpred != 0)
  ,ras(i, fetchWidth, sec)
  ,nBranches("P(%d)_BPred:nBranches", i)
  ,nTaken("P(%d)_BPred:nTaken", i)
  ,nMiss("P(%d)_BPred:nMiss", i)
  ,section(strdup(sec ? sec : "null" ))
{

  // Threads in SMT system share the predictor. Only the Ras is duplicated
  if (bpred)
    pred = bpred->pred;
  else
    pred = getBPred(id, fetchWidth, section);
}

BPredictor::~BPredictor()
{
  dump(section);

  if (!SMTcopy)
    delete pred;
    
  free(section);
}

void BPredictor::dump(const char *str) const
{
  // nothing?
}
