#ifndef CFGSEARCH_H
#define CFGSEARCH_H

// Contributed by Ehsan K.Ardestani
//                Jose Renau
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

#ifdef ENABLE_CUDA

#include <vector>
#include "Bundle.h"
#include "GPUInterface.h"

class CfgSearch {
  private:
    uint32_t kId;
    float    minV, maxV;
    int      maxNumSM, minNumSM;
    int   currNumSM, lastNumSM;
    float currED, lastED, minED;
    float currV, lastV;
    int   start, end;
    int   next;
    int   searchCntSM;
    int   searchCntV;
    int   searchCnt;
    bool  first;

    template <class T>
      T   nextStart(T i, T j) { return (i+j)/2; };

    
    int   nextNumSM()  {
      // We start from the highest available SMs and search down
      int next;

      searchCntSM ++;

      next = currNumSM - static_cast<int>((maxNumSM-minNumSM)/(log2(maxNumSM)));

      if (bestED()) {
        //next = nextStart <int> (minNumSM, currNumSM);
        lastNumSM = currNumSM;
      } else {
        //next = nextStart <int> (currNumSM, lastNumSM+1);
      }
      currNumSM = next;
      gpuTM->setKernelsNumSM(kId, next);
      gpuTM->reOrganize();
    };
    float nextV() {
      // We start from the lowest V and search up
      float next;
      minED            = first ? 1e9 : minED;
      first = false;
      searchCntV ++;

      if (bestED()) 
        lastV  = currV;

      switch (searchCntV) {
        case 1:
        case 2:
          next = currV + 0.1;
          break;
        default:
          if (currNumSM < maxNumSM) {
            next = maxV;
          } else {
            learningDone();
            next = recoverV();
          }
      }

      currV  = next;
      return next;
    };
    int   rndNumSM()  {
      searchCntSM ++;
      lastNumSM = currNumSM;
      int rnd = rand() % maxNumSM + 1;
      if (rnd > currNumSM) {
        currNumSM = nextStart <int> (currNumSM, rnd);
      } else {
        currNumSM = nextStart <int> (rnd, currNumSM);
      }
    };
    float rndV() {
      searchCntV ++;
      lastV = currV;
      float rnd = rand() / (RAND_MAX + 1);
      if (rnd > currV) {
        currV = nextStart <float> (currV, rnd);
      } else {
        currV = nextStart <float> (rnd, currV);
      }
    };

    void setCurrED(float currED_) {
      lastED       = currED;
      currED       = currED_;
      if (currED < minED)
        minED = currED;
    }

    int   recoverNumSM() { 
      if (currNumSM != lastNumSM) {
        currNumSM = lastNumSM; 
        gpuTM->setKernelsNumSM(kId, currNumSM);
        gpuTM->reOrganize();
      }
    };
    float   recoverV()     { return currV = lastV; };
    void learningOnGoing() { reexecute = 1; }; // defined in GPUInterface
    void learningDone()    { reexecute = 0; }; // defined in GPUInterface
    bool bigSave()         { return lastED - currED > 0.1*lastED ? true : false; };
    bool bestED()          { return currED == minED ? true : false; };
    void getMinNumSM()     { minNumSM = gpuTM->getMinNumSM() < maxNumSM? gpuTM->getMinNumSM() : maxNumSM;  };

  public:
    CfgSearch (int nsm, float maxv, float minv, float currv, uint32_t kid) {

      kId       = kid;
      maxNumSM  = nsm;   
      currNumSM = nsm;   // numSM;
      lastNumSM = currNumSM;
      minNumSM = 1; // gpuTM->getMinNumSM() < maxNumSM? gpuTM->getMinNumSM() : maxNumSM;

      maxV     = maxv;  // maxV_;
      currV    = currv;
      lastV    = currV;
      minV     = minv;

      currED      = 1e9; // a big number
      minED       = currED;
      searchCnt   = 0;
      searchCntSM = 0;
      searchCntV  = 0;
      learningDone();

      first = true;
    };

    float nextCfg( float ed ) {

      setCurrED( ed );
      getMinNumSM();
      learningOnGoing();

      searchCnt++;


      if (searchCntSM < log2(maxNumSM) && minNumSM < maxNumSM) {
        nextNumSM();
        return currV;
      } else if (searchCntV < (log2(10*(maxV-0.5)))) {
        recoverNumSM();
        return (nextV());
      }
      learningDone();
      recoverNumSM();
      return recoverV();
    };


    int   getNumSM() { return currNumSM; };
    float getV()     { return currV; };

    void setMaxNumSM ( uint64_t sm ){ maxNumSM = sm; }

};

#endif
#endif

