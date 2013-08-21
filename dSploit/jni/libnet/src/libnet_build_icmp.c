/*
 *  $Id: libnet_build_icmp.c,v 1.17 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_icmp.c - ICMP packet assemblers
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

/* some common cruft for completing ICMP error packets */
#define LIBNET_BUILD_ICMP_ERR_FINISH(len)                                    \
do                                                                           \
{                                                                            \
    n = libnet_pblock_append(l, p, (uint8_t *)&icmp_hdr, len);              \
    if (n == -1)                                                             \
    {                                                                        \
        goto bad;                                                            \
    }                                                                        \
                                                                             \
    if (payload_s && !payload)                                               \
    {                                                                        \
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,                             \
                "%s(): payload inconsistency\n", __func__);                  \
        goto bad;                                                            \
    }                                                                        \
                                                                             \
    if (payload_s)                                                           \
    {                                                                        \
        n = libnet_pblock_append(l, p, payload, payload_s);                  \
        if (n == -1)                                                         \
        {                                                                    \
            goto bad;                                                        \
        }                                                                    \
    }                                                                        \
                                                                             \
    if (sum == 0)                                                            \
    {                                                                        \
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);                \
    }                                                                        \
} while (0)

libnet_ptag_t
libnet_build_icmpv4_echo(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, const uint8_t *payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag) 
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_ECHO_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_ECHO_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_ECHO_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.icmp_id   = htons(id);            /* packet id */
    icmp_hdr.icmp_seq  = htons(seq);           /* packet seq */

    n = libnet_pblock_append(l, p, (uint8_t *)&icmp_hdr, LIBNET_ICMPV4_ECHO_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_ICMPV4_ECHO_H));
bad:
    libnet_pblock_delete(l, p);   
    return (-1);
}

libnet_ptag_t
libnet_build_icmpv4_mask(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, uint32_t mask, const uint8_t *payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_MASK_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_MASK_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_MASK_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.icmp_id   = htons(id);     /* packet id */
    icmp_hdr.icmp_seq  = htons(seq);    /* packet seq */
    icmp_hdr.icmp_mask = htonl(mask);   /* address mask */

    n = libnet_pblock_append(l, p, (uint8_t *)&icmp_hdr, LIBNET_ICMPV4_MASK_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_ICMPV4_MASK_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_icmpv4_timestamp(uint8_t type, uint8_t code, uint16_t sum,
uint16_t id, uint16_t seq, uint32_t otime, uint32_t rtime, uint32_t ttime,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_TS_H + payload_s;        /* size of memory block */
    h = LIBNET_ICMPV4_TS_H + payload_s;        /* hl for checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_TS_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type  = type;             /* packet type */
    icmp_hdr.icmp_code  = code;             /* packet code */
    icmp_hdr.icmp_sum   = (sum ? htons(sum) : 0); /* checksum */
    icmp_hdr.icmp_id    = htons(id);        /* packet id */
    icmp_hdr.icmp_seq   = htons(seq);       /* packet seq */
    icmp_hdr.icmp_otime = htonl(otime);     /* original timestamp */
    icmp_hdr.icmp_rtime = htonl(rtime);     /* receive timestamp */
    icmp_hdr.icmp_ttime = htonl(ttime);     /* transmit timestamp */

    n = libnet_pblock_append(l, p, (uint8_t *)&icmp_hdr, LIBNET_ICMPV4_TS_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    if (sum == 0)
    {
        /*
         *  If checksum is zero, by default libnet will compute a checksum
         *  for the user.  The programmer can override this by calling
         *  libnet_toggle_checksum(l, ptag, 1);
         */
        libnet_pblock_setflags(p, LIBNET_PBLOCK_DO_CHECKSUM);
    }
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_ICMPV4_TS_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_icmpv4_unreach(uint8_t type, uint8_t code, uint16_t sum,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 
    n = LIBNET_ICMPV4_UNREACH_H + payload_s;        /* size of memory block */

    /* 
     * FREDRAYNAL: as ICMP checksum includes what is embedded in 
     * the payload, and what is after the ICMP header, we need to include
     * those 2 sizes.
     */
    h = LIBNET_ICMPV4_UNREACH_H + payload_s + l->total_size; 

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_UNREACH_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.icmp_id   = 0;             /* must be 0 */
    icmp_hdr.icmp_seq  = 0;             /* must be 0 */

    LIBNET_BUILD_ICMP_ERR_FINISH(LIBNET_ICMPV4_UNREACH_H);

    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_UNREACH_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_icmpv4_timeexceed(uint8_t type, uint8_t code, uint16_t sum,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* size of memory block */
    n = LIBNET_ICMPV4_TIMXCEED_H;
    /* 
     * FREDRAYNAL: as ICMP checksum includes what is embedded in 
     * the payload, and what is after the ICMP header, we need to include
     * those 2 sizes.
     */
    h = LIBNET_ICMPV4_TIMXCEED_H + payload_s + l->total_size; 

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_TIMXCEED_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.icmp_id   = 0;             /* must be 0 */
    icmp_hdr.icmp_seq  = 0;             /* must be 0 */

    LIBNET_BUILD_ICMP_ERR_FINISH(LIBNET_ICMPV4_TIMXCEED_H);

    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_TIMXCEED_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_icmpv4_redirect(uint8_t type, uint8_t code, uint16_t sum,
uint32_t gateway, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)

{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv4_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV4_REDIRECT_H;               /* size of memory block */
    /* 
     * FREDRAYNAL: as ICMP checksum includes what is embedded in 
     * the payload, and what is after the ICMP header, we need to include
     * those 2 sizes.
     */
    h = LIBNET_ICMPV4_REDIRECT_H + payload_s + l->total_size; 

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV4_REDIRECT_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type      = type;             /* packet type */
    icmp_hdr.icmp_code      = code;             /* packet code */
    icmp_hdr.icmp_sum       = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.hun.gateway    = gateway;

    LIBNET_BUILD_ICMP_ERR_FINISH(LIBNET_ICMPV4_REDIRECT_H);

    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV4_REDIRECT_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_icmpv6_unreach(u_int8_t type, u_int8_t code, u_int16_t sum,
u_int8_t *payload, u_int32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    u_int32_t n, h;
    libnet_pblock_t *p;
    struct libnet_icmpv6_hdr icmp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_ICMPV6_UNREACH_H + payload_s;        /* size of memory block */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ICMPV6_UNREACH_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&icmp_hdr, 0, sizeof(icmp_hdr));
    icmp_hdr.icmp_type = type;          /* packet type */
    icmp_hdr.icmp_code = code;          /* packet code */
    icmp_hdr.icmp_sum  = (sum ? htons(sum) : 0);  /* checksum */
    icmp_hdr.id   = 0;             /* must be 0 */
    icmp_hdr.seq  = 0;             /* must be 0 */

    LIBNET_BUILD_ICMP_ERR_FINISH(LIBNET_ICMPV6_UNREACH_H);

    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_ICMPV6_UNREACH_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
