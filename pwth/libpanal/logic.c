#include <stdio.h>
#include <stdlib.h>
#include "logic.h"
#include "support.h"
#include "circuit.h"

/* initialize node */
void 
initialize_node(
	node_t *Y /* node to be initialized */)
{
	Y->lvalue = 0x0;
	Y->capacitance = 0.;
	Y->energy = 0.;
}

/* create an 1 input logic gate */
lgate1_t *
create_lgate1(
	node_t *A, /* inode */
	cmos_t *a, /* transistor size */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate1_t *lgate1 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */)
{
	lgate1_t *lgate;

	/* lgate data structure assignment */
	lgate = (lgate1_t *)malloc(sizeof(lgate1_t));
	if(!lgate)
		fatal("lack of virtual memory\n");

	/* Y node data structure assignment */
	lgate->Y = (node_t *)malloc(sizeof(node_t));
	if(!lgate->Y)
		fatal("lack of virtual memory\n");

	/* initialize Y node */
	initialize_node(lgate->Y);

	/* connect input nodes */
	lgate->A = A; 

	/* capacitance */
	switch(lgstyle)
	{
		case Static:
			/* estimate the input gate capacitances and assign them to the previous stage output nodes (the current stage input nodes) */
			A->capacitance += (estimate_MOSFET_CG(PCH, a->WPCH) + estimate_MOSFET_CG(NCH, a->WNCH)); 
		break;
		case Port:
			A->capacitance += 0.;
		break;
		default :
			fatal("check lgate type!");
		break;
	}
	
	/* evaluate logic value */
	lgate->lgate_op = lgate_op;

	return lgate;	
}

/* free lgate_1 instance */
void
free_lgate1(
	lgate1_t *lgate /* gate to be freed */)
{
	free(lgate);
}

/* create a 2 input logic gate */
lgate2_t *
create_lgate2(
	node_t *A, node_t *B, /* inodes: for TG types, A is G and B is D nodes */
	cmos_t *a, cmos_t *b, /* transistor sizes */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate2_t *lgate2 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */)
{
	lgate2_t *lgate;

	/* lgate data structure assignment */
	lgate = (lgate2_t *)malloc(sizeof(lgate2_t));
	if(!lgate)
		fatal("lack of virtual memory\n");

	/* Y node data structure assignment */
	lgate->Y = (node_t *)malloc(sizeof(node_t));
	if(!lgate->Y)
		fatal("lack of virtual memory\n");

	/* initialize Y node */
	initialize_node(lgate->Y);

	/* connect input nodes */
	lgate->A = A; lgate->B = B;

	/* capacitance */
	switch(lgstyle)
	{
		case Static:
			/* estimate the input gate capacitances and assign them to the previous stage output nodes (the current stage input nodes) */
			A->capacitance += (estimate_MOSFET_CG(PCH, a->WPCH) + estimate_MOSFET_CG(NCH, a->WNCH));
			B->capacitance += (estimate_MOSFET_CG(PCH, b->WPCH) + estimate_MOSFET_CG(NCH, b->WNCH));
		break;
		default :
			fatal("check lgate type!");
		break;
	}

	/* evaluate logic value */
	lgate->lgate_op = lgate_op;

	return lgate;	
}

/* free lgate_2 instance */
void
free_lgate2(
	lgate2_t *lgate /* gate to be freed */)
{
	free(lgate);
}

/* create a 3 input logic gate */
lgate3_t *
create_lgate3(
	node_t *A, node_t *B, node_t *C, /* inodes: for TG types, A is Ngate, B is Pgate, and C is D nodes */
	cmos_t *a, cmos_t *b, cmos_t *c, /* transistor sizes */
	lgstyle_t lgstyle, /* logic style */
	double (*lgate_op)(lgate3_t *lgate3 /* gate to be evaluated */, double voltage /* supply voltage */) /* logic and energy evaluation fn */)
{
	lgate3_t *lgate;

	/* lgate data structure assignment */
	lgate = (lgate3_t *)malloc(sizeof(lgate3_t));
	if(!lgate)
		fatal("lack of virtual memory\n");

	/* Y node data structure assignment */
	lgate->Y = (node_t *)malloc(sizeof(node_t));
	if(!lgate->Y)
		fatal("lack of virtual memory\n");

	/* initialize Y node */
	initialize_node(lgate->Y);

	/* connect input nodes */
	lgate->A = A; lgate->B = B; lgate->C = C;

	/* capacitance */
	switch(lgstyle)
	{
		case Static:
			/* estimate the input gate capacitances and assign them to the previous stage output nodes (the current stage input nodes) */
			A->capacitance += (estimate_MOSFET_CG(PCH, a->WPCH) + estimate_MOSFET_CG(NCH, a->WNCH));
			B->capacitance += (estimate_MOSFET_CG(PCH, b->WPCH) + estimate_MOSFET_CG(NCH, b->WNCH));
			C->capacitance += (estimate_MOSFET_CG(PCH, c->WPCH) + estimate_MOSFET_CG(NCH, c->WNCH));
		break;
		default :
			fatal("check lgate type!");
	}

	/* evaluate logic value */
	lgate->lgate_op = lgate_op;

	return lgate;	
}

/* free lgate_3 instance */
void
free_lgate3(
	lgate3_t *lgate /* gate to be freed */)
{
	free(lgate);
}


/* input port */
/* to propage the applied input and evaluate the energy dissipation by the input gate capacitance of the first stage nodes */
double  
/* return energy dissipation */
IPORT_op(
	lgate1_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y;
	node_t *A = lgate->A; 

	/* logic evaluation */
	lvalue = A->lvalue;
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue && lvalue) 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else /* no node transition */
		Y->energy = 0.; 

	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

/*  inverter */
double /* return energy dissipation */
INV_op(
	lgate1_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y;
	node_t *A = lgate->A; 

	/* logic evaluation */
	lvalue = !A->lvalue; 
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue) /* transition */ 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else 
		Y->energy = 0.; 

	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

/* 2-input NAND gate */
double /* return energy dissipation */
NAND2_op(
	lgate2_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y;
	node_t *A = lgate->A; node_t *B = lgate->B; 

	/* logic evaluation */
	lvalue = !(A->lvalue & B->lvalue);
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue ) /* transition */ 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else /* no node transition */
		Y->energy = 0.; 
	
	//probe_node("nand2x1", Y);
	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

/* 3-input NAND gate */
double /* return energy dissipation */
NAND3_op(
	lgate3_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y;
	node_t *A = lgate->A; node_t *B = lgate->B; node_t *C = lgate->C;

	/* logic evaluation */
	lvalue = !(A->lvalue & B->lvalue & C->lvalue);
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue) /* transition */ 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else /* no node transition */
		Y->energy = 0.; 
	
	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

/* 2-input NOR gate */
double /* return energy dissipation */
NOR2_op(
	lgate2_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y;
	node_t *A = lgate->A; node_t *B = lgate->B; 

	/* logic evaluation */
	lvalue = !(A->lvalue | B->lvalue);
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue) /* transition */ 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else /* no node transition */
		Y->energy = 0.; 
	
	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

/* 3-input NOR gate */
double /* return energy dissipation */
NOR3_op(
	lgate3_t *lgate /* gate to be evaluate */,
	double voltage /* supply voltage */)
{
	bit_t lvalue;
	node_t *Y = lgate->Y; node_t *A = lgate->A; node_t *B = lgate->B; node_t *C = lgate->C; 

	/* logic evaluation */
	lvalue = !(A->lvalue | B->lvalue | C->lvalue);
	
	/* DONT TOUCH: start */
	if(lvalue ^ Y->lvalue) /* transition */ 
		Y->energy = 0.5 * Y->capacitance * (voltage * voltage);
	else /* no node transition */
		Y->energy = 0.; 
	
	Y->lvalue = lvalue;

	return Y->energy;
	/* DONT TOUCH: end */
}

