all: hello_serial hello_openmp hello_cilk hello_tbb

hello_serial: hello_serial.cpp
	icpc -std=c++11 -o hello_serial hello_serial.cpp 

hello_openmp: 

hello_cilk: 

hello_tbb: 

clean:
	rm -f *.o hello_serial hello_openmp hello_cilk hello_tbb
	
.PHONY: clean
