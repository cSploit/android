# LICENSE
#
#
#
#

CC := gcc
LD := gcc

LDFLAGS := -Wl,-export-dynamic -rdynamic
LIBS := -lpthread -ldl -L../dSploitCommon -ldSploitCommon
CFLAGS := -g -O0 -Werror -Wall -I. -I../dSploitCommon -fPIC 
#CFLAGS += -DNDEBUG
SRCS:=$(wildcard *.c )
OBJS := $(SRCS:.c=.o)

dSploitd: $(OBJS) libdSploitCommon.so
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

	
libdSploitCommon.so:
	$(MAKE) -C ../dSploitCommon
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f dSploitd $(OBJS)
