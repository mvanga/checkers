LDFLAGS+=-lpthread

all: chess test

chess:
	gcc -Wall -fPIC -shared -o chess.so chess.c -ldl

test: test.o

clean:
	rm -rf *.o *.so test *.txt
