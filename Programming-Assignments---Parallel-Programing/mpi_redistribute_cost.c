// file: mpi_redistribute_cost.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char**argv){
    MPI_Init(&argc,&argv);
    int r,p; MPI_Comm_rank(MPI_COMM_WORLD,&r); MPI_Comm_size(MPI_COMM_WORLD,&p);
    int n = (argc>=2)? atoi(argv[1]) : 1000000;

    int base = n/p, extra = n%p;
    int locN = base + (r<extra?1:0);
    int start = r*base + (r<extra? r : extra);

    int *block = malloc(locN*sizeof(int));
    for (int i=0;i<locN;i++) block[i] = start + i;

    int *sendcnt = calloc(p,sizeof(int)), *sdispl = calloc(p,sizeof(int));
    for (int i=0;i<locN;i++) sendcnt[(start+i)%p]++;
    for (int k=1;k<p;k++) sdispl[k]=sdispl[k-1]+sendcnt[k-1];

    int *sendbuf = malloc(locN*sizeof(int)), *tmp = calloc(p,sizeof(int));
    for (int i=0;i<locN;i++){
        int dest = (start+i)%p;
        sendbuf[sdispl[dest] + tmp[dest]++] = block[i];
    }

    int *recvcnt = calloc(p,sizeof(int)), *rdispl = calloc(p,sizeof(int));
    MPI_Alltoall(sendcnt,1,MPI_INT,recvcnt,1,MPI_INT,MPI_COMM_WORLD);
    for (int k=1;k<p;k++) rdispl[k]=rdispl[k-1]+recvcnt[k-1];
    int cycN = rdispl[p-1]+recvcnt[p-1];
    int *cyclic = malloc(cycN*sizeof(int));

    double t0 = MPI_Wtime();
    MPI_Alltoallv(sendbuf, sendcnt, sdispl, MPI_INT,
                  cyclic, recvcnt, rdispl, MPI_INT, MPI_COMM_WORLD);
    double t1 = MPI_Wtime();
    if (r==0) printf("Block->Cyclic time: %.6f s\n", t1-t0);

    memset(sendcnt,0,p*sizeof(int));
    for (int i=0;i<cycN;i++){
        int g = cyclic[i];
        int owner = (g < (base+1)*extra) ? (g/(base+1)) : ((g-(base+1)*extra)/base + extra);
        sendcnt[owner]++;
    }
    sdispl[0]=0; for (int k=1;k<p;k++) sdispl[k]=sdispl[k-1]+sendcnt[k-1];
    int *sendbuf2 = malloc(cycN*sizeof(int));
    memset(tmp,0,p*sizeof(int));
    for (int i=0;i<cycN;i++){
        int g = cyclic[i];
        int owner = (g < (base+1)*extra) ? (g/(base+1)) : ((g-(base+1)*extra)/base + extra);
        sendbuf2[sdispl[owner] + tmp[owner]++] = g;
    }
    memset(recvcnt,0,p*sizeof(int)); memset(rdispl,0,p*sizeof(int));
    MPI_Alltoall(sendcnt,1,MPI_INT,recvcnt,1,MPI_INT,MPI_COMM_WORLD);
    for (int k=1;k<p;k++) rdispl[k]=rdispl[k-1]+recvcnt[k-1];
    int blkN2 = rdispl[p-1]+recvcnt[p-1];
    int *block2 = malloc(blkN2*sizeof(int));

    double t2 = MPI_Wtime();
    MPI_Alltoallv(sendbuf2, sendcnt, sdispl, MPI_INT,
                  block2, recvcnt, rdispl, MPI_INT, MPI_COMM_WORLD);
    double t3 = MPI_Wtime();
    if (r==0) printf("Cyclic->Block time: %.6f s\n", t3-t2);

    free(block); free(sendcnt); free(sdispl); free(sendbuf); free(tmp);
    free(recvcnt); free(rdispl); free(cyclic); free(sendbuf2); free(block2);
    MPI_Finalize(); return 0;
}
