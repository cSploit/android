# $Id: Makefile.am,v 1.18 2011-06-08 09:25:53 freddy77 Exp $
AM_CPPFLAGS	=	-I$(top_srcdir)/include
noinst_LTLIBRARIES	=	libtdssrv.la
libtdssrv_la_SOURCES=	query.c server.c login.c
libtdssrv_la_LIBADD =	../tds/libtds.la ../replacements/libreplacements.la $(NETWORK_LIBS) $(LTLIBICONV) $(FREETDS_LIBGCC)
noinst_PROGRAMS	= tdssrv
tdssrv_LDADD	= libtdssrv.la $(LTLIBICONV) $(NETWORK_LIBS)
tdssrv_SOURCES	= unittest.c
