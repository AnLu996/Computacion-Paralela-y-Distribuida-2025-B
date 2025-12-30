#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc,char**argv){
    MPI_Init(&argc,&argv);
    int r; MPI_Comm_rank(MPI_COMM_WORLD,&r);
    int iters = (argc>=2)? atoi(argv[1]) : 100000;
    int nbytes = (argc>=3)? atoi(argv[2]) : 8;

    char *buf = (char*)malloc(nbytes);
    if (r==0) for(int i=0;i<nbytes;i++) buf[i]=(char)i;

    clock_t c0 = clock();
    if (r==0){
        for (int i=0;i<iters;i++){
            MPI_Send(buf,nbytes,MPI_BYTE,1,0,MPI_COMM_WORLD);
            MPI_Recv(buf,nbytes,MPI_BYTE,1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
    } else if (r==1){
        for (int i=0;i<iters;i++){
            MPI_Recv(buf,nbytes,MPI_BYTE,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(buf,nbytes,MPI_BYTE,0,0,MPI_COMM_WORLD);
        }
    }
    clock_t c1 = clock();

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();
    if (r==0){
        for (int i=0;i<iters;i++){
            MPI_Send(buf,nbytes,MPI_BYTE,1,0,MPI_COMM_WORLD);
            MPI_Recv(buf,nbytes,MPI_BYTE,1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
    } else if (r==1){
        for (int i=0;i<iters;i++){
            MPI_Recv(buf,nbytes,MPI_BYTE,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Send(buf,nbytes,MPI_BYTE,0,0,MPI_COMM_WORLD);
        }
    }
    double t1 = MPI_Wtime();

    if (r==0){
        double clk = (double)(c1 - c0) / CLOCKS_PER_SEC;
        double wtm = t1 - t0;
        printf("iters=%d bytes=%d  clock()=%.6f s, MPI_Wtime=%.6f s\n", iters, nbytes, clk, wtm);
    }
    free(buf);
    MPI_Finalize(); 
    return 0;
}
