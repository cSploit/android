
# $Id: Makefile.am,v 1.15 2004/02/22 12:00:54 alor Exp $

include $(top_srcdir)/Makefile.am.common

noinst_HEADERS = wdg.h

noinst_LIBRARIES = libwdg.a

libwdg_a_CFLAGS = @EC_CFLAGS@ @NCURSINC@

libwdg_a_SOURCES = wdg.c \
                   wdg_compound.c \
                   wdg_debug.c \
                   wdg_dialog.c \
                   wdg_dynlist.c \
                   wdg_error.c \
                   wdg_file.c \
                   wdg_input.c \
                   wdg_list.c \
                   wdg_menu.c \
                   wdg_panel.c \
                   wdg_percentage.c \
                   wdg_scroll.c \
                   wdg_window.c
                   
EXTRA_DIST = README

# vim:ts=4:noexpandtab

