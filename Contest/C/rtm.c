#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef _WIN32
#include <malloc.h>
#else
#define memalign(alignment, size) ({ void *p; posix_memalign(&p, alignment, size); p; })
#endif

#define def_NPOP_AC 6
#define EPSILON1 0.01
#define EPSILON2 1000

typedef struct
{
    float* U0;
    float* U1;
} stParam_WaveField_AC;

typedef struct
{
    int VERBOSE;
    int type_EQ;
    int nnoi_global;
    int nnoj_global;
    int nnok_global;
    int nTime;
    float dx;
    float dy;
    float dz;
    float dt;
    int PRINT_SNAP;
    int INT_SNAPSHOT;
    float FC;
    float VP_def;
    float RHO_def;
    int NPOP;
    int nborda;
    int NNOI;
    int NNOJ;
    int NNOK;
    float sourceTf;
    float TotalMemAloc_CPU;
} stParam_MDF;

typedef struct
{
    float* WVLT;
    int n1;
} stParam_SrcWavelet;

typedef struct
{
    int* cI_grid;
    int* cJ_grid;
    int* cK_grid;
} stParam_SrcRcv3D;

int init_cte3D(stParam_MDF* pMDF, float* FATMDFX, float* FATMDFY, float* FATMDFZ, float* W, float* C)
{
    *FATMDFX = ((pMDF->dt * pMDF->dt) / (pMDF->dx * pMDF->dx));
    *FATMDFY = ((pMDF->dt * pMDF->dt) / (pMDF->dy * pMDF->dy));
    *FATMDFZ = ((pMDF->dt * pMDF->dt) / (pMDF->dz * pMDF->dz));

    W[0] = -3.0822809296250264930;
    W[1] = +1.8019078703451239918;
    W[2] = -0.32734121207503490301;
    W[3] = +0.83457210633103019141e-1;
    W[4] = -0.20320182331671760958e-1;
    W[5] = +0.38589461566722754295e-2;
    W[6] = -0.42216791567937593928e-3;

    C[0] = +0.76775350395009123508;
    C[1] = -0.16481185505302090350;
    C[2] = +0.20648054998780843275e-1;

    return 0;
}

void kernel_CPU_srcVet_3D(float* cpu_U1, float* cpu_V, int ind,
    int* ind_SrcI, int* ind_SrcJ, int* ind_SrcK,
    float val, float fat, int NNOI, int NNOJ)
{

    unsigned long int i = ind_SrcK[ind] * NNOI * NNOJ + ind_SrcJ[ind] * NNOI + ind_SrcI[ind];
    float v = cpu_V[i];

    cpu_U1[i] += fat * v * v * val;
}

void kernel_CPU_06_mod_3DRhoCte(float* gU0, float* gU1, float* gVorg,
    int nnoi, int nnoj, int k0, int k1,
    float FATMDFX, float FATMDFY, float FATMDFZ, float* W)
{

    int index_X, index_Y;
    int stride = nnoi * nnoj;
    int index, k;


    for (index_X = 0; index_X < nnoi; index_X++)
        for (index_Y = 0; index_Y < nnoj; index_Y++) {
            for (k = 0; k < k1 - k0; k++) {

                index = (index_Y * nnoi + index_X) + (k0 + k) * stride;

                if (gVorg[index] > 0.0f) {
                    gU1[index] = 2.0f * gU0[index] - gU1[index] + FATMDFX * gVorg[index] * gVorg[index] * (+W[6] * (gU0[index - 6] + gU0[index + 6]) + W[5] * (gU0[index - 5] + gU0[index + 5]) + W[4] * (gU0[index - 4] + gU0[index + 4]) + W[3] * (gU0[index - 3] + gU0[index + 3]) + W[2] * (gU0[index - 2] + gU0[index + 2]) + W[1] * (gU0[index - 1] + gU0[index + 1]) + W[0] * gU0[index]) + FATMDFY * gVorg[index] * gVorg[index] * (+W[6] * (gU0[index - 6 * nnoi] + gU0[index + 6 * nnoi]) + W[5] * (gU0[index - 5 * nnoi] + gU0[index + 5 * nnoi]) + W[4] * (gU0[index - 4 * nnoi] + gU0[index + 4 * nnoi]) + W[3] * (gU0[index - 3 * nnoi] + gU0[index + 3 * nnoi]) + W[2] * (gU0[index - 2 * nnoi] + gU0[index + 2 * nnoi]) + W[1] * (gU0[index - nnoi] + gU0[index + nnoi]) + W[0] * gU0[index]) + FATMDFZ * gVorg[index] * gVorg[index] * (+W[6] * (gU0[index + 6 * stride] + gU0[index - 6 * stride]) + W[5] * (gU0[index + 5 * stride] + gU0[index - 5 * stride]) + W[4] * (gU0[index + 4 * stride] + gU0[index - 4 * stride]) + W[3] * (gU0[index + 3 * stride] + gU0[index - 3 * stride]) + W[2] * (gU0[index + 2 * stride] + gU0[index - 2 * stride]) + W[1] * (gU0[index + stride] + gU0[index - stride]) + W[0] * gU0[index]);
                }
            }
        }
}

void exec_mod_06_ACRhoCte3D_CPU(stParam_WaveField_AC* cpu_stAC_U,
    float* c_VP0, int nnoi, int nnoj, int nnok,
    float FATMDFX, float FATMDFY, float FATMDFZ, float* W)
{

    static int k0, k1;

    k0 = 1 * def_NPOP_AC;
    k1 = nnok - 1 * def_NPOP_AC;

    kernel_CPU_06_mod_3DRhoCte(cpu_stAC_U->U0, cpu_stAC_U->U1,
        c_VP0, nnoi, nnoj, k0, k1, FATMDFX, FATMDFY, FATMDFZ, W);
}

int forward_wavefield_AC_3D(stParam_MDF* pMDF, stParam_SrcWavelet* st_SrcWavelet,
    stParam_SrcRcv3D* st_Src, stParam_WaveField_AC* cpu_stAC_U,
    float* c_VP0)
{

    int n;
    size_t SizeT;
    float* pCPU = NULL;
    float FAT_AMP;
    float W[7], C[3];
    float FATMDFX, FATMDFY, FATMDFZ;

    n = init_cte3D(pMDF, &FATMDFX, &FATMDFY, &FATMDFZ, W, C);

    SizeT = sizeof(float);
    SizeT *= pMDF->NNOI;
    SizeT *= pMDF->NNOJ;
    SizeT *= pMDF->NNOK;

    memset(cpu_stAC_U->U0, 0, SizeT);
    memset(cpu_stAC_U->U1, 0, SizeT);

    n = 0;
    while (n < pMDF->nTime) {
        FAT_AMP = 1.0f;
        if (n < st_SrcWavelet->n1 - 1) {
            kernel_CPU_srcVet_3D(cpu_stAC_U->U0, c_VP0, 0, st_Src->cI_grid,
                st_Src->cJ_grid, st_Src->cK_grid, st_SrcWavelet->WVLT[n],
                FAT_AMP, pMDF->NNOI, pMDF->NNOJ);
        }

        exec_mod_06_ACRhoCte3D_CPU(cpu_stAC_U, c_VP0,
            pMDF->NNOI, pMDF->NNOJ, pMDF->NNOK, FATMDFX, FATMDFY, FATMDFZ, W);

        pCPU = cpu_stAC_U->U0;
        cpu_stAC_U->U0 = cpu_stAC_U->U1;
        cpu_stAC_U->U1 = pCPU;

        n++;
    }

    return 0;
}

float Source_Ricker(float t, float fc)
{
    static float r, alpha, aux;
    static float pi;

    pi = 4.0f * atan(1.0f);
    alpha = sqrtf(.5) * pi * fc;
    aux = (t * alpha) * (t * alpha);
    r = (1.0 - 2.0 * aux) * expf(-aux);

    return r;
}

int find_arg_int(int argc, char** argv, char* name, int* val, int opc)
{
    int i = 0;
    int log = 0;
    char* teste = NULL;
    int len;

    strcat(name, "=");

    while (i < argc && log == 0) {
        teste = strstr(argv[i], name);
        if (teste != NULL) {
            len = strlen(name);
            val[0] = atoi(teste + len);
            log = 1;
            return 0;
        }
        ++i;
    }

    if (opc)
        val[0] = 0;
    return 2;
}

int find_arg_float(int argc, char** argv, char* name, float* val, int opc)
{
    int i = 0;
    int log = 0;
    char* teste = NULL;
    int len;

    strcat(name, "=");

    while (i < argc && log == 0) {
        teste = strstr(argv[i], name);
        if (teste != NULL) {
            len = strlen(name);
            val[0] = atof(teste + len);
            log = 1;
            return 0;
        }
        ++i;
    }

    if (opc) {
        val[0] = 0.0;
        return 2;
    }

    return 1;
}

int init_Param_MDF(stParam_MDF* pMDF, int argc, char** argv)
{
    int ival;
    float fval, pi;
    int flag;
    char* NAME;

    pi = 4.0f * atan(1.0f);

    NAME = (char*)malloc(128 * sizeof(char));

    sprintf(NAME, "VERBOSE");
    flag = find_arg_int(argc, argv, NAME, &ival, 1);
    if (flag == 0)
        pMDF->VERBOSE = ival;
    else
        pMDF->VERBOSE = 0;

    sprintf(NAME, "TIPO_EQUACAO");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    if (flag)
        return 1;

    pMDF->type_EQ = ival;

    if (pMDF->type_EQ != 0) {
        return 1;
    }

    sprintf(NAME, "N1_GLOBAL");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    if (flag)
        return 1;

    pMDF->nnok_global = ival;

    sprintf(NAME, "N2_GLOBAL");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    if (flag)
        return 1;

    pMDF->nnoi_global = ival;

    sprintf(NAME, "N3_GLOBAL");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    if (flag)
        return 1;

    pMDF->nnoj_global = ival;

    sprintf(NAME, "D1");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;

    pMDF->dz = fval;

    sprintf(NAME, "D2");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;

    pMDF->dx = fval;

    sprintf(NAME, "D3");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;

    pMDF->dy = fval;

    sprintf(NAME, "DT");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;

    pMDF->dt = fval;

    sprintf(NAME, "NTSTEP");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    if (flag)
        return 1;

    pMDF->nTime = ival;

    sprintf(NAME, "FC");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;

    pMDF->FC = fval;

    sprintf(NAME, "VP_DEF");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;
    pMDF->VP_def = fval;

    sprintf(NAME, "RHO_DEF");
    flag = find_arg_float(argc, argv, NAME, &fval, 0);
    if (flag)
        return 1;
    pMDF->RHO_def = fval;

    sprintf(NAME, "INT_SNAPSHOT");
    flag = find_arg_int(argc, argv, NAME, &ival, 0);
    pMDF->INT_SNAPSHOT = ival;

    pMDF->PRINT_SNAP = 1;

    pMDF->sourceTf = 2.0f * sqrtf(pi) / pMDF->FC;

    pMDF->nborda = 0;

    pMDF->NPOP = def_NPOP_AC;

    pMDF->NNOK = pMDF->nnok_global + 2 * pMDF->nborda;

    pMDF->NNOI = pMDF->nnoi_global + 2 * pMDF->nborda;

    pMDF->NNOJ = pMDF->nnoj_global + 2 * pMDF->nborda;

    pMDF->TotalMemAloc_CPU = 0;

    free(NAME);
    return 0;
}

void* alloc1(size_t n1, size_t type)
{
    void* p = memalign(32, n1 * type);

    memset((void*)p, '\0', n1 * type);

    return p;
}

void cria_BordaNeg_NPOP_3D(float* V, int NNOI, int NNOJ, int NNOK, int NPOP, float v)
{
    long int i, j, k;
    long int II, JJ, KK;

    for (k = 0; k < NNOK; k++) {
        KK = k * NNOI * NNOJ;
        for (j = 0; j < NNOJ; j++) {
            JJ = j * NNOI + KK;
            for (i = 0; i < NPOP; i++) {
                II = JJ + i;
                V[II] = v;

                II = JJ + NNOI - i - 1;
                V[II] = v;
            }
        }
    }

    for (k = 0; k < NNOK; k++) {
        KK = k * NNOI * NNOJ;
        for (j = 0; j < NPOP; j++) {
            JJ = j * NNOI + KK;
            for (i = 0; i < NNOI; i++) {
                II = JJ + i;
                V[II] = v;
            }

            JJ = (NNOJ - j - 1) * NNOI + KK;
            for (i = 0; i < NNOI; i++) {
                II = JJ + i;
                V[II] = v;
            }
        }
    }

    for (k = 0; k < NPOP; k++) {
        KK = k * NNOI * NNOJ;
        for (j = 0; j < NNOJ; j++) {
            JJ = j * NNOI + KK;
            for (i = 0; i < NNOI; i++) {
                II = JJ + i;
                V[II] = v;
            }
        }
        KK = (NNOK - k - 1) * NNOI * NNOJ;
        for (j = 0; j < NNOJ; j++) {
            JJ = j * NNOI + KK;
            for (i = 0; i < NNOI; i++) {
                II = JJ + i;
                V[II] = v;
            }
        }
    }
}

int def_prop_models(stParam_MDF* pMDF, float* c_VP0)
{
    long int i, nel;
    float v, vp;

    nel = pMDF->nnoi_global;
    nel *= pMDF->nnoj_global;
    nel *= pMDF->nnok_global;

    vp = pMDF->VP_def;
    for (i = 0; i < nel; ++i)
        c_VP0[i] = vp;

    v = -vp;
    cria_BordaNeg_NPOP_3D(c_VP0, pMDF->nnoi_global, pMDF->nnoj_global, pMDF->nnok_global, pMDF->NPOP, v);
    return 0;
}

void def_Src_wavelet(stParam_MDF* pMDF, stParam_SrcWavelet* st_Src)
{
    size_t SizeT;
    int i;
    float FC, TimeDelay, Amp;
    float fcR, sourceTf, t;

    FC = pMDF->FC;
    TimeDelay = 0.0f;
    Amp = 1000.0f;

    fcR = 0.5f * FC;
    sourceTf = pMDF->sourceTf;

    SizeT = pMDF->nTime * sizeof(float);
    memset(&st_Src->WVLT[0], 0, SizeT);

    for (i = 0; i < pMDF->nTime; i++) {
        t = i * pMDF->dt - sourceTf - TimeDelay;
        st_Src->WVLT[i] = Amp * Source_Ricker(t, fcR);
    }
}

int alocMem1D_CPU_i(int** cpu, size_t n1, stParam_MDF* pMDF)
{
    int* p = NULL;
    p = (int*)alloc1(n1, sizeof(int));

    if (p == NULL)
        return 1;

    *cpu = p;
    pMDF->TotalMemAloc_CPU += (1.0f * n1 * sizeof(int));

    return 0;
}

int aloc_SrcRcv(const int nel, stParam_SrcRcv3D* pSrcRcv, stParam_MDF* pMDF)
{
    int flag, i;

    i = nel;
    flag = 0;

    flag += alocMem1D_CPU_i(&pSrcRcv->cI_grid, i, pMDF);
    flag += alocMem1D_CPU_i(&pSrcRcv->cJ_grid, i, pMDF);
    flag += alocMem1D_CPU_i(&pSrcRcv->cK_grid, i, pMDF);

    return flag;
}

int read_SrcRcv(stParam_SrcRcv3D* st_Src, int argc, char** argv)
{
    int ival, opc, flag;
    char* NAME;
    char* vNAME;

    NAME = (char*)malloc(128 * sizeof(char));
    vNAME = (char*)malloc(256 * sizeof(char));

    sprintf(NAME, "SRC1");
    opc = 0;
    flag = find_arg_int(argc, argv, NAME, &ival, opc);
    if (flag)
        return 1;

    st_Src->cI_grid[0] = ival;

    sprintf(NAME, "SRC2");
    opc = 0;
    flag = find_arg_int(argc, argv, NAME, &ival, opc);
    if (flag)
        return 1;

    st_Src->cJ_grid[0] = ival;

    sprintf(NAME, "SRC3");
    opc = 0;
    flag = find_arg_int(argc, argv, NAME, &ival, opc);
    if (flag)
        return 1;

    st_Src->cK_grid[0] = ival;

    free(vNAME);
    free(NAME);

    return 0;
}

int alocMem1D_CPU_f(float** cpu, size_t n1, stParam_MDF* pMDF)
{
    float* p = NULL;
    p = (float*)alloc1(n1, sizeof(float));

    if (p == NULL)
        return 1;

    *cpu = p;
    pMDF->TotalMemAloc_CPU += (1.0f * n1 * sizeof(float));

    return 0;
}

void EXEC_TASKS_WORKERS_3D(stParam_MDF* pMDF, stParam_SrcWavelet* st_SrcWavelet,
    stParam_SrcRcv3D* st_Src, float* c_VP0,
    stParam_WaveField_AC* cpu_stAC_U)
{

    forward_wavefield_AC_3D(pMDF, st_SrcWavelet, st_Src, cpu_stAC_U, c_VP0);

}

int main(int _argc, char** _argv)
{
    int input_n, input_t;

    int argc=1;
    char** argv = (char**)malloc(20 * sizeof(char*));
    for(int i=0; i<20; i++){
      argv[i] = (char*)malloc(64 * sizeof(char));
    }

    scanf("%d %d", &input_n, &input_t);
    sprintf(argv[0], "%s", _argv[0]);
    sprintf(argv[argc++], "%d", input_n);
    sprintf(argv[argc++], "%d", input_t);

    int flag;
    size_t i, Nel2D;
    size_t SizeNel;
    size_t SizeT;

    stParam_MDF pMDF;
    stParam_SrcWavelet st_SrcWavelet;
    stParam_SrcRcv3D st_Src;

    float* c_VP0 = NULL;
    float* c_RHO = NULL;

    stParam_WaveField_AC cpu_stAC_U;

    argv[argc++] = "TIPO_EQUACAO=0";
    argv[argc++] = "D1=10";
    argv[argc++] = "D2=10";
    argv[argc++] = "D3=10";
    argv[argc++] = "DT=0.0005";
    argv[argc++] = "VP_DEF=3000";
    argv[argc++] = "RHO_DEF=1.0";
    argv[argc++] = "VERBOSE=5";
    argv[argc++] = "FC=45";
    argv[argc++] = "INT_SNAPSHOT=1000";
    argv[argc++] = "SRC1=64";
    argv[argc++] = "SRC2=64";
    argv[argc++] = "SRC3=64";
    sprintf(argv[argc++], "N1_GLOBAL=%s", argv[1]);
    sprintf(argv[argc++], "N2_GLOBAL=%s", argv[1]);
    sprintf(argv[argc++], "N3_GLOBAL=%s", argv[1]);
    sprintf(argv[argc++], "NTSTEP=%s", argv[2]);

    flag = init_Param_MDF(&pMDF, argc, argv);
    if (flag)
        exit(EXIT_FAILURE);

    Nel2D = pMDF.nnoi_global * pMDF.nnoj_global;
    SizeNel = Nel2D * pMDF.nnok_global;

    flag = alocMem1D_CPU_f((float**)&c_VP0, SizeNel, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    flag = alocMem1D_CPU_f((float**)&c_RHO, SizeNel, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    flag = def_prop_models(&pMDF, c_VP0);
    if (flag)
        exit(EXIT_FAILURE);

    flag = alocMem1D_CPU_f(&st_SrcWavelet.WVLT, pMDF.nTime, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    st_SrcWavelet.n1 = pMDF.nTime;

    def_Src_wavelet(&pMDF, &st_SrcWavelet);

    flag = aloc_SrcRcv(1, &st_Src, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    flag = read_SrcRcv(&st_Src, argc, argv);
    if (flag)
        exit(EXIT_FAILURE);

    SizeT = sizeof(float);
    SizeT *= pMDF.NNOI;
    SizeT *= pMDF.NNOJ;
    SizeT *= pMDF.NNOK;

    flag = alocMem1D_CPU_f(&cpu_stAC_U.U0, SizeT, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    flag = alocMem1D_CPU_f(&cpu_stAC_U.U1, SizeT, &pMDF);
    if (flag)
        exit(EXIT_FAILURE);

    EXEC_TASKS_WORKERS_3D(&pMDF, &st_SrcWavelet, &st_Src, c_VP0, &cpu_stAC_U);

    for (i = 0; i < SizeT; i++)
        if(fabs(cpu_stAC_U.U1[i]) >= EPSILON1 && fabs(cpu_stAC_U.U1[i]) <= EPSILON2)
            printf("%.5lf ",cpu_stAC_U.U1[i]);
    printf("\n");

    return 0;
}
