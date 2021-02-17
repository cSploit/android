##   -*- autoconf -*-

dnl unixODBC
dnl
dnl This file has been customized for unixODBC.
dnl

dnl AC_CHECK_LIBPT_NOC(LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN([AC_CHECK_LIBPT_NOC],
[AC_MSG_CHECKING([for $2 in -l$1])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $1 defining function $2, not
dnl just for library $1.  Separate tests with the same $1 and different $2s
dnl may have different results.
ac_save_LIBS="$LIBS"
LIBS="-l$1 $5 $LIBS"
AC_TRY_LINK(dnl
[ #ifdef __cplusplus
extern "C"
#endif
#include <pthread.h>
],
[$2(0)],
eval "ac_cv_lib_$ac_lib_var=yes",
eval "ac_cv_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
dnl
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
AC_MSG_RESULT(yes)
ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$1 $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi
])

dnl Check if the compiler works with a given command line option
dnl AC_CHECK_COMP_OPT(OPTION)
AC_DEFUN([AC_CHECK_COMP_OPT],
[AC_MSG_CHECKING([if compiler accepts -$1])
echo 'void f(){}' > conftest.c
if test -z "`${CC-cc} -$1 -c conftest.c 2>&1`"; then
  AC_MSG_RESULT(yes)
  CFLAGS="$CFLAGS -$1"
else
  AC_MSG_RESULT(no)
fi
rm -f conftest*
])

dnl Check for a lib, without checking the cache first
dnl AC_CHECK_LIB_NOC(LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN([AC_CHECK_LIB_NOC],
[AC_MSG_CHECKING([for $2 in -l$1 $5])
ac_save_LIBS="$LIBS"
LIBS="-l$1 $5 $LIBS"

AC_TRY_LINK(dnl
[/* Override any gcc2 internal prototype to avoid an error.  */
#ifdef __cplusplus
extern "C"
#endif
/* We use char because int might match the return type of a gcc2
    builtin and then its argument prototype would still apply.  */
char $2();
],
[$2()],
eval "ac_cv_lib_$ac_lib_var=yes",
eval "ac_cv_lib_$ac_lib_var=no")

LIBS="$ac_save_LIBS"

if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$3], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $1 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$1 $LIBS"
], [$3])
else
  AC_MSG_RESULT(no)
ifelse([$4], , , [$4
])dnl
fi
])

dnl ##
dnl ##  GNU Pth - The GNU Portable Threads
dnl ##  Copyright (c) 1999-2000 Ralf S. Engelschall <rse@engelschall.com>
dnl ##
dnl ##  This file is part of GNU Pth, a non-preemptive thread scheduling
dnl ##  library which can be found at http://www.gnu.org/software/pth/.
dnl ##
dnl ##  This library is free software; you can redistribute it and/or
dnl ##  modify it under the terms of the GNU Lesser General Public
dnl ##  License as published by the Free Software Foundation; either
dnl ##  version 2.1 of the License, or (at your option) any later version.
dnl ##
dnl ##  This library is distributed in the hope that it will be useful,
dnl ##  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl ##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
dnl ##  Lesser General Public License for more details.
dnl ##
dnl ##  You should have received a copy of the GNU Lesser General Public
dnl ##  License along with this library; if not, write to the Free Software
dnl ##  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
dnl ##  USA, or contact Ralf S. Engelschall <rse@engelschall.com>.
dnl ##
dnl ##  pth.m4: Autoconf macro for locating GNU Pth from within
dnl ##          configure.in of third-party software packages
dnl ##

dnl ##
dnl ##  Synopsis:
dnl ##  AC_CHECK_PTH([MIN-VERSION [,          # minimum Pth version, e.g. 1.2b3
dnl ##                DEFAULT-WITH-PTH [,     # default value for --with-pth option
dnl ##                DEFAULT-WITH-PTH-TEST [,# default value for --with-pth-test option
dnl ##                EXTEND-VARS [,          # whether CFLAGS/LDFLAGS/etc are extended
dnl ##                ACTION-IF-FOUND [,      # action to perform if Pth was found
dnl ##                ACTION-IF-NOT-FOUND     # action to perform if Pth was not found
dnl ##                ]]]]]])
dnl ##  Examples:
dnl ##  AC_CHECK_PTH(1.2.0)
dnl ##  AC_CHECK_PTH(1.2.0,,,no,CFLAGS="$CFLAGS -DHAVE_PTH $PTH_CFLAGS")
dnl ##  AC_CHECK_PTH(1.2.0,yes,yes,yes,CFLAGS="$CFLAGS -DHAVE_PTH")
dnl ##
dnl
dnl #   auxilliary macros
AC_DEFUN([_AC_PTH_ERROR], [dnl
AC_MSG_RESULT([*FAILED*])
echo " +------------------------------------------------------------------------+" 1>&2
cat <<EOT | sed -e 's/^[[ 	]]*/ | /' -e 's/>>/  /' 1>&2
$1
EOT
echo " +------------------------------------------------------------------------+" 1>&2
exit 1
])
AC_DEFUN([_AC_PTH_VERBOSE], [dnl
if test ".$verbose" = .yes; then
    AC_MSG_RESULT([  $1])
fi
])
dnl #   the user macro
AC_DEFUN([AC_CHECK_PTH], [dnl
dnl
dnl #   prerequisites
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
dnl
PTH_CPPFLAGS=''
PTH_CFLAGS=''
PTH_LDFLAGS=''
PTH_LIBS=''
AC_SUBST(PTH_CPPFLAGS)
AC_SUBST(PTH_CFLAGS)
AC_SUBST(PTH_LDFLAGS)
AC_SUBST(PTH_LIBS)
dnl #   command line options
AC_MSG_CHECKING(for GNU Pth)
_AC_PTH_VERBOSE([])
AC_ARG_WITH(pth,dnl
[  --with-pth[=ARG]        Build with GNU Pth Library  (default=]ifelse([$2],,yes,$2)[)],dnl
,dnl
with_pth="ifelse([$2],,yes,$2)"
)dnl
AC_ARG_WITH(pth-test,dnl
[  --with-pth-test         Perform GNU Pth Sanity Test (default=]ifelse([$3],,yes,$3)[)],dnl
,dnl
with_pth_test="ifelse([$3],,yes,$3)"
)dnl
_AC_PTH_VERBOSE([+ Command Line Options:])
_AC_PTH_VERBOSE([    o --with-pth=$with_pth])
_AC_PTH_VERBOSE([    o --with-pth-test=$with_pth_test])
dnl
dnl #   configuration
if test ".$with_pth" != .no; then
    _pth_subdir=no
    _pth_subdir_opts=''
    case "$with_pth" in
        subdir:* )
            _pth_subdir=yes
            changequote(, )dnl
            _pth_subdir_opts=`echo $with_pth | sed -e 's/^subdir:[^ 	]*[ 	]*//'`
            with_pth=`echo $with_pth | sed -e 's/^subdir:\([^ 	]*\).*$/\1/'`
            changequote([, ])dnl
            ;;
    esac
    _pth_version=""
    _pth_location=""
    _pth_type=""
    _pth_cppflags=""
    _pth_cflags=""
    _pth_ldflags=""
    _pth_libs=""
    if test ".$with_pth" = .yes; then
        #   via config script in $PATH
        changequote(, )dnl
        _pth_version=`(pth-config --version) 2>/dev/null |\
                      sed -e 's/^.*\([0-9]\.[0-9]*[ab.][0-9]*\).*$/\1/'`
        changequote([, ])dnl
        if test ".$_pth_version" != .; then
            _pth_location=`pth-config --prefix`
            _pth_type='installed'
            _pth_cppflags=`pth-config --cflags`
            _pth_cflags=`pth-config --cflags`
            _pth_ldflags=`pth-config --ldflags`
            _pth_libs=`pth-config --libs`
        fi
    elif test -d "$with_pth"; then
        with_pth=`echo $with_pth | sed -e 's;/*$;;'`
        _pth_found=no
        #   via locally included source tree
        if test ".$_pth_subdir" = .yes; then
            _pth_location="$with_pth"
            _pth_type='local'
            _pth_cppflags="-I$with_pth"
            _pth_cflags="-I$with_pth"
            if test -f "$with_pth/ltconfig"; then
                _pth_ldflags="-L$with_pth/.libs"
            else
                _pth_ldflags="-L$with_pth"
            fi
            _pth_libs="-lpth"
            changequote(, )dnl
            _pth_version=`grep '^const char PTH_Hello' $with_pth/pth_vers.c |\
                          sed -e 's;^.*Version[ 	]*\([0-9]*\.[0-9]*[.ab][0-9]*\)[ 	].*$;\1;'`
            changequote([, ])dnl
            _pth_found=yes
            ac_configure_args="$ac_configure_args --enable-subdir $_pth_subdir_opts"
            with_pth_test=no
        fi
        #   via config script under a specified directory
        #   (a standard installation, but not a source tree)
        if test ".$_pth_found" = .no; then
            for _dir in $with_pth/bin $with_pth; do
                if test -f "$_dir/pth-config"; then
                    test -f "$_dir/pth-config.in" && continue # pth-config in source tree!
                    changequote(, )dnl
                    _pth_version=`($_dir/pth-config --version) 2>/dev/null |\
                                  sed -e 's/^.*\([0-9]\.[0-9]*[ab.][0-9]*\).*$/\1/'`
                    changequote([, ])dnl
                    if test ".$_pth_version" != .; then
                        _pth_location=`$_dir/pth-config --prefix`
                        _pth_type="installed"
                        _pth_cppflags=`$_dir/pth-config --cflags`
                        _pth_cflags=`$_dir/pth-config --cflags`
                        _pth_ldflags=`$_dir/pth-config --ldflags`
                        _pth_libs=`$_dir/pth-config --libs`
                        _pth_found=yes
                        break
                    fi
                fi
            done
        fi
        #   in any subarea under a specified directory
        #   (either a special installation or a Pth source tree)
        if test ".$_pth_found" = .no; then
            changequote(, )dnl
            _pth_found=0
            for _file in x `find $with_pth -name "pth.h" -type f -print`; do
                test .$_file = .x && continue
                _dir=`echo $_file | sed -e 's;[^/]*$;;' -e 's;\(.\)/$;\1;'`
                _pth_version=`($_dir/pth-config --version) 2>/dev/null |\
                              sed -e 's/^.*\([0-9]\.[0-9]*[ab.][0-9]*\).*$/\1/'`
                if test ".$_pth_version" = .; then
                    _pth_version=`grep '^#define PTH_VERSION_STR' $_file |\
                                  sed -e 's;^#define[ 	]*PTH_VERSION_STR[ 	]*"\([0-9]*\.[0-9]*[.ab][0-9]*\)[ 	].*$;\1;'`
                fi
                _pth_cppflags="-I$_dir"
                _pth_cflags="-I$_dir"
                _pth_found=`expr $_pth_found + 1`
            done
            for _file in x `find $with_pth -name "libpth.[aso]" -type f -print`; do
                test .$_file = .x && continue
                _dir=`echo $_file | sed -e 's;[^/]*$;;' -e 's;\(.\)/$;\1;'`
                _pth_ldflags="-L$_dir"
                _pth_libs="-lpth"
                _pth_found=`expr $_pth_found + 1`
            done
            changequote([, ])dnl
            if test ".$_pth_found" = .2; then
                _pth_location="$with_pth"
                _pth_type="uninstalled"
            else
                _pth_version=''
            fi
        fi
    fi
    _AC_PTH_VERBOSE([+ Determined Location:])
    _AC_PTH_VERBOSE([    o path: $_pth_location])
    _AC_PTH_VERBOSE([    o type: $_pth_type])
    if test ".$_pth_version" = .; then
        if test ".$with_pth" != .yes; then
             _AC_PTH_ERROR([dnl
             Unable to locate GNU Pth under $with_pth.
             Please specify the correct path to either a GNU Pth installation tree
             (use --with-pth=DIR if you used --prefix=DIR for installing GNU Pth in
             the past) or to a GNU Pth source tree (use --with-pth=DIR if DIR is a
             path to a pth-X.Y.Z/ directory; but make sure the package is already
             built, i.e., the "configure; make" step was already performed there).])
        else
             _AC_PTH_ERROR([dnl
             Unable to locate GNU Pth in any system-wide location (see \$PATH).
             Please specify the correct path to either a GNU Pth installation tree
             (use --with-pth=DIR if you used --prefix=DIR for installing GNU Pth in
             the past) or to a GNU Pth source tree (use --with-pth=DIR if DIR is a
             path to a pth-X.Y.Z/ directory; but make sure the package is already
             built, i.e., the "configure; make" step was already performed there).])
        fi
    fi
    dnl #
    dnl #  Check whether the found version is sufficiently new
    dnl #
    _req_version="ifelse([$1],,1.0.0,$1)"
    for _var in _pth_version _req_version; do
        eval "_val=\"\$${_var}\""
        _major=`echo $_val | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\([[ab.]]\)\([[0-9]]*\)/\1/'`
        _minor=`echo $_val | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\([[ab.]]\)\([[0-9]]*\)/\2/'`
        _rtype=`echo $_val | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\([[ab.]]\)\([[0-9]]*\)/\3/'`
        _micro=`echo $_val | sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\([[ab.]]\)\([[0-9]]*\)/\4/'`
        case $_rtype in
            "a" ) _rtype=0 ;;
            "b" ) _rtype=1 ;;
            "." ) _rtype=2 ;;
        esac
        _hex=`echo dummy | awk '{ printf("%d%02d%1d%02d", major, minor, rtype, micro); }' \
              "major=$_major" "minor=$_minor" "rtype=$_rtype" "micro=$_micro"`
        eval "${_var}_hex=\"\$_hex\""
    done
    _AC_PTH_VERBOSE([+ Determined Versions:])
    _AC_PTH_VERBOSE([    o existing: $_pth_version -> 0x$_pth_version_hex])
    _AC_PTH_VERBOSE([    o required: $_req_version -> 0x$_req_version_hex])
    _ok=0
    if test ".$_pth_version_hex" != .; then
        if test ".$_req_version_hex" != .; then
            if test $_pth_version_hex -ge $_req_version_hex; then
                _ok=1
            fi
        fi
    fi
    if test ".$_ok" = .0; then
        _AC_PTH_ERROR([dnl
        Found Pth version $_pth_version, but required at least version $_req_version.
        Upgrade Pth under $_pth_location to $_req_version or higher first, please.])
    fi
    dnl #
    dnl #   Perform Pth Sanity Compile Check
    dnl #
    if test ".$with_pth_test" = .yes; then
        _ac_save_CPPFLAGS="$CPPFLAGS"
        _ac_save_CFLAGS="$CFLAGS"
        _ac_save_LDFLAGS="$LDFLAGS"
        _ac_save_LIBS="$LIBS"
        CPPFLAGS="$CPPFLAGS $_pth_cppflags"
        CFLAGS="$CFLAGS $_pth_cflags"
        LDFLAGS="$LDFLAGS $_pth_ldflags"
        LIBS="$LIBS $_pth_libs"
        _AC_PTH_VERBOSE([+ Test Build Environment:])
        _AC_PTH_VERBOSE([    o CPPFLAGS=\"$CPPFLAGS\"])
        _AC_PTH_VERBOSE([    o CFLAGS=\"$CFLAGS\"])
        _AC_PTH_VERBOSE([    o LDFLAGS=\"$LDFLAGS\"])
        _AC_PTH_VERBOSE([    o LIBS=\"$LIBS\"])
        cross_compile=no
        define(_code1, [dnl
        #include <stdio.h>
        #include <pth.h>
        ])
        define(_code2, [dnl
        int main(int argc, char *argv[])
        {
            FILE *fp;
            if (!(fp = fopen("conftestval", "w")))
                exit(1);
            fprintf(fp, "hmm");
            fclose(fp);
            pth_init();
            pth_kill();
            if (!(fp = fopen("conftestval", "w")))
                exit(1);
            fprintf(fp, "yes");
            fclose(fp);
            exit(0);
        }
        ])
        _AC_PTH_VERBOSE([+ Performing Sanity Checks:])
        _AC_PTH_VERBOSE([    o pre-processor test])
        AC_TRY_CPP(_code1, _ok=yes, _ok=no)
        if test ".$_ok" != .yes; then
            _AC_PTH_ERROR([dnl
            Found GNU Pth $_pth_version under $_pth_location, but
            was unable to perform a sanity pre-processor check. This means
            the GNU Pth header pth.h was not found.
            We used the following build environment:
            >> CPP="$CPP"
            >> CPPFLAGS="$CPPFLAGS"
            See config.log for possibly more details.])
        fi
        _AC_PTH_VERBOSE([    o link check])
        AC_TRY_LINK(_code1, _code2, _ok=yes, _ok=no)
        if test ".$_ok" != .yes; then
            _AC_PTH_ERROR([dnl
            Found GNU Pth $_pth_version under $_pth_location, but
            was unable to perform a sanity linker check. This means
            the GNU Pth library libpth.a was not found.
            We used the following build environment:
            >> CC="$CC"
            >> CFLAGS="$CFLAGS"
            >> LDFLAGS="$LDFLAGS"
            >> LIBS="$LIBS"
            See config.log for possibly more details.])
        fi
        _AC_PTH_VERBOSE([    o run-time check])
        AC_TRY_RUN(_code1 _code2, _ok=`cat conftestval`, _ok=no, _ok=no)
        if test ".$_ok" != .yes; then
            if test ".$_ok" = .no; then
                _AC_PTH_ERROR([dnl
                Found GNU Pth $_pth_version under $_pth_location, but
                was unable to perform a sanity execution check. This usually
                means that the GNU Pth shared library libpth.so is present
                but \$LD_LIBRARY_PATH is incomplete to execute a Pth test.
                In this case either disable this test via --without-pth-test,
                or extend \$LD_LIBRARY_PATH, or build GNU Pth as a static
                library only via its --disable-shared Autoconf option.
                We used the following build environment:
                >> CC="$CC"
                >> CFLAGS="$CFLAGS"
                >> LDFLAGS="$LDFLAGS"
                >> LIBS="$LIBS"
                See config.log for possibly more details.])
            else
                _AC_PTH_ERROR([dnl
                Found GNU Pth $_pth_version under $_pth_location, but
                was unable to perform a sanity run-time check. This usually
                means that the GNU Pth library failed to work and possibly
                caused a core dump in the test program. In this case it
                is strongly recommended that you re-install GNU Pth and this
                time make sure that it really passes its "make test" procedure.
                We used the following build environment:
                >> CC="$CC"
                >> CFLAGS="$CFLAGS"
                >> LDFLAGS="$LDFLAGS"
                >> LIBS="$LIBS"
                See config.log for possibly more details.])
            fi
        fi
        _extendvars="ifelse([$4],,yes,$4)"
        if test ".$_extendvars" != .yes; then
            CPPFLAGS="$_ac_save_CPPFLAGS"
            CFLAGS="$_ac_save_CFLAGS"
            LDFLAGS="$_ac_save_LDFLAGS"
            LIBS="$_ac_save_LIBS"
        fi
    else
        _extendvars="ifelse([$4],,yes,$4)"
        if test ".$_extendvars" = .yes; then
            if test ".$_pth_subdir" = .yes; then
                CPPFLAGS="$CPPFLAGS $_pth_cppflags"
                CFLAGS="$CFLAGS $_pth_cflags"
                LDFLAGS="$LDFLAGS $_pth_ldflags"
                LIBS="$LIBS $_pth_libs"
            fi
        fi
    fi
    PTH_CPPFLAGS="$_pth_cppflags"
    PTH_CFLAGS="$_pth_cflags"
    PTH_LDFLAGS="$_pth_ldflags"
    PTH_LIBS="$_pth_libs"
    AC_SUBST(PTH_CPPFLAGS)
    AC_SUBST(PTH_CFLAGS)
    AC_SUBST(PTH_LDFLAGS)
    AC_SUBST(PTH_LIBS)
    _AC_PTH_VERBOSE([+ Final Results:])
    _AC_PTH_VERBOSE([    o PTH_CPPFLAGS=\"$PTH_CPPFLAGS\"])
    _AC_PTH_VERBOSE([    o PTH_CFLAGS=\"$PTH_CFLAGS\"])
    _AC_PTH_VERBOSE([    o PTH_LDFLAGS=\"$PTH_LDFLAGS\"])
    _AC_PTH_VERBOSE([    o PTH_LIBS=\"$PTH_LIBS\"])
fi
if test ".$with_pth" != .no; then
    AC_MSG_RESULT([version $_pth_version, $_pth_type under $_pth_location])
    ifelse([$5], , :, [$5])
else
    AC_MSG_RESULT([no])
    ifelse([$6], , :, [$6])
fi
])

# AC_CHECK_LONG_LONG
#-------------------
AC_DEFUN([AC_CHECK_LONG_LONG],
[
AC_MSG_CHECKING(for long long)
AC_CACHE_VAL(ac_cv_type_long_long,
[
AC_TRY_COMPILE([
#include <stdio.h>
],
[
long long x;
],
ac_cv_type_long_long=yes,
ac_cv_type_long_long=no)
])
AC_MSG_RESULT($ac_cv_type_long_long)
if eval "test \"`echo $ac_cv_type_long_long`\" = yes"; then
  AC_DEFINE(HAVE_LONG_LONG, 1, [Define if you have long long])
fi
])# AC_CHECK_LONG_LONG

dnl From Bruno Haible.

AC_DEFUN([AM_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])

# AC_LIBLTDL_CONVENIENCE_G([DIRECTORY])
# -----------------------------------
# sets LIBLTDL to the link flags for the libltdl convenience library and
# LTDLINCL to the include flags for the libltdl header and adds
# --enable-ltdl-convenience to the configure arguments.  Note that
# AC_CONFIG_SUBDIRS is not called here.  If DIRECTORY is not provided,
# it is assumed to be `libltdl'.  LIBLTDL will be prefixed with
# '${top_builddir}/' and LTDLINCL will be prefixed with '${top_srcdir}/'
# (note the single quotes!).  If your package is not flat and you're not
# using automake, define top_builddir and top_srcdir appropriately in
# the Makefiles.
AC_DEFUN([AC_LIBLTDL_CONVENIENCE_G],
[AC_BEFORE([$0],[AC_LIBTOOL_SETUP])dnl
  case $enable_ltdl_convenience in
  no) AC_MSG_ERROR([this package needs a convenience libltdl]) ;;
  "") enable_ltdl_convenience=yes
      ac_configure_args="$ac_configure_args --enable-ltdl-convenience CFLAGS=-DWITHOUT_RTLD_GROUP" ;;
  esac
  LIBLTDL='${top_builddir}/'ifelse($#,1,[$1],['libltdl'])/libltdlc.la
  LTDLINCL='-I${top_srcdir}/'ifelse($#,1,[$1],['libltdl'])
  # For backwards non-gettext consistent compatibility...
  INCLTDL="$LTDLINCL"
])# AC_LIBLTDL_CONVENIENCE


AC_DEFUN([AC_CHECK_SEMUNDOO],
[
AC_LANG_C

AC_MSG_CHECKING([for semundo union])
AC_CACHE_VAL(ac_cv_semundo_union,
[
AC_TRY_LINK([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
],
[
union semun                 semctl_arg;
],
ac_cv_semundo_union=no,
ac_cv_semundo_union=yes)
])
AC_MSG_RESULT($ac_cv_semundo_union)
if eval "test \"`echo $ac_cv_semundo_union`\" = yes"; then
  AC_DEFINE(NEED_SEMUNDO_UNION, 1, [Define if you need semundo union])
fi
])

# ===========================================================================
#             http://autoconf-archive.cryp.to/ac_define_dir.html
# ===========================================================================
#
# SYNOPSIS
#
#   AC_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION])
#
# DESCRIPTION
#
#   This macro sets VARNAME to the expansion of the DIR variable, taking
#   care of fixing up ${prefix} and such.
#
#   VARNAME is then offered as both an output variable and a C preprocessor
#   symbol.
#
#   Example:
#
#      AC_DEFINE_DIR([DATADIR], [datadir], [Where data are placed to.])
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Stepan Kasal <kasal@ucw.cz>
#   Copyright (c) 2008 Andreas Schwab <schwab@suse.de>
#   Copyright (c) 2008 Guido U. Draheim <guidod@gmx.de>
#   Copyright (c) 2008 Alexandre Oliva
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([AC_DEFINE_DIR], [
  prefix_NONE=
  exec_prefix_NONE=
  test "x$prefix" = xNONE && prefix_NONE=yes && prefix=$ac_default_prefix
  test "x$exec_prefix" = xNONE && exec_prefix_NONE=yes && exec_prefix=$prefix
dnl In Autoconf 2.60, ${datadir} refers to ${datarootdir}, which in turn
dnl refers to ${prefix}.  Thus we have to use `eval' twice.
  eval ac_define_dir="\"[$]$2\""
  eval ac_define_dir="\"$ac_define_dir\""
  AC_SUBST($1, "$ac_define_dir")
  AC_DEFINE_UNQUOTED($1, "$ac_define_dir", [$3])
  test "$prefix_NONE" && prefix=NONE
  test "$exec_prefix_NONE" && exec_prefix=NONE
])

