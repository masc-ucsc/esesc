#ifndef CFGSEARCH_H
#define CFGSEARCH_H

/* License & includes {{{1 */
/* 
ESESC: Enhanced Super ESCalar simulator
Copyright (C) 2009 University of California, Santa Cruz.

Contributed by 
Ehsan K.Ardestani
Jose Renau

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

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

