/* include/config.h.  Generated from config.h.in by configure.  */
/* include/config.h.in.  Generated from configure.in by autoheader.  */

/* Define if /dev/dlpi is a directory. */
/* #undef DLPI_DEV_PREFIX */

/* Define if /dev/dlpi is available. */
/* #undef HAVE_DEV_DLPI */

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if the <sys/dlpi.h> header exists. */
/* #undef HAVE_DLPI */

/* Define to 1 if you have the `gethostbyname2' function. */
#define HAVE_GETHOSTBYNAME2 1

/* Define if we're building on HP/UX. */
/* #undef HAVE_HPUX11 */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define to 1 if you have the `packet' library (-lpacket). */
/* #undef HAVE_LIBPACKET */

/* Define to 1 if you have the `resolv' library (-lresolv). */
/* #undef HAVE_LIBRESOLV */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the `wpcap' library (-lwpcap). */
/* #undef HAVE_LIBWPCAP */

/* Define if you have the Linux /proc filesystem. */
#define HAVE_LINUX_PROCFS 1

/* Define to 1 if you have the <linux/socket.h> header file. */
#define HAVE_LINUX_SOCKET_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <net/ethernet.h> header file. */
/* #undef HAVE_NET_ETHERNET_H */

/* Define to 1 if you have the <net/pfilt.h> header file. */
/* #undef HAVE_NET_PFILT_H */

/* Define to 1 if you have the <net/raw.h> header file. */
/* #undef HAVE_NET_RAW_H */

/* Define if we're running on a Linux system with PF_PACKET sockets. */
/* #undef HAVE_PACKET_SOCKET */

/* Define if the sockaddr structure includes a sa_len member. */
/* #undef HAVE_SOCKADDR_SA_LEN */

/* Define if we are running on Solaris. */
/* #undef HAVE_SOLARIS */

/* Define if our version of Solaris supports IPv6. */
/* #undef HAVE_SOLARIS_IPV6 */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/bufmod.h> header file. */
/* #undef HAVE_SYS_BUFMOD_H */

/* Define to 1 if you have the <sys/dlpi_ext.h> header file. */
/* #undef HAVE_SYS_DLPI_EXT_H */

/* Define to 1 if you have the <sys/dlpi.h> header file. */
/* #undef HAVE_SYS_DLPI_H */

/* Define to 1 if you have the <sys/net/nit.h> header file. */
/* #undef HAVE_SYS_NET_NIT_H */

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* We are running on a big-endian machine. */
/* #undef LIBNET_BIG_ENDIAN */

/* Define if our build OS supports the BSD APIs */
/* #undef LIBNET_BSDISH_OS */

/* Define if libnet should byteswap data. */
/* #undef LIBNET_BSD_BYTE_SWAP */

/* We are running on a little-endian machine. */
#define LIBNET_LIL_ENDIAN 1

/* Define if snprintf() is unavailable on our system. */
/* #undef NO_SNPRINTF */

/* Name of package */
#define PACKAGE "libnet"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libnet"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libnet 1.1.5"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libnet"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.1.5"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if our version of Solaris has broken checksums. */
/* #undef STUPID_SOLARIS_CHECKSUM_BUG */

/* Version number of package */
#define VERSION "1.1.5"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* # undef _ALL_SOURCE */
#endif

/* Define as necessary to "unhide" header symbols. */
#define _BSD_SOURCE 1

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define for Solaris 2.5.1 so the uint32_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef was allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT32_T */

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef was allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define as necessary to "unhide" header symbols. */
#define __BSD_SOURCE 1

/* Enable extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif

/* Define if we should favor the BSD APIs when possible in Linux. */
#define __FAVOR_BSD 1

/* Define to the type of an unsigned integer type of width exactly 16 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint16_t */

/* Define to the type of an unsigned integer type of width exactly 32 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint32_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */
