#include <math.h>
#include "technology.h"

/** transistor capacitance component estimation **/

/* (poly) gate capacitances */
double /* in F */
estimate_MOSFET_CG(
	channel_t channel /* channel type NCH or PCH */, 
	double WCH /* channel width */)
{
	double Cpoly, Covlp;

	/* obtain appropriate Cpoly and Covlp for the given channel type */
	Cpoly = (channel == NCH) ? CPOLY_NDIFF : CPOLY_PDIFF;
	Covlp = (channel == NCH) ? COVLP_NDIFF : COVLP_PDIFF;

	return ((LCH - (2. * XL)) * Cpoly + 2.* Covlp ) * WCH;
}
   
/* source/drain capacitances */
double /* in F */
estimate_MOSFET_CSD(
	channel_t channel /* channel type NCH or PCH */, 
	double WCH /* channel width */, 
	double LSD /* source/drain length */)
{
	double CJ, CJSW;
	
	CJ = (channel == NCH) ? CJ_NDIFF : CJ_PDIFF; /* junction area capacitance */
	CJSW = (channel == NCH) ? CJSW_NDIFF : CJSW_PDIFF; /* junction side-wall capacitance */

	return (LSD * LCH * WCH) * CJ + (2. * LSD * LCH + WCH) * CJSW;
}

/** interconnect capacitance component estimation **/
	
/* interconnect capacitance to the underlying plane : Cground */
double /* in F */
estimate_interconnect_CG(
	double l /* length in um */, 
	double s /* space in um */,
	double w /* width in um */,
	double h /* height from the underlying plane in um*/,
	double t /* thickness in um */,
	double k /* dielectric constant */)
{
	return (8.8542E-18*k*(w/h+2.217*pow((s/(s+0.702*h)), 2.193)+1.171*pow((s/(s+1.510*h)), 0.7642)*pow((t/(t+4.532*h)), 0.1204)));
}

/* interconnect capacitance to the adjacent interconnect (or coupling capacitance : Cc */
double /* in F */
estimate_interconnect_CC(
	double l /* length in um */, 
	double s /* space in um */,
	double w /* width in um */,
	double h /* height from the underlying plane in um*/,
	double t /* thickness in um */,
	double k /* dielectric constant */)
{
	return (8.8542E-18*k*(1.412*t/s*exp(-(4*s)/(s+8.014*h) + 2.3704*pow(w/(w+0.3078*s), 0.25724)*pow(w/(w+0.3078*s), 0.7571)*exp(-(2*s)/(s+6*h)))));
}

/* interconnect resistance */
double /* in F */
estimate_interconnect_R(
	double l /* length in um */, 
	double w /* width in um */,
	double t /* thickness in um */,
	double rou /* ROU */)
{
	return rou / 100. * l / (w * t /* interconnect cross section area */);
}

__inline__
double
estimate_trCeff(
	double k /* transistor sizing ratio */)
{
	double trCeff;

	trCeff = Cox * k * Wtech * Ltech;

	return trCeff;
}

__inline__
/* estimate semi-global wire capacitance */
double 
estimate_sgwireCeff( 
	double length /* wire length */) 
{
	return (length * sgwireC);
}

__inline__
/* estimate semi-global wire resistance */
double 
estimate_gwireCeff( 
	double length /* wire length */) 
{
	return (length * gwireC);
}

__inline__
/* estimate global wire capacitance */
double 
estimate_sgwireReff( 
	double length /* wire length */) 
{
	return (length * sgwireR);
}

__inline__
/* estimate global wire resistance */
double 
estimate_gwireReff( 
	double length /* wire length */) 
{
	return (length * gwireR);
}

__inline__
double
estimate_gininvCeff(
	double k /* transistor sizing ratio */)
{
	double ginCeff;

	ginCeff = (beta + 1.) * estimate_trCeff(k);

	return ginCeff;
}


__inline__
double
estimate_ginnandCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin */)
{
	double ginCeff;

	ginCeff = (beta + fi) * estimate_trCeff(k);

	return ginCeff;
}

__inline__
double
estimate_ginnorCeff(
	double k /* transistor sizing ratio */)
{
	double ginCeff;

	ginCeff = 2. * estimate_trCeff(k);

	return ginCeff;
}

__inline__
double
estimate_sdCeff(
	int CH, /* channel type */
	double k /* transistor sizing ratio */)
{
	double Cja0, Cjp0, sdCeff, Vbi;

	Cja0 = (CH == NMOS) ? nCja0 : pCja0;
	Cjp0 = (CH == NMOS) ? nCjp0 : pCjp0;
	Vbi = (CH == NMOS) ? nVbi : pVbi;

	sdCeff 
		= Cja0 * (k * Wtech * Ltech / 2.) * pow((1. + Vdd / (2. * Vbi)), -0.5) 
			+ Cjp0 * (k * Wtech + Ltech) * pow((1. + Vdd / (2. * Vbi)), -0.3); 

	return sdCeff;
}

__inline__
double
estimate_sdinvCeff(
	double k /* transistor sizing ratio */)
{
	double sdCeff;

	sdCeff = estimate_sdCeff(NMOS, k) + estimate_sdCeff(PMOS, beta * k);

	return sdCeff;
}

__inline__
double
estimate_sdnandCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin*/)
{
	double sdCeff;

	sdCeff = estimate_sdCeff(NMOS, fi * k) + fi * estimate_sdCeff(PMOS, beta * k);

	return sdCeff;
}

__inline__
double
estimate_sdnorCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin */)
{
	double sdCeff;

	sdCeff 	= fi * estimate_sdCeff(NMOS, beta * k) 
			+ estimate_sdCeff(PMOS, beta * k) + estimate_sdCeff(PMOS, beta);

	return sdCeff;
}

__inline__
/* in uS/um */
double
estimate_gm(
	int CH /* channel type*/)
{
	double mobility, Idss, gm;

	/* mobility */	
	mobility = (CH == NMOS) ? nmobility : pmobility;
	/* saturation current */	
	Idss = (CH == NMOS) ? nIdss : pIdss;

	/* transconductance / um */	
	gm = sqrt(2. * mobility * Cox / Ltech * Idss);
#ifdef DEBUG
	// fprintf(stderr, "gm: %e\n", gm);
#endif 
	return gm;
}

__inline__
double
estimate_trReff(
	int CH, /* channel type*/
	double k /* transistor sizing ratio */)
{
	double gm, trReff;
	
	gm = estimate_gm(CH);
	trReff = 1. / (gm * k * Ltech);
#ifdef DEBUG
	// fprintf(stderr, "trReff: %e\n", trReff);
#endif
	return trReff;
}

__inline__
double
/* 
estimate_cReff(int CH, double k)
*/
estimate_cReff(
	double k /* transistor sizing ratio */)
{
	double Rc, cReff;
/*
	Rc = (CH == NMOS) ? (nRc) : (pRc);
*/
	Rc = (nRc + pRc) / 2.;
	cReff = 2. * Rc / k;
#ifdef DEBUG
	// fprintf(stderr, "cReff: %e\n", cReff);
#endif
	return cReff;
}

__inline__
double
estimate_trinvReff(
	double k /* transistor sizing ratio */)
{
	double NMOStrReff, PMOStrReff, trReff;

	NMOStrReff = estimate_trReff(NMOS, k);
	PMOStrReff = estimate_trReff(PMOS, k);

	trReff = (NMOStrReff + PMOStrReff / 2.) / 2.;
#ifdef DEBUG
	// fprintf(stderr, "trinvReff: %e\n", trReff);
#endif
	return trReff;
}

__inline__
double
estimate_goutinvReff(
	double k /* transistor sizing ratio */)
{
	double trminReff, cminReff, goutReff;

	trminReff = estimate_trinvReff(1.);
	cminReff = estimate_cReff(1.);
	
	goutReff = (trminReff + 2. * cminReff) / k;
#ifdef DEBUG
	// fprintf(stderr, "goutinvReff: %e\n", goutReff);
#endif

	return goutReff;
}

