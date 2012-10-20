/*
 *  $Id: libnet.h.in,v 1.5 2004/01/17 07:51:19 mike Exp $
 *
 *  libnet.h - Network routine library header file
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __LIBNET_H
#define __LIBNET_H
/**
 * @file libnet.h
 * @brief toplevel libnet header file
 */

/** 
 * @mainpage Libnet Packet Assembly Library
 *
 * @section intro Overview
 *
 * Libnet is a high-level API (toolkit) allowing the application programmer to 
 * construct and inject network packets. It provides a portable and simplified 
 * interface for low-level network packet shaping, handling and injection. 
 * Libnet hides much of the tedium of packet creation from the application 
 * programmer such as multiplexing, buffer management, arcane packet header 
 * information, byte-ordering, OS-dependent issues, and much more. Libnet 
 * features portable packet creation interfaces at the IP layer and link layer,
 * as well as a host of supplementary and complementary functionality. Using 
 * libnet, quick and simple packet assembly applications can be whipped up with
 * little effort. With a bit more time, more complex programs can be written 
 * (Traceroute and ping were easily rewritten using libnet and 
 * <a href="www.tcpdump.org">libpcap</a>).
 */ 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>

#if !defined(__WIN32__)
# include <sys/ioctl.h>
#endif /* __WIN32__ */

#if defined(HAVE_SYS_SOCKIO_H) && !defined(SIOCGIFADDR)
# include <sys/sockio.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#if !defined(__WIN32__)
# include <sys/socket.h>
# include <netinet/in.h>
# include <net/if.h>
#else /* __WIN32__ */
# if (__CYGWIN__)
#  include <sys/socket.h>
# endif
# include <ws2tcpip.h>
# include <windows.h>
# include <winsock2.h>
# include <win32/in_systm.h>
#endif /* __WIN32__ */

#if !(__linux__) && !(__WIN32__) && !(__APPLE__) && !(__CYGWIN__) && !(__GNU__)
#else   /* __linux__ */
# if (HAVE_NET_ETHERNET_H)
#  include <net/ethernet.h>
# endif  /* HAVE_NET_ETHERNET_H */
#endif  /* __linux__ */

#if !defined(__WIN32__)
# include <arpa/inet.h>
# include <sys/time.h>
# include <netdb.h>
#endif /* __WIN32__ */

#include <errno.h>
#include <stdarg.h>

#define LIBNET_VERSION  "1.1.5"

#define LIBNET_LIL_ENDIAN 1

#include "./libnet/libnet-types.h"
#include "./libnet/libnet-macros.h"
#include "./libnet/libnet-headers.h"
#include "./libnet/libnet-structures.h"
#include "./libnet/libnet-asn1.h"
#include "./libnet/libnet-functions.h"

#ifdef __cplusplus
}
#endif

#endif  /* __LIBNET_H */

/* EOF */
