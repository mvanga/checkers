LDFLAGS += -lpthread
CFLAGS = -Wall -Werror -g -rdynamic

SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
#	$(CC) $(CFLAGS)  -o $@ $<

clean:
	rm $(PROGS)
