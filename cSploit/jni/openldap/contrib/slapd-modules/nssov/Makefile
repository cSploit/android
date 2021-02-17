# $OpenLDAP$
# This work is part of OpenLDAP Software <http://www.openldap.org/>.
#
# Copyright 2008-2014 The OpenLDAP Foundation.
# Portions Copyright 2008 Howard Chu, Symas Corp. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted only as authorized by the OpenLDAP
# Public License.
#
# A copy of this license is available in the file LICENSE in the
# top-level directory of the distribution or, alternatively, at
# <http://www.OpenLDAP.org/license.html>.

# Path to the OpenLDAP source tree
LDAP_SRC=../../..

# Path to the OpenLDAP object tree - same as above unless
# you're doing out-of-tree builds.
LDAP_BUILD=../../..

LDAP_INC = -I$(LDAP_BUILD)/include -I$(LDAP_SRC)/include -I$(LDAP_SRC)/servers/slapd
LDAP_LIB = $(LDAP_BUILD)/libraries/libldap_r/libldap_r.la \
	$(LDAP_BUILD)/libraries/liblber/liblber.la

NLDAPD_INC=-Inss-pam-ldapd

LIBTOOL = $(LDAP_BUILD)/libtool
OPT = -g -O2 -Wall
CC = gcc
DEFS =
INCS = $(LDAP_INC) $(NLDAPD_INC)
LIBS = $(LDAP_LIB)

prefix=/usr/local
exec_prefix=$(prefix)
ldap_subdir=/openldap

libdir=$(exec_prefix)/lib
libexecdir=$(exec_prefix)/libexec
moduledir = $(libexecdir)$(ldap_subdir)
sysconfdir = $(prefix)/etc$(ldap_subdir)
schemadir = $(sysconfdir)/schema

all:	nssov.la

XOBJS = tio.lo

OBJS = alias.lo ether.lo group.lo host.lo netgroup.lo network.lo \
	nssov.lo passwd.lo protocol.lo rpc.lo service.lo shadow.lo pam.lo

.SUFFIXES: .c .o .lo

.c.lo:
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $<

tio.lo:	nss-pam-ldapd/tio.c
	$(LIBTOOL) --mode=compile $(CC) $(OPT) $(DEFS) $(INCS) -c $?

$(OBJS):	nssov.h

nssov.la:	$(OBJS) $(XOBJS)
	$(LIBTOOL) --mode=link $(CC) $(OPT) -version-info 0:0:0 \
	-rpath $(libdir) -module -o $@ $(OBJS) $(XOBJS) $(LIBS)

install: nssov.la
	mkdir -p $(DESTDIR)$(moduledir)
	$(LIBTOOL) --mode=install cp nssov.la $(DESTDIR)$(moduledir)
	cp ldapns.schema $(DESTDIR)$(schemadir)

clean:
	rm -f *.*o *.la .libs/*
	rm -rf .libs
