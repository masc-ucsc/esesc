#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOGIC_H
#define LOGIC_H
#include "technology.h"

/** basic data structures for power modeling **/

/* bit type */
typedef unsigned char bit_t;
typedef unsigned int  scalar_t;

/* logic style type */
typedef enum {
	Static /* static CMOS gate */, 
	Port /* port */, 
} lgstyle_t;

/* netlist node type */
typedef struct {
	bit_t lvalue; /* logic value */
	double capacitance;
	double energy; /* transition energy */
} node_t;

/* 1-input generic logic gate type */
typedef struct _lgate1_t lgate1_t;
struct _lgate1_t {
	node_t *Y; 	/* current output */
	node_t *A; 		/* connected input node ptrs */
	double energy; 	/* energy dissipation of the gate */
	double (*lgate_op)(lgate1_t *lgate, double voltage);
					/* logic and energy evaluation fn of the gate */
};

/* 2-input generic logic gate type */
typedef struct _lgate2_t lgate2_t;
struct _lgate2_t {
	node_t *Y; 	/* current output */
	node_t *A, *B;	/* connected input node ptrs */
	double energy;	/* energy dissipation of the gate */
	/* energy dissipation of the gate */
	double (*lgate_op)(lgate2_t *lgate, double voltage); 
					/* logic and energy evaluation fn of the gate */
};

/* 3-input generic logic gate type */
typedef struct _lgate3_t lgate3_t;
struct _lgate3_t {
	node_t *Y; 	/* current output */
	node_t *A, *B, *C;	/* connected input node ptrs */
	double energy;	/* energy dissipation of the gate */
	/* energy dissipation of the gate */
	double (*lgate_op)(lgate3_t *lgate, double voltage); 
					/* logic and energy evaluation fn of the gate */
};

/** basic methods for power modeling **/
/* initialize node */
extern void 
initialize_node(
	node_t *Y /* node to be initialized */);

/* create an 1 input logic gate */
extern lgate1_t *
create_lgate1(
	node_t *A, /* inode */
	cmos_t *a, /* transistor size */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate1_t *lgate1 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */);

/* free lgate_1 instance */
extern void
free_lgate1(
	lgate1_t *lgate /* gate to be freed */);


/* create a 2 input logic gate */
extern lgate2_t *
create_lgate2(
	node_t *A, node_t *B, /* inodes: for TG types, A is G and B is D nodes */
	cmos_t *a, cmos_t *b, /* transistor sizes */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate2_t *lgate2 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */);

/* free lgate_2 instance */
extern void
free_lgate2(
	lgate2_t *lgate /* gate to be freed */);

/* create a 3 input logic gate */
extern lgate3_t *
create_lgate3(
	node_t *A, node_t *B, node_t *C, /* inodes: for TG types, A is Ngate, B is Pgate, and C is D nodes */
	cmos_t *a, cmos_t *b, cmos_t *c, /* transistor sizes */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate3_t *lgate3 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */);

/* free lgate_3 instance */
extern void
free_lgate3(
	lgate3_t *lgate /* gate to be freed */);


/** baisc logic and energy evaluation function **/
/* input port */
/* to propage the applied input and evaluate the energy dissipation by the input gate capacitance of the first stage nodes */
extern double  
/* return energy dissipation */
IPORT_op(
	lgate1_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);

/*  inverter */
extern double /* return energy dissipation */
INV_op(
	lgate1_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);

/* 2-input NAND gate */
extern double /* return energy dissipation */
NAND2_op(
	lgate2_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);

/* 3-input NAND gate */
extern double /* return energy dissipation */
NAND3_op(
	lgate3_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);

/* 2-input NOR gate */
extern double /* return energy dissipation */
NOR2_op(
	lgate2_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);

/* 3-input NOR gate */
extern double /* return energy dissipation */
NOR3_op(
	lgate3_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */);
#endif /* LOGIC_H */
#ifdef __cplusplus
}
#endif
