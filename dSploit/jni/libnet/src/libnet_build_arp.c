/*
 *  $Id: libnet_build_arp.c,v 1.13 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_arp.c - ARP packet assembler
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


libnet_ptag_t
libnet_build_arp(uint16_t hrd, uint16_t pro, uint8_t hln, uint8_t pln,
uint16_t op, const uint8_t *sha, const uint8_t *spa, const uint8_t *tha, const uint8_t *tpa,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_arp_hdr arp_hdr;

    if (l == NULL)
    { 
        return (-1); 
    }

    n = LIBNET_ARP_H + (2 * hln) + (2 * pln) + payload_s;
    h = 0;  /* ARP headers have no checksum */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ARP_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&arp_hdr, 0, sizeof(arp_hdr));
    arp_hdr.ar_hrd = htons(hrd);       /* hardware address type */
    arp_hdr.ar_pro = htons(pro);       /* protocol address type */
    arp_hdr.ar_hln = hln;              /* hardware address length */
    arp_hdr.ar_pln = pln;              /* protocol address length */
    arp_hdr.ar_op  = htons(op);        /* opcode command */

    n = libnet_pblock_append(l, p, (uint8_t *)&arp_hdr, LIBNET_ARP_H);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad; 
    }
    n = libnet_pblock_append(l, p, sha, hln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }
    n = libnet_pblock_append(l, p, spa, pln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }
    n = libnet_pblock_append(l, p, tha, hln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    } 
    n = libnet_pblock_append(l, p, tpa, pln);
    if (n == -1)
    {
        /* err msg set in libnet_pblock_append() */
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ARP_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_autobuild_arp(uint16_t op, const uint8_t *sha, const uint8_t *spa, const uint8_t *tha,
uint8_t *tpa, libnet_t *l)
{
    u_short hrd;
    
    switch (l->link_type)
    {
        case 1: /* DLT_EN10MB */
            hrd = ARPHRD_ETHER;
            break;
        case 6: /* DLT_IEEE802 */
            hrd = ARPHRD_IEEE802; 
            break;
        default:
            hrd = 0;
            snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                    "%s(): unsupported link type\n", __func__);
            return (-1);
        /* add other link-layers */
    }
    
    return (libnet_build_arp(
        hrd,                                    /* hardware addr */
        ETHERTYPE_IP,                           /* protocol addr */
        6,                                      /* hardware addr size */
        4,                                      /* protocol addr size */
        op,                                     /* operation type */
        sha,                                    /* sender hardware addr */
        spa,                                    /* sender protocol addr */
        tha,                                    /* target hardware addr */
        tpa,                                    /* target protocol addr */
        NULL,                                   /* payload */
        0,                                      /* payload size */
        l,                                      /* libnet context */
        0));                                    /* libnet id */
}

/* EOF */
