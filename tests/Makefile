LDFLAGS += -lpthread
CFLAGS = -Wall -Werror

SRCS = $(wildcard *.c)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
#	$(CC) $(CFLAGS)  -o $@ $<

clean:
	rm $(PROGS)
