# Converting DocBook to HTML (several small files)
# http://www.freebsd.org/tutorials/docproj-primer/x3132.html#AEN3140

# $Id: Makefile.am,v 1.68 2012-01-17 03:46:35 jklowden Exp $

SHELL = /bin/sh
DOCDIR = .
PRODUCT = FreeTDS
TARGET_DOCDIR = $(DESTDIR)$(datadir)/doc/freetds-$(VERSION)

IMAGES = 	images/important.gif \
		images/note.gif \
		images/tip.gif \
		images/caution.gif \
		images/warning.gif

CALLOUTS =	images/callouts/1.gif \
		images/callouts/2.gif \
		images/callouts/3.gif \
		images/callouts/4.gif \
		images/callouts/5.gif \
		images/callouts/6.gif \
		images/callouts/7.gif \
		images/callouts/8.gif \
		images/callouts/9.gif \
		images/callouts/10.gif


EXTRA_DIST =	api_status.txt bcp.txt cap.txt getting_started.txt \
		policy.txt CodingStyle tds.html  \
		userguide.dsl.in userguide.sgml userguide.css \
		$(man_MANS) \
		$(DOCDIR)/userguide $(DOCDIR)/reference \
		images grep_sample_code tds_ssl.html \
		freebcp.1.in tsql.1.in osql.1.in bsqldb.1.in bsqlodbc.1.in \
		defncopy.1.in datacopy.1.in fisql.1.in freetds.conf.5.in

man_MANS = 	freebcp.1 \
		tsql.1 \
		osql.1 \
		bsqldb.1 \
		bsqlodbc.1 \
		defncopy.1 \
		datacopy.1 \
		fisql.1 \
		freetds.conf.5

noinst_DATA	= $(DOCDIR)/reference/index.html $(DOCDIR)/userguide/index.htm

$(DOCDIR)/userguide: $(DOCDIR)/userguide/index.htm

$(DOCDIR)/reference: $(DOCDIR)/reference/index.html

doc:  $(DOCDIR)/userguide $(DOCDIR)/reference

dist:	man

man:	$(man_MANS)

INSTALLDIRS = 	$(TARGET_DOCDIR)/userguide    \
		$(TARGET_DOCDIR)/reference    \
		$(TARGET_DOCDIR)/images       \
		$(TARGET_DOCDIR)/images/callouts

$(INSTALLDIRS) $(DOCDIR):
	$(MKDIR_P) $@  2>&1

# If we built the documentation ourselves, install that, 
# else if we have access to distributed documentation, install that.  
install-data-local: $(INSTALLDIRS) $(noinst_DATA)
	$(MKDIR_P) $(DOCDIR) 2>&1
	if test -r $(DOCDIR)/userguide ; then d=.; else d="$(srcdir)"; fi; \
	find $$d/$(DOCDIR)/userguide \( -type f -o -type l \) \
		-exec $(INSTALL) {} $(TARGET_DOCDIR)/userguide ';'
	if test -r $(DOCDIR)/reference ; then d=.; else d="$(srcdir)"; fi; \
	find $$d/$(DOCDIR)/reference \( -type f -o -type l \) \
		-exec $(INSTALL) {} $(TARGET_DOCDIR)/reference ';'
	find $(srcdir)/images -name \*.gif \
		-exec  $(INSTALL) {} $(TARGET_DOCDIR)/images/ ';'

uninstall-local:
	rm -rf $(TARGET_DOCDIR)

clean-local:
	rm -rf userguide.dsl doxy.log \
		dblib.api.sgml	\
		ctlib.api.sgml	\
		 odbc.api.sgml   

## In building the UG and reference manual, we confront several
## possible situations:
## 1.  CVS user (or developer) with or without jade/doxygen installed.
## 2.  Distribution user.  The distribution includes pre-built
##     documentation, ready to install.
## Either of these users may choose to build out-of-tree (e.g., in a
## build/ directory).  We want to build the docs if we can, link to
## distributed ones if they exist, else create stub files pointing to
## the website.  
##
## In the case of jade, we build the documentation only if the
## environment variable DOCBOOK_DSL is set.  A user would define
## DOCBOOK_DSL only if he intends to build the UG.  If the user builds
## FreeTDS out of tree, "make install" will put a symlink to the
## distributed UG HTML in the build directory, and install per usual.
##
## In the case of Doxygen, we detect its presence in the configure
## script, and invoke it if found.  If it is not found, we try to link
## the build tree's reference manual HTML to the distribution.  If
## that fails, we install a stub.

DISTRIBUTED_UG_DIR = $(srcdir)/$(DOCDIR)/userguide

UG_STUB = <html> \
	<P>at <a HREF="http://www.freetds.org/userguide/">www.freetds.org</a> \
	</html>

# To make the userguide, export DOCBOOK_DSL to point to docbook.dsl.
$(DOCDIR)/userguide/index.htm: 	$(DOCDIR) userguide.sgml \
				dblib.api.sgml ctlib.api.sgml odbc.api.sgml \
				userguide.dsl.in api_status.txt
	sed -ne's!SYSTEM "docbook.dsl" CDATA!SYSTEM "${DOCBOOK_DSL}" CDATA!; p' \
		$(srcdir)/userguide.dsl.in > .userguide.dsl
	mv .userguide.dsl userguide.dsl
	if test -h userguide; then rm userguide; fi
	$(MKDIR_P) $(DOCDIR)/userguide 2>&1
	touch $(DOCDIR)/userguide/t.htm
	rm -rf $(DOCDIR)/userguide/*
	$(MAKE) dblib.api.sgml ctlib.api.sgml odbc.api.sgml
# if we're not using jade, copy the distribution's userguide
	if test -z "${DOCBOOK_DSL}" ; then \
		if test -r $(DISTRIBUTED_UG_DIR) \
		   -a ! -r $(DOCDIR)/userguide ; then \
			cp -R $(DISTRIBUTED_UG_DIR) $(DOCDIR); \
		fi; \
	fi
# try jade ...
	(cd $(DOCDIR)/userguide && \
	if test -n "${DOCBOOK_DSL}" ; then \
##		The crazy path is due to the above 'cd' command
		SRCDOC=../$(srcdir); \
		if test ! -r userguide.sgml; then \
			ln -s $${SRCDOC}/userguide.sgml userguide.sgml; \
		fi; \
		openjade -b utf-8 -w all -w no-empty -w no-unused-param \
			 -d ../userguide.dsl -t sgml userguide.sgml; \
		if test -h userguide.sgml; then rm userguide.sgml; fi; \
## 		and copy the stylesheet
		cp $${SRCDOC}/userguide.css .; \
	fi)
# if still no joy, create a stub
	@echo pwd is `pwd`
	-ls -ld `dirname $@` $@
	if test ! -r $@ ; then \
		echo '$(UG_STUB)' > .index.htm && mv .index.htm $@; \
	fi
	test -f $(DOCDIR)/userguide/index.htm
	(cd $(DOCDIR)/userguide && \
	if test ! -h index.html ; then ln -s index.htm index.html; fi)

userguide: $(DOCDIR)/userguide/index.htm
reference: $(DOCDIR)/reference/index.html

DISTRIBUTED_REF_DIR = $(srcdir)/$(DOCDIR)/reference

REF_STUB = <html> \
	<P>at <a HREF="http://www.freetds.org/reference/">www.freetds.org</a> \
	</html>

$(DOCDIR)/reference/index.html: $(top_srcdir)/ChangeLog
	if test -h reference; then rm reference; fi
	$(MKDIR_P) $(DOCDIR)/reference 2>&1
	(cd .. && $(MAKE) doxy 2> doc/doxy.log)
# If "make doxy" doesn't create anything, 
# try to copy the distributed manual's HTML
	if test -r $(DISTRIBUTED_REF_DIR)/index.html -a ! $@ ; then \
		cp -R $(DISTRIBUTED_REF_DIR) $(DOCDIR); \
	fi
# If we don't find the source, generate a stub.
	if test ! -r $@ ; then \
		(cd $(DOCDIR)/reference && \
		echo '$(REF_STUB)' 	\
			> .index.html &&  \
		mv .index.html index.html); \
	fi

#
# The following sections generate API implementation reference tables
# as SGML "entities".
# The entities are used a little bit like include files in the UG, e.g.:  
#	<!ENTITY dblibapisgml SYSTEM "../../../dblib.api.sgml">
#
# and "dblibapisgml" is referred to later on:
#	
#	<sect1 id="dblib.api.summary">
#		<title>db-lib API Implementation Summary</title>
#		&dblibapisgml;
#	</sect1>
#
# This lets us use the api_status.txt document in our SGML.
#
API_TABLE_HEADING = \
	<tgroup cols=4><thead><row> \
		<entry>Category</> \
		<entry>Function</> \
		<entry>Status</> \
		<entry>Comments</> \
		</row></thead> 

API_FUNC_COL = <entry>Function</> 
API_DBLIB_FUNC_COLS = <entry>Microsoft name</><entry>Sybase name</>

API_PAGE_FMT = \
	<table id="%s.api"> \
	<title>&%s; API Implementation Summary</title>\n \
	<!-- table heading -->\n \
		%s\n \
	<tbody>\n \
		%s\n \
	</tbody></tgroup></table>\n

dblib.api.body ctlib.api.body odbc.api.body: $(srcdir)/api_status.txt
	NAME=`echo $@ | sed 's/\..*$$//'` && \
	sed -ne "/^$${NAME}[	]/ s!!<row><entry>!p" \
			$(srcdir)/api_status.txt \
		| sed 	-e's![	][	]*!</> <entry>!g' \
			-e's!$$!</></row>!'  > $@~
	mv $@~ $@

dblib.api.sgml: dblib.api.body
ctlib.api.sgml: ctlib.api.body
odbc.api.sgml: odbc.api.body

dblib.api.sgml ctlib.api.sgml odbc.api.sgml: 
	NAME=`echo $@ | sed 's/\..*$$//'` && \
	HEAD='$(API_TABLE_HEADING)' && \
	if [ $${NAME} = dblib ]; then \
		HEAD=`echo '$(API_TABLE_HEADING)' \
			| sed 's!$(API_FUNC_COL)!$(API_DBLIB_FUNC_COLS)!'`; \
	fi && \
	BODY=`cat $(@:.sgml=.body)` && \
	printf '$(API_PAGE_FMT)' $${NAME} $${NAME} "$${HEAD}" "$${BODY}"  > $@~
	mv $@~ $@

CLEANFILES = dblib.api.body ctlib.api.body odbc.api.body dblib.api.sgml ctlib.api.sgml odbc.api.sgml $(man_MANS)

distclean-local:
	rm -rf $(DOCDIR)/userguide $(DOCDIR)/reference

