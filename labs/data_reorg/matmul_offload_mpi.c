/*
 * Matrix Multiply.
 *
 * This is a simple matrix multiply program which will compute the product
 *
 *                C  = A * B
 *
 * A ,B and C are both square matrix. They are statically allocated and
 * initialized with constant number, so we can focuse on the parallelism.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <tbb/tick_count.h>
#include <mpi.h>

#define ORDER 8192   // the order of the matrix: you can assume power of two
#define AVAL  3.0    // initial value of A
#define BVAL  5.0    // initial value of B
#define TOL   0.001  // tolerance used to check the result

#define N ORDER
#define P ORDER
#define M ORDER

#pragma offload_attribute(push,target(mic))
double A[N][P] __attribute__((aligned(64)));
double B[P][M] __attribute__((aligned(64)));
double C[N][M] __attribute__((aligned(64)));
#pragma offload_attribute(pop)

// Initialize the matrices (uniform values to make an easier check)
void matrix_init(void) {
  int i, j;

  // A[N][P] -- Matrix A
  for (i=0; i<N; i++) {
    for (j=0; j<P; j++) {
      A[i][j] = AVAL;
    }
  }

  // B[P][M] -- Matrix B
  for (i=0; i<P; i++) {
    for (j=0; j<M; j++) {
      B[i][j] = BVAL;
    }
  }

  // C[N][M] -- result matrix for AB
  for (i=0; i<N; i++) {
    for (j=0; j<M; j++) {
      C[i][j] = 0.0;
    }
  }
}

// The actual mulitplication function, totally naive
double matrix_multiply(int fromRowIdx, int toRowIdx) {
  double start, end;


#pragma offload target(mic) in(A,B,C)
  {}
  // timer for the start of the computation
  // If you do any dynamic reorganization, 
  // do it before you start the timer
  // the timer value is captured.
  start = omp_get_wtime(); 

#pragma offload target(mic)
#pragma omp parallel for
  for (int i=fromRowIdx; i<toRowIdx; i++){
    for(int k=0; k<P; k++){
      for (int j=0; j<M; j++){
	C[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  // timer for the end of the computation
  end = omp_get_wtime();

#pragma offload target(mic) out(C)
  {}
  // return the amount of high resolution time spent
  return end - start;
}

// Function to check the result, relies on all values in each initial
// matrix being the same
int check_result(void) {
  int i, j;

  double e  = 0.0;
  double ee = 0.0;
  double v  = AVAL * BVAL * ORDER;

  for (i=0; i<N; i++) {
    for (j=0; j<M; j++) {
      e = C[i][j] - v;
      ee += e * e;
    }
  }

  if (ee > TOL) {
    return 0;
  } else {
    return 1;
  }
}

// main function
int main(int argc, char **argv) {
  // omp_set_num_threads(32);
  int rank, size;

  // Initialize MPI environment
  MPI_Init(&argc,&argv);
  // Get current machine ID
  MPI_Comm_rank(MPI_COMM_WORLD,&rank); 
  // Get total machine number
  MPI_Comm_size(MPI_COMM_WORLD,&size);

  // printf("I am %d out of %d \n", rank, size);

  int correct;
  double run_time, multiply_time;
  double mflops;

  // initialize the matrices with master machine
  if (rank == 0) {
      matrix_init();
      // printf("Finished matrix initialization\n");
  }
  MPI_Barrier(MPI_COMM_WORLD);

  // Send matrix value to different machines according to their machine ID
  if (rank == 0) {
    printf("Start copy data to all machines\n");
    run_time = omp_get_wtime();
  }
  int fromRowIdx = rank * N / size;
  int toRowIdx = (rank+1) * N / size;
  // printf("From row %d to row %d\n", fromRowIdx, toRowIdx-1);
  MPI_Bcast (B, P*M, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (rank == 0)
    MPI_Scatter (A, N*P/size, MPI_DOUBLE, MPI_IN_PLACE, N*P/size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  else
    MPI_Scatter (A, N*P/size, MPI_DOUBLE, A[fromRowIdx], N*P/size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    run_time = omp_get_wtime() - run_time;
    printf("Finish copy data to all machines, communication takes %f sec\n", run_time);
  }

  // multiply the slice and capture the runtime
  multiply_time = matrix_multiply(fromRowIdx, toRowIdx);

  // Gather results from all machines
  if (rank == 0) {
    printf("Start gather data from all machines\n");
    run_time = omp_get_wtime();
  }
  if (rank == 0)
    MPI_Gather (MPI_IN_PLACE, N*M/size, MPI_DOUBLE, C, N*M/size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  else
    MPI_Gather (C[fromRowIdx], N*M/size, MPI_DOUBLE, C, N*M/size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  if (rank == 0) {
    run_time = omp_get_wtime() - run_time;
    printf("Finish gather data from all machines, communication takes %f sec\n", run_time);
  }


  // verify that the result is sensible at master machine
  if (rank == 0)
    {
      correct  = check_result();

      // Compute the number of mega flops
      mflops = (2.0 * N * P * M) / (1000000.0 * run_time);
      printf("Order %d multiplication in %f seconds \n", ORDER, multiply_time);
      printf("Order %d multiplication at %f mflops\n", ORDER, mflops);

      // Display check results
      if (correct) {
	printf("\n Hey, it worked");
      } else {
	printf("\n Errors in multiplication");
      }
      printf("\n all done \n");
    }

  MPI_Finalize();
  return 0;
}
