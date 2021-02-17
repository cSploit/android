dnl $Id: type_socklen_t.m4,v 1.3 2006-03-29 16:24:37 freddy77 Exp $
##
# This macro came from internet, appear in lftp, rsync and others.
##
AC_DEFUN([TYPE_SOCKLEN_T],
[
  AC_CHECK_TYPE([socklen_t], ,[
    AC_MSG_CHECKING([for socklen_t equivalent])
    AC_CACHE_VAL([xml_cv_socklen_t_equiv],
    [
      # Systems have either "struct sockaddr *" or
      # "void *" as the second argument to getpeername
      xml_cv_socklen_t_equiv=
      for arg2 in "struct sockaddr" void; do
        for t in int size_t unsigned long "unsigned long"; do
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
# include <ws2tcpip.h>
int PASCAL getpeername (SOCKET, $arg2 *, $t *);
#elif defined(HAVE_SYS_SOCKET_H)
# include <sys/socket.h>
int getpeername (int, $arg2 *, $t *);
#endif

          ],[
            $t len;
            getpeername(0,0,&len);
          ])],[
            xml_cv_socklen_t_equiv="$t"
            break
          ])
        done
      done

      if test "x$xml_cv_socklen_t_equiv" = x; then
        AC_MSG_ERROR([Cannot find a type to use in place of socklen_t])
      fi
    ])
    AC_MSG_RESULT($xml_cv_socklen_t_equiv)
    AC_DEFINE_UNQUOTED(socklen_t, $xml_cv_socklen_t_equiv,
                      [type to use in place of socklen_t if not defined])],
    [#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
# include <ws2tcpip.h>
#endif])
])

