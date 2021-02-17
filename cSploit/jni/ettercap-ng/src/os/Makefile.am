
# $Id: Makefile.am,v 1.2 2004/06/27 12:51:02 alor Exp $

include $(top_srcdir)/Makefile.am.common

noinst_LIBRARIES = libec_os.a

libec_os_a_CFLAGS = @EC_CFLAGS@

libec_os_a_SOURCES = ec_dummy.c

if LINUX
   libec_os_a_SOURCES += ec_linux.c
endif

if BSD
   libec_os_a_SOURCES += ec_bsd.c
endif

if DARWIN
   libec_os_a_SOURCES += ec_darwin.c
endif

if SOLARIS
   libec_os_a_SOURCES += ec_solaris.c
endif

if CYGWIN
   libec_os_a_SOURCES += ec_cygwin.c
endif

if MINGW
   libec_os_a_SOURCES += ec_mingw.c
endif

   
# vim:ts=4:noexpandtab
