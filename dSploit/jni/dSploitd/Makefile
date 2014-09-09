# LICENSE
#
#
#
#

CC := gcc
LD := gcc

LDFLAGS := -Wl,-export-dynamic -rdynamic 
LIBS := -lpthread -ldl
CFLAGS := -g -O0 -Werror -Wall -I. -fPIC 
#CFLAGS := -g -O2 -Werror -Wall -I. -DNDEBUG
DSPLOITD_OBJS := common.o command.o connection.o FS.o handler.o child.o\
		 control.o list.o message.o msgqueue.o cleanup.o dSploitd.o \
		 reaper.o

dSploitd: $(DSPLOITD_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	
handlers:
	$(MAKE) -C handlers
	
jni_client: jni_client.o common.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	
.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -f $(DSPLOITD_OBJS)
	make -C handlers clean

.PHONY: handlers
