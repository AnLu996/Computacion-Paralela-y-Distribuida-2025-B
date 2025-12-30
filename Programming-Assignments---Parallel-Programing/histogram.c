// file: mpi_histogram.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // gethostname

int main(int argc, char** argv) {
    MPI_Init(&argc,&argv);
    int rank, size; 
    char hostname[256];
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    gethostname(hostname, 256);

    int n = 100000, bins = 10;
    if (rank==0) {
        if (argc>1) n = atoi(argv[1]);
        if (argc>2) bins = atoi(argv[2]);
    }
    MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&bins,1,MPI_INT,0,MPI_COMM_WORLD);

    int local_n = n/size;
    double *local = (double*)malloc(local_n*sizeof(double));
    int *local_counts = calloc(bins,sizeof(int));
    int *global_counts = NULL;

    if (rank==0) {
        double *data = (double*)malloc(n*sizeof(double));
        for (int i=0;i<n;i++) data[i] = (double)rand()/RAND_MAX;
        MPI_Scatter(data, local_n, MPI_DOUBLE, local, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        free(data);
    } else {
        MPI_Scatter(NULL, local_n, MPI_DOUBLE, local, local_n, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    for (int i=0;i<local_n;i++) {
        int bin = (int)(local[i] * bins); 
        if (bin==bins) bin = bins-1;
        local_counts[bin]++;
    }

    if (rank==0) global_counts = calloc(bins,sizeof(int));
    MPI_Reduce(local_counts, global_counts, bins, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Muestra en qué nodo se ejecutó cada proceso
    printf("Proceso %d ejecutándose en %s terminó su cómputo.\n", rank, hostname);

    if (rank==0) {
        printf("\nHistograma (%d bins):\n", bins);
        for (int i=0;i<bins;i++)
            printf("Bin %d: %d\n", i, global_counts[i]);
        free(global_counts);
    }

    free(local); free(local_counts);
    MPI_Finalize();
    return 0;
}
