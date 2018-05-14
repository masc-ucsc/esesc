// Contributed by Jose Renau
//                Milos Prvulovic
//                Smruti Sarangi
//                Luis Ceze
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

#include "FetchEngine.h"

#include "SescConf.h"
#include "alloca.h"

#include "GMemorySystem.h"
#include "MemObj.h"
#include "MemRequest.h"
#include "Pipeline.h"
extern bool MIMDmode;

//#define ENABLE_FAST_WARMUP 1
//#define FETCH_TRACE 1

// SBPT: Do not track RAT, just use last predictable LD
//#define SBPT_JUSTLAST 1
//#define SBPT_JUSTDELTA0 1

FetchEngine::FetchEngine(FlowID id, GMemorySystem *gms_, FetchEngine *fe)
    : gms(gms_)
    , avgFetchLost("P(%d)_FetchEngine_avgFetchLost", id)
    , avgBranchTime("P(%d)_FetchEngine_avgBranchTime", id)
    , avgBranchTime2("P(%d)_FetchEngine_avgBranchTime2", id)
    , avgFetchTime("P(%d)_FetchEngine_avgFetchTime", id)
    , avgFetched("P(%d)_FetchEngine_avgFetched", id)
    , nDelayInst1("P(%d)_FetchEngine:nDelayInst1", id)
    , nDelayInst2("P(%d)_FetchEngine:nDelayInst2", id) // Not enough BB/LVIDs per cycle
    , nDelayInst3("P(%d)_FetchEngine:nDelayInst3", id)
    , nBTAC("P(%d)_FetchEngine:nBTAC", id) // BTAC corrections to BTB
    , zeroDinst("P(%d)_zeroDinst:nBTAC", id)
#ifdef ESESC_TRACE_DATA
    , dataHist("P(%d)_dataHist", id)
    , dataSignHist("P(%d)_dataSignHist", id)
    , lastData("P(%d)_lastData", 2048, 4)
    , nbranchMissHist("P(%d)_nbranchMissHist", id)
    , nLoadAddr_per_branch("P(%d)_nLoadAddr_per_branch", id)
    , nLoadData_per_branch("P(%d)_nLoadData_per_branch", id)

#endif
//  ,szBB("FetchEngine(%d):szBB", id)
//  ,szFB("FetchEngine(%d):szFB", id)
//  ,szFS("FetchEngine(%d):szFS", id)
//,unBlockFetchCB(this)
//,unBlockFetchBPredDelayCB(this)
{
  // Constraints
  SescConf->isInt("cpusimu", "fetchWidth", id);
  SescConf->isBetween("cpusimu", "fetchWidth", 1, 1024, id);
  SescConf->isPower2("cpusimu", "fetchWidth", id);

  FetchWidth     = SescConf->getInt("cpusimu", "fetchWidth", id);
  FetchWidthBits = log2i(FetchWidth);

  Fetch2Width     = FetchWidth / 2;
  Fetch2WidthBits = log2i(Fetch2Width);

  AlignedFetch = SescConf->getBool("cpusimu", "alignedFetch", id);
  if(SescConf->checkBool("cpusimu", "traceAlign", id)) {
    TraceAlign = SescConf->getBool("cpusimu", "TraceAlign", id);
  } else {
    TraceAlign = false;
  }
  if(SescConf->checkBool("cpusimu", "TargetInLine", id)) {
    TargetInLine = SescConf->getBool("cpusimu", "TargetInLine", id);
  } else {
    TargetInLine = false;
  }

  if(SescConf->checkBool("cpusimu", "fetchOneLine", id)) {
    FetchOneLine = SescConf->getBool("cpusimu", "fetchOneLine", id);
  } else {
    FetchOneLine = !TraceAlign;
  }

  SescConf->isBetween("cpusimu", "bb4Cycle", 0, 1024, id);
  BB4Cycle = SescConf->getInt("cpusimu", "bb4Cycle", id);
  if(BB4Cycle == 0)
    BB4Cycle = USHRT_MAX;

  if(fe)
    bpred = new BPredictor(id, gms->getIL1(), fe->bpred);
  else
    bpred = new BPredictor(id, gms->getIL1());

  missInst = false;
  const char *apsection;
  const char *apstr;

  // Move to libmem/Prefetcher.cpp ; it can be stride or DVTAGE
  // FIXME: use AddressPredictor::create()

#ifdef ESESC_TRACE_DATA
  ideal_apred = new StrideAddressPredictor();
  // ideal_apred = new vtage(9, 4, 1, 3);
#endif

  const char *isection = SescConf->getCharPtr("cpusimu", "IL1", id);
  const char *pos      = strchr(isection, ' ');
  if(pos) {
    char *pos         = strdup(isection);
    *strchr(pos, ' ') = 0;
    isection          = pos;
  }

  const char *itype = SescConf->getCharPtr(isection, "deviceType");

  if(strcasecmp(itype, "icache") != 0) {
    isection        = SescConf->getCharPtr(isection, "lowerLevel");
    const char *pos = strchr(isection, ' ');
    if(pos) {
      char *pos         = strdup(isection);
      *strchr(pos, ' ') = 0;
      isection          = pos;
    }
    itype = SescConf->getCharPtr(isection, "deviceType");
    if(strcasecmp(itype, "icache") != 0) {
      MSG("ERROR: the icache should be the first or second after IL1 in the core");
      SescConf->notCorrect();
    }
  }
  LineSize = SescConf->getInt(isection, "bsize");
  if((LineSize / 4) < FetchWidth) {
    MSG("ERROR: icache line size should be larger than fetch width LineSize=%d fetchWidth=%d [%s]", LineSize, FetchWidth, isection);
    SescConf->notCorrect();
  }
  LineSizeBits = log2i(LineSize);

  // Get some icache L1 parameters
  enableICache = SescConf->getBool("cpusimu", "enableICache", id);
  IL1HitDelay  = SescConf->getInt(isection, "hitDelay");

  lastMissTime = 0;
}

FetchEngine::~FetchEngine() {
  delete bpred;
}

bool FetchEngine::processBranch(DInst *dinst, uint16_t n2Fetch) {
  const Instruction *inst = dinst->getInst();

  I(dinst->getInst()->isControl()); // getAddr is target only for br/jmp

  bool        fastfix;
  TimeDelta_t delay = bpred->predict(dinst, &fastfix);
#ifdef ESESC_TRACE_DATA
  if(dinst->getDataSign() != DS_NoData) {
    oracleDataLast[dinst->getLDPC()].chain(); // getLDPC only works (hash otherwise) when there is a single ldpc
  }
#endif
  if(delay == 0) {
    // printf(" good\n");
    return false;
  }

  setMissInst(dinst);

  Time_t n = (globalClock - lastMissTime);
  avgFetchTime.sample(n, dinst->getStatsFlag());

#if 0
  if (!dinst->isBiasBranch()) {
    if ( dinst->isTaken() && (dinst->getAddr() > dinst->getPC() && (dinst->getAddr() + 8<<2) <= dinst->getPC())) {
      fastfix = true;
    }
  }
#endif

  if(fastfix) {
    I(globalClock);
    unBlockFetchBPredDelayCB::schedule(delay, this, dinst, globalClock);
    // printf(" good\n");
  } else {
    // printf(" bad brpc:%llx\n",dinst->getPC());
    dinst->lockFetch(this);
  }

  return true;
}

void FetchEngine::chainPrefDone(AddrType pc, int distance, AddrType addr) {

#ifdef ESESC_TRACE_DATA
  // MSG("pfchain ldpc:%llx %d dist:%d addr:%llx",pc, oracleDataLast[pc].chained, distance, addr);
#endif
}

void FetchEngine::chainLoadDone(DInst *dinst) {
#ifdef ESESC_TRACE_DATA
  // MSG("ldchain ldpc:%llx %d dist:%d",dinst->getPC(), oracleDataLast[dinst->getPC()].chained, dinst->getChained());

  oracleDataLast[dinst->getPC()].dec_chain();
#endif
}

void FetchEngine::realfetch(IBucket *bucket, EmulInterface *eint, FlowID fid, int32_t n2Fetch) {

  AddrType lastpc = 0;

#ifdef USE_FUSE
  RegType last_dest = LREG_R0;
  RegType last_src1 = LREG_R0;
  RegType last_src2 = LREG_R0;
#endif

  do {
    DInst *dinst = 0;
    dinst        = eint->peekHead(fid);
    if(dinst == 0) {
#if 0
      fprintf(stderr,"Z");
#endif
      zeroDinst.inc(true);
      break;
    }

#ifdef ESESC_TRACE_DATA
    bool predictable = false;
    // I(dinst->getPC() != 0x1001ec98ULL);
    /*if (dinst->getInst()->isLoad()) {

      int64_t data_load=dinst->getData();
      MSG("load_data for pc(%llx) = %lld", dinst->getPC(), data_load);
      }*/

#if 0
    if (dinst->getInst()->isControl()) {
      bool p=bpred->get_Miss_Pred_Bool_Val();
      //MSG("Entering Fetch Engine Only in Miss_Prediction if (p!=0)Value of  p is %d",p);

      if(p) { //p=1 Miss Prediction
        // MSG("Miss Prediction so Value of  p is %d" , p);
        uint64_t pc_br = dinst->getPC();
        pc_br = pc_br<<16;

        nbranchMissHist.sample(dinst->getStatsFlag(),pc_br);
        // MSG("Branch Miss....sample");

        uint64_t ldpc_Src1 = oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
        uint64_t ldpc_Src2 = oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
        uint64_t load_data_Src1 = oracleDataLast[ldpc_Src1].addr;
        uint64_t load_addr_Src1 = oracleDataLast[ldpc_Src1].data;

        load_data_Src1=pc_br|(load_data_Src1 & 0xFFFF);
        load_addr_Src1=pc_br|(load_addr_Src1 & 0xFFFF);

        uint64_t load_data_Src2 = oracleDataLast[ldpc_Src2].addr;
        uint64_t load_addr_Src2 = oracleDataLast[ldpc_Src2].data;

        load_data_Src2=pc_br|(load_data_Src2 & 0xFFFF);
        load_addr_Src2=pc_br|(load_addr_Src2 & 0xFFFF);
        nLoadData_per_branch.sample(dinst->getStatsFlag(),load_data_Src1);//pc|16bit data cnt++
        nLoadData_per_branch.sample(dinst->getStatsFlag(),load_data_Src2);//cnt++ same data stucture
        //MSG(" LoadDataSampling ...working Well");
        nLoadAddr_per_branch.sample(dinst->getStatsFlag(),load_addr_Src1);//pc|16bit data cnt++
        nLoadAddr_per_branch.sample(dinst->getStatsFlag(),load_addr_Src2);//cnt++ same data stucture
        //MSG(" LoadAddrSampling ...working Well");
      }//p ends
    }//control
#endif

    if(dinst->getInst()->isLoad()) {

      bool ld_tracking = dinst->getPC() == 0x10006540 || dinst->getPC() == 0x10006544;

      // ld_tracking = false;

      if(ideal_apred) {

#if 0
        predictable = true; // FIXME2: ENABLE MAGIC/ORACLE PREDICTION FOR ALL
#else
        ideal_apred->exe_update(dinst->getPC(), dinst->getAddr(), dinst->getData());
        ideal_apred->ret_update(dinst->getPC(), dinst->getAddr(), dinst->getData());
        AddrType naddr = ideal_apred->predict(dinst->getPC(), 0, false); // ideal, predict after update

        if(naddr == dinst->getAddr())
          predictable = true;

        if(ld_tracking) {
          printf("predictable:%d ldpc:%llx naddr:%llx addr:%llx %d data:%d\n", predictable ? 1 : 0, dinst->getPC(), naddr,
                 dinst->getAddr(), dinst->getInst()->getDst1(), dinst->getData());
        }
#endif
      }

      if(predictable) {
        oracleDataRAT[dinst->getInst()->getDst1()].depth = 0;
        oracleDataRAT[dinst->getInst()->getDst1()].ldpc  = dinst->getPC();
        lastPredictable_ldpc                             = dinst->getPC();
        lastPredictable_addr                             = dinst->getAddr();
        lastPredictable_data                             = dinst->getData();
        oracleDataLast[dinst->getPC()].set(dinst->getData(), dinst->getAddr());
      } else {
        oracleDataRAT[dinst->getInst()->getDst1()].depth = 32;
        oracleDataRAT[dinst->getInst()->getDst1()].ldpc  = 0;
        oracleDataLast[dinst->getPC()].clear(dinst->getData(), dinst->getAddr());
      }

      if(oracleDataLast[dinst->getPC()].isChained()) {
        dinst->setChain(this, oracleDataLast[dinst->getPC()].inc_chain());
      }
    }
#if 1
    if(!dinst->getInst()->isLoad()) { // Not for LD-LD chain
      // Copy Other
      int      d = 32768;
      AddrType ldpc;
      int      d1    = oracleDataRAT[dinst->getInst()->getSrc1()].depth;
      int      d2    = oracleDataRAT[dinst->getInst()->getSrc2()].depth;
      AddrType ldpc2 = 0;
      if(d1 < d2 && d1 < 3) {
        d    = d1;
        ldpc = oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
#if 1
        if(d2 < 4)
          ldpc2 = oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
#endif
      } else if(d2 < d1 && d2 < 3) {
        d    = d2;
        ldpc = oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
#if 1
        if(d1 < 4)
          ldpc2 = oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
#endif
      } else if(d1 == d2 && d1 < 3) {
        // Closest ldpc
        AddrType x1 = dinst->getPC() - oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
        AddrType x2 = dinst->getPC() - oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
        if(d1 < d2)
          d = d1;
        else
          d = d2;
        if(x1 < x2) {
          ldpc = oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
          if(d2 < 2)
            ldpc2 = oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
        } else {
          ldpc = oracleDataRAT[dinst->getInst()->getSrc2()].ldpc;
          if(d1 < 2)
            ldpc2 = oracleDataRAT[dinst->getInst()->getSrc1()].ldpc;
        }
      } else {
        d    = 32768;
        ldpc = 0;
      }

#ifdef SBPT_JUSTDELTA0
      if(ldpc && oracleDataLast[ldpc].delta0) {
        d    = 32768;
        ldpc = 0;
      }
      if(ldpc2 && oracleDataLast[ldpc2].delta0) {
        ldpc2 = 0;
      }
#endif

      if(ldpc) {
        oracleDataRAT[dinst->getInst()->getDst1()].depth = d + 1;
        oracleDataRAT[dinst->getInst()->getDst1()].ldpc  = ldpc;

        AddrType data = oracleDataLast[ldpc].data;
        AddrType addr = oracleDataLast[ldpc].addr;

        if(oracleDataLast[ldpc].isChained()) {
#if 0
          if (oracleDataLast[ldpc].delta0) {
            MSG("brchain needs ldpc:%llx addr:%llx data:%lld dist:%d",ldpc,addr,data,0);
          }else{
            MSG("brchain needs ldpc:%llx addr:%llx data:%lld dist:%d",ldpc,addr,data,oracleDataLast[ldpc].chained);
          }
#endif
        }

        // bool tracking = dinst->getPC() == 0x12001b870 && dinst->getStatsFlag();
        // bool tracking = dinst->getPC() == 0x100072dc && dinst->getStatsFlag();
        bool tracking = dinst->getPC() == 0x10006548;
        tracking      = false;

        if(dinst->getInst()->isBranch()) {
          ldpc2brpc[ldpc] = dinst->getPC(); // Not used now. Once prediction is updated

          I(dinst->getDataSign() == DS_NoData);

#ifdef SBPT_JUSTLAST
          data  = lastPredictable_data;
          ldpc  = lastPredictable_ldpc;
          ldpc2 = 0;
          d     = 3;
#else
#if 0
          // OPT: Do not track BEQ if only src1 or src2 are ready
          // RESULTS: This is a bit worse than just try
          if (dinst->getInst()->hasSrc1Register()
              && dinst->getInst()->hasSrc2Register()
              && ldpc2 == 0) {
            d = 5;
          }
#endif
#endif
          if(d < 4) {
            dinst->setDataSign(data, ldpc);

#if 1
            if(ldpc2) {
#if 1
              // Always chain branches when both can be tracked
              oracleDataLast[ldpc].chain();
              oracleDataLast[ldpc2].chain();
#endif
              AddrType data3 = oracleDataLast[ldpc2].data;
              dinst->addDataSign(d, data3, ldpc2); // Trigger direct compare
              if(tracking) {
                printf("xxbr br ldpc:%llx %s ds:%d data:%d data3:%d ldpc:%llx ldpc2:%llx d:%d\n", dinst->getPC(),
                       dinst->isTaken() ? "T" : "NT", dinst->getDataSign(), data, data3, ldpc, ldpc2, d);
              }
            } else {
              if(tracking) {
                printf("yybr br ldpc:%llx %s ds:%d data:%d ldpc:%llx ldpc2:%llx d:%d\n", dinst->getPC(),
                       dinst->isTaken() ? "T" : "NT", dinst->getDataSign(), data, ldpc, ldpc2, d);
              }
            }
#endif

#if 0
            uint64_t s = dinst->getPC();
            s = s<<12;
            s = s | ((int)dinst->getDataSign())&0xFFF;

            uint64_t h_pc_data;
            h_pc_data = s<<16;
            h_pc_data = h_pc_data | ((int)dinst->getData())&0xFFFF;
            dataHist.sample(dinst->getStatsFlag(),static_cast<uint32_t>(h_pc_data));
            dataSignHist.sample(dinst->getStatsFlag(),s); // dinst->getDataSign());
#endif
          } else {

            if(tracking) {
              ldpc = lastPredictable_ldpc;
              data = lastPredictable_data;
              addr = lastPredictable_addr;
              printf("nopr br ldpc:%llx %s data:%d ldpc:%llx ldaddr:%llx r%d:%d r%d:%d\n", dinst->getPC(),
                     dinst->isTaken() ? "T" : "NT", data, ldpc, addr, dinst->getInst()->getSrc1(), d1, dinst->getInst()->getSrc2(),
                     d2);
            }
          }
        }
      }
    }
#endif
#endif

#ifdef ENABLE_FAST_WARMUP
    if(dinst->getPC() == 0) {
      eint->executeHead(fid); // Consume it for sure! (warmup mode)

      do {
        EventScheduler::advanceClock();
      } while(gms->getDL1()->isBusy(dinst->getAddr()));

      if(dinst->getInst()->isLoad()) {
        MemRequest::sendReqReadWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      } else if(dinst->getInst()->isStore()) {
        MemRequest::sendReqWriteWarmup(gms->getDL1(), dinst->getAddr());
        dinst->scrap(eint);
        dinst = 0;
      } else {
        I(0);
        dinst->scrap(eint);
        dinst = 0;
      }
      continue;
    }
#endif
    if(lastpc == 0) {
      bpred->fetchBoundaryBegin(dinst);
      if(!TraceAlign) {

        uint64_t entryPC = dinst->getPC() >> 2;

        uint16_t fetchLost;

        if(AlignedFetch) {
          fetchLost = (entryPC) & (FetchWidth - 1);
        } else {
          fetchLost = (entryPC) & (Fetch2Width - 1);
        }

        // No matter what, do not pass cache line boundary
        uint16_t fetchMaxPos = (entryPC & (LineSize / 4 - 1)) + FetchWidth;
        if(fetchMaxPos > (LineSize / 4)) {
          // MSG("entryPC=0x%llx lost1=%d lost2=%d",entryPC, fetchLost, (fetchMaxPos-LineSize/4));
          fetchLost += (fetchMaxPos - LineSize / 4);
        } else {
          // MSG("entryPC=0x%llx lost1=%d",entryPC, fetchLost);
        }

        avgFetchLost.sample(fetchLost, dinst->getStatsFlag());

        n2Fetch -= fetchLost;
      }

      n2Fetch--;
    } else {
      I(lastpc);

      if(lastpc == dinst->getPC()) { // Multiple uOPS no re-fetch issues
        // n2Fetch--;
      } else if((lastpc + 4) == dinst->getPC()) {
        n2Fetch--;
      } else if((lastpc + 8) != dinst->getPC()) {
        if(!TargetInLine || (lastpc >> LineSizeBits) != (dinst->getPC() >> LineSizeBits))
          maxBB--;
        if(maxBB < 1) {
          nDelayInst2.add(n2Fetch, dinst->getStatsFlag());
          break;
        }
        n2Fetch--; // There may be a NOP in the delay slot, do not count it
      } else {
        n2Fetch -= 2; // NOP still consumes delay slot
      }
      if(FetchOneLine) {
        if((lastpc >> LineSizeBits) != (dinst->getPC() >> LineSizeBits)) {
          break;
        }
      }
    }
    lastpc = dinst->getPC();

    eint->executeHead(fid); // Consume for sure

    dinst->setFetchTime();
    bucket->push(dinst);

#ifdef USE_FUSE
    if(dinst->getInst()->isControl()) {
      RegType src1 = dinst->getInst()->getSrc1();
      if(dinst->getInst()->doesJump2Label() && dinst->getInst()->getSrc2() == LREG_R0 &&
         (src1 == last_dest || src1 == last_src1 || src1 == last_src2 || src1 == LREG_R0)) {
        // MSG("pc %x fusion with previous", dinst->getPC());
        dinst->scrap(eint);
        continue;
      }
    }
#endif

#ifdef FETCH_TRACE
    static int bias_ninst   = 0;
    static int bias_firstPC = 0;
    bias_ninst++;
#endif

#ifdef FETCH_TRACE
    dinst->dump("FECHT");
#endif

    if(dinst->getInst()->isControl()) {
      bool stall_fetch = processBranch(dinst, n2Fetch);
      if(stall_fetch) {
        DInst *dinstn = eint->peekHead(fid);
        if(dinstn == 0) {
          zeroDinst.inc(true);
#if 0
          fprintf(stderr,"Z");
#endif
        }
        if(dinstn && dinst->getAddr() && n2Fetch) { // Taken branch and next inst is delay slot
          if(dinstn->getPC() == (lastpc + 4)) {
            eint->executeHead(fid);
            bucket->push(dinstn);
            n2Fetch--;
          }
        } else if(dinstn == 0)
          zeroDinst.inc(true);

#ifdef FETCH_TRACE
        if(dinst->isBiasBranch() && dinst->getFetchEngine()) {
          // OOPS. Thought that it was bias and it is a long misspredict
          MSG("ABORT first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
        if(bias_ninst > 256) {
          MSG("1ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
#endif
        break;
      }
#ifdef FETCH_TRACE
      if(bias_ninst > 256) {
        MSG("2ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
        bias_firstPC = dinst->getAddr();
        bias_ninst   = 0;
      } else if(!dinst->isBiasBranch()) {

        if(dinst->isTaken() && (dinst->getAddr() > dinst->getPC() && (dinst->getAddr() + 8 << 2) <= dinst->getPC())) {
          // Move instructions to predicated (up to 8)
          MSG("PRED  first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
        } else {
          if(bias_ninst < 16) {
            MSG("NOTRA first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          } else {
            MSG("3ENDS first=%llx last=%llx ninst=%d", bias_firstPC, dinst->getPC(), bias_ninst);
          }
          bias_firstPC = dinst->getAddr();
          bias_ninst   = 0;
        }
      }
#endif
    }

#ifdef USE_FUSE
    last_dest = dinst->getInst()->getDst1();
    last_src1 = dinst->getInst()->getSrc1();
    last_src2 = dinst->getInst()->getSrc2();
#endif

    // Fetch uses getHead, ROB retires getTail
  } while(n2Fetch > 0);

  bpred->fetchBoundaryEnd();

  if(enableICache && !bucket->empty()) {
    avgFetched.sample(bucket->size(), bucket->top()->getStatsFlag());
    MemRequest::sendReqRead(gms->getIL1(), bucket->top()->getStatsFlag(), bucket->top()->getPC(), 0xdeaddead,
                            &(bucket->markFetchedCB)); // 0xdeaddead as PC signature
  } else {
    bucket->markFetchedCB.schedule(IL1HitDelay);
#if 0
    if (bucket->top() != NULL)
    MSG("@%lld: Bucket [%p] (Top ID = %d) to be markFetched after %d cycles i.e. @%lld"
        ,(long long int)globalClock
        , bucket
        , (int) bucket->top()->getID()
        , IL1HitDelay
        , (long long int)(globalClock)+IL1HitDelay
        );
#endif
  }
}

void FetchEngine::fetch(IBucket *bucket, EmulInterface *eint, FlowID fid) {
  // Reset the max number of BB to fetch in this cycle (decreased in processBranch)
  maxBB = BB4Cycle;

  // You pass maxBB because there may be many fetches calls to realfetch in one cycle
  // (thanks to the callbacks)
  realfetch(bucket, eint, fid, FetchWidth);
}

void FetchEngine::dump(const char *str) const {
  char *nstr = (char *)alloca(strlen(str) + 20);
  sprintf(nstr, "%s_FE", str);
  bpred->dump(nstr);
}

void FetchEngine::unBlockFetchBPredDelay(DInst *dinst, Time_t missFetchTime) {
  clearMissInst(dinst, missFetchTime);

  Time_t n = (globalClock - missFetchTime);
  avgBranchTime2.sample(n, dinst->getStatsFlag()); // Not short branches
  // n *= FetchWidth; // FOR CPU
  n *= 1; // FOR GPU

  nDelayInst3.add(n, dinst->getStatsFlag());
}

void FetchEngine::unBlockFetch(DInst *dinst, Time_t missFetchTime) {
  clearMissInst(dinst, missFetchTime);

  I(missFetchTime != 0);
  Time_t n = (globalClock - missFetchTime) - 1;
  avgBranchTime.sample(n, dinst->getStatsFlag()); // Not short branches
  // n *= FetchWidth;  //FOR CPU
  n *= 1; // FOR GPU and for MIMD
  nDelayInst1.add(n, dinst->getStatsFlag());

  lastMissTime = globalClock;
}

void FetchEngine::clearMissInst(DInst *dinst, Time_t missFetchTime) {

  // MSG("\t\t\t\t\tCPU: %d\tU:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getID(),(int)
  // dinst->getID(),dinst->getPE(),dinst->getPC());
  I(missInst);
  missInst = false;

  I(dinst == missDInst);
#ifdef DEBUG
  missDInst = 0;
#endif

  cbPending.mycall();
}

void FetchEngine::setMissInst(DInst *dinst) {
  // MSG("CPU: %d\tL:ID: %d,DInst PE:%d, Dinst PC %x",(int) this->gproc->getID(),(int)
  // dinst->getID(),dinst->getPE(),dinst->getPC());

  I(!missInst);

  missInst = true;
#ifdef DEBUG
  missDInst = dinst;
#endif
}
