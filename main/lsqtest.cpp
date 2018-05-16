/*
   ESESC: Super ESCalar simulator
   Copyright (C) 2009 University of California, Santa Cruz.

   Contributed by Jose Renau
                  Basilio Fraguela
                  Milos Prvulovic
                  Smruti Sarangi

This file is part of ESESC.

ESESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

ESESC is distributed in the  hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
ESESC; see the file COPYING. If not, write to the Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * This launches the ESESC simulator environment with an ideal memory
 */

#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#include "nanassert.h"

#include "DInst.h"
#include "Instruction.h"

//*********** BEGIN CREATE FAKE PROCESSOR CLASS
uint64_t nReplays = 0;
#define GPROCESSOR_H 1
class GProcessor {
public:
  void replay(DInst *dinst) {
    nReplays++;
  }
};
//*********** END CREATE FAKE PROCESSOR CLASS
#include "LSQ.h"
#include "SescConf.h"

long long instTotal = 0;

LSQ *lsq;

void doTest() {
#if 0
  Instruction *ld    = Instruction::create(iLALU_LD, 0, 0, 0, 0, false );
  Instruction *st    = Instruction::create(iSALU_ST, 0, 0, 0, 0, false );
  Instruction *stadd = Instruction::create(iSALU_ADDR, 0, 0, 0, 0, false );

  DInst *dld1 = DInst::create(ld, rinst1, 0);
#endif
}

int main(int argc, const char **argv) {
  SescConf = new SConfig(argc, argv);

  GProcessor gproc;
  // lsq = new LSQFull(&gproc,0);
  lsq = new LSQFull(0);
  I(0);

  timeval startTime;
  gettimeofday(&startTime, 0);

  doTest();

  timeval endTime;
  gettimeofday(&endTime, 0);
  double msecs = (endTime.tv_sec - startTime.tv_sec) * 1000 + (endTime.tv_usec - startTime.tv_usec) / 1000;

  long double res = instTotal / 1000;
  res /= msecs;
  MSG("------------------");
  MSG("LSQ MIPS = %g secs = %g insts = %lld", (double)res, msecs / 1000, (long long)instTotal);
  MSG("------------------");

  return 0;
}
