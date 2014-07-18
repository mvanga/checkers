all: chess test

chess:
	gcc -Wall -fPIC -shared -o chess.so chess.c

test: test.o

clean:
	rm -rf *.o *.so test *.txt
