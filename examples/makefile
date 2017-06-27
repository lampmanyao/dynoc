CC = gcc
CFLAGS = -Wall -O2 -g
TARGET = example multi-thread-example
INC = -I../src -I../hiredis
LIB = ../src/libdynoc.a ../hiredis/libhiredis.a -lpthread 

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)

all: $(TARGET)

example: example.o
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LIB)

multi-thread-example: multi-thread-example.o
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.PHONY: clean
clean:
	rm -rf example.o.dSYM
	rm -f $(objs)
	rm -f $(TARGET)
