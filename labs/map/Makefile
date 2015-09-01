all: pass_serial pass_openmp pass_tbb pass_cilk

default: pass_serial pass_openmp pass_tbb pass_cilk

pass_openmp: pass_openmp.c
	icpc -std=c++11 -o pass_openmp pass_openmp.c -lcrypto -fopenmp

pass_tbb: pass_tbb.cpp
	icpc -std=c++11 -o pass_tbb pass_tbb.cpp -lcrypto -ltbb

pass_cilk: pass_cilk.c
	icpc -std=c++11 -o pass_cilk pass_cilk.c -lcrypto

pass_serial: pass_serial.c
	icpc -std=c++11 -o pass_serial pass_serial.c -lcrypto

clean:
	rm -f *.o pass_serial pass_openmp pass_cilk pass_tbb
