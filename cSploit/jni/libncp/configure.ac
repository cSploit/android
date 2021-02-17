dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)
AC_INIT([[ncpfs]],[[2.2.6]],[[vandrove@vc.cvut.cz]])
AC_CONFIG_SRCDIR([[lib/ncplib.c]])
AC_REVISION([[2.2.6]])
AC_CONFIG_HEADER(include/config.h)


AC_ARG_ENABLE(warnings,
[  --enable-warnings       enable verbose build warnings])

AC_ARG_ENABLE(pam,
[  --enable-pam            build PAM NCP authentication module])

AC_ARG_ENABLE(php,
[  --enable-php            build PHP NCP authentication module])

AC_ARG_WITH(php-config,
[  --with-php-config=PATH  build PHP NCP authentication module])

AC_ARG_ENABLE(ipx-tools,
[  --disable-ipx-tools     disables building of ipx tools])

AC_ARG_ENABLE(udp,
[  --disable-udp           disable NCP/UDP])

AC_ARG_ENABLE(ipx,
[  --disable-ipx           disable NCP/IPX])

AC_ARG_ENABLE(signatures,
[  --disable-signatures    disable packet signing code])

AC_ARG_ENABLE(nds,
[  --disable-nds           disable NDS support])

AC_ARG_ENABLE(mount-v2,
[  --disable-mount-v2      disable support for Linux 2.0])

AC_ARG_ENABLE(mount-v3,
[  --disable-mount-v3      disable support for Linux 2.2])

AC_ARG_ENABLE(kernel,
[  --disable-kernel        do not use kernel interface])

AC_ARG_ENABLE(reentrant,
[  --disable-reentrant     do not compile threading support into library])

AC_ARG_ENABLE(versions,
[  --disable-versions      do not use version symbols in library])

AC_ARG_ENABLE(function_sections,
[  --disable-function-sections   do not use -ffunction-sections for static libraries])

AC_ARG_ENABLE(trace,
[  --enable-trace          enable trace of ncp calls])

AC_ARG_ENABLE(ncp_debug,
[  --enable-ncp-debug      enable builtin debugging support])

if test "$enable_udp" = "no"
then
    PROTOCOL_IP=0
else
    AC_DEFINE(CONFIG_NATIVE_IP, 1, 
[If you want native IP support, uncomment CONFIG_NATIVE_IP
Do not forget that it is VERY ALPHA code, with almost no support
Enable only if you are interested in NCP over IP and you want help
me. -A ip.address is your friend. At this time you have to silently
ignore Login to server XXXX as YYYY - think that XXXX is ip.address and
not one taken from .nwclient. Also, if you have not .nwclient, you have
to add -S DUMMY on commandline.])
    PROTOCOL_IP=1
fi

if test "$enable_signatures" = "no"
then : ; else
    AC_DEFINE(SIGNATURES, 1,
[If you want to include packet signature support uncomment this.
If you are using 2.0.x Linux kernel, make sure that you also applied
kernel patches. 
Change only through 'configure', copies of this value are in makefiles])
    SIGNATURES=1
fi

if test "$enable_nds" = "no"
then : ; else
    AC_DEFINE(NDS_SUPPORT, 1,
[If you want to include NDS support, define NDS_SUPPORT symbol
Change only through 'configure', copies of this value are in makefiles])
    NDS_SUPPORT=1
fi

if test "$enable_kernel" = "no"
then 
    USE_KERNEL=0
    enable_mount_v2=no
    enable_mount_v3=no
else
    USE_KERNEL=1
fi

if test "$enable_mount_v2" = "no"
then 
    MOUNT2=0
else
    MOUNT2=1
fi

if test "$enable_mount_v3" = "no"
then
    MOUNT3=0
else
    MOUNT3=1
fi

if test "$enable_warnings" = "yes"
then
    CC_WARNINGS=yes
else
    CC_WARNINGS=no
fi
AC_SUBST(CC_WARNINGS)

if test "$enable_ncp_debug" = "yes"
then
    NCP_DEBUG_ENABLE=yes
    AC_DEFINE(NCP_DEBUG, 1, 
[Enable support for NCP_DEBUG environment variable.])
else
    NCP_DEBUG_ENABLE=no
fi

pam_forced=no
pam_path_forced=
if test "$enable_pam" = ""
then
    PAM=1
else
  pam_forced=yes
  case "$enable_pam" in
  no)	PAM=0; pam_path_forced="no";;
  yes)	PAM=1;;
  /*)	PAM=1; pam_path_forced="$enable_pam";;
  *)	AC_MSG_ERROR(Bad parameter for --enable-pam);;
  esac
fi

php_forced=no
php_path_forced=
PHP=yes
if test "$enable_php" != ""
then
  case "$enable_php" in
  no)	PHP=no; php_path_forced="no";;
  yes)	PHP=yes; php_path_forced=;;
  *)	AC_MSG_ERROR(Bad parameter for --enable-php. Use --with-php-config for specifying PHP directory);;
  esac
  php_forced=yes
fi
if test "$with_php_config" != ""
then
  case "$with_php_config" in
  no)	PHP2=no; php_path_forced="no";;
  yes)  PHP2=yes; php_path_forced=;;
  *)	PHP2=yes; php_path_forced="$with_php_config";;
  esac
  if test "$php_forced" = "yes"
  then
    if test "$PHP" != "$PHP2"
    then
      p1="--enable-php"
      if test "$PHP" = "no"
      then
        p1="--disable-php"
      fi
      case "$php_path_forced" in
        no)  p2="--without-php-config";;
	yes) p2="--with-php-config";;
	*)   p2="--with-php-config=$php_path_forced";;
      esac
      AC_MSG_ERROR($p1 conflicts with $p2);
    fi
  fi
  PHP="${PHP2}"
  php_forced=yes
fi

reentrant_forced=no
REENTRANT=1
if test "$enable_reentrant" != ""
then
    reentrant_forced=yes
    if test "$enable_reentrant" = "no"
    then
        REENTRANT=0
    fi
fi

versions_forced=no
VERSIONED_LINK=yes
if test "$enable_versions" != ""
then
    versions_forced=yes
    if test "$enable_versions" = "no"
    then
	VERSIONED_LINK=no
    fi
fi

function_sections_forced=no
function_sections=yes
if test "$enable_function_sections" != ""
then
    function_sections_forced=yes
    if test "$enable_function_sections" = "no"
    then
        function_sections=no
    fi
fi

if test "$enable_trace" = "yes"
then
    AC_DEFINE(NCP_TRACE_ENABLE, 1, [If you want to trace ncp calls uncomment this.])
    NCP_TRACE_ENABLE=1
fi

PROTOCOL_IPX=0
BUILD_IPX_TOOLS=0

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

dnl Mandatory gettext items
AC_SUBST(PACKAGE, ["$PACKAGE_NAME"])
AC_SUBST(VERSION, ["$PACKAGE_VERSION"])
AM_GNU_GETTEXT
localedir="\${prefix}/$DATADIRNAME/locale"
AC_SUBST(localedir)

dnl Checks for IPX support style
AC_CACHE_CHECK(for GNU style IPX support, ncp_cv_c_ipx_netipx,
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[[#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netipx/ipx.h>]]],
    [[[((struct sockaddr_ipx*)0)->sipx_family == AF_IPX]]])],
    [ncp_cv_c_ipx_netipx="yes"],
    [ncp_cv_c_ipx_netipx="no"]
  )
) 
ncp_ipx_headers_1="/* <asm/types> not required */"
ncp_ipx_headers_2="/* <.../ipx.h> not found */"
if test "$ncp_cv_c_ipx_netipx" = "no"
then
  AC_CACHE_CHECK(for Linux style IPX support, ncp_cv_c_ipx_linux,
    AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[[#include <sys/socket.h>
#include <asm/types.h>
#include <linux/ipx.h>]]],
      [[[((struct sockaddr_ipx*)0)->sipx_family == AF_IPX]]])],
      [ncp_cv_c_ipx_linux="yes"],
      [ncp_cv_c_ipx_linux="no"]
    )
  )
  if test "$ncp_cv_c_ipx_linux" = "no"
  then
    enable_ipx="no"
  else
    AC_DEFINE(HAVE_LINUX_IPX_H, 1, [Define if you have IPX header linux/ipx.h])
    ncp_ipx_headers_1="#include <asm/types.h>"
    ncp_ipx_headers_2="#include <linux/ipx.h>"
  fi
else
  AC_DEFINE(HAVE_NETIPX_IPX_H, 1, [Define if you have IPX header netipx/ipx.h])
  ncp_ipx_headers_1="#include <sys/ioctl.h>"
  ncp_ipx_headers_2="#include <netipx/ipx.h>"
fi
AC_SUBST(ncp_ipx_headers_1)
AC_SUBST(ncp_ipx_headers_2)

dnl If IPX is selected, check some IPX constants
ncp_ipx_defsiocprotoprivate="#if 0 /* #define SIOCPROTOPRIVATE not needed */"
ncp_ipx_defsiocaipx="#if 0/* #define SIOCAIPX* not needed */"
if test "$enable_ipx" = "no"
then : ; else
  AC_DEFINE(CONFIG_NATIVE_IPX, 1, 
[If you do not want IPX support, comment CONFIG_NATIVE_IPX out...
I cannot recommend doing it]) 
  PROTOCOL_IPX=1
  if test "$enable_ipx_tools" != "no"
  then
    ADDON_DEV_SUBDIRS="$ADDON_DEV_SUBDIRS ipx-1.0"
    BUILD_IPX_TOOLS=1
  fi
  
  dnl glibc-2.0.7 and 2.1.x does not have these defines...
  AC_CACHE_CHECK(for working IPX ioctl values, ncp_cv_c_ipx_ioctl,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[$ncp_ipx_headers_1
$ncp_ipx_headers_2]]],
      [[[SIOCAIPXITFCRT, SIOCAIPXPRISLT, SIOCIPXCFGDATA]]])],
      [ncp_cv_c_ipx_ioctl="yes"],
      [ncp_cv_c_ipx_ioctl="no"]
    )
  )
  if test "$ncp_cv_c_ipx_ioctl" = "no"
  then
    AC_CACHE_CHECK(for defined SIOCPROTOPRIVATE, ncp_cv_c_siocprotoprivate,
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[$ncp_ipx_headers_1
$ncp_ipx_headers_2]]],
        [[[SIOCPROTOPRIVATE]]])],
        [ncp_cv_c_siocprotoprivate="yes"],
        [ncp_cv_c_siocprotoprivate="no"]
      )
    )
    if test "$ncp_cv_c_siocprotoprivate" = "no"
    then
      ncp_ipx_defsiocprotoprivate="#if 1 /* SIOCPROTOPRIVATE was not found */"
      AC_CACHE_CHECK(for working IPX ioctl values after defining SIOCPROTOPRIVATE, ncp_cv_c_ipx_sioc,
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[$ncp_ipx_headers_1
$ncp_ipx_headers_2
#define SIOCPROTOPRIVATE 0x89E0]]],
          [[[SIOCAIPXITFCRT, SIOCAIPXPRISLT, SIOCIPXCFGDATA]]])],
          [ncp_cv_c_ipx_sioc="yes"],
          [ncp_cv_c_ipx_sioc="no"]
	)
      )
    else
      ncp_cv_c_ipx_sioc="no"
    fi
    if test "$ncp_cv_c_ipx_sioc" = "no"
    then
      ncp_ipx_defsiocaipx="#if 1 /* SIOCAIPX* values were not defined */"
    fi
  fi
fi
AC_SUBST(ncp_ipx_defsiocprotoprivate)
AC_SUBST(ncp_ipx_defsiocaipx)


dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

dnl sys/mount is required by ncpmount
dnl currently code does not work without sys/mount.h
AC_CHECK_HEADER(sys/mount.h)
if test "$ac_cv_header_sys_mount_h" = "no"
then
  if test "$USE_KERNEL" = "1"
  then
    AC_MSG_WARN(Cannot build kernel support because of no sys/mount.h file)
    USE_KERNEL=0
  fi
  ncp_sys_mount_h="/* sys/mount.h does not exist */"
  ncp_linux_fs_h="/* linux/fs.h is not needed */"
else
  ncp_sys_mount_h="#include <sys/mount.h>"
  AC_CACHE_CHECK(for mount constants in sys/mount.h, ncp_cv_linux_fs_needed,
    AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[[#include <sys/mount.h>]]],
      [[[mount(0,0,0,MS_RDONLY,0); umount(0)]]])],
      [ncp_cv_linux_fs_needed="yes"],
      [ncp_cv_linux_fs_needed="no"]
    )
  ) 
  if test "$ncp_cv_linux_fs_needed" = "yes"; then
    ncp_linux_fs_h="/* linux/fs.h not needed */"
  else
    ncp_linux_fs_h="#include <linux/fs.h>"
  fi
fi
AC_SUBST(ncp_sys_mount_h)
AC_SUBST(ncp_linux_fs_h)

dnl net/if is required by IPX tools
dnl currently code does not work without net/if.h
AC_CHECK_HEADER(net/if.h,
  [ncp_net_if_h="#include <net/if.h>"],
  [AC_CHECK_HEADER(linux/if.h,
    [ncp_net_if_h="#include <linux/if.h>"],
    [ncp_net_if_h="/* No interfaces support... */"
     AC_DEFINE(NO_NET_IF_SUPPORT, 1, [neither net/if.h nor linux/if.h was found, so ipx_* will not build...])
     AC_MSG_WARN(Neither <net/if.h> nor <linux/if.h> does exist, only partial make configured)]
  )]
)
AC_SUBST(ncp_net_if_h)

dnl net/route is required for now in ncplib
dnl it should not be required for IP-only
AC_CHECK_HEADER(net/route.h,
  [ncp_net_route_h="#include <net/route.h>"],
  [AC_CHECK_HEADER(linux/route.h,
    [ncp_net_route_h="#include <linux/route.h>"],
    [ncp_net_route_h="/* No route support... */"
     AC_DEFINE(NO_NET_ROUTE_SUPPORT, 1, [neither net/route.h nor linux/route.h was found ...])
     dnl In future, {net|linux}/route.h should be required only by ipx code
     AC_MSG_ERROR(Neither <net/route.h> nor <linux/route.h> does exist)]
  )]
)
AC_SUBST(ncp_net_route_h)

dnl netinet/if_ether.h vs. linux/if_ether.h
dnl required only in ipxdump/ipxparse
ncp_if_ether_support="yes"
AC_CHECK_HEADER(netinet/if_ether.h)
if test "$ac_cv_header_netinet_if_ether_h" = "no"
then
  AC_CHECK_HEADER(linux/if_ether.h)
  if test "$ac_cv_header_linux_if_ether_h" = "no"
  then
    ncp_if_ether_support="no"
  fi
fi
AC_SUBST(ncp_if_ether_support)

dnl LIBC5 without problems, LIBC6/6.1 are a bit problematic
AC_CHECK_HEADER(sys/select.h,
  [ncp_sys_select_h="#include <sys/select.h>"],
  [ncp_sys_select_h="#include <sys/time.h>"]
)
AC_SUBST(ncp_sys_select_h)

AC_CACHE_CHECK([for C9X integer types],
    ncp_cv_c9x_stdint,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#include <stdint.h>]]],
    [[[int_least16_t sl16;
int_least32_t sl32;
uint_least16_t ul16;
uint_least32_t ul32;
int_fast16_t sf16;
int_fast32_t sf32;
uint_fast16_t uf16;
uint_fast32_t uf32]]])],
    [ncp_cv_c9x_stdint="yes"],
    [ncp_cv_c9x_stdint="no"]
  )
)
if test "$ncp_cv_c9x_stdint" = "yes"
then
  stdint_types="1"
else
  stdint_types="0"
fi
AC_SUBST(stdint_types)

if test "$USE_KERNEL" = "1"
then
  if test "$ncp_sys_select_h" = "#include <sys/select.h>"
  then
    AC_CACHE_CHECK([whether sys/select.h and linux/posix_types.h may both be included], 
        ncp_cv_posix_and_select,
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#include <sys/select.h>
#include <linux/posix_types.h>]]],
        [[[fd_set a;
select(0, &a, &a, &a, 0)]]])],
        [ncp_cv_posix_and_select="yes"],
        [ncp_cv_posix_and_select="no"]
      )
    )
  else
    ncp_cv_posix_and_select="yes"
  fi
  if test "$ncp_cv_posix_and_select" = "no"
  then
    AC_MSG_WARN([sys/select.h and linux/posix_types.h cannot be included together, will not build kernel support])
    USE_KERNEL=0
  fi
fi

dnl Third parameter to accept is socklen_t or signed int
AC_CACHE_CHECK(for third parameter to accept,
    ncp_cv_socklen_t,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#include <sys/socket.h>]]],
    [[[socklen_t a;
     accept(0, NULL, &a)]]])],
    [ncp_cv_socklen_t="socklen_t"],
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#include <sys/socket.h>]]],
      [[[int a;
       accept(0, NULL, &a)]]])],
      [ncp_cv_socklen_t="int"],
      dnl Can you imagine another parameter? Except that sys/socket does not exist...
      [ncp_cv_socklen_t="unsigned int"]
    )
  )
)
if test "$ncp_cv_socklen_t" = "socklen_t"
then 
  socklen_t="/* socklen_t is defined by libc */"
else
  socklen_t="typedef $ncp_cv_socklen_t socklen_t;"
fi
AC_SUBST(socklen_t)

dnl UNICODE
AC_CHECK_TYPE([wchar_t], , [AC_DEFINE([wchar_t], [unsigned int], [Define to 'unsigned int' if wchar_t is not defined.])])
AC_CHECK_HEADERS(wchar.h iconv.h)
if test "$ac_cv_header_wchar_h" = "yes"
then
  wchar_h="#include <wchar.h>"
else
  wchar_h="/* no wchar_h header... */"
fi
AC_SUBST(wchar_h)

if test "$ac_cv_header_iconv_h" = "yes"
then
  AM_ICONV
fi

dnl AC_CHECK_HEADERS(fcntl.h malloc.h paths.h strings.h sys/ioctl.h sys/time.h syslog.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
dnl do we need them? Abort if not present...
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
dnl do we need them? Abort if not present...
dnl AC_FUNC_MEMCMP
dnl AC_TYPE_SIGNAL
dnl AC_FUNC_VPRINTF
dnl AC_CHECK_FUNCS(gettimeofday mktime select socket strdup strerror strspn strstr strtol strtoul uname)
AC_CHECK_FUNCS(wcslen wcscpy wcsdup wcscmp wcsncmp wcscasecmp wcsncasecmp wcsrev)

if test "$PAM" = "1"
then
  AC_CHECK_HEADER(security/pam_appl.h)
  if test "$ac_cv_header_security_pam_appl_h" = "yes" ; then
    if test "$pam_path_forced" = ""; then
      AC_CACHE_CHECK(for pam modules directory,
          ncp_cv_pam_directory,
        if test -d /lib/security ; then
          ncp_cv_pam_directory=/lib/security
        else if test -d /usr/lib/security ; then
          ncp_cv_pam_directory=/usr/lib/security
        else if test -d /usr/local/lib/security ; then
          ncp_cv_pam_directory=/usr/local/lib/security
        else
          dnl defaulting to /lib/security by Linux FS standard
          ncp_cv_pam_directory=/lib/security
        fi; fi; fi
      )
    else
      ncp_cv_pam_directory="$pam_path_forced"
    fi
    LIB_PAM_SECURITY=$ncp_cv_pam_directory

    AC_SUBST(LIB_PAM_SECURITY)
    ADDON_DEV_SUBDIRS="$ADDON_DEV_SUBDIRS contrib/pam"
  else
    if test "$pam_forced" = "yes"
    then
      AC_MSG_ERROR(Cannot build PAM module because of PAM headers are not installed)
    fi
    PAM=0
  fi
fi

if test "$PHP" = "yes"
then
  php_path_config="$php_path_forced"
  if test "$php_path_config" = ""
  then
    php_path_config=php-config
  fi
  case "$php_path_config" in
    /*) AC_MSG_CHECKING([for $php_path_config])
        if test -r "$php_path_config"
        then
	  php_found=yes
	else
	  php_found=no
	fi
	AC_MSG_RESULT([$php_found])
	;;
    *)  AC_CHECK_PROG(php_found, $php_path_config, yes, no) ;;
  esac
  if test "$php_found" = "yes"
  then
    ADDON_SUBDIRS="$ADDON_SUBDIRS contrib/php"
  else
    if test "$php_forced" = "yes"
    then
      AC_MSG_ERROR(Cannot build PHP module because of $php_path_config is not available)
    fi
    php_path_config=""
  fi
fi

if test "$NDS_SUPPORT" = "1"
then
  AC_CACHE_CHECK(for mpilib word size,
      ncp_cv_mpilib_unit,
    [case "$host_cpu" in
      i?86|ia32) ncp_cv_mpilib_unit="32";;
      *) ncp_cv_mpilib_unit="8";
     esac]
    )
  AC_DEFINE_UNQUOTED(UNITSIZE, $ncp_cv_mpilib_unit, [Define to 8 (big-endian) or 16 (32 bit little-endian except i386) or 32 (i386)])
  AC_DEFINE_UNQUOTED(MULTUNITSIZE, $ncp_cv_mpilib_unit, [Define to 8 (big-endian) or 16 (32 bit little-endian except i386) or 32 (i386)])
fi

if test "$REENTRANT" != "0"
then
  AC_CACHE_CHECK(for working pthread support,
      ncp_cv_pthread,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#include <stdlib.h>
#include <pthread.h>]]],
      [[[pthread_mutex_t a;
       pthread_once_t b;
       pthread_mutex_init(&a, NULL);
       pthread_mutex_lock(&a);
       pthread_mutex_unlock(&a);
       pthread_mutex_destroy(&a)]]])],
      [ncp_cv_pthread="yes"],
      [ncp_cv_pthread="no"]
    )
    if test "$ncp_cv_pthread" = "no"
    then
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[[#define _MIT_POSIX_THREADS 1
#include <stdlib.h>
#include <pthread.h>]]],
        [[[pthread_mutex_t a;
         pthread_once_t b;
         pthread_mutex_init(&a, NULL);
         pthread_mutex_lock(&a);
         pthread_mutex_unlock(&a);
         pthread_mutex_destroy(&a)]]])],
        [ncp_cv_pthread="MIT Posix threads needed"],
        [ncp_cv_pthread="no"]
      )
    fi
  )
  case "$ncp_cv_pthread" in
no) if test "$reentrant_forced" = "yes"
      then
        AC_MSG_ERROR(Cannot build threading libraries without working pthread support)
      fi
      REENTRANT=0
      ;;
MIT*) AC_DEFINE(_MIT_POSIX_THREADS, 1, [Define if you need this for correct threading_MIT_POSIX_THREADS ...])
      ;;
  esac
fi  
if test "$REENTRANT" != "0"
then
  AC_CACHE_CHECK(for working weak symbols,
      ncp_cv_weak_symbols,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[[#include <pthread.h>
#pragma weak pthread_mutex_lock]]],
      [[[pthread_mutex_t a;
       pthread_mutex_lock(&a)]]])],
      [ncp_cv_weak_symbols="yes"],
      [ncp_cv_weak_symbols="no"]
    )
  )
  if test "$ncp_cv_weak_symbols" = "no"
  then
    if test "$reentrant_forced" = "yes"
    then
      AC_MSG_ERROR(Cannot build threading libraries without working weak imports)
    fi
    REENTRANT=0  
  fi
fi
if test "$REENTRANT" = "0"
then
  REENTRANT_CFLAGS=""
  
  ncphost=none
  ncphosttested=yes
else
  REENTRANT_CFLAGS="-D_REENTRANT"

  ncphost=generic
  ncphosttested=yes
  case "$host" in
1750a-*)		;;
580-*)			;;
[cjt]90-*)		;;
a29k-*)			;;
alpha-*)		ncphost=alpha; ncphosttested=no ;;
alphaev56-*)		ncphost=alpha; ncphosttested=no ;;
alphaev6[78]-*)		ncphost=alpha; ncphosttested=no ;;
alphaev[4-8]-*)		ncphost=alpha; ncphosttested=no ;;
alphapca5[67]-*)	ncphost=alpha; ncphosttested=no ;;
arc-*)			;;
arm-*)			;;
armbe-*)		;;
armle-*)		;;
armv*-*)		;;
avr-*)			;;
bs2000-*)		;;
c30-*)			;;
c4x-*)			;;
c54x-*)			;;
c[123]*)		;;
clipper-*)		;;
cray2-*)		;;
cris-*)			;;
cydra-*)		;;
d10v-*)			;;
d30v-*)			;;
dsp16xx-*)		;;
elxsi-*)		;;
f30[01]-*)		;;
f700-*)			;;
fr30-*)			;;
fx80-*)			;;
h8300-*)		;;
h8500-*)		;;
hppa-*)			;;
hppa1.0-*)		;;
hppa1.1-*)		;;
hppa2.0-*)		;;
hppa2.0n-*)		;;
hppa2.0w-*)		;;
hppa64-*)		;;
i*86-*)			ncphost=i386; ncphosttested=yes ;;
i370-*)			;;
i860-*)			;;
i960-*)			;;
ia64-*)			;;
m32r-*)			;;
m5200-*)		;;
m680[012346]0-*)	ncphost=m68k; ncphosttested=no ;;
m681[12]-*)		;;
m68360-*)		;;
m683?2-*)		;;
m68hc1[12]-*)		;;
m68k-*)			ncphost=m68k; ncphosttested=no ;;
m88110-*)		;;
m88k-*)			;;
mcore-*)		;;
mips-*)			ncphost=mips; ncphosttested=no ;;
mips16-*)		;;
mips64*-*)		ncphost=mips64; ncphosttested=no ;;
mipsbe-*)		ncphost=mips; ncphosttested=no ;;
mipsel-*)		ncphost=mips; ncphosttested=no ;;
mipsle-*)		ncphost=mips; ncphosttested=no ;;
mipstx39-*)		;;
mipstx39el-*)		;;
mmix-*)			;;
mn10[23]00-*)		;;
none-*)			;;
np1-*)			;;
ns16k-*)		;;
ns32k-*)		;;
nsr-*)			;;
openrisc-*)		;;
orion-*)		;;
pdp10-*)		;;
pdp11-*)		;;
pj-*)			;;
pjl-*)			;;
pn-*)			;;
power-*)		;;
powerpc-*)		ncphost=ppc; ncphosttested=no ;;
powerpcle-*)		ncphost=ppc; ncphosttested=no ;;
ppcbe-*)		;;
pyramid-*)		;;
romp-*)			;;
rs6000-*)		;;
s390-*)			;;
s390x-*)		;;
sh-*)			;;
sh[34]-*)		;;
sh[34]eb-*)		;;
shbe-*)			;;
shle-*)			;;
sparc-*)		;;
sparc64-*)		;;
sparc86x-*)		;;
sparclet-*)		;;
sparclite-*)		;;
sparcv9-*)		;;
sparcv9b-*)		;;
spur-*)			;;
strongarm-*)		;;
sv1-*)			;;
t3e-*)			;;
tahoe-*)		;;
thumb-*)		;;
tic30-*)		;;
tic54x-*)		;;
tic80-*)		;;
tron-*)			;;
v70-*)			;;
v810-*)			;;
v850-*)			;;
vax-*)			;;
w65-*)			;;
we32k-*)		;;
x86-*)			ncphost=i386; ncphosttested=yes ;;
x86_64-*)		;;
xmp-*)			;;
xps100-*)		;;
xscale-*)		;;
ymp-*)			;;
z8k-*)			;;
esac
  AC_CACHE_CHECK(for working atomic operations,
      ncp_cv_asm_atomic_h,
    if test "$ncp_cv_asm_atomic_h" = "generic"
    then
      ncp_cv_asm_atomic_h="pthread based"
    else
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[[#include "include/private/asm-$ncphost/atomic.h"]]],
        [[[ncpt_atomic_t a;
         ncpt_atomic_set(&a,2);
         ncpt_atomic_dec(&a);
         if (ncpt_atomic_read(&a)) {
           if (!ncpt_atomic_dec_and_test(&a)) {
  	     ncpt_atomic_inc(&a);
	   }
         }]]])],
        [ncp_cv_asm_atomic_h="optimized"],
        [ncp_cv_asm_atomic_h="falling back to pthread based"
	 ncphost=generic
	 ncphosttested=yes
	]
      )
    fi
  )
  if test "$ncp_cv_asm_atomic_h" = "falling back to pthread based"
  then
    ncphost=generic
    ncphosttested=yes
  fi
fi
AC_SUBST(REENTRANT_CFLAGS)
AC_SUBST(ncphost)

AC_CACHE_CHECK(for ELF target,
    ncp_cv_elf_target,
[    ncp_cv_elf_target=no
     cat >conftest.c <<EOF
int main(void) { return 0;};
EOF
     if ${CC} ${CFLAGS} -o conftest conftest.c >/dev/null 2>&1
     then
       (file conftest | grep ELF >/dev/null 2>&1) && ncp_cv_elf_target=yes
     fi
     rm -f conftest*
     ]
)
ELF_TARGET="$ncp_cv_elf_target"
AC_SUBST(ELF_TARGET)

FFUNCTION_SECTIONS=""
GC_SECTIONS=""
if test "$function_sections" = "yes"
then
  AC_CACHE_CHECK(for function sections,
      ncp_cv_function_sections,
[    ncp_cv_function_sections=no
    cat >conftest.c <<EOF
int main(void) { return 0;};
EOF
    if ${CC} ${CFLAGS} -ffunction-sections -fdata-sections -Wl,--gc-sections -o conftest conftest.c >/dev/null 2>&1
    then
      ncp_cv_function_sections=yes
    fi
    rm -f conftest*
    ]
  )
  if test "$ncp_cv_function_sections" = "no"
  then
    if test "$function_sections_forced" = "yes"
    then
      AC_MSG_ERROR([Your compiler does not support -ffunction-sections or -fdata-sections or your linker does not support --gc-sections. Please reconfigure without --enable-function-sections.])
    fi
  fi
  function_sections="$ncp_cv_function_sections"
fi
if test "$function_sections" = "yes"
then
    FFUNCTION_SECTIONS="-ffunction-sections -fdata-sections "
    GC_SECTIONS="-Wl,--gc-sections "
fi

if test "$VERSIONED_LINK" = "yes"
then
  AC_CACHE_CHECK(for working versioned link,
      ncp_cv_versioned_link,
[    ncp_cv_versioned_link=no
    cat >conftest.vers <<EOF
VERS_1.0 {
    global: gsym;
            gundef;
    local: lsym;
           lundef;
};
EOF
    cat >conftest.c <<EOF
void lsym(void) { };
void gsym(void) { };
EOF
    if ${CC} -shared -Wl,-version-script=conftest.vers -o conftest.so conftest.c -lc >/dev/null 2>&1
    then
      ncp_cv_versioned_link=yes
    fi]
  )
  rm -f conftest.c conftest.vers conftest.so
  if test "$ncp_cv_versioned_link" = "no"
  then
    if test "$versions_forced" = "yes"
    then
      AC_MSG_ERROR(Cannot use versioned symbols in your environment)
    fi
    VERSIONED_LINK=no
  fi
fi
AC_SUBST(VERSIONED_LINK)

if test "$USE_KERNEL" = "0"
then
    MOUNT2=0
    MOUNT3=0
else
    AC_DEFINE(USE_KERNEL, 1, [Define when compiling for Linux kernel])
fi
if test "$MOUNT2" = "1"
then
    AC_DEFINE(MOUNT2, 1, [If defined, support for Linux2.0.x is compiled in])
fi
if test "$MOUNT3" = "1"
then
    AC_DEFINE(MOUNT3, 1, [If defined, support for Linux2.1-3.x is compiled in])
fi
if test "$PROTOCOL_IP" = "0"
then
    ncp_in_support="#undef NCP_IN_SUPPORT"
else
    ncp_in_support="#define NCP_IN_SUPPORT"
fi
if test "$PROTOCOL_IPX" = "0"
then
    ncp_ipx_support="#undef NCP_IPX_SUPPORT"
else
    ncp_ipx_support="#define NCP_IPX_SUPPORT"
fi
AC_SUBST(ncp_in_support)
AC_SUBST(ncp_ipx_support)

ADDON_SUBDIRS="$ADDON_SUBDIRS $ADDON_DEV_SUBDIRS"

AC_SUBST(NDS_SUPPORT)
AC_SUBST(SIGNATURES)
AC_SUBST(MOUNT2)
AC_SUBST(MOUNT3)
AC_SUBST(USE_KERNEL)
AC_SUBST(ADDON_SUBDIRS)
AC_SUBST(ADDON_DEV_SUBDIRS)
AC_SUBST(FFUNCTION_SECTIONS)
AC_SUBST(GC_SECTIONS)

AC_CONFIG_FILES([
Make.rules
Makefile
lib/Makefile
man/Makefile
util/Makefile
sutil/Makefile
ipx-1.0/Makefile
ipx-1.0/Samples/Makefile
ipxdump/Makefile
contrib/tknwmsg/Makefile
contrib/testing/Makefile
contrib/testing/pp/Makefile
contrib/tcl-utils/Makefile
contrib/tests/Makefile
intl/Makefile
po/Makefile.in
contrib/pam/Makefile
contrib/php/Makefile.in
])
AC_CONFIG_FILES([
include/private/libncp-atomic.h.tmp:include/private/libncp-atomic.h.in
include/ncp/nwnet.h.tmp:include/ncp/nwnet.h.in
include/ncp/ext/stdint.h.tmp:include/ncp/ext/stdint.h.in
include/ncp/ext/socket.h.tmp:include/ncp/ext/socket.h.in
include/ncp/kernel/fs.h.tmp:include/ncp/kernel/fs.h.in
include/ncp/kernel/if.h.tmp:include/ncp/kernel/if.h.in
include/ncp/kernel/ipx.h.tmp:include/ncp/kernel/ipx.h.in
include/ncp/kernel/route.h.tmp:include/ncp/kernel/route.h.in
include/ncp/kernel/types.h.tmp:include/ncp/kernel/types.h.in
], [
my_file=`echo "$ac_file" | sed 's,\.tmp$,,'`
if cmp -s "$my_file" "$ac_file"
then
echo "$as_me: $my_file is unchanged"
rm -f "$ac_file"
else
mv "$ac_file" "$my_file"
fi
])
if test "$php_path_config" != ""
then
  if test "$with_php_config" = "yes"
  then
    AC_MSG_ERROR(Current PHP configure does not handle --with-php-config correctly. Use --with-php-config=php-config instead.)
  fi
  AC_CONFIG_SUBDIRS(contrib/php)
fi
AC_OUTPUT

echo
tmp=""
if test "$BUILD_IPX_TOOLS" = "1"
then
  tmp=" and ipx-1.0"
fi
echo "Building ${PACKAGE_NAME}-${PACKAGE_VERSION}${tmp} with following configuration options:"
if test "$USE_KERNEL" = "1"
then
  tmp="yes"
else
  tmp="no"
fi
echo "kernel support:        ${tmp}"
if test "$tmp" = "yes"
then
  case "${MOUNT3}${MOUNT2}" in
  "00") tmp="none";;
  "01") tmp="linux-2.0";;
  "10") tmp="linux-2.2, linux-2.4";;
  *)    tmp="linux-2.0, linux-2.2, linux-2.4";;
  esac
  echo "ncpmount support:      ${tmp}"
fi
case "${PROTOCOL_IPX}${PROTOCOL_IP}" in
"00") tmp="none";;
"01") tmp="UDP, TCP";;
"10") tmp="IPX";;
*)    tmp="IPX, UDP, TCP";;
esac
echo "NCP supported:         ${tmp}"
if test "$NDS_SUPPORT" = "1"
then
  tmp="yes"
else
  tmp="no"
fi
echo "NDS support:           ${tmp}"

if test "$LIB_PAM_SECURITY" = ""
then
  tmp="no"
else
  tmp="$LIB_PAM_SECURITY"
fi
if test "$pam_forced" = "yes"
then
  if test "$pam_path_forced" = ""
  then
    tmp="${tmp} (forced, path autodetected)"
  else
    tmp="${tmp} (forced)"
  fi
else
  tmp="${tmp} (autodetected)"
fi
echo "PAM support:           ${tmp}"

if test "$php_path_config" = ""
then
  tmp="no"
else
  tmp="$php_path_config"
fi
if test "$php_forced" = "yes"
then
  if test "$php_path_forced" = ""
  then
    tmp="${tmp} (forced, path autodetected)"
  else
    tmp="${tmp} (forced)"
  fi
else
  tmp="${tmp} (autodetected)"
fi
echo "PHP support:           ${tmp}"

if test "$REENTRANT" = "0"
then
  tmp="disabled"
else
  tmp="enabled"
fi
if test "$reentrant_forced" = "yes"
then
  tmp="${tmp} (forced)"
else
  tmp="${tmp} (autodetected)"
fi
echo "Threading support:     ${tmp}"

case "$ncphost" in
none)    tmp="not needed" ;;
generic) tmp="available (pthread based)" ;;
*)       if test "$ncphosttested" = "yes"
         then
	   tmp=""
	 else
	   tmp=", untested" 
	 fi
	 tmp="available (optimized ${ncphost}${tmp})"
	 ;;
esac
echo "Atomic operations:     ${tmp}"

tmp="$VERSIONED_LINK"
if test "$versions_forced" = "yes"
then
  tmp="${tmp} (forced)"
else
  tmp="${tmp} (autodetected)"
fi
echo "Versioned symbols:     ${tmp}"

tmp="$function_sections"
if test "$function_sections_forced" = "yes"
then
  tmp="${tmp} (forced)"
else
  tmp="${tmp} (autodetected)"
fi
echo "Function sections:     ${tmp}"

echo "Compile-time warnings: ${CC_WARNINGS}"
if test "$NCP_TRACE_ENABLE" = "1"
then
  tmp="yes"
else
  tmp="no"
fi
echo "NCP trace:             ${tmp}"
echo "NCP debug:             ${NCP_DEBUG_ENABLE}"
echo
