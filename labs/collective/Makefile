all: crypto_serial crypto_mp crypto_tbb crypto_cilk

default: all

crypto_mp: 

crypto_tbb:

crypto_cilk:

crypto_serial: serial.c key.c
	icpc -std=c++11 -o crypto_serial serial.c key.c

clean:
	rm -f *.o crypto_serial crypto_mp crypto_tbb crypto_cilk decryptedOut encryptedOut
