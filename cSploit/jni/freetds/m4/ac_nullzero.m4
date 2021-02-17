# TDS_NULL_IS_ZERO ([ACTION-IF-TRUE], [ACTION-IF-FALSE])
# -------------------------------------------------------------------------
AC_DEFUN([TDS_NULL_IS_ZERO],
[AC_CACHE_CHECK([whether memset(0) sets pointers to NULL], tds_cv_null_is_zero,
[
tds_cv_null_is_zero=no
# compile a test program.
AC_RUN_IFELSE(
[AC_LANG_SOURCE([[#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif
int main () { char *p1,*p2; p1=NULL; memset(&p2,0,sizeof(p2));
return memcmp(&p1,&p2,sizeof(char*))?1:0; }]])],
	[tds_cv_null_is_zero=yes],
	[],
[# try to guess the endianness by grepping values into an object file
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[#include <stdio.h>
struct test { char begin[16]; void *ptrs[4]; char end[16]; } xxx[] = {
{ "abcdefghijklmnop", { NULL, NULL, NULL, NULL }, "qrstuvwxyzabcdef" },
{ "\x81\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96\x97", { NULL, NULL, NULL, NULL }, "\x98\x99\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\x81\x82\x83\x84\x85\x86" }
};]],
[[]])],
[if cat -v conftest.$ac_objext|grep 'abcdefghijklmnop\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\(\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\^@\)*qrstuvwxyzabcdef'>/dev/null ; then
	tds_cv_null_is_zero=yes
fi])])])
case $tds_cv_null_is_zero in
	yes)
	m4_default([$1],
	  [AC_DEFINE([NULL_REP_IS_ZERO_BYTES], 1,
	    [Define to 1 if memset(0) sets pointers to NULL.])])
	;;
	no)
	$2
	;;
esac
])# TDS_NULL_IS_ZERO

