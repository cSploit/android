# $Id: Makefile.am,v 1.51 2011-06-08 09:25:53 freddy77 Exp $
TESTS		=	t0001$(EXEEXT) t0002$(EXEEXT) t0003$(EXEEXT)\
			t0004$(EXEEXT) t0005$(EXEEXT) t0006$(EXEEXT)\
			t0007$(EXEEXT) t0008$(EXEEXT) t0009$(EXEEXT)\
			t0011$(EXEEXT) t0012$(EXEEXT) t0013$(EXEEXT)\
			t0014$(EXEEXT) t0015$(EXEEXT) t0016$(EXEEXT)\
			t0017$(EXEEXT) t0018$(EXEEXT) t0019$(EXEEXT)\
			t0020$(EXEEXT) t0021$(EXEEXT) t0022$(EXEEXT)\
			t0023$(EXEEXT) rpc$(EXEEXT) dbmorecmds$(EXEEXT)\
			bcp$(EXEEXT) thread$(EXEEXT) text_buffer$(EXEEXT)\
			done_handling$(EXEEXT) timeout$(EXEEXT) \
			hang$(EXEEXT) null$(EXEEXT) null2$(EXEEXT) \
			setnull$(EXEEXT) numeric$(EXEEXT)
check_PROGRAMS	=	$(TESTS)

SQL_DIST = 	bcp.sql dbmorecmds.sql done_handling.sql rpc.sql \
		t0001.sql t0002.sql t0003.sql t0004.sql t0005.sql t0006.sql t0007.sql t0009.sql \
		t0011.sql t0012.sql t0013.sql t0014.sql t0015.sql t0016.sql t0017.sql t0018.sql \
		t0020.sql t0022.sql t0023.sql text_buffer.sql timeout.sql numeric.sql

noinst_SCRIPTS	= $(SQL_DIST)

t0001_SOURCES	=	t0001.c common.c common.h t0001.sql
t0002_SOURCES	=	t0002.c common.c common.h
t0003_SOURCES	=	t0003.c common.c common.h
t0004_SOURCES	=	t0004.c common.c common.h
t0005_SOURCES	=	t0005.c common.c common.h
t0006_SOURCES	=	t0006.c common.c common.h
t0007_SOURCES	=	t0007.c common.c common.h
t0008_SOURCES	=	t0008.c common.c common.h
t0009_SOURCES	=	t0009.c common.c common.h
t0011_SOURCES	=	t0011.c common.c common.h
t0012_SOURCES	=	t0012.c common.c common.h
t0013_SOURCES	=	t0013.c common.c common.h
t0014_SOURCES	=	t0014.c common.c common.h
t0015_SOURCES	=	t0015.c common.c common.h
t0016_SOURCES	=	t0016.c common.c common.h
t0017_SOURCES	=	t0017.c common.c common.h
t0018_SOURCES	=	t0018.c common.c common.h
t0019_SOURCES	=	t0019.c common.c common.h
t0020_SOURCES	=	t0020.c common.c common.h
t0021_SOURCES	=	t0021.c common.c common.h
t0022_SOURCES	=	t0022.c common.c common.h
t0023_SOURCES	=	t0023.c common.c common.h
rpc_SOURCES	=	rpc.c common.c common.h
dbmorecmds_SOURCES =	dbmorecmds.c common.c common.h
bcp_SOURCES	=	bcp.c bcp.h common.c common.h 
thread_SOURCES	=	thread.c bcp.h common.c common.h
text_buffer_SOURCES =	text_buffer.c common.c common.h
done_handling_SOURCES =	done_handling.c common.c common.h
timeout_SOURCES =	timeout.c common.c common.h
hang_SOURCES	=	hang.c common.c common.h
null_SOURCES	=	null.c common.c common.h
null2_SOURCES	=	null2.c common.c common.h
setnull_SOURCES	=	setnull.c common.c common.h
numeric_SOURCES =	numeric.c common.c common.h

AM_CPPFLAGS	= 	-DFREETDS_TOPDIR=\"$(top_srcdir)\" -I$(top_srcdir)/include
if MINGW32
AM_LDFLAGS	=	-no-fast-install
else
AM_LDFLAGS	=	-no-install -L../.libs -R $(abs_builddir)/../.libs
endif
LIBS		=	../libsybdb.la ../../replacements/libreplacements.la $(LTLIBICONV) @NETWORK_LIBS@
EXTRA_DIST	=	t0016.in t0017.in t0017.in.be data.bin CMakeLists.txt $(SQL_DIST)
CLEANFILES	=	tdsdump.out t0013.out t0014.out t0016.out \
				t0016.err t0017.err t0017.out

$(SQL_DIST): 
	ln -s $(srcdir)/$@ .
