# manywarnings.m4 serial 5
dnl Copyright (C) 2008-2013 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson

# gl_MANYWARN_COMPLEMENT(OUTVAR, LISTVAR, REMOVEVAR)
# --------------------------------------------------
# Copy LISTVAR to OUTVAR except for the entries in REMOVEVAR.
# Elements separated by whitespace.  In set logic terms, the function
# does OUTVAR = LISTVAR \ REMOVEVAR.
AC_DEFUN([gl_MANYWARN_COMPLEMENT],
[
  gl_warn_set=
  set x $2; shift
  for gl_warn_item
  do
    case " $3 " in
      *" $gl_warn_item "*)
        ;;
      *)
        gl_warn_set="$gl_warn_set $gl_warn_item"
        ;;
    esac
  done
  $1=$gl_warn_set
])

# gl_MANYWARN_ALL_GCC(VARIABLE)
# -----------------------------
# Add all documented GCC warning parameters to variable VARIABLE.
# Note that you need to test them using gl_WARN_ADD if you want to
# make sure your gcc understands it.
AC_DEFUN([gl_MANYWARN_ALL_GCC],
[
  dnl First, check for some issues that only occur when combining multiple
  dnl gcc warning categories.
  AC_REQUIRE([AC_PROG_CC])
  if test -n "$GCC"; then

    dnl Check if -W -Werror -Wno-missing-field-initializers is supported
    dnl with the current $CC $CFLAGS $CPPFLAGS.
    AC_MSG_CHECKING([whether -Wno-missing-field-initializers is supported])
    AC_CACHE_VAL([gl_cv_cc_nomfi_supported], [
      gl_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -W -Werror -Wno-missing-field-initializers"
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[]], [[]])],
        [gl_cv_cc_nomfi_supported=yes],
        [gl_cv_cc_nomfi_supported=no])
      CFLAGS="$gl_save_CFLAGS"])
    AC_MSG_RESULT([$gl_cv_cc_nomfi_supported])

    if test "$gl_cv_cc_nomfi_supported" = yes; then
      dnl Now check whether -Wno-missing-field-initializers is needed
      dnl for the { 0, } construct.
      AC_MSG_CHECKING([whether -Wno-missing-field-initializers is needed])
      AC_CACHE_VAL([gl_cv_cc_nomfi_needed], [
        gl_save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS -W -Werror"
        AC_COMPILE_IFELSE(
          [AC_LANG_PROGRAM(
             [[void f (void)
               {
                 typedef struct { int a; int b; } s_t;
                 s_t s1 = { 0, };
               }
             ]],
             [[]])],
          [gl_cv_cc_nomfi_needed=no],
          [gl_cv_cc_nomfi_needed=yes])
        CFLAGS="$gl_save_CFLAGS"
      ])
      AC_MSG_RESULT([$gl_cv_cc_nomfi_needed])
    fi

    dnl Next, check if -Werror -Wuninitialized is useful with the
    dnl user's choice of $CFLAGS; some versions of gcc warn that it
    dnl has no effect if -O is not also used
    AC_MSG_CHECKING([whether -Wuninitialized is supported])
    AC_CACHE_VAL([gl_cv_cc_uninitialized_supported], [
      gl_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Werror -Wuninitialized"
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[]], [[]])],
        [gl_cv_cc_uninitialized_supported=yes],
        [gl_cv_cc_uninitialized_supported=no])
      CFLAGS="$gl_save_CFLAGS"])
    AC_MSG_RESULT([$gl_cv_cc_uninitialized_supported])

  fi

  # List all gcc warning categories.
  gl_manywarn_set=
  for gl_manywarn_item in \
    -W \
    -Wabi \
    -Waddress \
    -Waggressive-loop-optimizations \
    -Wall \
    -Warray-bounds \
    -Wattributes \
    -Wbad-function-cast \
    -Wbuiltin-macro-redefined \
    -Wcast-align \
    -Wchar-subscripts \
    -Wclobbered \
    -Wcomment \
    -Wcomments \
    -Wcoverage-mismatch \
    -Wcpp \
    -Wdeprecated \
    -Wdeprecated-declarations \
    -Wdisabled-optimization \
    -Wdiv-by-zero \
    -Wdouble-promotion \
    -Wempty-body \
    -Wendif-labels \
    -Wenum-compare \
    -Wextra \
    -Wformat-contains-nul \
    -Wformat-extra-args \
    -Wformat-nonliteral \
    -Wformat-security \
    -Wformat-y2k \
    -Wformat-zero-length \
    -Wfree-nonheap-object \
    -Wignored-qualifiers \
    -Wimplicit \
    -Wimplicit-function-declaration \
    -Wimplicit-int \
    -Winit-self \
    -Winline \
    -Wint-to-pointer-cast \
    -Winvalid-memory-model \
    -Winvalid-pch \
    -Wjump-misses-init \
    -Wlogical-op \
    -Wmain \
    -Wmaybe-uninitialized \
    -Wmissing-braces \
    -Wmissing-declarations \
    -Wmissing-field-initializers \
    -Wmissing-include-dirs \
    -Wmissing-parameter-type \
    -Wmissing-prototypes \
    -Wmudflap \
    -Wmultichar \
    -Wnarrowing \
    -Wnested-externs \
    -Wnonnull \
    -Wnormalized=nfc \
    -Wold-style-declaration \
    -Wold-style-definition \
    -Woverflow \
    -Woverlength-strings \
    -Woverride-init \
    -Wpacked \
    -Wpacked-bitfield-compat \
    -Wparentheses \
    -Wpointer-arith \
    -Wpointer-sign \
    -Wpointer-to-int-cast \
    -Wpragmas \
    -Wreturn-local-addr \
    -Wreturn-type \
    -Wsequence-point \
    -Wshadow \
    -Wsizeof-pointer-memaccess \
    -Wstack-protector \
    -Wstrict-aliasing \
    -Wstrict-overflow \
    -Wstrict-prototypes \
    -Wsuggest-attribute=const \
    -Wsuggest-attribute=format \
    -Wsuggest-attribute=noreturn \
    -Wsuggest-attribute=pure \
    -Wswitch \
    -Wswitch-default \
    -Wsync-nand \
    -Wsystem-headers \
    -Wtrampolines \
    -Wtrigraphs \
    -Wtype-limits \
    -Wuninitialized \
    -Wunknown-pragmas \
    -Wunsafe-loop-optimizations \
    -Wunused \
    -Wunused-but-set-parameter \
    -Wunused-but-set-variable \
    -Wunused-function \
    -Wunused-label \
    -Wunused-local-typedefs \
    -Wunused-macros \
    -Wunused-parameter \
    -Wunused-result \
    -Wunused-value \
    -Wunused-variable \
    -Wvarargs \
    -Wvariadic-macros \
    -Wvector-operation-performance \
    -Wvla \
    -Wvolatile-register-var \
    -Wwrite-strings \
    \
    ; do
    gl_manywarn_set="$gl_manywarn_set $gl_manywarn_item"
  done

  # Disable specific options as needed.
  if test "$gl_cv_cc_nomfi_needed" = yes; then
    gl_manywarn_set="$gl_manywarn_set -Wno-missing-field-initializers"
  fi

  if test "$gl_cv_cc_uninitialized_supported" = no; then
    gl_manywarn_set="$gl_manywarn_set -Wno-uninitialized"
  fi

  $1=$gl_manywarn_set
])
