all: streamtest

TARGET = ..
CFLAGS = -O3 -g3 -I../include/

%.o: %.c
	gcc -c $(CFLAGS) -o $@ $^

streamtest: streamtest.o $(TARGET)/symbol.o $(TARGET)/sample.o
	gcc $(CFLAGS) -o $@ $^

clean:
	rm -rf ../*.o
	rm -rf *.o
	- rm streamtest
