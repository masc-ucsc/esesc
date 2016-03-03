/*Copyright (c) <2006>, INRIA : Institut National de Recherche en Informatique et en Automatique (French National Research Institute for Computer Science and Applied Mathematics)
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/* author A. Seznec (2015) */
// this code is derived from the code of TAGE-SC-L by A. Seznec at CBP4
// it allows to reproduce the results in the IMLI  Micro 2015 paper on the CBP4 traces
// for more realistic design (and cleaner code), use predictorTAGE-GSC-IMLI.h

// total misprediction numbers
// TAGE-SC-L (CBP4 predictor): 2.365 MPKI
// TAGE-SC-L  + IMLI:  2.226 MPKI
// TAGE-GSC + IMLI: 2.313 MPKI
// TAGE-GSC : 2.473 MPKI
// TAGE alone: 2.563 MPKI


#define SC //use the statistical corrector: comment to get TAGE alone
#define LOCALH			// use local histories
#define LOOPPREDICTOR		//  use loop  predictor
#define IMLI			// using IMLI component
#define IMLISIC            //use IMLI-SIC
#define IMLIOH		//use IMLI-OH

//#define LARGE_SC
//#define STRICTSIZE
//uncomment to get the 256 Kbits record predictor mentioned in the paper achieves 2.228 MPKI

//#define POSTPREDICT
#define REALISTIC
//#define CHAMPIONSHIP
// uncomment to get a realistic predictor around 256 Kbits , with 12 1024 entries tagged tables in the TAGE predictor, and a global history and single local history GEHL statistical corrector
// total misprediction numbers
// TAGE-SC-L : 2.435 MPKI
// TAGE-SC-L  + IMLI:  2.294 MPKI
// TAGE-GSC + IMLI: 2.370 MPKI 
// TAGE-GSC : 2.531 MPKI
// TAGE alone: 2.602 MPKI

#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

//To get the predictor storage budget on stderr  uncomment the next line
#define PRINTSIZE
#include <inttypes.h>
#include <math.h>
#include <vector>
#include <inttypes.h>
#include <assert.h>

#include "InstOpcode.h"

#include "DOLC.h"

#define USE_DOLC 1

#ifdef REALISTIC 
#define LOGG 10 /* logsize of the  tagged TAGE tables*/
#define TBITS 8 /* minimum tag width*/
#define  POWER
//use geometric history length
#ifdef USE_DOLC
#define MAXHIST 192 // (128+32)
#define MINHIST 3
#else
//#define MINHIST 7
//#define MAXHIST 1000
#define MINHIST 3
#define MAXHIST 128
#endif
//probably not the best history length, but nice
#endif

#ifdef USE_DOLC
DOLC dolc(MAXHIST,3,9,16);
#endif

// number of tagged tables
#define NHIST 12
//#define NHIST 15

#ifndef STRICTSIZE
#define PERCWIDTH 6		//Statistical corrector maximum counter width
#else
#define PERCWIDTH 7 // appears as a reasonably efficient way to use the last available bits
#endif
//The statistical corrector components from CBP4

//global branch GEHL
#ifdef LARGE_SC
#define LOGGNB 9
#else
#define LOGGNB 10
#endif

#ifdef IMLI
#ifdef STRICTSIZE
#define GNB 2
int Gm[GNB] = { 17, 14 };
#else
#ifdef LARGE_SC
#define GNB 4
int Gm[GNB] = { 27,22, 17, 14 };
#else
#define GNB 2
int Gm[GNB] = {  17, 14 };
#endif
#endif
#else
#ifdef LARGE_SC
#define GNB 4
int Gm[GNB] = { 27,22, 17, 14 };
#else
#define GNB 2
int Gm[GNB] = {  17, 14 };
#endif

#endif
/*effective length is  -11,  we use (GHIST<<11)+IMLIcount; we force the IMLIcount zero when IMLI is not used*/


int8_t GGEHLA[GNB][(1 << LOGGNB)];
int8_t *GGEHL[GNB];

//large local history
#define  LOGLOCAL 8
#define NLOCAL (1<<LOGLOCAL)
#define INDLOCAL (PC & (NLOCAL-1))
#ifdef LARGE_SC
//three different local histories (just completely crazy :-)

#define LOGLNB 10
#define LNB 3
int Lm[LNB] = { 11, 6, 3 };
int8_t LGEHLA[LNB][(1 << LOGLNB)];
int8_t *LGEHL[LNB];
#else
//only one local history
#define LOGLNB 10
#define LNB 4
int Lm[LNB] = { 16, 11, 6, 3 };
int8_t LGEHLA[LNB][(1 << LOGLNB)];
int8_t *LGEHL[LNB];
#endif


// small local history
#define LOGSECLOCAL 4
#define NSECLOCAL (1<<LOGSECLOCAL)	//Number of second local histories
#define INDSLOCAL  (((PC ^ (PC >>5))) & (NSECLOCAL-1))
#define LOGSNB 9
#define SNB 4
int Sm[SNB] = { 16, 11, 6, 3 };
int8_t SGEHLA[SNB][(1 << LOGSNB)];
int8_t *SGEHL[SNB];


//third local history
#define LOGTNB 9
#ifdef STRICTSIZE
#define TNB 2
int Tm[TNB] = { 17, 14 };
#else
#define TNB 3
int Tm[TNB] = { 22, 17, 14 };
#endif
//effective local history size +11: we use IMLIcount + (LH) << 11
int8_t TGEHLA[TNB][(1 << LOGTNB)];
int8_t *TGEHL[TNB];
#define INDTLOCAL  (((PC ^ (PC >>3))) & (NSECLOCAL-1))	// different hash for the 3rd history


long long L_shist[NLOCAL];
long long S_slhist[NSECLOCAL];
long long T_slhist[NSECLOCAL];
long long HSTACK[16];
int pthstack;
#ifdef LARGE_SC
//return-stack associated history component
#ifdef STRICTSIZE
#define LOGPNB 8
#else
#define LOGPNB 9
#endif
#define PNB 4
int Pm[PNB] = { 16, 11, 6, 3 };
int8_t PGEHLA[PNB][(1 << LOGPNB)];
int8_t *PGEHL[PNB];
#else
//in this case we don´t use the call stack
#define PNB 2
#define LOGPNB 11
int Pm[PNB] = { 16, 11 };

int8_t PGEHLA[PNB][(1 << LOGPNB)];
int8_t *PGEHL[PNB];
#endif


//parameters of the loop predictor
#define LOGL 6
#define WIDTHNBITERLOOP 11	// we predict only loops with less than 1K iterations
#define LOOPTAG 12		//tag width in the loop predictor

//update threshold for the statistical corrector
#ifdef REALISTIC
#define LOGSIZEUP 0
#else
#define LOGSIZEUP 5
#endif
int Pupdatethreshold[(1 << LOGSIZEUP)];	//size is fixed by LOGSIZEUP
#define INDUPD (PC & ((1 << LOGSIZEUP) - 1))

// The three counters used to choose between TAGE ang SC on High Conf TAGE/Low Conf SC
int8_t FirstH, SecondH, ThirdH;
#define CONFWIDTH 7		//for the counters in the choser
#define PHISTWIDTH 27		// width of the path history used in TAGE

#define UWIDTH 2		// u counter width on TAGE
#define CWIDTH 5		// predictor counter width on the TAGE tagged tables

#define HISTBUFFERLENGTH 4096	// we use a 4K entries history buffer to store the branch history


//the counter(s) to chose between longest match and alternate prediction on TAGE when weak counters
//#define LOGSIZEUSEALT 0
#define LOGSIZEUSEALT 2
#define SIZEUSEALT  (1<<(LOGSIZEUSEALT))
#define INDUSEALT ((lastBoundaryPC>>2) & (SIZEUSEALT -1))


// utility class for index computation
// this is the cyclic shift register for folding 
// a long global history into a smaller number of bits; see P. Michaud's PPM-like predictor at CBP-1
class folded_history {
public:

  unsigned comp;
  int CLENGTH;
  int OLENGTH;
  int OUTPOINT;

  folded_history () {
  }

  void init(int original_length, int compressed_length, int N) {
    comp = 0;
    OLENGTH = original_length;
    CLENGTH = compressed_length;
    OUTPOINT = OLENGTH % CLENGTH;
  }

  void update (uint8_t * h, int PT) {
    comp = (comp << 1) ^ h[PT & (HISTBUFFERLENGTH - 1)];
    comp ^= h[(PT + OLENGTH) & (HISTBUFFERLENGTH - 1)] << OUTPOINT;
    comp ^= (comp >> CLENGTH);
    comp = (comp) & ((1 << CLENGTH) - 1);
  }

  void set(unsigned c) {
    comp = (c) & ((1 << CLENGTH) - 1);
  }
  void mix(unsigned c) {
    comp ^= (c) & ((1 << CLENGTH) - 1);
  }
};

#define CHAINENTRYSIZE 7

class ChainEntry {
public:

  int same_conf;
  int bias_conf;
  int mt_same_conf[2];
  int mt_bias_conf[2];

  ChainEntry() {
    bias_conf       = 0;
  }

  bool canPredict(bool master_taken) const { 
    return abs(bias_conf) == CHAINENTRYSIZE;
  };

  bool predict(bool master_taken) {
    if (bias_conf == CHAINENTRYSIZE)
      return true;
    if (bias_conf == -CHAINENTRYSIZE)
      return false;

    return false;
  }

  void dump(bool master_taken) {
    if (bias_conf == CHAINENTRYSIZE)
      printf("B_T ");
    if (bias_conf == -CHAINENTRYSIZE)
      printf("B_NT ");
  }

  void sinc(int &ctr) {
    if(ctr==CHAINENTRYSIZE)
      return;
    if(ctr<0)
      ctr = 0;
    else
      ctr++;
  }
  void sdec(int &ctr) {
    if(ctr==-CHAINENTRYSIZE)
      return;
    if (ctr>0)
      ctr = 0;
    else
      ctr--;
  }

  void updatectr(bool cond, int &ctr) {
    if (cond)
      sinc(ctr);
    else
      sdec(ctr);
  }

  void update(bool taken) {
    updatectr(taken, bias_conf);
  }

  void clear() {
    bias_conf = 0;
  }
};

#define LASTPREDSIZE 4
class ChainPrediction {
private:
protected:
public:
  ChainPrediction() {
  };
  ChainEntry ent[LASTPREDSIZE];
  void clear() {
    for(int i=0;i<LASTPREDSIZE;i++)
      ent[i].clear();
  }
};


class LastPrediction {
private:
  ChainPrediction **ctable;
  bool ahead_pred_used;
  bool ahead_pred;
protected:
public:
  struct LastEntry {
    int HitBank;
    int index;
    bool resolveDir;
    bool predDir;
    bool skip;
    bool highConf;

    LastEntry() {
      HitBank    = 0;
      index      = 0;
      resolveDir = false;
      predDir    = false;
      highConf   = false;
      skip       = true;
    };
  };
  LastEntry ent[LASTPREDSIZE];
  LastPrediction(ChainPrediction **ct) {
    ctable      = ct;
    ahead_pred_used = false;
    ahead_pred  = false;
  }

  void skipPred() {
    ahead_pred_used = false;
  }

  bool getPred(bool &valid) {
    skipPred();
    valid           = false;

#if 1
    return false; // disable chainpred
#endif

    int nvotes    = 0;
    int direction = 0; // >0 T, <0 NT

    int longest_bank = 0;
    for(int i=0;i<LASTPREDSIZE;i++) {
      if (ent[i].HitBank<1) // || ent[i].skip)
        continue; // No chain for bimodal

      if (longest_bank>ent[i].HitBank)
        continue;

      longest_bank = ent[i].HitBank;

      ChainPrediction &cp = ctable[ent[i].HitBank][ent[i].index];

      if (cp.ent[LASTPREDSIZE-1-i].canPredict(ent[i].resolveDir)) {
        nvotes++;
        if (cp.ent[LASTPREDSIZE-1-i].predict(ent[i].resolveDir))
          direction++;
        else
          direction--;
      }
    }

    if (nvotes!=abs(direction) || nvotes<2  || nvotes==0 || direction==0) {
      ahead_pred_used = false;
      valid = false;
      return false;
    }

    ahead_pred_used = true;
    ahead_pred = direction>0;
    valid = true;

    return ahead_pred;
  }

  bool add(int HitBank, bool resolveDir, bool predDir, int index, bool highConf) {
    LastEntry learn = ent[0];

    for(int i=0;i<LASTPREDSIZE-1;i++) {
      ent[i] = ent[i+1];
    }
    ent[LASTPREDSIZE-1].HitBank    = HitBank;
    ent[LASTPREDSIZE-1].resolveDir = resolveDir;
    ent[LASTPREDSIZE-1].predDir    = predDir;
    ent[LASTPREDSIZE-1].index      = index;
    ent[LASTPREDSIZE-1].highConf   = highConf;
    ent[LASTPREDSIZE-1].skip       = ahead_pred_used; // In real hardware setting HitBank=0 works, this is to simplify debug

    if (learn.HitBank) {
      for(int i=0;i<LASTPREDSIZE;i++) {
        // TODO: Can we move the learn further ahead, not wait 8 predictions late?
        ctable[learn.HitBank][learn.index].ent[i].update(ent[i].resolveDir);
      }
    }

#if 0
    if(ahead_pred_used) {
      printf("apred h=%d i=%d canPredict %s %s\n"
          ,ent[LASTPREDSIZE-1].HitBank
          ,ent[LASTPREDSIZE-1].index
          //,ent[LASTPREDSIZE-1].resolveDir == ent[LASTPREDSIZE-1].predDir?"agood":"abaad"
          ,ahead_pred == ent[LASTPREDSIZE-1].resolveDir?"agood":"abaad"
          ,ent[LASTPREDSIZE-1].resolveDir?"T":"NT"
          );
    }else{
      printf("npred h=%d i=%d canPredict %s %s\n"
          ,ent[LASTPREDSIZE-1].HitBank
          ,ent[LASTPREDSIZE-1].index
          ,ent[LASTPREDSIZE-1].resolveDir == ent[LASTPREDSIZE-1].predDir?"ngood":"nbaad"
          ,ent[LASTPREDSIZE-1].resolveDir?"T":"NT"
          );
    }
#endif

    if (!ahead_pred_used)
      return true;

    return ahead_pred != resolveDir;
  }
  
};

#ifdef LOOPPREDICTOR
class lentry			//loop predictor entry
{
public:
  uint16_t NbIter;		//10 bits
  uint8_t confid;		// 4bits
  uint16_t CurrentIter;		// 10 bits

  uint16_t TAG;			// 10 bits
  uint8_t age;			// 4 bits
  bool dir;			// 1 bit

  //39 bits per entry    
    lentry ()
  {
    confid = 0;
    CurrentIter = 0;
    NbIter = 0;
    TAG = 0;
    age = 0;
    dir = false;



  }

};
#endif


template <int BITS, int HSFT, int Log2FetchWidth, int bwidth>
class Bimodal {
private:
  uint8_t pred[1 << (BITS+Log2FetchWidth)];
  bool hyst[1 << ((BITS+Log2FetchWidth)-HSFT)];
  uint32_t getIndex(uint32_t pc, int shift=0) {
    return (pc & ((1 << (BITS+Log2FetchWidth))-1)) >> shift ;
  }

  static const uint8_t Max = (1<<bwidth)-1;
  int pos_p;
  int pos_h;
  
public:
  int init() {
    pos_p = 0;
    pos_h = 0;

    for(int i=0; i<(1<<(BITS+Log2FetchWidth)); i++) { 
      pred[i] = 0; 
    }
    for(int i=0; i<(1<<((BITS+Log2FetchWidth)-HSFT)); i++) { 
      hyst[i] = 1; 
    }

    return (1<<(BITS+Log2FetchWidth))+(1<<((BITS+Log2FetchWidth)-HSFT));
  }
  
  bool predict() const {
    return pred[pos_p] >= (1<<(bwidth-1));
  }
  bool highconf() const {
    if (pred[pos_p] == Max && hyst[pos_h])
      return true;
    if (pred[pos_p] == 0 && !hyst[pos_h])
      return true;

    return false;
  }

  void select(uint32_t fetchPC_, uint8_t boff_) {
    pos_p = getIndex((fetchPC_<<Log2FetchWidth) + (boff_ & ((1<<Log2FetchWidth)-1)));
    pos_h = getIndex((fetchPC_<<Log2FetchWidth) + (boff_ & ((1<<Log2FetchWidth)-1)), HSFT);
  }
  
  void update(bool taken) {
    int inter = (pred[pos_p] << 1) + hyst[pos_h];
    if(taken) {
      if (inter < ((1<<(bwidth+1))-1)) { inter++; }
    } else {
      if (inter > 0) { inter--; }
    }
    pred[pos_p] = inter>>1;
    hyst[pos_h] = ((inter & 1)==1);
  }
};

// TODO: Convert this class to GTable class that includes subtables inside
class gentry {
private:
  int       nsub;
  int8_t   *ctr;
  int8_t   *u;
  uint16_t *boff; // Signature per branch in the entry

#ifdef IMLI_STATS
  static int nuse[NHIST+1];
  static int nreset[NHIST+1];
  static int ncorrect[NHIST+1];
#endif


public:

  uint32_t tag;
  int      pos; // for branch offset select
  int      last_boff; 
  bool     thit;
  bool     hit;

  gentry() {
  }

  void print_stats() {
#ifdef IMLI_STATS
    double nuse_total = 1;
    for(int i = 1;i<=NHIST;i++) {
      nuse_total += nuse[i];
    }

    for(int i=1;i<=NHIST;i++) {
      printf("stats: table=%2d nuse=%4.1f%% (%4.1f%%) nreset=%5.4f"
          ,i
          ,100*nuse[i]/nuse_total
          ,100*((double)ncorrect[i])/(1.0+nuse[i]+ncorrect[i])
          ,nreset[i]/nuse_total
          );
      printf("\n");
    }
#endif
  }

  void allocate(int n) {
    nsub = n;
    ctr  = new int8_t[n+1]; // +1, last means unused
    u    = new int8_t[n+1];
    boff = new uint16_t[n+1];
    for(int i=0;i<=n;i++) {
      ctr[i]  = 0;
      u[i]    = 0;
      boff[i] = 0xFFFF;
    }
    tag = 0;
  }

  bool isHit() const { return hit; }
  bool isTagHit() const { return thit; }

  void select(AddrType t, int b) {
#ifndef CHAMPIONSHIP
    b = b>>1; // Drop lower bit 
#endif

    last_boff = b;
    if (t!=tag) {
      hit = false;
      thit = false;
      return;
    }

    thit = true;
    hit  = false;

    pos = nsub;
    for(int i=0;i<nsub;i++) {
      if (boff[i] == b) {
        pos = i;
        hit = true;
        break;
      }
    }

    if (pos == nsub) {
      hit = false;
      pos = nsub-1;
    }
  }

  void reset(int tableid, uint32_t t, bool taken) {
    tag = t;
#ifdef IMLI_STATS
    nreset[tableid]++;
#endif

    for(int i=0;i<nsub;i++) {
      boff[i] = 0xFFFF;
      ctr[i] = taken ? 0 : -1;
      u[i]    = 0;
    }
    pos = 0;
    boff[0] = pos;
    ctr[0] = taken ? 0 : -1;
  }

  bool ctr_steal(bool taken) {
    // Look for the ctr not highconf longest boff
 
    int p=nsub-1;
    bool weak=false;
    for(int i=0;i<nsub;i++) {
      if (boff[i] == 0xFFFF) {
        p    = i;
        weak = true;
        break;
      }
      if (ctr[i] == -1 || ctr[i] == 0) {
        p    = i;
        weak = true;
      }
    }
    if (!weak)
      return false;
     
    boff[p] = last_boff;
    ctr[p]  = taken ? 0 : -1;

    return true;
  }

  void ctr_update(int tableid, bool taken, int nbits) {
    if (!thit)
      return;
    if (!hit) {
      int freepos = nsub;

      for(int i=0;i<nsub;i++) {
        if (boff[i] == 0xFFFF) {
          freepos = i;
          break;
        }
      }
      if (freepos!=nsub) {
        boff[freepos] = last_boff;
        pos = freepos;
      }else{
        bool s = ctr_steal(taken);
        if (!s)
          return;
      }
      hit = true;
    }

#ifdef IMLI_STATS
    nuse[tableid]++;
    if (taken == ctr_isTaken())
      ncorrect[tableid]++;
#endif

    if (taken) {
      if (ctr[pos] < ((1 << (nbits - 1)) - 1))
        ctr[pos]++;
    } else {
      if (ctr[pos] > -(1 << (nbits - 1)))
        ctr[pos]--;
    }
  }

  bool ctr_weak() const {
    if (!hit)
      return true;

    return ctr[pos] == 0 || ctr[pos] == -1;
  }

  bool ctr_highconf() const {
    if (!hit)
      return false;

    return (abs(2 * ctr[pos] + 1) >= (1 << CWIDTH) - 1);
  }

  int ctr_get() const {
    if (!hit)
      return 0; // bias to taken if nothing is known

    return ctr[pos];
  }

  bool ctr_isTaken() const {
    return ctr_get() >= 0;
  }

  int u_get() const {
    return u[0];
  }

  void u_inc() {
    if (u[0] < (1 << UWIDTH) - 1)
      u[0]++;
  }

  void u_dec() {
    if (u[0])
      u[0]--;
  }

};

class IMLIBest {
public:

  IMLIBest() {
    reinit ();
#ifdef PRINTSIZE
    predictorsize ();
#endif
  }

  void print_stats() {
    gtable[0][0].print_stats();
  }

#ifdef POSTPREDICT
#define POSTPEXTRA 2
#define POSTPBITS 5
#define CTRBITS 3  // Chop 2 bits

  uint32_t postpsize;
  int8_t *postp;
  uint32_t ppi;

  uint32_t postp_index(uint32_t a, uint32_t b, uint32_t c) {
    int ctr[POSTPEXTRA+1];
    if (a)
      ctr[0] = gtable[a][GI[a]].ctr_get();
    else
      ctr[0] = bimodal.predict();
    if (b)
      ctr[1] = gtable[b][GI[b]].ctr_get();
    else
      ctr[1] = bimodal.predict();
    if (c)
      ctr[2] = gtable[c][GI[c]].ctr_get();
    else
      ctr[2] = bimodal.predict();

    for(int i=0;i<3;i++) {
      if (ctr[i] >= 3)
        ctr[i] = 3;
      else if (ctr[i] <= -4)
        ctr[i]= -4;
    }

    int v = 0;
    for (int i=POSTPEXTRA; i>=0; i--) {
      v = (v << CTRBITS) | (ctr[i] & (((1<<CTRBITS)-1)));
    }
    int u0 = (a>0)? gtable[a][GI[a]].u_get() : 1;
    v = (v << 1) | u0;
    v &= postpsize-1;
    return v;
  }
#endif


//IMLI related data declaration
long long IMLIcount;
#define MAXIMLIcount 1023
#ifdef IMLI
#ifdef IMLISIC
//IMLI-SIC related data declaration
#define LOGINB 10 // (LOG of IMLI-SIC table size +1)
#define INB 1
int Im[INB];
int8_t IGEHLA[INB][(1 << LOGINB)];
int8_t *IGEHL[INB];
#endif
//IMLI-OH related data declaration
#ifdef IMLIOH
long long localoh; //intermediate data to recover the two bits needed from the past outer iteration
#define SHIFTFUTURE 6 // (PC<<6) +IMLIcount to index the Outer History table
#define PASTSIZE 16
int8_t PIPE[PASTSIZE]; // the PIPE vector
#define OHHISTTABLESIZE 1024 //
int8_t ohhisttable[OHHISTTABLESIZE];
#ifdef STRICTSIZE
#define LOGFNB 7 // 64 entries
#else
#define LOGFNB 9 //256 entries
#endif
#define FNB 1
int Fm[FNB];
int8_t FGEHLA[FNB][(1 << LOGFNB)];
int8_t *FGEHL[FNB];
#endif
#endif

#ifndef POSTPREDICT
int8_t use_alt_on_na[SIZEUSEALT][2];
#endif

long long GHIST;
long long phist;		//path history

//The two BIAS tables in the SC component
#define LOGBIAS 7
int8_t Bias[(1 << (LOGBIAS + 1))];
#define INDBIAS ((((PC<<1) + pred_inter) ) & ((1<<(LOGBIAS+1)) -1))
int8_t BiasSK[(1 << (LOGBIAS + 1))];
#define INDBIASSK (((((PC^(PC>>LOGBIAS))<<1) + pred_inter) ) & ((1<<(LOGBIAS+1)) -1))

bool HighConf;
int LSUM;

int TICK;			// for the reset of the u counter


uint8_t ghist[HISTBUFFERLENGTH];
int ptghist;
folded_history ch_i[NHIST + 1];	//utility for computing TAGE indices
folded_history ch_t[2][NHIST + 1];	//utility for computing TAGE tags

//For the TAGE predictor
#define LOG2FETCHWIDTH 4
#define LOG2BIMODAL    4
#define HYSTSHIFT 2	
#define LOGB 10			// log of number of entries in bimodal predictor
#define BWIDTH  1

Bimodal<LOGB,HYSTSHIFT,LOG2BIMODAL,BWIDTH> bimodal;

gentry *gtable[NHIST + 1];	// tagged TAGE tables
ChainPrediction *ctable[NHIST + 1];	// Chain Table (field in the TAGE entries)
LastPrediction *last_pred;


int m[NHIST + 1];	// history lengths
int TB[NHIST + 1]; 	// tag width for the different tagged tables
int logg[NHIST + 1];	// log of number entries of the different tagged tables
int GI[NHIST + 1];		// indexes to the different tables are computed only once  
uint GTAG[NHIST + 1];		// tags for the different tables are computed only once  
bool pred_taken;		// prediction
bool alttaken;			// alternate  TAGEprediction
bool tage_pred;			// TAGE prediction
bool LongestMatchPred;
int HitBank;			// longest matching bank
int AltBank;			// alternate matching bank
uint64_t lastBoundaryPC;	// last PC that fetchBoundary was called
uint64_t lastBoundarySign;	 // lastBoundaryPC ^ PCs in not-taken in the branch bundle
int Seed;			// for the pseudo-random number generator

bool pred_inter;

#ifdef LOOPPREDICTOR
lentry *ltable;			//loop predictor table
//variables for the loop predictor
bool predloop;			// loop predictor prediction
int LIB;
int LI;
int LHIT;			//hitting way in the loop predictor
int LTAG;			//tag on the loop predictor
bool LVALID;			// validity of the loop predictor prediction
int8_t WITHLOOP;		// counter to monitor whether or not loop prediction is beneficial
#endif

int predictorsize() {

  int STORAGESIZE = 0;
  int inter = 0;

  for (int i = 1; i <= NHIST; i += 1) {
    int s = logg[i];
    int x = (1 << s) * (CWIDTH + UWIDTH + TB[i]);
    fprintf(stderr,"table[%d] size=%d log2entries=%d histlength=%d taglength=%d\n",i,x,s,m[i], TB[i]);

    STORAGESIZE += x;
  }

  STORAGESIZE += 2 * (SIZEUSEALT) * 4;
  fprintf (stderr, " altna size=%d log2entries=%d\n", 2*(SIZEUSEALT)*4,LOGSIZEUSEALT);

  inter = BWIDTH*(1 << (LOG2FETCHWIDTH+LOGB)) + (1 << (LOG2FETCHWIDTH+LOGB - HYSTSHIFT));
  fprintf (stderr, " bimodal table size=%d log2entries=%d\n", inter,LOGB);

  STORAGESIZE += inter;
  STORAGESIZE += m[NHIST];
  STORAGESIZE += PHISTWIDTH;
  STORAGESIZE += 10;		//the TICK counter

  fprintf (stderr, " (TAGE %d) ", STORAGESIZE);

#ifdef LOOPPREDICTOR
  inter = (1 << LOGL) * (2 * WIDTHNBITERLOOP + LOOPTAG + 4 + 4 + 1);
  fprintf (stderr, " (LOOP %d) ", inter);
  STORAGESIZE += inter;
#endif

#ifdef SC
  inter =0;
  
  inter += 16;			//global histories for SC
  inter = 8 * (1 << LOGSIZEUP);	//the update threshold counters
  inter += (PERCWIDTH) * 4 * (1 << (LOGBIAS));
  inter +=
    (GNB - 2) * (1 << (LOGGNB)) * (PERCWIDTH - 1) +
    (1 << (LOGGNB - 1)) * (2 * PERCWIDTH - 1);

  inter +=
    (PNB - 2) * (1 << (LOGPNB)) * (PERCWIDTH - 1) +
    (1 << (LOGPNB - 1)) * (2 * PERCWIDTH - 1);

#ifdef LOCALH
  inter +=
    (LNB - 2) * (1 << (LOGLNB)) * (PERCWIDTH - 1) +
    (1 << (LOGLNB - 1)) * (2 * PERCWIDTH - 1);
  inter += NLOCAL * Lm[0];

#ifdef REALISTIC
  inter +=
    (SNB - 2) * (1 << (LOGSNB)) * (PERCWIDTH - 1) +
    (1 << (LOGSNB - 1)) * (2 * PERCWIDTH - 1);
  inter +=
    (TNB - 2) * (1 << (LOGTNB)) * (PERCWIDTH - 1) +
    (1 << (LOGTNB - 1)) * (2 * PERCWIDTH - 1);
  inter += 16 * 16;		// the history stack
  inter += 4;			// the history stack pointer

  inter += NSECLOCAL * Sm[0];
  inter += NSECLOCAL * (Tm[0] - 11);
/* Tm[0] is artificially increased by 11 to accomodate IMLI*/
#endif


#endif


#ifdef IMLI
#ifdef IMLIOH
  inter += OHHISTTABLESIZE;
  inter += PASTSIZE;
/*the PIPE table*/
// in cases you add extra tables to IMLI OH, the formula is correct
  switch (FNB)
    {
    case 1:
      inter += (1 << (LOGFNB - 1)) * PERCWIDTH;
      break;
    default: inter +=
	(FNB - 2) * (1 << (LOGFNB)) * (PERCWIDTH - 1) +
	(1 << (LOGFNB - 1)) * (2 * PERCWIDTH-1);
    }
#endif
#ifdef IMLISIC  // in cases you add extra tables to IMLI SIC, the formula is correct
  switch (INB)
    {
    case 1:
      inter += (1 << (LOGINB - 1)) * PERCWIDTH;
      break;

    default:
      inter +=
	(INB - 2) * (1 << (LOGINB)) * (PERCWIDTH - 1) +
	(1 << (LOGINB - 1)) * (2 * PERCWIDTH-1);
    }
#endif

#endif

  inter += 3 * CONFWIDTH;	//the 3 counters in the choser
  STORAGESIZE += inter;


  fprintf (stderr, " (SC %d) ", inter);
#endif
#ifdef PRINTSIZE
  fprintf (stderr, " (TOTAL %d) ", STORAGESIZE);
#endif

  return (STORAGESIZE);
}

void reinit() {

#ifndef REALISTIC
 int tmp_m[] = { 0, 6, 10, 18, 25, 35, 55, 69, 105, 155, 230, 354, 479, 642, 1012, 1347 };
 int tmp_TB[] = { 0, 7, 9, 9, 9, 10, 11, 11, 12, 12, 12, 13, 14, 15, 15, 15 };	// tag width for the different tagged tables
 int tmp_logg[] = { 0, 10, 10, 10, 11, 10, 10, 10, 10, 10, 9, 9, 9, 8, 7, 7 };	// ORIG

 //int tmp_m[]    = { 0, 6, 18, 35, 69, 105};
 //int tmp_TB[]   = { 0, 9, 10, 11, 12, 13};
 //int tmp_logg[] = { 0, 9, 9 , 10,  9, 9};
#endif

#ifdef POSTPREDICT
  postpsize = 1 << ((1+POSTPEXTRA)*CTRBITS+1);
  postp = new int8_t [postpsize];
  for (int i=0; i<postpsize; i++) {
    postp[i] = -(((i>>1) >> (CTRBITS-1)) & 1);
  }
#endif

#ifdef IMLISIC
 for(int i=0;i<INB;i++)
   Im[i] = 10; // the IMLIcounter is limited to 10 bits
#endif

#ifdef IMLIOH
 for(int i=0;i<FNB;i++)
   Fm[i] = 2; 
#endif

#ifndef REALISTIC
 for(int i=0;i<=NHIST;i++) {
   m[i]    = tmp_m[i];
   TB[i]   = tmp_TB[i];
   logg[i] = tmp_logg[i];
 }
#endif

#ifdef POWER

    m[1] = MINHIST;
    m[NHIST] = MAXHIST;
    for (int i = 2; i <= NHIST; i++)
      {
	m[i] =
	  (int) (((double) MINHIST *
		  pow ((double) (MAXHIST) / (double) MINHIST,
		       (double) (i - 1) / (double) ((NHIST - 1)))) + 0.5);
      }
#endif
#ifdef REALISTIC
    for (int i = 1; i <= NHIST; i++)
    {
         TB[i]= TBITS + (i/2);
         logg[i]= LOGG;
         
    }
#endif    
#ifdef LOOPPREDICTOR
    ltable = new lentry[1 << (LOGL)];
#endif

    int galloc[]= {0, 4, 4, 3, 3, 2, 2};
    int ngalloc =6;

    for (int i = 1; i <= NHIST; i++) {
      gtable[i] = new gentry[1 << (logg[i])];
      for (int j = 0; j < (1<<logg[i]); j++) {
        int s;
        if (i>=ngalloc)
          s = galloc[ngalloc];
        else
          s = galloc[i];
        gtable[i][j].allocate(s);
      }
      ctable[i] = new ChainPrediction[1 << (logg[i])];
    }
    last_pred = new LastPrediction(ctable);

    for (int i = 1; i <= NHIST; i++)
      {
	ch_i[i].init (m[i], (logg[i]), i - 1);
	ch_t[0][i].init (ch_i[i].OLENGTH, TB[i], i);
	ch_t[1][i].init (ch_i[i].OLENGTH, TB[i] - 1, i + 2);
      }
#ifdef LOOPPREDICTOR
    LVALID = false;
    WITHLOOP = -1;
#endif
    Seed = 0;

    TICK = 0;
    phist = 0;
    Seed = 0;

    for (int i = 0; i < HISTBUFFERLENGTH; i++)
      ghist[0] = 0;
    ptghist = 0;

    for (int i = 0; i < (1 << LOGSIZEUP); i++)
      Pupdatethreshold[i] = 35;

    for (int i = 0; i < GNB; i++)
      GGEHL[i] = &GGEHLA[i][0];
    for (int i = 0; i < LNB; i++)
      LGEHL[i] = &LGEHLA[i][0];

    for (int i = 0; i < GNB; i++)
      for (int j = 0; j < ((1 << LOGGNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      GGEHL[i][j] = -1;

	    }
	}
    for (int i = 0; i < LNB; i++)
      for (int j = 0; j < ((1 << LOGLNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      LGEHL[i][j] = -1;

	    }
	}
    for (int i = 0; i < PNB; i++)
      PGEHL[i] = &PGEHLA[i][0];
    for (int i = 0; i < PNB; i++)
      for (int j = 0; j < ((1 << LOGPNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      PGEHL[i][j] = -1;

	    }
	}

#ifdef IMLI
#ifdef IMLIOH
    for (int i = 0; i < FNB; i++)
      FGEHL[i] = &FGEHLA[i][0];
    for (int i = 0; i < FNB; i++)
      for (int j = 0; j < ((1 << LOGFNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      FGEHL[i][j] = -1;

	    }
	}
#endif
#ifdef IMLISIC
    for (int i = 0; i < INB; i++)
      IGEHL[i] = &IGEHLA[i][0];
    for (int i = 0; i < INB; i++)
      for (int j = 0; j < ((1 << LOGINB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      IGEHL[i][j] = -1;

	    }

	}
#endif    
#endif
#ifndef REALISTIC
    for (int i = 0; i < SNB; i++)
      SGEHL[i] = &SGEHLA[i][0];
    for (int i = 0; i < TNB; i++)
      TGEHL[i] = &TGEHLA[i][0];

    for (int i = 0; i < SNB; i++)
      for (int j = 0; j < ((1 << LOGSNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      SGEHL[i][j] = -1;

	    }
	}
    for (int i = 0; i < TNB; i++)
      for (int j = 0; j < ((1 << LOGTNB) - 1); j++)
	{
	  if (!(j & 1))
	    {
	      TGEHL[i][j] = -1;

	    }
	}
#endif

    bimodal.init();

    for (int j = 0; j < (1 << (LOGBIAS + 1)); j++)
      Bias[j] = (j & 1) ? 15 : -16;
    for (int j = 0; j < (1 << (LOGBIAS + 1)); j++)
      BiasSK[j] = (j & 1) ? 15 : -16;


    for (int i = 0; i < NLOCAL; i++)
      {
	L_shist[i] = 0;
      }


    for (int i = 0; i < NSECLOCAL; i++)
      {
	S_slhist[i] = 0;

      }
    GHIST = 0;

#ifndef POSTPREDICT
    for (int i = 0; i < SIZEUSEALT; i++) {
      use_alt_on_na[i][0] = 0;
      use_alt_on_na[i][1] = 0;
    }
#endif

    TICK = 0;
    ptghist = 0;
    phist = 0;

  }
  // index function for the bimodal table

  int bindex(AddrType PC) {
    return ((PC) & ((1 << (LOGB)) - 1));
  }


// the index functions for the tagged tables uses path history as in the OGEHL predictor
//F serves to mix path history: not very important impact

  int F(long long A, int size, int bank) {
    int A1, A2;
    A = A & ((1 << size) - 1);
    A1 = (A & ((1 << logg[bank]) - 1));
    A2 = (A >> logg[bank]);
    A2 =
      ((A2 << bank) & ((1 << logg[bank]) - 1)) + (A2 >> (logg[bank] - bank));
    A = A1 ^ A2;
    A = ((A << bank) & ((1 << logg[bank]) - 1)) + (A >> (logg[bank] - bank));
    return (A);
  }

// gindex computes a full hash of PC, ghist and phist
  int gindex(unsigned int PC, int bank, long long hist, folded_history * ch_i, bool use_dolc_next) {
    int index;
#ifdef USE_DOLC
    // Both IFDEF do the same for dolc, but this is cleaner
    //index = PC ^ (PC >> (abs (logg[bank] - bank) + 1)) ^ ch_i[bank].comp;
    uint64_t sign1;
    sign1 = dolc.getSign(logg[bank], m[bank]);
    index = PC ^ (PC >> (abs (logg[bank] - bank) + 1)) ^ sign1;
#else
    int M = (m[bank] > PHISTWIDTH) ? PHISTWIDTH : m[bank];
    index =
      PC ^ (PC >> (abs (logg[bank] - bank) + 1))
      ^ ch_i[bank].comp ^ F (hist, M, bank);
#endif
    return (index & ((1 << (logg[bank])) - 1));

  }

  //  tag computation
  uint16_t gtag (unsigned int PC, int bank, folded_history * ch0, folded_history * ch1) {
    int tag = PC ^ ch0[bank].comp ^ (ch1[bank].comp << 1);
    return (tag & ((1 << TB[bank]) - 1));
  }

  // up-down saturating counter
  void ctrupdate (int8_t & ctr, bool taken, int nbits) {
    if (taken) {
      if (ctr < ((1 << (nbits - 1)) - 1))
        ctr++;
    } else {
      if (ctr > -(1 << (nbits - 1)))
        ctr--;
    }
  }

#ifdef LOOPPREDICTOR
  int lindex (AddrType PC) {
    return ((PC & ((1 << (LOGL - 2)) - 1)) << 2);
  }


//loop prediction: only used if high confidence
//skewed associative 4-way
//At fetch time: speculative
#define CONFLOOP 15
  bool getloop (AddrType PC)
  {
    LHIT = -1;

    LI = lindex (PC);
    LIB = ((PC >> (LOGL - 2)) & ((1 << (LOGL - 2)) - 1));
    LTAG = (PC >> (LOGL - 2)) & ((1 << 2 * LOOPTAG) - 1);
    LTAG ^= (LTAG >> LOOPTAG);
    LTAG = (LTAG & ((1 << LOOPTAG) - 1));

    for (int i = 0; i < 4; i++)
      {
	int index = (LI ^ ((LIB >> i) << 2)) + i;

	if (ltable[index].TAG == LTAG)
	  {
	    LHIT = i;
	    LVALID = ((ltable[index].confid == CONFLOOP)
		      || (ltable[index].confid * ltable[index].NbIter > 128));
	    {

	    }
	    if (ltable[index].CurrentIter + 1 == ltable[index].NbIter)
	      return (!(ltable[index].dir));
	    else
	      {
		
		return ((ltable[index].dir));
	      }

	  }
      }

    LVALID = false;
    return (false);

  }

  void loopupdate (AddrType PC, bool Taken, bool ALLOC) {
    if (LHIT >= 0)
      {
	int index = (LI ^ ((LIB >> LHIT) << 2)) + LHIT;
//already a hit 
	if (LVALID)
	  {
	    if (Taken != predloop)
	      {
// free the entry
		ltable[index].NbIter = 0;
		ltable[index].age = 0;
		ltable[index].confid = 0;
		ltable[index].CurrentIter = 0;
		return;

	      }
	    else if ((predloop != tage_pred) || ((MYRANDOM () & 7) == 0))
	      if (ltable[index].age < CONFLOOP)
		ltable[index].age++;
	  }

	ltable[index].CurrentIter++;
	ltable[index].CurrentIter &= ((1 << WIDTHNBITERLOOP) - 1);
	//loop with more than 2** WIDTHNBITERLOOP iterations are not treated correctly; but who cares :-)
	if (ltable[index].CurrentIter > ltable[index].NbIter)
	  {
	    ltable[index].confid = 0;
	    ltable[index].NbIter = 0;
//treat like the 1st encounter of the loop 
	  }
	if (Taken != ltable[index].dir)
	  {
	    if (ltable[index].CurrentIter == ltable[index].NbIter)
	      {
		if (ltable[index].confid < CONFLOOP)
		  ltable[index].confid++;
		if (ltable[index].NbIter < 3)
		  //just do not predict when the loop count is 1 or 2     
		  {
// free the entry
		    ltable[index].dir = Taken;
		    ltable[index].NbIter = 0;
		    ltable[index].age = 0;
		    ltable[index].confid = 0;
		  }
	      }
	    else
	      {
		if (ltable[index].NbIter == 0)
		  {
// first complete nest;
		    ltable[index].confid = 0;
		    ltable[index].NbIter = ltable[index].CurrentIter;
		  }
		else
		  {
//not the same number of iterations as last time: free the entry
		    ltable[index].NbIter = 0;
		    ltable[index].confid = 0;
		  }
	      }
	    ltable[index].CurrentIter = 0;
	  }

      }
    else if (ALLOC)

      {
	AddrType X = MYRANDOM () & 3;

	if ((MYRANDOM () & 3) == 0)
	  for (int i = 0; i < 4; i++)
	    {
	      int LHIT = (X + i) & 3;
	      int index = (LI ^ ((LIB >> LHIT) << 2)) + LHIT;
	      if (ltable[index].age == 0)
		{
		  ltable[index].dir = !Taken;
// most of mispredictions are on last iterations
		  ltable[index].TAG = LTAG;
		  ltable[index].NbIter = 0;
		  ltable[index].age = 7;
		  ltable[index].confid = 0;
		  ltable[index].CurrentIter = 0;
		  break;

		}
	      else
		ltable[index].age--;
	      break;
	    }
      }
  }
#endif

//just a simple pseudo random number generator: use available information
// to allocate entries  in the loop predictor
  int MYRANDOM() {
    Seed++;
    Seed ^= phist;
    Seed = (Seed >> 21) + (Seed << 11);

    return Seed;
  }

  void setTAGEIndex() {

    HitBank = 0;
    AltBank = 0;

    GI[0] = lastBoundaryPC;
    for (int i = 1; i <= NHIST; i++) {
      GI[i]   = gindex(lastBoundaryPC, i, phist, ch_i, false);
      GTAG[i] = ((GI[i-1]<<logg[i]) | GI[i-1]) & ((1 << TB[i]) - 1);
    }
  }

  uint64_t pcSign(uint64_t pc) const {
    uint64_t cid = pc>>2; 
    cid = (cid >> 7) ^ (cid); 
    return cid;
  }

  void setTAGEPred() { 

    for (int i = 1; i <= NHIST; i++) {
      if (gtable[i][GI[i]].isHit()) {
        LongestMatchPred = (gtable[i][GI[i]].ctr_isTaken());
        HitBank = i;
      }
    }

    for (int i = HitBank - 1; i > 0; i--) {
      if (gtable[i][GI[i]].isHit()) {
        AltBank = i;
        break;
      }
    }

#ifdef POSTPREDICT
    int WeakBank = 0;
    for (int i = AltBank - 1; i > 0; i--) {
      if (gtable[i][GI[i]].isHit()) {
        WeakBank = i;
        break;
      }
    }

    if (HitBank > 0) {
      if (AltBank > 0)
        alttaken = (gtable[AltBank][GI[AltBank]].ctr_isTaken());
      else
        alttaken = bimodal.predict();
    }else{
      alttaken = bimodal.predict();
      LongestMatchPred = alttaken;
    }
#else
//computes the prediction and the alternate prediction
    if (HitBank > 0) {

      if (AltBank > 0)
        alttaken = (gtable[AltBank][GI[AltBank]].ctr_isTaken());
      else
        alttaken = bimodal.predict();

      //if the entry is recognized as a newly allocated entry and 
      //USE_ALT_ON_NA is positive  use the alternate prediction
      int index = INDUSEALT ^ LongestMatchPred;
      bool Huse_alt_on_na = (use_alt_on_na[index][HitBank > (NHIST / 3)] >= 0);

      if (!Huse_alt_on_na || !gtable[HitBank][GI[HitBank]].ctr_weak())
        tage_pred = LongestMatchPred;
      else
        tage_pred = alttaken;

      HighConf = gtable[HitBank][GI[HitBank]].ctr_highconf();

    } else {
      HighConf = bimodal.highconf();
      alttaken = bimodal.predict();
      tage_pred = alttaken;
      LongestMatchPred = alttaken;
    }
#if 0
    static int conta_h=0;
    static int conta_l=0;
    if (HighConf)
      conta_h++;
    else
      conta_l++;

    if ((conta_h&0xFFFF)==0) {
      printf("High conf %d, low conf %d\n", conta_h, conta_l);
    }
#endif
#endif

#ifdef POSTPREDICT
    ppi = postp_index(HitBank,AltBank,WeakBank);
    assert(ppi<postpsize);
    //printf("postp[%d]=%d\n", ppi, postp[ppi]);
    tage_pred = (postp[ppi] >= 0);
#endif
  }
//compute the prediction


  void fetchBoundary(AddrType PC) {
#ifdef USE_DOLC
    dolc.update(lastBoundarySign^ pcSign(PC));
#endif

    lastBoundaryPC   = PC;
    lastBoundarySign = 0;

    setTAGEIndex();
  }

  uint32_t dohash(uint32_t addr, uint16_t offset) {
    uint32_t drop  = addr>>(LOGB-(LOG2FETCHWIDTH/2));
    uint32_t sign1 = addr ^ drop; //  ^ (drop<<(LOGB/2));
    uint32_t sign2 = (sign1<<(LOG2FETCHWIDTH/2)) ^ offset;
    
    return sign2;
  }

  int fetchBoundaryOffsetOthers(AddrType PC) {
    int boff = (PC-lastBoundaryPC)>>2; // compute branch offset (0,1,2,3,4...)
    if (boff)
      lastBoundarySign = dohash(lastBoundarySign, boff);

    return boff;
  }

  void fetchBoundaryOffsetBranch(AddrType PC) {

    int boff = fetchBoundaryOffsetOthers(PC);

    bimodal.select(lastBoundaryPC>>2,boff>>(LOG2FETCHWIDTH-LOG2BIMODAL));

    for (int i = 1; i <= NHIST; i++) {
      gtable[i][GI[i]].select(GTAG[i], boff);
    }
  }

  bool getPrediction(AddrType PC) {

    fetchBoundaryOffsetBranch(PC);
    setTAGEPred();

    pred_taken = tage_pred;

#ifdef LOOPPREDICTOR
    predloop = getloop (PC);	// loop prediction
    pred_taken = ((WITHLOOP >= 0) && (LVALID)) ? predloop : pred_taken;  
#endif

    pred_inter = pred_taken;

    if (HitBank==0 && false) {
      last_pred->skipPred();
    }else{
      bool last_pred_valid;
      bool last_pred_taken = last_pred->getPred(last_pred_valid);

      if (last_pred_valid) {
        tage_pred  = last_pred_taken; // Otherwise, tage updates can be triggered
        pred_taken = last_pred_taken;
      }
    }
  
#ifndef SC
    //just the TAGE predictor
    return(pred_taken);
#endif    
//Compute the SC prediction

    LSUM=0;
////// Very marginal effect    
// begin to bias the sum towards TAGE predicted direction
   LSUM = 1;
   
   LSUM += (2 *  PNB);

#ifdef LOCALH
   LSUM += (2* LNB);
#endif  
#ifndef REALISTIC
    LSUM += 2 * (SNB + TNB);
#endif
#ifdef IMLI
    LSUM += 8;
#endif    

    if (!pred_inter)
         LSUM = -LSUM;
////////////////////////////////////
//integrate BIAS prediction   
    int8_t ctr = Bias[INDBIAS];
    LSUM += (2 * ctr + 1);
    ctr = BiasSK[INDBIASSK];
    LSUM += (2 * ctr + 1);

//integrate the GEHL predictions
#ifdef IMLI
#ifdef IMLIOH
    localoh = 0;
     localoh = PIPE[(PC ^ (PC >> 4)) & (PASTSIZE - 1)] + (localoh << 1);
    for (int i = 0; i >= 0; i--)
      {
	localoh =
	  ohhisttable[(((PC ^ (PC >> 4)) << SHIFTFUTURE) + IMLIcount +
		     i) & (OHHISTTABLESIZE - 1)] + (localoh << 1);
      }


    if (IMLIcount >= 2)
      LSUM += 2 * Gpredict ((PC << 2), localoh, Fm, FGEHL, FNB, LOGFNB);
#endif


#ifdef IMLISIC
    LSUM += 2 * Gpredict (PC, IMLIcount, Im, IGEHL, INB, LOGINB);
#else
    long long interIMLIcount= IMLIcount;
/* just a trick to disable IMLIcount*/
    IMLIcount=0;
#endif
#endif

#ifndef IMLI
    IMLIcount = 0;
#endif

    LSUM += Gpredict((PC << 1) + pred_inter /*PC*/, (GHIST << 11) + IMLIcount, Gm, GGEHL, GNB, LOGGNB);

#ifdef LOCALH
    LSUM += Gpredict(PC, L_shist[INDLOCAL], Lm, LGEHL, LNB, LOGLNB);

#ifndef REALISTIC
    LSUM += Gpredict(PC, (T_slhist[INDTLOCAL] << 11) + IMLIcount, Tm, TGEHL, TNB, LOGTNB);
    LSUM += Gpredict(PC, S_slhist[INDSLOCAL], Sm, SGEHL, SNB, LOGSNB);

#endif
#endif
#ifdef IMLI    
#ifndef IMLISIC
IMLIcount= interIMLIcount;
#endif
#endif

#ifdef REALISTIC
    LSUM += Gpredict (PC, GHIST, Pm, PGEHL, PNB, LOGPNB);
#else
    LSUM += Gpredict (PC, HSTACK[pthstack], Pm, PGEHL, PNB, LOGPNB);
#endif

    bool SCPRED = (LSUM >= 0);

// chose between the SC output and the TAGE + loop  output

    if (pred_inter != SCPRED) {

      //Choser uses TAGE confidence and |LSUM|
      pred_taken = SCPRED;
      if (HighConf) {
        if ((abs (LSUM) < Pupdatethreshold[INDUPD] / 3))
          pred_taken = (FirstH < 0) ? SCPRED : pred_inter;

        else if ((abs (LSUM) < 2 * Pupdatethreshold[INDUPD] / 3))
          pred_taken = (SecondH < 0) ? SCPRED : pred_inter;
        else if ((abs (LSUM) < Pupdatethreshold[INDUPD]))
          pred_taken = (ThirdH < 0) ? SCPRED : pred_inter;

      }
    }

    return pred_taken;
  }

  void HistoryUpdate (AddrType PC, InstOpcode brtype, bool taken,
		      AddrType target, long long &X, int &Y,
		      folded_history * H, folded_history * G,
		      folded_history * J, long long &LH,
		      long long &SH, long long &TH, long long &PH,
		      long long &GBRHIST)
  {
//special treatment for unconditional branchs;
    int maxt;
    if (brtype == iBALU_LBRANCH)
      maxt = 1;
    else
      maxt = 4;

    // the return stack associated history

#ifdef IMLI
    if (brtype == iBALU_LBRANCH)
      if (target < PC)
	{

//This branch is a branch "loop" 
	  if (!taken)
	    {
//exit of the "loop"
	      IMLIcount = 0;


	    }
	  if (taken)
	    {
	      if (IMLIcount < (MAXIMLIcount))
		IMLIcount++;
	    }
	}
#ifdef IMLIOH
    if (IMLIcount >= 1)
      if (brtype == iBALU_LBRANCH)
	{
	  if (target >= PC)
	    {
	      PIPE[(PC ^ (PC >> 4)) & (PASTSIZE - 1)] =
		ohhisttable[(((PC ^ (PC >> 4)) << SHIFTFUTURE) +
			   IMLIcount) & (OHHISTTABLESIZE - 1)];
	      ohhisttable[(((PC ^ (PC >> 4)) << SHIFTFUTURE) +
			 IMLIcount) & (OHHISTTABLESIZE - 1)] = taken;
	    }

	}
#endif    
#endif

    if (brtype == iBALU_LBRANCH) {
      GBRHIST = (GBRHIST << 1) + taken;
      LH = (LH << 1) + (taken);
#ifndef REALISTIC
      SH = (SH << 1) + (taken);
      SH ^= (PC & 15);
      TH = (TH << 1) + (taken);
#endif
    }

#ifndef REALISTIC
    PH = (PH << 1) ^ (target ^ (target >> 5) ^ taken);

    if (brtype == iBALU_RET) {
      pthstack = (pthstack - 1) & 15;
    }

    if (brtype == iBALU_LCALL) {
      int index = (pthstack + 1) & 15;
      HSTACK[index] = HSTACK[pthstack];
      pthstack = index;
    }
#endif


#ifdef USE_DOLC
    //for (int i = 1; i <= NHIST; i++) {
    for (int i = 1; i <= NHIST; i++) {
      uint64_t sign1 = 0; // dolc.getSignInt(pcSign(PC), logg[i], m[i]);
      uint64_t sign2 = 0; // dolc.getSign(TB[i]  , m[i]);
      H[i].set(sign1);
      G[i].set(sign2); // Not used in DOLC
      J[i].set(sign2); // Not used in DOLC
    }
#else
    int T = ((PC) << 1) + taken;
    int PATH = PC;
#endif

    for (int t = 0; t < maxt; t++) {
#ifdef USE_DOLC
      ghist[Y & (HISTBUFFERLENGTH - 1)] = 0;
      X = 0;
      Y--;
#else
      bool DIR = (T & 1);
      T >>= 1;
      int PATHBIT = (PATH & 127);
      PATH >>= 1;
      //update  history
      Y--;
      ghist[Y & (HISTBUFFERLENGTH - 1)] = DIR;
      X = (X << 1) ^ PATHBIT;
      for (int i = 1; i <= NHIST; i++) {
        H[i].update (ghist, Y);
        G[i].update (ghist, Y);
        J[i].update (ghist, Y);
      }
#endif
    }


//END UPDATE  HISTORIES
  }

// PREDICTOR UPDATE

  void updatePredictor (AddrType PC, bool resolveDir, bool predDir, AddrType branchTarget) {

#if 0
    if (HitBank) {
      if (resolveDir != predDir)
        printf("mpred h=%d i=%d h_1i=%d ctr=%d u=%d mbaad %x r=%d p=%d %d\n",HitBank,GI[HitBank],GI[HitBank-1],gtable[HitBank][GI[HitBank]].ctr,gtable[HitBank][GI[HitBank]].u, PC,resolveDir,predDir, branchTarget-PC);
      else
        printf("mpred h=%d i=%d h_1i=%d ctr=%d u=%d mgood %x r=%d p=%d %d\n",HitBank,GI[HitBank],GI[HitBank-1],gtable[HitBank][GI[HitBank]].ctr,gtable[HitBank][GI[HitBank]].u, PC,resolveDir,predDir, branchTarget-PC);
    }
#endif

    bool update_tage = last_pred->add(HitBank,resolveDir,predDir,GI[HitBank],HighConf);

#ifdef LOOPPREDICTOR
    if (LVALID) {
      if (pred_taken != predloop)
        ctrupdate(WITHLOOP, (predloop == resolveDir), 7);
    }

    loopupdate(PC, resolveDir, (pred_taken != resolveDir));
#endif

#ifdef SC
    bool SCPRED = (LSUM >= 0);
    if (HighConf) {
      if (pred_inter != SCPRED) {
        if ((abs (LSUM) < Pupdatethreshold[INDUPD]))
          if ((abs (LSUM) < Pupdatethreshold[INDUPD] / 3))
            ctrupdate (FirstH, (pred_inter == resolveDir), CONFWIDTH);
          else if ((abs (LSUM) < 2 * Pupdatethreshold[INDUPD] / 3))
            ctrupdate (SecondH, (pred_inter == resolveDir), CONFWIDTH);
          else if ((abs (LSUM) < Pupdatethreshold[INDUPD]))
            ctrupdate (ThirdH, (pred_inter == resolveDir), CONFWIDTH);
      }
    }

    if ((SCPRED != resolveDir) || ((abs (LSUM) < Pupdatethreshold[INDUPD]))) {

      if (SCPRED != resolveDir)
        Pupdatethreshold[INDUPD] += 1;
      else
        Pupdatethreshold[INDUPD] -= 1;

      if (Pupdatethreshold[INDUPD] >= 256)
        Pupdatethreshold[INDUPD] = 255;

      if (Pupdatethreshold[INDUPD] < 0)
        Pupdatethreshold[INDUPD] = 0;

      ctrupdate(Bias[INDBIAS], resolveDir, PERCWIDTH);
      ctrupdate(BiasSK[INDBIASSK], resolveDir, PERCWIDTH);
#ifdef IMLI
#ifndef IMLISIC
      long long interIMLIcount= IMLIcount;
      /* just a trick to disable IMLIcount*/
      IMLIcount=0;
#endif
#endif
      Gupdate((PC << 1) + pred_inter /*PC*/, resolveDir, (GHIST << 11) + IMLIcount, Gm, GGEHL, GNB, LOGGNB);
      Gupdate(PC, resolveDir, L_shist[INDLOCAL], Lm, LGEHL, LNB, LOGLNB);

#ifdef REALISTIC
      Gupdate(PC, resolveDir, GHIST, Pm, PGEHL, PNB, LOGPNB);
#else
      Gupdate(PC, resolveDir, S_slhist[INDSLOCAL], Sm, SGEHL, SNB, LOGSNB);
      Gupdate(PC, resolveDir, (T_slhist[INDTLOCAL] << 11) + IMLIcount, Tm, TGEHL, TNB, LOGTNB);
      Gupdate(PC, resolveDir, HSTACK[pthstack], Pm, PGEHL, PNB, LOGPNB);
#endif

#ifdef IMLI
#ifdef IMLISIC
      Gupdate(PC, resolveDir, IMLIcount, Im, IGEHL, INB, LOGINB);
#else
      IMLIcount=interIMLIcount;
#endif

#ifdef IMLIOH
      if (IMLIcount >= 2)
        Gupdate ((PC << 2), resolveDir, localoh, Fm, FGEHL, FNB, LOGFNB);
#endif

#endif
    }
//ends update of the SC states
#endif

//TAGE UPDATE
    if (true) {

      bool ALLOC = ((tage_pred != resolveDir) & (HitBank < NHIST));
      if (pred_taken == resolveDir)
        if ((MYRANDOM () & 31) != 0)
          ALLOC = false;

//      if (!update_tage)
//        ALLOC = false;
      //do not allocate too often if the overall prediction is correct 

      if (HitBank > 0) {
        // Manage the selection between longest matching and alternate matching
        // for "pseudo"-newly allocated longest matching entry
        bool PseudoNewAlloc = gtable[HitBank][GI[HitBank]].ctr_weak();
        // an entry is considered as newly allocated if its prediction counter is weak
        if (PseudoNewAlloc) {
          if (LongestMatchPred == resolveDir)
            ALLOC = false;

          // if it was delivering the correct prediction, no need to allocate a new entry
          //even if the overall prediction was false
#ifndef POSTPREDICT
          if (LongestMatchPred != alttaken) {
            int index = (INDUSEALT) ^ LongestMatchPred;
            ctrupdate(use_alt_on_na[index][HitBank > (NHIST / 3)], (alttaken == resolveDir), 4);
          }
#endif
        }
      }

      if (ALLOC) {

        int T = 1; // NHIST; // Seznec has 1

        int A = 1;
        if ((MYRANDOM () & 127) < 32 && NHIST>8)
          A = 2;

        int Penalty = 0;
        int NA = 0;

        int weakBank = HitBank + A;
#if 1
        // First try tag (but not offset hit)
        for (int i = weakBank; i <= NHIST; i += 1) {

          if (gtable[i][GI[i]].isTagHit()) {
            weakBank = i;

            if (!gtable[i][GI[i]].ctr_steal(resolveDir) )
              continue;

           T--;
            if (T <= 0)
               break;
          }
        }
        //T = 1;
#endif

        // Then allocate a new tag if still not good enough
        if (T>0) {
          weakBank = HitBank + A;
          for (int i = weakBank; i <= NHIST; i += 1) {

            if (gtable[i][GI[i]].u_get() == 0) {
              weakBank = i;
              //ctable[i][GI[i]].clear();

              gtable[i][GI[i]].reset(i, GTAG[i], resolveDir);
              NA++;

              if (T <= 0)
                break;

              i += 1;
              T -= 1;
            } else {
              Penalty++;
#if 0
              static int total=0;
              total++;
              if ((total&0xFF)==0)
                printf("penalty %d total %d h=%d\n",Penalty,total,i);
#endif
            }
          }
        }

        TICK += (Penalty - NA);
        //just the best formula for the Championship
        if (TICK < 0)
          TICK = 0;
        if (TICK > 1023) {
          for (int i = 1; i <= NHIST; i++)
            for (int j = 0; j <= (1 << logg[i]) - 1; j++) {
              gtable[i][j].u_dec();
            }
          TICK = 0;
        }
      }
      if (!update_tage && HitBank) {
        if (gtable[HitBank][GI[HitBank]].isHit()) {
          gtable[HitBank][GI[HitBank]].u_dec();
        }
      }

      if (HitBank>0) {
        gtable[HitBank][GI[HitBank]].ctr_update(HitBank,resolveDir, CWIDTH);
        if (gtable[HitBank][GI[HitBank]].u_get() == 0 && AltBank>0) {
          gtable[AltBank][GI[AltBank]].ctr_update(0,resolveDir, CWIDTH);
        }else{
          bimodal.update(resolveDir);
        }
        if (LongestMatchPred != alttaken) { // HitBank and AltBank dissagree
          if (LongestMatchPred == resolveDir) {
            gtable[HitBank][GI[HitBank]].u_inc();
          }else{
            gtable[HitBank][GI[HitBank]].u_dec();
          }
        }
      }else{
        bimodal.update(resolveDir);
      }
    }
#ifdef POSTPREDICT
    assert(ppi<postpsize);
    ctrupdate(postp[ppi],resolveDir,POSTPBITS);
#endif
    //END TAGE UPDATE

    HistoryUpdate (PC, iBALU_LBRANCH, resolveDir, branchTarget, phist,
        ptghist, ch_i, ch_t[0],
        ch_t[1], L_shist[INDLOCAL],
        S_slhist[INDSLOCAL], T_slhist[INDTLOCAL], HSTACK[pthstack],
        GHIST);
    //END PREDICTOR UPDATE
  }

#define GINDEX (((long long) PC) ^ bhist ^ (bhist >> (8 - i)) ^ (bhist >> (16 - 2 * i)) ^ (bhist >> (24 - 3 * i)) ^ (bhist >> (32 - 3 * i)) ^ (bhist >> (40 - 4 * i))) & ((1 << (logs - (i >= (NBR - 2)))) - 1)

  int Gpredict(AddrType PC, long long BHIST, int *length, int8_t ** tab, int NBR, int logs) {

    int PERCSUM = 0;
    for (int i = 0; i < NBR; i++) {
      long long bhist = BHIST & ((long long) ((1 << length[i]) - 1));
      int16_t ctr = tab[i][GINDEX];
      PERCSUM += (2 * ctr + 1);
    }

    return PERCSUM;
  }

  void Gupdate (AddrType PC, bool taken, long long BHIST, int *length, int8_t ** tab, int NBR, int logs) {

    for (int i = 0; i < NBR; i++) {
      long long bhist = BHIST & ((long long) ((1 << length[i]) - 1));
      ctrupdate (tab[i][GINDEX], taken, PERCWIDTH- (i < (NBR-1)));
    }

  }

  void TrackOtherInst(AddrType PC, InstOpcode opType, AddrType branchTarget) {

    fetchBoundaryOffsetOthers(PC);

    bool taken = true;

    HistoryUpdate(PC, opType, taken, branchTarget, phist,
        ptghist, ch_i,
        ch_t[0], ch_t[1],
        L_shist[INDLOCAL],
        S_slhist[INDSLOCAL], T_slhist[INDTLOCAL],
        HSTACK[pthstack], GHIST);
  }

};

#endif
