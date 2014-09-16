# LICENSE
#
#
#
#

CC := gcc
LD := gcc

LDFLAGS := -Wl,-export-dynamic -rdynamic 
LIBS := -lpthread -ldl
CFLAGS := -g -O0 -Werror -Wall -I. -Icommon -fPIC 
#CFLAGS += -DNDEBUG
SRCS:=$(wildcard *.c common/*.c)
OBJS := $(SRCS:.c=.o)

dSploitd: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f $(OBJS)
	make -C handlers clean
