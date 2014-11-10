# LICENSE
#
#
#
#

CC := gcc
LD := gcc

LDFLAGS := -Wl,-export-dynamic -rdynamic
LIBS := -lpthread -ldl -L../cSploitCommon -lcSploitCommon
CFLAGS := -g -O0 -Werror -Wall -I. -I../cSploitCommon -fPIC 
#CFLAGS += -DNDEBUG
SRCS:=$(wildcard *.c )
OBJS := $(SRCS:.c=.o)

cSploitd: $(OBJS) libcSploitCommon.so
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

	
libcSploitCommon.so:
	$(MAKE) -C ../cSploitCommon
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f cSploitd $(OBJS)
