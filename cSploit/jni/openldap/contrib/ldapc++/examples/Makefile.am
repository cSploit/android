# $OpenLDAP$

##
# Copyright 2003-2014 The OpenLDAP Foundation, All Rights Reserved.
# COPYING RESTRICTIONS APPLY, see COPYRIGHT file
##
AM_CPPFLAGS = -I$(top_srcdir)/src
noinst_PROGRAMS = main readSchema startTls urlTest

main_SOURCES = main.cpp
main_LDADD = ../src/libldapcpp.la

readSchema_SOURCES = readSchema.cpp
readSchema_LDADD = ../src/libldapcpp.la

startTls_SOURCES = startTls.cpp
startTls_LDADD = ../src/libldapcpp.la

urlTest_SOURCES = urlTest.cpp
urlTest_LDADD = ../src/libldapcpp.la
