# OS X
CC = MACOSX_DEPLOYMENT_TARGET="10.3" gcc
LDFLAGS = -fno-common -bundle -undefined dynamic_lookup
CLUA=-I/usr/local/include
LLUA=-llua

