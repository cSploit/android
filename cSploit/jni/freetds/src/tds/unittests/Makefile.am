# $Id: Makefile.am,v 1.31 2011-10-31 13:58:41 freddy77 Exp $
TESTS		=	t0001$(EXEEXT) t0002$(EXEEXT) t0003$(EXEEXT)\
			t0004$(EXEEXT) t0005$(EXEEXT) t0006$(EXEEXT)\
			t0007$(EXEEXT) t0008$(EXEEXT) dynamic1$(EXEEXT)\
			convert$(EXEEXT) dataread$(EXEEXT) utf8_1$(EXEEXT)\
			utf8_2$(EXEEXT) utf8_3$(EXEEXT) numeric$(EXEEXT) \
			iconv_fread$(EXEEXT) toodynamic$(EXEEXT) \
			$(CHALLENGE_TEST) readconf$(EXEEXT) charconv$(EXEEXT) \
			nulls$(EXEEXT)

# flags test commented, not necessary for 0.62
# TODO add flags test again when needed

check_PROGRAMS	=	$(TESTS)

t0001_SOURCES	=	t0001.c common.c common.h
t0002_SOURCES	=	t0002.c common.c common.h
t0003_SOURCES	=	t0003.c common.c common.h
t0004_SOURCES	=	t0004.c common.c common.h
t0005_SOURCES	=	t0005.c common.c common.h
t0006_SOURCES	=	t0006.c common.c common.h
t0007_SOURCES	=	t0007.c
t0008_SOURCES	=	t0008.c
dynamic1_SOURCES =	dynamic1.c common.c common.h
convert_SOURCES	=	convert.c
dataread_SOURCES	=	dataread.c common.c common.h
# flags_SOURCES	=	flags.c common.c common.h
utf8_1_SOURCES	=	utf8_1.c common.c common.h utf8.c
utf8_2_SOURCES	=	utf8_2.c common.c common.h
utf8_3_SOURCES	=	utf8_3.c common.c common.h utf8.c
numeric_SOURCES =       numeric.c
iconv_fread_SOURCES	= iconv_fread.c
charconv_SOURCES	= charconv.c
toodynamic_SOURCES	= toodynamic.c common.c common.h
nulls_SOURCES	= nulls.c common.c common.h
if HAVE_SSPI
CHALLENGE_TEST =
else
CHALLENGE_TEST = challenge$(EXEEXT)
challenge_SOURCES	= challenge.c
endif
readconf_SOURCES	= readconf.c

AM_CPPFLAGS	=	-I$(top_srcdir)/include -I$(srcdir)/.. -I../ -DFREETDS_TOPDIR=\"$(top_srcdir)\"
if MINGW32
AM_LDFLAGS	=	-no-fast-install
else
AM_LDFLAGS	=	-no-install -L../.libs -R $(abs_builddir)/../.libs
endif
LIBS		=	../libtds.la ../../replacements/libreplacements.la $(LTLIBICONV) @NETWORK_LIBS@
CLEANFILES	=	tdsdump.out
EXTRA_DIST	=	challenge.c CMakeLists.txt readconf.in

