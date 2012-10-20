/*
 *  libnet
 *  libnet_build_token_ring.c - Token Ring (802.5)  packet assembler
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *                            Jason Damron      <jsdamron@hushmail.com>
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
libnet_build_token_ring(uint8_t ac, uint8_t fc, const uint8_t *dst, const uint8_t *src, 
uint8_t dsap, uint8_t ssap, uint8_t cf, const uint8_t *org, uint16_t type,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_token_ring_hdr token_ring_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* sanity check injection type if we're not in advanced mode */
    if (l->injection_type != LIBNET_LINK &&
            !(((l->injection_type) & LIBNET_ADV_MASK)))
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): called with non-link layer wire injection primitive\n",
                __func__);
        p = NULL;
        goto bad;
    }

    n = LIBNET_TOKEN_RING_H + payload_s;
    h = 0;
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_TOKEN_RING_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&token_ring_hdr, 0, sizeof(token_ring_hdr));
    token_ring_hdr.token_ring_access_control    = ac;
    token_ring_hdr.token_ring_frame_control     = fc;
    memcpy(token_ring_hdr.token_ring_dhost, dst, TOKEN_RING_ADDR_LEN);
    memcpy(token_ring_hdr.token_ring_shost, src, TOKEN_RING_ADDR_LEN);
    token_ring_hdr.token_ring_llc_dsap          = dsap;
    token_ring_hdr.token_ring_llc_ssap          = ssap;
    token_ring_hdr.token_ring_llc_control_field = cf;
    memcpy(&token_ring_hdr.token_ring_llc_org_code, org, LIBNET_ORG_CODE_SIZE); 
    token_ring_hdr.token_ring_type              = htons(type);

    n = libnet_pblock_append(l, p, (uint8_t *)&token_ring_hdr, 
            LIBNET_TOKEN_RING_H);
    if (n == -1)
    {
        goto bad;
    }
 
    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_TOKEN_RING_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_autobuild_token_ring(uint8_t ac, uint8_t fc, const uint8_t *dst, 
uint8_t dsap, uint8_t ssap, uint8_t cf, const uint8_t *org, uint16_t type, 
libnet_t *l)
{
    uint32_t n, h;
    struct libnet_token_ring_addr *src;
    libnet_pblock_t *p;
    libnet_ptag_t ptag;
    struct libnet_token_ring_hdr token_ring_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    /* sanity check injection type if we're not in advanced mode */
    if (l->injection_type != LIBNET_LINK &&
            !(((l->injection_type) & LIBNET_ADV_MASK)))
    {
        snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                "%s(): called with non-link layer wire injection primitive\n",
                __func__);         
        p = NULL;
        goto bad;
    }

    n = LIBNET_TOKEN_RING_H;
    h = 0;
    ptag = LIBNET_PTAG_INITIALIZER;

    /* Token Ring and Ethernet have the same address size - so just typecast */
    src = (struct libnet_token_ring_addr *) libnet_get_hwaddr(l);
    if (src == NULL)
    {
        /* err msg set in libnet_get_hwaddr() */
        return (-1);
    }

    /*
     *  Create a new pblock. 
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_TOKEN_RING_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&token_ring_hdr, 0, sizeof(token_ring_hdr));
    token_ring_hdr.token_ring_access_control    = ac;
    token_ring_hdr.token_ring_frame_control     = fc;
    memcpy(token_ring_hdr.token_ring_dhost, dst, TOKEN_RING_ADDR_LEN);
    memcpy(token_ring_hdr.token_ring_shost, src, TOKEN_RING_ADDR_LEN);
    token_ring_hdr.token_ring_llc_dsap          = dsap;
    token_ring_hdr.token_ring_llc_ssap          = ssap;
    token_ring_hdr.token_ring_llc_control_field = cf;
    memcpy(&token_ring_hdr.token_ring_llc_org_code, org, LIBNET_ORG_CODE_SIZE); 
    token_ring_hdr.token_ring_type              = htons(type);


    n = libnet_pblock_append(l, p, (uint8_t *)&token_ring_hdr, 
            LIBNET_TOKEN_RING_H);
    if (n == -1)
    {
        goto bad;
    }

    return (libnet_pblock_update(l, p, h, LIBNET_PBLOCK_TOKEN_RING_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1); 
}
/* EOF */
