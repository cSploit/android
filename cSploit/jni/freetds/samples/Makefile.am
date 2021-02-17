# version $Id: Makefile.am,v 1.3 2006-09-13 11:56:37 freddy77 Exp $

noinst_SCRIPTS	=	unixodbc.freetds.driver.template

EXTRA_DIST	=	README debug.c dyntest.c odbc.ini odbctest.php \
			odbctest.pl test.php test.pl \
			unixodbc.freetds.driver.template.in \
			unixodbc.install.sh \
			unixodbc.jdbc.datasource.template

SUFFIXES	=	.in

edit = sed \
	-e 's,@datadir\@,$(pkgdatadir),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@prefix\@,$(prefix),g'

unixodbc.freetds.driver.template: unixodbc.freetds.driver.template.in

CLEANFILES = unixodbc.freetds.driver.template

.in:
	rm -f $@ $@.tmp
	$(edit) $< >$@.tmp
	mv $@.tmp $@
