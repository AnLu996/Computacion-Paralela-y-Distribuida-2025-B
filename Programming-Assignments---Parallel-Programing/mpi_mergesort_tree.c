// file: mpi_mergesort_tree.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_int(const void*a,const void*b){int x=*(int*)a,y=*(int*)b; return (x>y)-(x<y);}
static int* merge_arrays(int*a,int na,int*b,int nb){
    int *c = malloc((na+nb)*sizeof(int)),i=0,j=0,k=0;
    while (i<na && j<nb) c[k++] = (a[i]<=b[j])? a[i++]: b[j++];
    while(i<na) c[k++]=a[i++]; while(j<nb) c[k++]=b[j++];
    return c;
}

int main(int argc,char**argv){
    MPI_Init(&argc,&argv);
    int r,p; MPI_Comm_rank(MPI_COMM_WORLD,&r); MPI_Comm_size(MPI_COMM_WORLD,&p);
    long long n = (argc>=2)? atoll(argv[1]) : 300000;
    MPI_Bcast(&n,1,MPI_LONG_LONG,0,MPI_COMM_WORLD);

    long long loc_n = n/p + (r<(n%p)?1:0);
    int *loc = malloc((size_t)loc_n*sizeof(int));
    srand(1234 + r*999);
    for (long long i=0;i<loc_n;i++) loc[i] = rand();
    qsort(loc, loc_n, sizeof(int), cmp_int);

    int step=1;
    while (step < p){
        if ((r % (2*step)) == 0){
            int from = r + step;
            if (from < p){
                long long recv_n; MPI_Recv(&recv_n,1,MPI_LONG_LONG,from,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                int *buf = malloc((size_t)recv_n*sizeof(int));
                MPI_Recv(buf,(int)recv_n,MPI_INT,from,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                int *merged = merge_arrays(loc,(int)loc_n,buf,(int)recv_n);
                free(loc); free(buf);
                loc = merged; loc_n += recv_n;
            }
        } else {
            int to = r - step;
            MPI_Send(&loc_n,1,MPI_LONG_LONG,to,0,MPI_COMM_WORLD);
            MPI_Send(loc,(int)loc_n,MPI_INT,to,1,MPI_COMM_WORLD);
            break;
        }
        step <<= 1;
    }

    if (r==0){
        printf("sorted[0..9]: ");
        for (int i=0;i<(loc_n<10?loc_n:10); i++) printf("%d ", loc[i]);
        puts("");
    }
    free(loc);
    MPI_Finalize(); return 0;
}
