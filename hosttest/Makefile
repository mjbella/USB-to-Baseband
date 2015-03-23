all: streamtest

TARGET = ..
CFLAGS = -O3 -g3 -I../include/

CONVOLUTION_CFLAGS := $(CFLAGS) -lm

LIBOBJS := $(TARGET)/convolution.o $(TARGET)/symbol.o $(TARGET)/sample.o

%.o: %.c
	gcc -c $(CFLAGS) -o $@ $^

# Supposed to be PIC-o, like a Position-Independent-Code object file
%.pico: %.c
	gcc -c $(CFLAGS) -fpic -o $@ $^

streamtest: streamtest.o $(LIBOBJS)
	gcc $(CFLAGS) -o $@ $^

streamtest.so: streamtest.pico $(LIBOBJS:.o=.pico)
	gcc -shared -o $@ $^

convolutiontest: convolutiontest.o $(LIBOBJS)
	gcc $(CONVOLUTION_CFLAGS) -o $@ $^

plotpng: plotdata
	./pngplot.sh filter_test.png raw filtered

plotdata: streamtest
	./streamtest 4>raw 5>filtered

clean:
	rm -rf ../*.o
	rm -rf *.o
	rm -rf *.pico
	- rm -rf *.png
	- rm -rf raw
	- rm -rf filtered
	- rm streamtest
	- rm streamtest.so
	- rm convolutiontest
