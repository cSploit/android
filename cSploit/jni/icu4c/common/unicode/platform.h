/*
******************************************************************************
*
*   Copyright (C) 1997-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

/**
 * \file 
 * \brief Basic types for the platform 
 */

/* Define the platform we're on. */
#ifndef U_LINUX
#define U_LINUX
#endif

/**
 * \def U_HAVE_DIRENT_H
 * Define whether dirent.h is available */
#ifndef U_HAVE_DIRENT_H
#define U_HAVE_DIRENT_H 1
#endif

/** Define whether inttypes.h is available */
#ifndef U_HAVE_INTTYPES_H
#define U_HAVE_INTTYPES_H 1
#endif

/**
 * Define what support for C++ streams is available.
 *     If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (1997711 is the date the ISO/IEC C++ FDIS was published), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 *     If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (198506 is the date when Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 *     If U_IOSTREAM_SOURCE is 0, then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 */

#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

#ifdef __BIONIC__
# define HAVE_ANDROID_OS 1
#endif

/**
 * \def U_HAVE_STD_STRING
 * Define whether the standard C++ (STL) <string> header is available.
 * For platforms that do not use platform.h and do not define this constant
 * in their platform-specific headers, std_string.h defaults
 * U_HAVE_STD_STRING to 1.
 * @draft ICU 4.2
 */
#ifdef HAVE_ANDROID_OS
#define U_HAVE_STD_STRING 0
#else
#ifndef U_HAVE_STD_STRING
#define U_HAVE_STD_STRING 1
#endif
#endif

/** @{ Determines whether specific types are available */
#ifndef U_HAVE_INT8_T
#define U_HAVE_INT8_T 1
#endif

#ifndef U_HAVE_UINT8_T
#define U_HAVE_UINT8_T 1
#endif

#ifndef U_HAVE_INT16_T
#define U_HAVE_INT16_T 1
#endif

#ifndef U_HAVE_UINT16_T
#define U_HAVE_UINT16_T 1
#endif

#ifndef U_HAVE_INT32_T
#define U_HAVE_INT32_T 1
#endif

#ifndef U_HAVE_UINT32_T
#define U_HAVE_UINT32_T 1
#endif

#ifndef U_HAVE_INT64_T
#define U_HAVE_INT64_T 1
#endif

#ifndef U_HAVE_UINT64_T
#define U_HAVE_UINT64_T 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Generic data types                                                        */
/*===========================================================================*/

#include <sys/types.h>

/* If your platform does not have the <inttypes.h> header, you may
   need to edit the typedefs below. */
#if U_HAVE_INTTYPES_H

/* autoconf 2.13 sometimes can't properly find the data types in <inttypes.h> */
/* os/390 needs <inttypes.h>, but it doesn't have int8_t, and it sometimes */
/* doesn't have uint8_t depending on the OS version. */
/* So we have this work around. */
#ifdef OS390
/* The features header is needed to get (u)int64_t sometimes. */
#include <features.h>
#if ! U_HAVE_INT8_T
typedef signed char int8_t;
#endif
#if !defined(__uint8_t)
#define __uint8_t 1
typedef unsigned char uint8_t;
#endif
#endif /* OS390 */

#include <inttypes.h>

#else /* U_HAVE_INTTYPES_H */

#if ! U_HAVE_INT8_T
typedef signed char int8_t;
#endif

#if ! U_HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif

#if ! U_HAVE_INT16_T
typedef signed short int16_t;
#endif

#if ! U_HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif

#if ! U_HAVE_INT32_T
typedef signed int int32_t;
#endif

#if ! U_HAVE_UINT32_T
typedef unsigned int uint32_t;
#endif

#if ! U_HAVE_INT64_T
    typedef signed long long int64_t;
/* else we may not have a 64-bit type */
#endif

#if ! U_HAVE_UINT64_T
    typedef unsigned long long uint64_t;
/* else we may not have a 64-bit type */
#endif

#endif

/** @} */

/*===========================================================================*/
/** @{ Compiler and environment features                                         */
/*===========================================================================*/

/* Define whether namespace is supported */
#ifndef U_HAVE_NAMESPACE
#define U_HAVE_NAMESPACE 1
#endif

/* Determines the endianness of the platform
   It's done this way in case multiple architectures are being built at once.
   For example, Darwin supports fat binaries, which can be both PPC and x86 based. */
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN)
#define U_IS_BIG_ENDIAN (BYTE_ORDER == BIG_ENDIAN)
#else
#define U_IS_BIG_ENDIAN 0
#endif

/* 1 or 0 to enable or disable threads.  If undefined, default is: enable threads. */
#define ICU_USE_THREADS 1

/* On strong memory model CPUs (e.g. x86 CPUs), we use a safe & quick double check lock. */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define UMTX_STRONG_MEMORY_MODEL 1
#endif

#ifndef U_DEBUG
#define U_DEBUG 0
#endif

#ifndef U_RELEASE
#define U_RELEASE 1
#endif

/* Determine whether to disable renaming or not. This overrides the
   setting in umachine.h which is for all platforms. */
#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 0
#endif

/* Determine whether to override new and delete. */
#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION 1
#endif
/* Determine whether to override placement new and delete for STL. */
#ifndef U_HAVE_PLACEMENT_NEW
#define U_HAVE_PLACEMENT_NEW 1
#endif

/* Determine whether to enable tracing. */
#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING 0
#endif

/* Do we allow ICU users to use the draft APIs by default? */
#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT 1
#endif

/* Define the library suffix in a C syntax. */
#define U_HAVE_LIB_SUFFIX 0
#define U_LIB_SUFFIX_C_NAME 
#define U_LIB_SUFFIX_C_NAME_STRING ""

/** @} */

/*===========================================================================*/
/** @{ Character data types                                                      */
/*===========================================================================*/

#if ((defined(OS390) && (!defined(__CHARSET_LIB) || !__CHARSET_LIB))) || defined(OS400)
#   define U_CHARSET_FAMILY 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                           */
/*===========================================================================*/
// BEGIN android-note
// We changed "ARM_FLAG" to "HAVE_ANDROID_OS" immediately below.
// Consensus seems to be that the intent of ARM_FLAG is actually that
// it represent whether we are compiling for Android, and if that's
// the case then we might as well use the standard definition instead.
// END android-note
#if HAVE_ANDROID_OS
#define U_HAVE_WCHAR_H      0
#define U_SIZEOF_WCHAR_T    1

#define U_HAVE_WCSCPY       0

#else

#ifndef U_HAVE_WCHAR_H
#define U_HAVE_WCHAR_H      1
#endif

#ifndef U_SIZEOF_WCHAR_T
#define U_SIZEOF_WCHAR_T    4
#endif

#ifndef U_HAVE_WCSCPY
#define U_HAVE_WCSCPY       1
#endif

#endif

/** @} */

/**
 * @{
 * \def U_DECLARE_UTF16
 * Do not use this macro. Use the UNICODE_STRING or U_STRING_DECL macros
 * instead.
 * @internal
 */
#if 1 || defined(U_CHECK_UTF16_STRING)
#if (defined(__xlC__) && defined(__IBM_UTF_LITERAL) && U_SIZEOF_WCHAR_T != 2) \
    || (defined(__HP_aCC) && __HP_aCC >= 035000) \
    || (defined(__HP_cc) && __HP_cc >= 111106)
#define U_DECLARE_UTF16(string) u ## string
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550)
/* || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x580) */
/* Sun's C compiler has issues with this notation, and it's unreliable. */
#define U_DECLARE_UTF16(string) U ## string
#elif U_SIZEOF_WCHAR_T == 2 \
    && (U_CHARSET_FAMILY == 0 || ((defined(OS390) || defined(OS400)) && defined(__UCS2__)))
#define U_DECLARE_UTF16(string) L ## string
#endif
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about POSIX support                                           */
/*===========================================================================*/
// BEGIN android-note
// See Android comment above.
// END android-note
#if !HAVE_ANDROID_OS
#define U_HAVE_NL_LANGINFO          1

#ifndef U_HAVE_NL_LANGINFO_CODESET
#define U_HAVE_NL_LANGINFO_CODESET  1
#endif

#endif

#ifndef U_NL_LANGINFO_CODESET
#define U_NL_LANGINFO_CODESET       CODESET
#endif

#if !HAVE_ANDROID_OS
#if 1
#define U_TZSET         tzset
#endif
#if 1
#define U_TIMEZONE      timezone
#endif
#if 1
#define U_TZNAME        tzname
#endif
#endif

#define U_HAVE_MMAP     1
#define U_HAVE_POPEN    1

/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#if 1
#define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550) 
#define U_EXPORT __global
/*#elif defined(__HP_aCC) || defined(__HP_cc)
#define U_EXPORT __declspec(dllexport)*/
#else
#define U_EXPORT
#endif

/* U_CALLCONV is releated to U_EXPORT2 */
#define U_EXPORT2

/* cygwin needs to export/import data */
#ifdef U_CYGWIN
#define U_IMPORT __declspec(dllimport)
#else
#define U_IMPORT 
#endif

/* @} */

/*===========================================================================*/
/** @{ Code alignment and C function inlining                                    */
/*===========================================================================*/

#ifndef U_INLINE
#   ifdef __cplusplus
#       define U_INLINE inline
#   else
#       define U_INLINE inline
#   endif
#endif

#ifndef U_ALIGN_CODE
#define U_ALIGN_CODE(n) 
#endif

/** @} */

/*===========================================================================*/
/** @{ Programs used by ICU code                                                 */
/*===========================================================================*/

/**
 * \def U_MAKE
 * What program to execute to run 'make'
 */
#ifndef U_MAKE
#define U_MAKE  "make"
#endif

/** @} */
#endif
