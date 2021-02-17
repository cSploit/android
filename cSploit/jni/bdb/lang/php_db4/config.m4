#
# Copyright (c) 2004, 2014 Oracle and/or its affiliates.  All rights reserved.
#
# http://www.apache.org/licenses/LICENSE-2.0.txt
#

dnl $Id$
dnl config.m4 for extension db4

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

PHP_ARG_WITH(db4, whether to enable db4 support,
[  --with-db4           Enable db4 support])

if test "$PHP_DB4" != "no"; then
  if test "$PHP_DB4" != "no"; then
    for i in $PHP_DB4 /usr/local/BerkeleyDB.6.0 /usr/local /usr; do
      if test -f "$i/db4/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/db4
        THIS_INCLUDE=$i/db4/db.h
        break
      elif test -f "$i/include/db4/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include/db4
        THIS_INCLUDE=$i/include/db4/db.h
        break
      elif test -f "$i/include/db/db4.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include/db4
        THIS_INCLUDE=$i/include/db/db4.h
        break
      elif test -f "$i/include/db4.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include
        THIS_INCLUDE=$i/include/db4.h
        break
      elif test -f "$i/include/db.h"; then
        THIS_PREFIX=$i
        INC_DIR=$i/include
        THIS_INCLUDE=$i/include/db.h
        break
      fi
    done
    PHP_ADD_INCLUDE($INC_DIR)
    PHP_ADD_LIBRARY_WITH_PATH(db_cxx, $THIS_PREFIX/lib, DB4_SHARED_LIBADD)
  fi 

  PHP_ADD_LIBRARY(db_cxx, $THIS_PREFIX/lib, DB4_SHARED_LIBADD)
  AC_MSG_RESULT(no)
  EXTRA_CXXFLAGS="-g -DHAVE_CONFIG_H -O2 -Wall"
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY(stdc++, 1, DB4_SHARED_LIBADD)
  PHP_NEW_EXTENSION(db4, db4.cpp, $ext_shared)
  PHP_ADD_MAKEFILE_FRAGMENT
  PHP_SUBST(DB4_SHARED_LIBADD)
  AC_MSG_WARN([*** A note about pthreads ***
  The db4 c++ library by default tries to link against libpthread on some
  systems (notably Linux).  If your PHP install is not linked against
  libpthread, you will need to disable pthread support in db4.  This can
  be done by compiling db4 with the flag  --with-mutex=x86/gcc-assembly.
  PHP can itself be forced to link against libpthread either by manually editing
  its build files (which some distributions do), or by building it with
  --with-experimental-zts.])


fi
