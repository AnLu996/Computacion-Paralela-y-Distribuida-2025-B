// file: mpi_pi_montecarlo.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc,&argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long tosses = (argc > 1) ? atoll(argv[1]) : 10000000LL;
    MPI_Bcast(&tosses, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    long long local = tosses / size + (rank < tosses % size);
    long long in_circle = 0;
    unsigned int seed = 1234 + rank;   // semilla distinta por proceso

    for (long long i = 0; i < local; i++) {
        double x = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&seed) / RAND_MAX * 2.0 - 1.0;
        if (x*x + y*y <= 1.0) in_circle++;
    }

    long long global_in_circle = 0;
    MPI_Reduce(&in_circle, &global_in_circle, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double pi = 4.0 * (double)global_in_circle / tosses;
        printf("tosses=%lld  hits=%lld  pi≈%.9f\n", tosses, global_in_circle, pi);
    }

    MPI_Finalize();
    return 0;
}
