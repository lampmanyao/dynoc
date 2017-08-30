cc = gcc
cflags = -Wall -O2
TARGET = example multi-thread-example
inc = -I../src -I../hiredis
lib = ../src/libdynoc.a ../hiredis/libhiredis.a -lpthread 

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)

all: $(TARGET)

debug: cflags = -Wall -O0 -DDEBUG -g
debug: $(TARGET)

example: example.o
	$(cc) $(cflags) $(inc) -o $@ $< $(lib)

multi-thread-example: multi-thread-example.o
	$(cc) $(cflags) $(inc) -o $@ $< $(lib)

%.o: %.c
	$(cc) $(cflags) $(inc) -c $< -o $@

.PHONY: clean
clean:
	rm -rf example.o.dSYM
	rm -f $(objs)
	rm -f $(TARGET)
