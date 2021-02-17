# $Id: Makefile.am,v 1.20 2012-03-06 20:40:12 freddy77 Exp $
SUBDIRS=	unittests
AM_CPPFLAGS=			-I$(top_srcdir)/include -I$(top_srcdir)/src/replacements
noinst_LTLIBRARIES=		libreplacements.la
libreplacements_la_SOURCES=	iconv.c gettimeofday.c fakepoll.c getpassarg.c socketpair.c win_mutex.c win_cond.c getaddrinfo.c
libreplacements_la_LDFLAGS=
libreplacements_la_LIBADD=	@LTLIBOBJS@
EXTRA_DIST=	asprintf.c \
		atoll.c \
		basename.c \
		getopt.c \
		readpassphrase.c \
		strlcat.c \
		getaddrinfo.c \
		strlcpy.c \
		strtok_r.c \
		strsep.c \
		vasprintf.c \
		socketpair.c \
		replacements.vcproj \
		ptw32_MCS_lock.c \
		CMakeLists.txt

