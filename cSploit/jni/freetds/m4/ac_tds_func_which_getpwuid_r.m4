dnl $Id: ac_tds_func_which_getpwuid_r.m4,v 1.4 2006-08-23 14:26:20 freddy77 Exp $
##
# Check getpwuid_r parameters
# There are three version of this function
#   int  getpwuid_r(uid_t uid, struct passwd *result, char *buffer, int buflen);
#   (hp/ux 10.20, digital unix 4)
#   struct passwd *getpwuid_r(uid_t uid, struct passwd * pwd, char *buffer, int buflen);
#   (SunOS 5.5, many other)
#   int  getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer, size_t buflen, struct passwd **result);
#   (hp/ux 11, many other, posix compliant)
##
AC_DEFUN([AC_tds_FUNC_WHICH_GETPWUID_R],
[if test x$ac_cv_func_getpwuid = xyes; then
AC_CACHE_CHECK(for which type of getpwuid_r, ac_cv_func_which_getpwuid_r, [
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <unistd.h>
#include <pwd.h>
  ], [[
struct passwd bpw;
char buf[1024];
char *dir = getpwuid_r(getuid(), &bpw, buf, sizeof(buf))->pw_dir;
]])],ac_cv_func_which_getpwuid_r=four_pw,
[AC_TRY_RUN([
#include <unistd.h>
#include <pwd.h>
int main() {
struct passwd bpw;
char buf[1024];
getpwuid_r(getuid(), &bpw, buf, sizeof(buf));
return 0;
}
],ac_cv_func_which_getpwuid_r=four, 
  [AC_TRY_RUN([
#include <unistd.h>
#include <pwd.h>
int main() {
struct passwd *pw, bpw;
char buf[1024];
getpwuid_r(getuid(), &bpw, buf, sizeof(buf), &pw);
return 0;
}
],ac_cv_func_which_getpwuid_r=five,
ac_cv_func_which_getpwuid_r=no)],
[# cross compile case
ac_cv_func_which_getpwuid_r=no
num_params=four
for params in "int" "size_t, struct passwd **"; do
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#include <unistd.h>
#include <pwd.h>
extern int getpwuid_r(uid_t, struct passwd *, char *, $params);
          ],[])],[
	if test $ac_cv_func_which_getpwuid_r != no; then
		AC_ERROR([Two types of getpwuid_r detected])
	fi
	ac_cv_func_which_getpwuid_r=$num_params
	])
	num_params=five
done
]
)]
)])

if test $ac_cv_func_which_getpwuid_r = four_pw; then
  AC_DEFINE(HAVE_FUNC_GETPWUID_R_4, 1, [Define to 1 if your system provides the 4-parameter version of getpwuid_r().])
  AC_DEFINE(HAVE_FUNC_GETPWUID_R_4_PW, 1, [Define to 1 if your system getpwuid_r() have 4 parameters and return struct passwd*.])
elif test $ac_cv_func_which_getpwuid_r = four; then
  AC_DEFINE(HAVE_FUNC_GETPWUID_R_4, 1, [Define to 1 if your system provides the 4-parameter version of getpwuid_r().])
elif test $ac_cv_func_which_getpwuid_r = five; then
  AC_DEFINE(HAVE_FUNC_GETPWUID_R_5, 1, [Define to 1 if your system provides the 5-parameter version of getpwuid_r().])
fi

fi
])

