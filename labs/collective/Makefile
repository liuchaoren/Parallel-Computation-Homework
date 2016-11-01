all: crypto_serial crypto_openmp crypto_tbb crypto_cilk

default: all

crypto_serial: crypto_serial.c key.c
	icpc -std=c++11 -o crypto_serial crypto_serial.c key.c

crypto_openmp: crypto_openmp.cpp key.c
	icpc -std=c++11 -o crypto_openmp crypto_openmp.cpp key.c -fopenmp

crypto_tbb: crypto_tbb.cpp key.c
	icpc -std=c++11 -o crypto_tbb crypto_tbb.cpp key.c -ltbb

crypto_cilk: crypto_cilk.cpp key.c
	icpc -std=c++11 -o crypto_cilk crypto_cilk.cpp key.c

clean:
	rm -f *.o crypto_serial crypto_openmp crypto_tbb crypto_cilk decryptedOut encryptedOut
