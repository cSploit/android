#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([network-radar], [1.0], [])
AC_CONFIG_SRCDIR([nbns.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([csploit-common], [sortedarray_get], [], [
  AC_MSG_ERROR([libcsploit-common is required])
])
AC_CHECK_LIB([cares], [ares_init_options], [], [
  AC_MSG_ERROR([libcares is required])
])
AC_CHECK_LIB([m], [pow], [], [
  AC_MSG_ERROR([missing math library])
])
AC_CHECK_LIB([pcap], [pcap_create])

AC_CHECK_FUNCS([pthread_create], [], [
  AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([libpthread not found])])])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h limits.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h], [], [
  AC_MSG_ERROR([some header is missing])])

AC_CHECK_HEADERS([linux/if_ether.h net/ethernet.h net/if_arp.h netinet/ip.h netinet/udp.h])

AC_CHECK_MEMBER([struct ether_arp.arp_spa], [
  AC_DEFINE([HAVE_NETINET_IF_ETHER_H], [], [
    Define to 1 if you have the <netinet/if_ether.h> header file.])
  ], [], [[#include <netinet/if_ether.h>]])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Custom stuff

AC_PREPROC_IFELSE([
  AC_LANG_PROGRAM([[#include <stddef.h>]], [[
    #ifdef __BIONIC__
    # error "bionic found"
    #endif
    ]])], [], [
    AC_DEFINE([BUG81370], [], [Workaround android bug #81370])])

AC_ARG_ENABLE([profile],
    AS_HELP_STRING([--enable-profile], [Enable profiling]))

AS_IF([test "x$enable_profile" = "xyes"], [
  AC_DEFINE([PROFILE], [], [Porfile])
])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset pow select socket strdup strerror strndup strrchr strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
