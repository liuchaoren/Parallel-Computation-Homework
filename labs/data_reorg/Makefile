default: all

all: matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg

OPT = -O2 

matmul_serial: matmul_serial.c
	icpc $(OPT) -std=c++11 -o matmul_serial matmul_serial.c -fopenmp

matmul_openmp:

matmul_reorg:

matmul_openmp_reorg:

clean:
	rm matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg
