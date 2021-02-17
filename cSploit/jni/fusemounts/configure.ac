#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fusemounts], [1.0], [https://github.com/tux-mind/fusemounts/issues])
AC_CONFIG_SRCDIR([finder.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [pow])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h], [], \
  [AC_MSG_ERROR([missing required headers])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([pow strchr strdup strstr strtoul], [], \
  [AC_MSG_ERROR([missing required headers])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
