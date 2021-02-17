
/***************************************************************************
 * nbase_misc.c -- Some small miscellaneous utility/compatibility          *
 * functions.                                                              *
 *                                                                         *
 ***********************IMPORTANT NMAP LICENSE TERMS************************
 *                                                                         *
 * The Nmap Security Scanner is (C) 1996-2013 Insecure.Com LLC. Nmap is    *
 * also a registered trademark of Insecure.Com LLC.  This program is free  *
 * software; you may redistribute and/or modify it under the terms of the  *
 * GNU General Public License as published by the Free Software            *
 * Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE CLARIFICATIONS  *
 * AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your right to use,    *
 * modify, and redistribute this software under certain conditions.  If    *
 * you wish to embed Nmap technology into proprietary software, we sell    *
 * alternative licenses (contact sales@nmap.com).  Dozens of software      *
 * vendors already license Nmap technology such as host discovery, port    *
 * scanning, OS detection, version detection, and the Nmap Scripting       *
 * Engine.                                                                 *
 *                                                                         *
 * Note that the GPL places important restrictions on "derivative works",  *
 * yet it does not provide a detailed definition of that term.  To avoid   *
 * misunderstandings, we interpret that term as broadly as copyright law   *
 * allows.  For example, we consider an application to constitute a        *
 * derivative work for the purpose of this license if it does any of the   *
 * following with any software or content covered by this license          *
 * ("Covered Software"):                                                   *
 *                                                                         *
 * o Integrates source code from Covered Software.                         *
 *                                                                         *
 * o Reads or includes copyrighted data files, such as Nmap's nmap-os-db   *
 * or nmap-service-probes.                                                 *
 *                                                                         *
 * o Is designed specifically to execute Covered Software and parse the    *
 * results (as opposed to typical shell or execution-menu apps, which will *
 * execute anything you tell them to).                                     *
 *                                                                         *
 * o Includes Covered Software in a proprietary executable installer.  The *
 * installers produced by InstallShield are an example of this.  Including *
 * Nmap with other software in compressed or archival form does not        *
 * trigger this provision, provided appropriate open source decompression  *
 * or de-archiving software is widely available for no charge.  For the    *
 * purposes of this license, an installer is considered to include Covered *
 * Software even if it actually retrieves a copy of Covered Software from  *
 * another source during runtime (such as by downloading it from the       *
 * Internet).                                                              *
 *                                                                         *
 * o Links (statically or dynamically) to a library which does any of the  *
 * above.                                                                  *
 *                                                                         *
 * o Executes a helper program, module, or script to do any of the above.  *
 *                                                                         *
 * This list is not exclusive, but is meant to clarify our interpretation  *
 * of derived works with some common examples.  Other people may interpret *
 * the plain GPL differently, so we consider this a special exception to   *
 * the GPL that we apply to Covered Software.  Works which meet any of     *
 * these conditions must conform to all of the terms of this license,      *
 * particularly including the GPL Section 3 requirements of providing      *
 * source code and allowing free redistribution of the work as a whole.    *
 *                                                                         *
 * As another special exception to the GPL terms, Insecure.Com LLC grants  *
 * permission to link the code of this program with any version of the     *
 * OpenSSL library which is distributed under a license identical to that  *
 * listed in the included docs/licenses/OpenSSL.txt file, and distribute   *
 * linked combinations including the two.                                  *
 *                                                                         *
 * Any redistribution of Covered Software, including any derived works,    *
 * must obey and carry forward all of the terms of this license, including *
 * obeying all GPL rules and restrictions.  For example, source code of    *
 * the whole work must be provided and free redistribution must be         *
 * allowed.  All GPL references to "this License", are to be treated as    *
 * including the terms and conditions of this license text as well.        *
 *                                                                         *
 * Because this license imposes special exceptions to the GPL, Covered     *
 * Work may not be combined (even as part of a larger work) with plain GPL *
 * software.  The terms, conditions, and exceptions of this license must   *
 * be included as well.  This license is incompatible with some other open *
 * source licenses as well.  In some cases we can relicense portions of    *
 * Nmap or grant special permissions to use it in other open source        *
 * software.  Please contact fyodor@nmap.org with any such requests.       *
 * Similarly, we don't incorporate incompatible open source software into  *
 * Covered Software without special permission from the copyright holders. *
 *                                                                         *
 * If you have any questions about the licensing restrictions on using     *
 * Nmap in other works, are happy to help.  As mentioned above, we also    *
 * offer alternative license to integrate Nmap into proprietary            *
 * applications and appliances.  These contracts have been sold to dozens  *
 * of software vendors, and generally include a perpetual license as well  *
 * as providing for priority support and updates.  They also fund the      *
 * continued development of Nmap.  Please email sales@nmap.com for further *
 * information.                                                            *
 *                                                                         *
 * If you have received a written license agreement or contract for        *
 * Covered Software stating terms other than these, you may choose to use  *
 * and redistribute Covered Software under those terms instead of these.   *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes (none     *
 * have been found so far).                                                *
 *                                                                         *
 * Source code also allows you to port Nmap to new platforms, fix bugs,    *
 * and add new features.  You are highly encouraged to send your changes   *
 * to the dev@nmap.org mailing list for possible incorporation into the    *
 * main distribution.  By sending these changes to Fyodor or one of the    *
 * Insecure.Org development mailing lists, or checking them into the Nmap  *
 * source code repository, it is understood (unless you specify otherwise) *
 * that you are offering the Nmap Project (Insecure.Com LLC) the           *
 * unlimited, non-exclusive right to reuse, modify, and relicense the      *
 * code.  Nmap will always be available Open Source, but this is important *
 * because the inability to relicense code has caused devastating problems *
 * for other Free Software projects (such as KDE and NASM).  We also       *
 * occasionally relicense the code to third parties as discussed above.    *
 * If you wish to specify special license conditions of your               *
 * contributions, just say so when you send them.                          *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the Nmap      *
 * license file for more details (it's in a COPYING file included with     *
 * Nmap, and also available from https://svn.nmap.org/nmap/COPYING         *
 *                                                                         *
 ***************************************************************************/

/* $Id: nbase_misc.c 32741 2014-02-20 18:44:12Z dmiller $ */

#include "nbase.h"

#ifndef WIN32
#include <errno.h>
#ifndef errno
extern int errno;
#endif
#else
#include <winsock2.h>
#endif

#include <limits.h>
#include <stdio.h>
#include "nbase_ipv6.h"
#include "nbase_crc32ct.h"

#include <assert.h>
#include <fcntl.h>

#ifdef WIN32
#include <conio.h>
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/* Returns the UNIX/Windows errno-equivalent.  Note that the Windows
   call is socket/networking specific.  The windows error number
   returned is like WSAMSGSIZE, but nbase.h includes #defines to
   correlate many of the common UNIX errors with their closest Windows
   equivalents.  So you can use EMSGSIZE or EINTR. */
int socket_errno() {
#ifdef WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

/* We can't just use strerror to get socket errors on Windows because it has
   its own set of error codes: WSACONNRESET not ECONNRESET for example. This
   function will do the right thing on Windows. Call it like
     socket_strerror(socket_errno())
*/
char *socket_strerror(int errnum) {
#ifdef WIN32
    static char buffer[128];

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        0, errnum, 0, buffer, sizeof(buffer), NULL);

    return buffer;
#else
    return strerror(errnum);
#endif
}

/* Compares two sockaddr_storage structures with a return value like strcmp.
   First the address families are compared, then the addresses if the families
   are equal. The structures must be real full-length sockaddr_storage
   structures, not something shorter like sockaddr_in. */
int sockaddr_storage_cmp(const struct sockaddr_storage *a,
  const struct sockaddr_storage *b) {
  if (a->ss_family < b->ss_family)
    return -1;
  else if (a->ss_family > b->ss_family)
    return 1;
  if (a->ss_family == AF_INET) {
    struct sockaddr_in *sin_a = (struct sockaddr_in *) a;
    struct sockaddr_in *sin_b = (struct sockaddr_in *) b;
    if (sin_a->sin_addr.s_addr < sin_b->sin_addr.s_addr)
      return -1;
    else if (sin_a->sin_addr.s_addr > sin_b->sin_addr.s_addr)
      return 1;
    else
      return 0;
  } else if (a->ss_family == AF_INET6) {
    struct sockaddr_in6 *sin6_a = (struct sockaddr_in6 *) a;
    struct sockaddr_in6 *sin6_b = (struct sockaddr_in6 *) b;
    return memcmp(sin6_a->sin6_addr.s6_addr, sin6_b->sin6_addr.s6_addr,
                  sizeof(sin6_a->sin6_addr.s6_addr));
  } else {
    assert(0);
  }
  return 0; /* Not reached */
}

int sockaddr_storage_equal(const struct sockaddr_storage *a,
  const struct sockaddr_storage *b) {
  return sockaddr_storage_cmp(a, b) == 0;
}

/* This function is an easier version of inet_ntop because you don't
   need to pass a dest buffer.  Instead, it returns a static buffer that
   you can use until the function is called again (by the same or another
   thread in the process).  If there is a weird error (like sslen being
   too short) then NULL will be returned. */
const char *inet_ntop_ez(const struct sockaddr_storage *ss, size_t sslen) {

  const struct sockaddr_in *sin = (struct sockaddr_in *) ss;
  static char str[INET6_ADDRSTRLEN];
#if HAVE_IPV6
  const struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) ss;
#endif

  str[0] = '\0';

  if (sin->sin_family == AF_INET) {
    if (sslen < sizeof(struct sockaddr_in))
      return NULL;
    return inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str));
  }
#if HAVE_IPV6
  else if(sin->sin_family == AF_INET6) {
    if (sslen < sizeof(struct sockaddr_in6))
      return NULL;
    return inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str));
  }
#endif
  //Some laptops report the ip and address family of disabled wifi cards as null
  //so yes, we will hit this sometimes.
  return NULL;
}

/* Create a new socket inheritable by subprocesses. On non-Windows systems it's
   just a normal socket. */
int inheritable_socket(int af, int style, int protocol) {
#ifdef WIN32
  /* WSASocket is just like socket, except that the sockets it creates are
     inheritable by subprocesses (such as are created by CreateProcess), while
     those created by socket are not. */
  return WSASocket(af, style, protocol, NULL, 0, 0);
#else
  return socket(af, style, protocol);
#endif
}

/* The dup function on Windows works only on file descriptors, not socket
   handles. This function accomplishes the same thing for sockets. */
int dup_socket(int sd) {
#ifdef WIN32
  HANDLE copy;

  if (DuplicateHandle(GetCurrentProcess(), (HANDLE) sd,
                      GetCurrentProcess(), &copy,
                      0, FALSE, DUPLICATE_SAME_ACCESS) == 0) {
    return -1;
  }

  return (int) copy;
#else
  return dup(sd);
#endif
}

int unblock_socket(int sd) {
#ifdef WIN32
  unsigned long one = 1;

  if (sd != 501) /* Hack related to WinIP Raw Socket support */
    ioctlsocket(sd, FIONBIO, &one);

  return 0;
#else
  int options;

  /* Unblock our socket to prevent recvfrom from blocking forever on certain
   * target ports. */
  options = fcntl(sd, F_GETFL);
  if (options == -1)
    return -1;

  return fcntl(sd, F_SETFL, O_NONBLOCK | options);
#endif /* WIN32 */
}

/* Convert a socket to blocking mode */
int block_socket(int sd) {
#ifdef WIN32
  unsigned long options = 0;

  if (sd != 501)
    ioctlsocket(sd, FIONBIO, &options);

  return 0;
#else
  int options;

  options = fcntl(sd, F_GETFL);
  if (options == -1)
    return -1;

  return fcntl(sd, F_SETFL, (~O_NONBLOCK) & options);
#endif
}

/* Use the SO_BINDTODEVICE sockopt to bind with a specific interface (Linux
   only). Pass NULL or an empty string to remove device binding. */
int socket_bindtodevice(int sd, const char *device) {
  char padded[sizeof(int)];
  size_t len;

  len = strlen(device) + 1;
  /* In Linux 2.6.20 and earlier, there is a bug in SO_BINDTODEVICE that causes
     EINVAL to be returned if the optlen < sizeof(int); this happens for example
     with the interface names "" and "lo". Pad the string with null characters
     so it is above this limit if necessary.
     http://article.gmane.org/gmane.linux.network/71887
     http://article.gmane.org/gmane.linux.network/72216 */
  if (len < sizeof(padded)) {
    /* We rely on strncpy padding with nulls here. */
    strncpy(padded, device, sizeof(padded));
    device = padded;
    len = sizeof(padded);
  }

#ifdef SO_BINDTODEVICE
  /* Linux-specific sockopt asking to use a specific interface. See socket(7). */
  if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, device, len) < 0)
    return 0;
#endif

  return 1;
}

/* Convert a time specification into a count of seconds. A time specification is
 * a non-negative real number, possibly followed by a units suffix. The suffixes
 * are "ms" for milliseconds, "s" for seconds, "m" for minutes, or "h" for
 * hours. Seconds is the default with no suffix. -1 is returned if the string
 * can't be parsed. */
double tval2secs(const char *tspec) {
  double d;
  char *tail;

  errno = 0;
  d = strtod(tspec, &tail);
  if (*tspec == '\0' || errno != 0)
    return -1;
  if (strcasecmp(tail, "ms") == 0)
    return d / 1000.0;
  else if (*tail == '\0' || strcasecmp(tail, "s") == 0)
    return d;
  else if (strcasecmp(tail, "m") == 0)
    return d * 60.0;
  else if (strcasecmp(tail, "h") == 0)
    return d * 60.0 * 60.0;
  else
    return -1;
}

long tval2msecs(const char *tspec) {
  double s, ms;

  s = tval2secs(tspec);
  if (s == -1)
    return -1;
  ms = s * 1000.0;
  if (ms > LONG_MAX || ms < LONG_MIN)
    return -1;

  return (long) ms;
}

/* Returns the unit portion of a time specification (such as "ms", "s", "m", or
   "h"). Returns NULL if there was a parsing error or no unit is present. */
const char *tval_unit(const char *tspec) {
  double d;
  char *tail;

  errno = 0;
  d = strtod(tspec, &tail);
  /* Avoid GCC 4.6 error "variable 'd' set but not used
     [-Wunused-but-set-variable]". */
  (void) d;
  if (*tspec == '\0' || errno != 0 || *tail == '\0')
    return NULL;

  return tail;
}

/* A replacement for select on Windows that allows selecting on stdin
 * (file descriptor 0) and selecting on zero file descriptors (just for
 * the timeout). Plain Windows select doesn't work on non-sockets like
 * stdin and returns an error if no file descriptors were given, because
 * they were NULL or empty.  This only works for sockets and stdin; if
 * you have a descriptor referring to a normal open file in the set,
 * Windows will return WSAENOTSOCK. */
int fselect(int s, fd_set *rmaster, fd_set *wmaster, fd_set *emaster, struct timeval *tv)
{
#ifdef WIN32
    static int stdin_thread_started = 0;
    int fds_ready = 0;
    int iter = -1, i;
    struct timeval stv;
    fd_set rset, wset, eset;

    /* Figure out whether there are any FDs in the sets, as @$@!$# Windows
       returns WSAINVAL (10022) if you call a select() with no FDs, even though
       the Linux man page says that doing so is a good, reasonably portable way
       to sleep with subsecond precision.  Sigh. */
    for(i = s; i > STDIN_FILENO; i--) {
        if ((rmaster != NULL && FD_ISSET(i, rmaster))
            || (wmaster != NULL && FD_ISSET(i, wmaster))
            || (emaster != NULL && FD_ISSET(i, emaster)))
            break;
        s--;
    }

    /* Handle the case where stdin is not being read from. */
    if (rmaster == NULL || !FD_ISSET(STDIN_FILENO, rmaster)) {
        if (s > 0) {
            /* Do a normal select. */
            return select(s, rmaster, wmaster, emaster, tv);
        } else {
            /* No file descriptors given. Just sleep. */
            if (tv == NULL) {
                /* Sleep forever. */
                while (1)
                    sleep(10000);
            } else {
                usleep(tv->tv_sec * 1000000UL + tv->tv_usec);
                return 0;
            }
        }
    }

    /* This is a hack for Windows, which doesn't allow select()ing on
     * non-sockets (like stdin).  We remove stdin from the fd_set and
     * loop while select()ing on everything else, with a timeout of
     * 125ms.  Then we check if stdin is ready and increment fds_ready
     * and set stdin in rmaster if it looks good.  We just keep looping
     * until we have something or it times out.
     */

    /* nbase_winunix.c has all the nasty details behind checking if
     * stdin has input. It involves a background thread, which we start
     * now if necessary. */
    if (!stdin_thread_started) {
        int ret = win_stdin_start_thread();
        assert(ret != 0);
        stdin_thread_started = 1;
    }

    FD_CLR(STDIN_FILENO, rmaster);

    if (tv) {
        int usecs = (tv->tv_sec * 1000000) + tv->tv_usec;

        iter = usecs / 125000;

        if (usecs % 125000)
            iter++;
    }

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_ZERO(&eset);

    while (!fds_ready && iter) {
        stv.tv_sec = 0;
        stv.tv_usec = 125000;

        if (rmaster)
            rset = *rmaster;
        if (wmaster)
            wset = *wmaster;
        if (emaster)
            eset = *emaster;

        fds_ready = 0;
        /* selecting on anything other than stdin? */
        if (s > 1)
            fds_ready = select(s, &rset, &wset, &eset, &stv);
        else
            usleep(stv.tv_sec * 1000000UL + stv.tv_usec);

        if (fds_ready > -1 && win_stdin_ready()) {
            FD_SET(STDIN_FILENO, &rset);
            fds_ready++;
        }

        if (tv)
            iter--;
    }

    if (rmaster)
        *rmaster = rset;
    if (wmaster)
        *wmaster = wset;
    if (emaster)
        *emaster = eset;

    return fds_ready;
#else
    return select(s, rmaster, wmaster, emaster, tv);
#endif
}


/*
 * CRC32 Cyclic Redundancy Check
 *
 * From: http://www.ietf.org/rfc/rfc1952.txt
 *
 * Copyright (c) 1996 L. Peter Deutsch
 *
 * Permission is granted to copy and distribute this document for any
 * purpose and without charge, including translations into other
 * languages and incorporation into compilations, provided that the
 * copyright notice and this notice are preserved, and that any
 * substantive changes or deletions from the original are clearly
 * marked.
 *
 */

/* Table of CRCs of all 8-bit messages. */
static unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void)
{
  unsigned long c;
  int n, k;

  for (n = 0; n < 256; n++) {
    c = (unsigned long) n;
    for (k = 0; k < 8; k++) {
      if (c & 1) {
        c = 0xedb88320L ^ (c >> 1);
      } else {
        c = c >> 1;
      }
    }
    crc_table[n] = c;
  }
  crc_table_computed = 1;
}

/*
   Update a running crc with the bytes buf[0..len-1] and return
 the updated crc. The crc should be initialized to zero. Pre- and
 post-conditioning (one's complement) is performed within this
 function so it shouldn't be done by the caller. Usage example:

   unsigned long crc = 0L;

   while (read_buffer(buffer, length) != EOF) {
     crc = update_crc(crc, buffer, length);
   }
   if (crc != original_crc) error();
*/
static unsigned long update_crc(unsigned long crc,
                unsigned char *buf, int len)
{
  unsigned long c = crc ^ 0xffffffffL;
  int n;

  if (!crc_table_computed)
    make_crc_table();
  for (n = 0; n < len; n++) {
    c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c ^ 0xffffffffL;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long nbase_crc32(unsigned char *buf, int len)
{
  return update_crc(0L, buf, len);
}


/*
 * CRC-32C (Castagnoli) Cyclic Redundancy Check.
 * Taken straight from Appendix C of RFC 4960 (SCTP), with the difference that
 * the remainder register (crc32) is initialized to 0xffffffffL rather than ~0L,
 * for correct operation on platforms where unsigned long is longer than 32
 * bits.
 */

/* Return the CRC-32C of the bytes buf[0..len-1] */
unsigned long nbase_crc32c(unsigned char *buf, int len)
{
  int i;
  unsigned long crc32 = 0xffffffffL;
  unsigned long result;
  unsigned char byte0, byte1, byte2, byte3;

  for (i = 0; i < len; i++) {
    CRC32C(crc32, buf[i]);
  }

  result = ~crc32;

  /*  result now holds the negated polynomial remainder;
   *  since the table and algorithm is "reflected" [williams95].
   *  That is, result has the same value as if we mapped the message
   *  to a polynomial, computed the host-bit-order polynomial
   *  remainder, performed final negation, then did an end-for-end
   *  bit-reversal.
   *  Note that a 32-bit bit-reversal is identical to four inplace
   *  8-bit reversals followed by an end-for-end byteswap.
   *  In other words, the bytes of each bit are in the right order,
   *  but the bytes have been byteswapped.  So we now do an explicit
   *  byteswap.  On a little-endian machine, this byteswap and
   *  the final ntohl cancel out and could be elided.
   */

  byte0 =  result        & 0xff;
  byte1 = (result >>  8) & 0xff;
  byte2 = (result >> 16) & 0xff;
  byte3 = (result >> 24) & 0xff;
  crc32 = ((byte0 << 24) | (byte1 << 16) | (byte2 <<  8) | byte3);
  return crc32;
}


/*
 * Adler32 Checksum Calculation.
 * Taken straight from RFC 2960 (SCTP).
 */

#define ADLER32_BASE 65521 /* largest prime smaller than 65536 */

/*
 * Update a running Adler-32 checksum with the bytes buf[0..len-1]
 * and return the updated checksum.  The Adler-32 checksum should
 * be initialized to 1.
 */
static unsigned long update_adler32(unsigned long adler,
                                    unsigned char *buf, int len)
{
  unsigned long s1 = adler & 0xffff;
  unsigned long s2 = (adler >> 16) & 0xffff;
  int n;

  for (n = 0; n < len; n++) {
    s1 = (s1 + buf[n]) % ADLER32_BASE;
    s2 = (s2 + s1)     % ADLER32_BASE;
  }
  return (s2 << 16) + s1;
}

/* Return the Adler32 of the bytes buf[0..len-1] */
unsigned long nbase_adler32(unsigned char *buf, int len)
{
  return update_adler32(1L, buf, len);
}

#undef ADLER32_BASE


/* This function returns a string containing the hexdump of the supplied
 * buffer. It uses current locale to determine if a character is printable or
 * not. It prints 73char+\n wide lines like these:

0000   e8 60 65 86 d7 86 6d 30  35 97 54 87 ff 67 05 9e  .`e...m05.T..g..
0010   07 5a 98 c0 ea ad 50 d2  62 4f 7b ff e1 34 f8 fc  .Z....P.bO{..4..
0020   c4 84 0a 6a 39 ad 3c 10  63 b2 22 c4 24 40 f4 b1  ...j9.<.c.".$@..

 * The lines look basically like Wireshark's hex dump.
 * WARNING: This function returns a pointer to a DYNAMICALLY allocated buffer
 * that the caller is supposed to free().
 * */
char *hexdump(const u8 *cp, u32 length){
  static char asciify[257];          /* Stores character table           */
  int asc_init=0;                    /* Flag to generate table only once */
  u32 i=0, hex=0, asc=0;             /* Array indexes                    */
  u32 line_count=0;                  /* For byte count at line start     */
  char *current_line=NULL;           /* Current line to write            */
  char *buffer=NULL;                 /* Dynamic buffer we return         */
  #define LINE_LEN 74                /* Length of printed line           */
  char line2print[LINE_LEN];         /* Stores current line              */
  char printbyte[16];                /* For byte conversion              */
  int bytes2alloc;                   /* For buffer                       */
  memset(line2print, ' ', LINE_LEN); /* We fill the line with spaces     */

  /* On the first run, generate a list of nice printable characters
   * (according to current locale) */
  if( asc_init==0){
      asc_init=1;
      for(i=0; i<256; i++){
        if( isalnum(i) || isdigit(i) || ispunct(i) ){ asciify[i]=i; }
        else{ asciify[i]='.'; }
      }
  }
  /* Allocate enough space to print the hex dump */
  bytes2alloc=(length%16==0)? (1 + LINE_LEN * (length/16)) : (1 + LINE_LEN * (1+(length/16))) ;
  buffer=(char *)safe_zalloc(bytes2alloc);
  current_line=buffer;
#define HEX_START 7
#define ASC_START 57
/* This is how or line looks like.
0000   00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f  .`e...m05.T..g..[\n]
01234567890123456789012345678901234567890123456789012345678901234567890123
0         1         2         3         4         5         6         7
       ^                                                 ^               ^
       |                                                 |               |
    HEX_START                                        ASC_START        Newline
*/
  i=0;
  while( i < length ){
    memset(line2print, ' ', LINE_LEN); /* Fill line with spaces */
    snprintf(line2print, sizeof(line2print), "%04x", (16*line_count++) % 0xFFFF); /* Add line No.*/
    line2print[4]=' '; /* Replace the '\0' inserted by snprintf() with a space */
    hex=HEX_START;  asc=ASC_START;
    do { /* Print 16 bytes in both hex and ascii */
        if (i%16 == 8) hex++; /* Insert space every 8 bytes */
        snprintf(printbyte, sizeof(printbyte), "%02x", cp[i]);/* First print the hex number */
        line2print[hex++]=printbyte[0];
        line2print[hex++]=printbyte[1];
        line2print[hex++]=' ';
        line2print[asc++]=asciify[ cp[i] ]; /* Then print its ASCII equivalent */
        i++;
    } while (i < length && i%16 != 0);
    /* Copy line to output buffer */
    line2print[LINE_LEN-1]='\n';
    memcpy(current_line, line2print, LINE_LEN);
    current_line += LINE_LEN;
  }
  buffer[bytes2alloc-1]='\0';
  return buffer;
} /* End of hexdump() */

/* This is like strtol or atoi, but it allows digits only. No whitespace, sign,
   or radix prefix. */
long parse_long(const char *s, char **tail)
{
    if (!isdigit((int) (unsigned char) *s)) {
        *tail = (char *) s;
        return 0;
    }

    return strtol(s, (char **) tail, 10);
}



/* This function takes a byte count and stores a short ascii equivalent
   in the supplied buffer. Eg: 0.122MB, 10.322Kb or 128B. */
char *format_bytecount(unsigned long long bytes, char *buf, size_t buflen) {
  assert(buf != NULL);

  if (bytes < 1000)
    Snprintf(buf, buflen, "%uB", (unsigned int) bytes);
  else if (bytes < 1000000)
    Snprintf(buf, buflen, "%.3fKB", bytes / 1000.0);
  else
    Snprintf(buf, buflen, "%.3fMB", bytes / 1000000.0);

  return buf;
}

/* Compare a canonical option name (e.g. "max-scan-delay") with a
   user-generated option such as "max_scan_delay" and returns 0 if the
   two values are considered equivalent (for example, - and _ are
   considered to be the same), nonzero otherwise. */
int optcmp(const char *a, const char *b) {
  while(*a && *b) {
    if (*a == '_' || *a == '-') {
      if (*b != '_' && *b != '-')
    return 1;
    }
    else if (*a != *b)
      return 1;
    a++; b++;
  }
  if (*a || *b)
    return 1;
  return 0;
}

/* Returns one if the file pathname given exists, is not a directory and
 * is readable by the executing process.  Returns two if it is readable
 * and is a directory.  Otherwise returns 0. */
int file_is_readable(const char *pathname) {
    char *pathname_buf = strdup(pathname);
    int status = 0;
    struct stat st;

#ifdef WIN32
    // stat on windows only works for "dir_name" not for "dir_name/" or "dir_name\\"
    int pathname_len = strlen(pathname_buf);
    char last_char = pathname_buf[pathname_len - 1];

    if(    last_char == '/'
        || last_char == '\\')
        pathname_buf[pathname_len - 1] = '\0';

#endif

  if (stat(pathname_buf, &st) == -1)
    status = 0;
  else if (access(pathname_buf, R_OK) != -1)
    status = S_ISDIR(st.st_mode) ? 2 : 1;

  free(pathname_buf);
  return status;
}

#if HAVE_PROC_SELF_EXE
static char *executable_path_proc_self_exe(void) {
  char buf[1024];
  char *path;
  int n;

  n = readlink("/proc/self/exe", buf, sizeof(buf));
  if (n < 0 || n >= sizeof(buf))
    return NULL;
  path = (char *) safe_malloc(n + 1);
  /* readlink does not null-terminate. */
  memcpy(path, buf, n);
  path[n] = '\0';

  return path;
}
#endif

#if HAVE_MACH_O_DYLD_H
#include <mach-o/dyld.h>
/* See the dyld(3) man page on OS X. */
static char *executable_path_NSGetExecutablePath(void) {
  char buf[1024];
  uint32_t size;

  size = sizeof(buf);
  if (_NSGetExecutablePath(buf, &size) == 0)
    return strdup(buf);
  else
    return NULL;
}
#endif

#if WIN32
static char *executable_path_GetModuleFileName(void) {
  char buf[1024];
  int n;

  n = GetModuleFileName(GetModuleHandle(0), buf, sizeof(buf));
  if (n <= 0 || n >= sizeof(buf))
    return NULL;

  return strdup(buf);
}
#endif

static char *executable_path_argv0(const char *argv0) {
  if (argv0 == NULL)
    return NULL;
  /* We can get the path from argv[0] if it contains a directory separator.
     (Otherwise it was looked up in $PATH). */
  if (strchr(argv0, '/') != NULL)
    return strdup(argv0);
#if WIN32
  if (strchr(argv0, '\\') != NULL)
    return strdup(argv0);
#endif
  return NULL;
}

char *executable_path(const char *argv0) {
  char *path;

  path = NULL;
#if HAVE_PROC_SELF_EXE
  if (path == NULL)
    path = executable_path_proc_self_exe();
#endif
#if HAVE_MACH_O_DYLD_H
  if (path == NULL)
    path = executable_path_NSGetExecutablePath();
#endif
#if WIN32
  if (path == NULL)
    path = executable_path_GetModuleFileName();
#endif
  if (path == NULL)
    path = executable_path_argv0(argv0);

  return path;
}
