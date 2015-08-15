prog:=pipeline_serial
objs:=$(patsubst %.c,%.o,$(wildcard *.cpp))

cc=icpc
ccflags= -MMD -debug -Wall
ldflags= -lopencv_core -lopencv_highgui -lopencv_imgproc

%.o: %.c
	$(cc) $(ccflags) -c $<

$(prog): $(objs)
	$(cc) $(ccflags) -o $@ $^ $(ldflags)


.PHONY: clean

clean:
	rm -f *.o *.d counts* *.output *~ $(prog) 
	
-include *.d

