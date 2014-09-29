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

all: libdSploitCommon.so libdSploitCommon_static.a

libdSploitCommon.so: $(OBJS)
	$(LD) $(LDFLAGS) -shared -o $@ $^ $(LIBS)
	
libdSploitCommon_static.a: $(OBJS)
	$(AR) -r $@ $^
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f dSploitd $(OBJS)
