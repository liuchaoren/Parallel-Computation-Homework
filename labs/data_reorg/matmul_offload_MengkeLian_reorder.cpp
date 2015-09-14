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

#define ORDER 4096   // the order of the matrix: you can assume power of two
#define AVAL  3.0    // initial value of A
#define BVAL  5.0    // initial value of B
#define TOL   0.001  // tolerance used to check the result

#define N ORDER
#define P ORDER
#define M ORDER

// Use linear memory to store all matrices
#pragma offload_attribute(push,target(mic))
double A[N*P] __attribute__((aligned(64)));
double B[P*M] __attribute__((aligned(64)));
double C[N*M] __attribute__((aligned(64)));
#pragma offload_attribute(pop)

// Initialize the matrices (uniform values to make an easier check)
void matrix_init(void) {
  // A[N][P] -- Matrix A
#pragma omp parallel for
  for (int i=0; i<N*P; i++) {
    A[i] = AVAL;
  }

  // B[P][M] -- Matrix B
#pragma omp parallel for
  for (int i=0; i<P*M; i++) {
    B[i] = BVAL;
  }

  // C[N][M] -- result matrix for AB
#pragma omp parallel for
  for (int i=0; i<N*M; i++) {
    C[i] = 0.0;
  }
}

// The actual mulitplication function, totally naive
double matrix_multiply(void) {
  double start, end;

  // Copy values of input matrices A,B,C from host to mic
#pragma offload target(mic) in(A,B,C)
  {}

  // timer for the start of the computation
  // If you do any dynamic reorganization, 
  // do it before you start the timer
  // the timer value is captured.
  start = omp_get_wtime(); 

  // Interchange loops for j and k so that innermost loop access consecutive memory addresses
#pragma offload target(mic)
#pragma omp parallel for // adding simd doubles running times 
  for (int i=0; i<N; i++){
    for (int k=0; k<P; k++){
      double temp = A[i*P+k];
      for (int j=0; j<M; j++){
	 C[i*M+j] += temp * B[k*M+j];
      }
    }
  }

  // timer for the end of the computation
  end = omp_get_wtime();

  // Copy back values of output matrix C from mic to host
#pragma offload target(mic) out(C)
  {}

  // return the amount of high resolution time spent
  return end - start;
}

// Function to check the result, relies on all values in each initial
// matrix being the same
int check_result(void) {
  double e  = 0.0;
  double ee = 0.0;
  double v  = AVAL * BVAL * ORDER;

#pragma omp parallel for reduction(+:ee)
  for (int i=0; i<N*M; i++) {
      e = C[i] - v;
      ee += e * e;
  }

  if (ee > TOL) {
    return 0;
  } else {
    return 1;
  }
}

// main function
int main(int argc, char **argv) {
  int correct;
  double run_time;
  double mflops;

  // initialize the matrices
  matrix_init();
  // multiply and capture the runtime
  run_time = matrix_multiply();
  // verify that the result is sensible
  correct  = check_result();

  // Compute the number of mega flops
  mflops = (2.0 * N * P * M) / (1000000.0 * run_time);
  printf("Order %d multiplication in %f seconds \n", ORDER, run_time);
  printf("Order %d multiplication at %f mflops\n", ORDER, mflops);

  // Display check results
  if (correct) {
    printf("\n Hey, it worked");
  } else {
    printf("\n Errors in multiplication");
  }
  printf("\n all done \n");

  return 0;
}
