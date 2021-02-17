# Generated automatically from Makefile.in by configure.
# Master Makefile for the GNU readline library.
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

srcdir = .

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib
mandir = $(prefix)/man/man1

SHELL = /bin/sh

INSTALL = /bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

RANLIB = ranlib
AR = ar
RM = rm
CP = cp
MV = mv

DEFS = -DHAVE_CONFIG_H
CFLAGS = -g
LDFLAGS = -g

# For libraries which include headers from other libraries.
INCLUDES = -I. -I$(srcdir)

CPPFLAGS = $(DEFS) $(INCLUDES)

PICFLAG=        -pic    # -fpic for some versions of gcc
SHLIB_OPTS=	-assert pure-text -ldl	# -Bshareable for some versions of gcc
MAJOR=		2
MINOR=		.0

.SUFFIXES:	.so
.c.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

.c.so:
	-mv $*.o z$*.o
	$(CC) -c $(PICFLAG) $(CPPFLAGS) $(CFLAGS) $< 
	mv $*.o $@
	-mv z$*.o $*.o

# The name of the main library target.
LIBRARY_NAME = libreadline.a
SHARED_READLINE = libreadline.so.$(MAJOR)$(MINOR)
SHARED_HISTORY = libhistory.so.$(MAJOR)$(MINOR)
SHARED_LIBS = $(SHARED_READLINE) $(SHARED_HISTORY)

# The C code source files for this library.
CSOURCES = $(srcdir)readline.c $(srcdir)funmap.c $(srcdir)keymaps.c \
	   $(srcdir)vi_mode.c $(srcdir)parens.c $(srcdir)rltty.c \
	   $(srcdir)complete.c $(srcdir)bind.c $(srcdir)isearch.c \
	   $(srcdir)display.c $(srcdir)signals.c $(srcdir)emacs_keymap.c \
	   $(srcdir)vi_keymap.c $(srcdir)history.c $(srcdir)tilde.c \
	   $(srcdir)xmalloc.c

# The header files for this library.
HSOURCES = readline.h rldefs.h chardefs.h keymaps.h history.h \
	   posixstat.h tilde.h rlconf.h

INSTALLED_HEADERS = readline.h chardefs.h keymaps.h history.h tilde.h

OBJECTS = readline.o vi_mode.o funmap.o keymaps.o parens.o search.o \
	  rltty.o complete.o bind.o isearch.o display.o signals.o \
	  history.o tilde.o xmalloc.o

SHARED_OBJ = readline.so vi_mode.so funmap.so keymaps.so parens.so search.so \
	  rltty.so complete.so bind.so isearch.so display.so signals.so \
	  history.so tilde.so xmalloc.so

# The texinfo files which document this library.
DOCSOURCE = doc/rlman.texinfo doc/rltech.texinfo doc/rluser.texinfo
DOCOBJECT = doc/readline.dvi
DOCSUPPORT = doc/Makefile
DOCUMENTATION = $(DOCSOURCE) $(DOCOBJECT) $(DOCSUPPORT)

SUPPORT = Makefile ChangeLog $(DOCSUPPORT) examples/[-a-z.]*

SOURCES  = $(CSOURCES) $(HSOURCES) $(DOCSOURCE)

THINGS_TO_TAR = $(SOURCES) $(SUPPORT)

##########################################################################

all: libreadline.a libhistory.a
shared: $(SHARED_LIBS)

libreadline.a: $(OBJECTS)
	$(RM) -f $@
	$(AR) cq $@ $(OBJECTS)
	-$(RANLIB) $@

libhistory.a: history.o
	$(RM) -f $@
	$(AR) cq $@ history.o
	-$(RANLIB) $@

$(SHARED_READLINE):	$(SHARED_OBJ)
	$(RM) -f $@
	ld ${SHLIB_OPTS} -o $@ $(SHARED_OBJ)

$(SHARED_HISTORY):	history.so
	$(RM) -f $@
	ld ${SHLIB_OPTS} -o $@ history.so

documentation: force
	[ ! -d doc ] && mkdir doc
	(if [ -d doc ]; then cd doc; $(MAKE) $(MFLAGS); fi)

force:

install: installdirs libreadline.a
	$(INSTALL_DATA) $(INSTALLED_HEADERS) $(incdir)/readline
	-$(MV) $(libdir)/libreadline.a $(libdir)/libreadline.old
	$(INSTALL_DATA) libreadline.a $(libdir)/libreadline.a
	-$(RANLIB) -t $(libdir)/libreadline.a

installdirs:
	[ ! -d $(incdir)/readline ] && { \
	  mkdir $(incdir)/readline && chmod 755 $(incdir)/readline; }
	[ ! -d $(libdir) ] && mkdir $(libdir)

uninstall:
	[ -n "$(incdir)" ] && cd $(incdir)/readline && \
		${RM} -f ${INSTALLED_HEADERS}
	[ -n "$(libdir)" ] && cd $(libdir) && \
		${RM} -f libreadline.a libreadline.old $(SHARED_LIBS)

clean::
	rm -f $(OBJECTS) *.a $(SHARED_OBJ) $(SHARED_LIBS)
	(if [ -d doc ]; then cd doc; $(MAKE) $(MFLAGS) $@; fi)

readline: readline.h rldefs.h chardefs.h
readline: $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(READLINE_DEFINES) \
	  $(LOCAL_INCLUDES) -DTEST -o readline readline.c vi_mode.o funmap.o \
	  keymaps.o -ltermcap

info:
install-info:
dvi:
check:
installcheck:

tags:	force
	etags $(CSOURCES) $(HSOURCES)

TAGS:	force
	ctags -x $(CSOURCES) $(HSOURCES) > $@

Makefile: config.status $(srcdir)/Makefile.in
	$(SHELL) config.status

config.status: configure
	$(SHELL) config.status --recheck

configure: configure.in            ## Comment-me-out in distribution
	cd $(srcdir); autoconf     ## Comment-me-out in distribution
config.h.in: configure.in          ## Comment-me-out in distribution
	cd $(srcdir); autoheader   ## Comment-me-out in distribution

mostlyclean: clean

distclean realclean: clean
	rm -f Makefile config.status config.h stamp-config

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

# Dependencies
readline.o: readline.c readline.h rldefs.h rlconf.h chardefs.h
readline.o: keymaps.h history.h
vi_mode.o:  rldefs.h rlconf.h readline.h history.h
funmap.o:   funmap.c readline.h rlconf.h
keymaps.o:  keymaps.c emacs_keymap.c vi_keymap.c keymaps.h chardefs.h rlconf.h
history.o: history.h memalloc.h
isearch.o: memalloc.h readline.h history.h
search.o: memalloc.h readline.h history.h
display.o: readline.h history.h rldefs.h rlconf.h
complete.o: readline.h rldefs.h rlconf.h
rltty.o: rldefs.h rlconf.h readline.h
bind.o: rldefs.h rlconf.h readline.h history.h
signals.o: rldefs.h rlconf.h readline.h history.h
parens.o: readline.h
