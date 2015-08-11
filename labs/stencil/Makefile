all: stencil_serial stencil_openmp stencil_cilk

stencil_serial: stencil_serial.cpp
	icpc -std=c++11 -o stencil_serial stencil_serial.cpp -Wall -Wextra -lopencv_core -lopencv_highgui -lm

stencil_openmp: 

stencil_cilk: 


clean:
	rm -f *.o stencil_serial stencil_openmp stencil_cilk
	
.PHONY: clean
