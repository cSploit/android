/* DO NOT EDIT: automatically built by dist/s_android. */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `fdatasync' function. */
/* #undef HAVE_FDATASYNC */

/* Define to 1 if you have the `gmtime_r' function. */
#define HAVE_GMTIME_R 1

/* Define to 1 if the system has the type `int16_t'. */
#define HAVE_INT16_T 1

/* Define to 1 if the system has the type `int32_t'. */
#define HAVE_INT32_T 1

/* Define to 1 if the system has the type `int64_t'. */
#define HAVE_INT64_T 1

/* Define to 1 if the system has the type `int8_t'. */
#define HAVE_INT8_T 1

/* Define to 1 if the system has the type `intptr_t'. */
#define HAVE_INTPTR_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `localtime_r' function. */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the `localtime_s' function. */
/* #undef HAVE_LOCALTIME_S */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if the system has the type `uint16_t'. */
#define HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#define HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint64_t'. */
#define HAVE_UINT64_T 1

/* Define to 1 if the system has the type `uint8_t'. */
#define HAVE_UINT8_T 1

/* Define to 1 if the system has the type `uintptr_t'. */
#define HAVE_UINTPTR_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "sqlite"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "sqlite 3.7.16.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "sqlite"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.7.16.2"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
/*
** Build options detected by SQLite's configure script but not normally part
** of config.h.  Accept what configure detected unless it was overridden on the
** command line.
*/
#ifndef HAVE_EDITLINE
#define HAVE_EDITLINE 0
#endif
#if !HAVE_EDITLINE
#undef HAVE_EDITLINE
#endif

#ifndef HAVE_READLINE
#define HAVE_READLINE 0
#endif
#if !HAVE_READLINE
#undef HAVE_READLINE
#endif

#ifndef SQLITE_OS_UNIX
#define SQLITE_OS_UNIX 1
#endif
#if !SQLITE_OS_UNIX
#undef SQLITE_OS_UNIX
#endif

#ifndef SQLITE_OS_WIN
#define SQLITE_OS_WIN 0
#endif
#if !SQLITE_OS_WIN
#undef SQLITE_OS_WIN
#endif

#ifndef SQLITE_THREADSAFE
#define SQLITE_THREADSAFE 1
#endif
#if !SQLITE_THREADSAVE
#undef SQLITE_THREADSAVE
#endif

#ifndef SQLITE_THREAD_OVERRIDE_LOCK
#define SQLITE_THREAD_OVERRIDE_LOCK -1
#endif
#if !SQLITE_THREAD_OVERRIDE_LOCK
#undef SQLITE_THREAD_OVERRIDE_LOCK
#endif

#ifndef SQLITE_TEMP_STORE
#define SQLITE_TEMP_STORE 1
#endif
#if !SQLITE_THREAD_OVERRIDE_LOCK
#undef SQLITE_THREAD_OVERRIDE_LOCK
#endif
