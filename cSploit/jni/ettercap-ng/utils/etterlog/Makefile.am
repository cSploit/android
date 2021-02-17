
# $Id: Makefile.am,v 1.19 2004/11/05 10:01:15 alor Exp $

include $(top_srcdir)/Makefile.am.common

# do not link ettercap libs
LIBS = 

bin_PROGRAMS = etterlog

etterlog_SOURCES = el_analyze.c \
                   el_conn.c \
                   el_decode.c \
                   el_decode_http.c \
                   el_display.c \
                   el_ec_compat.c \
                   el_log.c \
                   el_main.c \
                   el_parser.c \
                   el_profiles.c \
                   el_stream.c \
                   el_target.c \
                   $(top_srcdir)/src/ec_error.c \
                   $(top_srcdir)/src/ec_fingerprint.c \
                   $(top_srcdir)/src/ec_manuf.c \
                   $(top_srcdir)/src/ec_services.c \
                   $(top_srcdir)/src/ec_hash.c \
                   $(top_srcdir)/src/ec_file.c \
                   $(top_srcdir)/src/ec_inet.c \
                   $(top_srcdir)/src/ec_passive.c \
                   $(top_srcdir)/src/ec_strings.c \
                   $(top_srcdir)/src/ec_format.c
                   
if MISSING_STRSEP
   etterlog_SOURCES += $(top_srcdir)/src/missing/strsep.c
endif
if MISSING_STRLCAT
   etterlog_SOURCES += $(top_srcdir)/src/missing/strlcat.c
endif
if MISSING_STRCASESTR
   etterlog_SOURCES += $(top_srcdir)/src/missing/strcasestr.c
endif
if MISSING_MEMMEM
   etterlog_SOURCES += $(top_srcdir)/src/missing/memmem.c
endif
if MISSING_BASENAME
   etterlog_SOURCES += $(top_srcdir)/src/missing/basename.c
endif
if MISSING_INET_ATON
   etterlog_SOURCES += $(top_srcdir)/src/missing/inet_aton.c
endif
if MISSING_GETOPT_LONG
   etterlog_SOURCES += $(top_srcdir)/src/missing/getopt.c
endif

etterlog_CFLAGS = @EC_CFLAGS@

etterlog_LDADD = -lz @ICONVLIB@

if INET_ATON_WANTS_RESOLV
   etterlog_LDADD += -lresolv
endif


CLEANFILES = *.log *.out core* *~

# vim:ts=4:noexpandtab
