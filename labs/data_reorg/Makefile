default: all

all: matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg

OPT = -O2 

matmul_serial: matmul_serial.c
	icpc $(OPT) -std=c++11 -o matmul_serial matmul_serial.c -fopenmp

matmul_openmp:
	# icpc $(OPT) -std=c++11 -o matmul_openmp matmul_openmp.cpp -fopenmp -vec-report=5
	icpc $(OPT) -std=c++11 -o matmul_openmp matmul_openmp.cpp -fopenmp

matmul_reorg:
	icpc $(OPT) -std=c++11 -o matmul_reorg matmul_reorg1.cpp -fopenmp

matmul_openmp_reorg:
	icpc $(OPT) -std=c++11 -o matmul_openmp_reorg matmul_openmp_reorg.cpp -fopenmp

matmul_offload:
	icpc $(OPT) -std=c++11 -o matmul_offload matmul_offload1.cpp -fopenmp


clean:
	rm -f matmul_serial matmul_reorg matmul_openmp matmul_openmp_reorg matmul_offload  matmul_openmp.optrpt  matmul_reorg.optrpt matmul_openmp_reorg.optrpt
