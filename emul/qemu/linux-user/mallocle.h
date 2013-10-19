/* This file contains a set of routines to manage memory that is used by cuda device
 * and hence the data stored in the location will be stored in the little endian format.
 */
#if 0
#include "qemu.h"

#if defined(CONFIG_ESESC_CUDA)

#define PTRSIZE_HERE uint32_t

static abi_ulong mallocle_abs_start = 0;
static abi_ulong mallocle_next_start = 0;
static abi_ulong mallocle_max_size = 1024;

abi_ulong  mallocle(size_t size);


abi_ulong  mallocle(size_t size){
  if (mallocle_abs_start == 0){
    mallocle_abs_start = mmap_find_vma1(mallocle_abs_start,mallocle_max_size);
    printf("mallocle.h : mallocle_abs_start address is %u\n",mallocle_abs_start);
    mallocle_next_start = mallocle_abs_start;
  }
  abi_ulong retptr = mallocle_next_start;
  //printf("mallocle.h : mallocle-ed address is %u\n",mallocle_next_start);
  mallocle_next_start = mallocle_next_start + size;
  int i = 0;

  //4 byte alignment
  for (i = 0; i < size%4; i++){
    *(uint32_t*)(PTRSIZE_HERE)mallocle_next_start = 0x0;
    mallocle_next_start++;
  }

  mallocle_next_start = mallocle_next_start + size%4;
  return retptr;
}

#endif
#endif
