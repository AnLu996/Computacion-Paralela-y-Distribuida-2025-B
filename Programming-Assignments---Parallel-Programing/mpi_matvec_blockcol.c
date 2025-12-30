// file: mpi_matvec_blockcol.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int r, p; MPI_Comm_rank(MPI_COMM_WORLD, &r); MPI_Comm_size(MPI_COMM_WORLD, &p);

    int n = (argc >= 2) ? atoi(argv[1]) : 1024;
    if (n % p) { if (r == 0) fprintf(stderr, "n debe ser divisible por p\n"); MPI_Abort(MPI_COMM_WORLD, 1); }

    int cols = n / p;
    double *A_local = malloc((size_t)n * cols * sizeof(double));
    double *x_local = malloc((size_t)cols * sizeof(double));
    double *x = malloc((size_t)n * sizeof(double));
    double *y_local = calloc(n, sizeof(double));
    double *y = (r == 0) ? malloc((size_t)n * sizeof(double)) : NULL;

    if (r == 0) {
        double *A = malloc((size_t)n * n * sizeof(double));
        double *x_full = malloc((size_t)n * sizeof(double));
        for (int i = 0; i < n; i++) {
            x_full[i] = 1.0;
            for (int j = 0; j < n; j++) A[i * n + j] = (i == j) ? 2.0 : 0.0;
        }
        for (int dest = 0; dest < p; dest++) {
            int j0 = dest * cols;
            if (dest == 0) {
                for (int j = 0; j < cols; j++)
                    for (int i = 0; i < n; i++)
                        A_local[i * cols + j] = A[i * n + (j0 + j)];
                memcpy(x_local, x_full + j0, cols * sizeof(double));
            } else {
                double *buf = malloc((size_t)n * cols * sizeof(double));
                for (int j = 0; j < cols; j++)
                    for (int i = 0; i < n; i++)
                        buf[i * cols + j] = A[i * n + (j0 + j)];
                MPI_Send(buf, n * cols, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);
                MPI_Send(x_full + j0, cols, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
                free(buf);
            }
        }
        free(A); memcpy(x, x_full, n * sizeof(double)); free(x_full);
    } else {
        MPI_Recv(A_local, n * cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(x_local, cols, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Allgather(x_local, cols, MPI_DOUBLE, x, cols, MPI_DOUBLE, MPI_COMM_WORLD);

    for (int i = 0; i < n; i++) {
        double acc = 0.0;
        for (int j = 0; j < cols; j++) acc += A_local[i * cols + j] * x[r * cols + j];
        y_local[i] = acc;
    }

    MPI_Reduce(y_local, (r == 0 ? y : NULL), n, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    if (r == 0) {
        printf("y[0..4]: ");
        for (int i = 0; i < (n < 5 ? n : 5); i++) printf("%.1f ", y[i]);
        puts("");
        free(y);
    }
    free(A_local); free(x_local); free(x); free(y_local);
    MPI_Finalize();
    return 0;
}
