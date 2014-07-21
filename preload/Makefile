LDFLAGS+=-lpthread
CFLAGS += -Wall -ggdb -Wextra -rdynamic

all: chess test

chess:
	gcc -Wall -g -fPIC -shared -o chess.so chess.c -ldl

test: test.o

clean:
	rm -rf *.o *.so test *.txt
