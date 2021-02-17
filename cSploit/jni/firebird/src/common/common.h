/*
 *      PROGRAM:        JRD access method
 *      MODULE:         common.h
 *      DESCRIPTION:    Common descriptions for all GDS programs
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2001.11.20  Ann Harrison - make 64bitio.h conditional on not windows.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - MAC ("MAC", "MAC_AUX" and "MAC_CP" defines)
 *                          - EPSON, XENIX, DELTA, IMP, NCR3000, M88K
 *                          - NT Power PC and HP9000 s300
 *
 * 2002.04.16  Paul Beach - HP10 and unistd.h
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_UNISTD_H
// for _POSIX_THREADS test below
#include <unistd.h>
#endif

#if defined(_POSIX_THREADS) && _POSIX_THREADS >= 200112L
// above check is generally true. However, we use pthreads on some platforms
// where _POSIX_THREADS is defined to "1" or not even defined at all!
#define USE_POSIX_THREADS
#endif

#include "fb_macros.h"
#include "fb_types.h"

/*
  do not use links in source code to maintain platform neutrality
*/


/*****************************************************
* Linux platforms
*****************************************************/
#ifdef LINUX
#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)

#define FB_OS OsLinux

#ifdef __GNUC__
#define FB_CC CcGcc
#else
#define FB_CC CcIcc
#endif

// SLONG is a 32-bit integer on 64-bit platforms
//#if SIZEOF_LONG == 4
//#define SLONGFORMAT "ld"
//#define ULONGFORMAT "lu"
//#define XLONGFORMAT "lX"
//#define xLONGFORMAT "lx"
//#endif

// format for size_t
#ifndef SIZEFORMAT
#define SIZEFORMAT "zi"
#endif

//format for __LINE__
#define LINEFORMAT "d"

#define UNIX
#define IEEE

#ifdef AMD64
#define FB_CPU CpuAmd

// on buggy kernels ERESTARTNOHAND (==514) may be returned instead of EINTR
// use value '514' instead of ERESTARTNOHAND cause it's not present in std includes
// in theory such error codes should never be seen outside kernel
#define SYSCALL_INTERRUPTED(err) (((err) == EINTR) || ((err) == 514))   /* pjpg 20001102 */

#endif

#ifdef PPC
#define FB_CPU CpuPowerPc
#endif

#ifdef i386
#define I386
#define FB_CPU CpuIntel
#endif /* i386 */

#ifdef ARM
#define FB_CPU CpuArm
#endif /* ARM */

#ifdef sparc
#define FB_CPU CpuUltraSparc
#define RISC_ALIGNMENT
#endif /* sparc */

#ifdef MIPSEL
#define FB_CPU CpuMipsel
#endif /* mipsel */

#ifdef MIPSEB
#define FB_CPU CpuMips
#endif /* mips */

#ifdef IA64
#define FB_CPU CpuIa64
#define RISC_ALIGNMENT
#endif // IA64

#ifndef USE_POSIX_THREADS
// force pthread detection on Linux for distros that do not provide
// POSIX thread compatability
#define USE_POSIX_THREADS
#endif

#ifdef __s390__
#ifdef __s390x__
#define FB_CPU CpuS390x
#else
#define FB_CPU CpuS390
#endif  // __s390x__
#endif  // __s390__

#ifdef SH
#define FB_CPU CpuSh
#define RISC_ALIGNMENT
#endif /* sh */

#ifdef SHEB
#define FB_CPU CpuSheb
#define RISC_ALIGNMENT
#endif /* sheb */

#ifdef HPPA
#define FB_CPU CpuHppa
#define RISC_ALIGNMENT
#endif /* hppa */

#ifdef ALPHA
#define FB_CPU CpuAlpha
#define RISC_ALIGNMENT
#endif

#endif /* LINUX */


/*****************************************************
* Darwin Platforms
*****************************************************/
#ifdef DARWIN

//format for __LINE__
#define LINEFORMAT "d"

//#define SLONGFORMAT	"ld"
//#define ULONGFORMAT "lu"
//#define XLONGFORMAT "lX"
//#define xLONGFORMAT "lx"

#define FB_CC CcGcc

#define UNIX
#define FB_OS OsDarwin
#ifdef __ppc__
#define powerpc
#define FB_CPU CpuPowerPc
#endif
#ifdef i386
#define I386
#define FB_CPU CpuIntel
#endif
#ifdef __x86_64__
#define DARWIN64
#define FB_CPU CpuAmd
#endif
#ifdef __ppc64__
#define DARWINPPC64
#define FB_CPU CpuPowerPc64
#endif
#define IEEE
#define QUADCONST(n) (n##LL)
#define QUADFORMAT "q"
//#define MAP_ANON		correctly defined in sys/mman.h

#define API_ROUTINE __attribute__((visibility("default")))
#define API_ROUTINE_VARARG API_ROUTINE
#define INTERNAL_API_ROUTINE API_ROUTINE
#define FB_EXPORTED __attribute__((visibility("default")))

#define O_DIRECT F_NOCACHE
#endif /* Darwin Platforms */


/*****************************************************
* FreeBSD for Intel platforms
*****************************************************/
#ifdef FREEBSD

#define FB_OS OsFreeBsd
#define FB_CC CcGcc

#define UNIX
#define IEEE

#ifdef AMD64
#define FB_CPU CpuAmd
#else
#define I386
#define FB_CPU CpuIntel
#endif

#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)
#define NO_NFS					/* no MTAB_OPEN or MTAB_CLOSE in isc_file.c */

#ifndef USE_POSIX_THREADS
// force pthread detection on FREEBSD
#define USE_POSIX_THREADS
#endif

#endif /* FREEBSD */

/*****************************************************
* NetBSD
*****************************************************/
#ifdef NETBSD

#define FB_OS OsNetBsd
#define FB_CC CcGcc

#if defined(__i386__)

#define IEEE
#define I386
#define FB_CPU CpuIntel

#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)
#else /* !__i386__ */
#error Please add support for other ports
#endif

#define UNIX
#define NO_NFS					/* no MTAB_OPEN or MTAB_CLOSE in isc_file.c */

#endif /* NETBSD */


/*****************************************************
* Android
*****************************************************/
#ifdef ANDROID

#define NO_NFS					/* no MTAB_OPEN or MTAB_CLOSE in isc_file.c */

#endif /* NETBSD */


/*****************************************************
 * SUN platforms--the 386i is obsolete
*****************************************************/

#ifdef __sun

#ifdef SOLARIS

#define FB_OS OsSolaris

#ifdef __GNUC__
#define FB_CC CcGcc
#else
#define FB_CC CcSunStudio
#endif

/*  Define the following only on platforms whose standard I/O
 *  implementation is so weak that we wouldn't be able to fopen
 *  a file whose underlying file descriptor would be > 255.
 *  Hey, we're not running on PDP-11's any more: would it kill you
 *  to use a short instead of a char to hold the fileno?  :-(
 */

/* Why we (solarises) need to rewrite old BSD stdio
   so many times I suggest stdIO from
   http://www.research.att.com/sw/tools/sfio/
*/
/* 	Need to use full sfio not just stdio emulation to fix
	file descriptor number limit. nmcc Dec2002
*/

/* Update for Solaris 10. The problem still exists pre Solaris 10,
however if you are using a version of Solaris 10 after 11/06 the
problem with file descriptors has been solved and this define and
the following function can now be commented out.
If you are using Solaris 10 from 03/05 - 11/06 you need to patch
your operating system.
Earlier versions of Solaris still have the problem. pbeach Feb2010
*/

#if (!defined(__arch64__)) && (!defined(SFIO))
#error "need to use SFIO"
#endif

// this function is normally defined in stdio.h, but is missing in SFIO's h-file
extern "C" int remove(const char* path);

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a printf() format string, to indicate that the argument
   is an SINT64 or FB_UINT64. */
#define QUADFORMAT "ll"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int. */
#define QUADCONST(n) (n##LL)

#ifndef USE_POSIX_THREADS
#if defined(_POSIX_THREADS) && _POSIX_THREADS >= 1L
// Solaris 9 has _POSIX_THREADS = 1L
// Solaris 10 has _POSIX_THREADS >= 200112L
#define USE_POSIX_THREADS
#endif
#endif

#endif // SOLARIS

#define UNIX
#define IEEE

#ifdef __sparc
#define FB_CPU CpuUltraSparc
#define RISC_ALIGNMENT

#elif defined (__amd64)
#define FB_CPU CpuAmd

#elif defined (__i386)
#define FB_CPU CpuIntel

#else
#error What is FB_CPU for this Solaris platform????
#endif

#endif // __sun


/*****************************************************
* HP/UX platforms
*****************************************************/

#ifdef HPUX

#define USE_SHMEM_EXT	// Looks like everyone else can ISC_remap

#define UNIX

#define FB_OS OsHpux

#if defined (__HP_aCC)
#define FB_CC CcAcc
#undef HAVE___THREAD	// aCC error, __thread can be used only with C-like structs
#elif defined (__GNUC__)
#define FB_CC CcGcc
#endif

#define IEEE
// 16-Apr-2002 HP10 in unistd.h Paul Beach
//#define setreuid(ruid, euid)     setresuid (ruid, euid, -1)
//#define setregid(rgid, egid)     setresgid (rgid, egid, -1)

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a printf() format string, to indicate that the argument
   is an SINT64 or FB_UINT64. */
#define QUADFORMAT "ll"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int. */
#define QUADCONST(n) (n)

#ifndef USE_POSIX_THREADS
// HPUX v B.11.23 does not have _POSIX_THREADS defined, their implementation
// is incomplete, but good enough for us
#define USE_POSIX_THREADS
#endif

#error Need a way to define CpuHppa or CpuIa64
#define FB_CPU

#define RISC_ALIGNMENT

#endif /* HPUX */


/*****************************************************
* IBM AIX PowerPC
*****************************************************/

#ifdef _AIX						/* IBM AIX */

#define FB_OS OsAix
#define FB_CC CcXlc

#if SIZEOF_VOID_P == 4
#define FB_CPU CpuPowerPc
#endif
#if SIZEOF_VOID_P == 8
#define FB_CPU CpuPowerPc64
#endif

#define AIX
#define AIX_PPC
#define UNIX

#define IEEE
#define SYSCALL_INTERRUPTED(err)        (((err) == EINTR) || ((err) == ERESTART))	/* pjpg 20001102 */

#define QUADFORMAT "ll"			/* TMC 081700 */
#define QUADCONST(n) (n##LL)	/* TMC 081700 */

// AIX does not pass autoconf's test for mmap() correctness,
// but we do not use flag (MAP_FIXED) that fails.
#define HAVE_MMAP

// autoconf test AC_SYS_LARGEFILE defines _LARGE_FILES for AIX builds.
// But, in <standards.h>, _LARGE_FILE_API is defined, leading to conflict
// in 32-bit builds. Only one of these macros should be defined.
#undef _LARGE_FILE_API

#endif /* IBM AIX */



/*****************************************************
* Windows NT
*****************************************************/

#ifdef WIN_NT

#define NO_NFS

#define FB_OS OsWindows

#ifdef __GNUC__
#define FB_CC CcGcc
#elif defined(_MSC_VER)
#define FB_CC CcMsvc
#endif

#define SYS_ERR		Arg::Windows
//#define SLONGFORMAT "ld"
//#define ULONGFORMAT "lu"
//#define XLONGFORMAT "lX"
//#define xLONGFORMAT "lx"

//format for __LINE__
#define LINEFORMAT "d"

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a printf() format string, to indicate that the argument
   is an SINT64 or FB_UINT64. */
#define QUADFORMAT "I64"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int. */
#define QUADCONST(n) (n##LL)

#ifdef AMD64
#define FB_CPU CpuAmd
#else
#ifndef I386
#define I386
#endif
#define FB_CPU CpuIntel
#endif

#define IEEE
#define API_ROUTINE     __stdcall
#define API_ROUTINE_VARARG      __cdecl
#define CLIB_ROUTINE    __cdecl

#ifndef MAXPATHLEN
#ifdef MAX_PATH
#define MAXPATHLEN MAX_PATH
#else
#define MAXPATHLEN 260
#endif
#endif

#define ERRNO		GetLastError()
#define INET_ERRNO	WSAGetLastError()
#define H_ERRNO		WSAGetLastError()

// For Visual Studio 2003 and earlier enable Windows 9X support
#if defined _MSC_VER && (_MSC_VER < 1400)
#define WIN9X_SUPPORT
#endif

#endif /* WIN_NT */


#ifndef FB_CPU
#error Define FB_CPU for your platform
#endif
#ifndef FB_OS
#error Define FB_OS for your platform
#endif
#ifndef FB_CC
#error Define FB_CC for your platform
#endif


/*****************************************************
 * UNIX
*****************************************************/
#ifdef UNIX
#define SYS_ERR		Arg::Unix
#endif /* UNIX */

#ifndef SYS_ERR
#define SYS_ERR		Arg::Unix
#endif

#ifndef ERRNO
#define ERRNO		errno
#endif

#ifndef INET_ERRNO
#define INET_ERRNO	errno
#endif

#ifndef H_ERRNO
#define H_ERRNO		h_errno
#endif

/* various declaration modifiers */

#ifndef API_ROUTINE
#define API_ROUTINE
#define API_ROUTINE_VARARG
#endif

#ifndef CLIB_ROUTINE
#define CLIB_ROUTINE
#endif

#ifndef FB_EXPORTED
#define FB_EXPORTED
#endif


/* alignment macros */

#ifndef OLD_ALIGNMENT
#ifdef I386
/* Using internal alignment optimal for 386 processor and above
 */
//#define FB_ALIGNMENT       4
//#define FB_DOUBLE_ALIGN    8
#endif
#endif

#ifndef FB_ALIGNMENT
#error must define FB_ALIGNMENT for your system
#endif

#ifndef SHIFTLONG
/* Number of shifts needed to convert between char and LONG */
#define SHIFTLONG       2
#define BITS_PER_LONG   32
#endif

#ifndef FB_DOUBLE_ALIGN
#error must define FB_DOUBLE_ALIGN for your system
#endif



/* common return values */

#ifndef FINI_OK
#define FINI_OK         0
#define FINI_ERROR      1
#define STARTUP_ERROR   2		/* this is also used in iscguard.h, make sure these match */
#endif

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
#define FB_SUCCESS         0
#define FB_FAILURE         1

/* sys/paramh.h : compatibility purposes */
#ifndef NOFILE
#define NOFILE      20
#endif

/* data type definitions */

#ifndef ISC_TIMESTAMP_DEFINED
typedef SLONG ISC_DATE;
typedef ULONG ISC_TIME;
struct ISC_TIMESTAMP
{
	ISC_DATE timestamp_date;
	ISC_TIME timestamp_time;
};
#define ISC_TIMESTAMP_DEFINED
#endif	/* ISC_TIMESTAMP_DEFINED */

#define GDS_DATE	ISC_DATE
#define GDS_TIME	ISC_TIME
#define GDS_TIMESTAMP	ISC_TIMESTAMP


#ifndef BLOB_PTR
#define BLOB_PTR        UCHAR
#endif


#ifndef SLONGFORMAT
#if SIZEOF_LONG == 4
#define SLONGFORMAT "ld"
#define ULONGFORMAT "lu"
#define XLONGFORMAT "lX"
#define xLONGFORMAT "lx"
#else
#define SLONGFORMAT	"d"
#define ULONGFORMAT	"u"
#define XLONGFORMAT "X"
#define xLONGFORMAT "x"
#endif
#endif

// Shortcuts to make it easier to convert code using SLONGFORMAT/ULONGFORMAT
#define UQUADFORMAT QUADFORMAT"u"
#define SQUADFORMAT QUADFORMAT"d"

// format for size_t
#ifndef SIZEFORMAT
#if (SIZEOF_SIZE_T == 8)
#define SIZEFORMAT UQUADFORMAT
#else
#define SIZEFORMAT ULONGFORMAT
#endif
#endif // SIZEFORMAT

// format for __LINE__
#ifndef LINEFORMAT
#ifdef __GNUC__
#define LINEFORMAT "d"
#else
#define LINEFORMAT "ld"
#endif // __GNUC__
#endif

// format for OS handles
#if defined(WIN_NT) && (SIZEOF_VOID_P == 8)
#define HANDLEFORMAT SQUADFORMAT
#else
#define HANDLEFORMAT SLONGFORMAT
#endif


/* conditional compilation macros */

/* MAX and MIN for datatypes */

#define MAX_UCHAR		((UCHAR)0xFF)
#define MIN_UCHAR		0x00

#define MAX_SCHAR		0x7F
#define MIN_SCHAR		(-MAX_SCHAR - 1)

#define MAX_USHORT		((USHORT)0xFFFF)
#define MIN_USHORT		0x0000

#define MAX_SSHORT		0x7FFF
#define MIN_SSHORT		(-MAX_SSHORT - 1)

#define MAX_ULONG		((ULONG)0xFFFFFFFF)
#define MIN_ULONG		0x00000000

#define MAX_SLONG		0x7FFFFFFF
#define MIN_SLONG		(-MAX_SLONG - 1)

#define MAX_UINT64              ((FB_UINT64) QUADCONST(0xFFFFFFFFFFFFFFFF))
#define MIN_UINT64              QUADCONST(0x0000000000000000)

#define MAX_SINT64              QUADCONST(0x7FFFFFFFFFFFFFFF)
#define MIN_SINT64              (-MAX_SINT64 - 1)



/* commonly used macros */

#ifndef MAX
#define MAX(a, b)                (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)                (((a) < (b)) ? (a) : (b))
#endif

#define UPPER(c)                (((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 'A' : (c))
#define LOWWER(c)               (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

/* UPPER7 & LOWWER7 are to be used when the data is known to
   be 7-bit ASCII, eg: internal data, OS data.  NOT user data.  */

#define UPPER7(c)               (((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 'A' : (c))
#define LOWWER7(c)              (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

#ifndef ROUNDUP
#define ROUNDUP(n, b)            (((n) + (b) - 1) & ~((b) - 1))
#endif
#define ROUNDUP_LONG(len)       ROUNDUP (len, sizeof (SLONG))

#define JRD_BUGCHK 15			/* facility code for bugcheck messages */
#ifndef OFFSET
#define OFFSET(struct, fld)      ((size_t) &((struct) NULL)->fld)
#define OFFSETA(struct, fld)     ((size_t) ((struct) NULL)->fld)
#endif

#ifndef ODS_ALIGNMENT
#define ODS_ALIGNMENT           4
#endif

#ifndef FORMAT_ALIGNMENT
// Alignment for items in record format. Used for databases after ODS11.
// Always 64-bit to ensure ODS compatibility with 64-bit versions of the engine
#define FORMAT_ALIGNMENT           8
#endif

#ifndef SYSCALL_INTERRUPTED
#define SYSCALL_INTERRUPTED(err)        ((err) == EINTR)
#endif


/* data movement and allocation macros */

#define MOVE_CLEAR(to, length)			memset(to, 0, (size_t) (length))

// This macros are used to workaround shortage of standard conformance
// in Microsoft compilers. They could be replaced with normal procedure
// and generic macro if MSVC would support C99-style __VA_ARGS__
#define DEFINE_TRACE_ROUTINE(routine) void routine(const char* message, ...)

#ifdef HAVE_VSNPRINTF
#define VSNPRINTF(a, b, c, d) vsnprintf(a, b, c, d)
#else
#define VSNPRINTF(a, b, c, d) vsprintf(a, c, d)
#endif

#ifdef HAVE_SNPRINTF
#define SNPRINTF snprintf
#else
#define SNPRINTF(buffer, length, ...) sprintf(buffer, __VA_ARGS__)
#endif

#define IMPLEMENT_TRACE_ROUTINE(routine, subsystem) \
void routine(const char* message, ...) \
{ \
	Firebird::string format(subsystem ","); \
	format += message; \
	Firebird::string buffer; \
	va_list params; \
	va_start(params, message); \
	buffer.vprintf(format.c_str(), params); \
	va_end(params); \
	gds__trace(buffer.c_str()); \
}

#ifdef DEV_BUILD

/* Define any debugging symbols and macros here.  This
   ifdef will be executed during development builds. */

#define TRACE(msg)				gds__trace (msg)

#ifndef DEV_REPORT
#define DEV_REPORT(msg)         gds__log (msg)
#endif

#ifndef BREAKPOINT
#define BREAKPOINT(x)           GDS_breakpoint(x)
// fwd. decl. the function itself
#ifdef __cplusplus
extern "C" {
#endif
void GDS_breakpoint(int);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif

#endif /* DEV_BUILD */

#ifndef DEV_BUILD
#ifndef DEV_REPORT
#define DEV_REPORT(msg)         gds__log (msg)
#endif
#ifndef BREAKPOINT
#define BREAKPOINT(x)			/* nothing */
#endif
#ifndef TRACE
#define TRACE(msg)				/* nothing */
#endif
#endif


/* commonly used buffer sizes for dynamic buffer allocation */

#define BUFFER_XLARGE   2048
#define BUFFER_LARGE    1024
#define BUFFER_MEDIUM   512
#define BUFFER_SMALL    256
#define BUFFER_TINY     128

/* The default lseek offset type.  Changed from nothing to (off_t) to correctly support 64 bit IO */
#ifndef LSEEK_OFFSET_CAST
#define LSEEK_OFFSET_CAST (off_t)
#endif

#define STRINGIZE_AUX(x)	#x
#define STRINGIZE(x)		STRINGIZE_AUX(x)

#ifdef _MSC_VER
#define FB_CONST64(a) (a##i64)
#else
#define FB_CONST64(a) (a##LL)
#endif

// Check for "final" keyword support
#ifdef __GNUC__
#if ((__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ >= 5))
#define FB_FINAL __final
#endif
#endif
// Please add support for other compilers here
#ifndef FB_FINAL
#define FB_FINAL
#endif

#define FB_UNUSED(value) do { if (value) {} } while (false)

// 30 Dec 2002. Nickolay Samofatov
// This needs to be checked for all supported platforms
// The simpliest way to check it is to issue from correct client:
// declare external function abs2 double precision
//   returns double precision by value
//   entry_point 'IB_UDF_abs' module_name 'ib_udf';
// select abs2(2.0 / 3.0) from rdb$database;
// It will return big strange value in case of invalid define

// ASF: Currently, all little-endian are FB_SWAP_DOUBLE and big-endian aren't.
// AP: Define it for your hardware correctly in case your CPU do not follow mentioned rule.
//     The follwoing lines are kept for reference only.
//#if defined(i386) || defined(I386) || defined(_M_IX86) || defined(AMD64) || defined(ARM) || defined(MIPSEL) || defined(DARWIN64) || defined(IA64)
//#define		FB_SWAP_DOUBLE 1
//#elif defined(sparc) || defined(PowerPC) || defined(PPC) || defined(__ppc__) || defined(HPUX) || defined(MIPS) || defined(__ppc64__)
//#define		FB_SWAP_DOUBLE 0
//#else
//#error "Define FB_SWAP_DOUBLE for your platform correctly !"
//#endif

#ifndef FB_SWAP_DOUBLE
#ifdef WORDS_BIGENDIAN
#define FB_SWAP_DOUBLE 0
#else
#define FB_SWAP_DOUBLE 1
#endif
#endif

// Commonly used indices to access parts of double in correct order.
#if FB_SWAP_DOUBLE
#define FB_LONG_DOUBLE_FIRST 1
#define FB_LONG_DOUBLE_SECOND 0
#else
#define FB_LONG_DOUBLE_FIRST 0
#define FB_LONG_DOUBLE_SECOND 1
#endif

// Used in quad operations
#ifndef WORDS_BIGENDIAN
const int LOW_WORD		= 0;
const int HIGH_WORD		= 1;
#else
const int LOW_WORD		= 1;
const int HIGH_WORD		= 0;
#endif

#ifndef HAVE_WORKING_VFORK
#define vfork fork
#endif


// In many cases we need to assign wrong handle value to closed handle
// Some OSes define it, others - not
#ifndef WIN_NT
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (-1)
#endif
#endif

static const TEXT FB_SHORT_MONTHS[][4] =
{
	"Jan", "Feb", "Mar",
	"Apr", "May", "Jun",
	"Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec",
	"\0"
};

static const TEXT* const FB_LONG_MONTHS_UPPER[] =
{
	"JANUARY",
	"FEBRUARY",
	"MARCH",
	"APRIL",
	"MAY",
	"JUNE",
	"JULY",
	"AUGUST",
	"SEPTEMBER",
	"OCTOBER",
	"NOVEMBER",
	"DECEMBER",
	0
};

#endif /* COMMON_COMMON_H */

