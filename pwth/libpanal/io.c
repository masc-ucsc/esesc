/*
* io.c - I/O power analysis tools for address/data bus.
*
* This file is a part of the PowerAnalyzer tool suite written by
* Nam Sung Kim as a part of the PowerAnalyzer Project.
*  
* The tool suite is currently maintained by Nam Sung Kim.
* 
* Copyright (C) 2001 by Nam Sung Kim
*
* This source file is distributed "as is" in the hope that it will be
* useful.  The tool set comes with no warranty, and no author or
* distributor accepts any responsibility for the consequences of its
* use. 
* 
* Everyone is granted permission to copy, modify and redistribute
* this tool set under the following conditions:
* 
*    This source code is distributed for non-commercial use only. 
*    Please contact the maintainer for restrictions applying to 
*    commercial use.
*
*    Permission is granted to anyone to make or distribute copies
*    of this source code, either as received or modified, in any
*    medium, provided that all copyright notices, permission and
*    nonwarranty notices are preserved, and that the distributor
*    grants the recipient permission for further redistribution as
*    permitted by this document.
*
*    Permission is granted to distribute this file in compiled
*    or executable form under the same conditions that apply for
*    source code, provided that either:
*
*    A. it is accompanied by the corresponding machine-readable
*       source code,
*    B. it is accompanied by a written offer, with no time limit,
*       to give anyone a machine-readable copy of the corresponding
*       source code in return for reimbursement of the cost of
*       distribution.  This written offer must permit verbatim
*       duplication by anyone, or
*    C. it is distributed by someone who received only the
*       executable form, and is accompanied by a copy of the
*       written offer of source code that they received concurrently.
*
* In other words, you are welcome to use, share and improve this
* source file.  You are forbidden to forbid anyone else to use, share
* and improve what you give them.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "io.h"
#include "circuit.h"
#include "microstrip.h"

__inline__
/* estimate inverter buffer chain capacitance */
double 
estimate_ibufchainiCeff(
	double btr_opt,	/* optimial buffer stages ratio */ 
	double sCeff)
{
	double btrCeff;

	btrCeff = sCeff * ( 1. / (1. - 1. / btr_opt));

	return btrCeff;
}

double 
estimate_io_btr_opt(
    unsigned nbstages_opt,  /* optimial number of buffer stages */ 
	double sCeff)
{
	double btr_opt, ginmininvCeff;

	ginmininvCeff = estimate_gininvCeff(1.);
	btr_opt = pow((sCeff / ginmininvCeff), (1.0 / (double)nbstages_opt));
	return btr_opt;
}

double 
estimate_switching_io(
	double padCeff,
	double lCeff,
	unsigned nbstages_opt, /* optimial number of io buffer stages */
	double strip_length /* length of microstrip */
	)
{
	double btr_opt;
	double microCeff;
	double iCeff;
	microstrip_t micro_spec;
	micro_spec.dielectric = MICROSTRIP_ER;  
	micro_spec.width = MICROSTRIP_H;  
	micro_spec.thickness = MICROSTRIP_T;  
	micro_spec.height = MICROSTRIP_W;  
	micro_spec.length = strip_length;  
	microCeff = estimate_microstrip_capacitance(&micro_spec);
	fprintf(stderr, "strip_length =  %lf\n", strip_length);
	fprintf(stderr, "microCeff %3.4e\n", microCeff);
	btr_opt = estimate_btr_opt(nbstages_opt, padCeff + microCeff + lCeff);
	fprintf(stderr, "optimal buffer transistor ratio: %3.4e\n", btr_opt);
	iCeff = estimate_ibufchainiCeff(btr_opt, padCeff + microCeff + lCeff);
	iCeff += padCeff + microCeff + lCeff;
	fprintf(stderr, "iCeff: %3.4e\n", iCeff);
	return iCeff;
}

