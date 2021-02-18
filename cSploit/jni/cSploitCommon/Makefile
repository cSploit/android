## cSploit - a simple penetration testing suite
## Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
## 
## cSploit is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## cSploit is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with cSploit.  If not, see <http://www.gnu.org/licenses/>.

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
