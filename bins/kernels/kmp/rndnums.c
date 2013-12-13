/* rnds.h - Random Numbers
 *
 * Public Domain Software
 * 1991 by Larry Smith
 */

#include "rndnums.h"
#include <stdlib.h>

struct rndmill {
  unsigned long regA, regB, regC;
};

MILL *init_mill(unsigned long seed1,unsigned long seed2,unsigned long seed3) {
  MILL *result;

  result = (MILL *) malloc(sizeof(MILL));
  result->regA = seed1;
  result->regB = seed2;
  result->regC = seed3;
  return result;
};

void    nuke_mill(MILL *src) {
  free((char *)src);
};

unsigned long rndnum(MILL *src) {
  unsigned long result, bit;
  int j;

  result = 0;
  for(j = 0; j < 32; j++) {
    src->regA = (((
              (src->regA >> 31) ^
              (src->regA >> 6) ^
              (src->regA >> 4) ^
              (src->regA >> 2) ^
              (src->regA << 1) ^
              src->regA) & 0x00000001) << 31) | (src->regA >> 1);
    src->regB = (((
              (src->regB >> 30) ^
              (src->regB >> 2)) & 0x00000001) << 30) | (src->regB >> 1);
    src->regC = (((
              (src->regC >> 28) ^
              (src->regC >> 1)) & 0x00000001) << 28) | (src->regC >> 1);
    bit = ((src->regA & src->regB) | (~src->regA & src->regC)) & 0x00000001;
    result = (result << 1) ^ bit;
  }
  return result;
};

MILL   *checkpoint_mill(MILL *src) {
  return init_mill(src->regA, src->regB, src->regC);
};

void    reset_mill(MILL *src, MILL *chkpt) {
  src->regA = chkpt->regA;
  src->regB = chkpt->regB;
  src->regC = chkpt->regC;
};

