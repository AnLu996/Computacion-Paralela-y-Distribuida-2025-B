// rtm_cuda.cu
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>

#define def_NPOP_AC 6
#define EPSILON1 0.01f
#define EPSILON2 1000.0f

// -------- Ricker ----------
__host__ __device__
inline float ricker(float t, float fc_half){
    const float pi = 3.14159265358979323846f;
    float alpha = sqrtf(0.5f) * pi * fc_half;
    float aux = (t * alpha) * (t * alpha);
    return (1.0f - 2.0f * aux) * expf(-aux);
}

// -------- Kernels ----------
__global__ void inject_source_kernel(float* U0, const float* VP,
                                     int srcI, int srcJ, int srcK,
                                     float val, float fat,
                                     int NNOI, int NNOJ) {
    // Un solo hilo basta; la inyección es puntual
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        unsigned long idx = (unsigned long)srcK * (unsigned long)NNOI * (unsigned long)NNOJ
                          + (unsigned long)srcJ * (unsigned long)NNOI
                          + (unsigned long)srcI;
        float v = VP[idx];
        U0[idx] += fat * v * v * val;
    }
}

__global__ void stencil_update_kernel(const float* __restrict__ U0,
                                      float* __restrict__ U1,
                                      const float* __restrict__ VP,
                                      int nnoi, int nnoj, int nnok,
                                      float FATX, float FATY, float FATZ,
                                      const float* __restrict__ W)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;
    int k = blockIdx.z * blockDim.z + threadIdx.z;

    const int stride = nnoi * nnoj;

    // Restringimos a la "zona segura" (bordes negativos quedan fuera)
    if (i < def_NPOP_AC || i >= nnoi - def_NPOP_AC) return;
    if (j < def_NPOP_AC || j >= nnoj - def_NPOP_AC) return;
    if (k < def_NPOP_AC || k >= nnok - def_NPOP_AC) return;

    int index = (j * nnoi + i) + k * stride;
    float v = VP[index];
    if (v <= 0.0f) return;

    float v2 = v * v;

    // Eje X (i)
    float sx =
        W[6]*(U0[index - 6] + U0[index + 6]) +
        W[5]*(U0[index - 5] + U0[index + 5]) +
        W[4]*(U0[index - 4] + U0[index + 4]) +
        W[3]*(U0[index - 3] + U0[index + 3]) +
        W[2]*(U0[index - 2] + U0[index + 2]) +
        W[1]*(U0[index - 1] + U0[index + 1]) +
        W[0]*U0[index];

    // Eje Y (j)
    int offy = nnoi;
    float sy =
        W[6]*(U0[index - 6*offy] + U0[index + 6*offy]) +
        W[5]*(U0[index - 5*offy] + U0[index + 5*offy]) +
        W[4]*(U0[index - 4*offy] + U0[index + 4*offy]) +
        W[3]*(U0[index - 3*offy] + U0[index + 3*offy]) +
        W[2]*(U0[index - 2*offy] + U0[index + 2*offy]) +
        W[1]*(U0[index - offy]   + U0[index + offy])   +
        W[0]*U0[index];

    // Eje Z (k)
    int offz = stride;
    float sz =
        W[6]*(U0[index - 6*offz] + U0[index + 6*offz]) +
        W[5]*(U0[index - 5*offz] + U0[index + 5*offz]) +
        W[4]*(U0[index - 4*offz] + U0[index + 4*offz]) +
        W[3]*(U0[index - 3*offz] + U0[index + 3*offz]) +
        W[2]*(U0[index - 2*offz] + U0[index + 2*offz]) +
        W[1]*(U0[index - offz]   + U0[index + offz])   +
        W[0]*U0[index];

    // Fórmula explícita
    float next = 2.0f * U0[index] - U1[index]
               + FATX * v2 * sx
               + FATY * v2 * sy
               + FATZ * v2 * sz;

    U1[index] = next;
}

// -------- Utilidades host ----------
static inline void cudaCheck(cudaError_t e, const char* msg){
    if(e != cudaSuccess){
        fprintf(stderr, "CUDA error %s: %s\n", msg, cudaGetErrorString(e));
        exit(1);
    }
}

int main(){
    // ---------------- I/O ----------------
    int N, T;
    if (scanf("%d %d", &N, &T) != 2){
        fprintf(stderr, "Error: entrada esperada 'N T'\n");
        return 1;
    }

    // --- Parámetros (idénticos al baseline) ---
    const float dx = 10.f, dy = 10.f, dz = 10.f, dt = 0.0005f;
    const float FC  = 45.f;
    const float fcR = 0.5f * FC;
    const float VP_def = 3000.f;
    const float sourceTf = 2.0f * sqrtf(3.14159265358979323846f) / FC;
    const float FATX = (dt*dt)/(dx*dx);
    const float FATY = (dt*dt)/(dy*dy);
    const float FATZ = (dt*dt)/(dz*dz);

    // Coeficientes W (idénticos)
    const float hW[7] = {
        -3.0822809296250264930f,
        +1.8019078703451239918f,
        -0.32734121207503490301f,
        +0.083457210633103019141f,
        -0.020320182331671760958f,
        +0.0038589461566722754295f,
        -0.00042216791567937593928f
    };

    // Dimensiones y tamaño
    const size_t nnoi = (size_t)N;
    const size_t nnoj = (size_t)N;
    const size_t nnok = (size_t)N;
    const size_t nel  = nnoi * nnoj * nnok;
    const size_t bytes = nel * sizeof(float);

    // Host buffers
    float *hU0 = (float*)calloc(nel, sizeof(float));
    float *hU1 = (float*)calloc(nel, sizeof(float));
    float *hVP = (float*)malloc(bytes);

    if(!hU0 || !hU1 || !hVP){
        fprintf(stderr, "Error de memoria host\n");
        return 1;
    }

    // Modelo de VP: constante + “borde negativo” como en baseline
    std::fill(hVP, hVP + nel, VP_def);
    auto setV = [&](int i, int j, int k, float v){
        size_t idx = (size_t)k*nnoi*nnoj + (size_t)j*nnoi + (size_t)i;
        hVP[idx] = v;
    };
    // Borde negativo de ancho def_NPOP_AC en las 6 caras
    for(int k=0;k<N;k++){
        for(int j=0;j<N;j++){
            for(int t=0;t<def_NPOP_AC;t++){
                setV(t, j, k, -VP_def);                     // x-
                setV(N-1-t, j, k, -VP_def);                // x+
            }
        }
    }
    for(int k=0;k<N;k++){
        for(int t=0;t<def_NPOP_AC;t++){
            for(int i=0;i<N;i++){
                setV(i, t, k, -VP_def);                    // y-
                setV(i, N-1-t, k, -VP_def);               // y+
            }
        }
    }
    for(int t=0;t<def_NPOP_AC;t++){
        for(int j=0;j<N;j++){
            for(int i=0;i<N;i++){
                setV(i, j, t, -VP_def);                    // z-
                setV(i, j, N-1-t, -VP_def);               // z+
            }
        }
    }

    // Fuente centrada (como SRC=64/64/64 en baseline; si N<129, la centramos)
    int srcI = (N>129?64:N/2);
    int srcJ = (N>129?64:N/2);
    int srcK = (N>129?64:N/2);

    // Wavelet (amplitud base 1000 como baseline)
    const float Amp = 1000.f;

    // ------ Device memory ------
    float *dU0, *dU1, *dVP, *dW;
    cudaCheck(cudaMalloc(&dU0, bytes), "malloc dU0");
    cudaCheck(cudaMalloc(&dU1, bytes), "malloc dU1");
    cudaCheck(cudaMalloc(&dVP, bytes), "malloc dVP");
    cudaCheck(cudaMalloc(&dW,  7*sizeof(float)), "malloc dW");

    cudaCheck(cudaMemcpy(dU0, hU0, bytes, cudaMemcpyHostToDevice), "cpy U0");
    cudaCheck(cudaMemcpy(dU1, hU1, bytes, cudaMemcpyHostToDevice), "cpy U1");
    cudaCheck(cudaMemcpy(dVP, hVP, bytes, cudaMemcpyHostToDevice), "cpy VP");
    cudaCheck(cudaMemcpy(dW,  hW,  7*sizeof(float), cudaMemcpyHostToDevice), "cpy W");

    dim3 block(8,8,8);
    dim3 grid( (N+block.x-1)/block.x,
               (N+block.y-1)/block.y,
               (N+block.z-1)/block.z );

    // ------ Bucle temporal ------
    for (int n=0; n<T; ++n){
        float t = n * dt - sourceTf;      // mismo corrimiento (TimeDelay=0)
        float s = Amp * ricker(t, fcR);

        inject_source_kernel<<<1,1>>>(dU0, dVP, srcI, srcJ, srcK, s, 1.0f, (int)nnoi, (int)nnoj);
        cudaCheck(cudaGetLastError(), "inject");

        stencil_update_kernel<<<grid, block>>>(dU0, dU1, dVP, (int)nnoi, (int)nnoj, (int)nnok,
                                               FATX, FATY, FATZ, dW);
        cudaCheck(cudaGetLastError(), "stencil");

        // swap
        float* tmp = dU0; dU0 = dU1; dU1 = tmp;
    }

    // Copiamos el último U1 (tras el swap, U0 contiene el más reciente)
    cudaCheck(cudaMemcpy(hU1, dU0, bytes, cudaMemcpyDeviceToHost), "copy back");

    // ------ Salida (mismo criterio/orden) ------
    for (size_t i=0;i<nel;i++){
        float v = fabsf(hU1[i]);
        if (v >= EPSILON1 && v <= EPSILON2){
            // mantener 5 decimales y espacio
            printf("%.5f ", hU1[i]);
        }
    }
    printf("\n");

    // ------ Limpieza ------
    cudaFree(dU0); cudaFree(dU1); cudaFree(dVP); cudaFree(dW);
    free(hU0); free(hU1); free(hVP);
    return 0;
}
