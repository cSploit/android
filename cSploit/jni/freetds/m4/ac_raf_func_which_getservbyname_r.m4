dnl $Id: ac_raf_func_which_getservbyname_r.m4,v 1.3 2006-03-29 16:24:37 freddy77 Exp $
##
# Found on autoconf archive
# Based on Caolan McNamara's gethostbyname_r macro. 
# Based on David Arnold's autoconf suggestion in the threads faq.

AC_DEFUN([AC_raf_FUNC_WHICH_GETSERVBYNAME_R],
[ac_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $NETWORK_LIBS"
AC_CACHE_CHECK(for which type of getservbyname_r, ac_cv_func_which_getservbyname_r, [
        AC_LINK_IFELSE([AC_LANG_PROGRAM([
#               include <netdb.h>
        ],      [

        char *name;
        char *proto;
        struct servent *se;
        struct servent_data data;
        (void) getservbyname_r(name, proto, se, &data);

                ])],ac_cv_func_which_getservbyname_r=four,
                        [
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#   include <netdb.h>
  ], [[
        char *name;
        char *proto;
        struct servent *se, *res;
        char buffer[2048];
        int buflen = 2048;
        (void) getservbyname_r(name, proto, se, buffer, buflen, &res)
  ]])],ac_cv_func_which_getservbyname_r=six,

  [
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#   include <netdb.h>
  ], [[
        char *name;
        char *proto;
        struct servent *se;
        char buffer[2048];
        int buflen = 2048;
        (void) getservbyname_r(name, proto, se, buffer, buflen)
  ]])],ac_cv_func_which_getservbyname_r=five,ac_cv_func_which_getservbyname_r=no)

  ]

  )
                        ]
                )])

if test $ac_cv_func_which_getservbyname_r = six; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_6, 1, [Define to 1 if your system provides the 6-parameter version of getservbyname_r().])
elif test $ac_cv_func_which_getservbyname_r = five; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_5, 1, [Define to 1 if your system provides the 5-parameter version of getservbyname_r().])
elif test $ac_cv_func_which_getservbyname_r = four; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_4, 1, [Define to 1 if your system provides the 4-parameter version of getservbyname_r().])

fi
CFLAGS=$ac_save_CFLAGS
])

