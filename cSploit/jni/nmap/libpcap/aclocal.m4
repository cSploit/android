dnl @(#) $Header: /tcpdump/master/libpcap/aclocal.m4,v 1.93 2008-11-18 07:29:48 guy Exp $ (LBL)
dnl
dnl Copyright (c) 1995, 1996, 1997, 1998
dnl	The Regents of the University of California.  All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that: (1) source code distributions
dnl retain the above copyright notice and this paragraph in its entirety, (2)
dnl distributions including binary code include the above copyright notice and
dnl this paragraph in its entirety in the documentation or other materials
dnl provided with the distribution, and (3) all advertising materials mentioning
dnl features or use of this software display the following acknowledgement:
dnl ``This product includes software developed by the University of California,
dnl Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
dnl the University nor the names of its contributors may be used to endorse
dnl or promote products derived from this software without specific prior
dnl written permission.
dnl THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
dnl WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
dnl MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
dnl
dnl LBL autoconf macros
dnl

dnl
dnl Do whatever AC_LBL_C_INIT work is necessary before using AC_PROG_CC.
dnl
dnl It appears that newer versions of autoconf (2.64 and later) will,
dnl if you use AC_TRY_COMPILE in a macro, stick AC_PROG_CC at the
dnl beginning of the macro, even if the macro itself calls AC_PROG_CC.
dnl See the "Prerequisite Macros" and "Expanded Before Required" sections
dnl in the Autoconf documentation.
dnl
dnl This causes a steaming heap of fail in our case, as we were, in
dnl AC_LBL_C_INIT, doing the tests we now do in AC_LBL_C_INIT_BEFORE_CC,
dnl calling AC_PROG_CC, and then doing the tests we now do in
dnl AC_LBL_C_INIT.  Now, we run AC_LBL_C_INIT_BEFORE_CC, AC_PROG_CC,
dnl and AC_LBL_C_INIT at the top level.
dnl
AC_DEFUN(AC_LBL_C_INIT_BEFORE_CC,
    [AC_PREREQ(2.50)
    AC_BEFORE([$0], [AC_LBL_C_INIT])
    AC_BEFORE([$0], [AC_PROG_CC])
    AC_BEFORE([$0], [AC_LBL_FIXINCLUDES])
    AC_BEFORE([$0], [AC_LBL_DEVEL])
    AC_ARG_WITH(gcc, [  --without-gcc           don't use gcc])
    $1="-O"
    $2=""
    if test "${srcdir}" != "." ; then
	    $2="-I\$(srcdir)"
    fi
    if test "${CFLAGS+set}" = set; then
	    LBL_CFLAGS="$CFLAGS"
    fi
    if test -z "$CC" ; then
	    case "$host_os" in

	    bsdi*)
		    AC_CHECK_PROG(SHLICC2, shlicc2, yes, no)
		    if test $SHLICC2 = yes ; then
			    CC=shlicc2
			    export CC
		    fi
		    ;;
	    esac
    fi
    if test -z "$CC" -a "$with_gcc" = no ; then
	    CC=cc
	    export CC
    fi
])

dnl
dnl Determine which compiler we're using (cc or gcc)
dnl If using gcc, determine the version number
dnl If using cc, require that it support ansi prototypes
dnl If using gcc, use -O2 (otherwise use -O)
dnl If using cc, explicitly specify /usr/local/include
dnl
dnl usage:
dnl
dnl	AC_LBL_C_INIT(copt, incls)
dnl
dnl results:
dnl
dnl	$1 (copt set)
dnl	$2 (incls set)
dnl	CC
dnl	LDFLAGS
dnl	ac_cv_lbl_gcc_vers
dnl	LBL_CFLAGS
dnl
AC_DEFUN(AC_LBL_C_INIT,
    [AC_PREREQ(2.50)
    AC_BEFORE([$0], [AC_LBL_FIXINCLUDES])
    AC_BEFORE([$0], [AC_LBL_DEVEL])
    AC_BEFORE([$0], [AC_LBL_SHLIBS_INIT])
    if test "$GCC" = yes ; then
	    if test "$SHLICC2" = yes ; then
		    ac_cv_lbl_gcc_vers=2
		    $1="-O2"
	    else
		    AC_MSG_CHECKING(gcc version)
		    AC_CACHE_VAL(ac_cv_lbl_gcc_vers,
			ac_cv_lbl_gcc_vers=`$CC -v 2>&1 | \
			    sed -e '/^gcc version /!d' \
				-e 's/^gcc version //' \
				-e 's/ .*//' -e 's/^[[[^0-9]]]*//' \
				-e 's/\..*//'`)
		    AC_MSG_RESULT($ac_cv_lbl_gcc_vers)
		    if test $ac_cv_lbl_gcc_vers -gt 1 ; then
			    $1="-O2"
		    fi
	    fi
    else
	    AC_MSG_CHECKING(that $CC handles ansi prototypes)
	    AC_CACHE_VAL(ac_cv_lbl_cc_ansi_prototypes,
		AC_TRY_COMPILE(
		    [#include <sys/types.h>],
		    [int frob(int, char *)],
		    ac_cv_lbl_cc_ansi_prototypes=yes,
		    ac_cv_lbl_cc_ansi_prototypes=no))
	    AC_MSG_RESULT($ac_cv_lbl_cc_ansi_prototypes)
	    if test $ac_cv_lbl_cc_ansi_prototypes = no ; then
		    case "$host_os" in

		    hpux*)
			    AC_MSG_CHECKING(for HP-UX ansi compiler ($CC -Aa -D_HPUX_SOURCE))
			    savedcflags="$CFLAGS"
			    CFLAGS="-Aa -D_HPUX_SOURCE $CFLAGS"
			    AC_CACHE_VAL(ac_cv_lbl_cc_hpux_cc_aa,
				AC_TRY_COMPILE(
				    [#include <sys/types.h>],
				    [int frob(int, char *)],
				    ac_cv_lbl_cc_hpux_cc_aa=yes,
				    ac_cv_lbl_cc_hpux_cc_aa=no))
			    AC_MSG_RESULT($ac_cv_lbl_cc_hpux_cc_aa)
			    if test $ac_cv_lbl_cc_hpux_cc_aa = no ; then
				    AC_MSG_ERROR(see the INSTALL doc for more info)
			    fi
			    CFLAGS="$savedcflags"
			    $1="-Aa $$1"
			    AC_DEFINE(_HPUX_SOURCE,1,[needed on HP-UX])
			    ;;

		    osf*)
			    AC_MSG_CHECKING(for ansi mode in DEC compiler ($CC -std1))
			    savedcflags="$CFLAGS"
			    CFLAGS="-std1"
			    AC_CACHE_VAL(ac_cv_lbl_cc_osf1_cc_std1,
				AC_TRY_COMPILE(
				    [#include <sys/types.h>],
				    [int frob(int, char *)],
				    ac_cv_lbl_cc_osf1_cc_std1=yes,
				    ac_cv_lbl_cc_osf1_cc_std1=no))
			    AC_MSG_RESULT($ac_cv_lbl_cc_osf1_cc_std1)
			    if test $ac_cv_lbl_cc_osf1_cc_std1 = no ; then
				    AC_MSG_ERROR(see the INSTALL doc for more info)
			    fi
			    CFLAGS="$savedcflags"
			    $1="-std1 $$1"
			    ;;

		    *)
			    AC_MSG_ERROR(see the INSTALL doc for more info)
			    ;;
		    esac
	    fi
	    $2="$$2 -I/usr/local/include"
	    LDFLAGS="$LDFLAGS -L/usr/local/lib"

	    case "$host_os" in

	    irix*)
		    $1="$$1 -xansi -signed -g3"
		    ;;

	    osf*)
	    	    #
		    # Presumed to be DEC OSF/1, Digital UNIX, or
		    # Tru64 UNIX.
		    #
		    $1="$$1 -g3"
		    ;;

	    ultrix*)
		    AC_MSG_CHECKING(that Ultrix $CC hacks const in prototypes)
		    AC_CACHE_VAL(ac_cv_lbl_cc_const_proto,
			AC_TRY_COMPILE(
			    [#include <sys/types.h>],
			    [struct a { int b; };
			    void c(const struct a *)],
			    ac_cv_lbl_cc_const_proto=yes,
			    ac_cv_lbl_cc_const_proto=no))
		    AC_MSG_RESULT($ac_cv_lbl_cc_const_proto)
		    if test $ac_cv_lbl_cc_const_proto = no ; then
			    AC_DEFINE(const,[],
			        [to handle Ultrix compilers that don't support const in prototypes])
		    fi
		    ;;
	    esac
    fi
])

dnl
dnl Determine what options are needed to build a shared library
dnl
dnl usage:
dnl
dnl	AC_LBL_SHLIBS_INIT
dnl
dnl results:
dnl
dnl	V_CCOPT (modified to build position-independent code)
dnl	V_SHLIB_CMD
dnl	V_SHLIB_OPT
dnl	V_SONAME_OPT
dnl	V_RPATH_OPT
dnl
AC_DEFUN(AC_LBL_SHLIBS_INIT,
    [AC_PREREQ(2.50)
    if test "$GCC" = yes ; then
	    #
	    # On platforms where we build a shared library:
	    #
	    #	add options to generate position-independent code,
	    #	if necessary (it's the default in AIX and Darwin/OS X);
	    #
	    #	define option to set the soname of the shared library,
	    #	if the OS supports that;
	    #
	    #	add options to specify, at link time, a directory to
	    #	add to the run-time search path, if that's necessary.
	    #
	    V_SHLIB_CMD="\$(CC)"
	    V_SHLIB_OPT="-shared"
	    case "$host_os" in

	    aix*)
		    ;;

	    freebsd*|netbsd*|openbsd*|dragonfly*|linux*|osf*)
	    	    #
		    # Platforms where the linker is the GNU linker
		    # or accepts command-line arguments like
		    # those the GNU linker accepts.
		    #
		    # Some instruction sets require -fPIC on some
		    # operating systems.  Check for them.  If you
		    # have a combination that requires it, add it
		    # here.
		    #
		    PIC_OPT=-fpic
		    case "$host_cpu" in

		    sparc64*)
			case "$host_os" in

			freebsd*)
			    PIC_OPT=-fPIC
			    ;;
			esac
			;;
		    esac
		    V_CCOPT="$V_CCOPT $PIC_OPT"
		    V_SONAME_OPT="-Wl,-soname,"
		    V_RPATH_OPT="-Wl,-rpath,"
		    ;;

	    hpux*)
		    V_CCOPT="$V_CCOPT -fpic"
	    	    #
		    # XXX - this assumes GCC is using the HP linker,
		    # rather than the GNU linker, and that the "+h"
		    # option is used on all HP-UX platforms, both .sl
		    # and .so.
		    #
		    V_SONAME_OPT="-Wl,+h,"
		    #
		    # By default, directories specifed with -L
		    # are added to the run-time search path, so
		    # we don't add them in pcap-config.
		    #
		    ;;

	    solaris*)
		    V_CCOPT="$V_CCOPT -fpic"
		    #
		    # XXX - this assumes GCC is using the Sun linker,
		    # rather than the GNU linker.
		    #
		    V_SONAME_OPT="-Wl,-h,"
		    V_RPATH_OPT="-Wl,-R,"
		    ;;
	    esac
    else
	    #
	    # Set the appropriate compiler flags and, on platforms
	    # where we build a shared library:
	    #
	    #	add options to generate position-independent code,
	    #	if necessary (it's the default in Darwin/OS X);
	    #
	    #	if we generate ".so" shared libraries, define the
	    #	appropriate options for building the shared library;
	    #
	    #	add options to specify, at link time, a directory to
	    #	add to the run-time search path, if that's necessary.
	    #
	    # Note: spaces after V_SONAME_OPT are significant; on
	    # some platforms the soname is passed with a GCC-like
	    # "-Wl,-soname,{soname}" option, with the soname part
	    # of the option, while on other platforms the C compiler
	    # driver takes it as a regular option with the soname
	    # following the option.  The same applies to V_RPATH_OPT.
	    #
	    case "$host_os" in

	    aix*)
		    V_SHLIB_CMD="\$(CC)"
		    V_SHLIB_OPT="-G -bnoentry -bexpall"
		    ;;

	    freebsd*|netbsd*|openbsd*|dragonfly*|linux*)
		    #
		    # "cc" is GCC.
		    #
		    V_CCOPT="$V_CCOPT -fpic"
		    V_SHLIB_CMD="\$(CC)"
		    V_SHLIB_OPT="-shared"
		    V_SONAME_OPT="-Wl,-soname,"
		    V_RPATH_OPT="-Wl,-rpath,"
		    ;;

	    hpux*)
		    V_CCOPT="$V_CCOPT +z"
		    V_SHLIB_CMD="\$(LD)"
		    V_SHLIB_OPT="-b"
		    V_SONAME_OPT="+h "
		    #
		    # By default, directories specifed with -L
		    # are added to the run-time search path, so
		    # we don't add them in pcap-config.
		    #
		    ;;

	    osf*)
	    	    #
		    # Presumed to be DEC OSF/1, Digital UNIX, or
		    # Tru64 UNIX.
		    #
		    V_SHLIB_CMD="\$(CC)"
		    V_SHLIB_OPT="-shared"
		    V_SONAME_OPT="-soname "
		    V_RPATH_OPT="-rpath "
		    ;;

	    solaris*)
		    V_CCOPT="$V_CCOPT -Kpic"
		    V_SHLIB_CMD="\$(CC)"
		    V_SHLIB_OPT="-G"
		    V_SONAME_OPT="-h "
		    V_RPATH_OPT="-R"
		    ;;
	    esac
    fi
])

#
# Try compiling a sample of the type of code that appears in
# gencode.c with "inline", "__inline__", and "__inline".
#
# Autoconf's AC_C_INLINE, at least in autoconf 2.13, isn't good enough,
# as it just tests whether a function returning "int" can be inlined;
# at least some versions of HP's C compiler can inline that, but can't
# inline a function that returns a struct pointer.
#
# Make sure we use the V_CCOPT flags, because some of those might
# disable inlining.
#
AC_DEFUN(AC_LBL_C_INLINE,
    [AC_MSG_CHECKING(for inline)
    save_CFLAGS="$CFLAGS"
    CFLAGS="$V_CCOPT"
    AC_CACHE_VAL(ac_cv_lbl_inline, [
	ac_cv_lbl_inline=""
	ac_lbl_cc_inline=no
	for ac_lbl_inline in inline __inline__ __inline
	do
	    AC_TRY_COMPILE(
		[#define inline $ac_lbl_inline
		static inline struct iltest *foo(void);
		struct iltest {
		    int iltest1;
		    int iltest2;
		};

		static inline struct iltest *
		foo()
		{
		    static struct iltest xxx;

		    return &xxx;
		}],,ac_lbl_cc_inline=yes,)
	    if test "$ac_lbl_cc_inline" = yes ; then
		break;
	    fi
	done
	if test "$ac_lbl_cc_inline" = yes ; then
	    ac_cv_lbl_inline=$ac_lbl_inline
	fi])
    CFLAGS="$save_CFLAGS"
    if test ! -z "$ac_cv_lbl_inline" ; then
	AC_MSG_RESULT($ac_cv_lbl_inline)
    else
	AC_MSG_RESULT(no)
    fi
    AC_DEFINE_UNQUOTED(inline, $ac_cv_lbl_inline, [Define as token for inline if inlining supported])])

dnl
dnl If using gcc, make sure we have ANSI ioctl definitions
dnl
dnl usage:
dnl
dnl	AC_LBL_FIXINCLUDES
dnl
AC_DEFUN(AC_LBL_FIXINCLUDES,
    [if test "$GCC" = yes ; then
	    AC_MSG_CHECKING(for ANSI ioctl definitions)
	    AC_CACHE_VAL(ac_cv_lbl_gcc_fixincludes,
		AC_TRY_COMPILE(
		    [/*
		     * This generates a "duplicate case value" when fixincludes
		     * has not be run.
		     */
#		include <sys/types.h>
#		include <sys/time.h>
#		include <sys/ioctl.h>
#		ifdef HAVE_SYS_IOCCOM_H
#		include <sys/ioccom.h>
#		endif],
		    [switch (0) {
		    case _IO('A', 1):;
		    case _IO('B', 1):;
		    }],
		    ac_cv_lbl_gcc_fixincludes=yes,
		    ac_cv_lbl_gcc_fixincludes=no))
	    AC_MSG_RESULT($ac_cv_lbl_gcc_fixincludes)
	    if test $ac_cv_lbl_gcc_fixincludes = no ; then
		    # Don't cache failure
		    unset ac_cv_lbl_gcc_fixincludes
		    AC_MSG_ERROR(see the INSTALL for more info)
	    fi
    fi])

dnl
dnl Check for flex, default to lex
dnl Require flex 2.4 or higher
dnl Check for bison, default to yacc
dnl Default to lex/yacc if both flex and bison are not available
dnl Define the yy prefix string if using flex and bison
dnl
dnl usage:
dnl
dnl	AC_LBL_LEX_AND_YACC(lex, yacc, yyprefix)
dnl
dnl results:
dnl
dnl	$1 (lex set)
dnl	$2 (yacc appended)
dnl	$3 (optional flex and bison -P prefix)
dnl
AC_DEFUN(AC_LBL_LEX_AND_YACC,
    [AC_ARG_WITH(flex, [  --without-flex          don't use flex])
    AC_ARG_WITH(bison, [  --without-bison         don't use bison])
    if test "$with_flex" = no ; then
	    $1=lex
    else
	    AC_CHECK_PROGS($1, flex, lex)
    fi
    if test "$$1" = flex ; then
	    # The -V flag was added in 2.4
	    AC_MSG_CHECKING(for flex 2.4 or higher)
	    AC_CACHE_VAL(ac_cv_lbl_flex_v24,
		if flex -V >/dev/null 2>&1; then
			ac_cv_lbl_flex_v24=yes
		else
			ac_cv_lbl_flex_v24=no
		fi)
	    AC_MSG_RESULT($ac_cv_lbl_flex_v24)
	    if test $ac_cv_lbl_flex_v24 = no ; then
		    s="2.4 or higher required"
		    AC_MSG_WARN(ignoring obsolete flex executable ($s))
		    $1=lex
	    fi
    fi
    if test "$with_bison" = no ; then
	    $2=yacc
    else
	    AC_CHECK_PROGS($2, bison, yacc)
    fi
    if test "$$2" = bison ; then
	    $2="$$2 -y"
    fi
    if test "$$1" != lex -a "$$2" = yacc -o "$$1" = lex -a "$$2" != yacc ; then
	    AC_MSG_WARN(don't have both flex and bison; reverting to lex/yacc)
	    $1=lex
	    $2=yacc
    fi
    if test "$$1" = flex -a -n "$3" ; then
	    $1="$$1 -P$3"
	    $2="$$2 -p $3"
    fi])

dnl
dnl Checks to see if union wait is used with WEXITSTATUS()
dnl
dnl usage:
dnl
dnl	AC_LBL_UNION_WAIT
dnl
dnl results:
dnl
dnl	DECLWAITSTATUS (defined)
dnl
AC_DEFUN(AC_LBL_UNION_WAIT,
    [AC_MSG_CHECKING(if union wait is used)
    AC_CACHE_VAL(ac_cv_lbl_union_wait,
	AC_TRY_COMPILE([
#	include <sys/types.h>
#	include <sys/wait.h>],
	    [int status;
	    u_int i = WEXITSTATUS(status);
	    u_int j = waitpid(0, &status, 0);],
	    ac_cv_lbl_union_wait=no,
	    ac_cv_lbl_union_wait=yes))
    AC_MSG_RESULT($ac_cv_lbl_union_wait)
    if test $ac_cv_lbl_union_wait = yes ; then
	    AC_DEFINE(DECLWAITSTATUS,union wait,[type for wait])
    else
	    AC_DEFINE(DECLWAITSTATUS,int,[type for wait])
    fi])

dnl
dnl Checks to see if the sockaddr struct has the 4.4 BSD sa_len member
dnl
dnl usage:
dnl
dnl	AC_LBL_SOCKADDR_SA_LEN
dnl
dnl results:
dnl
dnl	HAVE_SOCKADDR_SA_LEN (defined)
dnl
AC_DEFUN(AC_LBL_SOCKADDR_SA_LEN,
    [AC_MSG_CHECKING(if sockaddr struct has the sa_len member)
    AC_CACHE_VAL(ac_cv_lbl_sockaddr_has_sa_len,
	AC_TRY_COMPILE([
#	include <sys/types.h>
#	include <sys/socket.h>],
	[u_int i = sizeof(((struct sockaddr *)0)->sa_len)],
	ac_cv_lbl_sockaddr_has_sa_len=yes,
	ac_cv_lbl_sockaddr_has_sa_len=no))
    AC_MSG_RESULT($ac_cv_lbl_sockaddr_has_sa_len)
    if test $ac_cv_lbl_sockaddr_has_sa_len = yes ; then
	    AC_DEFINE(HAVE_SOCKADDR_SA_LEN,1,[if struct sockaddr has the sa_len member])
    fi])

dnl
dnl Checks to see if there's a sockaddr_storage structure
dnl
dnl usage:
dnl
dnl	AC_LBL_SOCKADDR_STORAGE
dnl
dnl results:
dnl
dnl	HAVE_SOCKADDR_STORAGE (defined)
dnl
AC_DEFUN(AC_LBL_SOCKADDR_STORAGE,
    [AC_MSG_CHECKING(if sockaddr_storage struct exists)
    AC_CACHE_VAL(ac_cv_lbl_has_sockaddr_storage,
	AC_TRY_COMPILE([
#	include <sys/types.h>
#	include <sys/socket.h>],
	[u_int i = sizeof (struct sockaddr_storage)],
	ac_cv_lbl_has_sockaddr_storage=yes,
	ac_cv_lbl_has_sockaddr_storage=no))
    AC_MSG_RESULT($ac_cv_lbl_has_sockaddr_storage)
    if test $ac_cv_lbl_has_sockaddr_storage = yes ; then
	    AC_DEFINE(HAVE_SOCKADDR_STORAGE,1,[if struct sockaddr_storage exists])
    fi])

dnl
dnl Checks to see if the dl_hp_ppa_info_t struct has the HP-UX 11.00
dnl dl_module_id_1 member
dnl
dnl usage:
dnl
dnl	AC_LBL_HP_PPA_INFO_T_DL_MODULE_ID_1
dnl
dnl results:
dnl
dnl	HAVE_HP_PPA_INFO_T_DL_MODULE_ID_1 (defined)
dnl
dnl NOTE: any compile failure means we conclude that it doesn't have
dnl that member, so if we don't have DLPI, don't have a <sys/dlpi_ext.h>
dnl header, or have one that doesn't declare a dl_hp_ppa_info_t type,
dnl we conclude it doesn't have that member (which is OK, as either we
dnl won't be using code that would use that member, or we wouldn't
dnl compile in any case).
dnl
AC_DEFUN(AC_LBL_HP_PPA_INFO_T_DL_MODULE_ID_1,
    [AC_MSG_CHECKING(if dl_hp_ppa_info_t struct has dl_module_id_1 member)
    AC_CACHE_VAL(ac_cv_lbl_dl_hp_ppa_info_t_has_dl_module_id_1,
	AC_TRY_COMPILE([
#	include <sys/types.h>
#	include <sys/dlpi.h>
#	include <sys/dlpi_ext.h>],
	[u_int i = sizeof(((dl_hp_ppa_info_t *)0)->dl_module_id_1)],
	ac_cv_lbl_dl_hp_ppa_info_t_has_dl_module_id_1=yes,
	ac_cv_lbl_dl_hp_ppa_info_t_has_dl_module_id_1=no))
    AC_MSG_RESULT($ac_cv_lbl_dl_hp_ppa_info_t_has_dl_module_id_1)
    if test $ac_cv_lbl_dl_hp_ppa_info_t_has_dl_module_id_1 = yes ; then
	    AC_DEFINE(HAVE_HP_PPA_INFO_T_DL_MODULE_ID_1,1,[if ppa_info_t_dl_module_id exists])
    fi])

dnl
dnl Checks to see if -R is used
dnl
dnl usage:
dnl
dnl	AC_LBL_HAVE_RUN_PATH
dnl
dnl results:
dnl
dnl	ac_cv_lbl_have_run_path (yes or no)
dnl
AC_DEFUN(AC_LBL_HAVE_RUN_PATH,
    [AC_MSG_CHECKING(for ${CC-cc} -R)
    AC_CACHE_VAL(ac_cv_lbl_have_run_path,
	[echo 'main(){}' > conftest.c
	${CC-cc} -o conftest conftest.c -R/a1/b2/c3 >conftest.out 2>&1
	if test ! -s conftest.out ; then
		ac_cv_lbl_have_run_path=yes
	else
		ac_cv_lbl_have_run_path=no
	fi
	rm -f -r conftest*])
    AC_MSG_RESULT($ac_cv_lbl_have_run_path)
    ])

dnl
dnl Checks to see if unaligned memory accesses fail
dnl
dnl usage:
dnl
dnl	AC_LBL_UNALIGNED_ACCESS
dnl
dnl results:
dnl
dnl	LBL_ALIGN (DEFINED)
dnl
AC_DEFUN(AC_LBL_UNALIGNED_ACCESS,
    [AC_MSG_CHECKING(if unaligned accesses fail)
    AC_CACHE_VAL(ac_cv_lbl_unaligned_fail,
	[case "$host_cpu" in

	#
	# These are CPU types where:
	#
	#	the CPU faults on an unaligned access, but at least some
	#	OSes that support that CPU catch the fault and simulate
	#	the unaligned access (e.g., Alpha/{Digital,Tru64} UNIX) -
	#	the simulation is slow, so we don't want to use it;
	#
	#	the CPU, I infer (from the old
	#
	# XXX: should also check that they don't do weird things (like on arm)
	#
	#	comment) doesn't fault on unaligned accesses, but doesn't
	#	do a normal unaligned fetch, either (e.g., presumably, ARM);
	#
	#	for whatever reason, the test program doesn't work
	#	(this has been claimed to be the case for several of those
	#	CPUs - I don't know what the problem is; the problem
	#	was reported as "the test program dumps core" for SuperH,
	#	but that's what the test program is *supposed* to do -
	#	it dumps core before it writes anything, so the test
	#	for an empty output file should find an empty output
	#	file and conclude that unaligned accesses don't work).
	#
	# This run-time test won't work if you're cross-compiling, so
	# in order to support cross-compiling for a particular CPU,
	# we have to wire in the list of CPU types anyway, as far as
	# I know, so perhaps we should just have a set of CPUs on
	# which we know it doesn't work, a set of CPUs on which we
	# know it does work, and have the script just fail on other
	# cpu types and update it when such a failure occurs.
	#
	alpha*|arm*|bfin*|hp*|mips*|sh*|sparc*|ia64|nv1)
		ac_cv_lbl_unaligned_fail=yes
		;;

	*)
		cat >conftest.c <<EOF
#		include <sys/types.h>
#		include <sys/wait.h>
#		include <stdio.h>
		unsigned char a[[5]] = { 1, 2, 3, 4, 5 };
		main() {
		unsigned int i;
		pid_t pid;
		int status;
		/* avoid "core dumped" message */
		pid = fork();
		if (pid <  0)
			exit(2);
		if (pid > 0) {
			/* parent */
			pid = waitpid(pid, &status, 0);
			if (pid < 0)
				exit(3);
			exit(!WIFEXITED(status));
		}
		/* child */
		i = *(unsigned int *)&a[[1]];
		printf("%d\n", i);
		exit(0);
		}
EOF
		${CC-cc} -o conftest $CFLAGS $CPPFLAGS $LDFLAGS \
		    conftest.c $LIBS >/dev/null 2>&1
		if test ! -x conftest ; then
			dnl failed to compile for some reason
			ac_cv_lbl_unaligned_fail=yes
		else
			./conftest >conftest.out
			if test ! -s conftest.out ; then
				ac_cv_lbl_unaligned_fail=yes
			else
				ac_cv_lbl_unaligned_fail=no
			fi
		fi
		rm -f -r conftest* core core.conftest
		;;
	esac])
    AC_MSG_RESULT($ac_cv_lbl_unaligned_fail)
    if test $ac_cv_lbl_unaligned_fail = yes ; then
	    AC_DEFINE(LBL_ALIGN,1,[if unaligned access fails])
    fi])

dnl
dnl If using gcc and the file .devel exists:
dnl	Compile with -g (if supported) and -Wall
dnl	If using gcc 2 or later, do extra prototype checking
dnl	If an os prototype include exists, symlink os-proto.h to it
dnl
dnl usage:
dnl
dnl	AC_LBL_DEVEL(copt)
dnl
dnl results:
dnl
dnl	$1 (copt appended)
dnl	HAVE_OS_PROTO_H (defined)
dnl	os-proto.h (symlinked)
dnl
AC_DEFUN(AC_LBL_DEVEL,
    [rm -f os-proto.h
    if test "${LBL_CFLAGS+set}" = set; then
	    $1="$$1 ${LBL_CFLAGS}"
    fi
    if test -f .devel ; then
	    if test "$GCC" = yes ; then
		    if test "${LBL_CFLAGS+set}" != set; then
			    if test "$ac_cv_prog_cc_g" = yes ; then
				    $1="-g $$1"
			    fi
			    $1="$$1 -Wall"
			    if test $ac_cv_lbl_gcc_vers -gt 1 ; then
				    $1="$$1 -Wmissing-prototypes -Wstrict-prototypes"
			    fi
		    fi
	    else
		    case "$host_os" in

		    irix6*)
			    V_CCOPT="$V_CCOPT -n32"
			    ;;

		    *)
			    ;;
		    esac
	    fi
	    os=`echo $host_os | sed -e 's/\([[0-9]][[0-9]]*\)[[^0-9]].*$/\1/'`
	    name="lbl/os-$os.h"
	    if test -f $name ; then
		    ln -s $name os-proto.h
		    AC_DEFINE(HAVE_OS_PROTO_H, 1,
			[if there's an os_proto.h for this platform, to use additional prototypes])
	    else
		    AC_MSG_WARN(can't find $name)
	    fi
    fi])

dnl
dnl Improved version of AC_CHECK_LIB
dnl
dnl Thanks to John Hawkinson (jhawk@mit.edu)
dnl
dnl usage:
dnl
dnl	AC_LBL_CHECK_LIB(LIBRARY, FUNCTION [, ACTION-IF-FOUND [,
dnl	    ACTION-IF-NOT-FOUND [, OTHER-LIBRARIES]]])
dnl
dnl results:
dnl
dnl	LIBS
dnl
dnl XXX - "AC_LBL_LIBRARY_NET" was redone to use "AC_SEARCH_LIBS"
dnl rather than "AC_LBL_CHECK_LIB", so this isn't used any more.
dnl We keep it around for reference purposes in case it's ever
dnl useful in the future.
dnl

define(AC_LBL_CHECK_LIB,
[AC_MSG_CHECKING([for $2 in -l$1])
dnl Use a cache variable name containing the library, function
dnl name, and extra libraries to link with, because the test really is
dnl for library $1 defining function $2, when linked with potinal
dnl library $5, not just for library $1.  Separate tests with the same
dnl $1 and different $2's or $5's may have different results.
ac_lib_var=`echo $1['_']$2['_']$5 | sed 'y%./+- %__p__%'`
AC_CACHE_VAL(ac_cv_lbl_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"
LIBS="-l$1 $5 $LIBS"
AC_TRY_LINK(dnl
ifelse([$2], [main], , dnl Avoid conflicting decl of main.
[/* Override any gcc2 internal prototype to avoid an error.  */
]ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
extern "C"
#endif
])dnl
[/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
]),
	    [$2()],
	    eval "ac_cv_lbl_lib_$ac_lib_var=yes",
	    eval "ac_cv_lbl_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
])dnl
if eval "test \"`echo '$ac_cv_lbl_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$1 $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi
])

dnl
dnl AC_LBL_LIBRARY_NET
dnl
dnl This test is for network applications that need socket() and
dnl gethostbyname() -ish functions.  Under Solaris, those applications
dnl need to link with "-lsocket -lnsl".  Under IRIX, they need to link
dnl with "-lnsl" but should *not* link with "-lsocket" because
dnl libsocket.a breaks a number of things (for instance:
dnl gethostbyname() under IRIX 5.2, and snoop sockets under most
dnl versions of IRIX).
dnl
dnl Unfortunately, many application developers are not aware of this,
dnl and mistakenly write tests that cause -lsocket to be used under
dnl IRIX.  It is also easy to write tests that cause -lnsl to be used
dnl under operating systems where neither are necessary (or useful),
dnl such as SunOS 4.1.4, which uses -lnsl for TLI.
dnl
dnl This test exists so that every application developer does not test
dnl this in a different, and subtly broken fashion.

dnl It has been argued that this test should be broken up into two
dnl seperate tests, one for the resolver libraries, and one for the
dnl libraries necessary for using Sockets API. Unfortunately, the two
dnl are carefully intertwined and allowing the autoconf user to use
dnl them independantly potentially results in unfortunate ordering
dnl dependancies -- as such, such component macros would have to
dnl carefully use indirection and be aware if the other components were
dnl executed. Since other autoconf macros do not go to this trouble,
dnl and almost no applications use sockets without the resolver, this
dnl complexity has not been implemented.
dnl
dnl The check for libresolv is in case you are attempting to link
dnl statically and happen to have a libresolv.a lying around (and no
dnl libnsl.a).
dnl
AC_DEFUN(AC_LBL_LIBRARY_NET, [
    # Most operating systems have gethostbyname() in the default searched
    # libraries (i.e. libc):
    # Some OSes (eg. Solaris) place it in libnsl
    # Some strange OSes (SINIX) have it in libsocket:
    AC_SEARCH_LIBS(gethostbyname, nsl socket resolv)
    # Unfortunately libsocket sometimes depends on libnsl and
    # AC_SEARCH_LIBS isn't up to the task of handling dependencies like this.
    if test "$ac_cv_search_gethostbyname" = "no"
    then
	AC_CHECK_LIB(socket, gethostbyname,
                     LIBS="-lsocket -lnsl $LIBS", , -lnsl)
    fi
    AC_SEARCH_LIBS(socket, socket, ,
	AC_CHECK_LIB(socket, socket, LIBS="-lsocket -lnsl $LIBS", , -lnsl))
    # DLPI needs putmsg under HPUX so test for -lstr while we're at it
    AC_SEARCH_LIBS(putmsg, str)
    ])

dnl
dnl Test for __attribute__
dnl

AC_DEFUN(AC_C___ATTRIBUTE__, [
AC_MSG_CHECKING(for __attribute__)
AC_CACHE_VAL(ac_cv___attribute__, [
AC_COMPILE_IFELSE(
  AC_LANG_SOURCE([[
#include <stdlib.h>

static void foo(void) __attribute__ ((noreturn));

static void
foo(void)
{
  exit(1);
}

int
main(int argc, char **argv)
{
  foo();
}
  ]]),
ac_cv___attribute__=yes,
ac_cv___attribute__=no)])
if test "$ac_cv___attribute__" = "yes"; then
  AC_DEFINE(HAVE___ATTRIBUTE__, 1, [define if your compiler has __attribute__])
  V_DEFS="$V_DEFS -D_U_=\"__attribute__((unused))\""
else
  V_DEFS="$V_DEFS -D_U_=\"\""
fi
AC_MSG_RESULT($ac_cv___attribute__)
])

dnl
dnl Checks to see if tpacket_stats is defined in linux/if_packet.h
dnl If so then pcap-linux.c can use this to report proper statistics.
dnl
dnl -Scott Barron
dnl
AC_DEFUN(AC_LBL_TPACKET_STATS,
   [AC_MSG_CHECKING(if if_packet.h has tpacket_stats defined)
   AC_CACHE_VAL(ac_cv_lbl_tpacket_stats,
   AC_TRY_COMPILE([
#  include <linux/if_packet.h>],
   [struct tpacket_stats stats],
   ac_cv_lbl_tpacket_stats=yes,
   ac_cv_lbl_tpacket_stats=no))
   AC_MSG_RESULT($ac_cv_lbl_tpacket_stats)
   if test $ac_cv_lbl_tpacket_stats = yes; then
       AC_DEFINE(HAVE_TPACKET_STATS,1,[if if_packet.h has tpacket_stats defined])
   fi])

dnl
dnl Checks to see if the tpacket_auxdata struct has a tp_vlan_tci member.
dnl
dnl usage:
dnl
dnl	AC_LBL_LINUX_TPACKET_AUXDATA_TP_VLAN_TCI
dnl
dnl results:
dnl
dnl	HAVE_LINUX_TPACKET_AUXDATA_TP_VLAN_TCI (defined)
dnl
dnl NOTE: any compile failure means we conclude that it doesn't have
dnl that member, so if we don't have tpacket_auxdata, we conclude it
dnl doesn't have that member (which is OK, as either we won't be using
dnl code that would use that member, or we wouldn't compile in any case).
dnl
AC_DEFUN(AC_LBL_LINUX_TPACKET_AUXDATA_TP_VLAN_TCI,
    [AC_MSG_CHECKING(if tpacket_auxdata struct has tp_vlan_tci member)
    AC_CACHE_VAL(ac_cv_lbl_linux_tpacket_auxdata_tp_vlan_tci,
	AC_TRY_COMPILE([
#	include <sys/types.h>
#	include <linux/if_packet.h>],
	[u_int i = sizeof(((struct tpacket_auxdata *)0)->tp_vlan_tci)],
	ac_cv_lbl_linux_tpacket_auxdata_tp_vlan_tci=yes,
	ac_cv_lbl_linux_tpacket_auxdata_tp_vlan_tci=no))
    AC_MSG_RESULT($ac_cv_lbl_linux_tpacket_auxdata_tp_vlan_tci)
    if test $ac_cv_lbl_linux_tpacket_auxdata_tp_vlan_tci = yes ; then
	    HAVE_LINUX_TPACKET_AUXDATA=tp_vlan_tci
	    AC_SUBST(HAVE_LINUX_TPACKET_AUXDATA)
	    AC_DEFINE(HAVE_LINUX_TPACKET_AUXDATA_TP_VLAN_TCI,1,[if tp_vlan_tci exists])
    fi])

dnl
dnl Checks to see if Solaris has the dl_passive_req_t struct defined
dnl in <sys/dlpi.h>.
dnl
dnl usage:
dnl
dnl	AC_LBL_DL_PASSIVE_REQ_T
dnl
dnl results:
dnl 
dnl 	HAVE_DLPI_PASSIVE (defined)
dnl
AC_DEFUN(AC_LBL_DL_PASSIVE_REQ_T,
        [AC_MSG_CHECKING(if dl_passive_req_t struct exists)
       AC_CACHE_VAL(ac_cv_lbl_has_dl_passive_req_t,
                AC_TRY_COMPILE([
#       include <sys/types.h>
#       include <sys/dlpi.h>],
        [u_int i = sizeof(dl_passive_req_t)],
        ac_cv_lbl_has_dl_passive_req_t=yes,
        ac_cv_lbl_has_dl_passive_req_t=no))
    AC_MSG_RESULT($ac_cv_lbl_has_dl_passive_req_t)
    if test $ac_cv_lbl_has_dl_passive_req_t = yes ; then
            AC_DEFINE(HAVE_DLPI_PASSIVE,1,[if passive_req_t primitive
		exists])
    fi])
