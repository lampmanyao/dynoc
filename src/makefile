cc = gcc
cflags = -Wall -O2 -I../hiredis
srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)

target = libdynoc.a

all: $(target)

debug: cflags = -Wall -O0 -DDEBUG -g -I../hiredis
debug: $(target)

$(target): $(objs)
	ar rc $@ $^

%.o: %.c
	$(cc) $(cflags) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(objs)
	rm -f $(target)
