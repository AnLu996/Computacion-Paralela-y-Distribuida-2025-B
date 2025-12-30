// rtm_universal.cu  — compatible con CUDA y modo CPU
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>

#define def_NPOP_AC 6
#define EPSILON1 0.01f
#define EPSILON2 1000.0f

// ================================================================
// Compatibilidad: si no hay CUDA, definir stubs para compilar en CPU
// ================================================================
#ifndef __CUDACC__
#define __host__
#define __device__
#define __global__
#define cudaMalloc(ptr, size) (*(ptr)=malloc(size), (*(ptr)?cudaSuccess:cudaErrorMemoryAllocation))
#define cudaFree(ptr) free(ptr)
#define cudaMemcpy(dst, src, size, kind) memcpy(dst, src, size)
#define cudaMemcpyHostToDevice 0
#define cudaMemcpyDeviceToHost 0
#define cudaGetLastError() cudaSuccess
#define cudaDeviceSynchronize()
#define cudaSuccess 0
#define cudaErrorMemoryAllocation 2
#define cudaGetErrorString(e) "No CUDA runtime"
using cudaError_t = int;
#endif

// ================================================================
// Utilidades
// ================================================================
static inline void cudaCheck(cudaError_t e, const char* msg){
    if(e != 0){
        fprintf(stderr, "CUDA error %s: %s\n", msg, cudaGetErrorString(e));
        exit(1);
    }
}

// ================================================================
// Ricker Wavelet
// ================================================================
__host__ __device__
inline float ricker(float t, float fc_half){
    const float pi = 3.14159265358979323846f;
    float alpha = sqrtf(0.5f) * pi * fc_half;
    float aux = (t * alpha) * (t * alpha);
    return (1.0f - 2.0f * aux) * expf(-aux);
}

// ================================================================
// Núcleo de inyección de fuente
// ================================================================
__global__ void inject_source_kernel(float* U0, const float* VP,
                                     int srcI, int srcJ, int srcK,
                                     float val, float fat,
                                     int NNOI, int NNOJ) {
    #ifdef __CUDACC__
    if (threadIdx.x == 0 && blockIdx.x == 0)
    #endif
    {
        unsigned long idx = (unsigned long)srcK * (unsigned long)NNOI * (unsigned long)NNOJ
                          + (unsigned long)srcJ * (unsigned long)NNOI
                          + (unsigned long)srcI;
        float v = VP[idx];
        U0[idx] += fat * v * v * val;
    }
}

// ================================================================
// Núcleo de stencil (actualización explícita 3D)
// ================================================================
__global__ void stencil_update_kernel(const float* __restrict__ U0,
                                      float* __restrict__ U1,
                                      const float* __restrict__ VP,
                                      int nnoi, int nnoj, int nnok,
                                      float FATX, float FATY, float FATZ,
                                      const float* __restrict__ W)
{
#ifdef __CUDACC__
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;
#else
    // En modo CPU, paraleliza con OpenMP si está disponible
    int i = 0, j = 0, k = 0;
#endif

    const int stride = nnoi * nnoj;

#ifdef __CUDACC__
    if (i < def_NPOP_AC || i >= nnoi - def_NPOP_AC ||
        j < def_NPOP_AC || j >= nnoj - def_NPOP_AC ||
        k < def_NPOP_AC || k >= nnok - def_NPOP_AC) return;
#else
    #pragma omp parallel for collapse(3) private(i,j,k)
    for(k = def_NPOP_AC; k < nnok - def_NPOP_AC; k++)
    for(j = def_NPOP_AC; j < nnoj - def_NPOP_AC; j++)
    for(i = def_NPOP_AC; i < nnoi - def_NPOP_AC; i++)
#endif
    {
        int index = (j * nnoi + i) + k * stride;
        float v = VP[index];
        if (v <= 0.0f) continue;

        float v2 = v * v;
        int offy = nnoi, offz = stride;

        // Derivadas centradas 6º orden
        float sx =
            W[6]*(U0[index - 6] + U0[index + 6]) +
            W[5]*(U0[index - 5] + U0[index + 5]) +
            W[4]*(U0[index - 4] + U0[index + 4]) +
            W[3]*(U0[index - 3] + U0[index + 3]) +
            W[2]*(U0[index - 2] + U0[index + 2]) +
            W[1]*(U0[index - 1] + U0[index + 1]) +
            W[0]*U0[index];

        float sy =
            W[6]*(U0[index - 6*offy] + U0[index + 6*offy]) +
            W[5]*(U0[index - 5*offy] + U0[index + 5*offy]) +
            W[4]*(U0[index - 4*offy] + U0[index + 4*offy]) +
            W[3]*(U0[index - 3*offy] + U0[index + 3*offy]) +
            W[2]*(U0[index - 2*offy] + U0[index + 2*offy]) +
            W[1]*(U0[index - offy]   + U0[index + offy])   +
            W[0]*U0[index];

        float sz =
            W[6]*(U0[index - 6*offz] + U0[index + 6*offz]) +
            W[5]*(U0[index - 5*offz] + U0[index + 5*offz]) +
            W[4]*(U0[index - 4*offz] + U0[index + 4*offz]) +
            W[3]*(U0[index - 3*offz] + U0[index + 3*offz]) +
            W[2]*(U0[index - 2*offz] + U0[index + 2*offz]) +
            W[1]*(U0[index - offz]   + U0[index + offz])   +
            W[0]*U0[index];

        U1[index] = 2.0f * U0[index] - U1[index]
                  + FATX * v2 * sx + FATY * v2 * sy + FATZ * v2 * sz;
    }
}

// ================================================================
// Programa principal
// ================================================================
int main(){
    int N, T;
    if (scanf("%d %d", &N, &T) != 2){
        fprintf(stderr, "Error: entrada esperada 'N T'\n");
        return 1;
    }

    const float dx=10.f, dy=10.f, dz=10.f, dt=0.0005f;
    const float FC=45.f, fcR=0.5f*FC, VP_def=3000.f;
    const float sourceTf=2.0f*sqrtf(3.14159265358979323846f)/FC;
    const float FATX=(dt*dt)/(dx*dx), FATY=(dt*dt)/(dy*dy), FATZ=(dt*dt)/(dz*dz);
    const float hW[7]={-3.0822809f,1.8019078f,-0.3273412f,0.0834572f,-0.02032018f,0.00385894f,-0.00042216f};

    size_t nnoi=N, nnoj=N, nnok=N, nel=(size_t)N*N*N, bytes=nel*sizeof(float);
    float *hU0=(float*)calloc(nel,sizeof(float)), *hU1=(float*)calloc(nel,sizeof(float));
    float *hVP=(float*)malloc(bytes);
    if(!hU0||!hU1||!hVP){ fprintf(stderr,"Error de memoria\n"); return 1; }

    std::fill(hVP, hVP+nel, VP_def);
    auto idx=[&](int i,int j,int k){return (size_t)k*N*N+(size_t)j*N+i;};
    for(int k=0;k<N;k++)for(int j=0;j<N;j++)for(int t=0;t<def_NPOP_AC;t++){
        hVP[idx(t,j,k)]=-VP_def; hVP[idx(N-1-t,j,k)]=-VP_def;
        hVP[idx(j,t,k)]=-VP_def; hVP[idx(j,N-1-t,k)]=-VP_def;
        hVP[idx(i,j,t)]=-VP_def; hVP[idx(i,j,N-1-t)]=-VP_def;
    }

    int srcI=N/2, srcJ=N/2, srcK=N/2;
    const float Amp=1000.f;

    // --- Memoria en GPU o simulada en CPU ---
    float *dU0=hU0, *dU1=hU1, *dVP=hVP, *dW=(float*)hW;
#ifdef __CUDACC__
    cudaCheck(cudaMalloc(&dU0,bytes),"malloc U0");
    cudaCheck(cudaMalloc(&dU1,bytes),"malloc U1");
    cudaCheck(cudaMalloc(&dVP,bytes),"malloc VP");
    cudaCheck(cudaMalloc(&dW,7*sizeof(float)),"malloc W");
    cudaMemcpy(dU0,hU0,bytes,cudaMemcpyHostToDevice);
    cudaMemcpy(dU1,hU1,bytes,cudaMemcpyHostToDevice);
    cudaMemcpy(dVP,hVP,bytes,cudaMemcpyHostToDevice);
    cudaMemcpy(dW,hW,7*sizeof(float),cudaMemcpyHostToDevice);
#endif

    dim3 block(8,8,8);
    dim3 grid((N+block.x-1)/block.x,(N+block.y-1)/block.y,(N+block.z-1)/block.z);

    // --- Bucle temporal ---
    for(int n=0;n<T;n++){
        float t=n*dt-sourceTf;
        float s=Amp*ricker(t,fcR);
#ifdef __CUDACC__
        inject_source_kernel<<<1,1>>>(dU0,dVP,srcI,srcJ,srcK,s,1.0f,N,N);
        stencil_update_kernel<<<grid,block>>>(dU0,dU1,dVP,N,N,N,FATX,FATY,FATZ,dW);
#else
        inject_source_kernel(dU0,dVP,srcI,srcJ,srcK,s,1.0f,N,N);
        stencil_update_kernel(dU0,dU1,dVP,N,N,N,FATX,FATY,FATZ,dW);
#endif
        float* tmp=dU0; dU0=dU1; dU1=tmp;
    }

#ifdef __CUDACC__
    cudaMemcpy(hU1,dU0,bytes,cudaMemcpyDeviceToHost);
#endif

    for(size_t i=0;i<nel;i++){
        float v=fabsf(hU1[i]);
        if(v>=EPSILON1&&v<=EPSILON2) printf("%.5f ",hU1[i]);
    }
    printf("\n");

#ifdef __CUDACC__
    cudaFree(dU0); cudaFree(dU1); cudaFree(dVP); cudaFree(dW);
#endif
    free(hU0); free(hU1); free(hVP);
    return 0;
}
