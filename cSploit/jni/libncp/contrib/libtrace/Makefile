#
#
#
top_srcdir = ../..
top_builddir = ../..
this_srcdir = ${top_srcdir}/contrib/libtrace

include ${top_builddir}/Make.rules

.PHONY : all clean distclean mrproper

all: tracencp.so

tracencp.so: tracencp.c allsyms.c
	$(CC) $(CFLAGS) -o tracencp.so -shared tracencp.c -L${top_builddir}/lib -lncp -ldl

allsyms.c: ${top_builddir}/lib/libncp.so
	objdump -T ${top_builddir}/lib/libncp.so | grep 'DF .text' | rev | cut -d" " -f1 | rev | sed 's/^\(.*\)$$/DO_ASM_SYMBOL_EASY(\1)/' > allsyms.c

clean:
	-rm -f allsyms.c tracencp.so

distclean: clean

mrproper: clean
