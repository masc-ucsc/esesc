
#include "scstate.h"

void Scstate::sync()
{
#ifdef DEBUG
  for(int i=0;i<LREG_SCLAST;i++) {
    mod[i] = false;
  }
#endif
}

void Scstate::dump(const char *str) const
/* dump core stats {{{1 */
{
#ifdef DEBUG
  printf("Th[%d] %s: modified state\n",fid,str);
  for(int i=0;i<LREG_SCLAST;i++) {
    if (!mod[i])
      continue;
    printf("\trf[%d]=0x%lx ccrf=0x%lx\n",i, rf[i], iccrf[i]);
  }
#endif
}
/* }}} */

void Scstate::validateRegister(int reg_num, char *exp_reg_val) const
{
  char emul_reg_val[32];

  sprintf(emul_reg_val, "0x%08x", rf[reg_num]);
  if (strcmp(exp_reg_val, emul_reg_val) == 0) {
    // printf("    PASS: register values match \n");
  } else {
    if (reg_num != 26) {
      printf("    FAIL: register values don't match \n");
      printf("      reg_num %d. exp_reg_val %s. emul_reg_val %s \n", reg_num, exp_reg_val, emul_reg_val);
      I(0);
    } else {
      // Validate condition codes
      printf("condition codes %d \n", getICC(LREG_ICC));
    }
    //exit(0);
  } 

}
