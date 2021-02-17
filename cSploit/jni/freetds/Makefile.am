# version $Id: Makefile.am,v 1.50 2012-01-16 00:27:04 jklowden Exp $

## SUBDIRS determines in which directories automake will generate a Makefile
## See also AC_OUTPUT in configure.ac

SUBDIRS	=	include src doc samples win32 vms

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = 	interfaces PWD.in BUGS \
		freetds.conf locales.conf \
		autogen.sh freetds.spec.in freetds.spec tds.dox \
		config.rpath Nmakefile FreeTDS.sln CMakeLists.txt


ETC	=	$(DESTDIR)$(sysconfdir)
DOCDIR	=	doc

install-data-local:
	$(mkinstalldirs) $(ETC)
	if test ! -f $(ETC)/freetds.conf; then \
		$(INSTALL) $(srcdir)/freetds.conf $(ETC)/freetds.conf; \
	fi
	if test ! -f $(ETC)/locales.conf; then \
		$(INSTALL) $(srcdir)/locales.conf $(ETC)/locales.conf; \
	fi

clean-local:
	find . \( -name \*.test_output -o -name \*.bb -o -name \*.bbg -o -name \*.da -o -name \*.gc\* \) -exec rm -f {} \;

# enable some cheat so make distcheck work (see below)
DISTCHECK_CONFIGURE_FLAGS	=	\
	--enable-distcheck-build --enable-extra-checks

# do not uninstall configuration files (they may have been changed)
if DISTCHECK_BUILD
uninstall-local:
	rm -f $(ETC)/freetds.conf $(ETC)/locales.conf $(ETC)/pool.conf PWD
endif

test:
	@echo "The 'make test' option has been replaced with 'make check'";

all:
if DISTCHECK_BUILD
# if we are inside a make distcheck copy our real password file
	if test ! -f PWD -a -f $(srcdir)/../PWD; then cp $(srcdir)/../PWD PWD; fi
endif
# this prevent the store of passwords in CVS
	if test ! -f PWD; then cp $(srcdir)/PWD.in PWD; fi

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)-`date +"%Y%m%d"`

## The doxygen configuration file understands environment variables.  
doxy: $(SUBDIRS)
if HAVE_DOXYGEN
	$(MKDIR_P) $(DOCDIR)/reference
	rm -rf $(DOCDIR)/reference/*
	SRCDIR=$(top_srcdir) VERSION=$(VERSION) $(DOXYGEN) $(top_srcdir)/tds.dox
	if test -d $(DOCDIR)/reference/html ; then mv $(DOCDIR)/reference/html/* $(DOCDIR)/reference; rmdir $(DOCDIR)/reference/html; fi
else
	echo 'Doxygen not installed; reference manual not built.'
endif

