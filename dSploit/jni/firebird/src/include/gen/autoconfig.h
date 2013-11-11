/* src/include/gen/autoconfig.h.  Generated from config.h.in by configure.  */
/* builds/make.new/config/config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define this if OS is AIX */
/* #undef AIX */

/* Define this if CPU is Alpha */
/* #undef ALPHA */

/* Define this if CPU is amd64 */
/* #undef AMD64 */

/* Include pthread support for binary relocation? */
#define BR_PTHREAD 0

/* Define this if paths are case sensitive */
#define CASE_SENSITIVITY false

/* Define this if OS is DARWIN */
/* #undef DARWIN */

/* Use binary relocation? */
#define ENABLE_BINRELOC /**/

/* Alignment of long */
#define FB_ALIGNMENT 8

/* executables DIR (PREFIX/bin) */
#define FB_BINDIR ""

/* config files DIR (PREFIX) */
#define FB_CONFDIR ""

/* documentation root DIR (PREFIX/doc) */
#define FB_DOCDIR ""

/* Alignment of double */
#define FB_DOUBLE_ALIGN 8

/* guardian lock DIR (PREFIX) */
#define FB_GUARDDIR ""

/* QLI help DIR (PREFIX/help) */
#define FB_HELPDIR ""

/* C/C++ header files DIR (PREFIX/include) */
#define FB_INCDIR ""

/* international DIR (PREFIX/intl) */
#define FB_INTLDIR ""

/* Local IPC name */
#define FB_IPC_NAME "FirebirdIPI"

/* object code libraries DIR (PREFIX/lib) */
#define FB_LIBDIR ""

/* log files DIR (PREFIX) */
#define FB_LOGDIR ""

/* misc DIR (PREFIX/misc) */
#define FB_MISCDIR ""

/* message files DIR (PREFIX) */
#define FB_MSGDIR ""

/* Wnet pipe name */
#define FB_PIPE_NAME "interbas"

/* plugins DIR (PREFIX) */
#define FB_PLUGDIR ""

/* Installation path prefix */
#define FB_PREFIX "/opt/android-ndk/platforms/android-18/arch-arm/usr"

/* examples database DIR (PREFIX/examples/empbuild) */
#define FB_SAMPLEDBDIR ""

/* examples DIR (PREFIX/examples) */
#define FB_SAMPLEDIR ""

/* system admin executables DIR (PREFIX/bin) */
#define FB_SBINDIR ""

/* security database DIR (PREFIX) */
#define FB_SECDBDIR ""

/* Inet service name */
#define FB_SERVICE_NAME "gds_db"

/* Inet service port */
#define FB_SERVICE_PORT 3050

/* UDF DIR (PREFIX/UDF) */
#define FB_UDFDIR ""

/* Define this if OS is FreeBSD */
/* #undef FREEBSD */

/* Define this if getmntent needs second argument */
/* #undef GETMNTENT_TAKES_TWO_ARGUMENTS */

/* Define to 1 if the `getpgrp' function requires zero arguments. */
#define GETPGRP_VOID 1

/* Define this if gettimeofday accepts second (timezone) argument */
#define GETTIMEOFDAY_RETURNS_TIMEZONE 1

/* Define this if GPRE should support ADA */
/* #undef GPRE_ADA */

/* Define this if GPRE should support COBOL */
/* #undef GPRE_COBOL */

/* Define this if GPRE should support FORTRAN */
/* #undef GPRE_FORTRAN */

/* Define this if GPRE should support PASCAL */
/* #undef GPRE_PASCAL */

/* Define to 1 if you have the <aio.h> header file. */
/* #undef HAVE_AIO_H */

/* Define this if AO_compare_and_swap_full() is defined in atomic_ops.h */
/* #undef HAVE_AO_COMPARE_AND_SWAP_FULL */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the <atomic.h> header file. */
/* #undef HAVE_ATOMIC_H */

/* Define to 1 if you have the <atomic_ops.h> header file. */
/* #undef HAVE_ATOMIC_OPS_H */

/* Define to 1 if the system has the type `caddr_t'. */
/* #undef HAVE_CADDR_T */

/* Define to 1 if you have the <crypt.h> header file. */
/* #undef HAVE_CRYPT_H */

/* Define to 1 if you have the <ctype.h> header file. */
#define HAVE_CTYPE_H 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `dirname' function. */
#define HAVE_DIRNAME 1

/* Define to 1 if you have the `dladdr' function. */
#define HAVE_DLADDR 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define this if editline is in use */
#define HAVE_EDITLINE_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the `fchmod' function. */
#define HAVE_FCHMOD 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdatasync' function. */
#define HAVE_FDATASYNC 1

/* Define to 1 if you have the `fegetenv' function. */
/* #undef HAVE_FEGETENV */

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `flock' function. */
#define HAVE_FLOCK 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `fsync' function. */
#define HAVE_FSYNC 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `getmntent' function. */
#define HAVE_GETMNTENT 1

/* Define to 1 if you have the `getpagesize' function. */
/* #undef HAVE_GETPAGESIZE */

/* Define to 1 if you have the `getrlimit' function. */
#define HAVE_GETRLIMIT 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `getwd' function. */
/* #undef HAVE_GETWD */

/* Define to 1 if you have the `gmtime_r' function. */
#define HAVE_GMTIME_R 1

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define this if INFINITY is defined in math.h */
#define HAVE_INFINITY 1

/* Define to 1 if you have the `initgroups' function. */
#define HAVE_INITGROUPS 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `atomic_ops' library (-latomic_ops). */
/* #undef HAVE_LIBATOMIC_OPS */

/* Define to 1 if you have the <libio.h> header file. */
/* #undef HAVE_LIBIO_H */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
/* #undef HAVE_LIBPTHREAD */

/* Define to 1 if you have the `sfio' library (-lsfio). */
/* #undef HAVE_LIBSFIO */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `llrint' function. */
#define HAVE_LLRINT 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `localtime_r' function. */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <mntent.h> header file. */
#define HAVE_MNTENT_H 1

/* Define to 1 if you have the <mnttab.h> header file. */
/* #undef HAVE_MNTTAB_H */

/* Define this if multi-threading should be supported */
#define HAVE_MULTI_THREAD 1

/* Define to 1 if you have the `nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netconfig.h> header file. */
/* #undef HAVE_NETCONFIG_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <poll.h> header file. */
#define HAVE_POLL_H 1

/* Define this if posix_fadvise() is present on the platform */
/* #undef HAVE_POSIX_FADVISE */

/* Define to 1 if you have the `pread' function. */
#define HAVE_PREAD 1

/* Define if you have POSIX threads libraries and header files. */
/* #undef HAVE_PTHREAD */

/* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_PTHREAD_H 1

/* Define to 1 if you have the `pthread_keycreate' function. */
/* #undef HAVE_PTHREAD_KEYCREATE */

/* Define to 1 if you have the `pthread_key_create' function. */
#define HAVE_PTHREAD_KEY_CREATE 1

/* Define to 1 if you have the `pthread_mutexattr_setprotocol' function. */
/* #undef HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL */

/* Define to 1 if you have the `pthread_mutexattr_setrobust_np' function. */
/* #undef HAVE_PTHREAD_MUTEXATTR_SETROBUST_NP */

/* Define to 1 if you have the `pthread_mutex_consistent_np' function. */
/* #undef HAVE_PTHREAD_MUTEX_CONSISTENT_NP */

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `pwrite' function. */
#define HAVE_PWRITE 1

/* Define to 1 if you have the <rpc/rpc.h> header file. */
/* #undef HAVE_RPC_RPC_H */

/* Define to 1 if you have the <rpc/xdr.h> header file. */
/* #undef HAVE_RPC_XDR_H */

/* Define to 1 if you have the <semaphore.h> header file. */
#define HAVE_SEMAPHORE_H 1

/* Define to 1 if you have the `semtimedop' function. */
/* #undef HAVE_SEMTIMEDOP */

/* Define to 1 if the system has the type `semun'. */
/* #undef HAVE_SEMUN */

/* Define to 1 if you have the `sem_init' function. */
#define HAVE_SEM_INIT 1

/* Define to 1 if you have the `sem_timedwait' function. */
#define HAVE_SEM_TIMEDWAIT 1

/* Define to 1 if you have the `setitimer' function. */
#define HAVE_SETITIMER 1

/* Define to 1 if you have the <setjmp.h> header file. */
#define HAVE_SETJMP_H 1

/* Define to 1 if you have the `setmntent' function. */
/* #undef HAVE_SETMNTENT */

/* Define to 1 if you have the `setpgid' function. */
#define HAVE_SETPGID 1

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the `sigaction' function. */
#define HAVE_SIGACTION 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <socket.h> header file. */
/* #undef HAVE_SOCKET_H */

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror_r' function. */
#define HAVE_STRERROR_R 1

/* Define to 1 if you have the `stricmp' function. */
/* #undef HAVE_STRICMP */

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the `strnicmp' function. */
/* #undef HAVE_STRNICMP */

/* Define this if struct dirent has d_type */
#define HAVE_STRUCT_DIRENT_D_TYPE 1

/* Define to 1 if the system has the type `struct xdr_ops'. */
/* #undef HAVE_STRUCT_XDR_OPS */

/* Define to 1 if the system has the type `struct XDR::xdr_ops'. */
/* #undef HAVE_STRUCT_XDR__XDR_OPS */

/* Define to 1 if you have the `swab' function. */
/* #undef HAVE_SWAB */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/ipc.h> header file. */
#define HAVE_SYS_IPC_H 1

/* Define to 1 if you have the <sys/mntent.h> header file. */
/* #undef HAVE_SYS_MNTENT_H */

/* Define to 1 if you have the <sys/mnttab.h> header file. */
/* #undef HAVE_SYS_MNTTAB_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/sem.h> header file. */
/* #undef HAVE_SYS_SEM_H */

/* Define to 1 if you have the <sys/siginfo.h> header file. */
/* #undef HAVE_SYS_SIGINFO_H */

/* Define to 1 if you have the <sys/signal.h> header file. */
/* #undef HAVE_SYS_SIGNAL_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/timeb.h> header file. */
#define HAVE_SYS_TIMEB_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `tcgetattr' function. */
/* #undef HAVE_TCGETATTR */

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the <termio.h> header file. */
#define HAVE_TERMIO_H 1

/* Define to 1 if you have the `time' function. */
#define HAVE_TIME 1

/* Define to 1 if you have the `times' function. */
#define HAVE_TIMES 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if you have the <varargs.h> header file. */
/* #undef HAVE_VARARGS_H */

/* Define this if va_copy() is defined in stdarg.h */
#define HAVE_VA_COPY 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the <winsock2.h> header file. */
/* #undef HAVE_WINSOCK2_H */

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the file `AC_File'. */
#define HAVE__PROC_SELF_EXE 1

/* Define to 1 if you have the `_swab' function. */
/* #undef HAVE__SWAB */

/* Define it if compiler supports ISO syntax for thread-local storage */
#define HAVE___THREAD 1

/* Define this if CPU is HPPA */
/* #undef HPPA */

/* Define this if OS is HP-UX */
/* #undef HPUX */

/* Define this if OS is Linux */
#define LINUX 1

/* Define this if OS is NetBSD */
/* #undef NETBSD */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if the `setpgrp' function takes no argument. */
#define SETPGRP_VOID 1

/* Architecture is little-endian sh4 */
/* #undef SH */

/* Architecture is big-edian sh4 */
/* #undef SHEB */

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 4

/* Define this if OS is Solaris Sparc */
/* #undef SOLARIS */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define this if databases on raw devices should be supported */
#define SUPPORT_RAW_DEVICES 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define this if OS is Windows NT */
/* #undef WIN_NT */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define this if sem_init() works on the platform */
#define WORKING_SEM_INIT 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define this if OS is Solarix x86 */
/* #undef solx86 */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

#ifdef GETTIMEOFDAY_RETURNS_TIMEZONE
#define GETTIMEOFDAY(x) gettimeofday((x), (struct timezone *)0)
#else
#define GETTIMEOFDAY(x) gettimeofday((x))
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif
