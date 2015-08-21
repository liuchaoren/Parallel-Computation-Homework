all: pass_serial pass_openmp pass_tbb pass_cilk

default: pass_serial pass_openmp pass_tbb pass_cilk

pass_mp:

pass_tbb:

pass_cilk:

pass_serial: pass_serial.c
	icpc -std=c++11 -o pass_serial pass_serial.c -lcrypto

clean:
	rm *.o pass_serial
