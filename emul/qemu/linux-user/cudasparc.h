#if defined(CONFIG_ESESC_CUDA)

#include <stdint.h>
#include "mallocle.h"

//#include "/usr/local/cuda/include/cuda_runtime.h"
//#include "/usr/local/cuda/include/cuda_runtime_api.h"
//#include "/usr/local/cuda/include/driver_types.h" //TODO add this path to the project

//#include "/usr/local/cuda/include/cuda_runtime.h"
//#include "/usr/local/cuda/include/crt/host_runtime.h"



//#include "../esesc/cuda_runtime.h"
//#include "/usr/local/cuda/include/vector_types.h" //TODO add this path to the project

#include "../esesc/esesc_cuda_runtime.h"

#define __CUDAREGISTERFUNCTION    1
#define CUDAMALLOC                2
#define CUDAMEMCPY                3
#define CUDACONFIGURECALL         4
#define CUDASETUPARGUMENTS        5
#define CUDALAUNCH                6
#define CUDAFREE                  7
#define MALLOCLE                  8
#define CUDAUNREGISTERFATBINARY   9
#define PRINTQEMUDIRECTDATA       10

typedef union {
  int i;
  float f;
} myintfloat;


uint32_t callRealCudaFunctions(struct Arguments* p1, uint32_t func_num);


uint32_t callRealCudaFunctions(struct Arguments* p1, uint32_t func_num){
  //TODO: return the value back to the cuda application.

  int value = 0;
  size_t size  = 4;
  float* devPtr;
  value = cudaMalloc((void**)&devPtr, size);
//  int retval = 0;
  switch (func_num){

        case 1:
            printf("calling function cudaRegisterFunction\n");
            printf("Last argument is %d\n\n",*(int*)(uint64_t)(p1->args[9]));
            __cudaRegisterFunction(
                                   *(void***)((uint64_t)(p1->args[0])),
                                   *(const char**)((uint64_t)(p1->args[1])),
                                   *(char**)((uint64_t)(p1->args[2])),
                                   *(const char**)((uint64_t)(p1->args[3])),
                                   *(int*)((uint64_t)(p1->args[4])),
                                   *(uint3**)((uint64_t)(p1->args[5])),
                                   *(uint3**)((uint64_t)(p1->args[6])),
                                   *(dim3**)((uint64_t)(p1->args[7])),
                                   *(dim3**)((uint64_t)(p1->args[8])),
                                    *(int**) ((uint64_t)(p1->args[9]))
                                  );
            printf("Last argument is %d\n\n",*(int*)(uint64_t)(p1->args[9]));
            break;
        case 2:
            printf("calling function cudaMalloc\n");
            cudaMalloc(
                        *(void***)((uint64_t)tswap32(p1->args[0])),
                        *(size_t*)((uint64_t)tswap32(p1->args[1]))
                        );
            break;

        case 3:
            printf("\nCalling cudaMemcpy\n");
            cudaMemcpy(
                        *(void**)((uint64_t)tswap32(p1->args[0])),
                        *(void**)((uint64_t)tswap32(p1->args[1])),
                        *(size_t*)((uint64_t)tswap32(p1->args[2])),
                        *(int*)((uint64_t)tswap32(p1->args[3]))
                        );


            break;

        case 4:
            printf("\nCalling cudaConfigureCall\n");

            cudaConfigureCall(*(dim3*)((uint64_t)tswap32(p1->args[0])),
                              *(dim3*)((uint64_t)tswap32(p1->args[0])),
                              *(size_t*)((uint64_t)tswap32(p1->args[0])),
                              *(cudaStream_t*)((uint64_t)tswap32(p1->args[0]))
                              );
            break;

        case 5:
            printf("\nCalling cudaSetupArgument\n");
            cudaSetupArgument(*(const void **)((uint64_t)tswap32(p1->args[0])),
                              *(size_t*)((uint64_t)tswap32(p1->args[1])),
                              *(size_t*)((uint64_t)tswap32(p1->args[2]))
                              );
            break;

        case 6:
            printf("\nCalling cudaLaunch\n");
            cudaLaunch( *(const char**)((uint64_t)tswap32(p1->args[1])) ); //symbol
            break;

        case 7:
            printf("\nCalling cudaFree\n");
            cudaFree(*(void**)((uint64_t)tswap32(p1->args[1])));//devptr
            break;

        case MALLOCLE:
            printf("\nCalling mallocle\n");
            int size = tswap32(*(size_t*)((uint64_t)tswap32(p1->args[0])));

            printf("size is %d\n",size);
            //printf("cudasparc.h : address of ptr is %u\n",tswap32(p1->args[1]));
            abi_ulong temp = (mallocle(size));
            //printf("cudasparc.h : value @address of ptr is %u\n",temp);
            *((uint32_t*)(uint64_t)tswap32(p1->args[1])) = tswap32(temp);
            break;

        case PRINTQEMUDIRECTDATA:
            printf("\nCalling printqemudirectdata\n");
            int type = tswap32(*(int*)((uint64_t)tswap32(p1->args[1])));
            abi_ulong addr = (abi_ulong)tswap32(*(uint32_t*)((uint64_t)tswap32(p1->args[0])));
            printf("Address received is %u\n\n",addr);
            char* y = (char* )(uint64_t)addr;
            myintfloat a11;
            switch(type){
              case 1:
                printf("Direct QEMU DATA : %d\n",*((int*)(uint64_t)addr));
                printf("Direct QEMU DATA (swapped) : %d\n",tswap32(*((int*)(uint64_t)addr)));
                break;
              case 2:
                printf("Direct QEMU DATA : %f\n",*(float*)(uint64_t)addr);
                a11.i = tswap32(*(uint32_t*)(uint64_t)addr);
                printf("Direct QEMU DATA (swapped) : %f\n",a11.f);
                break;
              case 3:
                printf("Direct QEMU DATA : %s\n",y);
                break;
              default:
                printf("\nWhats type %d?\n",type);
            }
            printf("\n*********************************\n\n\n");
            break;

        default:
            printf("\nCuda function not defined.\n");
            //Set ret value (ret for syscall.c)
        }
    return 0;
}

#endif
