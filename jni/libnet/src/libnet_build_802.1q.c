/*
 *  $Id: libnet_build_802.1q.c,v 1.11 2004/04/13 17:32:28 mike Exp $
 *
 *  libnet
 *  libnet_build_802.1q.c - 802.1q packet assembler
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
libnet_build_802_1q(const uint8_t *dst, const uint8_t *src, uint16_t tpi, 
uint8_t priority, uint8_t cfi, uint16_t vlan_id, uint16_t len_proto,
const uint8_t* payload, uint32_t payload_s, libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p;
    struct libnet_802_1q_hdr _802_1q_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_802_1Q_H + payload_s;
    h = 0;
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_802_1Q_H);
    if (p == NULL)
    {
        return (-1);
    }
	
    memset(&_802_1q_hdr, 0, sizeof(_802_1q_hdr));
    memcpy(_802_1q_hdr.vlan_dhost, dst, ETHER_ADDR_LEN);
    memcpy(_802_1q_hdr.vlan_shost, src, ETHER_ADDR_LEN);
    _802_1q_hdr.vlan_tpi = htons(tpi);
    _802_1q_hdr.vlan_priority_c_vid = htons((priority << 13) | (cfi << 12)
            | (vlan_id & LIBNET_802_1Q_VIDMASK));
    _802_1q_hdr.vlan_len = htons(len_proto);

    n = libnet_pblock_append(l, p, (uint8_t *)&_802_1q_hdr, LIBNET_802_1Q_H);
    if (n == (uint32_t)-1)
    {
        goto bad;
    }

    /* boilerplate payload sanity check / append macro */
    LIBNET_DO_PAYLOAD(l, p);
 
    /*
     *  The link offset is actually 4 bytes further into the header than
     *  before (the VLAN header adds this 4 bytes).  We need to update the
     *  link offset pointer. XXX - should we set this here?
     *  Probably not.  We should make pblock_coalesce check the pblock type
     *  and adjust accordingly.
     */
    l->link_offset += 4;
    return (ptag ? ptag : libnet_pblock_update(l, p, h,
            LIBNET_PBLOCK_802_1Q_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}


/* EOF */
