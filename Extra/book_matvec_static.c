// matvec_static.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    const double* A; const double* x; double* y;
    int nrows, ncols, tid, thread_count;
} args_t;

void* worker(void* a) {
    args_t* t = (args_t*)a;
    int n = t->nrows, m = t->ncols, T = t->thread_count, id = t->tid;
    int local_n = (n + T - 1)/T;
    int start = id * local_n;
    int end   = start + local_n; if (end > n) end = n;
    for (int i = start; i < end; ++i) {
        double acc = 0.0;
        const double* row = t->A + (size_t)i*m;
        for (int j = 0; j < m; ++j) acc += row[j] * t->x[j];
        t->y[i] = acc;
    }
    return NULL;
}

int main(int argc, char** argv){
    int n=2000, m=2000, T=8;
    if (argc>=4){ n=atoi(argv[1]); m=atoi(argv[2]); T=atoi(argv[3]); }
    double* A=(double*)malloc((size_t)n*m*sizeof(double));
    double* x=(double*)malloc((size_t)m*sizeof(double));
    double* y=(double*)calloc(n,sizeof(double));
    for (int i=0;i<n*m;++i) A[i]=(i%7)*0.1;
    for (int j=0;j<m;++j)   x[j]=(j%5)*0.2;

    pthread_t* th = (pthread_t*)malloc(T*sizeof(pthread_t));
    args_t* a = (args_t*)malloc(T*sizeof(args_t));
    for (int t=0;t<T;++t){
        a[t]=(args_t){A,x,y,n,m,t,T};
        pthread_create(&th[t], NULL, worker, &a[t]);
    }
    for (int t=0;t<T;++t) pthread_join(th[t], NULL);
    printf("y[0]=%.3f, y[n-1]=%.3f\n", y[0], y[n-1]);
    free(A); free(x); free(y); free(th); free(a);
    return 0;
}
