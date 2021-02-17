dnl $Id: gettimemilli.m4,v 1.1 2006-08-11 12:40:32 freddy77 Exp $
dnl this macros try to detect which is the better implementation 
dnl for a monotonic clock on different platforms
dnl this is the results of some searching starting from 
dnl clock_gettime
dnl
dnl Linux and *BSD: clock_gettime(CLOCK_MONOTONIC)
dnl IRIX: clock_gettime(CLOCK_SGI_CYCLE)
dnl HU-UX: clock_gettime(CLOCK_REALTIME)
dnl   not monotonic but the better I found, same as gettimeofday
dnl Solaris: gethrtime (returns a 64bit, very fine and easy)
dnl Windows: GetTickCount
dnl
dnl I choose millisedonds precision cause this suite very well in 
dnl a unsigned int

dnl check clock_gettime
dnl some implemetations require -lrt or -lposix4 (Solaris)
AC_DEFUN([TDS_CLOCK_GETTIME],
[
  AC_SEARCH_LIBS(clock_gettime, [rt posix4])
  if test "$ac_cv_search_clock_gettime" != no; then
    AC_DEFINE(HAVE_CLOCK_GETTIME, 1, [Define if you have the clock_gettime function.])
  fi
])

dnl check for constants for clock_gettime
AC_DEFUN([TDS_CLOCK_GETTIME_CONST],
[
  AC_MSG_CHECKING(if clock_gettime support $1)
  AC_TRY_COMPILE([
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
], [struct timespec ts; clock_gettime($1, &ts); ], tds_clock_gettime_const=yes, tds_clock_gettime_const=no)
    AC_MSG_RESULT($tds_clock_gettime_const)
])

dnl check all
dnl if windows
dnl 	use GetTickCount
dnl else if gethrtime
dnl 	use gethrtime
dnl else if clock_gettime
dnl 	check CLOCK_MONOTONIC
dnl 	check CLOCK_SGI_CYCLE
dnl	check CLOCK_REALTIME
dnl else if gettimeofday
dnl	use gettimeofday
dnl end

AC_DEFUN([TDS_GETTIMEMILLI],
[
  if test x$ac_cv_func_gethrtime = xno; then
    tds_save_LIBS="$LIBS"
    TDS_CLOCK_GETTIME
    if test "$ac_cv_search_clock_gettime" != no; then
      for tds_const in CLOCK_MONOTONIC CLOCK_SGI_CYCLE CLOCK_REALTIME; do
        TDS_CLOCK_GETTIME_CONST($tds_const)
        if test $tds_clock_gettime_const = yes; then
          AC_DEFINE_UNQUOTED(TDS_GETTIMEMILLI_CONST, [$tds_const], [define to constant to use for clock_gettime])
          break
        fi
      done
      if test $tds_clock_gettime_const != yes; then
        LIBS="$tds_save_LIBS"
      fi
    fi
  fi
])
