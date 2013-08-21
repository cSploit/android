/*
 *  $Id: libnet_build_ipsec.c,v 1.12 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_ipsec.c - IP packet assembler
 *
 *  Copyright (c) 1998 - 2004 Mike D. Schiffman <mike@infonexus.com>
 *  Copyright (c) 2002 Jose Nazario <jose@crimelabs.net>
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
libnet_build_ipsec_esp_hdr(uint32_t spi, uint32_t seq, uint32_t iv,
const uint8_t *payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_esp_hdr esp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPSEC_ESP_HDR_H + payload_s;/* size of memory block */
    h = 0;

    memset(&esp_hdr, 0, sizeof(esp_hdr));
    esp_hdr.esp_spi = htonl(spi);      /* SPI */
    esp_hdr.esp_seq = htonl(seq);      /* ESP sequence number */
    esp_hdr.esp_iv = htonl(iv);        /* initialization vector */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPSEC_ESP_HDR_H);
    if (p == NULL)
    {
        return (-1);
    }

    n = libnet_pblock_append(l, p, (uint8_t *)&esp_hdr, LIBNET_IPSEC_ESP_HDR_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPSEC_ESP_HDR_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ipsec_esp_ftr(uint8_t len, uint8_t nh, int8_t *auth,
            const uint8_t *payload, uint32_t payload_s, libnet_t *l,
            libnet_ptag_t ptag)
{
    /* XXX we need to know the size of auth */
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_esp_ftr esp_ftr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPSEC_ESP_FTR_H + payload_s;/* size of memory block */
    h = 0;

    memset(&esp_ftr, 0, sizeof(esp_ftr));
    esp_ftr.esp_pad_len = len;      /* pad length */
    esp_ftr.esp_nh = nh;  /* next header pointer */
    esp_ftr.esp_auth = auth;        /* authentication data */

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPSEC_ESP_FTR_H);
    if (p == NULL)
    {
        return (-1);
    }

    n = libnet_pblock_append(l, p, (uint8_t *)&esp_ftr, LIBNET_IPSEC_ESP_FTR_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPSEC_ESP_FTR_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


libnet_ptag_t
libnet_build_ipsec_ah(uint8_t nh, uint8_t len, uint16_t res,
uint32_t spi, uint32_t seq, uint32_t auth, const uint8_t *payload,
uint32_t payload_s,  libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_ah_hdr ah_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_IPSEC_AH_H + payload_s;/* size of memory block */
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_IPSEC_AH_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&ah_hdr, 0, sizeof(ah_hdr));
    ah_hdr.ah_nh = nh;       /* next header */
    ah_hdr.ah_len = len;               /* length */
    ah_hdr.ah_res = (res ? htons(res) : 0);
    ah_hdr.ah_spi = htonl(spi);        /* SPI */
    ah_hdr.ah_seq = htonl(seq);        /* AH sequence number */
    ah_hdr.ah_auth = htonl(auth);      /* authentication data */

    n = libnet_pblock_append(l, p, (uint8_t *)&ah_hdr, LIBNET_IPSEC_AH_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_IPSEC_AH_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
