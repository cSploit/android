# $Id: Makefile.am,v 1.5 2011-06-08 09:25:52 freddy77 Exp $

AM_CPPFLAGS	= -I$(top_srcdir)/include 

if MINGW32
else
bin_PROGRAMS	= fisql

fisql_SOURCES	= fisql.c  \
		terminal.h terminal.c  \
		edit.h edit.c  \
		handlers.h handlers.c  \
		interrupt.h interrupt.c
		

fisql_LDADD	= ../../dblib/libsybdb.la \
		  ../../replacements/libreplacements.la \
		  $(NETWORK_LIBS) $(LTLIBICONV) $(READLINE_LIBS)
endif
