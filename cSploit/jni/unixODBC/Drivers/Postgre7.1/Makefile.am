lib_LTLIBRARIES = libodbcpsql.la

INCLUDES = -I@top_srcdir@/include $(LTDLINCL)

libodbcpsql_la_LDFLAGS = \
	-version-info 2:0:0 \
	-no-undefined \
	$(LIBSOCKET) $(LIBNSL) \
	-export-dynamic \
	-export-symbols @srcdir@/driver.exp \
	-module

EXTRA_DIST = \
	bind.h \
	columninfo.h \
	connection.h \
	convert.h \
	dlg_specific.h \
	environ.h \
	isql.h \
	isqlext.h \
	lobj.h \
	md5.h \
	misc.h \
	pgtypes.h \
	psqlodbc.h \
	qresult.h \
	resource.h \
	socket.h \
	statement.h \
	tuple.h \
	tuplelist.h \
	notice.txt \
	driver.exp

libodbcpsql_la_LIBADD = \
	../../lst/liblstlc.la \
	../../log/libloglc.la \
	../../ini/libinilc.la \
	../../odbcinst/libodbcinstlc.la \
	$(LIBLTDL) \
    $(LIBADD_CRYPT) $(LIBADD_POW)

libodbcpsql_la_DEPENDENCIES = \
	../../lst/liblstlc.la \
	../../log/libloglc.la \
	../../ini/libinilc.la \
	../../odbcinst/libodbcinstlc.la \
    ../../extras/libodbcextraslc.la \
    $(LTDLDEPS)

libodbcpsql_la_SOURCES = \
	bind.c \
	columninfo.c \
	connection.c \
	convert.c \
	dlg_specific.c \
	drvconn.c \
	environ.c \
	execute.c \
	info.c \
	lobj.c \
	md5.c \
	misc.c \
	options.c \
	parse.c \
	pgtypes.c \
	psqlodbc.c \
	qresult.c \
	results.c \
	socket.c \
	statement.c \
	tuple.c \
	tuplelist.c
