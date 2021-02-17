/*
 *  $Id: libnet_build_dhcp.c,v 1.10 2004/03/01 20:26:12 mike Exp $
 *
 *  libnet
 *  libnet_build_dhcp.c - DHCP/BOOTP packet assembler
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

#include "common.h"

libnet_ptag_t
libnet_build_dhcpv4(uint8_t opcode, uint8_t htype, uint8_t hlen, 
uint8_t hopcount, uint32_t xid, uint16_t secs, uint16_t flags,
uint32_t cip, uint32_t yip, uint32_t sip, uint32_t gip, const uint8_t *chaddr,
const char *sname, const char *file, const uint8_t *payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag)
{
    uint32_t n, h;
    libnet_pblock_t *p; 
    struct libnet_dhcpv4_hdr dhcp_hdr;

    if (l == NULL)
    { 
        return (-1);
    } 

    n = LIBNET_DHCPV4_H + payload_s;
    h = 0;          /* no checksum */
 
    /*
     *  Find the existing protocol block if a ptag is specified, or create
     *  a new one.
     */
    p = libnet_pblock_probe(l, ptag, n, LIBNET_PBLOCK_DHCPV4_H);
    if (p == NULL)
    {
        return (-1);
    }

    memset(&dhcp_hdr, 0, sizeof(dhcp_hdr));
    dhcp_hdr.dhcp_opcode    = opcode;
    dhcp_hdr.dhcp_htype     = htype;
    dhcp_hdr.dhcp_hlen      = hlen;
    dhcp_hdr.dhcp_hopcount  = hopcount;
    dhcp_hdr.dhcp_xid       = htonl(xid);
    dhcp_hdr.dhcp_secs      = htons(secs);
    dhcp_hdr.dhcp_flags     = htons(flags);
    dhcp_hdr.dhcp_cip       = htonl(cip);
    dhcp_hdr.dhcp_yip       = htonl(yip);
    dhcp_hdr.dhcp_sip       = htonl(sip);
    dhcp_hdr.dhcp_gip       = htonl(gip);

    if (chaddr)
    {
        size_t n = sizeof (dhcp_hdr.dhcp_chaddr);
        if (hlen < n)
            n = hlen;
        memcpy(dhcp_hdr.dhcp_chaddr, chaddr, n);
    }
    if (sname)
    { 
        strncpy(dhcp_hdr.dhcp_sname, sname, sizeof (dhcp_hdr.dhcp_sname) - 1);
    }
    if (file)
    {
        strncpy(dhcp_hdr.dhcp_file, file, sizeof (dhcp_hdr.dhcp_file) - 1);
    }
    dhcp_hdr.dhcp_magic = htonl(DHCP_MAGIC);

    n = libnet_pblock_append(l, p, (uint8_t *)&dhcp_hdr, LIBNET_DHCPV4_H);
    if (n == -1)
    {
        goto bad;
    }

    if (payload_s && !payload)
    {
         snprintf(l->err_buf, LIBNET_ERRBUF_SIZE,
                 "%s(): payload inconsistency", __func__);
        goto bad;
    }
 
    if (payload_s)
    {
        n = libnet_pblock_append(l, p, payload, payload_s);
        if (n == -1)
        {
            goto bad;
        }
    }
 
    return (ptag ? ptag : libnet_pblock_update(l, p, h, 
            LIBNET_PBLOCK_DHCPV4_H));
bad:
    libnet_pblock_delete(l, p);
    return (-1);
}

libnet_ptag_t
libnet_build_bootpv4(uint8_t opcode, uint8_t htype, uint8_t hlen,
uint8_t hopcount, uint32_t xid, uint16_t secs, uint16_t flags,
uint32_t cip, uint32_t yip, uint32_t sip, uint32_t gip, const uint8_t *chaddr,
const char *sname, const char *file, const uint8_t *payload, uint32_t payload_s,
libnet_t *l, libnet_ptag_t ptag)
{
    return (libnet_build_dhcpv4(opcode, htype, hlen, hopcount, xid, secs,
        flags, cip, yip, sip, gip, chaddr, sname, file, payload, payload_s, 
        l, ptag));
}

/* EOF */
