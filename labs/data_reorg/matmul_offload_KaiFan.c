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

#define BLOCKSIZE 128
#define NUM_WORKER 32

double A[N][P];
double B[P][M];
#pragma offload_attribute (push,target(mic))
double C[N][M] __attribute__((aligned(64)));
double Ar[N/BLOCKSIZE][P/BLOCKSIZE][BLOCKSIZE][BLOCKSIZE] __attribute__((aligned(64)));
double Br[P/BLOCKSIZE][M/BLOCKSIZE][BLOCKSIZE][BLOCKSIZE] __attribute__((aligned(64)));
#pragma offload_attribute (pop)

// Initialize the matrices (uniform values to make an easier check)
void matrix_init(void) {

	// A[N][P] -- Matrix A
#pragma omp parallel for
	for (int i=0; i<N; i++) {
		for (int j=0; j<P; j++) {
			A[i][j] = AVAL;
		}
	}

	// B[P][M] -- Matrix B
#pragma omp parallel for
	for (int i=0; i<P; i++) {
		for (int j=0; j<M; j++) {
			B[i][j] = BVAL;
		}
	}

	// C[N][M] -- result matrix for AB
#pragma omp parallel for
	for (int i=0; i<N; i++) {
		for (int j=0; j<M; j++) {
			C[i][j] = 0.0;
		}
	}
}

void matrix4d_init(void) {
#pragma omp parallel for
  for (int i = 0; i < N; i++) {
    for (int k = 0; k < P; k++) {
      Ar[i/BLOCKSIZE][k/BLOCKSIZE][i%BLOCKSIZE][k%BLOCKSIZE] = A[i][k];
    }
  }

#pragma omp parallel for
  for (int k = 0; k < P; k++) {
    for (int j = 0; j < M; j++) {
      Br[k/BLOCKSIZE][j/BLOCKSIZE][k%BLOCKSIZE][j%BLOCKSIZE] = B[k][j];
    }
  }

}

void __attribute__((target(mic))) block_multiply(int ii, int jj, int kk, double AB[BLOCKSIZE][BLOCKSIZE], double BB[BLOCKSIZE][BLOCKSIZE], double C[N][M]) {
  //  #pragma omp parallel for
  for (int i = 0; i < BLOCKSIZE; i++) {
    for (int j = 0; j < BLOCKSIZE; j++) {
      for (int k = 0; k < BLOCKSIZE; k++) {
	C[ii * BLOCKSIZE + i][jj * BLOCKSIZE + j] += AB[i][k] * BB[k][j];
      }
    }
  } 
}

// The actual mulitplication function, totally naive
double matrix_multiply(void) {
  double start, end;

  // timer for the start of the computation
  // If you do any dynamic reorganization, 
  // do it before you start the timer
  // the timer value is captured.

#pragma offload target(mic) in(C,Ar,Br)
  {}
  // start = omp_get_wtime();  
  start = omp_get_wtime(); 
  
#pragma offload target(mic)
#pragma omp parallel for collapse(2)
  for (int ii = 0; ii < N/BLOCKSIZE; ii++){
    for (int jj = 0; jj < M/BLOCKSIZE; jj++){
      for (int kk = 0; kk < P/BLOCKSIZE; kk++){
       	block_multiply(ii, jj, kk, Ar[ii][kk], Br[kk][jj], C);
	//	for (int i = 0; i < BLOCKSIZE; i++) {
        // for (int j = 0; j < BLOCKSIZE; j++) {
	//  for (int k = 0; k < BLOCKSIZE; k++) {
	//      C[ii*BLOCKSIZE+i][jj*BLOCKSIZE+j] += Ar[ii][kk][i][k] * Br[kk][jj][k][j];
	//    }
	//  }
	//} 
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
	int correct;
	double run_time;
	double mflops;

	omp_set_num_threads(NUM_WORKER);
	// initialize the matrices
	matrix_init();
	matrix4d_init();
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
