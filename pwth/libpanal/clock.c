#include <stdio.h>
#include <math.h>
#include "technology.h"
#include "circuit.h"
#include "clock.h"
#include "support.h"

/* estimate capacitance of clock distribution tree */
double 
estimate_interconnect_CT(
	clocktree_style_t style /* clock tree style */,
	double area /* die area */,
	int Ntree /* number of tree level */,
	double interconnect_C /* interconnect capacitance per unit length in F */)
{
	double interconnect_CT;
	int i;

	interconnect_CT = 0.0;
	switch(style) 
	{
		case Htree:
		for(i = 1; i <= Ntree; i++)
		{
			interconnect_CT += interconnect_C * sqrt(area) * pow(2.,(double)(Ntree - 1)) * (1. / pow(2., floor((double)i / 2.) + 1.));
         }
		break;

		case balHtree: 
		for(i= 1; i <= Ntree; i++) 
		{ 
			interconnect_CT += interconnect_C * sqrt(area) * pow(2.,(double)(i - 1)) * (1. / pow(2., floor((double)i / 2.) + 1.));

		} 
		break;
		default:
      		fatal("not supported clock tree type!");
		break;
	}

	fprintf(stderr, "total clock tree interconnect capacitance: %e\n", interconnect_CT);

	return interconnect_CT;
}

/* estimate clock distribution tree branch wire length */
double 
estimate_lbr(
	int N, /* Nth branch */
	double ledge /* edge length */)
{
	return ledge * 1. / (pow(2., floor((double)(N) / 2.) + 1.));
}

/* estimate clock distribution tree and buffer capacitance */
double estimate_switching_CT(
	clocktree_style_t style /* clock tree style */,
	double cskew /* target clock skew */,
	double area /* die area */,
	double switching_CN /* clocked node switching capacitance: this should be the sume of all the clock node input capacitance */,
	int nbstages_opt /* optimal clock buffer number of stages : 4-6 is known to optimal */)
{
	double dcskew, btr_opt;
	double interconnect_C, interconnect_R, interconnect_CT;
	double switching_CT = 0.0 /* clock tree switching */;
	int Ntree;

	/* estimatin clock distribution interconnect capacitance */
	/* clock distribution tree interconnect unit capacitance */
	interconnect_C = estimate_interconnect_CG(1.0 /* length in um */, 10. /* space in um: maximum allowed interconnect spacing in the model assuming there is no adjacent interconnect */, GWIDTH /* width in um */, GHEIGHT /* height from the underlying plane in um*/, GTHICKNESS /* thickness in um */, K /* dielectric constant */);
	
	fprintf(stderr, "interconnect_C: %3.4e (fF/um)\n", interconnect_C * 1E15);

	/* clock distribution tree interconnect unit resistance */
	interconnect_R = estimate_interconnect_R(1.0 /* length in um */, GWIDTH /* width in um */, GTHICKNESS /* thickness in um */, ROU_AL /* ROU: Aluminum is assumed  */);
	fprintf(stderr, "interconnect_R: %3.4e (ohm/um)\n", interconnect_R);


	/* desired clock skew */
	fprintf(stderr, "cskew: %3.4e (ps)\n", cskew * 1E12);
	fprintf(stderr, "RC: %3.4e (ps)\n", interconnect_C * interconnect_R * 1E12);
	dcskew 	= sqrt(cskew / (interconnect_C * interconnect_R)); 
	fprintf(stderr, "clock tree block length: %3.4e (um)\n", dcskew * 1E6);

	/* depth of clock tree */
	Ntree 	= (int)(sqrt(area) / dcskew + 1.0); 
	fprintf(stderr, "depth of clock tree: %u\n", Ntree);

	/* clock tree interconnect capacitaqnce */
	interconnect_CT = estimate_interconnect_CT(style, area, Ntree, interconnect_C); 

	switch(style) 
	{
		case Htree:
		/* clock tree switching capacitance */
		switching_CT = switching_CN + interconnect_CT;
		fprintf(stderr, "before buffering switching_CT:%3.4e\n", switching_CT);

		/* estimate optimal transistor stage ratio for cascade interver chain */
		btr_opt = estimate_btr_opt(nbstages_opt, switching_CT);
		fprintf(stderr, "btr_opt:%3.4e\n", btr_opt);
		switching_CT = estimate_switching_CIBC(btr_opt, switching_CT);
		fprintf(stderr, "after buffering switching_CT:%3.4e\n", switching_CT);
		break;

		case balHtree:
      		fatal("to be implemented!");
		break;
		default:
      		fatal("not supported clock tree type!");
		break;
	}

	return switching_CT;
}

