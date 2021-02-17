
# $Id: Makefile.am,v 1.19 2004/02/26 09:26:16 alor Exp $

include $(top_srcdir)/Makefile.am.common

# don't link ettercap libs
LIBS =

bin_PROGRAMS = etterfilter

AM_YFLAGS = --defines

etterfilter_SOURCES = ef_compiler.c \
                      ef_main.c \
                      ef_ec_compat.c \
                      ef_encode.c \
                      ef_parser.c \
                      ef_test.c \
                      ef_grammar.y \
                      ef_output.c \
                      ef_syntax.l \
                      ef_tables.c \
                      $(top_srcdir)/src/ec_error.c \
                      $(top_srcdir)/src/ec_file.c \
                      $(top_srcdir)/src/ec_filter.c \
                      $(top_srcdir)/src/ec_strings.c

noinst_HEADERS = ef_grammar.h
                   
if MISSING_STRSEP
   etterfilter_SOURCES += $(top_srcdir)/src/missing/strsep.c
endif
if MISSING_STRLCPY
   etterfilter_SOURCES += $(top_srcdir)/src/missing/strlcpy.c
endif
if MISSING_MEMMEM
   etterfilter_SOURCES += $(top_srcdir)/src/missing/memmem.c
endif
if MISSING_INET_ATON
   etterfilter_SOURCES += $(top_srcdir)/src/missing/inet_aton.c
endif
if MISSING_GETOPT_LONG
   etterfilter_SOURCES += $(top_srcdir)/src/missing/getopt.c
endif

etterfilter_CFLAGS = @EC_CFLAGS@

if BSD
   etterfilter_CFLAGS += -pthread
endif
   
etterfilter_LDADD = 

if INET_ATON_WANTS_RESOLV
   etterfilter_LDADD += -lresolv
endif

if HAVE_PCRE
   etterfilter_LDADD += @PCRELIB@ 
endif

CLEANFILES = *.log *.out core* *~

# vim:ts=4:noexpandtab
