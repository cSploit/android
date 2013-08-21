/*
 *  $Id: libnet_build_rip.c,v 1.9 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_rip.c - RIP packet assembler
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
libnet_build_rip(uint8_t cmd, uint8_t version, uint16_t rd, uint16_t af,
uint16_t rt, uint32_t addr, uint32_t mask, uint32_t next_hop,
uint32_t metric, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_rip_hdr rip_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_RIP_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_RIP_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&rip_hdr, 0, sizeof(rip_hdr));
    rip_hdr.rip_cmd      = cmd;
    rip_hdr.rip_ver      = version;
    rip_hdr.rip_rd       = htons(rd);
    rip_hdr.rip_af       = htons(af);
    rip_hdr.rip_rt       = htons(rt);
    rip_hdr.rip_addr     = htonl(addr);
    rip_hdr.rip_mask     = htonl(mask);
    rip_hdr.rip_next_hop = htonl(next_hop);
    rip_hdr.rip_metric   = htonl(metric);

    n = libnet_pblock_append(l, p, (uint8_t *)&rip_hdr, LIBNET_RIP_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_RIP_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
