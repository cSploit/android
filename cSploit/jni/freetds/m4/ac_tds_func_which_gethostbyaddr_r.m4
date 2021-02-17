dnl $Id: ac_tds_func_which_gethostbyaddr_r.m4,v 1.3 2006-03-29 16:24:37 freddy77 Exp $
##
# based on gethostbyname_r check and snippits from curl's check
##
AC_DEFUN([AC_tds_FUNC_WHICH_GETHOSTBYADDR_R],
[ac_save_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $NETWORK_LIBS"
AC_CACHE_CHECK(for which type of gethostbyaddr_r, ac_cv_func_which_gethostbyaddr_r, [
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
#include <sys/types.h>
#include <netdb.h>
  	], 	[
char * address;
int length;
int type;
struct hostent h;
struct hostent_data hdata;
int rc;
rc = gethostbyaddr_r(address, length, type, &h, &hdata);

])],ac_cv_func_which_gethostbyaddr_r=five, 
  [
dnl			ac_cv_func_which_gethostbyaddr_r=no
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#include <sys/types.h>
#include <netdb.h>
  ], [[
char * address;
int length;
int type;
struct hostent h;
char buffer[8192];
int h_errnop;
struct hostent * hp;

hp = gethostbyaddr_r(address, length, type, &h,
                     buffer, 8192, &h_errnop);

]])],ac_cv_func_which_gethostbyaddr_r=seven,
  
 [
dnl  ac_cv_func_which_gethostbyaddr_r=no
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
#include <sys/types.h>
#include <netdb.h>
  ], [[
char * address;
int length;
int type;
struct hostent h;
char buffer[8192];
int h_errnop;
struct hostent * hp;
int rc;

rc = gethostbyaddr_r(address, length, type, &h,
                     buffer, 8192, &hp, &h_errnop);

]])],ac_cv_func_which_gethostbyaddr_r=eight,ac_cv_func_which_gethostbyaddr_r=no)

]
  )
			]
		)])

if test $ac_cv_func_which_gethostbyaddr_r = eight; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYADDR_R_8, 1, [Define to 1 if your system provides the 8-parameter version of gethostbyaddr_r().])
elif test $ac_cv_func_which_gethostbyaddr_r = seven; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYADDR_R_7, 1, [Define to 1 if your system provides the 7-parameter version of gethostbyaddr_r().])
elif test $ac_cv_func_which_gethostbyaddr_r = five; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYADDR_R_5, 1, [Define to 1 if your system provides the 5-parameter version of gethostbyaddr_r().])

fi
CFLAGS=$ac_save_CFLAGS
])

