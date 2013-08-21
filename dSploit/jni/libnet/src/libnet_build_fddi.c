/*
 *  libnet
 *  libnet_build_fddi.c - Fiber Distributed Data Interface packet assembler
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
libnet_build_fddi(uint8_t fc, const uint8_t *dst, const uint8_t *src, uint8_t dsap,
uint8_t ssap, uint8_t cf, const uint8_t *org, uint16_t type, const uint8_t *payload,
uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    uint16_t protocol_type;
    libnet_pblock_t *p;
    struct libnet_fddi_hdr fddi_hdr;

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

    n = LIBNET_FDDI_H + payload_s;
    h = 0;
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_FDDI_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&fddi_hdr, 0, sizeof(fddi_hdr));
    fddi_hdr.fddi_frame_control     = fc;            /* Class/Format/Priority */
    memcpy(fddi_hdr.fddi_dhost, dst, FDDI_ADDR_LEN);  /* dst fddi address */
    memcpy(fddi_hdr.fddi_shost, src, FDDI_ADDR_LEN);  /* source fddi address */
    fddi_hdr.fddi_llc_dsap          = dsap;           /* */
    fddi_hdr.fddi_llc_ssap          = ssap;           /* */
    fddi_hdr.fddi_llc_control_field = cf;             /* Class/Format/Priority */
    memcpy(&fddi_hdr.fddi_llc_org_code, org, LIBNET_ORG_CODE_SIZE); 

    /* Deal with unaligned int16_t for type */
    protocol_type = htons(type);
    memcpy(&fddi_hdr.fddi_type, &protocol_type, sizeof(int16_t));   /* Protocol Type */

    n = libnet_pblock_append(l, p, (uint8_t *)&fddi_hdr, LIBNET_FDDI_H);
    if (n == -1)
    {
        goto bad;
    }
 
    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_FDDI_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_autobuild_fddi(uint8_t fc, const uint8_t *dst, uint8_t dsap, uint8_t ssap,
uint8_t cf, const uint8_t *org, uint16_t type, libnet_t *l)
{
    uint32_t n, h;
    uint16_t protocol_type;
    struct libnet_fddi_addr *src;
    libnet_pblock_t *p;
    libnet_ptag_t ptag;
    struct libnet_fddi_hdr fddi_hdr;

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

    n = LIBNET_FDDI_H;
    h = 0;
    ptag = LIBNET_PTAG_INITIALIZER;

    /* FDDI and Ethernet have the same address size - so just typecast */
    src = (struct libnet_fddi_addr *) libnet_get_hwaddr(l);
    if (src == NULL)
    {
        /* err msg set in libnet_get_hwaddr() */
        return (-1);
    }

    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_FDDI_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&fddi_hdr, 0, sizeof(fddi_hdr));
    fddi_hdr.fddi_frame_control     = fc;            /* Class/Format/Priority */
    memcpy(fddi_hdr.fddi_dhost, dst, FDDI_ADDR_LEN); /* destination fddi addr */
    memcpy(fddi_hdr.fddi_shost, src, FDDI_ADDR_LEN); /* source fddi addr */
    fddi_hdr.fddi_llc_dsap          = dsap;          /* */
    fddi_hdr.fddi_llc_ssap          = ssap;          /* */
    fddi_hdr.fddi_llc_control_field = cf;            /* Class/Format/Priority */
    memcpy(&fddi_hdr.fddi_llc_org_code, org, LIBNET_ORG_CODE_SIZE); 

    /* Deal with unaligned int16_t for type */
    protocol_type = htons(type);
    memcpy(&fddi_hdr.fddi_type, &protocol_type, sizeof(int16_t));

    n = libnet_pblock_append(l, p, (uint8_t *)&fddi_hdr, LIBNET_FDDI_H);
    if (n == -1)
    {
        goto bad;
    }

    return (libnet_pblock_update(l, p, h, LIBNET_PBLOCK_FDDI_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1); 
}
/* EOF */
