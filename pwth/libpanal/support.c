#include <stdio.h>
#include <stdlib.h>
#include "logic.h"

void 
fatal(
	char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(1);
}

void 
probe_node(
	char *str, 
	node_t *Y)
{
	//fprintf(stderr, "%s\t:%x\t%e\t%e\n", str, (unsigned int)Y->lvalue, Y->energy, Y->capacitance);
	fprintf(stderr, "%s\t:%x\n", str, (unsigned int)Y->lvalue);
}

void 
unpack_vector(
	int bwidth /* bit-width */,
	node_t *vector[], 
	scalar_t scalar)
{
	int i;

	for(i = 0; i < bwidth; i++)
	{
		vector[i]->lvalue = scalar & 0x01;
		scalar = scalar >> 1;
	}
}

scalar_t 
pack_vector(
	int bwidth /* bit-width */,
	node_t *vector[])
{
	scalar_t scalar;
	int i;

	scalar = 0x0;
	for(i = 0; i < bwidth * 8; i++)
	{
		scalar = scalar << 1;
		scalar |= (vector[bwidth - 1 - i]->lvalue) & 0x1; 
	}
	return scalar;
}


