/*
 * Cuda runtime API, along with the requisite structure and enum definitions. v2.1
 * */
#ifndef CUDA_RUNTIME_H_INCLUDED
#define CUDA_RUNTIME_H_INCLUDED

#include "host_defines.h"
#include "builtin_types.h"
#include "__cudaFatFormat.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#define __dv(x) =x
#else
#define __dv(x)
#endif

#define CUDARTAPI

/*******************************************************************************
* *                                                                              *
* *                                                                              *
* *                                                                              *
* *******************************************************************************/

extern void** __cudaRegisterFatBinary(void *fatCubin);
extern void __cudaUnregisterFatBinary(void **fatCubinHandle);
extern void __cudaRegisterVar(void **fatCubinHandle, char *hostVar, char *deviceAddress, const char *deviceName, int ext, int size, int constant, int global);

extern __host__ cudaError_t CUDARTAPI cudaFuncGetAttributes(struct cudaFuncAttributes *attr, const char *func);

extern __host__ cudaError_t CUDARTAPI cudaHostAlloc(void **pHost, size_t size, unsigned int flags);

extern void __cudaRegisterTexture(
    void **fatCubinHandle,
    const struct textureReference *hostVar,
    const void **deviceAddress,
    const char *deviceName,
    int dim,
    int norm,
    int ext
);

extern void __cudaRegisterShared(
    void **fatCubinHandle,
    void **devicePtr
);

extern void __cudaRegisterSharedVar(
    void **fatCubinHandle,
    void **devicePtr,
    size_t size,
    size_t alignment,
    int storage
);

extern void __cudaRegisterFunction(
    void **fatCubinHandle,
    const char *hostFun,
    char *deviceFun,
    const char *deviceName,
    int thread_limit,
    uint3 *tid,
    uint3 *bid,
    dim3 *bDim,
    dim3 *gDim,
    int *wSize
);



/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaMalloc3D(struct cudaPitchedPtr* pitchDevPtr, struct cudaExtent extent);
extern  cudaError_t CUDARTAPI cudaMalloc3DArray(struct cudaArray** arrayPtr, const struct cudaChannelFormatDesc* desc, struct cudaExtent extent);
extern  cudaError_t CUDARTAPI cudaMemset3D(struct cudaPitchedPtr pitchDevPtr, int value, struct cudaExtent extent);
extern  cudaError_t CUDARTAPI cudaMemcpy3D(const struct cudaMemcpy3DParms *p);
extern  cudaError_t CUDARTAPI cudaMemcpy3DAsync(const struct cudaMemcpy3DParms *p, cudaStream_t stream);


/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaMalloc(void **devPtr, size_t size);
extern  cudaError_t CUDARTAPI cudaMallocHost(void **ptr, size_t size);
extern  cudaError_t CUDARTAPI cudaMallocPitch(void **devPtr, size_t *pitch, size_t width, size_t height);
extern  cudaError_t CUDARTAPI cudaMallocArray(struct cudaArray **array, const struct cudaChannelFormatDesc *desc, size_t width, size_t height __dv(1));
extern  cudaError_t CUDARTAPI cudaFree(void *devPtr);
extern  cudaError_t CUDARTAPI cudaFreeHost(void *ptr);
extern  cudaError_t CUDARTAPI cudaFreeArray(struct cudaArray *array);


/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaMemcpy(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpyToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpyFromArray(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpyArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t count, enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToDevice));
extern  cudaError_t CUDARTAPI cudaMemcpy2D(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpy2DToArray(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpy2DFromArray(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind);
extern  cudaError_t CUDARTAPI cudaMemcpy2DArrayToArray(struct cudaArray *dst, size_t wOffsetDst, size_t hOffsetDst, const struct cudaArray *src, size_t wOffsetSrc, size_t hOffsetSrc, size_t width, size_t height, enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToDevice));
extern  cudaError_t CUDARTAPI cudaMemcpyToSymbol(const char *symbol, const void *src, size_t count, size_t offset __dv(0), enum cudaMemcpyKind kind __dv(cudaMemcpyHostToDevice));
extern  cudaError_t CUDARTAPI cudaMemcpyFromSymbol(void *dst, const char *symbol, size_t count, size_t offset __dv(0), enum cudaMemcpyKind kind __dv(cudaMemcpyDeviceToHost));

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaMemcpyAsync(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpyToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpyFromArrayAsync(void *dst, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t count, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpy2DAsync(void *dst, size_t dpitch, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpy2DToArrayAsync(struct cudaArray *dst, size_t wOffset, size_t hOffset, const void *src, size_t spitch, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpy2DFromArrayAsync(void *dst, size_t dpitch, const struct cudaArray *src, size_t wOffset, size_t hOffset, size_t width, size_t height, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpyToSymbolAsync(const char *symbol, const void *src, size_t count, size_t offset, enum cudaMemcpyKind kind, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaMemcpyFromSymbolAsync(void *dst, const char *symbol, size_t count, size_t offset, enum cudaMemcpyKind kind, cudaStream_t stream);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaMemset(void *mem, int c, size_t count);
extern  cudaError_t CUDARTAPI cudaMemset2D(void *mem, size_t pitch, int c, size_t width, size_t height);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaGetSymbolAddress(void **devPtr, const char *symbol);
extern  cudaError_t CUDARTAPI cudaGetSymbolSize(size_t *size, const char *symbol);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaGetDeviceCount(int *count);
extern  cudaError_t CUDARTAPI cudaGetDeviceProperties(struct cudaDeviceProp *prop, int device);
extern  cudaError_t CUDARTAPI cudaChooseDevice(int *device, const struct cudaDeviceProp *prop);
extern  cudaError_t CUDARTAPI cudaSetDevice(int device);
extern  cudaError_t CUDARTAPI cudaGetDevice(int *device);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaBindTexture(size_t *offset, const struct textureReference *texref, const void *devPtr, const struct cudaChannelFormatDesc *desc, size_t size __dv(UINT_MAX));
extern  cudaError_t CUDARTAPI cudaBindTextureToArray(const struct textureReference *texref, const struct cudaArray *array, const struct cudaChannelFormatDesc *desc);
extern  cudaError_t CUDARTAPI cudaUnbindTexture(const struct textureReference *texref);
extern  cudaError_t CUDARTAPI cudaGetTextureAlignmentOffset(size_t *offset, const struct textureReference *texref);
extern  cudaError_t CUDARTAPI cudaGetTextureReference(const struct textureReference **texref, const char *symbol);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaGetChannelDesc(struct cudaChannelFormatDesc *desc, const struct cudaArray *array);
extern  struct cudaChannelFormatDesc CUDARTAPI cudaCreateChannelDesc(int x, int y, int z, int w, enum cudaChannelFormatKind f);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaGetLastError(void);
extern  const char* CUDARTAPI cudaGetErrorString(cudaError_t error);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaConfigureCall(dim3 gridDim, dim3 blockDim, size_t sharedMem __dv(0), cudaStream_t stream __dv(0));
extern  cudaError_t CUDARTAPI cudaSetupArgument(const void *arg, size_t size, size_t offset);
extern  cudaError_t CUDARTAPI cudaLaunch(const char *symbol);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaStreamCreate(cudaStream_t *stream);
extern  cudaError_t CUDARTAPI cudaStreamDestroy(cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaStreamSynchronize(cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaStreamQuery(cudaStream_t stream);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaEventCreate(cudaEvent_t *event);
extern  cudaError_t CUDARTAPI cudaEventRecord(cudaEvent_t event, cudaStream_t stream);
extern  cudaError_t CUDARTAPI cudaEventQuery(cudaEvent_t event);
extern  cudaError_t CUDARTAPI cudaEventSynchronize(cudaEvent_t event);
extern  cudaError_t CUDARTAPI cudaEventDestroy(cudaEvent_t event);
extern  cudaError_t CUDARTAPI cudaEventElapsedTime(float *ms, cudaEvent_t start, cudaEvent_t end);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaSetDoubleForDevice(double *d);
extern  cudaError_t CUDARTAPI cudaSetDoubleForHost(double *d);

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

extern  cudaError_t CUDARTAPI cudaThreadExit(void);
extern  cudaError_t CUDARTAPI cudaThreadSynchronize(void);

#ifdef __cplusplus
}
#endif

#endif

