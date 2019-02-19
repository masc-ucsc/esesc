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

#include <alloca.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <iostream>
#include <ios>
#include <fstream>

#include "BPred.h"
#include "IMLIBest.h"
#include "MemObj.h"
#include "Report.h"
#include "SescConf.h"

extern "C" uint64_t esesc_mem_read(uint64_t addr);

//#define CLOSE_TARGET_OPTIMIZATION 1

/*****************************************
 * BPred
 */

BPred::BPred(int32_t i, const char *sec, const char *sname, const char *name)
    : id(i)
    , nHit("P(%d)_BPred%s_%s:nHit", i, sname, name)
    , nMiss("P(%d)_BPred%s_%s:nMiss", i, sname, name) {
  if(SescConf->checkInt(sec, "addrShift")) {
    SescConf->isBetween(sec, "addrShift", 0, 8);
    addrShift = SescConf->getInt(sec, "addrShift");
  } else {
    addrShift = 0;
  }
  maxCores = SescConf->getRecordSize("", "cpuemul");
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
    : BPred(i, section, sname, "RAS")
    , RasSize(SescConf->getInt(section, "rasSize"))
    , rasPrefetch(SescConf->getInt(section, "rasPrefetch")) {
  // Constraints
  SescConf->isInt(section, "rasSize");
  SescConf->isBetween(section, "rasSize", 0, 128); // More than 128???

  if(RasSize == 0) {
    stack = 0;
    return;
  }

  stack = new AddrType[RasSize];
  I(stack);

  index = 0;
}

BPRas::~BPRas() {
  delete stack;
}

void BPRas::tryPrefetch(MemObj *il1, bool doStats, int degree) {

  if(rasPrefetch == 0)
    return;

  for(int j = 0; j < rasPrefetch; j++) {
    int i = index - j - 1;
    while(i < 0)
      i = RasSize - 1;

    il1->tryPrefetch(stack[i], doStats, degree, PSIGN_RAS, PSIGN_RAS);
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

    if(doUpdate) {
      index--;
      if(index < 0)
        index = RasSize - 1;
    }

    uint64_t phase=0;
    for(int i=0;i<4;i++) {
      int pos = index - i;
      if (pos<0)
        pos = RasSize - 1;
      phase = (phase>>3) ^ stack[index];
    }

#ifdef USE_DOLC
    //idolc.setPhase(phase);
#endif

    if(stack[index] == dinst->getAddr() || (stack[index] + 4) == dinst->getAddr() || (stack[index] + 2) == dinst->getAddr()) {
      // MSG("RET  %llx -> %llx  (stack=%llx) good",dinst->getPC(),dinst->getAddr(), stack[index]);
      return CorrectPrediction;
    }
    // MSG("RET  %llx -> %llx  (stack=%llx) miss",dinst->getPC(),dinst->getAddr(), stack[index]);

    return MissPrediction;
  } else if(dinst->getInst()->isFuncCall() && stack) {

    // MSG("CALL %llx -> %llx  (stack=%llx)",dinst->getPC(),dinst->getAddr(), stack[index]);

    if(doUpdate) {
      stack[index] = dinst->getPC();
      index++;

      if(index >= RasSize)
        index = 0;
    }
  }

  return NoPrediction;
}

/*****************************************
 * BTB
 */
BPBTB::BPBTB(int32_t i, const char *section, const char *sname, const char *name)
    : BPred(i, section, sname, name ? name : "BTB")
    , nHitLabel("P(%d)_BPred%s_%s:nHitLabel", i, sname, name ? name : "BTB") {
  if(SescConf->checkInt(section, "btbHistorySize"))
    btbHistorySize = SescConf->getInt(section, "btbHistorySize");
  else
    btbHistorySize = 0;

  if(btbHistorySize)
    dolc = new DOLC(btbHistorySize + 1, 5, 2, 2);
  else
    dolc = 0;

  if(SescConf->checkBool(section, "btbicache"))
    btbicache = SescConf->getBool(section, "btbicache");
  else
    btbicache = false;

  if(SescConf->getInt(section, "btbSize") == 0) {
    // Oracle
    data = 0;
    return;
  }

  data = BTBCache::create(section, "btb", "P(%d)_BPred%s_BTB:", i, sname);
  I(data);
}

BPBTB::~BPBTB() {
  if(data)
    data->destroy();
}

void BPBTB::updateOnly(DInst *dinst) {
  if(data == 0 || !dinst->isTaken())
    return;

  uint32_t key = calcHist(dinst->getPC());

  BTBCache::CacheLine *cl = data->fillLine(key, 0xdeaddead);
  I(cl);

  cl->inst = dinst->getAddr();
}

PredType BPBTB::predict(DInst *dinst, bool doUpdate, bool doStats) {
  bool ntaken = !dinst->isTaken();

  if(data == 0) {
    // required when BPOracle
    if(dinst->getInst()->doesCtrl2Label())
      nHitLabel.inc(doUpdate && dinst->getStatsFlag() && doStats);
    else
      nHit.inc(doUpdate && dinst->getStatsFlag() && doStats);

    if(ntaken) {
      // Trash result because it shouldn't have reach BTB. Otherwise, the
      // worse the predictor, the better the results.
      return NoBTBPrediction;
    }
    return CorrectPrediction;
  }

  uint32_t key = calcHist(dinst->getPC());

  if(dolc) {
    if(doUpdate)
      dolc->update(dinst->getPC());

    key ^= dolc->getSign(btbHistorySize, btbHistorySize);
  }

  if(ntaken || !doUpdate) {
    // The branch is not taken. Do not update the cache
    if(dinst->getInst()->doesCtrl2Label() && btbicache) {
      nHitLabel.inc(doStats && doUpdate && dinst->getStatsFlag());
      return NoBTBPrediction;
    }

    BTBCache::CacheLine *cl = data->readLine(key);

    if(cl == 0) {
      nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
      return NoBTBPrediction; // NoBTBPrediction because BTAC would hide the prediction
    }

    if(cl->inst == dinst->getAddr()) {
      nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
      return CorrectPrediction;
    }

    nMiss.inc(doStats && doUpdate && dinst->getStatsFlag());
    return MissPrediction;
  }

  I(doUpdate);

  // The branch is taken. Update the cache

  if(dinst->getInst()->doesCtrl2Label() && btbicache) {
    nHitLabel.inc(doStats && doUpdate && dinst->getStatsFlag());
    return CorrectPrediction;
  }

  BTBCache::CacheLine *cl = data->fillLine(key, 0xdeaddead);
  I(cl);

  AddrType predictID = cl->inst;
  cl->inst           = dinst->getAddr();

  if(predictID == dinst->getAddr()) {
    nHit.inc(doStats && doUpdate && dinst->getStatsFlag());
    // MSG("hit :%llx -> %llx",dinst->getPC(), dinst->getAddr());
    return CorrectPrediction;
  }

  // MSG("miss:%llx -> %llx (%llx)",dinst->getPC(), dinst->getAddr(), predictID);
  nMiss.inc(doStats && doUpdate && dinst->getStatsFlag());
  return NoBTBPrediction;
}

/*****************************************
 * BPOracle
 */

PredType BPOracle::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(!dinst->isTaken())
    return CorrectPrediction; // NT

  return btb.predict(dinst, doUpdate, doStats);
}

/*****************************************
 * BPTaken
 */

PredType BPTaken::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump() || dinst->isTaken())
    return btb.predict(dinst, doUpdate, doStats);

  PredType p = btb.predict(dinst, false, doStats);

  if(p == CorrectPrediction)
    return CorrectPrediction; // NotTaken and BTB empty

  return MissPrediction;
}

/*****************************************
 * BPNotTaken
 */

PredType BPNotTaken::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  return dinst->isTaken() ? MissPrediction : CorrectPrediction;
}

/*****************************************
 * BPMiss
 */

PredType BPMiss::predict(DInst *dinst, bool doUpdate, bool doStats) {
  return MissPrediction;
}

/*****************************************
 * BPNotTakenEnhaced
 */

PredType BPNotTakenEnhanced::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats); // backward branches predicted as taken (loops)

  if(dinst->isTaken()) {
    AddrType dest = dinst->getAddr();
    if(dest < dinst->getPC())
      return btb.predict(dinst, doUpdate, doStats); // backward branches predicted as taken (loops)

    return MissPrediction; // Forward branches predicted as not-taken, but it was taken
  }

  return CorrectPrediction;
}

/*****************************************
 * BP2bit
 */

BP2bit::BP2bit(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "2bit")
    , btb(i, section, sname)
    , table(section, SescConf->getInt(section, "size"), SescConf->getInt(section, "bits")) {
  // Constraints
  SescConf->isInt(section, "size");
  SescConf->isPower2(section, "size");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "bits", 1, 7);

  // Done
}

PredType BP2bit::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool taken = dinst->isTaken();
  
  uint64_t pc = dinst->getPC();
  //uint64_t raw_op = esesc_mem_read(pc);
  //checking if br data is working fine
  //MSG("pc=%llx raw_op=%llx data1=%llx data2=%llx taken=%d",pc, raw_op, dinst->getBrData1(), dinst->getBrData2(), taken);

  bool ptaken;
  if(doUpdate)
    ptaken = table.predict(calcHist(dinst->getPC()), taken);
  else
    ptaken = table.predict(calcHist(dinst->getPC()));

#if 0
  if (dinst->getStatsFlag())
    printf("bp2bit %llx %llx %s\n",dinst->getPC(), calcHist(dinst->getPC()) & 32767,taken?"T":"NT");
#endif

  if(taken != ptaken) {
    if(doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPTLdbp
 */

BPLdbp::BPLdbp(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "ldbp")
    , btb(i, section, sname)
    , ldbp_table(section, SescConf->getInt(section, "size"), SescConf->getInt(section, "bits")) {
  // Constraints
  SescConf->isInt(section, "size");
  SescConf->isPower2(section, "size");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "bits", 1, 7);

  // Done
}

PredType BPLdbp::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool     taken  = dinst->isTaken();
  AddrType br_pc = dinst->getPC();
  bool     ptaken;
  AddrType t_tag = dinst->getLDPC() ^ (br_pc << 7) ^ (dinst->getDataSign() << 10);
  BrOpType br_op = branch_type(br_pc);
  uint64_t raw_op = esesc_mem_read(br_pc); 

  //FIXME - add br opcode conditions for ldbp to perform prediction
  //MSG("ldbp1 brpc=%llx ldpc=%llx br_opcode=%llx br_op=%d d1=%u d2=%u correct_pred=%d is_pred=%d",dinst->getPC(),dinst->getLDPC(),
  //    (raw_op & 3),br_op,dinst->getBrData1(),dinst->getBrData2(),
  //    outcome_calculator(br_op, dinst->getBrData1(), dinst->getBrData2())==taken,dinst->is_br_ld_chain_predictable());
  if(dinst->is_br_ld_chain_predictable()) {
    ptaken = outcome_calculator(br_op, dinst->getBrData1(), dinst->getBrData2());
    auto it = ldbp_map.find(t_tag);
    if(it == ldbp_map.end()) { //if tag not found in ldbp_map
      ldbp_map.clear();
      ldbp_map[t_tag] = ptaken;
      return NoPrediction;
    }
    ldbp_map[t_tag] = ptaken;
  }else {
    return NoPrediction;
  }

  if(taken != ptaken) {
    if(doUpdate)
      btb.updateOnly(dinst);
    // tDataTable.update(t_tag, taken); //update needed here?
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

BrOpType BPLdbp::branch_type(AddrType br_pc) {
  uint64_t raw_op = esesc_mem_read(br_pc);
  uint8_t br_opcode = raw_op & 3;
  uint8_t get_br_bits = (raw_op >> 12) & 7;  //extract bits 12 to 14 to get Br Type
  if(br_opcode == 3) {
    switch(get_br_bits){
      case BEQ:
        return BEQ;
      case BNE:
        return BNE;
      case BLT:
        return BLT;
      case BGE:
        return BGE;
      case BLTU:
        return BLTU;
      case BGEU:
        return BGEU;
      default:
        MSG("ILLEGAL_BR=%llx OP_TYPE:%u", br_pc, get_br_bits);
        break;
    }
  }else if(br_opcode == 1) {
    if(get_br_bits == 4 || get_br_bits == 5){
      return BEQ;
    }else if(get_br_bits == 6 || get_br_bits == 7) {
      return BNE;
    }
  }
  return ILLEGAL_BR;
}

bool BPLdbp::outcome_calculator(BrOpType br_op, uint64_t br_data1, uint64_t br_data2) {
  if(br_op == BEQ) {
    if(br_data1 == br_data2)
      return 1;
    return 0;
  }else if(br_op == BNE) {
    if(br_data1 != br_data2)
      return 1;
    return 0;
  }else if(br_op == BLT || br_op == BLTU) {
    if(br_data1 < br_data2)
      return 1;
    return 0;
  }else if(br_op == BGE || br_op == BGEU) {
    if(br_data1 >= br_data2)
      return 1;
    return 0;
  }

  return 0; //default is "not taken"

}


/*****************************************
 * BPTData
 */

BPTData::BPTData(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "tdata")
    , btb(i, section, sname)
    , tDataTable(section, SescConf->getInt(section, "size"), SescConf->getInt(section, "bits")) {
  // Constraints
  SescConf->isInt(section, "size");
  SescConf->isPower2(section, "size");
  SescConf->isGT(section, "size", 1);

  SescConf->isBetween(section, "bits", 1, 7);

  // Done
}

PredType BPTData::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool     taken  = dinst->isTaken();
  AddrType old_pc = dinst->getPC();
  bool     ptaken;
  // printf("TDATA pc:%llx ldpc:%llx ds:%d\n", dinst->getPC(), dinst->getLDPC(), dinst->getDataSign());
  // AddrType t_tag = dinst->getLDPC() ^ (old_pc<<7) ^ (old_pc>>3) ^ (dinst->getDataSign()<<10) ^ (dinst->getDataSign()<<2);
  AddrType t_tag = dinst->getLDPC() ^ (old_pc << 7) ^ (dinst->getDataSign() << 10);
  // AddrType t_tag = dinst->getPC() ^ dinst->getLDPC() ^ dinst->getDataSign();

  if(doUpdate)
    ptaken = tDataTable.predict(t_tag, taken);
  else
    ptaken = tDataTable.predict(t_tag);

  if(!tDataTable.isLowest(t_tag) && !tDataTable.isHighest(t_tag))
    return NoPrediction; // Only if Highly confident

  if(taken != ptaken) {
    if(doUpdate)
      btb.updateOnly(dinst);
    // tDataTable.update(t_tag, taken); //update needed here?
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPIMLI: SC-TAGE-L with IMLI from Seznec Micro paper
 */

BPIMLI::BPIMLI(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "imli")
    , btb(i, section, sname)
    , FetchPredict(SescConf->getBool(section, "FetchPredict")) {

  int bimodalSize = SescConf->getInt(section, "bimodalSize");
  SescConf->isInt(section, "bimodalSize");
  SescConf->isGT(section, "bimodalSize", 1);
  SescConf->isPower2(section, "bimodalSize");

  int FetchWidth = SescConf->getInt("cpusimu", "fetchWidth");
  SescConf->isInt("cpusimu", "fetchWidth");
  SescConf->isGT("cpusimu", "fetchWidth", 1);
  SescConf->isPower2("cpusimu", "fetchWidth");

  int log2fetchwidth = log2(FetchWidth);
  int blogb          = log2(bimodalSize) - log2(FetchWidth);

  int bwidth = SescConf->getInt(section, "bimodalWidth");
  SescConf->isGT(section, "bimodalWidth", 1);

  int nhist = SescConf->getInt(section, "nhist");
  SescConf->isGT(section, "nhist", 1);

  bool statcorrector = SescConf->getBool(section, "statcorrector");
  if(SescConf->checkBool(section, "dataHistory")) {
    dataHistory = SescConf->getBool(section, "dataHistory");
  } else {
    dataHistory = false;
  }

  imli = new IMLIBest(log2fetchwidth, blogb, bwidth, nhist, statcorrector);
}

void BPIMLI::fetchBoundaryBegin(DInst *dinst) {
  if(FetchPredict)
    imli->fetchBoundaryBegin(dinst->getPC());
}

void BPIMLI::fetchBoundaryEnd() {
  if(FetchPredict)
    imli->fetchBoundaryEnd();
}

PredType BPIMLI::predict(DInst *dinst, bool doUpdate, bool doStats) {

  if(dinst->getInst()->isJump() || dinst->getInst()->isFuncRet()) {
    imli->TrackOtherInst(dinst->getPC(), dinst->getInst()->getOpcode(), dinst->getAddr());
    dinst->setBiasBranch(true);
    return btb.predict(dinst, doUpdate, doStats);
  }

  bool taken = dinst->isTaken();

  if(!FetchPredict)
    imli->fetchBoundaryBegin(dinst->getPC());

  bool     bias;
  AddrType pc = dinst->getPC();
  bool ptaken = imli->getPrediction(pc, bias); // pass taken for statistics
  dinst->setBiasBranch(bias);

  if(doUpdate) {
    imli->updatePredictor(pc, taken, ptaken, dinst->getAddr());
  }

  if(!FetchPredict)
    imli->fetchBoundaryEnd();

  if(taken != ptaken) {
    //MSG("0x%llx t:%d p%d",dinst->getPC(), taken,ptaken);
    if(doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BP2level
 */

BP2level::BP2level(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "2level")
    , btb(i, section, sname)
    , l1Size(SescConf->getInt(section, "l1Size"))
    , l1SizeMask(l1Size - 1)
    , historySize(SescConf->getInt(section, "historySize"))
    , historyMask((1 << historySize) - 1)
    , globalTable(section, SescConf->getInt(section, "l2Size"), SescConf->getInt(section, "l2Bits"))
    , dolc(SescConf->getInt(section, "historySize"), 5, 2, 2) {
  // Constraints
  SescConf->isInt(section, "l1Size");
  SescConf->isPower2(section, "l1Size");
  SescConf->isBetween(section, "l1Size", 0, 32768);

  SescConf->isInt(section, "historySize");
  SescConf->isBetween(section, "historySize", 1, 63);

  SescConf->isInt(section, "l2Size");
  SescConf->isPower2(section, "l2Size");
  SescConf->isBetween(section, "l2Bits", 1, 7);

  if(SescConf->checkBool(section, "useDolc")) {
    useDolc = SescConf->checkBool(section, "useDolc");
  } else {
    useDolc = false;
  }

  I((l1Size & (l1Size - 1)) == 0);

  historyTable = new HistoryType[l1Size * maxCores];
  I(historyTable);
}

BP2level::~BP2level() {
  delete historyTable;
}

PredType BP2level::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump()) {
    if(useDolc)
      dolc.update(dinst->getPC());
    return btb.predict(dinst, doUpdate, doStats);
  }

  bool        taken   = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  uint16_t    l1Index = iID & l1SizeMask;
  l1Index             = l1Index + dinst->getFlowId() * l1Size;
  I(l1Index < (maxCores * l1Size));

  HistoryType l2Index = historyTable[l1Index];

  if(useDolc)
    dolc.update(dinst->getPC());

  // update historyTable statistics
  if(doUpdate) {
    HistoryType nhist = 0;
    if(useDolc)
      nhist = dolc.getSign(historySize, historySize);
    else
      nhist = ((l2Index << 1) | ((iID >> 2 & 1) ^ (taken ? 1 : 0))) & historyMask;
    historyTable[l1Index] = nhist;
  }

  // calculate Table possition
  l2Index = ((l2Index ^ iID) & historyMask) | (iID << historySize);

  if(useDolc && taken)
    dolc.update(dinst->getAddr());

  bool ptaken;
  if(doUpdate)
    ptaken = globalTable.predict(l2Index, taken);
  else
    ptaken = globalTable.predict(l2Index);

  if(taken != ptaken) {
    if(doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

/*****************************************
 * BPHybid
 */

BPHybrid::BPHybrid(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "Hybrid")
    , btb(i, section, sname)
    , historySize(SescConf->getInt(section, "historySize"))
    , historyMask((1 << historySize) - 1)
    , globalTable(section, SescConf->getInt(section, "l2Size"), SescConf->getInt(section, "l2Bits"))
    , ghr(0)
    , localTable(section, SescConf->getInt(section, "localSize"), SescConf->getInt(section, "localBits"))
    , metaTable(section, SescConf->getInt(section, "MetaSize"), SescConf->getInt(section, "MetaBits"))

{
  // Constraints
  SescConf->isInt(section, "localSize");
  SescConf->isPower2(section, "localSize");
  SescConf->isBetween(section, "localBits", 1, 7);

  SescConf->isInt(section, "MetaSize");
  SescConf->isPower2(section, "MetaSize");
  SescConf->isBetween(section, "MetaBits", 1, 7);

  SescConf->isInt(section, "historySize");
  SescConf->isBetween(section, "historySize", 1, 63);

  SescConf->isInt(section, "l2Size");
  SescConf->isPower2(section, "l2Size");
  SescConf->isBetween(section, "l2Bits", 1, 7);
}

BPHybrid::~BPHybrid() {
}

PredType BPHybrid::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool        taken   = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  HistoryType l2Index = ghr;

  // update historyTable statistics
  if(doUpdate) {
    ghr = ((ghr << 1) | ((iID >> 2 & 1) ^ (taken ? 1 : 0))) & historyMask;
  }

  // calculate Table possition
  l2Index = ((l2Index ^ iID) & historyMask) | (iID << historySize);

  bool globalTaken;
  bool localTaken;
  if(doUpdate) {
    globalTaken = globalTable.predict(l2Index, taken);
    localTaken  = localTable.predict(iID, taken);
  } else {
    globalTaken = globalTable.predict(l2Index);
    localTaken  = localTable.predict(iID);
  }

  bool metaOut;
  if(!doUpdate) {
    metaOut = metaTable.predict(l2Index); // do not update meta
  } else if(globalTaken == taken && localTaken != taken) {
    // global is correct, local incorrect
    metaOut = metaTable.predict(l2Index, false);
  } else if(globalTaken != taken && localTaken == taken) {
    // global is incorrect, local correct
    metaOut = metaTable.predict(l2Index, true);
  } else {
    metaOut = metaTable.predict(l2Index); // do not update meta
  }

  bool ptaken = metaOut ? localTaken : globalTaken;

  if(taken != ptaken) {
    if(doUpdate)
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
    : BPred(i, section, sname, "2BcgSkew")
    , btb(i, section, sname)
    , BIM(section, SescConf->getInt(section, "BIMSize"))
    , G0(section, SescConf->getInt(section, "G0Size"))
    , G0HistorySize(SescConf->getInt(section, "G0HistorySize"))
    , G0HistoryMask((1 << G0HistorySize) - 1)
    , G1(section, SescConf->getInt(section, "G1Size"))
    , G1HistorySize(SescConf->getInt(section, "G1HistorySize"))
    , G1HistoryMask((1 << G1HistorySize) - 1)
    , metaTable(section, SescConf->getInt(section, "MetaSize"))
    , MetaHistorySize(SescConf->getInt(section, "MetaHistorySize"))
    , MetaHistoryMask((1 << MetaHistorySize) - 1) {
  // Constraints
  SescConf->isInt(section, "BIMSize");
  SescConf->isPower2(section, "BIMSize");
  SescConf->isGT(section, "BIMSize", 1);

  SescConf->isInt(section, "G0Size");
  SescConf->isPower2(section, "G0Size");
  SescConf->isGT(section, "G0Size", 1);

  SescConf->isInt(section, "G0HistorySize");
  SescConf->isBetween(section, "G0HistorySize", 1, 63);

  SescConf->isInt(section, "G1Size");
  SescConf->isPower2(section, "G1Size");
  SescConf->isGT(section, "G1Size", 1);

  SescConf->isInt(section, "G1HistorySize");
  SescConf->isBetween(section, "G1HistorySize", 1, 63);

  SescConf->isInt(section, "MetaSize");
  SescConf->isPower2(section, "MetaSize");
  SescConf->isGT(section, "MetaSize", 1);

  SescConf->isInt(section, "MetaHistorySize");
  SescConf->isBetween(section, "MetaHistorySize", 1, 63);

  history = 0x55555555;
}

BP2BcgSkew::~BP2BcgSkew() {
  // Nothing?
}

PredType BP2BcgSkew::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  HistoryType iID = calcHist(dinst->getPC());

  bool taken = dinst->isTaken();

  HistoryType xorKey1 = history ^ iID;
  HistoryType xorKey2 = history ^ (iID >> 2);
  HistoryType xorKey3 = history ^ (iID >> 4);

  HistoryType metaIndex = (xorKey1 & MetaHistoryMask) | iID << MetaHistorySize;
  HistoryType G0Index   = (xorKey2 & G0HistoryMask) | iID << G0HistorySize;
  HistoryType G1Index   = (xorKey3 & G1HistoryMask) | iID << G1HistorySize;

  bool metaOut = metaTable.predict(metaIndex);

  bool BIMOut = BIM.predict(iID);
  bool G0Out  = G0.predict(G0Index);
  bool G1Out  = G1.predict(G1Index);

  bool gskewOut = (G0Out ? 1 : 0) + (G1Out ? 1 : 0) + (BIMOut ? 1 : 0) >= 2;

  bool ptaken = metaOut ? BIMOut : gskewOut;

  if(ptaken != taken) {
    if(!doUpdate)
      return MissPrediction;

    BIM.predict(iID, taken);
    G0.predict(G0Index, taken);
    G1.predict(G1Index, taken);

    BIMOut = BIM.predict(iID);
    G0Out  = G0.predict(G0Index);
    G1Out  = G1.predict(G1Index);

    gskewOut = (G0Out ? 1 : 0) + (G1Out ? 1 : 0) + (BIMOut ? 1 : 0) >= 2;
    if(BIMOut != gskewOut)
      metaTable.predict(metaIndex, (BIMOut == taken));
    else
      metaTable.reset(metaIndex, (BIMOut == taken));

    I(doUpdate);
    btb.updateOnly(dinst);
    return MissPrediction;
  }

  if(doUpdate) {
    if(metaOut) {
      BIM.predict(iID, taken);
    } else {
      if(BIMOut == taken)
        BIM.predict(iID, taken);
      if(G0Out == taken)
        G0.predict(G0Index, taken);
      if(G1Out == taken)
        G1.predict(G1Index, taken);
    }

    if(BIMOut != gskewOut)
      metaTable.predict(metaIndex, (BIMOut == taken));

    history = history << 1 | ((iID >> 2 & 1) ^ (taken ? 1 : 0));
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
    : BPred(i, section, sname, "yags")
    , btb(i, section, sname)
    , historySize(24)
    , historyMask((1 << 24) - 1)
    , table(section, SescConf->getInt(section, "size"), SescConf->getInt(section, "bits"))
    , ctableTaken(section, SescConf->getInt(section, "l1size"), SescConf->getInt(section, "l1bits"))
    , ctableNotTaken(section, SescConf->getInt(section, "l2size"), SescConf->getInt(section, "l2bits")) {
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

  CacheTaken        = new uint8_t[SescConf->getInt(section, "l1size")];
  CacheTakenMask    = SescConf->getInt(section, "l1size") - 1;
  CacheTakenTagMask = (1 << SescConf->getInt(section, "tagbits")) - 1;

  CacheNotTaken        = new uint8_t[SescConf->getInt(section, "l2size")];
  CacheNotTakenMask    = SescConf->getInt(section, "l2size") - 1;
  CacheNotTakenTagMask = (1 << SescConf->getInt(section, "tagbits")) - 1;

  // Done
}

BPyags::~BPyags() {
}

PredType BPyags::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool        taken   = dinst->isTaken();
  HistoryType iID     = calcHist(dinst->getPC());
  HistoryType iIDHist = ghr;
  bool        choice;
  if(doUpdate) {
    ghr    = ((ghr << 1) | ((iID >> 2 & 1) ^ (taken ? 1 : 0))) & historyMask;
    choice = table.predict(iID, taken);
  } else
    choice = table.predict(iID);

  iIDHist = ((iIDHist ^ iID) & historyMask) | (iID << historySize);

  bool ptaken;
  if(choice) {
    ptaken = true;

    // Search the not taken cache. If we find an entry there, the
    // prediction from the cache table will override the choice table.

    HistoryType cacheIndex = iIDHist & CacheNotTakenMask;
    HistoryType tag        = iID & CacheNotTakenTagMask;
    bool        cacheHit   = (CacheNotTaken[cacheIndex] == tag);

    if(cacheHit) {
      if(doUpdate) {
        CacheNotTaken[cacheIndex] = tag;
        ptaken                    = ctableNotTaken.predict(iIDHist, taken);
      } else {
        ptaken = ctableNotTaken.predict(iIDHist);
      }
    } else if((doUpdate) && (taken == false)) {
      CacheNotTaken[cacheIndex] = tag;
      (void)ctableNotTaken.predict(iID, taken);
    }
  } else {
    ptaken = false;
    // Search the taken cache. If we find an entry there, the prediction
    // from the cache table will override the choice table.

    HistoryType cacheIndex = iIDHist & CacheTakenMask;
    HistoryType tag        = iID & CacheTakenTagMask;
    bool        cacheHit   = (CacheTaken[cacheIndex] == tag);

    if(cacheHit) {
      if(doUpdate) {
        CacheTaken[cacheIndex] = tag;
        ptaken                 = ctableTaken.predict(iIDHist, taken);
      } else {
        ptaken = ctableTaken.predict(iIDHist);
      }
    } else if((doUpdate) && (taken == true)) {
      CacheTaken[cacheIndex] = tag;
      (void)ctableTaken.predict(iIDHist, taken);
      ptaken = false;
    }
  }

  if(taken != ptaken) {
    if(doUpdate)
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

BPOgehl::BPOgehl(int32_t i, const char *section, const char *sname)
    : BPred(i, section, sname, "ogehl")
    , btb(i, section, sname)
    , mtables(SescConf->getInt(section, "mtables"))
    , glength(SescConf->getInt(section, "glength"))
    , nentry(3)
    , addwidth(8)
    , logpred(log2i(SescConf->getInt(section, "tsize")))
    , THETA(SescConf->getInt(section, "mtables"))
    , MAXTHETA(31)
    , THETAUP(1 << (SescConf->getInt(section, "tcbits") - 1))
    , PREDUP(1 << (SescConf->getInt(section, "tbits") - 1))
    , TC(0) {
  SescConf->isInt(section, "tsize");
  SescConf->isPower2(section, "tsize");
  SescConf->isGT(section, "tsize", 1);
  SescConf->isBetween(section, "tbits", 1, 15);
  SescConf->isBetween(section, "tcbits", 1, 15);
  SescConf->isBetween(section, "mtables", 3, 32);

  pred = new char *[mtables];
  for(int32_t i = 0; i < mtables; i++) {
    pred[i] = new char[1 << logpred];
    for(int32_t j = 0; j < (1 << logpred); j++)
      pred[i][j] = 0;
  }

  T       = new int[nentry * logpred + 1];
  ghist   = new int64_t[(glength >> 6) + 1];
  MINITAG = new uint8_t[(1 << (logpred - 1))];

  for(int32_t i = 0; i < (glength >> 6) + 1; i++) {
    ghist[i] = 0;
  }

  for(int32_t j = 0; j < (1 << (logpred - 1)); j++)
    MINITAG[j] = 0;
  AC = 0;

  double initset = 3;
  double tt      = ((double)glength) / initset;
  double Pow     = pow(tt, 1.0 / (mtables + 1));

  histLength     = new int[mtables + 3];
  usedHistLength = new int[mtables];
  histLength[0]  = 0;
  histLength[1]  = 3;
  for(int32_t i = 2; i < mtables + 3; i++)
    histLength[i] = (int)((initset * pow(Pow, (double)(i - 1))) + 0.5);
  for(int32_t i = 0; i < mtables; i++) {
    usedHistLength[i] = histLength[i];
  }
}

BPOgehl::~BPOgehl() {
}

PredType BPOgehl::predict(DInst *dinst, bool doUpdate, bool doStats) {
  if(dinst->getInst()->isJump())
    return btb.predict(dinst, doUpdate, doStats);

  bool taken  = dinst->isTaken();
  bool ptaken = false;

  int32_t      S   = 0; // mtables/2
  HistoryType *iID = (HistoryType *)alloca(mtables * sizeof(HistoryType));

  // Prediction is sum of entries in M tables (table 1 is half-size to fit in 64k)
  for(int32_t i = 0; i < mtables; i++) {
    if(i == 1)
      logpred--;
    iID[i] = geoidx(dinst->getPC() >> 2, ghist, usedHistLength[i], (i & 3) + 1);
    if(i == 1)
      logpred++;
    S += pred[i][iID[i]];
  }
  ptaken = (S >= 0);
#if 0
  for (int32_t i = 0; i < mtables; i++) {
    MSG("0x%llx %s (%d) idx=%d pred=%d total=%d",dinst->getPC(), ptaken==taken?"C":"M", i, iID[i], (int)pred[i][iID[i]],S);
  }
#endif

  if(doUpdate) {

    // Update theta (threshold)
    if(taken != ptaken) {
      TC++;
      if(TC > THETAUP - 1) {
        TC = THETAUP - 1;
        if(THETA < MAXTHETA) {
          TC = 0;
          THETA++;
        }
      }
    } else if(S < THETA && S >= -THETA) {
      TC--;
      if(TC < -THETAUP) {
        TC = -THETAUP;
        if(THETA > 0) {
          TC = 0;
          THETA--;
        }
      }
    }

    if(taken != ptaken || (S < THETA && S >= -THETA)) {

      // Update M tables
      for(int32_t i = 0; i < mtables; i++) {
        if(taken) {
          if(pred[i][iID[i]] < PREDUP - 1)
            pred[i][iID[i]]++;
        } else {
          if(pred[i][iID[i]] > -PREDUP)
            pred[i][iID[i]]--;
        }
      }

      // Update history lengths
      if(taken != ptaken) {
        miniTag = MINITAG[iID[mtables - 1] >> 1];
        if(miniTag != genMiniTag(dinst)) {
          AC -= 4;
          if(AC < -256) {
            AC                = -256;
            usedHistLength[6] = histLength[6];
            usedHistLength[4] = histLength[4];
            usedHistLength[2] = histLength[2];
          }
        } else {
          AC++;
          if(AC > 256 - 1) {
            AC                = 256 - 1;
            usedHistLength[6] = histLength[mtables + 2];
            usedHistLength[4] = histLength[mtables + 1];
            usedHistLength[2] = histLength[mtables];
          }
        }
      }
      MINITAG[iID[mtables - 1] >> 1] = genMiniTag(dinst);
    }

    // Update branch/path histories
    for(int32_t i = (glength >> 6) + 1; i > 0; i--)
      ghist[i] = (ghist[i] << 1) + (ghist[i - 1] < 0);
    ghist[0] = ghist[0] << 1;
    if(taken) {
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

  if(taken != ptaken) {
    if(doUpdate)
      btb.updateOnly(dinst);
    return MissPrediction;
  }

  return ptaken ? btb.predict(dinst, doUpdate, doStats) : CorrectPrediction;
}

int32_t BPOgehl::geoidx(uint64_t Add, int64_t *histo, int32_t m, int32_t funct) {
  uint64_t inter, Hh, Res;
  int32_t  x, i, shift;
  int32_t  PT;
  int32_t  MinAdd;
  int32_t  FUNCT;

  MinAdd = nentry * logpred - m;
  if(MinAdd > 20)
    MinAdd = 20;

  if(MinAdd >= 8) {
    inter = ((histo[0] & ((1 << m) - 1)) << (MinAdd)) + ((Add & ((1 << MinAdd) - 1)));
  } else {
    for(x = 0; x < nentry * logpred; x++) {
      T[x] = ((x * (addwidth + m - 1)) / (nentry * logpred - 1));
    }

    T[nentry * logpred] = addwidth + m;
    inter               = 0;

    Hh = histo[0];
    Hh >>= T[0];
    inter = (Hh & 1);
    PT    = 1;

    for(i = 1; T[i] < m; i++) {
      if((T[i] & 0xffc0) == (T[i - 1] & 0xffc0)) {
        shift = T[i] - T[i - 1];
      } else {
        Hh = histo[PT];
        PT++;
        shift = T[i] & 63;
      }

      inter = (inter << 1);
      Hh    = Hh >> shift;
      inter ^= (Hh & 1);
    }

    Hh = Add;
    for(; T[i] < m + addwidth; i++) {
      shift = T[i] - m;
      inter = (inter << 1);
      inter ^= ((Hh >> shift) & 1);
    }
  }

  FUNCT = funct;
  Res   = inter & ((1 << logpred) - 1);
  for(i = 1; i < nentry; i++) {
    inter = inter >> logpred;
    Res ^= ((inter & ((1 << logpred) - 1)) >> FUNCT) ^ ((inter & ((1 << FUNCT) - 1)) << ((logpred - FUNCT)));
    FUNCT = (FUNCT + 1) % logpred;
  }

  return ((int)Res);
}

/*****************************************
 * LGW: Local Global Wavelet
 *
 * Extending OGEHL with TAGE ideas and LGW
 *
 */

LoopPredictor::LoopPredictor(int n)
    : nentries(n) {

  table = new LoopEntry[nentries];
}

void LoopPredictor::update(uint64_t key, uint64_t tag, bool taken) {

  // FIXME: add a small learning fully assoc (12 entry?) to learn loops. Backup with a 2-way loop entry
  //
  // FIXME: add some resilience to have loops that alternate between different loop sizes.
  //
  // FIXME: systematically check all the loops in CBP, and see how to capture them all

  LoopEntry *ent = &table[key % nentries];

  if(ent->tag != tag) {
    ent->tag         = tag;
    ent->confidence  = 0;
    ent->currCounter = 0;
    ent->dir         = taken;
  }

  ent->currCounter++;
  if(ent->dir != taken) {

    if(ent->iterCounter == ent->currCounter) {
      // MSG("1updt: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir,
      // ent->confidence);
      ent->confidence++;
    } else {
      // MSG("2updt: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir,
      // ent->confidence);
      ent->tag        = 0;
      ent->confidence = 0;
    }

    if(ent->confidence == 0)
      ent->dir = taken;

    ent->iterCounter = ent->currCounter;
    ent->currCounter = 0;
  }
}

bool LoopPredictor::isLoop(uint64_t key, uint64_t tag) const {
  const LoopEntry *ent = &table[key % nentries];

  if(ent->tag != tag)
    return false;

  return (ent->confidence * ent->iterCounter) > 800 && ent->confidence > 7;
}

bool LoopPredictor::isTaken(uint64_t key, uint64_t tag, bool taken) {

  I(isLoop(key, tag));

  LoopEntry *ent = &table[key % nentries];

  bool dir = ent->dir;
  if((ent->currCounter + 1) == ent->iterCounter)
    dir = !ent->dir;

  if(dir != taken) {
    // MSG("baad: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir,
    // ent->confidence);

    ent->confidence /= 2;
  } else {
    // MSG("good: key=%llx tag=%llx curr=%d iter=%d dir=%d conf=%d", key, tag, ent->currCounter, ent->iterCounter, ent->dir,
    // ent->confidence);
  }

  return dir;
}

uint32_t LoopPredictor::getLoopIter(uint64_t key, uint64_t tag) const {
  const LoopEntry *ent = &table[key % nentries];

  if(ent->tag != tag)
    return 0;

  return ent->iterCounter;
}

/*****************************************
 * BPredictor
 */

BPred *BPredictor::getBPred(int32_t id, const char *sec, const char *sname) {
  BPred *pred = 0;

  const char *type = SescConf->getCharPtr(sec, "type");

  // Normal Predictor
  if(strcasecmp(type, "oracle") == 0) {
    pred = new BPOracle(id, sec, sname);
  } else if(strcasecmp(type, "Miss") == 0) {
    pred = new BPMiss(id, sec, sname);
  } else if(strcasecmp(type, "NotTaken") == 0) {
    pred = new BPNotTaken(id, sec, sname);
  } else if(strcasecmp(type, "NotTakenEnhanced") == 0) {
    pred = new BPNotTakenEnhanced(id, sec, sname);
  } else if(strcasecmp(type, "Taken") == 0) {
    pred = new BPTaken(id, sec, sname);
  } else if(strcasecmp(type, "2bit") == 0) {
    pred = new BP2bit(id, sec, sname);
  } else if(strcasecmp(type, "2level") == 0) {
    pred = new BP2level(id, sec, sname);
  } else if(strcasecmp(type, "2BcgSkew") == 0) {
    pred = new BP2BcgSkew(id, sec, sname);
  } else if(strcasecmp(type, "Hybrid") == 0) {
    pred = new BPHybrid(id, sec, sname);
  } else if(strcasecmp(type, "yags") == 0) {
    pred = new BPyags(id, sec, sname);
  } else if(strcasecmp(type, "imli") == 0) {
    pred = new BPIMLI(id, sec, sname);
  } else if(strcasecmp(type, "tdata") == 0) {
    pred = new BPTData(id, sec, sname);
  } else if(strcasecmp(type, "ldbp") == 0) {
    pred = new BPLdbp(id, sec, sname);
  } else if(strcasecmp(type, "sogehl") == 0) {
    pred = new BPSOgehl(id, sec, sname);
  } else {
    MSG("BPredictor::BPredictor Invalid branch predictor type [%s] in section [%s]", type, sec);
    SescConf->notCorrect();
    return 0;
  }
  I(pred);

  return pred;
}

BPredictor::BPredictor(int32_t i, MemObj *iobj, BPredictor *bpred)
    : id(i)
    , SMTcopy(bpred != 0)
    , il1(iobj)
    , nBTAC("P(%d)_BPred:nBTAC", id)
    , nBranches("P(%d)_BPred:nBranches", id)
    , nTaken("P(%d)_BPred:nTaken", id)
    , nMiss("P(%d)_BPred:nMiss", id)
    , nBranches2("P(%d)_BPred:nBranches2", id)
    , nTaken2("P(%d)_BPred:nTaken2", id)
    , nMiss2("P(%d)_BPred:nMiss2", id)
    , nBranches3("P(%d)_BPred:nBranches3", id)
    , nTaken3("P(%d)_BPred:nTaken3", id)
    , nMiss3("P(%d)_BPred:nMiss3", id)
    , nFixes1("P(%d)_BPred:nFixes1", id)
    , nFixes2("P(%d)_BPred:nFixes2", id)
    , nFixes3("P(%d)_BPred:nFixes3", id)
    , nUnFixes("P(%d)_BPred:nUnFixes", id)
    , nAgree3("P(%d)_BPred:nAgree3", id) {
  const char *bpredSection  = SescConf->getCharPtr("cpusimu", "bpred", id);
  const char *bpredSection2 = 0;
  bpredDelay2               = SescConf->getInt(bpredSection, "BTACDelay");
  bpredDelay3               = bpredDelay2;

  if(SescConf->checkCharPtr("cpusimu", "bpred2", id)) {
    bpredSection2 = SescConf->getCharPtr("cpusimu", "bpred2", id);
    bpredDelay3   = SescConf->getInt(bpredSection2, "BTACDelay");
  }

  const char *bpredSection3    = 0;
  const char *bpredSectionMeta = 0;
  if(SescConf->checkCharPtr("cpusimu", "bpred3", id))
    bpredSection3 = SescConf->getCharPtr("cpusimu", "bpred3", id);
  if(SescConf->checkCharPtr("cpusimu", "bpredMeta", id))
    bpredSectionMeta = SescConf->getCharPtr("cpusimu", "bpredMeta", id);

  FetchWidth = SescConf->getInt("cpusimu", "fetchWidth", id);

  if(!(bpredDelay1 <= bpredDelay2 <= bpredDelay3)) {
    MSG("ERROR: bpredDelay (%d) should be <= bpredDelay2 (%d) <= bpredDelay3 (%d)", bpredDelay1, bpredDelay2, bpredDelay3);
    SescConf->notCorrect();
  }

  if(bpredDelay2)
    SescConf->isBetween("cpusimu", "bpredDelay", 1, bpredDelay2, id);
  else
    SescConf->isBetween("cpusimu", "bpredDelay", 1, 1024, id);

  bpredDelay1 = SescConf->getInt("cpusimu", "bpredDelay", id);
  //bpredDelay2 = SescConf->getInt("cpusimu", "bpredDelay2", id);
  //bpredDelay3 = SescConf->getInt("cpusimu", "bpredDelay3", id);

  SescConf->isInt(bpredSection, "BTACDelay");
  SescConf->isBetween(bpredSection, "BTACDelay", 0, 1024);

  ras = new BPRas(id, bpredSection, "");

  // Threads in SMT system share the predictor. Only the Ras is duplicated
  if(bpred) {
    pred1 = bpred->pred1;
    pred2 = bpred->pred2;
    pred3 = bpred->pred3;
  } else {
    pred1 = getBPred(id, bpredSection, "");
    pred2 = 0;
    pred3 = 0;
    if(bpredSection2) {
      pred2 = getBPred(id, bpredSection2, "2");
    }
    if(bpredSection3) {
      if(bpredSectionMeta)
        meta = getBPred(id, bpredSectionMeta, "M");
      pred3 = getBPred(id, bpredSection3, "3");
    }
    if(bpredSection3 && !bpredSection2) {
      MSG("ERROR: bpred3 present and bpred2 missing. Not allowed");
      SescConf->notCorrect();
    }
  }
}

BPredictor::~BPredictor() {

  if(SMTcopy)
    return;

  delete pred1;
  if(pred2)
    delete pred2;
  if(pred3)
    delete pred3;
  if(meta)
    delete meta;
  pred1 = 0;
  pred2 = 0;
  pred3 = 0;
  meta  = 0;
}

void BPredictor::fetchBoundaryBegin(DInst *dinst) {
  pred1->fetchBoundaryBegin(dinst);
  if(pred2)
    pred2->fetchBoundaryBegin(dinst);
  if(pred3 == 0)
    return;

  pred3->fetchBoundaryBegin(dinst);
  if(meta)
    meta->fetchBoundaryBegin(dinst);
}

void BPredictor::fetchBoundaryEnd() {
  pred1->fetchBoundaryEnd();
  if(pred2)
    pred2->fetchBoundaryEnd();
  if(pred3 == 0)
    return;
  pred3->fetchBoundaryEnd();
  if(meta)
    meta->fetchBoundaryEnd();
}

/*
bool  BPredictor::Miss_Prediction(DInst *dinst) {


  PredType outcome1;
  PredType outcome2;
  PredType outcome3;

  outcome1 = ras->doPredict(dinst);
  if( outcome1 == NoPrediction ) {
    outcome1 = predict1(dinst);
    outcome2 = outcome1;
    if (pred2)
      outcome2 = predict2(dinst);
    outcome3 = outcome2;
    if (pred3) {
      if (dinst->isBiasBranch())
       //MSG(" pred3 is biased"); // update to keep prediction up to date, but nothing to do
      else
        outcome3 = predict3(dinst);
    }
  }else {
    outcome2 = outcome1;
  }
  if (outcome1 == !CorrectPrediction || outcome2 == !CorrectPrediction || outcome3 == !CorrectPrediction) {
      //MSG(" MissPredictio/ NOT Correct Predictionn");
      return true;
  }else {
      //MSG(" Correct Prediction ..p values is");
      return false;
  }
}
*/

PredType BPredictor::predict1(DInst *dinst) {
  I(dinst->getInst()->isControl());

#if 0
  printf("BPRED: pc: %x ", dinst->getPC() );
  printf(" fun call: %d, ", dinst->getInst()->isFuncCall()); printf("fun ret: %d, ", dinst->getInst()->isFuncRet());printf("taken: %d, ", dinst->isTaken());  printf("target addr: 0x%x\n", dinst->getAddr());
#endif

  nBranches.inc(dinst->getStatsFlag());
  nTaken.inc(dinst->isTaken() && dinst->getStatsFlag());

  PredType p = pred1->doPredict(dinst);

  nMiss.inc(p != CorrectPrediction && dinst->getStatsFlag());

  return p;
}

PredType BPredictor::predict2(DInst *dinst) {
  I(dinst->getInst()->isControl());

  nBranches2.inc(dinst->getStatsFlag());
  nTaken2.inc(dinst->isTaken() && dinst->getStatsFlag());
  // No RAS in L2

  PredType p = pred2->doPredict(dinst);

  nMiss2.inc(p != CorrectPrediction && dinst->getStatsFlag());

  return p;
}

PredType BPredictor::predict3(DInst *dinst) {
  I(dinst->getInst()->isControl());

#if 0
  if (dinst->isBiasBranch())
    return NoPrediction;
#endif

  if(dinst->getDataSign() == DS_NoData)
    return NoPrediction;

  nBranches3.inc(dinst->getStatsFlag());
  nTaken3.inc(dinst->isTaken() && dinst->getStatsFlag());
  // No RAS in L2

  PredType p = pred3->doPredict(dinst);
  if(p == NoPrediction)
    return p;

  nMiss3.inc(p != CorrectPrediction && dinst->getStatsFlag());

  return p;
}

TimeDelta_t BPredictor::predict(DInst *dinst, bool *fastfix) {

  *fastfix = true;

  PredType outcome1;
  PredType outcome2;
  PredType outcome3;
  dinst->setBiasBranch(false);

  bool used3 = false;

  outcome1 = ras->doPredict(dinst);
  if(outcome1 == NoPrediction) {
  //if(pred1) {
    outcome1 = predict1(dinst);
    outcome2 = outcome1;
    if(pred2) {
      outcome2 = predict2(dinst);
      // bool tracking2   = dinst->getPC() == 0x100072dc && dinst->getStatsFlag();
      bool tracking2 = false;
      if(tracking2) {
        //printf("btrack2 %x %d %s %x %d\n", dinst->getPC(), outcome2, dinst->isTaken() ? "T" : "NT", dinst->getLDPC(),
               //dinst->getDataSign());
        //printf("bmeta2 bias, pred %d %s\n", outcome2, dinst->isTaken() ? "T" : "NT");
      }
    }
    outcome3 = outcome2;
    /*
        //bool tracking = dinst->getPC() == 0x12000e288 && dinst->getStatsFlag();
        //bool tracking = dinst->getPC() == 0x12000e288 && dinst->getStatsFlag();
        //bool tracking = dinst->getPC() == 0x12001b870 && dinst->getStatsFlag();
        bool tracking = dinst->getPC() == 0x100072dc && dinst->getStatsFlag();
        //bool tracking = false;
    */
    if(pred3) {
      //AddrType old_addr  = dinst->getAddr();
      //AddrType old_addr1 = dinst->getAddr();
      //AddrType old_pc    = dinst->getPC();
#if 1
      // dinst->setPC(dinst->getLDPC() ^ (old_pc<<7) ^ (old_pc>>3) ^ (dinst->getDataSign()<<10) ^ (dinst->getDataSign()<<2));
      //dinst->setPC(dinst->getLDPC() ^ (old_pc << 7) ^ (dinst->getDataSign() << 10));
#endif
      /*
              //try3 = try3 || !dinst->isBiasBranch(); //try3 || dinst->isBiasBranch() gives 98.5% bpred accuracy for bzip2
              try3 = worth_checking3 || !dinst->isBiasBranch();
              //try3 = true;
              //try3 = old_pc == 0x100072dc;
              if (try3) {
                outcome3 = predict3(dinst);
                used3    = true;
                if (tracking) {
                  printf("btrack o:%d %s ldpc:%llx ds:%d brpc:%llx i:%x\n", outcome3, dinst->isTaken()?"T":"NT",dinst->getLDPC(),
      dinst->getDataSign(), old_pc, pred3->calcHist(dinst->getPC())); printf("bmeta3 bias, pred %d %d %s\n", outcome2, outcome3,
      dinst->isTaken()?"T":"NT");
                }
              }else{
      #if 1
                pred3->update(dinst); // update to keep prediction up to date, but nothing to do
      #endif
                if (tracking)
                  printf("bmeta2 bias, no pred %d %s\n", outcome2, dinst->isTaken()?"T":"NT");
              }

              dinst->setPC(old_pc);
            }else{
                if (tracking) {
                  printf("bmeta2 else, pred %d %s\n", outcome2, dinst->isTaken()?"T":"NT");
                }
      #if 1
              // With history. Only needed if _data predictor has history

              if (dinst->isBiasBranch() || dinst->getDataSign() == DS_NoData) {
                // good for sjeng 0x100072dc if (dinst->getDataSign() == DS_NoData)
                pred3->update(dinst); // update to keep prediction up to date, but nothing to do
                if (tracking)
                  printf("bpred2 bias, no pred %d %s\n", outcome2, dinst->isTaken()?"T":"NT");
              } else {
                outcome3 = predict3(dinst);
                if (!dinst->isBiasBranch()) {
                  if (dinst->getPC() == 0x100072dc && dinst->getStatsFlag())
                    printf("bpred3 not bias, no pred %d %s\n", outcome2, dinst->isTaken()?"T":"NT");
                  outcome3 = outcome2; // If unsure, use default TAGE
                } else {
                  if (tracking)
                    printf("bpred3 bias, pred %d %d %s\n", outcome2, outcome3, dinst->isTaken()?"T":"NT");

                  used3 = true;
                }
              }
      #endif
            }
      =======*/
      //dinst->setAddr(old_addr);
      outcome3 = predict3(dinst);
      used3    = true;
      //dinst->setPC(old_pc);

      if(outcome3 == NoPrediction) {
        outcome3 = outcome2;
        used3    = false;
      }
    }
  } else {
    outcome2 = outcome1;
    outcome3 = outcome1;
  }

  if(dinst->getInst()->isFuncRet() || dinst->getInst()->isFuncCall()) {
    dinst->setBiasBranch(true);
    ras->tryPrefetch(il1, dinst->getStatsFlag(), 1);
  }

  if(outcome1 == CorrectPrediction && outcome2 == CorrectPrediction && outcome3 == CorrectPrediction) {
    unset_Miss_Pred_Bool(); //
    // MSG("Starting ****** Hit Predction value is EnumType=0 ******");
    // std::cout<<" Hit Prediction matches 0 ="<<get_prediction_type()>>std::endl;
  } else {
    // std::cout<<" Miss Predction value is EnumType!=0 and should beActually is ******"<<get_prediction_type()<<std::endl;
    set_Miss_Pred_Bool();
    // MSG("Starting ****** Miss Predction value is EnumType=1,2,3,4 ******");
    // std::cout<<" Resetting the Bool variableto ...  "<<get_prediction_type()<<std::endl;
  }

  if(outcome1 == CorrectPrediction && outcome2 == CorrectPrediction && outcome3 == CorrectPrediction) {
    nAgree3.inc(dinst->getStatsFlag());

    if(dinst->isTaken()) {
#ifdef CLOSE_TARGET_OPTIMIZATION
      int distance = dinst->getAddr() - dinst->getPC();

      uint32_t delta = distance / FetchWidth + 1;
      if(delta < bpredDelay1 && distance > 0)
        return delta - 1;
#endif

      return bpredDelay1 - 1;
    }
    return 0;
  }

  if(dinst->getInst()->doesJump2Label()) {
    nBTAC.inc(dinst->getStatsFlag());
    return bpredDelay1 - 1;
  }

  int32_t bpred_total_delay = bpredDelay1 - 1;

  if(outcome1 != CorrectPrediction && outcome2 == CorrectPrediction && outcome3 == CorrectPrediction) {
    nFixes1.inc(dinst->getStatsFlag());
    bpred_total_delay = bpredDelay2 - 1;
  } else if(outcome1 != CorrectPrediction && outcome2 != CorrectPrediction && outcome3 == CorrectPrediction) {
    nFixes2.inc(dinst->getStatsFlag());
    bpred_total_delay = bpredDelay3 - 1;
  } else if(outcome1 == CorrectPrediction && outcome2 != CorrectPrediction && outcome3 == CorrectPrediction) {
    nFixes3.inc(dinst->getStatsFlag());
    bpred_total_delay = bpredDelay3 - 1;
  } else if(outcome3 != CorrectPrediction) {
    nUnFixes.inc(dinst->getStatsFlag());
    *fastfix          = false;
    bpred_total_delay = 2; // Anything but zero
  } else {
    nAgree3.inc(dinst->getStatsFlag());
  }

  return bpred_total_delay;
}

void BPredictor::dump(const char *str) const {
  // nothing?
}
