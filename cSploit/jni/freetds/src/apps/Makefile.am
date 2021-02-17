# $Id: Makefile.am,v 1.31 2011-06-08 09:25:51 freddy77 Exp $

AM_CPPFLAGS	= -I$(top_srcdir)/include 

SUBDIRS		= fisql

DIST_SUBDIRS	= $(SUBDIRS)

# build bsqlodbc only if the ODBC library was to be built
if ODBC
bin_PROGRAMS	= tsql freebcp bsqldb defncopy datacopy bsqlodbc
else
bin_PROGRAMS	= tsql freebcp bsqldb defncopy datacopy
endif


dist_bin_SCRIPTS = osql

freebcp_LDADD	= ../dblib/libsybdb.la ../replacements/libreplacements.la $(LTLIBICONV) $(NETWORK_LIBS)
freebcp_SOURCES = freebcp.c freebcp.h

tsql_LDADD	= ../tds/libtds.la \
		  ../replacements/libreplacements.la \
		  $(NETWORK_LIBS) $(LTLIBICONV) $(FREETDS_LIBGCC) $(READLINE_LIBS)

bsqldb_LDADD	= ../dblib/libsybdb.la \
		  ../replacements/libreplacements.la \
		  $(NETWORK_LIBS) $(LTLIBICONV)

if ODBC
bsqlodbc_CPPFLAGS	= $(ODBC_INC) $(AM_CPPFLAGS)
bsqlodbc_LDADD	= $(ODBCLIB) $(ODBCNODMLIBAPP) \
		  ../replacements/libreplacements.la $(NETWORK_LIBS) $(LTLIBICONV)
endif

defncopy_LDADD	= ../dblib/libsybdb.la \
		  ../replacements/libreplacements.la \
		  $(NETWORK_LIBS) $(LTLIBICONV)

datacopy_SOURCES= datacopy.c
datacopy_LDADD	= ../dblib/libsybdb.la \
		  ../replacements/libreplacements.la \
		$(NETWORK_LIBS) $(LTLIBICONV)

EXTRA_DIST = CMakeLists.txt
