bin_PROGRAMS = isql dltest odbcinst iusql odbc_config slencheck

INCLUDES = -I@top_srcdir@/include $(LTDLINCL) 

EXTRA_DIST = \
	isql.h \
	COPYING

odbcinst_LDADD = \
	../odbcinst/libodbcinst.la \
	../ini/libinilc.la \
	$(LIBLTDL) 

odbcinst_DEPENDENCIES = \
	../odbcinst/libodbcinst.la \
	../ini/libinilc.la \
    $(LTDLDEPS)

odbcinst_SOURCES = odbcinst.c 

isql_LDADD = \
	../DriverManager/libodbc.la \
	../extras/libodbcextraslc.la \
 	$(READLINE)

isql_DEPENDENCIES = \
	../DriverManager/libodbc.la

isql_SOURCES = isql.c

iusql_LDADD = \
	../DriverManager/libodbc.la \
	../odbcinst/libodbcinst.la \
	../ini/libinilc.la \
 	$(READLINE)

iusql_DEPENDENCIES = \
	../odbcinst/libodbcinst.la \
	../DriverManager/libodbc.la \
	../ini/libinilc.la \
	../extras/libodbcextraslc.la

iusql_SOURCES = iusql.c

dltest_SOURCES = dltest.c

odbc_config_SOURCES = odbc-config.c

slencheck_LDADD = \
	../DriverManager/libodbc.la \
 	$(READLINE)

slencheck_DEPENDENCIES = \
	../DriverManager/libodbc.la

slencheck_SOURCES = slencheck.c


dltest_DEPENDENCIES = $(LTDLDEPS)
dltest_LDADD = $(LIBLTDL)
