
# $Id: Makefile.am,v 1.2 2004/02/22 12:00:53 alor Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = daemon text

if NCURSES
   SUBDIRS += curses
endif
	
if GTK
   SUBDIRS += gtk
endif
   
# vim:ts=4:noexpandtab
