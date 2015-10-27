all: starhole_serial starhole_openmp starhole_cilk starhole_tbb

starhole_serial: starhole_serial.cpp
	icpc -g -std=c++11 -o starhole_serial starhole_serial.cpp -Wall -Wextra -lopencv_core -lopencv_highgui -lm

starhole_cilk: starhole_cilk.cpp
	icpc -g -std=c++11 -o starhole_cilk starhole_cilk.cpp -Wall -Wextra -lopencv_core -lopencv_highgui -lm -ltbb

starhole_tbb: starhole_tbb.cpp
	icpc -g -std=c++11 -o starhole_tbb starhole_tbb.cpp -Wall -Wextra -lopencv_core -lopencv_highgui -lm -ltbb

starhole_openmp: starhole_openmp.cpp
	icpc -debug -std=c++11 -o starhole_openmp starhole_openmp.cpp -Wall -Wextra -lopencv_core -lopencv_highgui -lm -fopenmp -ltbb

clean:
	rm -f *.o starhole_serial starhole_openmp starhole_cilk starhole_tbb
	
.PHONY: clean
