/*
 *  $Id: libnet_raw.c,v 1.9 2004/02/18 18:19:00 mike Exp $
 *
 *  libnet
 *  libnet_raw.c - raw sockets routines
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

#if (HAVE_CONFIG_H)
#include "../include/config.h"
#endif
#if (!(_WIN32) || (__CYGWIN__)) 
#include "../include/libnet.h"
#else
#include "../include/win32/libnet.h"
#endif

#if defined (__WIN32__)
int
libnet_open_raw4(libnet_t *l)
{
    return (libnet_open_link(l));
}

int
libnet_open_raw6(libnet_t *l)
{
    return (libnet_open_link(l));
}

int
libnet_close_raw4(libnet_t *l)
{
    return (libnet_close_link_interface(l));
}

int
libnet_close_raw6(libnet_t *l)
{
    return (libnet_close_link_interface(l));
}
#else
int
libnet_open_raw4(libnet_t *l)
{
    int len; /* now supposed to be socklen_t, but maybe old systems used int? */

#if !(__WIN32__)
     int n = 1;
#if (__svr4__)
     void *nptr = &n;
#else
    int *nptr = &n;
#endif  /* __svr4__ */
#else 
	BOOL n;
#endif

    if (l == NULL)
    { 
        return (-1);
    } 

    l->fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (l->fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "%s(): SOCK_RAW allocation failed: %s\n",
		 __func__, strerror(errno));
        goto bad;
    }

#ifdef IP_HDRINCL
/* 
 * man raw
 *
 * The IPv4 layer generates an IP header when sending a packet unless
 * the IP_HDRINCL socket option is enabled on the socket.  When it
 * is enabled, the packet must contain an IP header.  For
 * receiving the IP header is always included in the packet.
 */
#if !(__WIN32__)
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, nptr, sizeof(n)) == -1)
#else
    n = TRUE;
    if (setsockopt(l->fd, IPPROTO_IP, IP_HDRINCL, &n, sizeof(n)) == -1)
#endif

    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "%s(): set IP_HDRINCL failed: %s\n",
                __func__, strerror(errno));
        goto bad;
    }
#endif /*  IP_HDRINCL  */

#ifdef SO_SNDBUF

/*
 * man 7 socket 
 *
 * Sets and  gets  the  maximum  socket  send buffer in bytes. 
 *
 * Taken from libdnet by Dug Song
 */
    len = sizeof(n);
    if (getsockopt(l->fd, SOL_SOCKET, SO_SNDBUF, &n, &len) < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
		 "%s(): get SO_SNDBUF failed: %s\n",
		 __func__, strerror(errno));
        goto bad;
    }
    
    for (n += 128; n < 1048576; n += 128)
    {
        if (setsockopt(l->fd, SOL_SOCKET, SO_SNDBUF, &n, len) < 0) 
        {
            if (errno == ENOBUFS)
            {
                break;
            }
             snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                     "%s(): set SO_SNDBUF failed: %s\n",
                     __func__, strerror(errno));
             goto bad;
        }
    }
#endif

#ifdef SO_BROADCAST
/*
 * man 7 socket
 *
 * Set or get the broadcast flag. When  enabled,  datagram  sockets
 * receive packets sent to a broadcast address and they are allowed
 * to send packets to a broadcast  address.   This  option  has  no
 * effect on stream-oriented sockets.
 */
    if (setsockopt(l->fd, SOL_SOCKET, SO_BROADCAST, nptr, sizeof(n)) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): set SO_BROADCAST failed: %s\n",
                __func__, strerror(errno));
        goto bad;
    }
#endif  /*  SO_BROADCAST  */
    return (l->fd);

bad:
    return (-1);    
}


int
libnet_close_raw4(libnet_t *l)
{
    if (l == NULL)
    { 
        return (-1);
    }

    return (close(l->fd));
}

#if ((defined HAVE_SOLARIS && !defined HAVE_SOLARIS_IPV6) || defined (__WIN32__))
int libnet_open_raw6(libnet_t *l)
{
	return (-1);
}

#else
int
libnet_open_raw6(libnet_t *l)
{
#if !(__WIN32__)
#if (__svr4__)
     int one = 1;
     void *oneptr = &one;
#else
#if (__linux__)
    int one = 1;
    int *oneptr = &one;
#endif
#endif  /* __svr4__ */
#else
    BOOL one;
#endif

/* Solaris IPv6 stuff */
    
    if (l == NULL)
    { 
        return (-1);
    } 

    l->fd = socket(PF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (l->fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "%s(): SOCK_RAW allocation failed: %s\n", __func__,
                strerror(errno));
        goto bad;
    }

#if (__linux__)
    if (setsockopt(l->fd, SOL_SOCKET, SO_BROADCAST, oneptr, sizeof(one)) == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): set SO_BROADCAST failed: %s\n", __func__,
                strerror(errno));
        goto bad;
    }
#endif  /* __linux__ */
    return (l->fd);

bad:
    return (-1);    
}
#endif

int
libnet_close_raw6(libnet_t *l)
{
    if (l == NULL)
    { 
         return (-1);
    }
    return (close(l->fd));
}
#endif
/* EOF */
