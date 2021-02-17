# $Id: Makefile.am,v 1.74 2012-03-06 20:40:12 freddy77 Exp $

SUBDIRS			=	unittests
AM_CPPFLAGS		=	-I$(top_srcdir)/include

noinst_LTLIBRARIES	=	libtds.la
if HAVE_SSPI
AUTH_FILES_DIST	= challenge.c md4.c md5.c des.c gssapi.c hmac_md5.c
AUTH_FILES	= sspi.c
else
AUTH_FILES	= challenge.c md4.c md5.c des.c gssapi.c hmac_md5.c
AUTH_FILES_DIST	= sspi.c
endif
libtds_la_SOURCES=	mem.c token.c util.c login.c read.c \
	write.c convert.c numeric.c config.c query.c iconv.c \
	locale.c threadsafe.c vstrbuild.c \
	tdsstring.c getmac.c data.c net.c \
	tds_checks.c tds_checks.h log.c \
	bulk.c packet.c stream.c \
	$(AUTH_FILES)
libtds_la_LDFLAGS=
libtds_la_LIBADD=

noinst_HEADERS		= tds_willconvert.h encodings.h num_limits.h tds_types.h
EXTRA_DIST		= tds_willconvert.h encodings.h num_limits.h tds_types.h \
	TDS.vcproj CMakeLists.txt $(AUTH_FILES_DIST)

if HAVE_DOXYGEN
doxyfile: $(srcdir)/tds.dox
	cp $(srcdir)/tds.dox doxyfile
	
## The doxygen configuration file understands environment variables.  

doxy: doxyfile  $(libtds_la_SOURCES)
	SRCDIR=$(srcdir); \
	TOP_SRCDIR=$(top_srcdir); \
	export SRCDIR TOP_SRCDIR; \
	$(DOXYGEN) doxyfile
endif

# Perl is needed to build from a CVS checkout.  
# Perl is *not* needed to build from a tarball.  

## We have to build num_limits.h because automake can't infer the
## dependency (on num_limits.pl).  

data.c:	tds_types.h

if HAVE_PERL_SOURCES
BUILT_SOURCES = tds_willconvert.h encodings.h num_limits.h tds_types.h

clean-local: 
	cd $(srcdir) && rm -f $(BUILT_SOURCES)
	
tds_willconvert.h: tds_willconvert.pl Makefile
	perl $(srcdir)/tds_willconvert.pl > $@.tmp
	mv $@.tmp $@

encodings.h: encodings.pl alternative_character_sets.h Makefile
	perl $(srcdir)/encodings.pl $(srcdir) > $@.tmp 2> /dev/null
	mv $@.tmp $@

num_limits.h: num_limits.pl Makefile
	perl $(srcdir)/num_limits.pl > $@.tmp
	mv $@.tmp $@

tds_types.h: types.pl Makefile ../../misc/types.csv
	perl $(srcdir)/types.pl < $(srcdir)/../../misc/types.csv > $@.tmp
	mv $@.tmp $@
endif
