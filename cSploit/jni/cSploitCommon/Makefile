# LICENSE
#
#
#
#

CC := gcc
LD := gcc
AR := ar

LIBS := -lpthread -ldl
CFLAGS := -g -O0 -Werror -Wall -I. -fPIC 
#CFLAGS += -DNDEBUG
SRCS:=$(wildcard *.c )
OBJS := $(SRCS:.c=.o)

all: libcSploitCommon.so libcSploitCommon_static.a

libcSploitCommon.so: $(OBJS)
	$(LD) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
	
libcSploitCommon_static.a: $(OBJS)
	$(AR) -r $@ $^
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f cSploitd $(OBJS)
