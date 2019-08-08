
#include <math.h>
#include <iostream>
#include <stdlib.h>

#include "DInst.h"
#include "MemObj.h"
#include "MemRequest.h"
#include "SescConf.h"

#include "AddressPredictor.h"

BimodalStride::BimodalStride(int _size)
    : size(_size) {

  if(!ISPOWER2(size)) {
    MSG("BimodalStride needs a power of 2 size %d", size);
    SescConf->notCorrect();
  }

  table.resize(size);
}

void BimodalLastEntry::update(int ndelta) {
  // No need to waste space for delta 0
  if(delta == ndelta) {
    if(conf < 63) {
      conf++;
    } else {
      if(conf2 > 16)
        conf2 /= 2;
      else if(conf2)
        conf2--;
    }
  } else {
    if(conf > 16)
      conf /= 2;
    else if(conf)
      conf--;

    if(delta2 == ndelta) {
      if(conf2 < 63)
        conf2++;
    } else {
      if(conf2 > 16)
        conf2 /= 2;
      else if(conf2)
        conf2--;
    }
    if(conf2 == 0)
      delta2 = ndelta;
  }

  if(conf < conf2) {
    // Swap deltas if delta2 is more confident
    int      tmp_delta = delta;
    uint16_t tmp_conf  = conf;

    delta = delta2;
    conf  = conf2;

    delta2 = tmp_delta;
    conf2  = tmp_conf;
  }

  I(conf >= conf2);
}

void BimodalStride::update(AddrType pc, AddrType naddr) {

  AddrType la     = table[get_index(pc)].addr;
  int      ndelta = naddr - la;

  table[get_index(pc)].addr = naddr;

  // printf("UPDATE pc:%llx addr:%llx last:%llx delta:%d\n", pc, naddr, la, ndelta);
  update_delta(pc, ndelta);
}

void BimodalStride::update_addr(AddrType pc, AddrType naddr) {
  table[get_index(pc)].addr = naddr;
}

void BimodalStride::update_delta(AddrType pc, int ndelta) {
#ifndef STRIDE_DELTA0
  if(ndelta == 0)
    return;
#endif

  table[get_index(pc)].update(ndelta);
}

/*************************************
STRIDE Address Predictor
**************************************/

StrideAddressPredictor::StrideAddressPredictor()
    : bimodal(1024) {
  // FIXME: use a size for # entries, and conf level
}

uint16_t StrideAddressPredictor::exe_update(AddrType pc, AddrType addr, DataType data) {

  bimodal.update(pc, addr);

  return bimodal.get_conf(pc);
}

uint16_t StrideAddressPredictor::ret_update(AddrType pc, AddrType addr, DataType data) {
  if(bimodal.get_conf(pc) > 16) //if conf > 16, the mark as prefetch
    return 1;

  return 0;
}

AddrType StrideAddressPredictor::predict(AddrType ppc, int distance, bool inLines) {

  if(bimodal.get_conf(ppc) < 2)
    return 0; // not predictable;

  int delta = bimodal.get_delta(ppc);
#ifndef STRIDE_DELTA0
  if(delta == 0 && !inLines)
    return 0;
#endif

  int offset = delta * distance;

  if(delta < 64 && inLines)
    offset = 64 * distance; // At least 1 cache line delta

  bool ld_tracking = ppc == 0x10006540 || ppc == 0x10006544;
  if(ld_tracking)
    printf("STRIDE ldpc:%x dist:%d addr:%llx delta:%d conf:%d\n", ppc, distance, bimodal.get_addr(ppc), delta,
           bimodal.get_conf(ppc));
  return bimodal.get_addr(ppc) + offset;
}

bool StrideAddressPredictor::try_chain_predict(MemObj *dl1, AddrType pc, int distance) {
  return false;
}

/**************************************

VTAGE

**************************************/

vtage::vtage(uint64_t _bsize, uint64_t _log2fetchwidth, uint64_t _bwidth, uint64_t _nhist)
    : bimodal(_bsize)
    , tagePrefetchBaseNum("tagePrefetchBaseNum(%d)", 0)
    , tagePrefetchHistNum("tagePrefetchHistNum(%d)", 0)
    , log2fetchwidth(_log2fetchwidth)
    , nhist(_nhist) {
  m      = new int[nhist + 1];
  TB     = new int[nhist + 1];
  logg   = new int[nhist + 1];
  GI     = new AddrType[nhist + 1];
  GTAG   = new AddrType[nhist + 1];
  gtable = new vtage_gentry *[nhist + 1];

  // geometric history length calculation for tagged tables
  m[0]     = 0;
  m[1]     = VTAGE_MINHIST;
  m[nhist] = VTAGE_MAXHIST;
  for(uint64_t i = 2; i <= nhist; i++) {
    m[i] = (int)(((double)VTAGE_MINHIST *
                  pow((double)(VTAGE_MAXHIST) / (double)VTAGE_MINHIST, (double)(i - 1) / (double)((nhist - 1)))) +
                 0.5);
  }

  for(uint64_t i = 1; i <= nhist; i++) {
    TB[i]   = VTAGE_TBITS + (i / 2);
    logg[i] = VTAGE_LOGG;
    GI[i]   = 0;
    GTAG[i] = 0;
  }

  for(uint64_t i = 1; i <= nhist; i++) {
    gtable[i] = new vtage_gentry[(1 << (logg[i])) + 1];
    for(int j = 0; j < (1 << logg[i]); j++) {
      gtable[i][j].allocate();
    }
  }

  use_alt_on_na = 0;
}

void vtage::setVtagePrediction(AddrType pc) {

  hit_bank = 0;
  alt_bank = 0;

  int hitpred_conf = 0;
  int altpred_conf = 0;

  for(uint64_t i = 1; i <= nhist; i++) {
    if(gtable[i][GI[i]].isHit()) {

      altpred_delta = hitpred_delta;
      altpred_conf  = hitpred_conf;
      alt_bank      = hit_bank;

      hitpred_delta = gtable[i][GI[i]].get_delta();
      hitpred_conf  = gtable[i][GI[i]].get_conf();
      hit_bank      = i;
    }
  }

  use_alt_pred = false;

  // computes the prediction and the alternate prediction
  if(hit_bank == 0) {
    I(alt_bank == 0);
    pred_delta    = bimodal.get_delta(pc);
    pred_conf     = bimodal.get_conf(pc);
    hitpred_delta = pred_delta;
    altpred_delta = pred_delta;
  } else {
    if(alt_bank == 0) {
      altpred_delta = bimodal.get_delta(pc);
      pred_conf     = bimodal.get_conf(pc);
    }

    if((use_alt_on_na < 0) || !gtable[hit_bank][GI[hit_bank]].conf_weak()) {
      pred_delta = hitpred_delta;
      pred_conf  = hitpred_conf;
    } else {
      pred_delta   = altpred_delta;
      pred_conf    = altpred_conf;
      use_alt_pred = true;
    }
  }
}

AddrType vtage::gindex(uint16_t offset, int bank) {

  AddrType pc = get_pc(lastBoundaryPC, offset);

  uint8_t delta_pos = last[get_last_index(pc)].last_delta_spec_pos;

  int bits  = logg[bank];
  int i_max = m[bank];
  if(i_max > 256)
    i_max = 256;

  uint16_t nbits = 0;
  AddrType sign  = pcSign(lastBoundaryPC);

  for(int i = 0; i < i_max; i++) {
    uint64_t oBits = 8;
    uint8_t  x     = delta_pos - i;
    uint64_t h     = last[get_last_index(pc)].last_delta[x] & ((1UL << 7) - 1);

    // Rotate
    uint64_t drop = sign >> (64 - oBits);
    sign          = sign ^ drop;
    sign          = (sign << oBits) + h;

    nbits += oBits;
  }

  if(bits > nbits) {
    int nfolds = bits / nbits;
    for(int i = 0; i < nfolds; i++) {
      sign = sign + (sign << (i * bits));
    }
  } else {
    int nfolds = nbits / bits;

    for(int i = 0; i < nfolds; i++) {
      sign = sign + (sign >> (i * bits));
    }
  }

  sign &= ((1 << (bits)) - 1);

  AddrType index = lastBoundaryPC ^ (lastBoundaryPC >> (bank + 1)) ^ (sign);

  return (index & ((1 << (logg[bank])) - 1));
}

void vtage::setVtageIndex(uint16_t offset) {

  hit_bank = 0;
  alt_bank = 0;

  GI[0] = (lastBoundaryPC >> 2) & ((1 << (logg[0])) - 1);
  for(uint64_t i = 1; i <= nhist; i++) {
    GI[i]   = gindex(offset, i);
    GTAG[i] = ((GI[i - 1] << (logg[i] / 2)) ^ GI[i - 1]) & ((1 << TB[i]) - 1);
  }
}

void vtage::fetchBoundaryLdOffset(AddrType pc) {

  uint16_t loff = get_offset(pc);

  for(uint64_t i = 1; i <= nhist; i++) {
    gtable[i][GI[i]].select(GTAG[i], loff);
  }
}

AddrType vtage::predict(AddrType pc, int distance, bool inLines) {

  if(bimodal.get_conf(pc) > 60) {
    int delta = bimodal.get_delta(pc);
#ifndef STRIDE_DELTA0
    if(delta == 0 && !inLines)
      return 0;
#endif

    int offset = delta * distance;

    if(delta < 64 && inLines)
      offset = 64 * distance; // At least 1 cache line delta

    tagePrefetchBaseNum.inc();
    return bimodal.get_addr(pc) + offset;
  }

  lastBoundaryPC = get_boundary(pc);

  uint16_t offset = get_offset(pc);
  AddrType addr   = last[get_last_index(pc)].addr;

  last[get_last_index(pc)].clear_spec(); // TODO: Reuse spec if possible

  for(int i = 0; i < distance; i++) {
    setVtageIndex(offset);
    fetchBoundaryLdOffset(pc);
    setVtagePrediction(pc);

    if(pred_conf < 8) {
      last[get_last_index(pc)].clear_spec();
      return 0; // Too far
    }

    last[get_last_index(pc)].spec_add(pred_delta);

#ifdef DEBUG_STRIDESO2
    if(pc == 0x12000e220)
      printf("dist:%d addr:%llx pred_delta:%d delta:%d conf:%d\n", i, addr, pred_delta,
             (int)addr - (int)last[get_last_index(pc)].addr, pred_conf);
#endif

    if(inLines && pred_delta < 64)
      pred_delta = 64;

    addr = addr + pred_delta;
  }
  tagePrefetchHistNum.inc();

  last[get_last_index(pc)].clear_spec();
  return addr;
}

bool vtage::try_chain_predict(MemObj *dl1, AddrType pc, int distance) {
  return false;
}

uint16_t vtage::exe_update(AddrType pc, AddrType addr, DataType data) {

  uint16_t offset = get_offset(pc);
  lastBoundaryPC  = get_boundary(pc);

  setVtageIndex(offset);

  int ndelta = last[get_last_index(pc)].get_delta(addr);

#ifndef STRIDE_DELTA0
  if(ndelta == 0)
    return 0;
#endif

#if 0
  if (ndelta>1024 || ndelta <1024) {
    last[get_last_index(pc)].reset(addr,pc);
    return 0;
  }
#endif

  setVtagePrediction(pc);

  updateVtage(pc, ndelta, offset);

  bimodal.update(pc, addr);

  last[get_last_index(pc)].add(addr, pc); // WARNING: also clears the spec history

#ifdef DEBUG_STRIDESO2
  if(pc == 0x12000e220)
    printf("pc:%llx addr:%llx pred_delta:%d delta:%d conf:%d a:%d h:%d\n", pc, addr, pred_delta, ndelta, pred_conf, alt_bank,
           hit_bank);
#endif

  return pred_conf;
}

uint16_t vtage::ret_update(AddrType pc, AddrType addr, DataType data) {
  return 0;
}

void vtage::updateVtage(AddrType pc, int ndelta, uint16_t loff) {

  bool correct = (pred_delta == ndelta);
  bool alloc   = !correct && (hit_bank < nhist);

  if(bimodal.get_conf(pc) > 60)
    alloc = false; // Do not alloc if bimodal is highly confident

  if(bimodal.get_conf(pc) > 16)
    if(bimodal.get_delta(pc) == ndelta)
      alloc = false; // Do not learn deltas that match bimodal

  if(hit_bank > 0 && use_alt_pred) {
    if(altpred_delta == pred_delta)
      alloc = false;
  }

  if(alloc) {
    int penalty = 0;
    int T       = 1;
    int NA      = 0;

    int weakBank = hit_bank + 1;

    // First try tag (but not offset hit)
    for(uint64_t i = weakBank; i <= nhist; i += 1) {

      if(gtable[i][GI[i]].isTagHit()) {
        weakBank = i;

        if(!gtable[i][GI[i]].conf_steal())
          continue;

        NA++;
        T--;
        if(T <= 0)
          break;
      }
    }

    if(T > 0) {
      weakBank = hit_bank + 1;
      for(uint64_t i = weakBank; i <= nhist; i += 1) {
        if(gtable[i][GI[i]].u_get() == 0) {
          weakBank = i;
          if(gtable[i][GI[i]].isTagHit()) {
            gtable[i][GI[i]].conf_force_steal(ndelta);
          } else {
            gtable[i][GI[i]].reset(GTAG[i], loff, ndelta);
          }

          NA++;

          if(T <= 0)
            break;

          i += 1;
          T -= 1;

        } else {
          penalty++;
        }
      }
    }

    // Could not find a place to allocate
    TICK += (penalty - NA);
    if(TICK < -127)
      TICK = -127;
    else if(TICK > 63)
      TICK = 63;

    if(T) {
      if(TICK > 0) {
        for(uint64_t i = 1; i <= nhist; i += 1) {
          int idx1 = GI[i];
          gtable[i][idx1].u_dec();
          TICK--;
        }
      }
    }
  }

  if(hit_bank) {
    if(gtable[hit_bank][GI[hit_bank]].isHit()) {
      if(!correct) {
        gtable[hit_bank][GI[hit_bank]].u_dec();
      }
    }
  }

  if(hit_bank > 0) {
    gtable[hit_bank][GI[hit_bank]].conf_update(ndelta);
    if(gtable[hit_bank][GI[hit_bank]].u_get() == 0 && alt_bank > 0) {
      gtable[alt_bank][GI[alt_bank]].conf_update(ndelta);
    }
    if(hitpred_delta != altpred_delta) { // hit_bank and alt_bank dissagree
      if(correct) {
        if(!use_alt_pred)
          gtable[hit_bank][GI[hit_bank]].u_inc();
      } else {
        gtable[hit_bank][GI[hit_bank]].u_dec();
      }
    }
  }

#if 0
  // Always update bimodal to use the conf
  if (alt_bank==0) {
    bimodal.update_delta(pc,ndelta);
  }
#endif
}

void vtage::rename(DInst *dinst) {
}

void vtage_gentry::allocate() {
  conf  = 0;
  u     = 0;
  loff  = 0xFFFF;
  delta = 0;
  tag   = 0;
}

void vtage_gentry::select(AddrType t, int b) {

  if(t != tag) {
    hit  = false;
    thit = false;
    return;
  }

  thit = true;
  hit  = (loff == b);
}

bool vtage_gentry::conf_steal() {

  if(hit)
    return true;

  if(!thit)
    return false;

  if(conf != -1 && conf != 0)
    return false;

  conf = 0;
  hit  = thit;

  return true;
}

void vtage_gentry::conf_force_steal(int ndelta) {
  if(!thit)
    return;

  delta = ndelta;
  conf  = 0;
  hit   = thit;

  u_dec();
}

void vtage_gentry::reset(AddrType t, uint16_t _loff, int ndelta) {
  tag = t;

  loff  = _loff;
  delta = ndelta;
  conf  = 0;

  hit  = true;
  thit = true;
}

void vtage_gentry::conf_update(int ndelta) {
  if(!thit)
    return;

  if(delta == ndelta) {
    if(conf < 63) {
      conf++;
    }
  } else {
    if(conf > 16)
      conf /= 2;
    else if(conf)
      conf--;

    if(conf == 0) {
      delta = ndelta;
    }
  }
}

/*************************************
Indirect Address Predictor
**************************************/

class PredictableEntry {
public:
  bool     predictable;
  AddrType addr;
  DataType data;
  AddrType data_delta;
  int64_t  base;
  uint8_t  factor;
  uint8_t  conf;
  AddrType source_pc;
};
std::map<AddrType, PredictableEntry> adhist; // Address/Data History

std::vector<AddrType> last_pcs; // Last n PCs
int                   last_pcs_pos;

IndirectAddressPredictor::IndirectAddressPredictor()
    : bimodal(1024) {
  // FIXME: use a size for # entries, and conf level

  const char *section = SescConf->getCharPtr("cpusimu", "prefetcher", 0); // FIXME: use conf
  maxPrefetch         = SescConf->getInt(section, "maxPrefetch", 0);

  chainPredict = true;
  last_pcs.resize(16);
  last_pcs_pos = 0;
}

#define PTRACE(a...)
//#define PTRACE(a...)   do{ if () {fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0)
//#define PTRACE(a...)   do{ if (true) {fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0)
//#define PTRACE(a...)   do{ if ( (pc == 0x120081c24 || pc == 0x120081c1c)) {fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0)
//#define PTRACE(a...)   do{ if ( (pc >= 0x120081c18 && pc <= 0x120081c30)) {fprintf(stderr,##a); fprintf(stderr,"\n"); } }while(0)

uint16_t IndirectAddressPredictor::exe_update(AddrType pc, AddrType addr, DataType data) {

  // Same as base bimodal

  // FIXME: update at ret, but use the addr in predict
  bimodal.update(pc, addr);

  uint16_t conf = bimodal.get_conf(pc);

  // PTRACE("exe_update pc:%llx c:%d addr:%llx",pc,conf,addr);

  return conf;
}

extern "C" uint64_t esesc_mem_read(uint64_t addr);

uint16_t IndirectAddressPredictor::ret_update(AddrType pc, AddrType addr, DataType data) {

  if(!chainPredict)
    return 0;

  // Do nt update bimodal & exe & ret. 2 diff last_addr

  uint16_t conf = bimodal.get_conf(pc);

  // PTRACE("ret_update pc:%llx c:%d addr:%llx",pc,conf,addr);

  AddrType         last_addr = 0;
  PredictableEntry pa;
  if(adhist.find(pc) != adhist.end()) {
    pa             = adhist[pc];
    last_addr      = pa.addr;
    pa.data_delta  = data - pa.data;
    pa.addr        = addr;
    pa.data        = data;
    pa.predictable = conf > 60;
    if(pa.predictable)
      pa.source_pc = 0;
  } else {
    pa.data       = 0;
    pa.addr       = 0;
    pa.data_delta = 0;
    pa.source_pc  = 0;
    pa.factor     = 0;
    pa.base       = 0;
    pa.conf       = 0; // Not predictable when allocated
  }

  bool indirect = false;

  if(!pa.predictable && last_addr != addr && last_addr) {
    if(pa.source_pc) {

      // WARNING: do not use adhist[pa.source].data_delta because it can have many prev executions for a given current execution
      AddrType pred_addr = adhist[pa.source_pc].base + adhist[pa.source_pc].factor * adhist[pa.source_pc].data;

#if 0
      if (pa.base != adhist[pa.source_pc].base)
        MSG("INTERESTING: Base changes for pc:%llx source_pc:%llx from %llx to %llx",pc,pa.source_pc, pa.base, adhist[pa.source_pc].base);
      if (pa.factor != adhist[pa.source_pc].factor)
        MSG("INTERESTING: factor changes for pc:%llx source_pc:%llx from %d to %d",pc,pa.source_pc, pa.factor, adhist[pa.source_pc].factor);
#endif

      if(pred_addr == addr) {
        if(pa.conf < 63)
          pa.conf++;
        // PTRACE("pc:%llx predictable with addr:%llx pc:%llx c:%d factor:%d
        // base:%llx",pc,addr,pa.source_pc,pa.conf,adhist[pa.source_pc].factor, adhist[pa.source_pc].base);
      } else {
        if(pa.conf > 0)
          pa.conf--;
        // PTRACE("pc:%llx NOT predict with addr:%llx %llx pc:%llx c:%d",pc,addr,pred_addr,pa.source_pc,pa.conf);
      }
      if(pa.conf == 0) {
        if(pa.source_pc)
          adhist[pa.source_pc].factor = 0; // Disable indirect

        pa.source_pc = 0; // Try to re-learn again
      } else {
        indirect = true;
      }
    } else {
      for(size_t i = 0; i < last_pcs.size(); i++) {
        int pos = last_pcs_pos - i;
        if(pos < 0)
          pos = last_pcs.size() - 1;

        AddrType ppc = last_pcs[pos];
        if(!adhist[ppc].predictable)
          continue; // Only try to correlate predictable
        if(adhist[ppc].data_delta == 0)
          continue;

        for(int factor = 1; factor < 32; factor++) { // Try some easy to multiply factors (Power2)
          AddrType pred_addr = last_addr + factor * adhist[ppc].data_delta;
          if(pred_addr == addr) {
            AddrType base = addr - factor * adhist[ppc].data;
            // PTRACE("pc:%llx matches addr:%llx pc:%llx with la:%llx + factor:%d * d:%d,
            // base:%llx",pc,addr,ppc,last_addr,factor,adhist[ppc].data_delta,base);
            pa.source_pc = ppc;
            pa.conf      = 1;
#if 0
            pa.base      = base;
            pa.factor    = factor;
#endif
            adhist[ppc].factor = factor;
            adhist[ppc].base   = base;
            break;
          }
        }
        if(pa.source_pc)
          break;
      }
    }
  }

  adhist[pc] = pa;

  last_pcs_pos++;
  if(last_pcs_pos >= last_pcs.size())
    last_pcs_pos = 0;
  last_pcs[last_pcs_pos] = pc;

  return indirect ? 64 : 0;
}

AddrType IndirectAddressPredictor::predict(AddrType ppc, int distance, bool inLines) {
  if(chainPredict)
    return 0;

  if(bimodal.get_conf(ppc) < 2)
    return 0; // not predictable;

  int delta = bimodal.get_delta(ppc);
#ifndef STRIDE_DELTA0
  if(delta == 0 && !inLines)
    return 0;
#endif
  int offset = delta * distance;

  if(delta < 64 && inLines)
    offset = 64 * distance; // At least 1 cache line delta

  return bimodal.get_addr(ppc) + offset;
}

void IndirectAddressPredictor::performed(MemObj *DL1, AddrType pc, AddrType ld1_addr) {
  I(chainPredict);

  if(adhist.find(pc) == adhist.end())
    return;

  if(adhist[pc].data == 0 || adhist[pc].factor == 0)
    return;

  uint64_t ld1_data = esesc_mem_read(ld1_addr);
  if(adhist[pc].factor != 1)
    ld1_data = (uint32_t)ld1_data; // No long

  AddrType pred_addr = adhist[pc].base + (adhist[pc].factor * ld1_data);

  // MSG("trying %s pc:%llx cl:%llx b:%llx f:%d d:%d",DL1->getName(), (long long)pc,(long long)pred_addr,(long long)adhist[pc].base,
  // adhist[pc].factor, ld1_data);

#if 1
  DL1->tryPrefetch(pred_addr, false, 1, PSIGN_INDIRECT, pc); // degree 1 to give high priority
#else
  DL1->tryPrefetch(pred_addr, false, 1, PSIGN_INDIRECT, pc); // degree 1 to give high priority
#endif
}

bool IndirectAddressPredictor::try_chain_predict(MemObj *DL1, AddrType pc, int distance) {

  if(!chainPredict)
    return false;

  uint16_t conf = bimodal.get_conf(pc);
  if(conf < 2)
    return false; // not predictable;

  int offset = bimodal.get_delta(pc) * distance;
  if(offset == 0)
    return false;

  AddrType ld1_addr = bimodal.get_addr(pc) + offset;

  // PTRACE("try_chain pc:%llx conf:%d factor:%d dist:%d",pc, bimodal.get_conf(pc), adhist[pc].factor, distance);

  bool chain = true;
  if(adhist.find(pc) == adhist.end())
    chain = false;
  if(adhist[pc].data == 0 || adhist[pc].factor == 0)
    chain = false;

  CallbackBase *cb = 0;
  if(distance >= 2 && chain) {
    cb = performedCB::create(this, DL1, pc, ld1_addr);
  }
  DL1->tryPrefetch(ld1_addr, true, distance, PSIGN_STRIDE, pc, cb);

  return true;
}
