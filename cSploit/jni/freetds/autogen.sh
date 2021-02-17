#!/bin/sh
# Run this to generate all the initial makefiles, etc.

# $Id: autogen.sh,v 1.10 2011-03-22 17:54:04 jklowden Exp $

# From automake.info:
#
#    Many packages come with a script called `bootstrap.sh' or
# `autogen.sh', that will just call `aclocal', `libtoolize', `gettextize'
# or `autopoint', `autoconf', `autoheader', and `automake' in the right
# order.  Actually this is precisely what `autoreconf' can do for you.
# If your package has such a `bootstrap.sh' or `autogen.sh' script,
# consider using `autoreconf'.  That should simplify its logic a lot
# (less things to maintain, yum!), it's even likely you will not need the
# script anymore, and more to the point you will not call `aclocal'
# directly anymore.

srcdir=`dirname $0`
PKG_NAME="FreeTDS."

# If autoreconf encounters an error, it might be because this is the 
# very first time it was run, meaning that some files e.g. config.sub
# are missing.  We retry with --install (and perhaps we should 
# try --force, too).  
#
# Revision 1.6 was the last one not to use autoreconf.  If you can't get
# this (simpler) one to work, you might try that one. 

(	cd $srcdir
	echo running `which autoreconf` in `pwd`: 
	autoreconf || autoreconf --install
) || exit

#conf_flags="--enable-maintainer-mode --enable-compile-warnings" #--enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" '...' \
  	| tr ' ' \\n \
  	| sed  's/^-/	&/'
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME
else
  echo Skipping configure process.
fi
