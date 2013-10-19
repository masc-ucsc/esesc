#ifdef __cplusplus
extern "C" {
#endif
#ifndef SUPPORT_H
#define SUPPORT_H
#include "logic.h"
extern void 
fatal(
	char *str);

extern void 
probe_node(
	char *str, 
	node_t *Y);

extern void 
unpack_vector(
	int bwidth /* bit-width */,
	node_t *vector[], 
	scalar_t scalar);

extern scalar_t 
pack_vector(
	int bwidth /* bit-width */,
	node_t *vector[]);
#endif /* SUPPORT_H */
#ifdef __cplusplus
}
#endif
