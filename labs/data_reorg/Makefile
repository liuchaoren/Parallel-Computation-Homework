default: matmul_serial

all: matmul_serial matmul_openmp matmul_cilk matmul_tbb

matmul_serial: matmul_serial.c
	icpc -std=c++11 -o matmul_serial matmul_serial.c -fopenmp

matmul_openmp:

matmul_cilk:

matmul_tbb:

clean:
	rm matmul_serial matmul_openmp matmul_cilk matmul_tbb 2>/dev/null
