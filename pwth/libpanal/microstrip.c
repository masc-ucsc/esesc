
#include <math.h>
#include <stdio.h>  
#include "microstrip.h"

//double estimate_resistance(metal_t *spec)
//{
//	double Rou = (spec->metal_type == Al) ? Rou_Al : Rou_Cu;
//	return (Rou * spec->length * 1E-2) / (spec->width * spec->thickness); 
//}


double estimate_microstrip_capacitance(microstrip_t * spec) {
	double Cmicro;
	double tmp;


	Cmicro = 
			1E-12 * (0.67 * (spec->dielectric + 1.41) / log((5.98 * spec->height) / 
			(0.8 * spec->width + spec->thickness))) * spec->length;
/* units in mInches & Inches */ 
	tmp = log((5.98 * spec->height) / 
			(0.8 * spec->width + spec->thickness));
			
	return Cmicro;	  
}



