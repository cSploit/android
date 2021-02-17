# $Id: Makefile.am,v 1.33 2011-06-08 09:25:53 freddy77 Exp $

TESTS		=	t0001$(EXEEXT) t0002$(EXEEXT) t0003$(EXEEXT) t0004$(EXEEXT) \
			t0005$(EXEEXT) t0006$(EXEEXT) t0007$(EXEEXT) t0008$(EXEEXT) \
			t0009$(EXEEXT) connect_fail$(EXEEXT) ct_options$(EXEEXT) \
			lang_ct_param$(EXEEXT) array_bind$(EXEEXT) cs_diag$(EXEEXT) \
			get_send_data$(EXEEXT) rpc_ct_param$(EXEEXT) rpc_ct_setparam$(EXEEXT) \
			ct_diagclient$(EXEEXT) ct_diagserver$(EXEEXT) ct_diagall$(EXEEXT) \
			cs_config$(EXEEXT) cancel$(EXEEXT) blk_in$(EXEEXT) \
			blk_out$(EXEEXT) ct_cursor$(EXEEXT) ct_cursors$(EXEEXT) \
			ct_dynamic$(EXEEXT) blk_in2$(EXEEXT) datafmt$(EXEEXT)

check_PROGRAMS	=	$(TESTS)

t0001_SOURCES		= t0001.c common.c common.h
t0002_SOURCES		= t0002.c common.c common.h
t0003_SOURCES		= t0003.c common.c common.h
t0004_SOURCES		= t0004.c common.c common.h
t0005_SOURCES		= t0005.c common.c common.h
t0006_SOURCES		= t0006.c
t0007_SOURCES		= t0007.c common.c common.h
t0008_SOURCES		= t0008.c common.c common.h
t0009_SOURCES		= t0009.c common.c common.h
connect_fail_SOURCES	= connect_fail.c common.c common.h
ct_options_SOURCES	= ct_options.c common.c common.h
lang_ct_param_SOURCES	= lang_ct_param.c common.c common.h
array_bind_SOURCES	= array_bind.c common.c common.h
cs_diag_SOURCES		= cs_diag.c common.c common.h
get_send_data_SOURCES	= get_send_data.c common.c common.h
rpc_ct_param_SOURCES	= rpc_ct_param.c common.c common.h
rpc_ct_setparam_SOURCES	= rpc_ct_setparam.c common.c common.h
ct_diagclient_SOURCES	= ct_diagclient.c common.c common.h
ct_diagserver_SOURCES	= ct_diagserver.c common.c common.h
ct_diagall_SOURCES	= ct_diagall.c common.c common.h
cs_config_SOURCES	= cs_config.c common.c common.h
cancel_SOURCES		= cancel.c common.c common.h
blk_in_SOURCES		= blk_in.h blk_in.c common.c common.h
blk_out_SOURCES		= blk_out.c common.c common.h
ct_cursor_SOURCES	= ct_cursor.c common.c common.h
ct_cursors_SOURCES	= ct_cursors.c common.c common.h
ct_dynamic_SOURCES	= ct_dynamic.c common.c common.h
blk_in2_SOURCES		= blk_in2.c common.c common.h
datafmt_SOURCES		= datafmt.c common.c common.h

AM_CPPFLAGS	=	-I$(top_srcdir)/include
if MINGW32
AM_LDFLAGS	=	-no-fast-install
else
AM_LDFLAGS	=	-no-install -L../.libs -R $(abs_builddir)/../.libs
endif
LIBS		=	../libct.la ../../replacements/libreplacements.la $(LTLIBICONV) @NETWORK_LIBS@
CLEANFILES	=	tdsdump.out
EXTRA_DIST	=	CMakeLists.txt
