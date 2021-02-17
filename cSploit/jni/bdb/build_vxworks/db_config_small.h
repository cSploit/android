/* DO NOT EDIT: automatically built by dist/s_vxworks. */
/* !!!
 * The CONFIG_TEST option may be added using the Tornado project build.
 * DO NOT modify it here.
 */
/* Define to 1 if you want to build a version for running the test suite. */
/* #undef CONFIG_TEST */

/* Defined to a size to limit the stack size of Berkeley DB threads. */
/* #undef DB_STACKSIZE */

/* We use DB_WIN32 much as one would use _WIN32 -- to specify that we're using
   an operating system environment that supports Win32 calls and semantics. We
   don't use _WIN32 because Cygwin/GCC also defines _WIN32, even though
   Cygwin/GCC closely emulates the Unix environment. */
/* #undef DB_WIN32 */

/* !!!
 * The DEBUG option may be added using the Tornado project build.
 * DO NOT modify it here.
 */
/* Define to 1 if you want a debugging version. */
/* #undef DEBUG */

/* Define to 1 if you want a version that logs read operations. */
/* #undef DEBUG_ROP */

/* Define to 1 if you want a version that logs write operations. */
/* #undef DEBUG_WOP */

/* !!!
 * The DIAGNOSTIC option may be added using the Tornado project build.
 * DO NOT modify it here.
 */
/* Define to 1 if you want a version with run-time diagnostic checking. */
/* #undef DIAGNOSTIC */

/* Define to 1 if 64-bit types are available. */
#define HAVE_64BIT_TYPES 1

/* Define to 1 if you have the `abort' function. */
#define HAVE_ABORT 1

/* Define to 1 if you have the `atoi' function. */
#define HAVE_ATOI 1

/* Define to 1 if you have the `atol' function. */
#define HAVE_ATOL 1

/* Define to 1 if platform reads and writes files atomically. */
/* #undef HAVE_ATOMICFILEREAD */

/* Define to 1 to use Solaris library routes for atomic operations. */
/* #undef HAVE_ATOMIC_SOLARIS */

/* Define to 1 to use native atomic operations. */
/* #undef HAVE_ATOMIC_SUPPORT */

/* Define to 1 to use GCC and x86 or x86_64 assemlby language atomic
   operations. */
/* #undef HAVE_ATOMIC_X86_GCC_ASSEMBLY */

/* Define to 1 if you have the `backtrace' function. */
/* #undef HAVE_BACKTRACE */

/* Define to 1 if you have the `backtrace_symbols' function. */
/* #undef HAVE_BACKTRACE_SYMBOLS */

/* Define to 1 if you have the `bsearch' function. */
#define HAVE_BSEARCH 1

/* Define to 1 if you have the `clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define to 1 if clock_gettime supports CLOCK_MONOTONIC. */
/* #undef HAVE_CLOCK_MONOTONIC */

/* Define to 1 if building compression support. */
/* #undef HAVE_COMPRESSION */

/* Define to 1 if Berkeley DB release includes strong cryptography. */
/* #undef HAVE_CRYPTO */

/* Define to 1 if using Intel IPP for cryptography. */
/* #undef HAVE_CRYPTO_IPP */

/* Define to 1 if you have the `ctime_r' function. */
#define HAVE_CTIME_R 1

/* Define to 1 if ctime_r takes a buffer length as a third argument. */
#define HAVE_CTIME_R_3ARG 1

/* Define to 1 if building the DBM API. */
/* #undef HAVE_DBM */

/* Define to 1 if you have the `directio' function. */
/* #undef HAVE_DIRECTIO */

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 to use dtrace for performance event tracing. */
/* #undef HAVE_DTRACE */

/* Define to 1 if you have the <execinfo.h> header file. */
/* #undef HAVE_EXECINFO_H */

/* Define to 1 if you have EXIT_SUCCESS/EXIT_FAILURE #defines. */
#define HAVE_EXIT_SUCCESS 1

/* Define to 1 if you have the `fchmod' function. */
/* #undef HAVE_FCHMOD */

/* Define to 1 if you have the `fclose' function. */
#define HAVE_FCLOSE 1

/* Define to 1 if you have the `fcntl' function. */
/* #undef HAVE_FCNTL */

/* Define to 1 if fcntl/F_SETFD denies child access to file descriptors. */
/* #undef HAVE_FCNTL_F_SETFD */

/* Define to 1 if you have the `fdatasync' function. */
/* #undef HAVE_FDATASYNC */

/* Define to 1 if you have the `fgetc' function. */
#define HAVE_FGETC 1

/* Define to 1 if you have the `fgets' function. */
#define HAVE_FGETS 1

/* Define to 1 if allocated filesystem blocks are not zeroed. */
#define HAVE_FILESYSTEM_NOTZERO 1

/* Define to 1 if you have the `fopen' function. */
#define HAVE_FOPEN 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if you have the `fwrite' function. */
#define HAVE_FWRITE 1

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `getenv' function. */
#define HAVE_GETENV 1

/* Define to 1 if you have the `getgid' function. */
#define HAVE_GETGID 1

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if getopt supports the optreset variable. */
#define HAVE_GETOPT_OPTRESET 1

/* Define to 1 if you have the `getrusage' function. */
/* #undef HAVE_GETRUSAGE */

/* Define to 1 if you have the `gettimeofday' function. */
/* #undef HAVE_GETTIMEOFDAY */

/* Define to 1 if you have the `getuid' function. */
/* #undef HAVE_GETUID */

/* Define to 1 if building Hash access method. */
/* #undef HAVE_HASH */

/* Define to 1 if building Heap access method. */
#define HAVE_HEAP 1

/* Define to 1 if you have the `hstrerror' function. */
/* #undef HAVE_HSTRERROR */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the `isalpha' function. */
#define HAVE_ISALPHA 1

/* Define to 1 if you have the `isdigit' function. */
#define HAVE_ISDIGIT 1

/* Define to 1 if you have the `isprint' function. */
#define HAVE_ISPRINT 1

/* Define to 1 if you have the `isspace' function. */
#define HAVE_ISSPACE 1

/* Define to 1 if you have a localization function to support globalization. */
/* #undef HAVE_LOCALIZATION */

/* Define to 1 if you have the `localtime' function. */
#define HAVE_LOCALTIME 1

/* Define to 1 if you want to enable log checksums. */
#define HAVE_LOG_CHECKSUM 1

/* Define to 1 if you have the `memcmp' function. */
#define HAVE_MEMCMP 1

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mlock' function. */
/* #undef HAVE_MLOCK */

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 where mmap() incrementally extends the accessible mapping as
   the underlying file grows. */
/* #undef HAVE_MMAP_EXTEND */

/* Define to 1 if you have the `mprotect' function. */
/* #undef HAVE_MPROTECT */

/* Define to 1 if you have the `munlock' function. */
/* #undef HAVE_MUNLOCK */

/* Define to 1 if you have the `munmap' function. */
/* #undef HAVE_MUNMAP */

/* Define to 1 to use the GCC compiler and 68K assembly language mutexes. */
/* #undef HAVE_MUTEX_68K_GCC_ASSEMBLY */

/* Define to 1 to use the AIX _check_lock mutexes. */
/* #undef HAVE_MUTEX_AIX_CHECK_LOCK */

/* Define to 1 to use the GCC compiler and Alpha assembly language mutexes. */
/* #undef HAVE_MUTEX_ALPHA_GCC_ASSEMBLY */

/* Define to 1 to use the GCC compiler and ARM assembly language mutexes. */
/* #undef HAVE_MUTEX_ARM_GCC_ASSEMBLY */

/* Define to 1 to use the Apple/Darwin _spin_lock_try mutexes. */
/* #undef HAVE_MUTEX_DARWIN_SPIN_LOCK_TRY */

/* Define to 1 to use the UNIX fcntl system call mutexes. */
/* #undef HAVE_MUTEX_FCNTL */

/* Define to 1 to use the GCC compiler and PaRisc assembly language mutexes.
   */
/* #undef HAVE_MUTEX_HPPA_GCC_ASSEMBLY */

/* Define to 1 to use the msem_XXX mutexes on HP-UX. */
/* #undef HAVE_MUTEX_HPPA_MSEM_INIT */

/* Define to 1 to use test-and-set mutexes with blocking mutexes. */
/* #undef HAVE_MUTEX_HYBRID */

/* Define to 1 to use the GCC compiler and IA64 assembly language mutexes. */
/* #undef HAVE_MUTEX_IA64_GCC_ASSEMBLY */

/* Define to 1 to use the GCC compiler and MIPS assembly language mutexes. */
/* #undef HAVE_MUTEX_MIPS_GCC_ASSEMBLY */

/* Define to 1 to use the msem_XXX mutexes on systems other than HP-UX. */
/* #undef HAVE_MUTEX_MSEM_INIT */

/* Define to 1 to use the GCC compiler and PowerPC assembly language mutexes.
   */
/* #undef HAVE_MUTEX_PPC_GCC_ASSEMBLY */

/* Define to 1 to use POSIX 1003.1 pthread_XXX mutexes. */
/* #undef HAVE_MUTEX_PTHREADS */

/* Define to 1 to use Reliant UNIX initspin mutexes. */
/* #undef HAVE_MUTEX_RELIANTUNIX_INITSPIN */

/* Define to 1 to use the IBM C compiler and S/390 assembly language mutexes.
   */
/* #undef HAVE_MUTEX_S390_CC_ASSEMBLY */

/* Define to 1 to use the GCC compiler and S/390 assembly language mutexes. */
/* #undef HAVE_MUTEX_S390_GCC_ASSEMBLY */

/* Define to 1 to use the SCO compiler and x86 assembly language mutexes. */
/* #undef HAVE_MUTEX_SCO_X86_CC_ASSEMBLY */

/* Define to 1 to use the obsolete POSIX 1003.1 sema_XXX mutexes. */
/* #undef HAVE_MUTEX_SEMA_INIT */

/* Define to 1 to use the SGI XXX_lock mutexes. */
/* #undef HAVE_MUTEX_SGI_INIT_LOCK */

/* Define to 1 to use the Solaris _lock_XXX mutexes. */
/* #undef HAVE_MUTEX_SOLARIS_LOCK_TRY */

/* Define to 1 to use the Solaris lwp threads mutexes. */
/* #undef HAVE_MUTEX_SOLARIS_LWP */

/* Define to 1 to use the GCC compiler and Sparc assembly language mutexes. */
/* #undef HAVE_MUTEX_SPARC_GCC_ASSEMBLY */

/* Define to 1 if the Berkeley DB library should support mutexes. */
#define HAVE_MUTEX_SUPPORT 1

/* Define to 1 if mutexes hold system resources. */
#define HAVE_MUTEX_SYSTEM_RESOURCES 1

/* Define to 1 to configure mutexes intra-process only. */
/* #undef HAVE_MUTEX_THREAD_ONLY */

/* Define to 1 to use the CC compiler and Tru64 assembly language mutexes. */
/* #undef HAVE_MUTEX_TRU64_CC_ASSEMBLY */

/* Define to 1 to use the UNIX International mutexes. */
/* #undef HAVE_MUTEX_UI_THREADS */

/* Define to 1 to use the UTS compiler and assembly language mutexes. */
/* #undef HAVE_MUTEX_UTS_CC_ASSEMBLY */

/* Define to 1 to use VMS mutexes. */
/* #undef HAVE_MUTEX_VMS */

/* Define to 1 to use VxWorks mutexes. */
#define HAVE_MUTEX_VXWORKS 1

/* Define to 1 to use the MSVC compiler and Windows mutexes. */
/* #undef HAVE_MUTEX_WIN32 */

/* Define to 1 to use the GCC compiler and Windows mutexes. */
/* #undef HAVE_MUTEX_WIN32_GCC */

/* Define to 1 to use the GCC compiler and 64-bit x86 assembly language
   mutexes. */
/* #undef HAVE_MUTEX_X86_64_GCC_ASSEMBLY */

/* Define to 1 to use the GCC compiler and 32-bit x86 assembly language
   mutexes. */
/* #undef HAVE_MUTEX_X86_GCC_ASSEMBLY */

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the O_DIRECT flag. */
/* #undef HAVE_O_DIRECT */

/* Define to 1 if building partitioned database support. */
/* #undef HAVE_PARTITION */

/* Define to 1 to enable some kind of performance event monitoring. */
/* #undef HAVE_PERFMON */

/* Define to 1 to enable performance event monitoring of *_stat() statistics.
   */
/* #undef HAVE_PERFMON_STATISTICS */

/* Define to 1 if you have the `pread' function. */
/* #undef HAVE_PREAD */

/* Define to 1 if you have the `printf' function. */
#define HAVE_PRINTF 1

/* Define to 1 if you have the `pstat_getdynamic' function. */
/* #undef HAVE_PSTAT_GETDYNAMIC */

/* Define to 1 if it is OK to initialize an already initialized
   pthread_cond_t. */
/* #undef HAVE_PTHREAD_COND_REINIT_OKAY */

/* Define to 1 if it is OK to initialize an already initialized
   pthread_rwlock_t. */
/* #undef HAVE_PTHREAD_RWLOCK_REINIT_OKAY */

/* Define to 1 if you have the `pthread_self' function. */
/* #undef HAVE_PTHREAD_SELF */

/* Define to 1 if you have the `pthread_yield' function. */
/* #undef HAVE_PTHREAD_YIELD */

/* Define to 1 if you have the `pwrite' function. */
/* #undef HAVE_PWRITE */

/* Define to 1 if building on QNX. */
/* #undef HAVE_QNX */

/* Define to 1 if you have the `qsort' function. */
#define HAVE_QSORT 1

/* Define to 1 if building Queue access method. */
/* #undef HAVE_QUEUE */

/* Define to 1 if you have the `raise' function. */
#define HAVE_RAISE 1

/* Define to 1 if you have the `rand' function. */
#define HAVE_RAND 1

/* Define to 1 if you have the `random' function. */
/* #undef HAVE_RANDOM */

/* Define to 1 if building replication support. */
/* #undef HAVE_REPLICATION */

/* Define to 1 if building the Berkeley DB replication framework. */
/* #undef HAVE_REPLICATION */

/* Define to 1 if you have the `sched_yield' function. */
#define HAVE_SCHED_YIELD 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setgid' function. */
#define HAVE_SETGID 1

/* Define to 1 if you have the `setuid' function. */
#define HAVE_SETUID 1

/* Define to 1 to configure Berkeley DB to use shared, read/write latches. */
#define HAVE_SHARED_LATCHES 1

/* Define to 1 if shmctl/SHM_LOCK locks down shared memory segments. */
/* #undef HAVE_SHMCTL_SHM_LOCK */

/* Define to 1 if you have the `shmget' function. */
/* #undef HAVE_SHMGET */

/* Define to 1 if you have the `sigaction' function. */
/* #undef HAVE_SIGACTION */

/* Define to 1 if thread identifier type db_threadid_t is integral. */
#define HAVE_SIMPLE_THREAD_TYPE 1

/* Define to 1 if you have the `snprintf' function. */
/* #undef HAVE_SNPRINTF */

/* Define to 1 if you have the `stat' function. */
#define HAVE_STAT 1

/* Define to 1 if building statistics support. */
/* #undef HAVE_STATISTICS */

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
/* #undef HAVE_STRCASECMP */

/* Define to 1 if you have the `strcat' function. */
#define HAVE_STRCAT 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strdup' function. */
/* #undef HAVE_STRDUP */

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if building without output message content. */
/* #undef HAVE_STRIPPED_MESSAGES */

/* Define to 1 if you have the `strncat' function. */
#define HAVE_STRNCAT 1

/* Define to 1 if you have the `strncmp' function. */
#define HAVE_STRNCMP 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strsep' function. */
/* #undef HAVE_STRSEP */

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if `st_blksize' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1

/* Define to 1 if you have the `sysconf' function. */
/* #undef HAVE_SYSCONF */

/* Define to 1 if port includes files in the Berkeley DB source code. */
#define HAVE_SYSTEM_INCLUDE_FILES 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/sdt.h> header file. */
/* #undef HAVE_SYS_SDT_H */

/* Define to 1 if you have the <sys/select.h> header file. */
/* #undef HAVE_SYS_SELECT_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #undef HAVE_SYS_STAT_H */

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the <sys/types.h> header file. */
/* #undef HAVE_SYS_TYPES_H */

/* Define to 1 if you have the `time' function. */
#define HAVE_TIME 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if unlink of file with open file descriptors will fail. */
#define HAVE_UNLINK_WITH_OPEN_FAILURE 1

/* Define to 1 if port includes historic database upgrade support. */
#define	HAVE_UPGRADE_SUPPORT 1

/* Define to 1 if building access method verification support. */
/* #undef HAVE_VERIFY */

/* Define to 1 if you have the `vsnprintf' function. */
/* #undef HAVE_VSNPRINTF */

/* Define to 1 if building VxWorks. */
#define HAVE_VXWORKS 1

/* Define to 1 if you have the `yield' function. */
/* #undef HAVE_YIELD */

/* Define to 1 if you have the `_fstati64' function. */
/* #undef HAVE__FSTATI64 */

/* Define to the sub-directory in which libtool stores uninstalled libraries. */
/* #undef LT_OBJDIR */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "Oracle Technology Network Berkeley DB forum"

/* Define to the full name of this package. */
#define PACKAGE_NAME "Berkeley DB"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "Berkeley DB 6.0.30"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "db-6.0.30"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.oracle.com/technology/software/products/berkeley-db/index.html"

/* Define to the version of this package. */
#define PACKAGE_VERSION "6.0.30"

/* The size of a `char', as computed by sizeof. */
/* #undef SIZEOF_CHAR */

/* The size of a `char *', as computed by sizeof. */
#define SIZEOF_CHAR_P 4

/* The size of a `int', as computed by sizeof. */
/* #undef SIZEOF_INT */

/* The size of a `long', as computed by sizeof. */
/* #undef SIZEOF_LONG */

/* The size of a `long long', as computed by sizeof. */
/* #undef SIZEOF_LONG_LONG */

/* The size of `off_t', as computed by sizeof. */
/* #undef SIZEOF_OFF_T */

/* The size of a `short', as computed by sizeof. */
/* #undef SIZEOF_SHORT */

/* The size of a `size_t', as computed by sizeof. */
/* #undef SIZEOF_SIZE_T */

/* The size of a `unsigned char', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_CHAR */

/* The size of a `unsigned int', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_INT */

/* The size of a `unsigned long', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_LONG */

/* The size of a `unsigned long long', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_LONG_LONG */

/* The size of a `unsigned short', as computed by sizeof. */
/* #undef SIZEOF_UNSIGNED_SHORT */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
/* #undef TIME_WITH_SYS_TIME */

/* Define to 1 to mask harmless uninitialized memory read/writes. */
/* #undef UMRW */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline
#endif

/* type to use in place of socklen_t if not defined */
/* #undef socklen_t */
