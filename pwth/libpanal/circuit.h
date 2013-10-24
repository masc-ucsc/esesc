#include "technology.h"
#ifdef __cplusplus
extern "C" {
#endif
/* estimate optimal transistor stage ratio for cascade inverter buffer */
/* b: buffer */
extern double
estimate_btr_opt(
	unsigned nbstages_opt,  /* optimial number of buffer stages */
	double switching_CL /* load switching capacitance */);

/* estimate inverter buffer chain (IBC) capacitance */
extern double
estimate_switching_CIBC(
	double btr_opt, /* optimial buffer stages ratio */
	double switching_CL /* load switching capacitance */);

#ifdef __cplusplus
}
#endif
