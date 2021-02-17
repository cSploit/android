dnl ############################# INITIALISATION ###############################

AC_PREREQ([2.67])
AC_INIT
AC_CONFIG_SRCDIR([src])

AC_CONFIG_AUX_DIR(builds/make.new/config)
AC_CACHE_VAL(ac_cv_prog_make_set, [AC_PROG_MAKE_SET])
AC_PREFIX_DEFAULT(/usr/local/firebird)

m4_ifdef([AC_CONFIG_MACRO_DIR],
         [],
         [m4_define([AC_CONFIG_MACRO_DIR], [])])
AC_CONFIG_MACRO_DIR(m4)

AC_CONFIG_HEADER(src/include/gen/autoconfig.auto:builds/make.new/config/config.h.in)

dnl XE_APPEND(value, varname)
define([XE_APPEND],[[$2]="$[$2] [$1]"])

dnl XE_PREPEND(value, varname)
define([XE_PREPEND],[[$2]="[$1] $[$2]"])

dnl XE_CONF_DIR(param, help, variable, default)
define([XE_CONF_DIR],[
AC_ARG_WITH([$1], 
	[AS_HELP_STRING([--with-$1],[$2])], 
	[[$3]="$withval"
	 AC_DEFINE_UNQUOTED([$3], "$[$3]", [$2])], 
	[[$3]="[`if test ""x$prefix"" = ""xNONE"" ; then echo ""$ac_default_prefix""; else echo ""$prefix""; fi`][$4]"
	 AC_DEFINE_UNQUOTED([$3], [""], [$2])]
)
AC_SUBST([$3])
])

define([XE_CONF_FILE],[
AC_ARG_WITH([$1],
	[AS_HELP_STRING([--with-$1],[$2])],
	[[$3]="$withval"
	 AC_DEFINE_UNQUOTED([$3], "$[$3]", [$2])],
	[[$3]="$4"
	 AC_DEFINE_UNQUOTED([$3], ["$4"], [$2])]
)
AC_SUBST([$3])
])

define([XE_SAVE_ENV], [
pre_save_restore_cflags=$CFLAGS
pre_save_restore_cxxflags=$CXXFLAGS
pre_save_restore_libs=$LIBS
])

define([XE_RESTORE_ENV], [
CFLAGS=$pre_save_restore_cflags
CXXFLAGS=$pre_save_restore_cxxflags
LIBS=$pre_save_restore_libs
])

sinclude(acx_pthread.m4)
sinclude(binreloc.m4)

dnl ############################# ADD TESTS BELOW ##############################

AC_CANONICAL_BUILD
CPU_TYPE=$build_cpu
AC_SUBST(CPU_TYPE)


dnl EKU: set appropiate defaults for each platform
dnl      EDITLINE_FLG     : support fancy command line editing in isql
dnl      SHRLIB_EXT       : suffix of shared library files

RAW_DEVICES_FLG=Y
EXPORT_SYMBOLS_STYLE=linux
INSTALL_PREFIX=""

dnl Use /usr/lib for library links on most platforms except some 64-bit ones
libdir=/usr/lib

dnl Should system editline be used
STD_EDITLINE=false

dnl Test for special ar options?
AR_OPT_CHECK=false

case "$build" in
  x*64-*-darwin*)
    MAKEFILE_PREFIX=darwin_x86_64
    MAKEFILE_POSTFIX=darwin
    PLATFORM=DARWIN
    INSTALL_PREFIX=darwin
    AC_DEFINE(DARWIN, 1, [Define this if OS is DARWIN])
	XE_APPEND(-framework CoreFoundation,LIBS)
    EDITLINE_FLG=Y
    SHRLIB_EXT=dylib
    CPU_TYPE=x86_64
    EXPORT_SYMBOLS_STYLE=darwin
    ;;

  i*86-*-darwin*)
    MAKEFILE_PREFIX=darwin_i386
    MAKEFILE_POSTFIX=darwin
    PLATFORM=DARWIN
    INSTALL_PREFIX=darwin
    AC_DEFINE(DARWIN, 1, [Define this if OS is DARWIN])
	XE_APPEND(-framework CoreFoundation,LIBS)
    EDITLINE_FLG=Y
    SHRLIB_EXT=dylib
    CPU_TYPE=i386
    EXPORT_SYMBOLS_STYLE=darwin
    ;;

  powerpc-*-darwin*)
    MAKEFILE_PREFIX=darwin_powerpc
dnl MAKEFILE_PREFIX=darwin_ppc64
    MAKEFILE_POSTFIX=darwin
    PLATFORM=DARWIN
    INSTALL_PREFIX=darwin
    AC_DEFINE(DARWIN, 1, [Define this if OS is DARWIN])
	XE_APPEND(-framework CoreFoundation,LIBS)
    EDITLINE_FLG=Y
    SHRLIB_EXT=dylib
dnl CPU_TYPE=ppc64
	EXPORT_SYMBOLS_STYLE=darwin
    ;;

  powerpc-*-aix*)
    dnl ibm xlC has many invocations, like xlC, xlc++, or xlc_r7
    comp=`echo "$CXX" | cut -c1-3 | dd conv=lcase 2>/dev/null`
    if test "$comp" = "xlc"; then
        AR_OPT_CHECK=true
        TLS_OPTIONS="-qtls=default"
        MAKEFILE_PREFIX=aix_powerpc_xlc
    else
        MAKEFILE_PREFIX=aix_powerpc
    fi
    MAKEFILE_POSTFIX=aix
    PLATFORM=AIX
    INSTALL_PREFIX=aix
    AC_DEFINE(AIX, 1, [Define this if OS is AIX])
    EDITLINE_FLG=N
    SHRLIB_EXT=so
	EXPORT_SYMBOLS_STYLE=aix
    ;;

  amd64-*-freebsd* | x86_64*-*-freebsd* | x86_64*-*-k*bsd*-gnu)
    MAKEFILE_PREFIX=freebsd_amd64
    case "$build" in
        x86_64*-*-k*bsd-gnu)      # Debian/kFreeBSD
            PLATFORM=GENTOOFREEBSD
            INSTALL_PREFIX=linux
            ;;
		*)
            PLATFORM=FREEBSD
            INSTALL_PREFIX=freebsd
            ;;
    esac
    AC_DEFINE(FREEBSD, 1, [Define this if OS is FreeBSD])
    AC_DEFINE(AMD64, 1, [Define this if CPU is amd64])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  *-gentoo-freebsd*)
    MAKEFILE_PREFIX=freebsd
    PLATFORM=GENTOOFREEBSD
    AC_DEFINE(FREEBSD, 1, [Define this if OS is FreeBSD])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  *-*-freebsd* | *-*-k*bsd*-gnu)
    MAKEFILE_PREFIX=freebsd
    case "$build" in
        *-*-k*bsd-gnu)      # Debian/kFreeBSD
            PLATFORM=GENTOOFREEBSD
            INSTALL_PREFIX=linux
            ;;
        *)
            PLATFORM=FREEBSD
            ;;
    esac
    AC_DEFINE(FREEBSD, 1, [Define this if OS is FreeBSD])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  mips-*-linux*)
    MAKEFILE_PREFIX=linux_mips
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX)
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    PTHREAD_CFLAGS=-pthread
    PTHREAD_LIBS=-lpthread
    ;;

  mipsel-*-linux*)
    MAKEFILE_PREFIX=linux_mipsel
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX)
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    PTHREAD_CFLAGS=-pthread
    PTHREAD_LIBS=-lpthread
    ;;

  x86_64*-*-linux* | x86_64*-*-gnu*)
    MAKEFILE_PREFIX=linux_amd64
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
	libdir=/usr/lib64
    CPU_TYPE=amd64
    ;;

  ia64*-*-linux*)
    MAKEFILE_PREFIX=linux_ia64
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    libdir=/usr/lib
    CPU_TYPE=ia64
    ;;

  arm*-*-linux*)
    MAKEFILE_PREFIX=linux_arm
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    STD_EDITLINE=true
    ;;

  sparc*-*-linux* | sparc*-*-gnu* | sparc*-*-k*bsd*-gnu)
    MAKEFILE_PREFIX=linux_sparc32
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  powerpc*-*-linux*)
    MAKEFILE_PREFIX=linux_powerpc
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  i*86*-*-linux*)
    MAKEFILE_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX)
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  s390*-*-linux*)
    MAKEFILE_PREFIX=linux_s390x
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  sh*eb-*-linux*)
    MAKEFILE_PREFIX=linux_generic
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    AC_DEFINE(SHEB, 1, [Architecture is big-edian sh4])
    LOCK_MANAGER_FLG=Y
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;


  sh*-*-linux*)
    MAKEFILE_PREFIX=linux_generic
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    AC_DEFINE(SH, 1, [Architecture is little-endian sh4])
    LOCK_MANAGER_FLG=Y
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  hppa*-*-linux*)
    MAKEFILE_PREFIX=linux_generic
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    AC_DEFINE(HPPA, 1, [Define this if CPU is HPPA])
    LOCK_MANAGER_FLG=Y
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  alpha*-*-linux*)
   MAKEFILE_PREFIX=linux_generic
   INSTALL_PREFIX=linux
   PLATFORM=LINUX
   AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
   AC_DEFINE(ALPHA, 1, [Define this if CPU is Alpha])
   LOCK_MANAGER_FLG=Y
   EDITLINE_FLG=Y
   SHRLIB_EXT=so
   ;;

  *-*-linux* | *-*-gnu*)
    MAKEFILE_PREFIX=linux_generic
    INSTALL_PREFIX=linux
    PLATFORM=LINUX
    AC_DEFINE(LINUX, 1, [Define this if OS is Linux])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  *-*-netbsd*)
    MAKEFILE_PREFIX=netbsd
    PLATFORM=NETBSD
    AC_DEFINE(NETBSD, 1, [Define this if OS is NetBSD])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  ia64-*-hpux*)
    comp=`echo "$CXX" | cut -c1-3`
    case $comp in
       aCC) MAKEFILE_PREFIX=hpux_aCC ;
           PTHREAD_CFLAGS=-mt
           ;;
       *)  MAKEFILE_PREFIX=hpux_ia64 ;
           ;;
    esac
    INSTALL_PREFIX=hpux
    PLATFORM=HPUX
    AC_DEFINE(HPUX, 1, [Define this if OS is HP-UX])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    libdir=/usr/lib/pa20_64
	EXPORT_SYMBOLS_STYLE=hpux
    ;;

  hppa*-*-hpux*)
    comp=`echo "$CXX" | cut -c1-3`
    case $comp in
       aCC) MAKEFILE_PREFIX=hpux_aCC ;
           PTHREAD_CFLAGS=-mt
           ;;
       *)  MAKEFILE_PREFIX=hpux ;
           ;;
    esac
    INSTALL_PREFIX=hpux
    PLATFORM=HPUX
    AC_DEFINE(HPUX, 1, [Define this if OS is HP-UX])
    EDITLINE_FLG=Y
    SHRLIB_EXT=sl
    libdir=/usr/lib/pa20_64
	EXPORT_SYMBOLS_STYLE=hpux
    ;;

  i386-pc-solaris*)
    dnl detect native compiler, Sun Studio Pro
    comp=`echo "$CXX" | cut -c1-2`
    case $comp in
       CC) MAKEFILE_PREFIX=solaris
           PTHREAD_CFLAGS=-mt
           PTHREAD_LIBS=-lpthread
           ;;
       *)  MAKEFILE_PREFIX=solx86gcc ;
           PTHREAD_CFLAGS=-pthreads ;;
    esac
    dnl if this is amd64 system, we should install our libraries in /usr/lib/amd64
    isa=`isainfo -k`
    if test "$isa" = "amd64"; then
        libdir=/usr/lib/amd64
    fi
    CAS_OPTIONS=`pwd`"/src/common/classes/fb_cas_sax.il";
    INSTALL_PREFIX=solaris
    PLATFORM=solx86
    AC_DEFINE(solx86, 1, [Define this if OS is Solarix x86])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  sparc-sun-solaris*)
    dnl detect native compiler, Sun Studio Pro
    comp=`echo "$CXX" | cut -c1-3`
    case $comp in
       CC) MAKEFILE_PREFIX=solaris ;
           dnl inline assembly for atomic operations on Solaris 9 w SSPRO
           PTHREAD_CFLAGS=-mt
           PTHREAD_LIBS=-lpthread
           if test "${build#*solaris}" = "2.9"; then
             ATOMIC_OPTIONS=`pwd`"/src/common/classes/fb_atomic.il";
           fi
           ;;
       *)  MAKEFILE_PREFIX=solaris-64gcc ;
    esac
    CAS_OPTIONS=`pwd`"/src/common/classes/fb_cas.il";
    INSTALL_PREFIX=solaris
    PLATFORM=SOLARIS
    AC_DEFINE(SOLARIS, 1, [Define this if OS is Solaris Sparc])
    EDITLINE_FLG=Y
    SHRLIB_EXT=so
    ;;

  *-*-mingw*)
    MAKEFILE_PREFIX=mingw
    PLATFORM=win32
    AC_DEFINE(WIN_NT, 1, [Define this if OS is Windows NT])
    AC_DEFINE(HAVE_MULTI_THREAD, 1, [Define this if multi-threading should be supported])
    EDITLINE_FLG=N
    RAW_DEVICES_FLG=N
    SHRLIB_EXT=dll
    ;;

  *)
    AC_MSG_ERROR(unsupported platform ${build})
    ;;
esac

if test "$INSTALL_PREFIX" = ""; then
  INSTALL_PREFIX=$MAKEFILE_PREFIX
fi

AM_BINRELOC

AC_SUBST(MAKEFILE_PREFIX)
AC_SUBST(AR_OPTIONS)
AC_SUBST(PLATFORM)
AC_SUBST(SHRLIB_EXT)
AC_DEFINE(CASE_SENSITIVITY, true, [Define this if paths are case sensitive])


dnl Some controllable options

AC_ARG_ENABLE(developer,
  [  --enable-developer      use developer mode rules (default=no)],
  [case "$enableval" in
     yes) DEVEL_FLG=Y;;
     no)  DEVEL_FLG=N;;
     *)   AC_MSG_ERROR(bad value '${enableval}' for --enable-developer);;
   esac])
AC_SUBST(DEVEL_FLG)

CROSS=
IS_CROSS=N
AC_ARG_WITH(cross-build,
  [  --with-cross-build      build for platform different from current one],
  [CROSS=${withval}
   IS_CROSS=Y])
AC_SUBST(IS_CROSS)

dnl Avoid dumb '-g -O2' autoconf's default
dnl Debugging information and optimization flags should be set in prefix.$platform file
dnl Should be replaced with AC_PROG_GCC_DEFAULT_FLAGS() when available
if test "$CFLAGS" = "-g -O2" -o "$CFLAGS" = "-g" -o "$CFLAGS" = "-O2"; then
	CFLAGS=
fi

dnl Find out how to use threads on this platform
XE_SAVE_ENV()
ACX_PTHREAD([
  AC_DEFINE(HAVE_MULTI_THREAD, 1,
    [Define this if multi-threading should be supported])])
XE_RESTORE_ENV()

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)

AC_ARG_ENABLE(raw-devices,
  [  --enable-raw-devices    enable databases on raw devices (default on POSIX)],
  [case "$enableval" in
     yes) RAW_DEVICES_FLG=Y;;
     no)  RAW_DEVICES_FLG=N;;
     *)   AC_MSG_ERROR(bad value '${enableval}' for --enable-raw-devices);;
   esac])
if test "$RAW_DEVICES_FLG" = "Y"; then
  AC_DEFINE(SUPPORT_RAW_DEVICES, 1,
    [Define this if databases on raw devices should be supported])
fi

AC_ARG_ENABLE(rpath,
  [  --enable-rpath          enable linking with RPATH (default)],
  [],
  [enable_rpath=yes])
if test "x$enable_rpath" != "xno" || test "x$enable_rpath" = "x" ; then
  AC_SUBST(USE_RPATH, 1)
fi

AC_ARG_WITH(editline,
  [  --with-editline         support fancy command line editing],
  [case "$withval" in
     yes) EDITLINE_FLG=Y;;
     no)  EDITLINE_FLG=N;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-editline);;
   esac])
AC_ARG_WITH(system-editline,
  [  --with-system-editline  use OS supplied libeditline],
  [case "$withval" in
     yes) STD_EDITLINE=true;;
     no)  STD_EDITLINE=false;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-system-editline);;
   esac])

# not need editline in default libs, but need to test for its presence
READLINE=edit	# builtin default
XE_SAVE_ENV()
if test "$STD_EDITLINE" = "true"; then
  AC_CHECK_LIB(edit, readline, [READLINE=edit EDITLINE_FLG=Y],
    AC_CHECK_LIB(editline, readline, [READLINE=editline EDITLINE_FLG=Y],
      AC_CHECK_LIB(readline, readline, [READLINE=readline EDITLINE_FLG=Y],
        [STD_EDITLINE=false
         if test "$EDITLINE_FLG" = "Y"; then
           AC_MSG_WARN([[[--with-system-editline specified, not found. Using bundled editline]]])])))
		 fi
fi
XE_RESTORE_ENV()

AC_SUBST(READLINE)
AC_SUBST(STD_EDITLINE)
AC_SUBST(EDITLINE_FLG)

FB_SERVICE_NAME=gds_db
FB_SERVICE_PORT=3050
FB_IPC_NAME=FirebirdIPI
AC_ARG_WITH(service-name,
  [  --with-service-name     specify inet service name (default=gds_db)],
  [FB_SERVICE_NAME=${withval}])
AC_ARG_WITH(service-port,
  [  --with-service-port     specify inet service port (default=3050)],
  [FB_SERVICE_PORT=${withval}])
AC_ARG_WITH(ipc-name,
  [  --with-ipc-name         specify local IPC name (default=FirebirdIPI)],
  [FB_IPC_NAME=${withval}])
AC_DEFINE_UNQUOTED(FB_SERVICE_NAME,"$FB_SERVICE_NAME", [Inet service name])
AC_DEFINE_UNQUOTED(FB_SERVICE_PORT,$FB_SERVICE_PORT, [Inet service port])
AC_DEFINE_UNQUOTED(FB_IPC_NAME,"$FB_IPC_NAME", [Local IPC name])
AC_SUBST(FB_SERVICE_NAME)
AC_SUBST(FB_SERVICE_PORT)
AC_SUBST(FB_IPC_NAME)

dnl Use non-standard directory placement control
dnl Sorry - GNU standard is not Ok for us
XE_CONF_DIR(fbbin, [executables DIR (PREFIX/bin)], FB_BINDIR, /bin)
XE_CONF_DIR(fbsbin, [system admin executables DIR (PREFIX/bin)], FB_SBINDIR, /bin)
XE_CONF_DIR(fbconf, [config files DIR (PREFIX)], FB_CONFDIR)
XE_CONF_DIR(fblib, [object code libraries DIR (PREFIX/lib)], FB_LIBDIR, /lib)
XE_CONF_DIR(fbinclude, [C/C++ header files DIR (PREFIX/include)], FB_INCDIR, /include)
XE_CONF_DIR(fbdoc, [documentation root DIR (PREFIX/doc)], FB_DOCDIR, /doc)
XE_CONF_DIR(fbudf, [UDF DIR (PREFIX/UDF)], FB_UDFDIR, /UDF)
XE_CONF_DIR(fbsample, [examples DIR (PREFIX/examples)], FB_SAMPLEDIR, /examples)
XE_CONF_DIR(fbsample-db, [examples database DIR (PREFIX/examples/empbuild)], FB_SAMPLEDBDIR, /examples/empbuild)
XE_CONF_DIR(fbhelp, [QLI help DIR (PREFIX/help)], FB_HELPDIR, /help)
XE_CONF_DIR(fbintl, [international DIR (PREFIX/intl)], FB_INTLDIR, /intl)
XE_CONF_DIR(fbmisc, [misc DIR (PREFIX/misc)], FB_MISCDIR, /misc)
XE_CONF_DIR(fbsecure-db, [security database DIR (PREFIX)], FB_SECDBDIR)
XE_CONF_DIR(fbmsg, [message files DIR (PREFIX)], FB_MSGDIR)
XE_CONF_DIR(fblog, [log files DIR (PREFIX)], FB_LOGDIR)
XE_CONF_FILE(fblogfilename, [log file name within log dir (firebird.log)], FB_LOGFILENAME, firebird.log)
XE_CONF_DIR(fbglock, [guardian lock DIR (PREFIX)], FB_GUARDDIR)
XE_CONF_DIR(fbplugins, [plugins DIR (PREFIX)], FB_PLUGDIR, /plugins)

dnl gpre built-in language support (C/C++ are mandatory)
GPRE_LANGUAGE_MODULES=""
AC_ARG_WITH(gpre-ada,
  [  --with-gpre-ada         support ADA in gpre (default=no)],
  [case "$withval" in
     yes) XE_APPEND(ada.cpp,GPRE_LANGUAGE_MODULES)
          AC_DEFINE(GPRE_ADA, 1, [Define this if GPRE should support ADA]);;
     no)  ;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-gpre-ada);;
   esac])
AC_ARG_WITH(gpre-cobol,
  [  --with-gpre-cobol       support COBOL in gpre (default=no)],
  [case "$withval" in
     yes) XE_APPEND(cob.cpp rmc.cpp,GPRE_LANGUAGE_MODULES)
          AC_DEFINE(GPRE_COBOL, 1, [Define this if GPRE should support COBOL]);;
     no)  ;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-gpre-cobol);;
   esac])
AC_ARG_WITH(gpre-fortran,
  [  --with-gpre-fortran     support FORTRAN in gpre (default=no)],
  [case "$withval" in
     yes) XE_APPEND(ftn.cpp,GPRE_LANGUAGE_MODULES)
          AC_DEFINE(GPRE_FORTRAN, 1, [Define this if GPRE should support FORTRAN]);;
     no)  ;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-gpre-fortran);;
   esac])
AC_ARG_WITH(gpre-pascal,
  [  --with-gpre-pascal      support PASCAL in gpre (default=no)],
  [case "$withval" in
     yes) XE_APPEND(pas.cpp,GPRE_LANGUAGE_MODULES)
          AC_DEFINE(GPRE_PASCAL, 1, [Define this if GPRE should support PASCAL]);;
     no)  ;;
     *)   AC_MSG_ERROR(bad value '${withval}' for --with-gpre-pascal);;
   esac])
AC_SUBST(GPRE_LANGUAGE_MODULES)



dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC_STDC
AC_PROG_GCC_TRADITIONAL
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
LT_INIT([dlopen win32-dll])

dnl Check for system header files
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(assert.h)
AC_CHECK_HEADERS(ctype.h)
AC_CHECK_HEADERS(errno.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(grp.h)
AC_CHECK_HEADERS(pwd.h)
AC_CHECK_HEADERS(libio.h)
AC_CHECK_HEADERS(locale.h)
AC_CHECK_HEADERS(math.h)
AC_CHECK_HEADERS(pthread.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(sys/stat.h)
AC_CHECK_HEADERS(sys/uio.h)
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(sys/timeb.h)
AC_CHECK_HEADERS(sys/param.h)
AC_CHECK_HEADERS(sys/mount.h)
AC_CHECK_HEADERS(sys/ioctl.h)
AC_CHECK_HEADERS(sys/select.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(setjmp.h)
AC_CHECK_HEADERS(stdarg.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(signal.h sys/signal.h sys/siginfo.h)
AC_CHECK_HEADERS(termio.h termios.h)
if test "$EDITLINE_FLG" = "Y"; then
  AC_HEADER_DIRENT
  AC_DEFINE(HAVE_EDITLINE_H, 1, [Define this if editline is in use])
fi
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(varargs.h)
AC_CHECK_HEADERS(crypt.h)
AC_CHECK_HEADERS(netinet/in.h rpc/rpc.h netconfig.h)
AC_CHECK_HEADERS(rpc/xdr.h,,,[#include <rpc/rpc.h>])
AC_CHECK_HEADERS(aio.h)
AC_CHECK_HEADERS(mntent.h mnttab.h sys/mntent.h sys/mnttab.h)
AC_CHECK_HEADERS(sys/ipc.h sys/file.h)
AC_CHECK_HEADERS(socket.h sys/socket.h sys/sockio.h winsock2.h)
AC_CHECK_HEADERS(sys/resource.h)
AC_CHECK_HEADERS(sys/sem.h)
AC_CHECK_HEADERS(semaphore.h)
AC_CHECK_HEADERS(float.h)
AC_CHECK_HEADERS(atomic.h)
AC_CHECK_HEADERS(atomic_ops.h)
AC_CHECK_HEADERS(poll.h)
AC_CHECK_HEADERS(langinfo.h)
AC_CHECK_HEADERS(iconv.h)
AC_CHECK_HEADERS(libio.h)

dnl check for ICU presence
AC_CHECK_HEADER(unicode/ucnv.h,,AC_MSG_ERROR(ICU support not found - please install development ICU package))
dnl setting ICU_OK here is done to only avoid default action
AC_CHECK_LIB(icuuc, main, ICU_OK=yes, AC_MSG_ERROR(ICU support not found - please install development ICU package))

dnl check for tommath presence
XE_SAVE_ENV()
LIBS=
AC_CHECK_HEADER(tommath.h,,AC_MSG_ERROR(Include file for tommath not found - please install development tommath package))
AC_CHECK_LIB(tommath, mp_init, MATHLIB=-ltommath, AC_MSG_ERROR(Library tommath not found - please install development tommath package))
XE_RESTORE_ENV()
AC_SUBST(MATHLIB)

dnl Check for libraries
AC_SEARCH_LIBS(dlopen, dl)
AC_CHECK_LIB(m, main)
if test "$EDITLINE_FLG" = "Y"; then
	AC_CHECK_LIB(curses, tgetent, TERMLIB=curses, \
		AC_CHECK_LIB(ncurses, tgetent, TERMLIB=ncurses, \
			AC_CHECK_LIB(termcap, tgetent, TERMLIB=termcap, \
				AC_CHECK_LIB(tinfo, tgetent, TERMLIB=tinfo, \
					AC_MSG_ERROR(termcap support not found)))))
	AC_SUBST(TERMLIB)
	XE_APPEND(-l$TERMLIB, LIBS)
fi

AC_SEARCH_LIBS(inet_aton, resolv)

dnl HP-UX atomic routines are in atomic library, not standard C library.
dnl If we can't find atomic routines in vendor library, look for atomic_ops.
AC_SEARCH_LIBS(atomic_inc, atomic,,
	AC_CHECK_LIB(atomic_ops, main))


dnl Check for libraries for static C++ runtime linking
AC_CHECK_LIB(supc++, main, XE_APPEND(-lsupc++, STATIC_CXXSUPPORT_LIB))
AC_CHECK_LIB(gcc_eh, main, XE_APPEND(-lgcc_eh, STATIC_CXXSUPPORT_LIB))
AC_SUBST(STATIC_CXXSUPPORT_LIB)

AC_LANG_PUSH(C++)
dnl check for INFINITY in math.h
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <math.h>]], [[double x = INFINITY;]])],
	AC_DEFINE(HAVE_INFINITY, 1, [Define this if INFINITY is defined in math.h]))

dnl check for va_copy() in stdarg.h
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <stdarg.h>
		void vafun(const char* fmt, ...) {va_list par, par2; va_start(par, fmt); va_copy(par2, par); va_end(par2); va_end(par);}]], 
		[[return 0;]])],
	AC_DEFINE(HAVE_VA_COPY, 1, [Define this if va_copy() is defined in stdarg.h]))

if test "$AR_OPT_CHECK" = "true"; then
dnl if 64-bit mode, then archive tool, ar, needs -X64 option
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(,[[int lp64 = __64BIT__; ]])],
        [AR_OPTIONS=-X64],)
fi
AC_LANG_POP(C++)

dnl Check for functions
AC_CHECK_FUNCS(gettimeofday)
if test "$ac_cv_func_gettimeofday" = "yes"; then
    AC_MSG_CHECKING(if gettimeofday accepts second (timezone) argument)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/time.h>]], [[struct timeval tp; gettimeofday(&tp, (struct timezone *)0);]])],[AC_DEFINE(GETTIMEOFDAY_RETURNS_TIMEZONE, 1,
       [Define this if gettimeofday accepts second (timezone) argument])
     AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
  AH_BOTTOM([#ifdef GETTIMEOFDAY_RETURNS_TIMEZONE
#define GETTIMEOFDAY(x) gettimeofday((x), (struct timezone *)0)
#else
#define GETTIMEOFDAY(x) gettimeofday((x))
#endif])
fi
AC_CHECK_FUNCS(time times)
AC_CHECK_FUNCS(nanosleep)
AC_SEARCH_LIBS(gethostname,nsl)
AC_SEARCH_LIBS(gethostbyname,nsl)
AC_SEARCH_LIBS(connect,socket)
AC_CHECK_FUNCS(strcasecmp stricmp)
AC_CHECK_FUNCS(strncasecmp strnicmp)
AC_SEARCH_LIBS(dirname, gen)
AC_CHECK_FUNCS(dirname)
AC_CHECK_FUNCS(sigaction setitimer)
AC_CHECK_FUNCS(snprintf vsnprintf)
AC_CHECK_FUNCS(swab _swab)
AC_FUNC_MMAP
AC_FUNC_FORK
AC_FUNC_SETPGRP
AC_CHECK_FUNCS(setpgid)
AC_FUNC_GETPGRP
AC_CHECK_FUNCS(flock)
AC_CHECK_FUNCS(dladdr)
AC_CHECK_FUNCS(initgroups)
AC_CHECK_FUNCS(getpagesize)
AC_CHECK_FUNCS(pread pwrite)
AC_CHECK_FUNCS(getcwd getwd)
AC_CHECK_FUNCS(setmntent getmntent)
if test "$ac_cv_func_getmntent" = "yes"; then
  AC_MSG_CHECKING(if getmntent needs second argument)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>
#include <sys/mnttab.h>]], [[FILE *f; struct mntent *m; getmntent(f, m);]])],[AC_DEFINE(GETMNTENT_TAKES_TWO_ARGUMENTS, 1,
    [Define this if getmntent needs second argument])
   AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
fi
AC_CHECK_FUNCS(setrlimit getrlimit)
AC_CHECK_FUNCS(tcgetattr strdup)
AC_CHECK_FUNCS(mkstemp)
AC_CHECK_FUNCS(pthread_keycreate pthread_key_create)
AC_CHECK_FUNCS(llrint)
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(gmtime_r)
AC_CHECK_FUNCS(fchmod)
AC_CHECK_FUNCS(semtimedop)
AC_CHECK_FUNCS(fegetenv)
AC_CHECK_FUNCS(strerror_r)
case $host in
	*-darwin*)
		ac_cv_func_fdatasync=no
		;;
	*)
		AC_CHECK_FUNCS(fdatasync fsync)
		;;
esac
AC_CHECK_FUNCS(poll)
dnl AC_CHECK_FUNCS(AO_compare_and_swap_full)
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#include <atomic_ops.h>]], [[AO_T x; AO_compare_and_swap_full(&x, 0, 0); return 0;]])],
	AC_DEFINE(HAVE_AO_COMPARE_AND_SWAP_FULL, 1, [Define this if AO_compare_and_swap_full() is defined in atomic_ops.h]))

dnl Check for time function
AC_SEARCH_LIBS(clock_gettime, rt)
AC_CHECK_FUNCS(clock_gettime)

dnl Checks for pthread functions
AC_CHECK_FUNCS(pthread_mutexattr_setprotocol)
AC_CHECK_FUNCS(pthread_mutexattr_setrobust_np)
AC_CHECK_FUNCS(pthread_mutex_consistent_np)
AC_CHECK_FUNCS(pthread_rwlockattr_setkind_np)
AC_CHECK_FUNCS(pthread_cancel)
AC_CHECK_FUNCS(sigset)

dnl Semaphores
if test "$ac_cv_header_semaphore_h" = "yes"; then
AC_SEARCH_LIBS(sem_init, rt pthread)
AC_CHECK_FUNCS(sem_timedwait)
AC_CHECK_FUNCS(sem_init)
if test "$ac_cv_func_sem_init" = "yes"; then
AC_MSG_CHECKING(for working sem_init())
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <semaphore.h>
	main () {
		sem_t s;
		exit(sem_init(&s,0,0));
		}
	]])],[AC_DEFINE(WORKING_SEM_INIT,1,[Define this if sem_init() works on the platform])
AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)
AC_SEARCH_LIBS(sem_open, rt)],[])
fi
fi

dnl Some OS place iconv() support in separate library
if test "$ac_cv_header_iconv_h" = "yes"; then
AC_SEARCH_LIBS(iconv_open, iconv)
fi

dnl HPUX has a bug in .h files. To detect it we need C++ here.
AC_LANG_PUSH(C++)
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <fcntl.h>]], [[posix_fadvise(0, 0, 0, 0);]])],
	AC_DEFINE(HAVE_POSIX_FADVISE, 1, [Define this if posix_fadvise() is present on the platform]))
AC_LANG_POP(C++)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
AC_C_VOLATILE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_SYS_LARGEFILE
if test "$ac_cv_sys_file_offset_bits" = "no"; then
  AC_MSG_CHECKING(for native large file support)
  AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <unistd.h>
    main () {
    exit(!(sizeof(off_t) == 8));
  }]])],[ac_cv_sys_file_offset_bits=64; AC_DEFINE(_FILE_OFFSET_BITS,64)
   AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)],[])
fi

AC_CHECK_SIZEOF(void *)
VOID_PTR_SIZE=$ac_cv_sizeof_void_p
AC_SUBST(VOID_PTR_SIZE)

AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_TYPES([socklen_t],,,[#include <sys/socket.h>])
AH_BOTTOM([#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif])
AC_CHECK_TYPES([semun])
if test "$ac_cv_type_semun" = "no"; then
  AC_MSG_CHECKING(if sem.h defines semun)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
      #include <sys/ipc.h>
      #include <sys/sem.h>]], [[union semun s;]])],[ac_cv_type_semun=yes; AC_DEFINE(HAVE_SEMUN) AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
fi

AC_CHECK_TYPES([struct XDR::xdr_ops],,,[#include <rpc/rpc.h>
#include <rpc/xdr.h>])
AC_CHECK_TYPES([struct xdr_ops],,,[#include <rpc/rpc.h>
#include <rpc/xdr.h>])

dnl AC_STRUCT_DIRENT_D_TYPE
AC_CHECK_MEMBER([struct dirent.d_type],
				AC_DEFINE(HAVE_STRUCT_DIRENT_D_TYPE,1,[Define this if struct dirent has d_type]),,
				[#include <dirent.h>])

dnl EKU: try to determine the alignment of long and double
dnl      replaces FB_ALIGNMENT and FB_DOUBLE_ALIGN in src/jrd/common.h
AC_MSG_CHECKING(alignment of long)
AC_RUN_IFELSE([AC_LANG_SOURCE([[main () {
  struct s {
    char a;
    long long b;
  };
  exit((int)&((struct s*)0)->b);
}]])],[ac_cv_c_alignment=$ac_status],[ac_cv_c_alignment=$ac_status],[])
AC_MSG_RESULT($ac_cv_c_alignment)
AC_DEFINE_UNQUOTED(FB_ALIGNMENT, $ac_cv_c_alignment, [Alignment of long])

AC_MSG_CHECKING(alignment of double)
AC_RUN_IFELSE([AC_LANG_SOURCE([[main () {
  struct s {
    char a;
    double b;
  };
  exit((int)&((struct s*)0)->b);
}]])],[ac_cv_c_double_align=$ac_status],[ac_cv_c_double_align=$ac_status],[])
AC_MSG_RESULT($ac_cv_c_double_align)
AC_DEFINE_UNQUOTED(FB_DOUBLE_ALIGN, $ac_cv_c_double_align, [Alignment of double])

dnl EKU: Add any platform specific tests below
case "$PLATFORM" in
  LINUX)
    dnl MOD: Check for /proc/self/exe mainly used on linux systems
    dnl      this is used to determine path to executable file.
    AC_CHECK_FILES(/proc/self/exe)
    ;;

  FREEBSD|GENTOOFREEBSD)
    dnl the AC_CHECK_TYPES for socklen_t doesn't include sys/socket.h
    dnl so test for socklen_t with sys/socket.h included
    AC_MSG_CHECKING(if sys/socket.h defines socklen_t)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
       #include <sys/socket.h>]], [[socklen_t s;]])],[AC_DEFINE(HAVE_SOCKLEN_T) AC_MSG_RESULT(yes)],[AC_MSG_RESULT(no)])
    ;;

  SOLARIS|solx86)
    if test "$EDITLINE_FLG" = "Y"; then
       AC_CHECK_LIB(termcap, main, XE_APPEND(-ltermcap,EDITLINE_A))
    fi

    AC_CHECK_TYPES([caddr_t])

    AC_ARG_WITH(sfio,
      AS_HELP_STRING([--with-sfio=PATH],[use sfio (replacement stdio on Solaris) (default=no)]),
		[ac_cv_use_sfio=$withval],[ac_cv_use_sfio=no])

    if test "$ac_cv_use_sfio" = "no"; then
      SFIO_DIR=""
      SFIO_FLAGS=""
      SFIO_LDFLAGS=""
    else
      AC_CHECK_LIB(sfio, main)
      dnl Kkuznetsov: using sfio-mt looks like double protecting of FILE*
      dnl and I avoid to use vthread based on posix threading
      #AC_CHECK_LIB(sfio-mt, main,,
      #AC_MSG_ERROR([sfio library not found]),-lvthread)
      #XE_PREPEND(-lvthread,LIBS)

      SFIO_DIR="$ac_cv_use_sfio"
      SFIO_FLAGS="-DSFIO -I$ac_cv_use_sfio/include"
      SFIO_LDFLAGS="-L$ac_cv_use_sfio/lib"
      SFIO_EXAMPLES="Y"
    fi
    ;;

  win32)
    FB_PIPE_NAME=interbas
    AC_ARG_WITH(pipe-name,
      [  --with-pipe-name        specify wnet pipe name (default=interbas)],
      [FB_PIPE_NAME=${withval}])
    AH_VERBATIM(FB_PIPE_NAME,
[/* Wnet pipe name */
#define FB_PIPE_NAME "interbas"])
    AC_DEFINE_UNQUOTED(FB_PIPE_NAME,"$FB_PIPE_NAME")
    AC_SUBST(FB_PIPE_NAME)
    XE_PREPEND( -mno-cygwin -mthreads -lmpr -lversion -lws2_32 -lole32,LIBS)
    ;;

  *)
    ;;
esac

dnl Detect support for ISO syntax for thread-locals
XE_SAVE_ENV()
CXXFLAGS=$TLS_OPTIONS
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[__thread int a = 42;]],[[a = a + 1;]])],
  AC_DEFINE(HAVE___THREAD, 1, [Define it if compiler supports ISO syntax for thread-local storage]),)
XE_RESTORE_ENV()
AC_SUBST(TLS_OPTIONS)
AC_SUBST(ATOMIC_OPTIONS)
AC_SUBST(CAS_OPTIONS)

dnl ##################### DO NOT ADD ANY TESTS BELOW ###########################

AC_SUBST(SFIO_DIR)
AC_SUBST(SFIO_FLAGS)
AC_SUBST(SFIO_LDFLAGS)
AC_SUBST(SFIO_EXAMPLES)

dnl The following file defines the version number strings. Specially for install/uninstall routines
. ./src/misc/writeBuildNum.sh

AC_SUBST(FIREBIRD_VERSION)
AC_SUBST(FIREBIRD_PACKAGE_VERSION)
AC_SUBST(FB_BUILD_NUM, $BuildNum)

BUILD_ROOT_DIR=`pwd`
SRC_ROOT=$BUILD_ROOT_DIR/src
GEN_ROOT=$BUILD_ROOT_DIR/gen
NEW_FIREBIRD_DIR=$GEN_ROOT/firebird

AC_SUBST(BUILD_ROOT_DIR)
AC_SUBST(NEW_FIREBIRD_DIR)
AC_SUBST(EXPORT_SYMBOLS_STYLE)

AC_DEFINE_UNQUOTED(FB_PREFIX, $(if test "x$prefix" = "xNONE" ; then echo \"$ac_default_prefix\"; else echo \"$prefix\"; fi), [Installation path prefix])

FB_TARGETS="Debug Release"
test "x$CROSS" = "x" || FB_TARGETS="$FB_TARGETS Native"

AC_CONFIG_COMMANDS(,,[

FB_TARGETS="$FB_TARGETS"
for fb_tgt in \$FB_TARGETS; do

	echo "Creating \$fb_tgt directories"

dnl ### GEN ### directories for databases and misc
	mkdir -p gen/\$fb_tgt/examples
	mkdir -p gen/\$fb_tgt/refDatabases

dnl # output
	mkdir -p gen/\$fb_tgt/include
	mkdir -p gen/\$fb_tgt/intl
	mkdir -p gen/\$fb_tgt/firebird/UDF
	mkdir -p gen/\$fb_tgt/firebird/bin
	mkdir -p gen/\$fb_tgt/firebird/plugins
	mkdir -p gen/\$fb_tgt/firebird/examples/api
	mkdir -p gen/\$fb_tgt/firebird/examples/dbcrypt
	mkdir -p gen/\$fb_tgt/firebird/examples/empbuild
	mkdir -p gen/\$fb_tgt/firebird/examples/include
	mkdir -p gen/\$fb_tgt/firebird/examples/interfaces
	mkdir -p gen/\$fb_tgt/firebird/examples/package
	mkdir -p gen/\$fb_tgt/firebird/examples/stat
	mkdir -p gen/\$fb_tgt/firebird/examples/udf
	mkdir -p gen/\$fb_tgt/firebird/examples/udr
	mkdir -p gen/\$fb_tgt/firebird/lib
	mkdir -p gen/\$fb_tgt/firebird/misc
	mkdir -p gen/\$fb_tgt/firebird/help
	mkdir -p gen/\$fb_tgt/firebird/plugins/udr

dnl ### TEMP ### directories for generated .cpp, .o and .d by module name
	for src_dir in `cd src; ls -R -1 * | grep : | tr -d : | tr "\n" " "; cd ..`; do
    	mkdir -p temp/\$fb_tgt/\$src_dir
	done

	src/misc/writeBuildNum.sh createMakeVersion gen/Make.Version

dnl # sql files for UDF declarations
	for sql_file in ib_udf.sql fbudf/fbudf.sql ib_udf2.sql; do
		cp src/extlib/\$sql_file gen/\$fb_tgt/firebird/UDF
	done
done

for src_dir in `ls -R -1 examples | grep : | tr -d : | tr "\n" " "`; do
   	mkdir -p temp/\$src_dir
done

dnl # rebuild version header if needed
./src/misc/writeBuildNum.sh rebuildHeader

])


MAKE_SRC_DIR=builds/posix

dnl common files for all posix hosts
dnl TODO: fix "arch-specific/linux/" paths for common posix scripts with SVN
for fb_tgt in $FB_TARGETS; do
AC_CONFIG_FILES([
gen/$fb_tgt/firebird/firebird.conf:builds/install/misc/firebird.conf.in
gen/$fb_tgt/firebird/databases.conf:builds/install/misc/databases.conf.in
gen/$fb_tgt/firebird/fbtrace.conf:src/utilities/ntrace/fbtrace.conf
gen/$fb_tgt/firebird/intl/fbintl.conf:builds/install/misc/fbintl.conf
gen/$fb_tgt/firebird/plugins.conf:builds/install/misc/plugins.conf
])
done

if test "x$CROSS" != "x"; then
AC_CONFIG_FILES([gen/make.crossPlatform:builds/posix/make.$CROSS])
fi

AC_CONFIG_FILES([
gen/Release/firebird/bin/fb_config:builds/install/posix-common/fb_config.in
gen/Release/firebird/bin/posixLibrary.sh:builds/install/posix-common/posixLibrary.sh.in
gen/Release/firebird/bin/tarMainInstall.sh:builds/install/posix-common/tarMainInstall.sh.in
gen/Release/firebird/bin/tarinstall.sh:builds/install/posix-common/tarinstall.sh.in
gen/Release/firebird/bin/preinstall.sh:builds/install/posix-common/preinstall.sh.in
gen/Release/firebird/bin/postinstall.sh:builds/install/posix-common/postinstall.sh.in
gen/Release/firebird/bin/preuninstall.sh:builds/install/posix-common/preuninstall.sh.in
gen/Release/firebird/bin/postuninstall.sh:builds/install/posix-common/postuninstall.sh.in
gen/Release/firebird/bin/taruninstall.sh:builds/install/posix-common/taruninstall.sh.in
gen/Release/firebird/bin/tarMainUninstall.sh:builds/install/posix-common/tarMainUninstall.sh.in
gen/Release/firebird/bin/changeServerMode.sh:builds/install/posix-common/changeServerMode.sh.in
gen/Release/firebird/bin/registerDatabase.sh:builds/install/posix-common/registerDatabase.sh.in
gen/vers.sh:builds/posix/vers.sh.in
])

dnl: Platform Specific Files

case "$PLATFORM" in
  LINUX|GENTOOFREEBSD)
    AC_CONFIG_COMMANDS(,,[
        mkdir -p gen/install/scripts
        mkdir -p gen/install/misc])

    INSTALL_SRC_DIR=builds/install/arch-specific/linux
    AC_CONFIG_FILES([
        gen/install/makeInstallImage.sh:builds/install/arch-specific/linux/makeInstallImage.sh.in
        gen/install/misc/firebird.xinetd:builds/install/arch-specific/linux/firebird.xinetd.in
        gen/install/misc/firebird.init.d.generic:builds/install/arch-specific/linux/firebird.init.d.generic.in
        gen/install/misc/firebird.init.d.mandrake:builds/install/arch-specific/linux/firebird.init.d.mandrake.in
        gen/install/misc/firebird.init.d.suse:builds/install/arch-specific/linux/firebird.init.d.suse.in
        gen/install/misc/firebird.init.d.debian:builds/install/arch-specific/linux/firebird.init.d.debian.in
        gen/install/misc/firebird.init.d.gentoo:builds/install/arch-specific/linux/firebird.init.d.gentoo.in
        gen/install/misc/firebird.init.d.slackware:builds/install/arch-specific/linux/firebird.init.d.slackware.in
        gen/install/misc/rc.config.firebird:builds/install/arch-specific/linux/rc.config.firebird.in
        gen/Release/firebird/bin/linuxLibrary.sh:builds/install/arch-specific/linux/linuxLibrary.sh.in
        gen/install/misc/firebird-classic@.service:builds/install/arch-specific/linux/firebird-classic.service.in
        gen/install/misc/firebird-classic.socket:builds/install/arch-specific/linux/firebird-classic.socket.in
        gen/install/misc/firebird-superserver.service:builds/install/arch-specific/linux/firebird-superserver.service.in
    ],
	[chmod a+x gen/install/scripts/*.sh gen/install/*sh 2>/dev/null])
    ;;

  HPUX)
    HP_INSTALL_SRC_DIR=builds/install/arch-specific/hpux
    AC_CONFIG_FILES([gen/install/makeinstallImage.sh:${HP_INSTALL_SRC_DIR}/makeinstallImage.sh.in
    gen/firebirdCS.psf:${HP_INSTALL_SRC_DIR}/firebirdCS.psf.in
    gen/firebird/bin/preInstall.sh:${HP_INSTALL_SRC_DIR}/preInstall.sh.in
    gen/firebird/bin/tarInstall.sh:${HP_INSTALL_SRC_DIR}/tarInstall.sh.in
    gen/firebird/bin/tarmainInstall.sh:${HP_INSTALL_SRC_DIR}/tarmainInstall.sh.in
    gen/firebird/bin/tarUninstall.sh:${HP_INSTALL_SRC_DIR}/tarUninstall.sh.in
    gen/firebird/bin/tarmainUninstall.sh:${HP_INSTALL_SRC_DIR}/tarmainUninstall.sh.in
    gen/firebird/bin/postInstall.sh:${HP_INSTALL_SRC_DIR}/postInstall.sh.in
    gen/firebird/bin/preUninstall.sh:${HP_INSTALL_SRC_DIR}/preUninstall.sh.in
    gen/firebird/bin/postUninstall.sh:${HP_INSTALL_SRC_DIR}/postUninstall.sh.in
    gen/install/misc/firebird.init.d.hpux:builds/install/arch-specific/hpux/misc/firebird.init.d.hpux.in
    gen/install/misc/rc.config.firebird.hpux:builds/install/arch-specific/hpux/misc/rc.config.firebird.hpux.in
    gen/Release/firebird/bin/hpuxLibrary.sh:builds/install/arch-specific/hpux/misc/hpuxLibrary.sh.in],
    [chmod a+x gen/install/scripts/*.sh gen/install/*.sh 2>/dev/null])
    ;;

  AIX)
    AIX_INSTALL_SRC_DIR=builds/install/arch-specific/aix
    AC_CONFIG_FILES([gen/install/scripts/rpmheader.aix.txt:${AIX_INSTALL_SRC_DIR}/rpmheader.txt.in
    gen/install/scripts/rpmfiles.aix.txt:${AIX_INSTALL_SRC_DIR}/rpmfiles.txt.in
    gen/install/makeInstallImage.aix.sh:${AIX_INSTALL_SRC_DIR}/makeInstallImage.sh.in
    gen/firebird/bin/preinstall.aix.sh:builds/install/arch-specific/aix/misc/preinstall.sh.in
    gen/firebird/bin/tarinstall.aix.sh:builds/install/arch-specific/aix/misc/tarinstall.sh.in
    gen/firebird/bin/tarmaininstall.aix.sh:builds/install/arch-specific/aix/misc/tarmaininstall.sh.in
    gen/firebird/bin/taruninstall.aix.sh:builds/install/arch-specific/aix/misc/taruninstall.sh.in
    gen/firebird/bin/tarmainuninstall.aix.sh:builds/install/arch-specific/aix/misc/tarmainuninstall.sh.in
    gen/firebird/bin/postinstall.aix.sh:builds/install/arch-specific/aix/misc/postinstall.sh.in
    gen/firebird/bin/preuninstall.aix.sh:builds/install/arch-specific/aix/misc/preuninstall.sh.in
    gen/firebird/bin/postuninstall.aix.sh:builds/install/arch-specific/aix/misc/postuninstall.sh.in
    gen/install/misc/firebird.init.d.aix:builds/install/arch-specific/aix/misc/firebird.init.d.aix.in
    gen/install/misc/rc.config.firebird.aix:builds/install/arch-specific/aix/misc/rc.config.firebird.aix.in
    gen/firebird/bin/changedbapassword.aix.sh:builds/install/misc/changeDBAPassword.sh.in
    gen/firebird/bin/changemulticonnectmode.aix.sh:builds/install/misc/changeServerMode.sh.in
    gen/firebird/bin/changegdslibrarycompatiblelink.aix.sh:builds/install/misc/changeGdsLibraryCompatibleLink.sh.in
    gen/firebird/bin/createaliasdb.aix.sh:builds/install/misc/createAliasDB.sh.in
    gen/firebird/bin/aixLibrary.sh:builds/install/arch-specific/aix/misc/aixLibrary.sh.in],
    [chmod a+x gen/install/scripts/*.sh gen/install/*.sh 2>/dev/null])
    ;;
  FREEBSD)
    AC_CONFIG_FILES([gen/install/install.sh:builds/install/arch-specific/freebsd/install.sh.in
                     gen/firebird/bin/freebsdLibrary.sh:builds/install/arch-specific/freebsd/freebsdLibrary.sh.in]
      ,[chmod a+x gen/install/*.sh gen/firebird/bin/*sh 2>/dev/null])
    ;;


  NETBSD)
    AC_CONFIG_FILES([gen/install/install.netbsd.sh:builds/install/arch-specific/netbsd/install.sh.in]
      ,[chmod a+x gen/install/install.netbsd.sh 2>/dev/null])
    ;;


  solx86|SOLARIS)
    AC_CONFIG_FILES([gen/install/Pkginfo:builds/install/arch-specific/solaris/pkginfo.in
                     gen/install/prototype.main:builds/install/arch-specific/solaris/prototype.in
                     gen/install/Preinstall:builds/install/arch-specific/solaris/preinstall.in
                     gen/install/Postinstall:builds/install/arch-specific/solaris/postinstall.in
                     gen/install/Postremove:builds/install/arch-specific/solaris/postremove.in
                     gen/install/prototypelg:builds/install/arch-specific/solaris/libgcc/prototypelg.in
                     gen/install/pkginfolg:builds/install/arch-specific/solaris/libgcc/pkginfolg.in
      ],[chmod a+x gen/install/Pre* gen/install/Post* 2>/dev/null])
    ;;

  DARWIN)
    ;;
esac
if test -f ${MAKE_SRC_DIR}/postfix.${MAKEFILE_POSTFIX};then
	POSTFIX_FILE=gen/make.platform.postfix:${MAKE_SRC_DIR}/postfix.${MAKEFILE_POSTFIX}
	POSTFIX_INCLUDE='include $(ROOT)/gen/make.platform.postfix'
else
	POSTFIX_FILE=
	POSTFIX_INCLUDE=
fi
AC_SUBST(POSTFIX_INCLUDE)

UDF_COMMENT="#"
if test -n "${with_fbudf}"; then
	UDF_COMMENT="#
#  #  For this build Restrict UDF means Restrict ${with_fbudf}
#"
fi
AC_SUBST(UDF_COMMENT)

AC_CONFIG_FILES(
gen/make.rules:${MAKE_SRC_DIR}/make.rules
gen/make.defaults:${MAKE_SRC_DIR}/make.defaults
gen/make.platform:${MAKE_SRC_DIR}/prefix.${MAKEFILE_PREFIX}
gen/darwin.defaults:${MAKE_SRC_DIR}/darwin.defaults
${POSTFIX_FILE}
gen/make.shared.variables:${MAKE_SRC_DIR}/make.shared.variables
gen/make.shared.targets:${MAKE_SRC_DIR}/make.shared.targets
gen/Makefile:${MAKE_SRC_DIR}/Makefile.in
gen/examples/Makefile.examples:${MAKE_SRC_DIR}/Makefile.in.examples
gen/examples/Makefile.plugins_examples:${MAKE_SRC_DIR}/Makefile.in.plugins_examples
gen/Makefile.install:builds/install/arch-specific/${INSTALL_PREFIX}/Makefile.in
Makefile:Makefile.in
)
if test "$STD_EDITLINE" = "false"; then
  AC_CONFIG_FILES(gen/Makefile.extern.editline:${MAKE_SRC_DIR}/Makefile.in.extern.editline)
fi


case "$PLATFORM" in
  SOLARIS | solx86 )
AC_CONFIG_FILES(
gen/make.platform.solaris.examples:${MAKE_SRC_DIR}/make.platform.solaris.examples
)
;;
esac

AC_OUTPUT

dnl Print out the build configuration (the most interesting thing)
echo
echo
echo The Firebird3 package has been configured with the following options:
echo

case $DEVEL_FLG in
 Y)       echo "                 Developer mode : enabled";;
 N)       echo "                 Developer mode : disabled";;
esac

if test $ac_cv_sizeof_void_p -lt 8; then
  case $ac_cv_sys_file_offset_bits in
   no)      echo "64 bit I/O for 32-bit platforms : disabled";;
   64)      echo "64 bit I/O for 32-bit platforms : enabled";;
  esac
fi

case $RAW_DEVICES_FLG in
 Y)       echo "                    Raw devices : enabled";;
 N)       echo "                    Raw devices : disabled";;
esac

echo           "                   Service name : $FB_SERVICE_NAME"
echo           "                   Service port : $FB_SERVICE_PORT"

case "$PLATFORM" in
 win32)   echo "                      Pipe name : $FB_PIPE_NAME";;
esac

echo           "                   GPRE modules : c_cxx.cpp$GPRE_LANGUAGE_MODULES"
echo

echo           "                    Install Dir : $prefix"
echo

dnl EOF
