all: crypto_serial crypto_openmp crypto_tbb crypto_cilk

default: all

crypto_openmp:

crypto_tbb:

crypto_cilk:

crypto_serial: crypto_serial.c key.c
	icpc -std=c++11 -o crypto_serial crypto_serial.c key.c

clean:
	rm -f *.o crypto_serial crypto_openmp crypto_tbb crypto_cilk decryptedOut encryptedOut
