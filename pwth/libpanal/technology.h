#ifdef __cplusplus
extern "C" {
#endif
#ifndef TECHNOLOGY_H
#define TECHNOLOGY_H

/* MOSFET transisotr channel type  */
typedef enum {PCH, NCH} channel_t;

/* cmos transisotr size */
typedef struct {
	double WPCH; 
	double WNCH;
} cmos_t;


#define LCH				(0.18)		/* physical channel length (um) */
#define XL				(0.02)		/* poly (gate) overlap with active area (um) */

/* poly over */
#define CPOLY_PDIFF		(8332E-18)	/* poly to p active capacitance between oxide (aF/um^2) */
#define CPOLY_NDIFF		(8548E-18)	/* poly to n active capacitance between oxide (aF/um^2) */

/* poly overlap over diff */
#define COVLP_PDIFF		(690E-18) 	/* p poly to diff (F/um) */
#define COVLP_NDIFF		(741E-18) 	/* n poly to diff (F/um) */

/* diff side wall */
#define CJ_PDIFF		(1171E-18)  /* p diff side wall area (F/um^2) */
#define CJ_NDIFF		(970E-18)  /* n diff side wall area (F/um^2) */
#define CJSW_PDIFF		(215E-18)  /* p diff side wall peri (F/um) */
#define CJSW_NDIFF		(261E-18)  /* n diff side wall peri (F/um) */

#define ROU_CU			(2.2)		/* ohm / cm */
#define ROU_AL			(3.3)		/* ohm / cm */

#define MLSD			(3.)		/* minimum source/drain length in multiple of LCH from the technology design rule */
#define ACTIVE_MARGINE	(0.18)		/* poly extension from active edge (um) */
#define WELL_MARGINE	(0.6)		/* poly extension to well edge (um) */
#define	CPOLY_MARGINE	((ACTIVE_MARGINE + WELL_MARGINE) * Leff * CPOLY_WELL_A)
#define CPOLY_WELL_A	(105E-18)	/* poly to well capacitance (aF/um^2) */

#define K				(3.5)
/* global interconnect dimension */
#define GWIDTH			(0.8)		/* width in um */
#define GHEIGHT			(0.65)		/* height in um */
#define GTHICKNESS		(1.25)		/* thickness in um */

/* intermediate interconnect dimension */
#define IWIDTH			(0.35)		/* width in um */
#define IHEIGHT			(0.65)		/* height in um */
#define ITHICKNESS		(0.65)		/* thickness in um */

/* local interconnect dimension */
#define LWIDTH			(0.28)		/* width in um */
#define LHEIGHT			(0.45)		/* height in um */
#define LTHICKNESS		(0.65)		/* thickness in um */

#define sgwireC 	(414.0E-18) /* F/um */
#define gwireC 		(440.0E-18) /* F/um */
#define sgwireR 	(96.0E-3)   /* Ohm/um */
#define gwireR 		(20.0E-3)   /* Ohm/um */
#define nVth		(0.3) 			/* minimum transistor NMOS Vth */
#define pVth		(0.34)			/* minimum transistor PMOS Vth */
#define Vth			(nVth / 2. + pVth / 2.) 
#define Ltech		(0.18)				/* min technology channel width in um */
#define Ld			(0.007631769) 		/* channel length overlap in um  */
#define Leff		(Ltech - 2. * Ld)	/* min effective channel legth */
#define Wtech       (0.27)				/* min technology channel width in um */
#define Wd			(0.001798714) 		/* channel width overlap in um */
#define Weff        (Wtech - 2. *Wd) 	/* min effective channel width in um */
#define eox			(3.9 * 8.85E-18)	/* permiability of Si in F/um */
#define tox			(4.1E-3)			/* SiO2 thinkness in um */
#define pmobility	(84.78)				/* p-type mobility */
#define nmobility	(409.16)			/* n-type mobility */
#define beta		(nmobility / pmobility)
#define Cox			(eox / tox)
#define nIdss		(546E-6) 	/* NMOS saturation current A/um */
#define pIdss		(256E-6) 	/* PMOS saturation current A/um */
#define nRc			(11.3)		/* n-active contact resistance */		
#define pRc			(11.8)		/* p-active contact resistance */		
#define Vdd			(1.8)			/* nominal supply voltage */
#define Vio			(3.3)			/* I/O circuitry supply voltage */
#define nVbi		(0.7420424)		/* built in potential in V */ 
#define pVbi		(0.8637545)		
#define nCja0		(0.9868856E-15) /* junction capacitance in F/um^2 */
#define pCja0		(1.181916E-15)
#define nCjp0		(0.2132647E-15) /* side wall capacitance in F/um */
#define pCjp0		(0.1716535E-15)
#define RKp			(0.82) /* Rent constant for microprocessor */
#define Rp			(0.45) /* Rent exponent for microprocessor */
#define packageCeff	(3.548E-12)	/* PAD plate from ARTISAN TSMC 0.18um  */
#define interspace		(3.0 * Ltech)
#define interwidth		(2.0 * Ltech)
#define interpitch		(interspace + interwidth)
#define NCH  1
#define PCH  0
#define NMOS  1
#define PMOS  0


/* CMOS 0.18um model parameters  - directly from Appendix II of tech report */
#define Cndiffarea    	0.137e-15 	/* F/um2 at 1.5V */
#define Cpdiffarea    	0.343e-15 	/* F/um2 at 1.5V */
#define Cndiffside    	0.275e-15 	/* F/um at 1.5V */
#define Cpdiffside    	0.275e-15 	/* F/um at 1.5V */
#define Cndiffovlp    	0.138e-15 	/* F/um at 1.5V */
#define Cpdiffovlp    	0.138e-15 	/* F/um at 1.5V */
#define Cnoxideovlp   	0.263e-15 	/* F/um assuming 25% Miller effect */
#define Cpoxideovlp   	0.338e-15 	/* F/um assuming 25% Miller effect */
#define Cgate         	1.95e-15	/* F/um2 */
#define Cgatepass     	1.45e-15	/* F/um2 */
#define Cpolywire	  	0.25e-15	/* F/um */
#define Cmetal 			275e-18		/* */

#define Rnchannelstatic	25800		/* ohms*um of channel width */
#define Rpchannelstatic	61200  		/* ohms*um of channel width */
#define Rnchannelon		 8751
#define Rpchannelon		20160
#define Rmetal			48e-3

#define C_IO 3e-12 /* capacitance for IO pad */

/* (poly) gate capacitances */
extern double /* in F */
estimate_MOSFET_CG(
	channel_t channel /* channel type NCH or PCH */, 
	double WCH /* channel width */);

/* source/drain capacitances */
extern double /* in F */
estimate_MOSFET_CSD(
	channel_t channel /* channel type NCH or PCH */, 
	double WCH /* channel width */, 
	double LSD /* source/drain length */);

extern double /* in F */
estimate_interconnect_CG(
	double l /* length in um */, 
	double s /* space in um */,
	double w /* width in um */,
	double h /* height from the underlying plane in um*/,
	double t /* thickness in um */,
	double k /* dielectric constant */);

/* interconnect capacitance to the adjacent interconnect (or coupling capacitance : Cc */
extern double /* in F */
estimate_interconnect_CC(
	double l /* length in um */, 
	double s /* space in um */,
	double w /* width in um */,
	double h /* height from the underlying plane in um*/,
	double t /* thickness in um */,
	double k /* dielectric constant */);

/* interconnect resistance */
extern double /* in OHM */
estimate_interconnect_R(
	double l /* length in um */, 
	double w /* width in um */,
	double t /* thickness in um */,
	double rou /* ROU */);

extern double
estimate_trCeff(
	double k /* transistor sizing ratio */);

extern double 
estimate_sgwireCeff(
	double length);

extern double 
estimate_gwireCeff(
	double length);

extern double 
estimate_sgwireReff(
	double length); 

extern double 
estimate_gwireReff(
	double length);

extern double
estimate_gininvCeff(
	double k /* transistor sizing ratio */);

extern double
estimate_ginnandCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin */);

extern double
estimate_ginnorCeff(
	double k /* transistor sizing ratio */);

extern double
estimate_sdCeff(
	int CH, /* channel type */
	double k /* transistor sizing ratio */);

extern double
estimate_sdinvCeff(
	double k /* transistor sizing ratio */);

extern double
estimate_sdnandCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin*/);

extern double
estimate_sdnorCeff(
	double k, /* transistor sizing ratio */
	double fi /* average fanin */);

extern double
estimate_gm(
	int CH /* channel type*/);

extern double
estimate_trReff(
	int CH, /* channel type*/
	double k /* transistor sizing ratio */);

extern double
estimate_cReff(
	double k /* transistor sizing ratio */);

extern double
estimate_trinvReff(
	double k /* transistor sizing ratio */);

extern double
estimate_goutinvReff(
	double k /* transistor sizing ratio */);


#endif
#ifdef __cplusplus
}
#endif

