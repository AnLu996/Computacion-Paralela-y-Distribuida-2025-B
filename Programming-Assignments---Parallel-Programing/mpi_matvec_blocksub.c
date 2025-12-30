// file: mpi_matvec_blocksub.c
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc,char**argv){
    MPI_Init(&argc,&argv);
    int r,p; MPI_Comm_rank(MPI_COMM_WORLD,&r); MPI_Comm_size(MPI_COMM_WORLD,&p);
    int q = (int)(sqrt((double)p)+0.5);
    if (q*q!=p){ if(r==0) fprintf(stderr,"comm_sz debe ser cuadrado perfecto\n"); MPI_Abort(MPI_COMM_WORLD,1); }
    int n = (argc>=2)? atoi(argv[1]) : 1024;
    if (n%q){ if(r==0) fprintf(stderr,"n debe ser múltiplo de sqrt(p)\n"); MPI_Abort(MPI_COMM_WORLD,1); }

    int dims[2]={q,q}, periods[2]={0,0}, coords[2];
    MPI_Comm grid; MPI_Cart_create(MPI_COMM_WORLD,2,dims,periods,1,&grid);
    MPI_Cart_coords(grid,r,2,coords);
    int row=coords[0], col=coords[1];

    int bs = n/q;
    double *A = malloc((size_t)bs*bs*sizeof(double));
    double *x_block = malloc((size_t)bs*sizeof(double));
    double *x_row = malloc((size_t)n*sizeof(double));
    double *y_block = calloc(bs,sizeof(double));

    for (int i=0;i<bs;i++) for (int j=0;j<bs;j++)
        A[i*bs+j] = ((row*bs+i)==(col*bs+j))?2.0:0.0;
    for (int i=0;i<bs;i++) x_block[i]=1.0;

    MPI_Comm col_comm, row_comm;
    int remain_dims_row[2]={0,1}, remain_dims_col[2]={1,0};
    MPI_Cart_sub(grid, remain_dims_row, &row_comm);
    MPI_Cart_sub(grid, remain_dims_col, &col_comm);

    double *x_col = malloc((size_t)bs*q*sizeof(double));
    MPI_Allgather(x_block, bs, MPI_DOUBLE, x_col, bs, MPI_DOUBLE, col_comm);
    for (int k=0;k<q;k++) for (int i=0;i<bs;i++) x_row[k*bs+i] = x_col[k*bs+i];

    int j0 = col*bs;
    for (int i=0;i<bs;i++){
        double acc=0.0;
        for (int j=0;j<bs;j++) acc += A[i*bs+j] * x_row[j0+j];
        y_block[i] = acc;
    }

    double *y_row = (col==0)? malloc((size_t)bs*sizeof(double)) : NULL;
    MPI_Reduce(y_block, (col==0? y_row: NULL), bs, MPI_DOUBLE, MPI_SUM, 0, row_comm);

    if (row==0 && col==0){
        printf("y[0..4]: ");
        for (int i=0;i<(bs<5?bs:5); i++) printf("%.1f ", y_row[i]);
        puts("");
        free(y_row);
    }
    free(A); free(x_block); free(x_row); free(x_col); free(y_block);
    MPI_Comm_free(&row_comm); MPI_Comm_free(&col_comm); MPI_Comm_free(&grid);
    MPI_Finalize(); return 0;
}
