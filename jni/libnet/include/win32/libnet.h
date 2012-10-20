/*
 *  $Id: libnet.h,v 1.7 2004/01/03 20:31:00 mike Exp $
 *
 *  libnet.h - Network routine library header file for Win32 VC++
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

#ifdef _WIN32

#ifndef __LIBNET_H
#define __LIBNET_H

#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "in_systm.h"
#include "pcap.h"


#ifdef __cplusplus
extern "C" {
#endif

/* __WIN32__ is NOT a predefined MACRO, use _WIN32
 * __CYGWIN__ is defined within the cygwin environment.
 */ 
#ifndef __WIN32__
#define __WIN32__ _WIN32
#endif

#define LIBNET_LIL_ENDIAN 1
#define HAVE_CONFIG_H 1

/* Some UNIX to Win32 conversions */
#define STDOUT_FILENO stdout
#define snprintf _snprintf 
#define write _write
#define open _open
#define random rand
#define close closesocket
#define __func__ __FUNCTION__

/* __FUNCTION__ available in VC ++ 7.0 (.NET) and greater */
#if _MSC_VER < 1300
#define __FUNCTION__ __FILE__
#endif

#pragma comment (lib,"ws2_32")    /* Winsock 2 */
#pragma comment (lib,"iphlpapi")  /* IP Helper */
#pragma comment (lib,"wpcap")     /* Winpcap   */
#pragma comment (lib,"packet")   

/* "@LIBNET_VERSION@" will not work in VC++, so version.h doesn't get populated */
#define VERSION  "1.1.1"

/* To use Win32 native versions */
#define WPCAP 1
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include "../libnet/libnet-macros.h"
#include "../libnet/libnet-headers.h"
#include "../libnet/libnet-structures.h"
#include "../libnet/libnet-asn1.h"
#include "../libnet/libnet-functions.h"

#ifdef __cplusplus
}
#endif

#endif  /* __LIBNET_H */

#endif
/* EOF */
