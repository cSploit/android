# Linux
CC = gcc
LDFLAGS = -fPIC -fno-common -shared
LUA = lua5.1
CLUA=$(shell pkg-config --cflags ${LUA})
LLUA=$(shell pkg-config --libs ${LUA})

# Example:
# cc -Wall -Werror -g -I wurldtech/rst `libnet-config --cflags --defines`
# `dnet-config --cflags` -O0 -DNDEBUG -fPIC -fno-common -shared
# -I/usr/include/lua5.1 -o wurldtech/lgram/net.so wurldtech/lgram/net.c -lrt
#  -lm `dnet-config --libs` `libnet-config --libs` -llua5.1

