// file: mpi_butterfly_sum.c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int r, p; MPI_Comm_rank(MPI_COMM_WORLD, &r); MPI_Comm_size(MPI_COMM_WORLD, &p);

    int t = p; while (t > 1 && t % 2 == 0) t /= 2;
    int ok = (t == 1);
    int val = r + 1;

    if (!ok) {
        int sum = 0;
        MPI_Allreduce(&val, &sum, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
        if (r == 0) printf("Sum = %d\n", sum);
        MPI_Finalize();
        return 0;
    }

    for (int d = 1; d < p; d <<= 1) {
        int partner = r ^ d, recv;
        MPI_Sendrecv(&val, 1, MPI_INT, partner, 0, &recv, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        val += recv;
    }

    if (r == 0) printf("Butterfly sum = %d\n", val);
    MPI_Finalize();
    return 0;
}
