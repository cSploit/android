# $Id: Makefile.am,v 1.29 2011-06-08 09:25:53 freddy77 Exp $
SUBDIRS		=	unittests
AM_CPPFLAGS	=	-I$(top_srcdir)/include

lib_LTLIBRARIES	=	libct.la
if MINGW32
MINGW_SOURCES   =	../../win32/initnet.c
else
MINGW_SOURCES   =
endif
libct_la_SOURCES=	ct.c cs.c blk.c ctutil.c $(MINGW_SOURCES)

# version bumped for 0.64, I hope we'll bump it less in the future -- freddy77
if MACOSX
SYMBOLS		=
else
SYMBOLS		=	-export-symbols-regex '^(cs_|ct_|blk_).*'	
endif
libct_la_LDFLAGS=	-version-info 4:0:0 $(SYMBOLS) $(FREETDS_SYMBOLIC)

libct_la_LIBADD	=	../tds/libtds.la ../replacements/libreplacements.la $(NETWORK_LIBS) $(LTLIBICONV) $(FREETDS_LIBGCC)

EXTRA_DIST		= ctlib.def CMakeLists.txt

