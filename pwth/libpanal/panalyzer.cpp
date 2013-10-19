
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#include "SescConf.h"
#include "Report.h"

#include "clock.h"

double calcClockTreeCap() {

  const char *technology    = SescConf->getCharPtr("","technology",0);
  clocktree_style_t style;

  // FIXME: use char
  const char *styleARG = SescConf->getCharPtr(technology, "clockTreeStyle");

  if(strcasecmp(styleARG,"htree") == 0) {
    style = Htree;
  }else if(strcasecmp(styleARG,"balhtree") == 0) {
    style = balHtree;
  }else{
    fprintf(stderr, "clockTreeStyle is Htree or balHtree");
    exit(-1);
  }

  double cskew = SescConf->getInt(technology, "skewBudget"); 
  cskew *= 1e-12;

  double area = SescConf->getInt(technology, "areaOfChip");
  area *= 1e6;

  double switching_CN=SescConf->getInt(technology, "loadInClockNode"); 
  switching_CN *= 1e-12;

  /* optimal clock buffer number of stages : 4-6 is known to optimal */
  int32_t nbstages_opt = SescConf->getInt(technology, "optimalNumberOfBuffer");

  fprintf(stderr, "Clock Area : %9.2fmm^2\n", area/1e6);

  return estimate_switching_CT(style, cskew, area, switching_CN, nbstages_opt);
}

const char *getOutputFile(int argc, const char **argv) {

  for(int i=0;i<argc;i++) {
    if (argv[i][0] != '-' || argv[i][1] != 'o')
      continue;

    // -c found
    if (argv[i][2]==0) {
      if (argc == (i+1)) {
        MSG("ERROR: Invalid command line call. Use -o tmp.conf");
        exit(-2);
      }

      return argv[i+1];
    }
    return &argv[i][2];
  }

  return 0;
}

int main(int argc, const char **argv) {

  //-------------------------------
  // Look for output file
  const char *outfile=getOutputFile(argc, argv);

  if (outfile==0) {
    fprintf(stderr,"Usage:\n\t%s -c <tmp.conf> -o <power.conf>\n",argv[0]);
    exit(-1);
  }


  //-------------------------------
  SescConf = new SConfig(argc, argv);

  unlink(outfile);
  Report::openFile(outfile);

  const char *technology = SescConf->getCharPtr("","technology");
  fprintf(stderr,"technology = [%s]\n",technology);
  double Mhz  = SescConf->getDouble(technology,"frequency");
  double tech = SescConf->getInt(technology,"tech");

  
  double VddPow = 4.5/(pow((800/tech),(2.0/3.0)));
  if (VddPow < 0.7)
    VddPow=0.7;
  if (VddPow > 5.0)
    VddPow=5.0;

  fprintf(stderr, "Freq : %9.2fGHz\n", Mhz/1e9);
  fprintf(stderr, "Vdd  : %9.2f v\n" , VddPow);

  // -------------------------
  double clockCap = calcClockTreeCap(); // 180nm tech

  fprintf(stderr, "Clock tree cap   : %9.2f fF\n", clockCap*1e12);
  fprintf(stderr, "Clock tree power : %9.2f W\n",  clockCap*Mhz*VddPow*VddPow);
  fprintf(stderr, "Clock tree Energy: %9.2f pJ\n", clockCap*VddPow*VddPow*1e9);

  //  fprintf(stderr, "total IO switching capacitance:%3.4e\n", estimate_switching_io());
  //  fprintf(stderr, "total random logic switching capacitance:%3.4e\n", estimate_switching_random_logic());

  // dump the cactify configuration
  SescConf->dump(true);

  Report::close();
}

