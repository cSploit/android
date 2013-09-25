/*
 *  $Id: libnet_test.h,v 1.3 2004/01/29 21:17:16 mike Exp $
 *
 *  libnet_test.h
 *
 *  Copyright (c) 1998 - 2001 Mike D. Schiffman <mike@infonexus.com>
 */

#ifndef __LIBNET_TEST_H
#define __LIBNET_TEST_H

#ifndef _WIN32
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#include "../include/win32/getopt.h"
#endif

#define libnet_timersub(tvp, uvp, vvp)                                  \
        do {                                                            \
                (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;          \
                (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;       \
                if ((vvp)->tv_usec < 0) {                               \
                        (vvp)->tv_sec--;                                \
                        (vvp)->tv_usec += 1000000;                      \
                }                                                       \
        } while (0)

u_char enet_src[6] = {0x0d, 0x0e, 0x0a, 0x0d, 0x00, 0x00};
u_char enet_dst[6] = {0x00, 0x10, 0x67, 0x00, 0xb1, 0x86};
u_char ip_src[4]   = {0x0a, 0x00, 0x00, 0x01};
u_char ip_dst[4]   = {0x0a, 0x00, 0x00, 0x02};
u_char fddi_src[6] = {0x00, 0x0d, 0x0e, 0x0a, 0x0d, 0x00};
u_char fddi_dst[6] = {0x00, 0x10, 0x67, 0x00, 0xb1, 0x86};
u_char tr_src[6]   = {0x00, 0x0d, 0x0e, 0x0a, 0x0d, 0x00};
u_char tr_dst[6]   = {0x00, 0x10, 0x67, 0x00, 0xb1, 0x86};

u_char org_code[3] = {0x00, 0x00, 0x00};

void usage(char *);

#if defined(__WIN32__)
#include <win32/getopt.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef _WIN32
#include <sys/time.h>
#endif
#if defined(__GNUC__)         /* mingw compiler */
extern __attribute__((dllimport)) char *optarg;
#else   /* assume msvc */
#ifndef _WIN32
extern __dllspec(dllimport) char *optarg;
#endif
#endif
#endif  /* __WIN32__ */

#endif  /* __LIBNET_TEST_H */

/* EOF */
