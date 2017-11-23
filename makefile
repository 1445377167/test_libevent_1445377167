targets = $(patsubst %.c, %, $(wildcard *.c))

CC = gcc
CFLAGS = -Wall -g -levent

all:$(targets)

$(targets):%:%.c
	$(CC) $< -o $@ $(CFLAGS)

.PHONY:clean all
clean:
	rm -rf $(targets) *.fifo
