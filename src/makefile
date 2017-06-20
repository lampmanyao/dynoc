cc = gcc
cflags = -Wall -g -O2 -I../../hiredis
srcs = $(wildcard *.c)
cobjs = $(srcs:.c=.o)
objdir = .objs
objs = $(addprefix $(objdir)/, $(cobjs))

$(objdir):
	@mkdir -p $@

target = libdynoc.a

all: $(target)

$(target): $(objs)
	ar rc $@ $^

$(objdir)/%.o: %.c | $(objdir)
	$(cc) $(cflags) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(objdir)
	rm -f $(target)
