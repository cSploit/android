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
CFLAGS  := -g -O0 -shared -I../cSploitCommon -I../cSploitDaemon -fPIC -Wall -Wno-trigraphs -Werror
LDFLAGS := -Wl,--as-needed -lpthread

SRCS:= $(wildcard *.c)

HANDLERS:= $(SRCS:.c=.so)

handlers: $(HANDLERS)

%.so: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	
clean:
	rm -f $(HANDLERS)
