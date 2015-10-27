all: stencil_serial stencil_openmp stencil_offload stencil_cuda

OPT = -O2

stencil_serial: stencil_serial.c bmploader.o
	icpc $(OPT) -o stencil_serial stencil_serial.c bmploader.o -fopenmp

stencil_openmp: stencil_openmp.c bmploader.o
	icpc $(OPT) -o stencil_openmp stencil_openmp.c bmploader.o -fopenmp

stencil_offload: stencil_offload.c bmploader.o
	icpc $(OPT) -o stencil_offload stencil_offload.c bmploader.o -fopenmp

stencil_cuda: stencil_cuda.cu bmploader.o
	nvcc $(OPT) -o stencil_cuda stencil_cuda.cu bmploader.o -Xcompiler -fopenmp

bmploader.o: bmploader.c
	icpc $(OPT) -c bmploader.c

clean:
	rm -f *.o stencil_serial stencil_cuda stencil_openmp stencil_offload img-new.bmp
