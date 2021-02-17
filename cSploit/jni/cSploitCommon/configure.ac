#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libcsploit-common], [1.0], [])
AC_CONFIG_SRCDIR([src/message.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

LT_INIT

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen], [], [AC_MSG_ERROR([libdl not found])])
AC_CHECK_FUNCS([pthread_create], [], [
  AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([libpthread not found])])])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h unistd.h], [], [AC_MSG_ERROR([some header is missing])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strerror])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
