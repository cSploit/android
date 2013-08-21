/*
 *  $Id: libnet-macros.h,v 1.7 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet-macros.h - Network routine library macro header file
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

#ifndef __LIBNET_MACROS_H
#define __LIBNET_MACROS_H
/**
 * @file libnet-macros.h
 * @brief libnet macros and symbolic constants
 */

/* for systems without snprintf */
#if defined(NO_SNPRINTF)
#define snprintf(buf, len, args...) sprintf(buf, ##args)
#endif


/**
 * Used for libnet's name resolution functions, specifies that no DNS lookups
 * should be performed and the IP address should be kept in numeric form.
 */
#define LIBNET_DONT_RESOLVE 0

/**
 * Used for libnet's name resolution functions, specifies that a DNS lookup
 * can be performed if needed to resolve the IP address to a canonical form.
 */
#define LIBNET_RESOLVE      1

/**
 * Used several places, to specify "on" or "one"
 */
#define LIBNET_ON	0

/**
 * Used several places, to specify "on" or "one"
 */
#define LIBNET_OFF	1

/**
 * IPv6 error code
 */
#ifndef IN6ADDR_ERROR_INIT
#define IN6ADDR_ERROR_INIT { { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
                                 0xff, 0xff } } }
#endif

/**
 * Used for libnet_get_prand() to specify function disposition
 */ 
#define LIBNET_PR2          0
#define LIBNET_PR8          1
#define LIBNET_PR16         2
#define LIBNET_PRu16        3
#define LIBNET_PR32         4
#define LIBNET_PRu32        5
#define LIBNET_PRAND_MAX    0xffffffff

/**
 * The biggest an IP packet can be -- 65,535 bytes.
 */
#define LIBNET_MAX_PACKET   0xffff
#ifndef IP_MAXPACKET
#define IP_MAXPACKET        0xffff
#endif


/* ethernet addresses are 6 octets long */
#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN      0x6
#endif

/* FDDI addresses are 6 octets long */
#ifndef FDDI_ADDR_LEN
#define FDDI_ADDR_LEN       0x6
#endif

/* token ring addresses are 6 octets long */
#ifndef TOKEN_RING_ADDR_LEN
#define TOKEN_RING_ADDR_LEN 0x6
#endif

/* LLC Organization Code is 3 bytes long */
#define LIBNET_ORG_CODE_SIZE  0x3

/**
 * The libnet error buffer is 256 bytes long.
 */ 
#define LIBNET_ERRBUF_SIZE      0x100

/**
 * IP and TCP options can be up to 40 bytes long.
 */
#define LIBNET_MAXOPTION_SIZE   0x28

/* some BSD variants have this endianess problem */
#if (LIBNET_BSD_BYTE_SWAP)
#define FIX(n)      ntohs(n)
#define UNFIX(n)    htons(n)
#else
#define FIX(n)      (n)
#define UNFIX(n)    (n)
#endif

/* used internally for packet builders */
#define LIBNET_DO_PAYLOAD(l, p)                                              \
if (payload_s && !payload)                                                   \
{                                                                            \
    snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,                                 \
            "%s(): payload inconsistency\n", __func__);                      \
    goto bad;                                                                \
}                                                                            \
if (payload_s)                                                               \
{                                                                            \
    n = libnet_pblock_append(l, p, payload, payload_s);                      \
    if (n == (uint32_t) - 1)                                                 \
    {                                                                        \
        goto bad;                                                            \
    }                                                                        \
}                                                                            \


/* used internally for checksum stuff */
#define LIBNET_CKSUM_CARRY(x) \
    (x = (x >> 16) + (x & 0xffff), (~(x + (x >> 16)) & 0xffff))

/* used interally for OSPF stuff */
#define LIBNET_OSPF_AUTHCPY(x, y) \
    memcpy((uint8_t *)x, (uint8_t *)y, sizeof(y))
#define LIBNET_OSPF_CKSUMBUF(x, y) \
    memcpy((uint8_t *)x, (uint8_t *)y, sizeof(y))  

/* used internally for NTP leap indicator, version, and mode */
#define LIBNET_NTP_DO_LI_VN_MODE(li, vn, md) \
    ((uint8_t)((((li) << 6) & 0xc0) | (((vn) << 3) & 0x38) | ((md) & 0x7)))

/* Not all systems have IFF_LOOPBACK */
#ifdef IFF_LOOPBACK
#define LIBNET_ISLOOPBACK(p) ((p)->ifr_flags & IFF_LOOPBACK)
#else
#define LIBNET_ISLOOPBACK(p) (strcmp((p)->ifr_name, "lo0") == 0)
#endif

/* advanced mode check */
#define LIBNET_ISADVMODE(x) (x & 0x08)

/* context queue macros and constants */
#define LIBNET_LABEL_SIZE   64
#define LIBNET_LABEL_DEFAULT "cardshark"
#define CQ_LOCK_UNLOCKED    (u_int)0x00000000
#define CQ_LOCK_READ        (u_int)0x00000001
#define CQ_LOCK_WRITE       (u_int)0x00000002

/**
 * Provides an interface to iterate through the context queue of libnet
 * contexts. Before calling this macro, be sure to set the queue using
 * libnet_cq_head().
 */
#define for_each_context_in_cq(l) \
    for (l = libnet_cq_head(); libnet_cq_last(); l = libnet_cq_next())

/* return 1 if write lock is set on cq */
#define cq_is_wlocked() (l_cqd.cq_lock & CQ_LOCK_WRITE)

/* return 1 if read lock is set on cq */
#define cq_is_rlocked() (l_cqd.cq_lock & CQ_LOCK_READ)

/* return 1 if any lock is set on cq */
#define cq_is_locked() (l_cqd.cq_lock & (CQ_LOCK_READ | CQ_LOCK_WRITE))

/* check if a context queue is locked */
#define check_cq_lock(x) (l_cqd.cq_lock & x)

#endif  /* __LIBNET_MACROS_H */

/* EOF */
