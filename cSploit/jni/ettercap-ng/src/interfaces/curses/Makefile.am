
# $Id: Makefile.am,v 1.11 2004/10/12 14:27:28 alor Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = widgets

noinst_HEADERS = ec_curses.h

noinst_LIBRARIES = libec_curses.a

libec_curses_a_CFLAGS = @EC_CFLAGS@ @NCURSINC@ -Iwidgets

libec_curses_a_SOURCES = ec_curses.c \
                         ec_curses_filters.c \
                         ec_curses_help.c \
                         ec_curses_hosts.c \
                         ec_curses_live.c \
                         ec_curses_logging.c \
                         ec_curses_mitm.c \
                         ec_curses_offline.c \
                         ec_curses_plugins.c \
                         ec_curses_start.c \
                         ec_curses_targets.c \
                         ec_curses_view.c \
                         ec_curses_view_connections.c \
                         ec_curses_view_profiles.c 

# hack to include libwdg.a objects into libec_curses.a
all: libec_curses.a 
	@if test "`find widgets/libwdg.a -newer libec_curses.a`" != "" -o "`ar t libec_curses.a | grep libwdg`" = ""; then \
		$(AR) x widgets/libwdg.a;			\
		$(AR) ru libec_curses.a *wdg*;	\
      $(RANLIB) libec_curses.a; \
		echo "Joining libwdg objects into libec_curses..."; \
	fi

# vim:ts=4:noexpandtab

