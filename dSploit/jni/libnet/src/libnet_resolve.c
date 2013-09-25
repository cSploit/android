/*
 *  $Id: libnet_resolve.c,v 1.21 2004/11/09 07:05:07 mike Exp $
 *
 *  libnet
 *  libnet_resolve.c - various name resolution type routines
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

#ifndef HAVE_GETHOSTBYNAME2
struct hostent *
gethostbyname2(const char *name, int af)
{
        return gethostbyname(name);
}
#endif

char *
libnet_addr2name4(uint32_t in, uint8_t use_name)
{
	#define HOSTNAME_SIZE 512
    static char hostname[HOSTNAME_SIZE+1], hostname2[HOSTNAME_SIZE+1];
    static uint16_t which;
    uint8_t *p;

    struct hostent *host_ent = NULL;
    struct in_addr addr;

    /*
     *  Swap to the other buffer.  We swap static buffers to avoid having to
     *  pass in a int8_t *.  This makes the code that calls this function more
     *  intuitive, but makes this function ugly.  This function is seriously
     *  non-reentrant.  For threaded applications (or for signal handler code)
     *  use host_lookup_r().
     */
    which++;
    
    if (use_name == LIBNET_RESOLVE)
    {
		addr.s_addr = in;
        host_ent = gethostbyaddr((int8_t *)&addr, sizeof(struct in_addr), AF_INET);
        /* if this fails, we silently ignore the error and move to plan b! */
    }
    if (!host_ent)
    {

        p = (uint8_t *)&in;
   		snprintf(((which % 2) ? hostname : hostname2), HOSTNAME_SIZE,
                 "%d.%d.%d.%d",
                 (p[0] & 255), (p[1] & 255), (p[2] & 255), (p[3] & 255));
    }
    else if (use_name == LIBNET_RESOLVE)
    {
		char *ptr = ((which % 2) ? hostname : hostname2);
		strncpy(ptr, host_ent->h_name, HOSTNAME_SIZE);
		ptr[HOSTNAME_SIZE] = '\0';
	
    }
    return (which % 2) ? (hostname) : (hostname2);
}

void
libnet_addr2name4_r(uint32_t in, uint8_t use_name, char *hostname,
        int hostname_len)
{
    uint8_t *p;
    struct hostent *host_ent = NULL;
    struct in_addr addr;

    if (use_name == LIBNET_RESOLVE)
    {   
		addr.s_addr = in;
        host_ent = gethostbyaddr((int8_t *)&addr, sizeof(struct in_addr),
                AF_INET);
    }
    if (!host_ent)
    {
        p = (uint8_t *)&in;
        snprintf(hostname, hostname_len, "%d.%d.%d.%d",
                (p[0] & 255), (p[1] & 255), (p[2] & 255), (p[3] & 255));
    }
    else
    {
        strncpy(hostname, host_ent->h_name, hostname_len - 1);
		hostname[sizeof(hostname) - 1] = '\0';
    }
}

uint32_t
libnet_name2addr4(libnet_t *l, char *host_name, uint8_t use_name)
{
    struct in_addr addr;
    struct hostent *host_ent; 
    uint32_t m;
    uint val;
    int i;

    if (use_name == LIBNET_RESOLVE)
    {
		if ((addr.s_addr = inet_addr(host_name)) == -1)
        {
            if (!(host_ent = gethostbyname(host_name)))
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                        "%s(): %s\n", __func__, hstrerror(h_errno));
                /* XXX - this is actually 255.255.255.255 */
                return (-1);
            }
            memcpy(&addr.s_addr, host_ent->h_addr, host_ent->h_length);
        }
        /* network byte order */
        return (addr.s_addr);
    }
    else
    {
        /*
         *  We only want dots 'n decimals.
         */
        if (!isdigit(host_name[0]))
        {
            if (l)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): expecting dots and decimals\n", __func__);
            }
            /* XXX - this is actually 255.255.255.255 */
            return (-1);
        }

        m = 0;
        for (i = 0; i < 4; i++)
        {
            m <<= 8;
            if (*host_name)
            {
                val = 0;
                while (*host_name && *host_name != '.')
                {   
                    val *= 10;
                    val += *host_name - '0';
                    if (val > 255)
                    {
                        if (l)
                        {
                            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                            "%s(): value greater than 255\n", __func__);
                        }
                        /* XXX - this is actually 255.255.255.255 */
                        return (-1);
                    }
                    host_name++;
                }
                m |= val;
                if (*host_name)
                {
                    host_name++;
                }
            }
        }
        /* host byte order */
       return (ntohl(m));
    }
}

void
libnet_addr2name6_r(struct libnet_in6_addr addr, uint8_t use_name,
            char *host_name, int host_name_len)
{
    struct hostent *host_ent = NULL;

    if (use_name == LIBNET_RESOLVE)
    {    
#ifdef HAVE_SOLARIS 
#ifdef HAVE_SOLARIS_IPV6
        host_ent = getipnodebyaddr((int8_t *)&addr, sizeof(struct in_addr),
                AF_INET6, NULL);
#else
        /* XXX - Gah!  Can't report error! */
        host_ent = NULL;
#endif
#else
        host_ent = gethostbyaddr((int8_t *)&addr, sizeof(struct in_addr),
                AF_INET6);
#endif
    }
    if (!host_ent)
    {
#if !defined(__WIN32__) /* Silence Win32 warning */
        inet_ntop(AF_INET6, &addr, host_name, host_name_len);
#endif
    }
    else
    {
        strncpy(host_name, host_ent->h_name, host_name_len -1);
		host_name[sizeof(host_name) - 1] = '\0';
    }
}

const struct libnet_in6_addr in6addr_error = IN6ADDR_ERROR_INIT;

struct libnet_in6_addr
libnet_name2addr6(libnet_t *l, char *host_name, uint8_t use_name)
{
#if !defined (__WIN32__)
    struct libnet_in6_addr addr;
    struct hostent *host_ent; 
#endif
   
    if (use_name == LIBNET_RESOLVE)
    {
#ifdef __WIN32__
        /* XXX - we don't support this yet */
        if (l)
        {        
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't resolve IPv6 addresses\n", __func__);
        }
        return (in6addr_error);
#else
#ifdef HAVE_SOLARIS 
#ifdef HAVE_SOLARIS_IPV6
        if (!(host_ent = getipnodebyname((int8_t *)&addr,
                sizeof(struct in_addr), AF_INET6, NULL)))
#else
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE, 
                "%s(): %s\n", __func__, strerror(errno));
        return (in6addr_error);
#endif
#else
        if (!(host_ent = gethostbyname2(host_name, AF_INET6)))
#endif
        {
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): %s", __func__, strerror(errno));
            return (in6addr_error);
        }
        memcpy(&addr, host_ent->h_addr, host_ent->h_length);
        return (addr);
#endif  /* !__WIN32__ */
    }
    else
    {
		#if defined(__WIN32__) /* Silence Win32 warning */
		if (l)
        {        
               snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): can't resolve IPv6 addresses.\n", __func__);
        }
        return (in6addr_error);
        #else
        if(!inet_pton(AF_INET6, host_name, &addr))
        {
            if (l)
            {
                snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): invalid IPv6 address\n", __func__);
            }
            return (in6addr_error);
        }
        return (addr);
        #endif
    }
}

struct libnet_in6_addr
libnet_get_ipaddr6(libnet_t *l)
{
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
           "%s(): not yet Implemented\n", __func__);
    return (in6addr_error);
}

#if !defined(__WIN32__)
uint32_t
libnet_get_ipaddr4(libnet_t *l)
{
    struct ifreq ifr;
    register struct sockaddr_in *sin;
    int fd;

    if (l == NULL)
    {
        return (-1);
    }

    /* create dummy socket to perform an ioctl upon */
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): socket(): %s\n", __func__, strerror(errno));
        return (-1);
    }

    sin = (struct sockaddr_in *)&ifr.ifr_addr;

    if (l->device == NULL)
    {
        if (libnet_select_device(l) == -1)
        {
            /* error msg set in libnet_select_device() */
            close(fd);
            return (-1);
        }
    }
    strncpy(ifr.ifr_name, l->device, sizeof(ifr.ifr_name) -1);
	ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';
	
    ifr.ifr_addr.sa_family = AF_INET;

    if (ioctl(fd, SIOCGIFADDR, (int8_t*) &ifr) < 0)
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): ioctl(): %s\n", __func__, strerror(errno));
        close(fd);
        return (-1);
    }
    close(fd);
    return (sin->sin_addr.s_addr);
}
#else
#include <Packet32.h>
uint32_t
libnet_get_ipaddr4(libnet_t *l)
{
    long npflen = 0;
    struct sockaddr_in sin;
    struct npf_if_addr ipbuff;

    memset(&sin,0,sizeof(sin));
    memset(&ipbuff,0,sizeof(ipbuff));

    npflen = sizeof(ipbuff);
    if (PacketGetNetInfoEx(l->device, &ipbuff, &npflen))
    {
        sin = *(struct sockaddr_in *)&ipbuff.IPAddress;
    }
    return (sin.sin_addr.s_addr);
}
#endif /* WIN32 */

uint8_t *
libnet_hex_aton(const char *s, int *len)
{
    uint8_t *buf;
    int i;
    int32_t l;
    char *pp;
        
    while (isspace(*s))
    {
        s++;
    }
    for (i = 0, *len = 0; s[i]; i++)
    {
        if (s[i] == ':')
        {
            (*len)++;
        }
    }
    buf = malloc(*len + 1);
    if (buf == NULL)
    {
        return (NULL);
    }
    /* expect len hex octets separated by ':' */
    for (i = 0; i < *len + 1; i++)
    {
        l = strtol(s, &pp, 16);
        if (pp == s || l > 0xff || l < 0)
        {
            *len = 0;
            free(buf);
            return (NULL);
        }
        if (!(*pp == ':' || (i == *len && (isspace(*pp) || *pp == '\0'))))
        {
            *len = 0;
            free(buf);
            return (NULL);
        }
        buf[i] = (uint8_t)l;
        s = pp + 1;
    }
    /* return int8_tacter after the octets ala strtol(3) */
    (*len)++;
    return (buf);
}

/* EOF */
