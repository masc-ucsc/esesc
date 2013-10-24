/*
* clock_panalyzer.h - basic clock power analyzer data types, strcutres and 
* thier manipulation functions. 
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
#ifdef __cplusplus
extern "C" {
#endif
#ifndef CLOCK_H
#define CLOCK_H
/* clock clocktree style - this should be done by configurator - future work! */
typedef enum {
    Htree /* H-tree */,
    balHtree /* balanced H-tree */
} clocktree_style_t;

/* estimate capacitance of clock distribution tree */
extern double estimate_interconnect_CT(
	clocktree_style_t style /* clock tree style */,
	double area /* area */,
	int Ntree /* number of tree level */,
	double interconnect_C /* interconnect capacitance per unit length in F */);

/* estimate clock distribution tree branch wire length */
extern double 
estimate_lbr(
	int N, /* Nth branch */
	double ledge /* edge length */);

/* estimate clock distribution tree and buffer capacitance */
//double estimate_switching_CT(
extern double estimate_switching_CT(
	clocktree_style_t clockStyle /* clock tree style */,
	double cskew /* target clock skew */,
	double area /* chip area */,
	double switching_CN /* clocked node switching capacitance: this should be the sume of all the clock node input capacitance */,
	int nbstages_opt /* optimal clock buffer number of stages : 4-6 is known to optimal */);
#endif /* CLOCK_H */
#ifdef __cplusplus
}
#endif

