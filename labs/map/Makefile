all: pass_serial pass_openmp pass_tbb pass_cilk

default: pass_serial pass_openmp pass_tbb pass_cilk

pass_mp: pass_openmp.cpp
	icpc -Wall -g -std=c++11 -o pass_openmp pass_openmp.cpp -lcrypto -fopenmp -lssl

pass_tbb:

pass_cilk:

pass_serial: pass_serial.c
	icpc -std=c++11 -o pass_serial pass_serial.c -lcrypto

clean:
	rm *.o pass_serial
