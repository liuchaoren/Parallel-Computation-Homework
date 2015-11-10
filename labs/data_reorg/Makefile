default: all

all: matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg matmul_offload matmul_mpi matmul_mpi_openmp matmul_mpi_offload

OPT = -O2 

matmul_serial: matmul_serial.c
	icpc $(OPT) -std=c++11 -o matmul_serial matmul_serial.c -fopenmp

matmul_openmp:
	icpc $(OPT) -std=c++11 -o matmul_openmp matmul_openmp.cpp -fopenmp

matmul_reorg:
	icpc $(OPT) -std=c++11 -o matmul_reorg matmul_reorg.cpp -fopenmp

matmul_openmp_reorg:
	icpc $(OPT) -std=c++11 -o matmul_openmp_reorg matmul_openmp_reorg.cpp -fopenmp

matmul_offload:
	icpc $(OPT) -std=c++11 -o matmul_offload matmul_offload.cpp -fopenmp

matmul_mpi:
	mpiicpc -std=c++11 -o matmul_mpi matmul_mpi.c -fopenmp

matmul_mpi_openmp:
	mpiicpc -std=c++11 -o matmul_mpi_openmp matmul_mpi_openmp.c -fopenmp

matmul_mpi_offload:
	mpiicpc -std=c++11 -o matmul_mpi_offload matmul_mpi_offload.c -fopenmp

clean:
	rm -f matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg matmul_offload matmul_mpi matmul_mpi_openmp matmul_mpi_offload
