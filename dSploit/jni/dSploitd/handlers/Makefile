## LICENSE
##
##
CC := gcc
CFLAGS  := -g -O0 -shared -I.. -fPIC
LDFLAGS := -Wl,--as-needed -lpthread

SRCS:= $(wildcard *.c)

HANDLERS:= $(SRCS:.c=.so)

handlers: $(HANDLERS)

%.so: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	
clean:
	rm -f $(HANDLERS)
