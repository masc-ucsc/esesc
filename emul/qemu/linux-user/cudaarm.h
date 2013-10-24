#ifndef CUDAARM_H
#define CUDAARM_H


#include <stdint.h>
#include "mallocle.h"
#include "../esesc/esesc_cuda_runtime.h"

#if  defined (CONFIG_ESESC_system) || defined (CONFIG_ESESC_user)
#include "../esesc/esesc_qemu.h"
#endif


#define CUDAREGISTERFUNCTION      1
#define CUDAMALLOC                2
#define CUDAMEMCPY                3
#define CUDACONFIGURECALL         4
#define CUDASETUPARGUMENTS        5
#define CUDALAUNCH                6
#define CUDAFREE                  7
#define MALLOCLE                  8
#define CUDAREGISTERFATBINARY     9
#define CUDAUNREGISTERFATBINARY   10
#define PRINTDIRECTQEMUDATA       11
#define CUDAGETDEVICECOUNT        12
#define CUDAGETDEVICEPROPERTIES   13
#define CUDASETDEVICE             14
#define CUDAGETDEVICE             15
#define CUDAFUNCGETATTRIBUTES     16
#define CUDAMEMCPYTOSYMBOL        17
#define CUDAMEMSET                18
#define CUGAREGISTERVAR           19
#define CUDATHREADSYNCHRONIZE     20
#define CUDATHREADEXIT            21
#define CUDAGETLASTERROR          22
#define CUGAREGISTERTEX           23
#define CUDACREATECHANNELDESC     24
#define CUDABINDTEXTURE           25
#define CUDAUNBINDTEXTURE         26

#define CUDAEVENTCREATE           27
#define CUDAEVENTRECORD           28
#define CUDAEVENTQUERY            29
#define CUDAEVENTSYNCHRONIZE      30
#define CUDAEVENTDESTROY          31
#define CUDAEVENTELAPSEDTIME      32

#define CUDAPROGDONE              101
#define ALLOCINITHOSTDEVPTRS      111

/******* OSOLETE ********/
#define STOREHOSTPTR              104
#define STOREDEVPTR               105
#define STOREHOSTPTRSIZE          106 //Obsolete should remove
#define STOREDEVPTRSIZE           107 //Obsolete should remove
#define STORENUMTHREADS           108 //Obsolete should remove
#define STORENUMBYTES             109 //Obsolete should remove
#define STORETRACESIZE            110 //Obsolete should remove

#define PTRSZ                     uint32_t
#define CUDADEBUG                   0

typedef union {
  int i;
  float f;
} myintfloat;

//uint32_t h_trace_ptr;
//uint32_t d_trace_ptr;
//uint32_t* h_pausenextbb_qemu;

uint8_t relaunch_same_kernel;
uint8_t reexecute_same_kernel;
uint64_t numthreads;
uint64_t blockthreads;
uint64_t numbytes;
uint64_t tracesize = 1;

// This should be obtained from esesc.
uint32_t DFL_REG32 = 1;        	//Number of 32 bit registers in LIs and LOs
uint32_t DFL_REG64 = 1;         	//Number of 64 bit registers in LIs and LOs
uint32_t DFL_REGFP = 1;         	//Number of floating point registers in LIs and LOs
uint32_t DFL_REGFP64 = 1;         	//Number of 64 bit floating point registers in LIs and LOs

uint32_t SM_REG32 = 1;         	//Number of 32 bit registers stored in shared memory
uint32_t SM_REG64 = 1;        	  //Number of 64 bit registers stored in shared memory
uint32_t SM_REGFP = 1;         	//Number of floating point registers stored in shared memory
uint32_t SM_REGFP64 = 1;         	//Number of 64 bit floating point registers stored in shared memory
uint32_t SM_ADDR = 1;         	  //Total number of registers stored in shared memory

uint32_t TRACESIZE = 1;
//uint32_t exitcount = 0;

T_TRACE* d_trace;
T_CONTEXT_32 *d_context_32,*d_context_fp,*d_shared_fp,*d_shared_32;
T_SHARED_ADDR *d_shared_addr;
T_CONTEXT_64 *d_context_64,*d_shared_64, *d_context_fp64, *d_shared_fp64  ;

T_TRACE* h_trace;
T_CONTEXT_32 *h_context_32,*h_context_fp;
T_SHARED_32 *h_shared_fp, *h_shared_32;
T_CONTEXT_64 *h_context_64, *h_context_fp64, *h_shared_fp64;
T_SHARED_64 *h_shared_64;
T_SHARED_ADDR *h_shared_addr;

uint32_t* array;
uint32_t array_ptr;
char* last_kernel = NULL;
char* current_kernel = NULL;

uint32_t callRealCudaFunctions(struct Arguments* p1, uint32_t func_num, void* env, uint32_t qemuid);

struct ConfigParamStruct{
  dim3 a;
  dim3 b;
  size_t c;
  cudaStream_t d;
};

struct ConfigSetupArgStruct{
  void* a;
  size_t b;
  size_t c;
};

struct ConfigParamStruct config_params[40];
int param_count = 0;
struct ConfigSetupArgStruct setup_args[80];
int setup_arg_count = 0;

int nextbb = 0;
int kernelcount = 0;
int loopcount = 0;
int tempvar = 0;
struct cudaChannelFormatDesc tempf;
#if  defined (CONFIG_ESESC_system) || defined (CONFIG_ESESC_user)
/*
   int esesc_allow_large_tb_gpu=0; // Don't need both of these for now...
   int esesc_single_inst_tb_gpu=1;

   void esesc_set_rabbit_gpu(void)
// This function enables fastforwarding.
{
if (esesc_allow_large_tb == 1 && esesc_single_inst_tb == 0)
return;
esesc_allow_large_tb_gpu = 1;
esesc_single_inst_tb_gpu = 0;
}

void esesc_set_timing_gpu(void)
// This function enables detailed simulation.
{
if (esesc_allow_large_tb == 0 && esesc_single_inst_tb == 1)
return;
esesc_allow_large_tb_gpu = 0;
esesc_single_inst_tb_gpu = 1;
}
*/
#endif



uint32_t callRealCudaFunctions(struct Arguments* p1, uint32_t func_num, void* env, uint32_t qemuid){
  //TODO: return the value back to the cuda application.
  int returnval;

  switch (func_num){

    case STORENUMTHREADS:
      //numthreads =*(uint64_t*) ((PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;

    case STORENUMBYTES:
      numbytes =*(uint64_t*) ((PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;

    case STORETRACESIZE:
      tracesize =*(uint64_t*) ((PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;

    case STOREHOSTPTR:
      //      h_trace_ptr = (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])));
      //      h_trace = (uint32_t*)(h_trace_ptr);
      break;

    case STOREDEVPTR:
      //      d_trace_ptr =(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])));
      //      d_trace = (uint32_t*)(d_trace_ptr);
      break;

    case STOREHOSTPTRSIZE:
    case STOREDEVPTRSIZE:
      break;

    case CUDAPROGDONE:
      GPUReader_CUDA_exec_done();
      if (tracesize!= 0) free(h_trace);
      /*
      if (tracesize!= 0) free(h_context_32);
      if (tracesize!= 0) free(h_context_64);
      if (tracesize!= 0) free(h_context_fp);

      if (tracesize!= 0) free(h_shared_32);
      if (tracesize!= 0) free(h_shared_64);
      if (tracesize!= 0) free(h_shared_fp);
      if (tracesize!= 0) free(h_shared_addr);
      */
      cudaFree(d_trace);
      cudaFree(d_context_32);
      cudaFree(d_context_64);
      cudaFree(d_context_fp);

      cudaFree(d_shared_32);
      cudaFree(d_shared_64);
      cudaFree(d_shared_fp);
      cudaFree(d_shared_addr);

      break;


    case ALLOCINITHOSTDEVPTRS:

      param_count     = 0;
      setup_arg_count = 0;

      numthreads = GPUReader_getNumThreads();
      blockthreads = GPUReader_getBlockThreads();

      array_ptr = (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])));
      array = (uint32_t*)(array_ptr);

      uint64_t numthreads_frombin = *((uint64_t*)(array[0]));
      uint64_t blockthreads_frombin = *((uint64_t*)(array[1]));

      current_kernel = (char*) (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1])));
      if ( strcmp(current_kernel,"") == 0 ){
        //ERROR:
        printf("\ncudaarm.h: Kernel Str Specified by the benchmark is blank!\n");
        exit(-1);
      }

      fprintf (stderr, "Numthreads %llu Blockthreads %llu for benchmark kernel %s\n\n", numthreads_frombin, blockthreads_frombin, current_kernel);

      //if (current_kernel != last_kernel) {
        GPUReader_setCurrentKernel(current_kernel);
        if (CUDADEBUG) printf("setting: %s\n",(const char*) (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))));
      //}

      //if ((numthreads != numthreads_frombin) || (blockthreads != blockthreads_frombin)){
      GPUReader_setNumThreads(numthreads_frombin, blockthreads_frombin);
      //} 

      if ((current_kernel != last_kernel) || (last_kernel == NULL) || (numthreads != numthreads_frombin) || (blockthreads != blockthreads_frombin) ){

        if (last_kernel != NULL){
          //This means we are in the loop because the numthreads are different. Therefore we have to free what we have first.
          free(h_trace);
          cudaFree(d_trace);
          cudaFree(d_context_32);
          cudaFree(d_context_64);
          cudaFree(d_context_fp);
          cudaFree(d_context_fp64);

          cudaFree(d_shared_32);
          cudaFree(d_shared_64);
          cudaFree(d_shared_fp);
          cudaFree(d_shared_fp64);
          cudaFree(d_shared_addr);
        }

        numthreads = numthreads_frombin;
        blockthreads = blockthreads_frombin;

        //We use only TRACESIZE
        GPUReader_getKernelParameters(&DFL_REG32,&DFL_REG64,&DFL_REGFP, &DFL_REGFP64, &SM_REG32,&SM_REG64,&SM_REGFP, &SM_REGFP64, &SM_ADDR ,&TRACESIZE);

        tracesize = TRACESIZE;
        numbytes = tracesize*sizeof(T_TRACE)*numthreads;

        if (CUDADEBUG) printf("\n\n **** Allocating Device pointers **** \n\n");

        h_trace = (T_TRACE*) malloc(numthreads *tracesize * sizeof(T_TRACE));
        if (h_trace == NULL){
          printf("\nCould not allocate memory for h_trace");
          exit(-1);
        }

        //Allocate on Device
        returnval = cudaMalloc( (void**) &d_trace,      numthreads *tracesize * sizeof(T_TRACE));
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_trace **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }
        memset((void *) h_trace,0,numthreads *tracesize * sizeof(T_TRACE));

        returnval = ( cudaMemcpy( d_trace,     h_trace,      numthreads * tracesize * sizeof(T_TRACE),cudaMemcpyHostToDevice ) );
        if (returnval != 0) {
          printf("\n*********** 399: CUDAMEMCPY ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }


        returnval = ( cudaMalloc( (void**) &d_context_32, numthreads * DFL_REG32 * sizeof(T_CONTEXT_32) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_context_32 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_context_64, numthreads * DFL_REG64 * sizeof(T_CONTEXT_64) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_context_64 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_context_fp, numthreads * DFL_REGFP * sizeof(T_CONTEXT_32) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_context_fp **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_context_fp64, numthreads * DFL_REGFP64 * sizeof(T_CONTEXT_64) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_context_fp64 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_shared_32,  numthreads * SM_REG32 * sizeof(T_SHARED_32) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_shared_32 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_shared_64,  numthreads * SM_REG64 * sizeof(T_SHARED_64) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_shared_64 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_shared_fp,  numthreads * SM_REGFP * sizeof(T_SHARED_32) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_shared_fp **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_shared_fp64,  numthreads * SM_REGFP64 * sizeof(T_SHARED_64) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_shared_fp64 **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

        returnval = ( cudaMalloc( (void**) &d_shared_addr,numthreads * SM_ADDR * sizeof(T_SHARED_ADDR) ) );
        if (returnval != 0) {
          printf("\n***********CUDAMALLOC ERROR #%d while allocating d_shared_addr **********   : %s\n",returnval,cudaGetErrorString(returnval));
          exit(1);
        }

      }

        /******************* MEMSET ********************/
		  
        returnval = cudaMemset (d_trace, 0, numthreads * TRACESIZE * sizeof(T_TRACE));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset D_TRACE ERROR #%d**********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval),numthreads * TRACESIZE * sizeof(T_TRACE));
          //exit(1);
        }
        
		  
        returnval = cudaMemset (d_context_32, 0, numthreads * DFL_REG32 * sizeof(T_CONTEXT_32));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset DFL_REG32 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval),numthreads * DFL_REG32 * sizeof(T_CONTEXT_32));
          //exit(1);
        }
        
        returnval = cudaMemset (d_context_64, 0, numthreads * DFL_REG64 * sizeof(T_CONTEXT_64));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset DFL_REG64 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * DFL_REG64 * sizeof(T_CONTEXT_64));
          //exit(1);
        }
        
        returnval = cudaMemset (d_context_fp, 0, numthreads * DFL_REGFP * sizeof(T_CONTEXT_32));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset DFL_REGFP ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * DFL_REGFP * sizeof(T_CONTEXT_32));
          //exit(1);
        }
        
        returnval = cudaMemset (d_context_fp64, 0, numthreads * DFL_REGFP64 * sizeof(T_CONTEXT_64));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset DFL_REGFP64 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * DFL_REGFP64 * sizeof(T_CONTEXT_64));
          //exit(1);
        }

        returnval = cudaMemset (d_shared_32, 0, numthreads * SM_REG32 * sizeof(T_SHARED_32));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset SM_REG32 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval),numthreads * SM_REG32 * sizeof(T_SHARED_32));
          //exit(1);
        }

        returnval = cudaMemset (d_shared_64, 0, numthreads * SM_REG64 * sizeof(T_SHARED_64));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset SM_REG64 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * SM_REG64 * sizeof(T_SHARED_64));
          //exit(1);
        }

        returnval = cudaMemset (d_shared_fp, 0, numthreads * SM_REGFP * sizeof(T_SHARED_32));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset SM_REGFP ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * SM_REGFP * sizeof(T_SHARED_32));
          //exit(1);
        }

        returnval = cudaMemset (d_shared_fp64, 0, numthreads * SM_REGFP64 * sizeof(T_SHARED_64));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset SM_REGFP64 ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * SM_REGFP64 * sizeof(T_SHARED_64));
          //exit(1);
        }

        returnval = cudaMemset (d_shared_addr, 0, numthreads * SM_ADDR * sizeof(T_SHARED_ADDR));
        if (returnval != 0) {
          printf("\n*********** Allocinit: cudaMemset SM_ADDR ERROR #%d **********   : %s (%llu) \n",returnval,cudaGetErrorString(returnval), numthreads * SM_ADDR * sizeof(T_SHARED_ADDR));
          //exit(1);
        }

      array[0] = (uint32_t) d_trace;
      array[1] = (uint32_t) d_context_32;
      array[2] = (uint32_t) d_context_fp;
      array[3] = (uint32_t) d_shared_fp;
      array[4] = (uint32_t) d_shared_32;
      array[5] = (uint32_t) d_shared_addr;
      array[6] = (uint32_t) d_context_64;
      array[7] = (uint32_t) d_shared_64;
      array[8] = (uint32_t) d_context_fp64;
      array[9] = (uint32_t) d_shared_fp64;
 /*
      fprintf(stderr,"\n\n\n\n\n\nQEMUDR[%s]:h_trace[size= %llu] = %p d_trace = %p d_context_32= %p d_context_64 = %p d_context_fp = %p d_context_fp64 = %p d_shared_32 = %p d_shared_64 = %p d_shared_fp = %p,  d_shared_fp64 = %p, d_shared_addr = %p\n\n\n\n\n\n",
          current_kernel,
          numthreads*TRACESIZE*4,
          h_trace,
          d_trace,
          d_context_32,
          d_context_64,
          d_context_fp,
          d_context_fp64,
          d_shared_32,
          d_shared_64,
          d_shared_fp,
          d_shared_fp64,
          d_shared_addr);
*/
      last_kernel = current_kernel;
      break;


    case CUDAREGISTERFUNCTION:

      __cudaRegisterFunction(
          (void**)     (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (const char*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (char*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[2]))),
          (const char*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[3]))),
          (*(int*)((PTRSZ)(p1->args[4]))),
          (uint3*)     (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[5]))),
          (uint3*)     (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[6]))),
          (dim3*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[7]))),
          (dim3*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[8]))),
          (int*)       (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[9])))
          );
      //__cudaRegisterFunction(__cudaFatCubinHandle, (const char*)funptr, (char*)__device_fun(fun), #fun, thread_limit, __ids)

      if (CUDADEBUG) printf("Last Error: %s\n",cudaGetErrorString(cudaGetLastError()));
      break;


    case CUDAMALLOC:
      if (CUDADEBUG) printf("Calling function cudaMalloc ---> %d bytes\n",(int)(*(size_t*)((PTRSZ)(p1->args[1]))));

      returnval = cudaMalloc(
          (void**)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (*(size_t*)((PTRSZ)(p1->args[1])))
          );

      if (returnval != 0) {
        printf("\n***********CUDAMALLOC ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      GPUReader_mallocAddress(
          *(uint32_t*)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (*(size_t*)((PTRSZ)(p1->args[1]))),
          &qemuid
      );

      break;

    case CUDAMEMCPY:
      if (CUDADEBUG) printf("\nCalling cudaMemcpy -----> %d BYTES\n",(int)(*(size_t*)((PTRSZ)(p1->args[2]))));
      returnval = cudaMemcpy(
          (void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (*(size_t*)((PTRSZ)(p1->args[2]))),
          (*(enum cudaMemcpyKind*)((PTRSZ)(p1->args[3])))
      );

      if (returnval != 0) {
        printf("\n*********** CUDAMEMCPY ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      returnval = cudaThreadSynchronize();
      if (returnval != 0) {
        printf("\n*********** cudaThreadSynchronize ERROR #%d after CudaMemcpy called by binary %s**********   : %s\n",returnval,current_kernel,cudaGetErrorString(returnval));
        exit(1);
      }

      GPUReader_mapcudaMemcpy(
          *(uint32_t*)((PTRSZ)(p1->args[0])),
          *(uint32_t*)((PTRSZ)(p1->args[1])),
          (*(size_t*)((PTRSZ)(p1->args[2]))),
          (*(enum cudaMemcpyKind*)((PTRSZ)(p1->args[3]))),
          env, 
          &qemuid
          //((CPUState *)env)->fid
      );
      
      ((CPUState *)env)->fid = qemuid;

      break;

    case CUDACONFIGURECALL:
      //cudaError_t cudaConfigureCall	(	dim3 	gridDim, dim3 	blockDim, size_t 	sharedMem, cudaStream_t 	stream)
      //Specifies the grid and block dimensions for the device call to be executed similar to the execution configuration
      //syntax. cudaConfigureCall() is stack based. Each call pushes data on top of an execution stack. This data contains
      //the dimension for the grid and thread blocks, together with any arguments for the call.


      if (CUDADEBUG) printf("\nCalling cudaConfigureCall..shared mem size = %d Stream = %d\n",(int)(*(size_t*)((PTRSZ)(p1->args[2]))),(int)*(cudaStream_t*)(PTRSZ)(p1->args[3]));

      returnval = cudaConfigureCall((dim3)(*(dim3*)((PTRSZ)(p1->args[0]))),
          (dim3)(*(dim3*)((PTRSZ)(p1->args[1]))),
          (*(size_t*)((PTRSZ)(p1->args[2]))),
          (*(cudaStream_t*)((PTRSZ)(p1->args[3])))
          );

      //Store the params
      config_params[param_count].a = (dim3)(*(dim3*)((PTRSZ)(p1->args[0])));
      config_params[param_count].b = (dim3)(*(dim3*)((PTRSZ)(p1->args[1])));
      config_params[param_count].c = (*(size_t*)((PTRSZ)(p1->args[2])));
      config_params[param_count].d = (*(cudaStream_t*)((PTRSZ)(p1->args[3])));
      param_count += 1;

      if (returnval != 0) {
        printf("\n*********** CUDACONFIGURECALL ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      break;

    case CUDASETUPARGUMENTS:
      //cudaError_t cudaSetupArgument	(	T arg, size_t 	offset)
      //Pushes size bytes of the argument pointed to by arg at offset bytes from the start of the
      //parameter passing area, which starts at offset 0.The arguments are stored in the top of
      //the execution stack. cudaSetupArgument() must be preceded by a call to cudaConfigureCall().

      if (CUDADEBUG)
      {
        printf("\nCalling cudaSetupArgument pointer= %d size = %u offset = %u\n",(int)*(int*)(const void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),(int)(*(size_t*)((PTRSZ)(p1->args[1]))),(int)(*(size_t*)((PTRSZ)(p1->args[2]))));
      }
      returnval = cudaSetupArgument((const void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (*(size_t*)((PTRSZ)(p1->args[1]))),
          (*(size_t*)((PTRSZ)(p1->args[2])))
          );

      if (returnval != 0) {
        printf("\n*********** CUDASETUPARGUMENT ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      //Store the args
      setup_args[setup_arg_count].a = (void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])));
      setup_args[setup_arg_count].b = (*(size_t*)((PTRSZ)(p1->args[1])));
      setup_args[setup_arg_count].c = (*(size_t*)((PTRSZ)(p1->args[2])));
      setup_arg_count += 1;

      break;

    case CUDALAUNCH:
      
      reexecute_same_kernel = 0;
      //exitcount = 0;
      
      do { 

        if (CUDADEBUG) printf("\nCalling cudaLaunch\n");
        relaunch_same_kernel = 1;
        GPUReader_declareLaunch();
        // init the trace array
        GPUReader_init_trace(h_trace);

        while (relaunch_same_kernel == 1){

          returnval = cudaMemcpy((void*)d_trace,(void*)h_trace,numbytes,cudaMemcpyHostToDevice);
          if (returnval != 0) {
            printf("\n*********** ERROR copying h_trace to d_trace #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
            exit(1);
          }

          // Launch kernel
          returnval = cudaLaunch((const char*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])))); //symbol
          if (returnval != 0) {
            printf("\n*********** CUDALAUNCH ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
            exit(1);
          } else {
            returnval = cudaThreadSynchronize();
            if (returnval != 0) {
              printf("\n*********** cudaThreadSynchronize ERROR #%d after kernel %s**********   : %s\n",returnval,current_kernel,cudaGetErrorString(returnval));
              exit(1);
            }
            //Copy from Device to Host
            returnval = cudaMemcpy((void*)h_trace,(void*)d_trace,numbytes,cudaMemcpyDeviceToHost);
            if (returnval != 0) {
              printf("\n*********** ERROR copying d_trace to h_trace #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
              exit(1);
            }

            tempvar++;
            // Decode the trace
            // printf("\n\nCalling GPUDecodeTrace\n\n");
            relaunch_same_kernel = (GPUReader_decode_trace(h_trace,&qemuid,env));
            //exitcount++;
            //fprintf(stderr, "Executing loop %d times\n",exitcount);

            /*          if (exitcount >= 81){
                        exit(-1); // Alamelu
                        }
                        */

            if (relaunch_same_kernel){
              //Call Configurecall
              for (loopcount = 0; loopcount < param_count; loopcount++) {
                returnval = cudaConfigureCall(config_params[loopcount].a,config_params[loopcount].b,config_params[loopcount].c,config_params[loopcount].d);
                if (returnval != 0) {
                  printf("\n*********** CUDACONFIGURECALL ERROR #%d loopcount = %d**********   : %s\n",returnval,loopcount,cudaGetErrorString(returnval));
                  exit(1);
                }
              }
              //Call Setup Arguments
              for (loopcount = 0; loopcount < setup_arg_count; loopcount++) {
                returnval = cudaSetupArgument(setup_args[loopcount].a,
                    setup_args[loopcount].b,
                    setup_args[loopcount].c
                    );

                if (returnval != 0) {
                  printf("\n*********** cudaSetupArgument ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
                  exit(1);
                }
              }
            }
          }
        }//end of while loop
        reexecute_same_kernel = GPUReader_reexecute_same_kernel();
        if (reexecute_same_kernel == 1){
          //If we are reexecuting the same kernel again, then we need to call configureCall and setupArgs again.
          
          //Call Configurecall
          for (loopcount = 0; loopcount < param_count; loopcount++) {
            returnval = cudaConfigureCall(config_params[loopcount].a,config_params[loopcount].b,config_params[loopcount].c,config_params[loopcount].d);
            if (returnval != 0) {
              printf("\n*********** CUDACONFIGURECALL ERROR #%d loopcount = %d**********   : %s\n",returnval,loopcount,cudaGetErrorString(returnval));
              exit(1);
            }
          }
          //Call Setup Arguments
          for (loopcount = 0; loopcount < setup_arg_count; loopcount++) {
            returnval = cudaSetupArgument(setup_args[loopcount].a,
                setup_args[loopcount].b,
                setup_args[loopcount].c
                );

            if (returnval != 0) {
              printf("\n*********** cudaSetupArgument ERROR #%d**********   : %s\n",returnval,cudaGetErrorString(returnval));
              exit(1);
            }
          }
          printf("\n\n\n ***************** RE-executing same kernel *****************\n\n\n");
        }
      } while (reexecute_same_kernel == 1);

      ((CPUState *)env)->fid = qemuid;
      //printf("\n\n\n Setting new qemu fid = %d\n\n\n",qemuid);
      param_count     = 0;
      setup_arg_count = 0;
      break;



    case CUDAFREE:
      if (CUDADEBUG)
        printf("\nCalling cudaFree on pointer %p\n",(void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));

      cudaFree((void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;



    case CUDAREGISTERFATBINARY:
      if (CUDADEBUG)
        printf("\n Calling cudaRegisterFatBinary\n");
      if (CUDADEBUG)
        printf("\n Recived  %p , %p\n",(void*)(PTRSZ)p1->args[0],g2h((void*)(PTRSZ)p1->args[0]));

      void** retHandle = 0x0;
      retHandle = __cudaRegisterFatBinary(((void*)(PTRSZ)p1->args[0]));//(void*)brec);
      p1->args[2] = *(uint32_t*)&(retHandle);

      if (CUDADEBUG)
        printf("\n Return value from __cudaRegisterFatBinary %p:%u \n",retHandle,p1->args[2]);

      break;



    case CUDAUNREGISTERFATBINARY:
      if (CUDADEBUG) printf("\n Calling cudaUnregisterFatBinary\n");
      __cudaUnregisterFatBinary((void**)(PTRSZ)(*(uint32_t*)(PTRSZ)(p1->args[0])));
      break;

#if 0 
    case MALLOCLE:
      if (CUDADEBUG) 
        printf("\n MallocLE not executed. You are probably running an outdate version of this program"); 
      exit(-1);

      int size = tswap32(*(size_t*)((PTRSZ)tswap32(p1->args[0])));
/*
      if (CUDADEBUG) printf("size is %d\n",size);
      abi_ulong temp = (mallocle(size));
      *((uint32_t*)(PTRSZ)tswap32(p1->args[1])) = tswap32(temp);
*/
      break;
#endif

    case PRINTDIRECTQEMUDATA:
      if (CUDADEBUG) printf("\nCalling printqemudirectdata\n");
      int type = tswap32(*(int*)((PTRSZ)tswap32(p1->args[1])));
      abi_ulong addr = (abi_ulong)tswap32(*(uint32_t*)((PTRSZ)tswap32(p1->args[0])));
      if (CUDADEBUG) printf("Address received is %u\n\n",addr);
      char* y = (char* )(PTRSZ)addr;
      myintfloat a11;
      switch(type){
        case 1:
          printf("Direct QEMU DATA : %d\n",*((int*)(PTRSZ)addr));
          printf("Direct QEMU DATA (swapped) : %d\n",tswap32(*((int*)(PTRSZ)addr)));
          break;
        case 2:
          printf("Direct QEMU DATA : %f\n",*(float*)(PTRSZ)addr);
          a11.i = tswap32(*(uint32_t*)(PTRSZ)addr);
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

    case CUDAGETDEVICECOUNT:
      if (CUDADEBUG) printf("\nCalling cudaGetDeviceCount\n");
      cudaGetDeviceCount((int*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;
    case CUDAGETDEVICEPROPERTIES:
      if (CUDADEBUG) printf("\nCalling cudaGetDeviceProperties\n");
      cudaGetDeviceProperties((struct cudaDeviceProp *)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (*(int32_t*)((PTRSZ)(p1->args[1])))
          );
      break;
    case CUDASETDEVICE:
      if (CUDADEBUG) printf("\nCalling cudaSetDevice %u\n",(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      cudaSetDevice((*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;

    case CUDAGETDEVICE:
      if (CUDADEBUG) printf("\nCalling cudaGetDevice %u\n",(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      cudaGetDevice((int*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))));
      break;

    case CUDAMEMCPYTOSYMBOL:
      if (CUDADEBUG) printf("\nCalling cudaMemcpyToSymbol");
      //cudaMemcpyToSymbol(const char *symbol, const void *src,size_t count, size_t offset, enum cudaMemcpyKind kind )

      returnval = cudaMemcpyToSymbol(
          (const char *)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (const void*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (*(size_t*)((PTRSZ)(p1->args[2]))),
          (*(size_t*)((PTRSZ)(p1->args[3]))),
          (*(enum cudaMemcpyKind*)((PTRSZ)(p1->args[4])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaMemcpyToSymbol ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      break;

    case CUDAMEMSET:

      if (CUDADEBUG) printf("\nCalling cudaMemset");

      //cudaMemset(void *mem, int c, size_t count)
      returnval = cudaMemset(
          (void *)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (*(int*)((PTRSZ)(p1->args[1]))),
          (*(size_t*)((PTRSZ)(p1->args[2])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaMemset ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      break;

    case CUGAREGISTERVAR:
      if (CUDADEBUG) printf("\nCalling __cudaRegisterVar");

      //__cudaRegisterVar( void **fatCubinHandle, char *hostVar, char *deviceAddress, const char *deviceName, int ext, int size, int constant, int global )
      __cudaRegisterVar(
          (void**)     (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (char*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (char*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[2]))),
          (const char*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[3]))),
          (*(int*)     ((PTRSZ)(p1->args[4]))),
          (*(int*)     ((PTRSZ)(p1->args[5]))),
          (*(int*)     ((PTRSZ)(p1->args[6]))),
          (*(int*)     ((PTRSZ)(p1->args[7])))
          );

      break;

    case CUDATHREADSYNCHRONIZE:
      if (CUDADEBUG) printf("\nCalling cudaThreadSynchronize");
      returnval = cudaThreadSynchronize();
      if (returnval != 0) {
        printf("\n*********** cudaThreadSynchronize ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;

    case CUDATHREADEXIT:
      if (CUDADEBUG) printf("\nCalling cudaThreadExit");
      cudaThreadExit();
      break;

    case CUDAGETLASTERROR:
      if (CUDADEBUG) printf("\nCalling cudaGetLastError");
      returnval = cudaGetLastError();
      if (returnval != 0) {
        printf("\n*********** cudaGetLastError returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      *((uint32_t*)((PTRSZ)(p1->args[0]))) = returnval;
      break;

    case CUGAREGISTERTEX:
      //__cudaRegisterTexture( void **fatCubinHandle, const struct textureReference *hostVar, const void **deviceAddress, const char *deviceName, int dim, int norm, int ext )
      //

      __cudaRegisterTexture( 
          (void**)                              (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (const struct textureReference*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (const void**)                        (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[2]))),
          (const char*)                         (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[3]))),
          (*(int*)                              ((PTRSZ)(p1->args[4]))),
          (*(int*)                              ((PTRSZ)(p1->args[5]))),
          (*(int*)                              ((PTRSZ)(p1->args[6])))
          );

      break;

    case CUDABINDTEXTURE:
      //cudaError_t CUDARTAPI cudaBindTexture(size_t *offset, const struct textureReference *texref, const void *devPtr,const struct cudaChannelFormatDesc *desc, size_t size )
      returnval = cudaBindTexture(
          (size_t*)                             (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
          (const struct textureReference*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1]))),
          (const void*)                         (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[2]))),
          (const struct cudaChannelFormatDesc*) (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[3]))),
          (*(size_t*)                           (PTRSZ)(p1->args[4]))
          );

      if (returnval != 0) {
        printf("\n*********** cudaBindTexture returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      break;


    case CUDAUNBINDTEXTURE:
      //cudaError_t CUDARTAPI cudaUnbindTexture(const struct textureReference *texref);

      returnval = cudaUnbindTexture(
          (const struct textureReference*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaUnbindTexture returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }

      break;

    case CUDAFUNCGETATTRIBUTES:
      //cudaError_t CUDARTAPI cudaFuncGetAttributes(struct cudaFuncAttributes *attr,const char *func);

      returnval = cudaFuncGetAttributes( (struct cudaFuncAttributes*)      (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0]))),
                                         (const char* )                    (PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[1])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaUnbindTexture returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;


    case CUDACREATECHANNELDESC:

        //cudaCreateChannelDesc(int x, int y, int z, int w, enum cudaChannelFormatKind f)

      tempf = cudaCreateChannelDesc( *(int*)p1->args[0], 
          *(int*)p1->args[1], 
          *(int*)p1->args[2], 
          *(int*)p1->args[3],
          *((enum cudaChannelFormatKind*) p1->args[4])
          );

        p1->args[0]  = (uint32_t) (&tempf);

      break;


    case CUDAEVENTCREATE:
      returnval = cudaEventCreate(
          (cudaEvent_t*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventCreate returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;

    case CUDAEVENTRECORD:
      returnval = cudaEventRecord(
          (*(cudaEvent_t*)((PTRSZ)(p1->args[0])))
          ,(*(cudaStream_t*)((PTRSZ)(p1->args[1])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventRecord returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;

    case CUDAEVENTQUERY:
      returnval = cudaEventQuery(
          (*(cudaEvent_t*)((PTRSZ)(p1->args[0])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventQuery returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;


    case CUDAEVENTSYNCHRONIZE:
      returnval = cudaEventSynchronize(
          (*(cudaEvent_t*)((PTRSZ)(p1->args[0])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventSynchronize returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;

    case CUDAEVENTDESTROY:
      returnval = cudaEventDestroy(
          (*(cudaEvent_t*)((PTRSZ)(p1->args[0])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventDestroy returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;


    case CUDAEVENTELAPSEDTIME:
      returnval = cudaEventElapsedTime(
          (float*)(PTRSZ)(*(uint32_t*)((PTRSZ)(p1->args[0])))
          ,(*(cudaEvent_t*)((PTRSZ)(p1->args[1])))
          ,(*(cudaEvent_t*)((PTRSZ)(p1->args[2])))
          );

      if (returnval != 0) {
        printf("\n*********** cudaEventElapsedTime returned ERROR #%d **********   : %s\n",returnval,cudaGetErrorString(returnval));
        exit(1);
      }
      break;


    default:
      printf("\n*************FIXME****************** Cuda function %d not defined.\n",func_num);
      //Set ret value (ret for syscall.c)
  }
  return 0;
}

#endif
