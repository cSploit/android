NWSDK = /mnt/novell/nwsdk
#NWSDK = /usr/src/ndk
CFLAGS = -DN_PLAT_NLM -nostdinc -fpack-struct -I${NWSDK}/include/nlm -I${NWSDK}/include -W -Wall -Winline -O2 -fomit-frame-pointer -mpreferred-stack-boundary=2 -fverbose-asm
#SERVER = /vana2/sys
SERVER = /nw5/sys

ncphook.nlm: ncphook.o ncphook.def prelude.o
	nlmconv -T ncphook.def -l ld

prelude.o: prelude.c
	$(CC) $(CFLAGS) -c -o prelude.o prelude.c

ncphook.o: ncphook.s
	$(CC) $(CFLAGS) -c ncphook.s

ncphook.s: ncphook.c ncphook.h
	$(CC) $(CFLAGS) -S ncphook.c

clean:
	rm -f *.o *.s *.nlm

mrproper: clean

distclean: mrproper

install: ncphook.nlm
	cp ncphook.nlm ${SERVER}/ncphook.nlm
