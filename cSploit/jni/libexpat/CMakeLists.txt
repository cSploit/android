# Copyright (C) 2007-2012 LuaDist.
# Created by Peter Kapec, Peter Drahoš
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.

project ( libexpat C )
cmake_minimum_required ( VERSION 2.8 )
include ( cmake/dist.cmake )

# Generating expat_config.h file:
message ( "Configure: Expat - generating expat_config.h:" )
include ( CheckIncludeFiles )
include ( CheckFunctionExists )
include ( CheckSymbolExists )
include ( TestBigEndian )
include ( CheckTypeSize )

test_big_endian ( ENDIAN_TEST )
if ( ENDIAN_TEST )
  set ( BYTEORDER 4321 )
else ( )
  set ( BYTEORDER 1234 )
endif ( )

check_function_exists ( bcopy HAVE_BCOPY )
check_include_files ( dlfcn.h HAVE_DLFCN_H )
check_include_files ( fcntl.h HAVE_FCNTL_H )
check_function_exists ( getpagesize HAVE_GETPAGESIZE )
check_include_files ( inttypes.h HAVE_INTTYPES_H )
check_function_exists ( memmove HAVE_MEMMOVE )
check_include_files ( memory.h HAVE_MEMORY_H )
check_function_exists ( mmap HAVE_MMAP )
check_include_files ( stdint.h HAVE_STDINT_H )
check_include_files ( stdlib.h HAVE_STDLIB_H )
check_include_files ( strings.h HAVE_STRINGS_H )
check_include_files ( string.h HAVE_STRING_H )
check_include_files ( "sys/stat.h" HAVE_SYS_STAT_H )
check_include_files ( "sys/types.h" HAVE_SYS_TYPES_H )
check_include_files ( unistd.h HAVE_UNISTD_H )
set ( PACKAGE_BUGREPORT "expat-bugs@libexpat.org" )
set ( PACKAGE_NAME "expat" )
set ( PACKAGE_STRING "expat 2.0.0" )
set ( PACKAGE_TARNAME "expat" )
set ( PACKAGE_VERSION "2.0.0" )
check_include_files ( "stdlib.h;stdarg.h;string.h;float.h" STDC_HEADERS )

#WARNING: Following tests might NOT be the original test as found in
# original ./configure script!
#TODO: Someone has to check it, pliz (I'm not an automake guru).

# how to check WORDS_BIGENDIAN ? maybe:
set ( WORDS_BIGENDIAN ${ENDIAN_TEST} )

# TODO:
#/* Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C. */
#cmakedefine __func__

# TODO:
#/* Define to empty if `const' does not conform to ANSI C. */
#cmakedefine const

#/* Define to `long' if <sys/types.h> does not define. */
check_type_size ( "off_t" SIZEOF_OFF_T )
if ( HAVE_SIZEOF_OFF_T )
  set ( HAVE_OFF_T 1 )
else ( )
  set ( HAVE_OFF_T 0 )
  set ( off_t "long" )
endif ( )

#/* Define to `unsigned' if <sys/types.h> does not define. */
check_type_size ( "size_t" SIZEOF_SIZE_T )
if ( HAVE_SIZEOF_SIZE_T )
  set ( HAVE_SIZE_T 1 )
else ( )
  set ( HAVE_SIZE_T 0 )
  set ( size_t "unsigned" )
endif ( )

configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/expat_config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/expat_config.h )

message ( "Configure: Expat - done." )
# end of generating expat_config.h file:

###########################################################################################

# Expat library
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_BINARY_DIR} )

if ( MSVC )
  set ( DEF_FILE lib/libexpat.def )
endif ( )

set ( SRC_EXPATLIB lib/xmlparse.c lib/xmlrole.c lib/xmltok.c )

add_definitions ( -DHAVE_EXPAT_CONFIG_H )

add_library ( expat ${SRC_EXPATLIB} ${DEF_FILE} )

install_library ( expat )
install_header ( lib/expat.h lib/expat_external.h )
install_data ( COPYING Changes README )
install_doc ( doc/ )

# IMPROVE: install xmlwf utility too? (see below test suite)

# Tests
include ( CTest )
if ( BUILD_TESTING )
  add_executable ( runtests tests/runtests.c tests/chardata.c tests/minicheck.c )
  target_link_libraries ( runtests expat )
  add_test ( runtests runtests )
  add_executable ( runtestspp tests/runtestspp.cpp tests/runtests.c tests/chardata.c 
    tests/minicheck.c )
  target_link_libraries ( runtestspp expat )
  add_test ( runtestspp runtestspp )

  if ( WIN32 OR MINGW )
    set ( FILEMAP win32filemap )
  else ( )
    set ( FILEMAP unixfilemap )
  endif ( )

  add_executable ( xmlwf xmlwf/xmlwf.c xmlwf/xmlfile.c xmlwf/codepage.c xmlwf/${FILEMAP}.c )
  target_link_libraries ( xmlwf expat )
  # These optional tests are normally disabled.
  # They don't seem to complete work using the GNU makefiles either.
  # Similer errors as seen in http://mail.libexpat.org/pipermail/expat-checkins/2006-August.txt
  if ( 0 AND UNIX )
    # IMPROVE: shell script only runs on *nix.
    if ( NOT EXISTS xmlts.zip )
      file ( DOWNLOAD http://www.w3.org/XML/Test/xmlts20020606.zip ./xmlts.zip )
    endif ( )
    if ( NOT EXISTS XML-Test-Suite )
      execute_process ( COMMAND unzip -q xmlts.zip )
      # IMPROVE: unzip might not exist
    endif ( )
    if ( NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/xmltest.sh )
      file ( READ ${CMAKE_CURRENT_SOURCE_DIR}/tests/xmltest.sh _data )
      string ( REGEX REPLACE "XMLWF=[^\n]*" "XMLWF=\$MYDIR/xmlwf" _data "${_data}" )
      file ( WRITE ${CMAKE_CURRENT_BINARY_DIR}/xmltest.sh "${_data}" )
    endif ( )
    add_test ( xmltest sh ${CMAKE_CURRENT_BINARY_DIR}/xmltest.sh )
    set_property ( TEST xmltest PROPERTY PASS_REGULAR_EXPRESSION "Failed: 0" )
  endif ( )
endif ( )
