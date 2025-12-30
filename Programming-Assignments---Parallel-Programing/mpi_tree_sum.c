// file: mpi_tree_sum.c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int r, p; MPI_Comm_rank(MPI_COMM_WORLD, &r); MPI_Comm_size(MPI_COMM_WORLD, &p);

    int x = r + 1;
    int pow2 = 1; while (pow2 * 2 <= p) pow2 *= 2;

    if (r >= pow2) {
        int partner = r - pow2;
        MPI_Send(&x, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
    } else if (r + pow2 < p) {
        int partner = r + pow2, tmp;
        MPI_Recv(&tmp, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        x += tmp;
    }

    for (int step = 1; step < pow2; step <<= 1) {
        if (r < pow2 && r % (2 * step) == 0) {
            int from = r + step, tmp;
            MPI_Recv(&tmp, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            x += tmp;
        } else if (r < pow2 && r % (2 * step) == step) {
            int to = r - step;
            MPI_Send(&x, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
            break;
        }
    }

    if (r == 0) printf("Tree sum = %d\n", x);
    MPI_Finalize();
    return 0;
}
