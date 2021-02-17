dnl $Id: check_openssl.m4,v 1.2 2006-03-27 07:22:54 jklowden Exp $
# OpenSSL check

AC_DEFUN([CHECK_OPENSSL],
[AC_MSG_CHECKING(if openssl is wanted)
AC_ARG_WITH(openssl, AS_HELP_STRING([--with-openssl], [--with-openssl=DIR build with OpenSSL (license NOT compatible cf. User Guide)])
,[ AC_MSG_RESULT(yes)
    for dir in $withval /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr; do
        ssldir="$dir"
        if test -f "$dir/include/openssl/ssl.h"; then
            found_ssl="yes"
            CFLAGS="$CFLAGS -I$ssldir/include"
            break
        fi
    done
    if test x$found_ssl != xyes; then
        AC_MSG_ERROR(Cannot find OpenSSL libraries)
    else
        echo "OpenSSL found in $ssldir"
        NETWORK_LIBS="$NETWORK_LIBS -lssl -lcrypto"
        LDFLAGS="$LDFLAGS -L$ssldir/lib"
        HAVE_OPENSSL=yes
        AC_DEFINE(HAVE_OPENSSL, 1, [Define if you have the OpenSSL.])
    fi
    AC_SUBST(HAVE_OPENSSL)
 ],[
    AC_MSG_RESULT(no)
 ])
])


