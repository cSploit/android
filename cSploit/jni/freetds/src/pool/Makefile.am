# $Id: Makefile.am,v 1.17 2012-01-16 00:27:04 jklowden Exp $
AM_CPPFLAGS	=	-I$(top_srcdir)/include -I. -I$(SERVERDIR)
bin_PROGRAMS	=	tdspool

tdspool_SOURCES	=	config.c main.c member.c user.c util.c pool.h
SERVERDIR	=	../server
LDADD		=	../server/libtdssrv.la $(LTLIBICONV) $(NETWORK_LIBS)
EXTRA_DIST	=	BUGS pool.conf

ETC	=	$(DESTDIR)$(sysconfdir)

install-data-local:
	$(mkinstalldirs) $(ETC)
	if test ! -f $(ETC)/pool.conf; then \
		$(INSTALL) $(srcdir)/pool.conf $(ETC)/pool.conf; \
	fi
