CC = gcc
CFLAGS = -Wall -O2 -g
TARGET = example
INC = -I../src -I../../hiredis
LIB = -lpthread ../src/libdynoc.a ../../hiredis/libhiredis.a

srcs = $(wildcard *.c)
objs = $(srcs:.c=.o)

all: $(TARGET)

$(TARGET): $(objs)
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

.PHONY: clean
clean:
	rm -rf example.o.dSYM
	rm -f $(objs)
	rm -f $(TARGET)
