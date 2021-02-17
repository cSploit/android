#  Copyright (c) 2008-2010, International Business Machines Corporation and others. All Rights Reserved.
# 
#
# Makefile for regenerating configure in the face of a bad ^M
# This should become unnecessary for autoconf past 2.63
# 
# Usage:    MAKE -f configure.mk configure

AUTOCONF=autoconf
all: configure common/icucfg.h.in

common/icucfg.h.in: configure.in
	autoheader

configure:	configure.in ./aclocal.m4
	( $(AUTOCONF) && mv configure configure.tmp && sed -e 's%^ac_cr=.*%ac_cr=`echo X |tr X "\\015"`%'  < configure.tmp > configure && chmod a+rx $@ && rm configure.tmp ) || ( rm $@ ; "echo configure build failed" ; /usr/bin/false  )

-include configure-local.mk
