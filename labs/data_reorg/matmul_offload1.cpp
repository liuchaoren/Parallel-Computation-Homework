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

#define D 64

// Use linear memory to store all matrices
#pragma offload_attribute(push,target(mic))
double A[D][D][N/D][P/D] __attribute__((aligned(64)));;
double B[D][D][P/D][M/D] __attribute__((aligned(64)));;
double C[D][D][N/D][M/D] __attribute__((aligned(64)));;
#pragma offload_attribute(pop)

// Initialize the matrices (uniform values to make an easier check)
void matrix_init(void) {
// A[N][P] -- Matrix A
  for (int i = 0; i < D; i++) {
    for (int j = 0; j < D; j++) {
      for (int ii = 0; ii < N/D; ii++) {
	for (int jj = 0; jj < P/D; jj++) {
	  A[i][j][ii][jj] = AVAL;
	}
      }
    }
  }

  // B[P][M] -- Matrix B
  for (int i = 0; i < D; i++) {
    for (int j = 0; j < D; j++) {
      for (int ii = 0; ii < P/D; ii++) {
	for (int jj = 0; jj < M/D; jj++) {
	  B[i][j][ii][jj] = BVAL;
	}
      }
    }
  }
  
  // C[N][M] -- result matrix for AB
  for (int i = 0; i < D; i++) {
    for (int j = 0; j < D; j++) {
      for (int ii = 0; ii < N/D; ii++) {
	for (int jj = 0; jj < M/D; jj++) {
	  C[i][j][ii][jj] = 0.0;
	}
      }
    }
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
#pragma omp parallel for collapse(3)
  for (int i = 0; i < D; i++) {
    for (int k = 0; k < D; k++) {
      for (int j = 0; j < D; j++) {
	for (int ii = 0; ii < N/D; ii++) {
	  for (int kk = 0; kk < P/D; kk++) {
	    for (int jj = 0; jj < M/D; jj++) {	  	    
	      C[i][j][ii][jj] += A[i][k][ii][kk] * B[k][j][kk][jj];
	    }
	  }
	}
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

  for (int i = 0; i < D; i++) {
    for (int j = 0; j < D; j++) {
      for (int ii = 0; ii < N/D; ii++) {
	for (int jj = 0; jj < M/D; jj++) {
	  e = C[i][j][ii][jj] - v;
	  ee = e * e;
	}
      }
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
