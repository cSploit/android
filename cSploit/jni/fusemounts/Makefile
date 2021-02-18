# 
# fusermounts - show mounted fuse filesystems
# Copyright (C) 2014  massimo dragano aka tux_mind <massimo.dragano@gmail.com>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

CC ?= gcc
CFLAGS ?= -Wall -Werror
LDFLAGS ?= -lm

OBJS := main.o finder.o creator.o bridge.o

fusemounts: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
		
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f $(OBJS)
	
.PHONY: clean