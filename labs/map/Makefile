all: pass_init pass_mp pass_tbb pass_cilk

default: pass_init pass_mp pass_tbb pass_cilk

pass_mp:

pass_tbb:

pass_cilk:

pass_init: pass_init.c
	icpc -std=c++11 -o pass_init pass_init.c -lcrypto

clean:
	rm *.o pass_init
