/*
 *  $Id: libnet_build_ethernet.c,v 1.9 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_ethernet.c - ethernet packet assembler
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
libnet_build_ethernet(const uint8_t *dst, const uint8_t *src, uint16_t type, 
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_ethernet_hdr eth_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* sanity check injection type if we're not in advanced mode */
    if (l->injection_type != LIBNET_LINK &&
            !(((l->injection_type) & LIBNET_ADV_MASK)))
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "%s(): called with non-link layer wire injection primitive",
                    __func__);
        p = NULL;
        goto bad;
    }

    n = LIBNET_ETH_H + payload_s;
    h = 0;
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ETH_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&eth_hdr, 0, sizeof(eth_hdr));
    memcpy(eth_hdr.ether_dhost, dst, ETHER_ADDR_LEN);  /* destination address */
    memcpy(eth_hdr.ether_shost, src, ETHER_ADDR_LEN);  /* source address */
    eth_hdr.ether_type = htons(type);                  /* packet type */

    n = libnet_pblock_append(l, p, (uint8_t *)&eth_hdr, LIBNET_ETH_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ETH_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_autobuild_ethernet(const uint8_t *dst, uint16_t type, libnet_t *l)
{
    uint32_t n, h;
    struct libnet_ether_addr *src;
    libnet_pblock_t *p;
    libnet_ptag_t ptag;
    struct libnet_ethernet_hdr eth_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* sanity check injection type if we're not in advanced mode */
    if (l->injection_type != LIBNET_LINK &&
            !(((l->injection_type) & LIBNET_ADV_MASK)))
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
            "libnet_autobuild_ethernet() called with non-link layer wire"
            " injection primitive");
        p = NULL;
        goto bad;
    }

    n = LIBNET_ETH_H;
    h = 0;
    ptag = LIBNET_PTAG_INITIALIZER;
    src = libnet_get_hwaddr(l);
    if (src == NULL)
    {
        /* err msg set in libnet_get_hwaddr() */
        return (-1);
    }

    /*
     *  Create a new pblock. 
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_ETH_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&eth_hdr, 0, sizeof(eth_hdr));
    memcpy(eth_hdr.ether_dhost, dst, ETHER_ADDR_LEN);  /* destination address */
    memcpy(eth_hdr.ether_shost, src, ETHER_ADDR_LEN);  /* source address */
    eth_hdr.ether_type = htons(type);                  /* packet type */

    n = libnet_pblock_append(l, p, (uint8_t *)&eth_hdr, LIBNET_ETH_H);
    if (n == -1)
    {
        goto bad;
    }

    return (libnet_pblock_update(l, p, h, LIBNET_PBLOCK_ETH_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1); 
}
/* EOF */
