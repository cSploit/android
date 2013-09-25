/*
 *  $Id: libnet_build_mpls.c,v 1.10 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_mpls.c - MPLS packet assembler
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
libnet_build_mpls(uint32_t label, uint8_t experimental, uint8_t bos, 
uint8_t ttl, const uint8_t *payload, uint32_t payload_s, libnet_t *l,
libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_mpls_hdr mpls_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_MPLS_H + payload_s;
    h = 0;

    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_MPLS_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&mpls_hdr, 0, sizeof(mpls_hdr));
    mpls_hdr.mpls_les = htonl((((label & 0x000fffff) << 12) |
                              ((experimental & 0x07) <<  9) |
                              ((bos & 0x01)          <<  8) |
                              ((ttl & 0xff))));

    n = libnet_pblock_append(l, p, (uint8_t *)&mpls_hdr, LIBNET_MPLS_H);
    if (n == -1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);

    /*
     *  The link offset is actually 4 bytes further into the header than
     *  before (the MPLS header adds this 4 bytes).  We need to update the
     *  link offset pointer. XXX - should we set this here?
     *  Probably not.  We should make pblock_coalesce check the pblock type
     *  and adjust accordingly.
     */
    l->link_offset += 4;

    return (ptag ? ptag : libnet_pblock_update(l, p, h, LIBNET_PBLOCK_MPLS_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

/* EOF */
