dnl $Id: ac_caolan_func_which_gethostbyname_r.m4,v 1.3 2006-03-29 16:24:36 freddy77 Exp $
##
# @synopsis AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R
#
# Provides a test to determine the correct 
# way to call gethostbyname_r
#
# defines HAVE_FUNC_GETHOSTBYNAME_R_6 if it needs 6 arguments (e.g linux)
# defines HAVE_FUNC_GETHOSTBYNAME_R_5 if it needs 5 arguments (e.g. solaris)
# defines HAVE_FUNC_GETHOSTBYNAME_R_3 if it needs 3 arguments (e.g. osf/1)
#
# if used in conjunction in gethostname.c the api demonstrated
# in test.c can be used regardless of which gethostbyname_r 
# exists. These example files found at
# http://www.csn.ul.ie/~caolan/publink/gethostbyname_r
#
# @author Caolan McNamara <caolan@skynet.ie>
# updated with AC_LINK_IFELSE March 2006 <jklowden@schemamania.org>.
# Could probably be replaced with better version: 
# 	http://autoconf-archive.cryp.to/ax_func_which_gethostbyname_r.html
#
# based on David Arnold's autoconf suggestion in the threads faq
##
AC_DEFUN([AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R],
[ac_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $NETWORK_LIBS"
AC_CACHE_CHECK(for which type of gethostbyname_r, ac_cv_func_which_gethostname_r, [
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
#		include <netdb.h> 
  	], 	[

        char *name;
        struct hostent *he;
        struct hostent_data data;
        (void) gethostbyname_r(name, he, &data);

		])],ac_cv_func_which_gethostname_r=three, 
			[
dnl			ac_cv_func_which_gethostname_r=no
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#   include <netdb.h>
  ], [[
	char *name;
	struct hostent *he, *res;
	char buffer[2048];
	int buflen = 2048;
	int h_errnop;
	(void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
  ]])],ac_cv_func_which_gethostname_r=six,
  
  [
dnl  ac_cv_func_which_gethostname_r=no
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#   include <netdb.h>
  ], [[
			char *name;
			struct hostent *he;
			char buffer[2048];
			int buflen = 2048;
			int h_errnop;
			(void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
  ]])],ac_cv_func_which_gethostname_r=five,ac_cv_func_which_gethostname_r=no)

  ]
  
  )
			]
		)])

if test $ac_cv_func_which_gethostname_r = six; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_6, 1, [Define to 1 if your system provides the 6-parameter version of gethostbyname_r().])
elif test $ac_cv_func_which_gethostname_r = five; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_5, 1, [Define to 1 if your system provides the 5-parameter version of gethostbyname_r().])
elif test $ac_cv_func_which_gethostname_r = three; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_3, 1, [Define to 1 if your system provides the 3-parameter version of gethostbyname_r().])

fi
CFLAGS=$ac_save_CFLAGS
])


