#include <math.h>
#include <stdio.h>
#include "technology.h"

/* estimate optimal transistor stage ratio for cascade inverter buffer */
/* b: buffer */
double
estimate_btr_opt(
	unsigned nbstages_opt,  /* optimial number of buffer stages */
	double switching_CL /* load switching load capacitance */)
{
	fprintf(stderr, "in btr_opt nbstage_opt:%d\n", nbstages_opt);
	return pow((switching_CL / (estimate_MOSFET_CG(NCH /* channel type NCH or PCH */, 3. * LCH/* channel width */) + estimate_MOSFET_CG(PCH /* channel type NCH or PCH */, 1.5 * 3. * LCH /* channel width */))), (1.0 / (double)nbstages_opt));
}

/* estimate inverter buffer chain (IBC) capacitance */
double
estimate_switching_CIBC(
	double btr_opt, /* optimial buffer stages ratio */
	double switching_CL /* load switching capacitance */)
{

	return switching_CL * (1. + 1. / (1. - 1. / btr_opt));
}


