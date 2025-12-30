// libro/matvec_static.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

typedef struct {
    const double* A; const double* x; double* y;
    int nrows, ncols, tid, thread_count;
} args_t;

void* worker(void* a){
    args_t* t=(args_t*)a;
    int n=t->nrows, m=t->ncols, T=t->thread_count, id=t->tid;
    int local_n=(n+T-1)/T;
    int start=id*local_n;
    int end=(start+local_n>n)?n:start+local_n;
    for (int i=start;i<end;++i){
        double acc=0.0;
        const double* row=t->A+(size_t)i*m;
        for(int j=0;j<m;++j) acc+=row[j]*t->x[j];
        t->y[i]=acc;
    }
    return NULL;
}

int main(int argc,char** argv){
    int n=4000,m=4000,T=8;
    if(argc>=4){n=atoi(argv[1]);m=atoi(argv[2]);T=atoi(argv[3]);}
    double* A=malloc((size_t)n*m*sizeof(double));
    double* x=malloc(m*sizeof(double));
    double* y=calloc(n,sizeof(double));
    for(int i=0;i<n*m;++i)A[i]=(i%7)*0.1;
    for(int j=0;j<m;++j)x[j]=(j%5)*0.2;
    pthread_t* th=malloc(T*sizeof(pthread_t));
    args_t* a=malloc(T*sizeof(args_t));
    struct timeval t0,t1; gettimeofday(&t0,NULL);
    for(int t=0;t<T;++t){
        a[t]=(args_t){A,x,y,n,m,t,T};
        pthread_create(&th[t],NULL,worker,&a[t]);
    }
    for(int t=0;t<T;++t)pthread_join(th[t],NULL);
    gettimeofday(&t1,NULL);
    double ms=(t1.tv_sec-t0.tv_sec)*1000.0+(t1.tv_usec-t0.tv_usec)/1000.0;
    printf("matvec_static,%dx%d,%d,-,%.3f\n",n,m,T,ms);
    free(A);free(x);free(y);free(th);free(a);
    return 0;
}
