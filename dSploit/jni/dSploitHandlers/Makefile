## LICENSE
##
##
CC := gcc
CFLAGS  := -g -O0 -shared -I../dSploitCommon -I../dSploitd -fPIC -Wall -Wno-trigraphs -Werror
LDFLAGS := -Wl,--as-needed -lpthread

SRCS:= $(wildcard *.c)

HANDLERS:= $(SRCS:.c=.so)

handlers: $(HANDLERS)

%.so: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	
clean:
	rm -f $(HANDLERS)
