#include<mpi.h>
#include<stdio.h>

int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int world_rank; // processor id
    int world_size; // total number of processors

    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size < 2) {
        printf("Program requires at least 2 processors to show message sending");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int number;

    if (world_rank == 0) {
        number = -1;
        MPI_Send(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    } else if (world_rank == 1) {
        MPI_Recv(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process 1 received number %d from process 0\n", number);
    }

    MPI_Finalize();
}