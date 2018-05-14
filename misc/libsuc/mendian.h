#ifndef MENDIAN_H_
#define MENDIAN_H_

#ifdef __LITTLE_ENDIAN__
// 1st method
#define MENDIAN_LE 1
#else
// 2nd method
#include <endian.h>
#ifdef __BYTE_ORDER
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define MENDIAN_LE 1
#endif
#endif
// END method selection
#endif

#ifdef MENDIAN_LE
#define SWAP_LONG(X) \
  ((((unsigned long long)SWAP_WORD((uint32_t)((X) >> 32))) | ((unsigned long long)SWAP_WORD((uint32_t)(X))) << 32))

#define SWAP_WORD(X)                                                                                                        \
  (((((uint32_t)(X)) >> 24) & 0x000000ff) | ((((uint32_t)(X)) >> 8) & 0x0000ff00) | ((((uint32_t)(X)) << 8) & 0x00ff0000) | \
   ((((uint32_t)(X)) << 24) & 0xff000000))

#define SWAP_SHORT(X) (((((uint16_t)X) & 0xff00) >> 8) | ((((uint16_t)X) & 0x00ff) << 8))

#else
// BIG ENDIAN

#define SWAP_LONG(X) (X)
#define SWAP_WORD(X) (X)
#define SWAP_SHORT(X) (X)

#endif

#endif /* MENDIAN_H_ */
